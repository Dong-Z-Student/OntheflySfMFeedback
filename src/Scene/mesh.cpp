#include "../Base/TimerProfiler.h"
#include "dynamic_graph_cut.h"
#include "mesh.h"
#include "PathPlanner.h"
#include "rawio.h"
#include "ray_tetra_host.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include <string>
#include <tbb/tbb.h>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {
	struct EdgeHash {
		size_t operator()(const std::pair<size_t, size_t>& e) const noexcept {
			size_t h1 = std::hash<size_t>{}(e.first);
			size_t h2 = std::hash<size_t>{}(e.second);
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
		}
	};

	std::vector<Eigen::Vector4f> generateColorsWithQuantiles(
		const std::vector<double>& weights,
		std::vector<double>& quantiles_out)
	{
		const Eigen::Vector4f RED(0.70f, 0.0f, 0.13f, 1.0f);
		const Eigen::Vector4f WHITE(1.0f, 1.0f, 1.0f, 1.0f);
		const Eigen::Vector4f BLUE(0.23f, 0.29f, 0.75f, 1.0f);

		const size_t n = weights.size();
		quantiles_out.clear();

		if (n == 0) {
			return {};
		}

		std::vector<Eigen::Vector4f> colors(n);
		quantiles_out.resize(n);

		if (n == 1) {
			quantiles_out[0] = 0.0;
			colors[0] = BLUE;
			colors[0][3] = 1.0f;
			return colors;
		}

		std::vector<size_t> indices(n);
		std::iota(indices.begin(), indices.end(), 0);

		std::sort(indices.begin(), indices.end(),
			[&weights](size_t a, size_t b) {
				return weights[a] < weights[b];
			});

		const double inv = 1.0 / static_cast<double>(n - 1);

		size_t i = 0;
		while (i < n) {
			size_t j = i + 1;
			const double v = weights[indices[i]];

			while (j < n && weights[indices[j]] == v) {
				++j;
			}

			const double q = static_cast<double>(j - 1) * inv;
			for (size_t k = i; k < j; ++k) {
				quantiles_out[indices[k]] = q;
			}

			i = j;
		}

		for (size_t k = 0; k < n; ++k) {
			const double q = quantiles_out[k];
			Eigen::Vector4f color;

			if (q < 0.5) {
				const float t = static_cast<float>(q * 2.0); // q / 0.5
				color = (1.0f - t) * BLUE + t * WHITE;
			}
			else {
				const float t = static_cast<float>((q - 0.5) * 2.0); // (q - 0.5) / 0.5
				color = (1.0f - t) * WHITE + t * RED;
			}

			color[3] = 1.0f;
			colors[k] = color;
		}

		return colors;
	}
}

static TimerProfiler profiler;

double ComputeCosFacetCellAngle(const Delaunay& triangulation, const Delaunay::Facet& facet)
{
	if (triangulation.is_infinite(facet.first)) {
		return 1.0;
	}

	const K::Triangle_3 triangle = triangulation.triangle(facet);

	const K::Vector_3 facet_normal =
		CGAL::cross_product(triangle[1] - triangle[0], triangle[2] - triangle[0]);
	const double facet_normal_length_squared = facet_normal.squared_length();
	if (facet_normal_length_squared == 0.0) {
		return 0.5;
	}
	auto cell = facet.first;
	const K::Point_3 cc = CGAL::circumcenter(
		cell->vertex(0)->point(),
		cell->vertex(1)->point(),
		cell->vertex(2)->point(),
		cell->vertex(3)->point()
	);
	const K::Vector_3 co_tangent = cc - triangle[0];
	const float co_tangent_length_squared = co_tangent.squared_length();
	if (co_tangent_length_squared == 0.0) {
		return 0.5;
	}

	return (facet_normal * co_tangent) /
		std::sqrt(facet_normal_length_squared * co_tangent_length_squared);
}


