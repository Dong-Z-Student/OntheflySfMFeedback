#pragma once
#include <cstdint>
#include <vector>

// 基础几何类型

struct float3f
{
	float x;
	float y;
	float z;
};

// 原始输入数据结构

// points
struct RawPointRecord
{
	uint64_t raw_point_id;
	float    x;
	float    y;
	float    z;
};

// images
struct RawImageRecord
{
	uint64_t raw_image_id;
	float    cx;
	float    cy;
	float    cz;
};

// rays
struct RawRayRecord
{
	uint64_t raw_image_id;
	uint64_t raw_point_id;
};

// Dense 映射后的 CPU 内部结构

struct DensePoint
{
	uint64_t point_id;        // dense id
	uint64_t raw_point_id;    // original id

	float x;
	float y;
	float z;
};

struct DenseImage
{
	uint64_t image_id;       // dense id
	uint64_t raw_image_id;    // original id

	float cx;
	float cy;
	float cz;
};

namespace RayTetra
{
	constexpr int MAX_TETRA_PER_RAY = 256;

	struct Vec3d
	{
		double x, y, z;
	};

	struct GPUTetra
	{
		Vec3d v[4];              // 四面体四个顶点
		int neigh[4];            // 第 i 个面对应的邻接 cell dense idx；无邻接则 -1
		uint64_t cell_info;      // 对应 cell info !important
	};

	struct RayInput
	{
		uint64_t rayid;			 // 光线编号
		Vec3d p0;                // 光线起点
		Vec3d p1;                // 光线终点
		int entry_cell;          // CPU 预处理得到
		int target_cell;         // CPU 预处理得到
	};

	struct RayTraceResult
	{
		uint64_t ray_id = -1;
		int tetra_num = 0;
		std::vector<uint64_t> cell_ids;   // 穿过的 tetra 的 info()
		std::vector<int> next_idx;             // 相邻 tetra 编号
		int status = 0;                   // 0表示成功
	};

	enum TraceStatus
	{
		TRACE_SUCCESS = 0,
		TRACE_NO_NEXT_FACE = 1,
		TRACE_HIT_OUTSIDE = 2,
		TRACE_EXCEED_MAX_STEPS = 3,
		TRACE_INVALID_ENTRY_OR_TARGET = 4
	};
}
