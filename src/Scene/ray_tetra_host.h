#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "cgal_types.h"
#include "entities.h"

namespace RayTetra
{
	// CPU ×ÜÈë¿Ú
	bool BuildAndTraceRays(
		Delaunay& delaunay,
		std::vector<Segment>& importRays,
		std::unordered_map<uint64_t, RayTraceResult>& results,
		std::string& errMsg);
}