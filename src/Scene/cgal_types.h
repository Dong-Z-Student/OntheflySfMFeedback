#pragma once
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Handle_hash_function.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Triangulation_cell_base_with_info_3.h>
#include <CGAL/Triangulation_data_structure_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>

namespace PMP = CGAL::Polygon_mesh_processing;

struct CellInfo
{
	uint64_t value;

	constexpr CellInfo() noexcept
		: value((std::numeric_limits<uint64_t>::max)()) {}

	constexpr CellInfo(uint64_t v) noexcept : value(v) {}

	constexpr operator uint64_t() const noexcept { return value; }

	CellInfo& operator=(uint64_t v) noexcept
	{
		value = v;
		return *this;
	}
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel         K;
typedef CGAL::Triangulation_vertex_base_with_info_3<uint64_t, K>      Vb;
typedef CGAL::Triangulation_cell_base_with_info_3<CellInfo, K>           Cb;
typedef CGAL::Triangulation_data_structure_3<Vb, Cb>                Tds;
typedef CGAL::Delaunay_triangulation_3<K, Tds, CGAL::Fast_location> Delaunay;

typedef Delaunay::Vertex_handle Vertex_handle;
typedef Delaunay::Cell_handle   Cell_handle;

typedef K::Point_3                                                            Point;
typedef K::Segment_3														  Segment;
typedef CGAL::Surface_mesh<Point> CMesh;