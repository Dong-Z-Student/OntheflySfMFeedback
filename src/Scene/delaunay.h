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

	// cell id ---> cell handle
	std::unordered_map<uint64_t, Cell_handle> cellIdToCell;

	// cell idx ---> cell id
	std::unordered_map<uint64_t, uint64_t> cellIdxTocellId;

	// cell id ---> cell idx
	std::unordered_map<uint64_t, uint64_t> cellIdTocellIdx;

	inline uint64_t getCellIdx(uint64_t cellid) { return cellIdTocellIdx.at(cellid); }
	inline uint64_t getCellId(uint64_t cellidx) { return cellIdxTocellId.at(cellidx); }

	bool cellIndexBuilt = false;
};

namespace delaunay
{
	bool Build(const DenseData& dense, DelaunayData& outData);

	bool Build(const RawData& rawdata, DelaunayData& outData);

	bool Update(const RawData& scene, const RawData& last_scene, DelaunayData& outData);

	// ต๗สิ
	void PrintSummary(const DelaunayData& data);

	CMesh build_surface_mesh(
		const std::unordered_set<Vertex_handle>& surface_vertices,
		const std::vector<Delaunay::Facet>& surface_facets);
}