RawData CopyReconstruction(Reconstruction& model)
{
	RawData raw;
	rawio::CopyFromReconstruction(model, raw);
	return raw;
}

bool GenerateMeshModel(
	std::shared_ptr<RawData>& pre_raw_ptr,
	std::shared_ptr<DelaunayData>& delaunayData_ptr,
	std::shared_ptr<RawData>& raw_ptr,
	RenderData& renderData
) {
	RawData& raw = *raw_ptr;
	rawio::FilterScene(raw);

	bool isFirstBuild = pre_raw_ptr == nullptr;
	if (isFirstBuild)
	{
		delaunayData_ptr = std::make_shared<DelaunayData>();
		if (!delaunay::Build(raw, *delaunayData_ptr))
		{
			std::cerr << "Build Delaunay failed." << std::endl;
			return false;
		}
	}
	else
	{
		if (!delaunay::Update(raw, *pre_raw_ptr, *delaunayData_ptr))
		{
			std::cerr << "Build Delaunay failed." << std::endl;
			return false;
		}
	}

	//delaunayData_ptr = std::make_shared<DelaunayData>();
	//DelaunayData& delaunayData = *delaunayData_ptr;
	//delaunay::Build(raw, delaunayData);

	// =============== 权重图计算部分 ===============
	DelaunayData& delaunayData = *delaunayData_ptr;
	CellGraphData cell_graph_data;
	Delaunay& dt = delaunayData.dt;
	cell_graph_data.reserve(dt.number_of_cells());

	for (auto cit = dt.all_cells_begin(); cit != dt.all_cells_end(); ++cit) {
		Cell_handle c = cit;
		cell_graph_data.emplace(c->info(), DelaunayCellData(c->info()));
	}

	const double epsilon = 1e-3;
	std::vector<RayTetra::DirectionRay> direction_rays(raw.rays.size());
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, raw.rays.size(), 10240),
		[&](const tbb::blocked_range<size_t>& range) {
			for (size_t i = range.begin(); i < range.end(); ++i) {
				const auto& startV = raw.images[raw.rays[i].raw_image_id];
				const auto& endV = raw.points[raw.rays[i].raw_point_id];
				auto& dir_ray = direction_rays[i];
				dir_ray.start = K::Point_3(startV.cx, startV.cy, startV.cz);
				dir_ray.end = K::Point_3(endV.x, endV.y, endV.z);
				K::Vector_3 direction(dir_ray.start, dir_ray.end);
				double length = std::sqrt(CGAL::to_double(direction.squared_length()));
				if (length > 1e-12) {
					dir_ray.viewing_ray = K::Ray_3(dir_ray.start, dir_ray.end);
					dir_ray.normalized_direction = direction / length;
					dir_ray.nearEndPoint = dir_ray.end - dir_ray.normalized_direction * epsilon;
					dir_ray.farEndPoint = dir_ray.end + dir_ray.normalized_direction * epsilon;
				}
				else {
					dir_ray.viewing_ray = K::Ray_3(dir_ray.start, dir_ray.end);
					dir_ray.normalized_direction = K::Vector_3(0.0, 0.0, 0.0);
					dir_ray.nearEndPoint = dir_ray.end;
					dir_ray.farEndPoint = dir_ray.end;
				}
			}
		});

	std::unordered_map<uint64_t, RayTetra::RayTraceResult> trace_results;
	std::string errMsg;
	RayTetra::BuildAndTraceRays(delaunayData, direction_rays, trace_results, errMsg);

	tbb::enumerable_thread_specific<CellGraphData> thread_local_data;
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, raw.rays.size(), 10240),
		[&](const tbb::blocked_range<size_t>& range) {
			auto& local_cell_graph = thread_local_data.local();

			for (size_t i = range.begin(); i < range.end(); ++i) {
				const auto& dir_ray = direction_rays[i];

				bool flag = false;
				auto fid = trace_results.find(i);
				if (fid != trace_results.end()) {
					const auto& res = fid->second;
					if (res.status == 0) {
						flag = true;
					}
				}
				if (!flag) continue;

				const auto& trace = trace_results[i];
				local_cell_graph[trace.cell_ids.front()].source_weight += alphafree;

				for (int j = 0; j < static_cast<int>(trace.cell_ids.size()) - 1; j++) {
					local_cell_graph[trace.cell_ids[j]]
						.edge_weights[trace.next_idx[j]] += alphafree;
				}

				{
					const Delaunay::Cell_handle behind_point_cell =
						dt.locate(dir_ray.farEndPoint);

					int behind_neighbor_idx = -1;
					double behind_distance_squared = 0.0;

					for (int neighbor_idx = 0; neighbor_idx < 4; ++neighbor_idx) {
						const K::Triangle_3 triangle =
							dt.triangle(behind_point_cell, neighbor_idx);

						K::Point_3 inter_point;
						if (CGAL::assign(inter_point,
							CGAL::intersection(dir_ray.viewing_ray, triangle))) {
							const double distance_squared =
								(inter_point - dir_ray.end).squared_length();
							if (distance_squared > behind_distance_squared) {
								behind_distance_squared = distance_squared;
								behind_neighbor_idx = neighbor_idx;
							}
						}
					}

					if (behind_neighbor_idx >= 0) {
						local_cell_graph[behind_point_cell->info()]
							.edge_weights[behind_neighbor_idx] += alphavisual;

						const auto& inside_cell =
							behind_point_cell->neighbor(behind_neighbor_idx);
						local_cell_graph[inside_cell->info()].sink_weight += alphaoccupied;
					}
				}
			}
		});

	for (const auto& local_data : thread_local_data) {
		for (const auto& [cell_handle, cell_data] : local_data) {
			auto& target = cell_graph_data[cell_handle];
			target.source_weight += cell_data.source_weight;
			target.sink_weight += cell_data.sink_weight;
			for (size_t j = 0; j < cell_data.edge_weights.size(); ++j) {
				target.edge_weights[j] += cell_data.edge_weights[j];
			}
		}
	}

	int num_nodes = dt.number_of_cells() + 2;
	DynamicGraphCut gc(num_nodes, 0);

	gc.setTerminalWeights(0, std::numeric_limits<double>::max(), 0.0);
	gc.setTerminalWeights(1, 0.0, std::numeric_limits<double>::max());

	for (auto& cell_data : cell_graph_data) {
		const size_t nodeid = delaunayData.getCellIdx(cell_data.second.index);
		gc.setTerminalWeights(
			nodeid,
			cell_data.second.source_weight,
			cell_data.second.sink_weight);

		for (int i = 0; i < 4; ++i) {
			Cell_handle ch = delaunayData.cellIdToCell[cell_data.first];
			const Delaunay::Facet facet = std::make_pair(ch, i);

			const Delaunay::Facet mirror_facet = dt.mirror_facet(facet);
			const auto& mirror_cell_data = cell_graph_data.at(mirror_facet.first->info());

			if (cell_data.second.index > mirror_cell_data.index) {
				continue;
			}

			const double edge_shape_weight =
				1.0 - std::min(
					ComputeCosFacetCellAngle(dt, facet),
					ComputeCosFacetCellAngle(dt, mirror_facet));

			const float forward_edge_weight =
				cell_data.second.edge_weights[facet.second] + edge_shape_weight;
			const float backward_edge_weight =
				mirror_cell_data.edge_weights[mirror_facet.second] + edge_shape_weight;

			const size_t mirror_nodeid = delaunayData.getCellIdx(mirror_cell_data.index);
			gc.addEdge(nodeid, mirror_nodeid, forward_edge_weight, backward_edge_weight);
		}
	}

	profiler.Start("GraphCut");
	gc.computeMaxFlow();
	profiler.Stop("GraphCut");

	PlyMesh mesh;
	std::unordered_set<Vertex_handle> surface_vertices;
	std::vector<Delaunay::Facet> surface_facets;
	surface_vertices.reserve(dt.number_of_vertices());
	surface_facets.reserve(dt.number_of_finite_cells() * 4);

	for (auto it = dt.finite_facets_begin();
		it != dt.finite_facets_end();
		++it) {

		if (dt.is_infinite(it->first->neighbor(it->second))) {
			continue;
		}

		const auto& cell_data = cell_graph_data.at(it->first->info());
		const auto& mirror_cell_data =
			cell_graph_data.at(it->first->neighbor(it->second)->info());

		const size_t nodeid = delaunayData.getCellIdx(cell_data.index);
		const size_t mirror_nodeid = delaunayData.getCellIdx(mirror_cell_data.index);
		const bool cell_is_source = gc.getSegment(nodeid) == DynamicGraphCut::GraphType::SOURCE;
		const bool mirror_cell_is_source =
			gc.getSegment(mirror_nodeid) == DynamicGraphCut::GraphType::SOURCE;

		if (cell_is_source == mirror_cell_is_source) {
			continue;
		}

		for (int i = 0; i < 3; ++i) {
			const auto& vertex =
				it->first->vertex(dt.vertex_triple_index(it->second, i));
			surface_vertices.insert(vertex);
		}

		if (cell_is_source) {
			surface_facets.push_back(*it);
		}
		else {
			surface_facets.push_back(dt.mirror_facet(*it));
		}
	}

	std::unordered_map<Delaunay::Vertex_handle, size_t>
		surface_vertex_indices;
	surface_vertex_indices.reserve(surface_vertices.size());
	mesh.vertices.reserve(surface_vertices.size());
	for (const auto& vertex : surface_vertices) {
		mesh.vertices.emplace_back(
			vertex->point().x(), vertex->point().y(), vertex->point().z(), vertex->info());
		surface_vertex_indices.emplace(vertex, surface_vertex_indices.size());
	}
	mesh.faces.reserve(surface_facets.size());
	for (size_t i = 0; i < surface_facets.size(); ++i) {
		const auto& facet = surface_facets[i];
		mesh.faces.emplace_back(
			surface_vertex_indices.at(facet.first->vertex(
				dt.vertex_triple_index(facet.second, 0))),
			surface_vertex_indices.at(facet.first->vertex(
				dt.vertex_triple_index(facet.second, 1))),
			surface_vertex_indices.at(facet.first->vertex(
				dt.vertex_triple_index(facet.second, 2))));
	}

	std::vector<double> weights, quantiles;
	QualityAssessment(raw, mesh, weights);
	auto colors = generateColorsWithQuantiles(weights, quantiles);
	GenerateRenderData(mesh, colors, renderData);
	GeneratePredictivePath(raw, mesh, weights, quantiles, renderData);
	return true;
}

