#include "dynamic_graph_cut.h"
#include "mesh.h"
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

bool GenerateMeshModel(RawData& raw, RenderData& renderData) {
	rawio::FilterScene(raw);

	DelaunayData delaunayData;
	delaunay::Build(raw, delaunayData);
	//delaunay::PrintSummary(delaunayData);

	// =============== 权重图计算部分 ===============
	CellGraphData cell_graph_data;
	Delaunay& dt = delaunayData.dt;
	cell_graph_data.reserve(dt.number_of_cells());

	for (auto cit = dt.all_cells_begin(); cit != dt.all_cells_end(); ++cit) {
		Cell_handle c = cit;
		cell_graph_data.emplace(c->info(), DelaunayCellData(c->info()));
	}

	const double epsilon = 1e-3;

	std::vector<Segment> rays_seg;
	rays_seg.reserve(raw.rays.size());

	for (const auto& r : raw.rays) {
		const auto& startV = raw.images[r.raw_image_id];
		const auto& endV = raw.points[r.raw_point_id];
		Point start(startV.cx, startV.cy, startV.cz);
		Point end(endV.x, endV.y, endV.z);
		rays_seg.emplace_back(start, end);
	}

	std::unordered_map<uint64_t, RayTetra::RayTraceResult> trace_results;
	std::string errMsg;
	RayTetra::BuildAndTraceRays(dt, rays_seg, trace_results, errMsg);

	tbb::enumerable_thread_specific<CellGraphData> thread_local_data;
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, raw.rays.size(), 10240),
		[&](const tbb::blocked_range<size_t>& range) {
			auto& local_cell_graph = thread_local_data.local();

			for (size_t i = range.begin(); i < range.end(); ++i) {
				const auto& startV = raw.images[raw.rays[i].raw_image_id];
				const auto& endV = raw.points[raw.rays[i].raw_point_id];

				K::Point_3 start(startV.cx, startV.cy, startV.cz);
				K::Point_3 end(endV.x, endV.y, endV.z);
				K::Vector_3 direction(start, end);
				const K::Ray_3 viewing_ray = K::Ray_3(start, end);

				double length = std::sqrt(CGAL::to_double(direction.squared_length()));
				K::Vector_3 normalized_direction = direction / length;

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
						dt.locate(end + normalized_direction * epsilon);

					int behind_neighbor_idx = -1;
					double behind_distance_squared = 0.0;

					for (int neighbor_idx = 0; neighbor_idx < 4; ++neighbor_idx) {
						const K::Triangle_3 triangle =
							dt.triangle(behind_point_cell, neighbor_idx);

						K::Point_3 inter_point;
						if (CGAL::assign(inter_point,
							CGAL::intersection(viewing_ray, triangle))) {
							const double distance_squared =
								(inter_point - end).squared_length();
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
		const size_t nodeid = cell_data.second.index;
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

			const size_t mirror_nodeid = mirror_cell_data.index;
			gc.addEdge(nodeid, mirror_nodeid, forward_edge_weight, backward_edge_weight);
		}
	}

	gc.computeMaxFlow();

	std::unordered_set<Delaunay::Vertex_handle> surface_vertices;
	std::vector<Delaunay::Facet> surface_facets;
	surface_vertices.reserve(dt.number_of_vertices());
	surface_facets.reserve(dt.number_of_finite_cells() * 4);

	for (auto it = dt.finite_facets_begin(); it != dt.finite_facets_end(); ++it) {
		if (dt.is_infinite(it->first->neighbor(it->second))) {
			continue;
		}

		const auto& cell_data = cell_graph_data.at(it->first->info());
		const auto& mirror_cell_data = cell_graph_data.at(it->first->neighbor(it->second)->info());

		const size_t nodeid = cell_data.index;
		const size_t mirror_nodeid = mirror_cell_data.index;
		const bool cell_is_source =
			gc.getSegment(nodeid) == DynamicGraphCut::GraphType::SOURCE;
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

	CMesh mesh = delaunay::build_surface_mesh(surface_vertices, surface_facets);

	// Generate Render Data
	GenerateRenderData(mesh, renderData);

	return true;
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
