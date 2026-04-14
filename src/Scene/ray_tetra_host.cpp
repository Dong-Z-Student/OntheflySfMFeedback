#include "ray_tetra_cuda.h"
#include "ray_tetra_host.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>f
#include <limits>
#include <stdexcept>
#include <tbb/tbb.h>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace RayTetra
{
	struct HullFacet
	{
		Cell_handle cell;
		int opposite;
		Vec3d a, b, c;
	};

	static inline Vec3d ToVec3d(const Point& p)
	{
		return Vec3d{ p.x(), p.y(), p.z() };
	}

	static inline Vec3d MakeVec3d(double x, double y, double z)
	{
		return Vec3d{ x, y, z };
	}

	static inline Vec3d operator+(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{ a.x + b.x, a.y + b.y, a.z + b.z };
	}

	static inline Vec3d operator-(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{ a.x - b.x, a.y - b.y, a.z - b.z };
	}

	static inline Vec3d operator*(const Vec3d& a, double s)
	{
		return Vec3d{ a.x * s, a.y * s, a.z * s };
	}

	static inline double Dot(const Vec3d& a, const Vec3d& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	static inline Vec3d Cross(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
	}

	static inline double Norm2(const Vec3d& a)
	{
		return Dot(a, a);
	}

	static inline double Norm(const Vec3d& a)
	{
		return std::sqrt(Norm2(a));
	}

	// Möller–Trumbore，返回线段 p0->p1 与三角形是否相交
	// 命中时返回参数 t, 交点 = p0 + t * (p1-p0), 其中 t∈[0,1]
	static bool IntersectSegmentTriangle(
		const Vec3d& p0,
		const Vec3d& p1,
		const Vec3d& a,
		const Vec3d& b,
		const Vec3d& c,
		double& tOut,
		double eps = 1e-12)
	{
		Vec3d dir = p1 - p0;
		Vec3d e1 = b - a;
		Vec3d e2 = c - a;

		Vec3d pvec = Cross(dir, e2);
		double det = Dot(e1, pvec);

		if (std::fabs(det) < eps)
			return false;

		double invDet = 1.0 / det;
		Vec3d tvec = p0 - a;
		double u = Dot(tvec, pvec) * invDet;
		if (u < -eps || u > 1.0 + eps)
			return false;

		Vec3d qvec = Cross(tvec, e1);
		double v = Dot(dir, qvec) * invDet;
		if (v < -eps || u + v > 1.0 + eps)
			return false;

		double t = Dot(e2, qvec) * invDet;
		if (t < -eps || t > 1.0 + eps)
			return false;

		tOut = t;
		return true;
	}

	static std::vector<HullFacet> BuildHullFacets(const Delaunay& delaunay)
	{
		std::vector<HullFacet> facets;

		for (auto cit = delaunay.finite_cells_begin(); cit != delaunay.finite_cells_end(); ++cit)
		{
			for (int i = 0; i < 4; ++i)
			{
				Cell_handle nb = cit->neighbor(i);
				if (!delaunay.is_infinite(nb))
					continue;

				int idx[3];
				int k = 0;
				for (int v = 0; v < 4; ++v)
				{
					if (v != i) idx[k++] = v;
				}

				HullFacet f;
				f.cell = cit;
				f.opposite = i;
				f.a = ToVec3d(cit->vertex(idx[0])->point());
				f.b = ToVec3d(cit->vertex(idx[1])->point());
				f.c = ToVec3d(cit->vertex(idx[2])->point());

				facets.push_back(std::move(f));
			}
		}

		return facets;
	}

	// 找 entry cell：
	// 遍历所有 hull face（有限 cell 的某个面邻接 infinite cell），
	// 用 ray segment 与 hull triangle 求交，取最早进入的交点。
	static bool FindEntryCellByHullIntersection(
		const std::vector<HullFacet>& hullFacets,
		const Vec3d& p0,
		const Vec3d& p1,
		Cell_handle& entryCell)
	{
		const double eps = 1e-10;
		double bestT = std::numeric_limits<double>::infinity();
		bool found = false;

		for (const auto& f : hullFacets)
		{
			double t = 0.0;
			if (!IntersectSegmentTriangle(p0, p1, f.a, f.b, f.c, t, 1e-12))
				continue;

			if (t > eps && t < bestT)
			{
				bestT = t;
				entryCell = f.cell;
				found = true;
			}
		}

		return found;
	}

	static bool ExportFiniteCells(
		const Delaunay& delaunay,
		std::vector<GPUTetra>& tetraHost,
		std::unordered_map<uint64_t, int>& cellToIndex)
	{
		tetraHost.clear();
		cellToIndex.clear();

		int idx = 0; // use dense idx
		for (auto cit = delaunay.finite_cells_begin(); cit != delaunay.finite_cells_end(); ++cit)
		{
			cellToIndex[cit->info()] = idx;
			idx++;
		}

		tetraHost.resize(cellToIndex.size());

		for (auto cit = delaunay.finite_cells_begin(); cit != delaunay.finite_cells_end(); ++cit)
		{
			int cid = cellToIndex[cit->info()];
			GPUTetra gt{};

			for (int i = 0; i < 4; ++i)
			{
				gt.v[i] = ToVec3d(cit->vertex(i)->point());
			}

			for (int i = 0; i < 4; ++i)
			{
				Cell_handle nb = cit->neighbor(i);
				if (delaunay.is_infinite(nb))
				{
					gt.neigh[i] = -1;
				}
				else
				{
					auto it = cellToIndex.find(nb->info());
					if (it == cellToIndex.end())
						return false;
					gt.neigh[i] = it->second;
				}
			}

			gt.cell_info = cit->info();
			tetraHost[cid] = gt;
		}

		return true;
	}

	bool BuildAndTraceRays(
		DelaunayData& delaunaydata,
		std::vector<RayTetra::DirectionRay>& importRays,
		std::unordered_map<uint64_t, RayTraceResult>& results,
		std::string& errMsg)
	{
		results.clear();
		errMsg.clear();
		const Delaunay& delaunay = delaunaydata.dt;
		std::vector<GPUTetra> tetraHost;
		std::unordered_map<uint64_t, int> cellidToDenseIndex;

		if (!ExportFiniteCells(delaunay, tetraHost, cellidToDenseIndex))
		{
			errMsg = "ExportFiniteCells failed.";
			return false;
		}

		std::vector<RayInput> rayHost;
		rayHost.reserve(importRays.size());
		std::vector<HullFacet> hullfacets = BuildHullFacets(delaunay);

		tbb::enumerable_thread_specific<std::vector<RayInput>> thread_local_data;
		tbb::parallel_for(tbb::blocked_range<size_t>(0, importRays.size(), 10240),
			[&](const tbb::blocked_range<size_t>& range) {
				auto& local = thread_local_data.local();
				if (local.empty()) {
					local.reserve(range.size());
				}
				for (size_t i = range.begin(); i < range.end(); ++i) {
					const RayTetra::DirectionRay& dir_ray = importRays[i];
					Vec3d p0 = MakeVec3d(
						CGAL::to_double(dir_ray.start.x()),
						CGAL::to_double(dir_ray.start.y()),
						CGAL::to_double(dir_ray.start.z())
					);
					Vec3d p1 = MakeVec3d(
						CGAL::to_double(dir_ray.end.x()),
						CGAL::to_double(dir_ray.end.y()),
						CGAL::to_double(dir_ray.end.z())
					);

					Cell_handle entryCell;
					Cell_handle targetCell = delaunay.locate(dir_ray.nearEndPoint);
					if (delaunay.is_infinite(targetCell)) {
						continue;
					}

					bool ok = FindEntryCellByHullIntersection(hullfacets, p0, p1, entryCell);
					if (!ok || entryCell == Cell_handle())
					{
						//errMsg = "Failed to find entry tetra from hull intersection, ray id = " + std::to_string(i);
						continue;
					}

					auto itEntry = cellidToDenseIndex.find(entryCell->info());
					auto itTarget = cellidToDenseIndex.find(targetCell->info());
					if (itEntry == cellidToDenseIndex.end() || itTarget == cellidToDenseIndex.end())
					{
						//errMsg = "Entry/target cell not found in exported cell map, ray id = " + std::to_string(i);
						continue;
					}

					RayInput ri{};
					ri.p0 = p0;
					ri.p1 = p1;
					ri.entry_cell = itEntry->second;
					ri.target_cell = itTarget->second;
					ri.rayid = i;

					local.push_back(std::move(ri));
				}
			});

		for (auto& local : thread_local_data) {
			rayHost.insert(rayHost.end(),
				std::make_move_iterator(local.begin()),
				std::make_move_iterator(local.end()));
		}

		// 调用 CUDA
		std::vector<int> outCounts;
		std::vector<uint64_t> outCellInfos;
		std::vector<int> outNeighbors;
		std::vector<int> outStatus;

		std::cout << "ray tracing on delaunay tetra, total rays: " << rayHost.size() << "\n";
		auto t1 = std::chrono::high_resolution_clock::now();

		bool cudaOk = LaunchCudaTrace(
			tetraHost,
			rayHost,
			MAX_TETRA_PER_RAY,
			outCounts,
			outCellInfos,
			outNeighbors,
			outStatus,
			errMsg);

		auto t2 = std::chrono::high_resolution_clock::now();
		double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "Cuda Trace total time: " << ms << " ms\n";

		if (!cudaOk)
			return false;

		// 整理输出

		for (size_t rid = 0; rid < rayHost.size(); ++rid)
		{
			RayTraceResult rr;
			rr.ray_id = rid;
			rr.tetra_num = outCounts[rid];
			rr.status = outStatus[rid];

			rr.cell_ids.reserve(rr.tetra_num);
			for (int k = 0; k < rr.tetra_num; ++k)
			{
				rr.cell_ids.push_back(outCellInfos[rid * MAX_TETRA_PER_RAY + k]);

				if (k < rr.tetra_num - 1)
				{
					rr.next_idx.push_back(outNeighbors[rid * MAX_TETRA_PER_RAY + k]);
				}
			}

			results[rayHost[rid].rayid] = std::move(rr);
		}

		return true;
	}
}