void QualityAssessment(const RawData& raw, PlyMesh& mesh, std::vector<double>& weights)
{
	std::vector<double> all_img_track;
	std::vector<double> all_reproj_error;
	const size_t face_count = mesh.faces.size();
	all_img_track.reserve(face_count);
	all_reproj_error.reserve(face_count);
	weights.reserve(face_count);

	for (auto& f : mesh.faces) {
		auto track1 = raw.points.at(mesh.vertices[f.vertex_idx1].point3D_idx).track;
		auto track2 = raw.points.at(mesh.vertices[f.vertex_idx2].point3D_idx).track;
		auto track3 = raw.points.at(mesh.vertices[f.vertex_idx3].point3D_idx).track;
		f.img_track = static_cast<double>(track1 + track2 + track3) / 3.0;

		auto error1 = raw.points.at(mesh.vertices[f.vertex_idx1].point3D_idx).error;
		auto error2 = raw.points.at(mesh.vertices[f.vertex_idx2].point3D_idx).error;
		auto error3 = raw.points.at(mesh.vertices[f.vertex_idx3].point3D_idx).error;
		f.reprojection_error = (error1 + error2 + error3) / 3.0;

		all_img_track.push_back(f.img_track);
		all_reproj_error.push_back(f.reprojection_error);
	}

	double img_track_min = 0.0, img_track_max = 0.0;
	double reproj_error_min = 0.0, reproj_error_max = 0.0;

	if (!all_img_track.empty()) {
		std::sort(all_img_track.begin(), all_img_track.end());
		size_t min_idx = static_cast<size_t>(all_img_track.size() * 0.05);
		size_t max_idx = std::min(static_cast<size_t>(all_img_track.size() * 0.95), all_img_track.size() - 1);
		img_track_min = all_img_track[min_idx];
		img_track_max = all_img_track[max_idx];
	}

	if (!all_reproj_error.empty()) {
		std::sort(all_reproj_error.begin(), all_reproj_error.end());
		size_t min_idx = static_cast<size_t>(all_reproj_error.size() * 0.05);
		size_t max_idx = std::min(static_cast<size_t>(all_reproj_error.size() * 0.95), all_reproj_error.size() - 1);
		reproj_error_min = all_reproj_error[min_idx];
		reproj_error_max = all_reproj_error[max_idx];
	}

	double w_track = 0.8;
	double w_error = 0.2;

	for (auto& f : mesh.faces) {
		double range_track = img_track_max - img_track_min;
		double range_error = reproj_error_max - reproj_error_min;

		if (range_track <= 0.0) range_track = 1.0;
		if (range_error <= 0.0) range_error = 1.0;

		double normalized_track = (f.img_track - img_track_min) / range_track;
		double normalized_error = (reproj_error_max - f.reprojection_error) / range_error;

		normalized_track = std::max(0.0, std::min(1.0, normalized_track));
		normalized_error = std::max(0.0, std::min(1.0, normalized_error));

		f.Qtotal = w_track * normalized_track +
			w_error * normalized_error;

		weights.push_back(f.Qtotal);
	}
}

