#include "delaunay.h"

#include <iostream>
#include <list>

namespace delaunay
{
	const uint64_t kInfiniteVertexInfo = std::numeric_limits<uint64_t>::max();
	// Build Delaunay with unique vertex index & unique cell index
	bool Build(const DenseData& dense, DelaunayData& outData)
	{
		outData = DelaunayData{};

		if (dense.points.empty())
		{
			std::cerr << "No points for Delaunay build." << std::endl;
			return false;
		}

		std::vector<std::pair<K::Point_3, uint64_t>> cgalPoints;
		cgalPoints.reserve(dense.points.size());

		for (const auto& p : dense.points)
		{
			cgalPoints.emplace_back(K::Point_3(p.x, p.y, p.z), p.point_id);
		}

		outData.dt.insert(cgalPoints.begin(), cgalPoints.end());

		//std::cout << "Delaunay vertices : " << outData.dt.number_of_vertices() << std::endl;
		//std::cout << "Delaunay finite cells : " << outData.dt.number_of_finite_cells() << std::endl;

		for (auto vit = outData.dt.finite_vertices_begin();
			vit != outData.dt.finite_vertices_end(); ++vit)
		{
			Vertex_handle vit_handle = vit;
			const uint64_t pid = vit_handle->info();
			outData.pointIdToVertex[pid] = vit_handle;
		}

		uint64_t cellId = 0;
		for (auto cit = outData.dt.all_cells_begin();
			cit != outData.dt.all_cells_end(); ++cit)
		{
			Cell_handle cit_handle = cit;
			cit_handle->info() = cellId;
			outData.cellIdToCell[cellId] = cit_handle;
			cellId++;
		}

		outData.cellIndexBuilt = true;
		return true;
	}

	bool Build(const RawData& rawdata, DelaunayData& outData)
	{
		if (rawdata.points.empty())
		{
			std::cerr << "No points for Delaunay build." << std::endl;
			return false;
		}

		std::vector<const std::pair<const uint64_t, RawPointRecord>*> sortedPoints;
		sortedPoints.reserve(rawdata.points.size());

		for (const auto& kv : rawdata.points)
		{
			sortedPoints.emplace_back(&kv);
		}

		std::sort(sortedPoints.begin(), sortedPoints.end(),
			[](const auto* a, const auto* b)
			{
				return a->first < b->first;
			});

		std::vector<std::pair<K::Point_3, uint64_t>> cgalPoints;
		cgalPoints.reserve(sortedPoints.size());

		for (const auto* kv : sortedPoints)
		{
			const auto& [idx, p] = *kv;
			cgalPoints.emplace_back(K::Point_3(p.x, p.y, p.z), p.raw_point_id);
		}

		outData.dt.insert(cgalPoints.begin(), cgalPoints.end());

		//std::cout << "Delaunay vertices : " << outData.dt.number_of_vertices() << std::endl;
		//std::cout << "Delaunay finite cells : "
		//	<< outData.dt.number_of_finite_cells() << std::endl;

		for (auto vit = outData.dt.finite_vertices_begin();
			vit != outData.dt.finite_vertices_end(); ++vit)
		{
			Vertex_handle vit_handle = vit;
			const uint64_t pid = vit_handle->info();
			outData.pointIdToVertex[pid] = vit_handle;
		}

		outData.dt.infinite_vertex()->info() = kInfiniteVertexInfo;

		uint64_t cellIdx = 2;
		for (auto cit = outData.dt.all_cells_begin();
			cit != outData.dt.all_cells_end(); ++cit)
		{
			Cell_handle cit_handle = cit;
			cit_handle->info() = cellIdx;
			outData.cellIdToCell[cellIdx] = cit_handle;
			outData.cellIdTocellIdx[cellIdx] = cellIdx;
			outData.cellIdxTocellId[cellIdx] = cellIdx;
			cellIdx++;
		}

		LOG(INFO) << "Delaunay vertices number  : " << outData.dt.number_of_vertices();
		LOG(INFO) << "Delaunay cells number : " << outData.dt.number_of_cells();

		outData.cellIndexBuilt = true;
		return true;
	}

