#include "rawio.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>


namespace
{
	static std::string Trim(const std::string& s)
	{
		const char* ws = " \t\r\n";
		const size_t b = s.find_first_not_of(ws);
		if (b == std::string::npos)
			return "";
		const size_t e = s.find_last_not_of(ws);
		return s.substr(b, e - b + 1);
	}

	static void ReplaceCommaWithSpace(std::string& s)
	{
		std::replace(s.begin(), s.end(), ',', ' ');
	}

	static bool IsLineEmptyOrComment(const std::string& line)
	{
		const std::string t = Trim(line);
		return t.empty() || t[0] == '#';
	}

	constexpr double kScale = 1000000.0; // 保留 6 位小数

	inline int64_t Quantize6(float v) noexcept
	{
		return static_cast<int64_t>(std::llround(static_cast<double>(v) * kScale));
	}

	struct PointKey
	{
		int64_t x;
		int64_t y;
		int64_t z;

		bool operator==(const PointKey& other) const noexcept
		{
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct PointKeyHash
	{
		std::size_t operator()(const PointKey& k) const noexcept
		{
			std::size_t h1 = std::hash<int64_t>{}(k.x);
			std::size_t h2 = std::hash<int64_t>{}(k.y);
			std::size_t h3 = std::hash<int64_t>{}(k.z);

			std::size_t seed = h1;
			seed ^= h2 + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	struct RayKey
	{
		uint64_t image_id;
		uint64_t point_id;

		bool operator==(const RayKey& other) const noexcept
		{
			return image_id == other.image_id && point_id == other.point_id;
		}
	};

	struct RayKeyHash
	{
		size_t operator()(const RayKey& k) const noexcept
		{
			size_t h1 = std::hash<uint64_t>{}(k.image_id);
			size_t h2 = std::hash<uint64_t>{}(k.point_id);
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};

	struct PointEntry
	{
		uint64_t pointId;
		PointKey key;
		RawPointRecord* pt;
	};

	struct PointRemap
	{
		uint64_t oldPointId;
		uint64_t canonicalPointId;
	};

	static inline bool PointKeyLess(const PointKey& a, const PointKey& b) noexcept
	{
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
}

namespace rawio
{
	static bool ReadPointsTxt(
		const std::string& path,
		std::unordered_map<uint64_t, RawPointRecord>& outPoints,
		std::unordered_map<uint64_t, uint64_t>& group)
	{
		outPoints.clear();

		std::ifstream fin(path);
		if (!fin.is_open())
		{
			std::cerr << "Failed to open points file: " << path << std::endl;
			return false;
		}

		std::unordered_map<std::string, uint64_t> ump;

		std::string line;
		size_t lineNo = 0;
		while (std::getline(fin, line))
		{
			++lineNo;
			if (IsLineEmptyOrComment(line))
				continue;
			std::string xyzRaw;
			{
				size_t p1 = line.find(',');
				if (p1 == std::string::npos)
				{
					std::cerr << "Invalid line format at line " << lineNo << ": " << line << std::endl;
					return false;
				}
				xyzRaw = line.substr(p1 + 1);
			}
			ReplaceCommaWithSpace(line);
			std::istringstream iss(line);

			RawPointRecord rec{};
			if (!(iss >> rec.raw_point_id >> rec.x >> rec.y >> rec.z))
			{
				std::cerr << "Invalid line in points file at line " << lineNo << ": " << line << std::endl;
				return false;
			}

			if (auto it = ump.find(xyzRaw); it == ump.end()) {
				ump[xyzRaw] = rec.raw_point_id;
				group[rec.raw_point_id] = rec.raw_point_id;
				outPoints[rec.raw_point_id] = std::move(rec);
			}
			else {
				uint64_t groupid = ump[xyzRaw];
				group[rec.raw_point_id] = groupid;
			}
		}
		return true;
	}

	bool ReadImagesTxt(const std::string& path, std::unordered_map<uint64_t, RawImageRecord>& outImages)
	{
		outImages.clear();

		std::ifstream fin(path);
		if (!fin.is_open())
		{
			std::cerr << "Failed to open images file: " << path << std::endl;
			return false;
		}

		std::string line;
		size_t lineNo = 0;
		while (std::getline(fin, line))
		{
			++lineNo;
			if (IsLineEmptyOrComment(line))
				continue;

			ReplaceCommaWithSpace(line);
			std::istringstream iss(line);

			RawImageRecord rec{};
			if (!(iss >> rec.raw_image_id >> rec.cx >> rec.cy >> rec.cz))
			{
				std::cerr << "Invalid line in images file at line " << lineNo << ": " << line << std::endl;
				return false;
			}

			outImages[rec.raw_image_id] = std::move(rec);
		}

		return true;
	}

	bool ReadRaysTxt(
		const std::string& path,
		std::vector<RawRayRecord>& outRays,
		std::unordered_map<uint64_t, uint64_t>& group,
		std::unordered_set<uint64_t>& removedPointIds
	)
	{
		outRays.clear();
		removedPointIds.clear();

		std::ifstream fin(path);
		if (!fin.is_open())
		{
			std::cerr << "Failed to open rays file: " << path << std::endl;
			return false;
		}

		std::unordered_set<RayKey, RayKeyHash> seen;

		std::string line;
		size_t lineNo = 0;
		while (std::getline(fin, line))
		{
			++lineNo;
			if (IsLineEmptyOrComment(line))
				continue;

			ReplaceCommaWithSpace(line);
			std::istringstream iss(line);

			RawRayRecord rec{};
			if (!(iss >> rec.raw_image_id >> rec.raw_point_id))
			{
				std::cerr << "Invalid line in rays file at line "
					<< lineNo << ": " << line << std::endl;
				return false;
			}

			if (auto it = group.find(rec.raw_point_id); it != group.end())
			{
				rec.raw_point_id = group[rec.raw_point_id];
			}

			RayKey key{ rec.raw_image_id, rec.raw_point_id };
			if (seen.insert(key).second)
			{
				outRays.push_back(rec);
			}
		}

		std::unordered_map<uint64_t, size_t> pointCount;
		for (const auto& r : outRays)
		{
			pointCount[r.raw_point_id]++;
		}

		const size_t MIN_IMAGE_COUNT = 5;

		for (const auto& [pid, cnt] : pointCount)
		{
			if (cnt < MIN_IMAGE_COUNT)
			{
				removedPointIds.insert(pid);
			}
		}

		std::vector<RawRayRecord> filtered;
		filtered.reserve(outRays.size());

		for (const auto& r : outRays)
		{
			if (removedPointIds.find(r.raw_point_id) == removedPointIds.end())
			{
				filtered.push_back(r);
			}
		}

		outRays.swap(filtered);

		return true;
	}

	bool ReadAll(
		const std::string& pointsPath,
		const std::string& imagesPath,
		const std::string& raysPath,
		RawData& outData)
	{
		outData = RawData{};
		std::unordered_map<uint64_t, uint64_t> duplicate_group;
		std::unordered_set<uint64_t> removedPointIds;

		if (!ReadPointsTxt(pointsPath, outData.points, duplicate_group))
			return false;
		if (!ReadImagesTxt(imagesPath, outData.images))
			return false;
		if (!ReadRaysTxt(raysPath, outData.rays, duplicate_group, removedPointIds))
			return false;

		for (uint64_t pid : removedPointIds)
		{
			outData.points.erase(pid);
		}

		std::cout << "Read points: " << outData.points.size() << std::endl;
		std::cout << "Read images: " << outData.images.size() << std::endl;
		std::cout << "Read rays  : " << outData.rays.size() << std::endl;

		return true;
	}

	void CopyFromReconstruction(Reconstruction& reconstruction, RawData& outData)
	{
		outData.points.reserve(reconstruction.NumPoints3D());
		outData.images.reserve(reconstruction.NumImages());
		outData.rays.reserve(reconstruction.NumPoints3D() * 6);
		for (const auto& point3D : reconstruction.Points3D()) {
			if (point3D.second.Track().Length() <= 3) continue;
			RawPointRecord recpt{};
			recpt.raw_point_id = point3D.first;
			const auto& xyz = point3D.second.XYZ();
			recpt.x = static_cast<float>(xyz.x());
			recpt.y = static_cast<float>(xyz.y());
			recpt.z = static_cast<float>(xyz.z());
			recpt.track = point3D.second.Track().Length();
			recpt.error = point3D.second.Error();
			outData.points[recpt.raw_point_id] = recpt;

			for (const auto& element : point3D.second.Track().Elements()) {
				RawRayRecord recray{};
				recray.raw_image_id = element.image_id;
				recray.raw_point_id = recpt.raw_point_id;
				outData.rays.push_back(recray);
			}
		}
		for (const image_t image_id : reconstruction.RegImageIds()) {
			const auto& image = reconstruction.Image(image_id);
			RawImageRecord recimg{};
			recimg.raw_image_id = image.ImageId();
			const auto& imgproj = image.ProjectionCenter();
			recimg.cx = static_cast<float>(imgproj.x());
			recimg.cy = static_cast<float>(imgproj.y());
			recimg.cz = static_cast<float>(imgproj.z());
			outData.images[recimg.raw_image_id] = recimg;
		}
	}
	void FilterScene(RawData& inputData)
	{
		std::unordered_map<uint64_t, RawPointRecord> oldPoints = std::move(inputData.points);
		std::vector<RawRayRecord> oldRays = std::move(inputData.rays);

		inputData.points.clear();
		inputData.rays.clear();

		// old_point_id -> canonical_point_id
		std::unordered_map<uint64_t, uint64_t> pointRemap;
		pointRemap.reserve(oldPoints.size());
		pointRemap.max_load_factor(0.7f);

		std::unordered_map<PointKey, uint64_t, PointKeyHash> pointKeyToId;
		pointKeyToId.reserve(oldPoints.size());
		pointKeyToId.max_load_factor(0.7f);
		inputData.points.reserve(oldPoints.size());
		inputData.points.max_load_factor(0.7f);

		for (auto& kv : oldPoints)
		{
			const uint64_t pointId = kv.first;
			RawPointRecord& pt = kv.second;

			PointKey key{
				Quantize6(pt.x),
				Quantize6(pt.y),
				Quantize6(pt.z)
			};

			auto it = pointKeyToId.find(key);
			if (it == pointKeyToId.end())
			{
				pointKeyToId.emplace(key, pointId);
				pointRemap.emplace(pointId, pointId);
				inputData.points.emplace(pointId, std::move(pt));
			}
			else
			{
				pointRemap.emplace(pointId, it->second);
			}
		}

		oldPoints.clear();
		oldPoints.rehash(0);

		std::unordered_set<RayKey, RayKeyHash> seenRays;
		seenRays.reserve(oldRays.size());
		seenRays.max_load_factor(0.7f);

		inputData.rays.reserve(oldRays.size());

		for (const auto& ray : oldRays)
		{
			auto itRemap = pointRemap.find(ray.raw_point_id);
			if (itRemap == pointRemap.end())
			{
				continue;
			}

			const uint64_t canonicalPointId = itRemap->second;
			RayKey rayKey{ ray.raw_image_id, canonicalPointId };

			if (seenRays.insert(rayKey).second)
			{
				inputData.rays.push_back(RawRayRecord{
					ray.raw_image_id,
					canonicalPointId
					});
			}
		}
	}

	void FilterScene_SortBased(RawData& inputData)
	{
		std::unordered_map<uint64_t, RawPointRecord> oldPoints = std::move(inputData.points);
		std::vector<RawRayRecord> oldRays = std::move(inputData.rays);

		inputData.points.clear();
		inputData.rays.clear();

		std::vector<PointEntry> pointEntries;
		pointEntries.reserve(oldPoints.size());

		for (auto& kv : oldPoints)
		{
			pointEntries.push_back(PointEntry{
				kv.first,
				PointKey{
					Quantize6(kv.second.x),
					Quantize6(kv.second.y),
					Quantize6(kv.second.z)
				},
				&kv.second
				});
		}

		std::sort(pointEntries.begin(), pointEntries.end(),
			[](const PointEntry& a, const PointEntry& b)
			{
				if (PointKeyLess(a.key, b.key)) return true;
				if (PointKeyLess(b.key, a.key)) return false;
				return a.pointId < b.pointId;
			});

		// old_point_id -> canonical_point_id
		std::vector<PointRemap> remaps;
		remaps.reserve(pointEntries.size());

		inputData.points.reserve(oldPoints.size());
		inputData.points.max_load_factor(0.7f);
		for (size_t i = 0; i < pointEntries.size();)
		{
			size_t j = i + 1;
			while (j < pointEntries.size() && pointEntries[i].key == pointEntries[j].key)
			{
				++j;
			}
			const uint64_t canonicalPointId = pointEntries[i].pointId;
			inputData.points.emplace(canonicalPointId, std::move(*pointEntries[i].pt));
			for (size_t k = i; k < j; ++k)
			{
				remaps.push_back(PointRemap{
					pointEntries[k].pointId,
					canonicalPointId
					});
			}
			i = j;
		}

		std::sort(remaps.begin(), remaps.end(),
			[](const PointRemap& a, const PointRemap& b)
			{
				return a.oldPointId < b.oldPointId;
			});

		std::sort(oldRays.begin(), oldRays.end(),
			[](const RawRayRecord& a, const RawRayRecord& b)
			{
				if (a.raw_point_id != b.raw_point_id) return a.raw_point_id < b.raw_point_id;
				return a.raw_image_id < b.raw_image_id;
			});

		std::vector<RawRayRecord> remappedRays;
		remappedRays.reserve(oldRays.size());

		size_t rayIdx = 0;
		size_t remapIdx = 0;

		while (rayIdx < oldRays.size() && remapIdx < remaps.size())
		{
			const uint64_t rayPointId = oldRays[rayIdx].raw_point_id;
			const uint64_t remapPointId = remaps[remapIdx].oldPointId;

			if (rayPointId < remapPointId)
			{
				const uint64_t missingId = rayPointId;
				while (rayIdx < oldRays.size() && oldRays[rayIdx].raw_point_id == missingId)
				{
					++rayIdx;
				}
			}
			else if (rayPointId > remapPointId)
			{
				++remapIdx;
			}
			else
			{
				const uint64_t canonicalPointId = remaps[remapIdx].canonicalPointId;

				while (rayIdx < oldRays.size() && oldRays[rayIdx].raw_point_id == rayPointId)
				{
					remappedRays.push_back(RawRayRecord{
						oldRays[rayIdx].raw_image_id,
						canonicalPointId
						});
					++rayIdx;
				}
				++remapIdx;
			}
		}

		std::sort(remappedRays.begin(), remappedRays.end(),
			[](const RawRayRecord& a, const RawRayRecord& b)
			{
				if (a.raw_image_id != b.raw_image_id) return a.raw_image_id < b.raw_image_id;
				return a.raw_point_id < b.raw_point_id;
			});

		remappedRays.erase(
			std::unique(remappedRays.begin(), remappedRays.end(),
				[](const RawRayRecord& a, const RawRayRecord& b)
				{
					return a.raw_image_id == b.raw_image_id &&
						a.raw_point_id == b.raw_point_id;
				}),
			remappedRays.end());

		inputData.rays = std::move(remappedRays);

		oldPoints.clear();
		oldPoints.rehash(0);
	}

	SceneDiff CompareSceneDiff(const RawData& prev, const RawData& next)
	{
		SceneDiff diff;
		std::vector<uint64_t> prev_ids;
		std::vector<uint64_t> next_ids;
		prev_ids.reserve(prev.points.size());
		next_ids.reserve(next.points.size());

		for (const auto& kv : prev.points) { prev_ids.push_back(kv.first); }
		for (const auto& kv : next.points) { next_ids.push_back(kv.first); }

		std::sort(prev_ids.begin(), prev_ids.end());
		std::sort(next_ids.begin(), next_ids.end());

		size_t i = 0;
		size_t j = 0;
		while (i < prev_ids.size() && j < next_ids.size())
		{
			if (prev_ids[i] == next_ids[j])
			{
				++i;
				++j;
			}
			else if (prev_ids[i] < next_ids[j])
			{
				diff.removed_points.push_back(prev_ids[i]);
				++i;
			}
			else
			{
				diff.added_points.push_back(next_ids[j]);
				++j;
			}
		}
		while (i < prev_ids.size())
		{
			diff.removed_points.push_back(prev_ids[i]);
			++i;
		}
		while (j < next_ids.size())
		{
			diff.added_points.push_back(next_ids[j]);
			++j;
		}

		return diff;
	}
}