void GenerateRenderData(const CMesh& mesh, RenderData& renderData)
{
	renderData.mesh_data.clear();
	renderData.mesh_lines.clear();

	renderData.mesh_data.reserve(mesh.number_of_faces());
	renderData.mesh_lines.reserve(mesh.number_of_edges());

	constexpr float fr = 0.678f;
	constexpr float fg = 0.847f;
	constexpr float fb = 0.902f;
	constexpr float fa = 1.0f;

	constexpr float er = 0.0f;
	constexpr float eg = 0.7f;
	constexpr float eb = 0.0f;
	constexpr float ea = 1.0f;

	for (CMesh::Face_index f : mesh.faces())
	{
		const auto h0 = mesh.halfedge(f);
		const auto h1 = mesh.next(h0);
		const auto h2 = mesh.next(h1);

		const Point& p0 = mesh.point(mesh.target(h0));
		const Point& p1 = mesh.point(mesh.target(h1));
		const Point& p2 = mesh.point(mesh.target(h2));

		renderData.mesh_data.emplace_back(TrianglePainterData{
			static_cast<float>(p0.x()), static_cast<float>(p0.y()), static_cast<float>(p0.z()), fr, fg, fb, fa,
			static_cast<float>(p1.x()), static_cast<float>(p1.y()), static_cast<float>(p1.z()), fr, fg, fb, fa,
			static_cast<float>(p2.x()), static_cast<float>(p2.y()), static_cast<float>(p2.z()), fr, fg, fb, fa
			});
	}

	for (CMesh::Edge_index e : mesh.edges())
	{
		const auto h = mesh.halfedge(e);

		const Point& p0 = mesh.point(mesh.source(h));
		const Point& p1 = mesh.point(mesh.target(h));

		renderData.mesh_lines.emplace_back(LinePainterData{
			static_cast<float>(p0.x()), static_cast<float>(p0.y()), static_cast<float>(p0.z()), er, eg, eb, ea,
			static_cast<float>(p1.x()), static_cast<float>(p1.y()), static_cast<float>(p1.z()), er, eg, eb, ea
			});
	}
}

