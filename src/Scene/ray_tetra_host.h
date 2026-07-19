#pragma once

#include "cgal_types.h"
#include "delaunay.h"
#include "entities.h"
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace RayTetra
{
	struct DirectionRay {
		K::Point_3 start;
		K::Point_3 end;
		K::Ray_3 viewing_ray;
		K::Vector_3 normalized_direction;
		K::Point_3 nearEndPoint;
		K::Point_3 farEndPoint;
	};

	// CPU ×ÜÈë¿Ú
	bool BuildAndTraceRays(
		DelaunayData& delaunay,
		std::vector<RayTetra::DirectionRay>& importRays,
		std::unordered_map<uint64_t, RayTraceResult>& results,
		std::string& errMsg);
}