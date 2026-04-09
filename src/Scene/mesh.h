#pragma once
#include "delaunay.h"
#include "Reconstruction.h"
#include <array>

const double alphafree = 1000.0;
const double alphaoccupied = 1000.0;
const double alphavisual = 1.0;

struct DelaunayCellData {
	DelaunayCellData() : DelaunayCellData(-1) {}
	explicit DelaunayCellData(int index)
		: index(index),
		source_weight(0.0),
		sink_weight(0.0),
		edge_weights{}
	{
		std::fill(edge_weights.begin(), edge_weights.end(), 0.0);
	}
	int index;
	float source_weight;
	float sink_weight;
	std::array<float, 4> edge_weights;
};

struct DelaunayTriangulationRayCaster {
	struct Intersection {
		Delaunay::Facet facet;
		double target_distance_squared = 0.0;
	};

	explicit DelaunayTriangulationRayCaster(Delaunay& triangulation)
		: triangulation_(triangulation) {
		FindHullFacets();
	}

	void CastRaySegment(const K::Segment_3& ray_segment,
		std::vector<Intersection>* intersections) const {
		intersections->clear();

		Cell_handle next_cell =
			triangulation_.locate(ray_segment.start());

		bool next_cell_found = true;
		while (next_cell_found) {
			next_cell_found = false;

			if (triangulation_.is_infinite(next_cell)) {
				// Linearly check all hull facets for intersection.

				for (const auto& hull_facet : hull_facets_) {
					// Check if the ray origin is infront of the facet.
					const K::Triangle_3 triangle = triangulation_.triangle(hull_facet);
					if (CGAL::orientation(
						triangle[0], triangle[1], triangle[2], ray_segment.start()) ==
						K::Orientation::NEGATIVE) {
						continue;
					}

					// Check if the segment intersects the facet.
					K::Point_3 intersection_point;
					if (!CGAL::assign(intersection_point,
						CGAL::intersection(ray_segment, triangle))) {
						continue;
					}

					// Make sure the next intersection is closer to target than previous.
					const double target_distance_squared =
						(intersection_point - ray_segment.end()).squared_length();
					if (!intersections->empty() &&
						intersections->back().target_distance_squared <
						target_distance_squared) {
						continue;
					}

					Intersection intersection;
					intersection.facet =
						Delaunay::Facet(hull_facet.first, hull_facet.second);
					intersection.target_distance_squared = target_distance_squared;
					intersections->push_back(intersection);

					next_cell = hull_facet.first->neighbor(hull_facet.second);
					next_cell_found = true;

					break;
				}
			}
			else {
				// Check all neighboring finite facets for intersection.

				for (int i = 0; i < 4; ++i) {
					// Check if the ray origin is infront of the facet.
					const K::Triangle_3 triangle = triangulation_.triangle(next_cell, i);
					if (CGAL::orientation(
						triangle[0], triangle[1], triangle[2], ray_segment.start()) ==
						K::Orientation::NEGATIVE) {
						continue;
					}

					// Check if the segment intersects the facet.
					K::Point_3 intersection_point;
					if (!CGAL::assign(intersection_point,
						CGAL::intersection(ray_segment, triangle))) {
						continue;
					}

					// Make sure the next intersection is closer to target than previous.
					const double target_distance_squared =
						(intersection_point - ray_segment.end()).squared_length();
					if (!intersections->empty() &&
						intersections->back().target_distance_squared <
						target_distance_squared) {
						continue;
					}

					Intersection intersection;
					intersection.facet = Delaunay::Facet(next_cell, i);
					intersection.target_distance_squared = target_distance_squared;
					intersections->push_back(intersection);

					next_cell = next_cell->neighbor(i);
					next_cell_found = true;

					break;
				}
			}
		}
	}

private:
	// Find all finite facets of infinite cells.
	void FindHullFacets() {
		Vertex_handle inf = triangulation_.infinite_vertex();

		std::vector<Cell_handle> infinite_cells;
		triangulation_.incident_cells(inf, std::back_inserter(infinite_cells));

		for (Cell_handle c : infinite_cells) {
			int i_inf = c->index(inf);
			hull_facets_.emplace_back(c, i_inf);
		}
	}

	Delaunay& triangulation_;
	std::vector<Delaunay::Facet> hull_facets_;
};

double ComputeCosFacetCellAngle(const Delaunay& triangulation,
	const Delaunay::Facet& facet);

RawData CopyReconstruction(Reconstruction& model);
bool GenerateMeshModel(RawData& raw);

using CellGraphData = std::unordered_map<uint64_t, DelaunayCellData>;

using IntersectionVec = std::vector<DelaunayTriangulationRayCaster::Intersection>;