	bool Update(const RawData& scene, const RawData& last_scene, DelaunayData& outData)
	{
		std::unordered_set<uint64_t> last_cellid_set;
		last_cellid_set.reserve(outData.dt.number_of_cells());

		uint64_t max_cellid = 1;
		for (auto cit = outData.dt.all_cells_begin(); cit != outData.dt.all_cells_end(); ++cit)
		{
			const uint64_t cid = cit->info();
			last_cellid_set.insert(cid);
			if (max_cellid < cid) max_cellid = cid;
		}

		std::vector<Vertex_handle> toRemove;
		if (last_scene.points.size() > scene.points.size())
		{
			toRemove.reserve(last_scene.points.size() - scene.points.size());
		}

		for (const auto& kv : last_scene.points)
		{
			const uint64_t id = kv.first;
			if (scene.points.find(id) != scene.points.end())
			{
				continue;
			}

			auto it = outData.pointIdToVertex.find(id);
			if (it == outData.pointIdToVertex.end())
			{
				continue;
			}

			Vertex_handle vh = it->second;
			if (vh != Vertex_handle() && !outData.dt.is_infinite(vh))
			{
				toRemove.push_back(vh);
			}
		}

		std::vector<std::pair<K::Point_3, uint64_t>> toInsert;
		if (scene.points.size() > last_scene.points.size())
		{
			toInsert.reserve(scene.points.size() - last_scene.points.size());
		}

		for (const auto& kv : scene.points)
		{
			const uint64_t id = kv.first;
			if (last_scene.points.find(id) != last_scene.points.end())
			{
				continue;
			}

			const RawPointRecord& p = kv.second;
			toInsert.emplace_back(K::Point_3(p.x, p.y, p.z), p.raw_point_id);
		}

		const size_t removedCount = toRemove.size();
		for (Vertex_handle vh : toRemove)
		{
			outData.dt.remove(vh);
		}

		const size_t addedCount = toInsert.size();
		if (!toInsert.empty())
		{
			outData.dt.insert(toInsert.begin(), toInsert.end());
		}

		outData.dt.infinite_vertex()->info() = kInfiniteVertexInfo;

		outData.pointIdToVertex.clear();
		outData.pointIdToVertex.reserve(outData.dt.number_of_vertices());
		for (auto vit = outData.dt.finite_vertices_begin(); vit != outData.dt.finite_vertices_end(); ++vit)
		{
			Vertex_handle vh = vit;
			outData.pointIdToVertex[vh->info()] = vh;
		}

		const size_t cellCount = outData.dt.number_of_cells();
		outData.cellIdToCell.clear();
		outData.cellIdToCell.reserve(cellCount);

		outData.cellIdTocellIdx.clear();
		outData.cellIdTocellIdx.reserve(cellCount);

		outData.cellIdxTocellId.clear();
		outData.cellIdxTocellId.reserve(cellCount);

		uint64_t next_cellid = max_cellid + 1;
		uint64_t cell_idx = 2;

		for (auto cit = outData.dt.all_cells_begin(); cit != outData.dt.all_cells_end(); ++cit)
		{
			if (last_cellid_set.find(cit->info()) == last_cellid_set.end())
			{
				cit->info() = next_cellid++;
			}

			const uint64_t cid = cit->info();
			Cell_handle ch = cit;

			outData.cellIdToCell[cid] = ch;
			outData.cellIdTocellIdx[cid] = cell_idx;
			outData.cellIdxTocellId[cell_idx] = cid;
			++cell_idx;
		}

		outData.cellIndexBuilt = true;

		//LOG(INFO) << "Added vertices : " << addedCount;
		//LOG(INFO) << "Removed vertices : " << removedCount;
		//LOG(INFO) << "Delaunay vertices : " << outData.dt.number_of_vertices();
		//LOG(INFO) << "Delaunay finite cells : " << outData.dt.number_of_cells();

		return true;
	}


	void PrintSummary(const DelaunayData& data)
	{
		std::cout << "===== Delaunay Summary =====" << std::endl;
		std::cout << "Vertices    : " << data.dt.number_of_vertices() << std::endl;
		std::cout << "Finite cells: " << data.dt.number_of_finite_cells() << std::endl;
		std::cout << "Indexed     : " << (data.cellIndexBuilt ? "true" : "false") << std::endl;
	}

	CMesh build_surface_mesh(
		const std::unordered_set<Vertex_handle>& surface_vertices,
		const std::vector<Delaunay::Facet>& surface_facets)
	{
		CMesh mesh;

		std::vector<Point> points;
		points.reserve(surface_vertices.size());

		std::unordered_map<Vertex_handle, std::size_t> vh_to_pid;
		vh_to_pid.reserve(surface_vertices.size());

		for (Vertex_handle vh : surface_vertices)
		{
			const auto& p = vh->point();
			std::size_t pid = points.size();
			points.emplace_back(p.x(), p.y(), p.z());
			vh_to_pid[vh] = pid;
		}
		std::vector<std::vector<std::size_t>> polygons;
		polygons.reserve(surface_facets.size());

		for (const Delaunay::Facet& f : surface_facets)
		{
			Cell_handle c = f.first;
			int opp = f.second;

			std::vector<std::size_t> tri;
			tri.reserve(3);

			bool ok = true;
			for (int i = 0; i < 4; ++i)
			{
				if (i == opp) continue;

				Vertex_handle vh = c->vertex(i);
				auto it = vh_to_pid.find(vh);
				if (it == vh_to_pid.end())
				{
					// facet 顶点不在 surface_vertices 中，跳过这个面
					ok = false;
					break;
				}

				tri.push_back(it->second);
			}

			if (ok && tri.size() == 3)
				polygons.push_back(std::move(tri));
		}

		PMP::orient_polygon_soup(points, polygons);

		PMP::polygon_soup_to_polygon_mesh(points, polygons, mesh);

		return mesh;
	}
}