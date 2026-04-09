#pragma once
#include "entities.h"
#include <cstdint>
#include <string>
#include <vector>

namespace RayTetra
{
	// CUDA ½Ó¿Ú
	bool LaunchCudaTrace(
		const std::vector<GPUTetra>& tetraHost,
		const std::vector<RayInput>& rayHost,
		int maxTetraPerRay,
		std::vector<int>& outCounts,
		std::vector<uint64_t>& outCellInfos,
		std::vector<int>& outNeighbors,
		std::vector<int>& outStatus,
		std::string& errMsg);
}