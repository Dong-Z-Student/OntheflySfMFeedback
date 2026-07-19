#include "ray_tetra_cuda.h"

#include <cmath>
#include <cstdint>
#include <cuda_runtime.h>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace RayTetra
{
	static inline void CheckCuda(cudaError_t err, const char* msg, std::string& errMsg)
	{
		if (err != cudaSuccess)
		{
			std::ostringstream oss;
			oss << msg << " : " << cudaGetErrorString(err);
			errMsg = oss.str();
		}
	}

	__device__ inline Vec3d vadd(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{ a.x + b.x, a.y + b.y, a.z + b.z };
	}

	__device__ inline Vec3d vsub(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{ a.x - b.x, a.y - b.y, a.z - b.z };
	}

	__device__ inline Vec3d vmul(const Vec3d& a, double s)
	{
		return Vec3d{ a.x * s, a.y * s, a.z * s };
	}

	__device__ inline double vdot(const Vec3d& a, const Vec3d& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	__device__ inline Vec3d vcross(const Vec3d& a, const Vec3d& b)
	{
		return Vec3d{
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
	}

	// 线段 p(t)=p0+t*dir, t∈[0,1]，与三角形相交测试
	__device__ bool IntersectSegmentTriangleDevice(
		const Vec3d& p0,
		const Vec3d& dir,
		const Vec3d& a,
		const Vec3d& b,
		const Vec3d& c,
		double& tOut,
		double eps = 1e-12)
	{
		Vec3d e1 = vsub(b, a);
		Vec3d e2 = vsub(c, a);

		Vec3d pvec = vcross(dir, e2);
		double det = vdot(e1, pvec);

		if (fabs(det) < eps)
			return false;

		double invDet = 1.0 / det;
		Vec3d tvec = vsub(p0, a);
		double u = vdot(tvec, pvec) * invDet;
		if (u < -eps || u > 1.0 + eps)
			return false;

		Vec3d qvec = vcross(tvec, e1);
		double v = vdot(dir, qvec) * invDet;
		if (v < -eps || u + v > 1.0 + eps)
			return false;

		double t = vdot(e2, qvec) * invDet;
		if (t < -eps || t > 1.0 + eps)
			return false;

		tOut = t;
		return true;
	}

	__global__ void TraceKernel(
		const GPUTetra* tetra,
		int tetraCount,
		const RayInput* rays,
		int rayCount,
		int maxTetraPerRay,
		uint64_t* outCellInfos,
		int* neighborInfos,
		int* outCounts,
		int* outStatus)
	{
		int rid = blockIdx.x * blockDim.x + threadIdx.x;
		if (rid >= rayCount)
			return;

		const RayInput ray = rays[rid];

		if (ray.entry_cell < 0 || ray.entry_cell >= tetraCount ||
			ray.target_cell < 0 || ray.target_cell >= tetraCount)
		{
			outCounts[rid] = 0;
			outStatus[rid] = TRACE_INVALID_ENTRY_OR_TARGET;
			return;
		}

		const double eps = 1e-10;
		const double epsT = 1e-10;

		Vec3d p0 = ray.p0;
		Vec3d p1 = ray.p1;
		Vec3d dir = vsub(p1, p0);

		int current = ray.entry_cell;
		double tCur = 0.0;
		int count = 0;
		int status = TRACE_SUCCESS;

		bool is_first = true;

		for (int step = 0; step < maxTetraPerRay; ++step)
		{
			// 记录当前 tetra
			outCellInfos[rid * maxTetraPerRay + count] = tetra[current].cell_info;
			++count;

			// 已到终点所在 tetra
			if (current == ray.target_cell)
			{
				status = TRACE_SUCCESS;
				break;
			}

			const GPUTetra& tet = tetra[current];

			// 枚举 4 个面
			int bestFace = -1, worstFace = -1;
			double bestT = 1e100;
			double worstT = 0.0;

			for (int face = 0; face < 4; ++face)
			{
				int idx[3]{};
				int k = 0;
				for (int v = 0; v < 4; ++v)
				{
					if (v != face) idx[k++] = v;
				}

				const Vec3d& a = tet.v[idx[0]];
				const Vec3d& b = tet.v[idx[1]];
				const Vec3d& c = tet.v[idx[2]];

				double tHit = 0.0;
				if (!IntersectSegmentTriangleDevice(p0, dir, a, b, c, tHit, 1e-12))
					continue;

				if (tHit > tCur + epsT && tHit <= 1.0 + eps)
				{
					if (tHit < bestT)
					{
						bestT = tHit;
						bestFace = face;
					}

					if (tHit > worstT)
					{
						worstT = tHit;
						worstFace = face;
					}
				}
			}

			if (bestFace < 0)
			{
				status = TRACE_NO_NEXT_FACE;
				break;
			}

			int next = tet.neigh[worstFace];
			neighborInfos[rid * maxTetraPerRay + (count - 1)] = worstFace;
			if (next < 0 || next >= tetraCount)
			{
				status = TRACE_HIT_OUTSIDE;
				break;
			}

			tCur = bestT;
			current = next;

			if (count >= maxTetraPerRay)
			{
				status = TRACE_EXCEED_MAX_STEPS;
				break;
			}
		}

		outCounts[rid] = count;
		outStatus[rid] = status;
	}

	bool LaunchCudaTrace(
		const std::vector<GPUTetra>& tetraHost,
		const std::vector<RayInput>& rayHost,
		int maxTetraPerRay,
		std::vector<int>& outCounts,
		std::vector<uint64_t>& outCellInfos,
		std::vector<int>& neighborInfos,
		std::vector<int>& outStatus,
		std::string& errMsg)
	{
		errMsg.clear();

		const int tetraCount = static_cast<int>(tetraHost.size());
		const int rayCount = static_cast<int>(rayHost.size());

		if (tetraCount == 0)
		{
			errMsg = "tetraHost is empty.";
			return false;
		}

		if (rayCount == 0)
		{
			outCounts.clear();
			outCellInfos.clear();
			outStatus.clear();
			return true;
		}

		GPUTetra* dTetra = nullptr;
		RayInput* dRays = nullptr;
		uint64_t* dOutCellInfos = nullptr;
		int* dNeighborInfos = nullptr;
		int* dOutCounts = nullptr;
		int* dOutStatus = nullptr;

		size_t tetraBytes = sizeof(GPUTetra) * tetraHost.size();
		size_t rayBytes = sizeof(RayInput) * rayHost.size();
		size_t outInfoBytes = sizeof(uint64_t) * rayHost.size() * maxTetraPerRay;
		size_t outNeighborBytes = sizeof(int) * rayHost.size() * maxTetraPerRay;
		size_t outCountBytes = sizeof(int) * rayHost.size();
		size_t outStatusBytes = sizeof(int) * rayHost.size();

		cudaError_t err;

		err = cudaMalloc((void**)&dTetra, tetraBytes);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMalloc dTetra failed", errMsg);
			return false;
		}

		err = cudaMalloc((void**)&dRays, rayBytes);
		if (err != cudaSuccess)
		{
			cudaFree(dTetra);
			CheckCuda(err, "cudaMalloc dRays failed", errMsg);
			return false;
		}

		err = cudaMalloc((void**)&dOutCellInfos, outInfoBytes);
		if (err != cudaSuccess)
		{
			cudaFree(dTetra);
			cudaFree(dRays);
			CheckCuda(err, "cudaMalloc dOutCellInfos failed", errMsg);
			return false;
		}

		err = cudaMalloc((void**)&dNeighborInfos, outNeighborBytes);
		if (err != cudaSuccess)
		{
			cudaFree(dTetra);
			cudaFree(dRays);
			CheckCuda(err, "cudaMalloc dNeighborInfos failed", errMsg);
			return false;
		}

		err = cudaMalloc((void**)&dOutCounts, outCountBytes);
		if (err != cudaSuccess)
		{
			cudaFree(dTetra);
			cudaFree(dRays);
			cudaFree(dOutCellInfos);
			CheckCuda(err, "cudaMalloc dOutCounts failed", errMsg);
			return false;
		}

		err = cudaMalloc((void**)&dOutStatus, outStatusBytes);
		if (err != cudaSuccess)
		{
			cudaFree(dTetra);
			cudaFree(dRays);
			cudaFree(dOutCellInfos);
			cudaFree(dOutCounts);
			CheckCuda(err, "cudaMalloc dOutStatus failed", errMsg);
			return false;
		}

		err = cudaMemcpy(dTetra, tetraHost.data(), tetraBytes, cudaMemcpyHostToDevice);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemcpy dTetra failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaMemcpy(dRays, rayHost.data(), rayBytes, cudaMemcpyHostToDevice);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemcpy dRays failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaMemset(dOutCellInfos, 0, outInfoBytes);
		err = cudaMemset(dNeighborInfos, -1, outNeighborBytes);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemset dOutCellInfos failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaMemset(dOutCounts, 0, outCountBytes);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemset dOutCounts failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaMemset(dOutStatus, 0, outStatusBytes);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemset dOutStatus failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		const int threads = 256;
		const int blocks = (rayCount + threads - 1) / threads;

		TraceKernel << <blocks, threads >> > (
			dTetra,
			tetraCount,
			dRays,
			rayCount,
			maxTetraPerRay,
			dOutCellInfos,
			dNeighborInfos,
			dOutCounts,
			dOutStatus);

		err = cudaGetLastError();
		if (err != cudaSuccess)
		{
			CheckCuda(err, "TraceKernel launch failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaDeviceSynchronize();
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaDeviceSynchronize failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		outCounts.resize(rayCount);
		outCellInfos.resize(static_cast<size_t>(rayCount) * maxTetraPerRay);
		neighborInfos.resize(static_cast<size_t>(rayCount) * maxTetraPerRay);
		outStatus.resize(rayCount);

		err = cudaMemcpy(outCounts.data(), dOutCounts, outCountBytes, cudaMemcpyDeviceToHost);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemcpy outCounts failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		err = cudaMemcpy(outCellInfos.data(), dOutCellInfos, outInfoBytes, cudaMemcpyDeviceToHost);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemcpy outCellInfos failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}
		err = cudaMemcpy(neighborInfos.data(), dNeighborInfos, outNeighborBytes, cudaMemcpyDeviceToHost);

		err = cudaMemcpy(outStatus.data(), dOutStatus, outStatusBytes, cudaMemcpyDeviceToHost);
		if (err != cudaSuccess)
		{
			CheckCuda(err, "cudaMemcpy outStatus failed", errMsg);
			cudaFree(dTetra); cudaFree(dRays); cudaFree(dOutCellInfos); cudaFree(dOutCounts); cudaFree(dOutStatus);
			return false;
		}

		cudaFree(dTetra);
		cudaFree(dRays);
		cudaFree(dOutCellInfos);
		cudaFree(dNeighborInfos);
		cudaFree(dOutCounts);
		cudaFree(dOutStatus);

		return true;
	}
}