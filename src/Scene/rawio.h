#pragma once

#include "entities.h"
#include "Reconstruction.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct RawData
{
	std::unordered_map<uint64_t, RawPointRecord> points;
	std::unordered_map<uint64_t, RawImageRecord> images;
	std::vector<RawRayRecord> rays;
};

struct DenseData
{
	std::vector<DensePoint> points;
	std::vector<DenseImage> images;

	// rays -- dense id
	struct DenseRayRecord
	{
		uint64_t image_id; // dense image id
		uint64_t point_id; // dense point id
	};

	std::vector<DenseRayRecord> rays;

	std::unordered_map<uint64_t, uint64_t> rawPointToDense;
	std::unordered_map<uint64_t, uint64_t> rawImageToDense;

	std::vector<uint64_t> denseToRawPoint;
	std::vector<uint64_t> denseToRawImage;
};


namespace rawio
{
	bool ReadPointsTxt(const std::string& path, std::unordered_map<uint64_t, RawPointRecord>& outPoints, std::unordered_map<uint64_t, uint64_t>& group);
	bool ReadImagesTxt(const std::string& path, std::unordered_map<uint64_t, RawImageRecord>& outImages);
	bool ReadRaysTxt(const std::string& path, std::vector<RawRayRecord>& outRays, std::unordered_map<uint64_t, uint64_t>& group, std::unordered_set<uint64_t>& removedPointIds);

	bool ReadAll(
		const std::string& pointsPath,
		const std::string& imagesPath,
		const std::string& raysPath,
		RawData& outData);

	void CopyFromReconstruction(Reconstruction& model, RawData& outData);
	void FilterScene(RawData& inputData); // ∏¥‘”∂»O(n)
	void FilterScene_SortBased(RawData& inputData); // ∏¥‘”∂»O(nlogn)
	SceneDiff CompareSceneDiff(const RawData& prev, const RawData& next);
}