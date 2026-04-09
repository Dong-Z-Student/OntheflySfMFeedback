#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>


#include "cgal_types.h"
#include "entities.h"
#include "rawio.h"

struct DelaunayData
{
	Delaunay dt;

	std::unordered_map<uint64_t, Vertex_handle> pointIdToVertex;

	std::unordered_map<uint64_t, Cell_handle> cellIdToCell;

	bool cellIndexBuilt = false;
};

namespace delaunay
{
	bool Build(const DenseData& dense, DelaunayData& outData);

	bool Build(const RawData& raw, DelaunayData& outData);

	// ต๗สิ
	void PrintSummary(const DelaunayData& data);

	CMesh build_surface_mesh(
		const std::unordered_set<Vertex_handle>& surface_vertices,
		const std::vector<Delaunay::Facet>& surface_facets);
}