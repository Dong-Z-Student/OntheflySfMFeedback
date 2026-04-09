#include "delaunay.h"

#include <iostream>
#include <list>

namespace delaunay
{
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
		outData = DelaunayData{};

		if (rawdata.points.empty())
		{
			std::cerr << "No points for Delaunay build." << std::endl;
			return false;
		}

		std::vector<std::pair<K::Point_3, uint64_t>> cgalPoints;
		cgalPoints.reserve(rawdata.points.size());

		for (const auto& [idx, p] : rawdata.points)
		{
			cgalPoints.emplace_back(K::Point_3(p.x, p.y, p.z), p.raw_point_id);
		}

		outData.dt.insert(cgalPoints.begin(), cgalPoints.end());

		std::cout << "Delaunay vertices : " << outData.dt.number_of_vertices() << std::endl;
		std::cout << "Delaunay finite cells : "
			<< outData.dt.number_of_finite_cells() << std::endl;

		for (auto vit = outData.dt.finite_vertices_begin();
			vit != outData.dt.finite_vertices_end(); ++vit)
		{
			Vertex_handle vit_handle = vit;
			const uint64_t pid = vit_handle->info();
			outData.pointIdToVertex[pid] = vit_handle;
		}

		uint64_t cellId = 2;
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