void GenerateRenderData(const PlyMesh& mesh,
	const std::vector<Eigen::Vector4f>& face_colors,
	RenderData& renderData) {
	using TriangleElement = std::tuple<image_t, point2D_t, point2D_t, point2D_t>;
	const auto& vertices = mesh.vertices;
	const auto& faces = mesh.faces;

	renderData.mesh_data.reserve(faces.size());
	renderData.mesh_lines.reserve(faces.size() * 3);

	std::unordered_set<std::pair<size_t, size_t>, EdgeHash> drawnEdges;
	drawnEdges.reserve(faces.size() * 3);

	constexpr float fill_r = 0.678f;
	constexpr float fill_g = 0.847f;
	constexpr float fill_b = 0.902f;
	constexpr float fill_a = 1.0f;

	constexpr float line_r = 0.0f;
	constexpr float line_g = 0.7f;
	constexpr float line_b = 0.0f;
	constexpr float line_a = 1.0f;

	for (size_t idx = 0; idx < faces.size(); ++idx) {
		const auto& f = faces[idx];
		const auto& v0 = vertices[f.vertex_idx1];
		const auto& v1 = vertices[f.vertex_idx2];
		const auto& v2 = vertices[f.vertex_idx3];

		renderData.mesh_data.emplace_back(TrianglePainterData{
			v0.x, v0.y, v0.z, fill_r, fill_g, fill_b, fill_a,
			v1.x, v1.y, v1.z, fill_r, fill_g, fill_b, fill_a,
			v2.x, v2.y, v2.z, fill_r, fill_g, fill_b, fill_a
			});

		std::array<std::pair<size_t, size_t>, 3> edges{ {
			{f.vertex_idx1, f.vertex_idx2},
			{f.vertex_idx2, f.vertex_idx3},
			{f.vertex_idx1, f.vertex_idx3}
		} };

		for (const auto& edge : edges) {
			size_t i = edge.first;
			size_t j = edge.second;

			if (i > j) {
				std::swap(i, j);
			}
			std::pair<size_t, size_t> normalized{ i, j };
			if (drawnEdges.emplace(normalized).second) {
				const auto& a = vertices[i];
				const auto& b = vertices[j];

				renderData.mesh_lines.emplace_back(LinePainterData{
					a.x, a.y, a.z, line_r, line_g, line_b, line_a,
					b.x, b.y, b.z, line_r, line_g, line_b, line_a
					});
			}
		}

		Eigen::Vector4f color(fill_r, fill_g, fill_b, fill_a);
		if (idx < face_colors.size()) {
			color = face_colors[idx];
		}
		renderData.quality_data.emplace_back(TrianglePainterData{
			v0.x, v0.y, v0.z, color[0], color[1], color[2], color[3],
			v1.x, v1.y, v1.z, color[0], color[1], color[2], color[3],
			v2.x, v2.y, v2.z, color[0], color[1], color[2], color[3]
			});
	}
}

void OutputMeshPly(const CMesh& mesh, int imagenum)
{
	std::cout << "mesh info: "
		<< mesh.number_of_vertices() << " vertices, "
		<< mesh.number_of_faces() << " faces, "
		<< mesh.number_of_edges() << " edges"
		<< std::endl;

	if (!CGAL::is_valid_polygon_mesh(mesh)) {
		std::cerr << "warning: mesh is not a valid polygon mesh" << std::endl;
	}

	fs::path outputDir = "../output";
	fs::create_directories(outputDir);

	fs::path outputPath = outputDir / ("output_" + std::to_string(imagenum) + ".ply");
	if (!CGAL::IO::write_polygon_mesh(
		outputPath.string(),
		mesh,
		CGAL::parameters::stream_precision(17)))
	{
		std::cerr << "failed to write: " << outputPath << std::endl;
		return;
	}
	else {
		std::cout << "saved to " << outputPath << std::endl;
	}
}
