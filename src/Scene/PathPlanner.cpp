#include "PathPlanner.h"

#include "DBSCAN.h"

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glog/logging.h>

namespace {

constexpr float kEpsilon = 1e-6f;
constexpr float kPi = 3.14159265358979323846f;

struct Plane {
	Eigen::Vector3f normal = Eigen::Vector3f::UnitZ();
	Eigen::Vector3f point = Eigen::Vector3f::Zero();
};

struct OrientedBox {
	Eigen::Vector3f origin = Eigen::Vector3f::Zero();
	Eigen::Vector3f u = Eigen::Vector3f::UnitX();
	Eigen::Vector3f v = Eigen::Vector3f::UnitY();
	Eigen::Vector3f n = Eigen::Vector3f::UnitZ();
	Eigen::Vector3f minimum = Eigen::Vector3f::Zero();
	Eigen::Vector3f maximum = Eigen::Vector3f::Zero();
	float scene_diagonal = 1.0f;
};

struct LowQualityFace {
	Eigen::Vector3f center = Eigen::Vector3f::Zero();
	Eigen::Vector3f normal = Eigen::Vector3f::UnitZ();
	float deficit = 0.0f;
};

struct CandidateViewpoint {
	Eigen::Vector3f position = Eigen::Vector3f::Zero();
	Eigen::Vector3f target = Eigen::Vector3f::Zero();
	float priority = 0.0f;
};

Eigen::Vector3f VertexPosition(const PlyMeshVertex& vertex) {
	return Eigen::Vector3f(vertex.x, vertex.y, vertex.z);
}

std::vector<Eigen::Vector3f> CameraCenters(const RawData& raw) {
	std::vector<std::pair<uint64_t, Eigen::Vector3f>> ordered;
	ordered.reserve(raw.images.size());
	for (const auto& [image_id, image] : raw.images) {
		ordered.emplace_back(
			image_id,
			Eigen::Vector3f(image.cx, image.cy, image.cz));
	}
	std::sort(ordered.begin(), ordered.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

	std::vector<Eigen::Vector3f> centers;
	centers.reserve(ordered.size());
	for (const auto& [image_id, center] : ordered) {
		(void)image_id;
		centers.push_back(center);
	}
	return centers;
}

float SceneDiagonal(const PlyMesh& mesh) {
	if (mesh.vertices.empty()) {
		return 1.0f;
	}
	Eigen::Vector3f minimum = Eigen::Vector3f::Constant(
		std::numeric_limits<float>::max());
	Eigen::Vector3f maximum = Eigen::Vector3f::Constant(
		std::numeric_limits<float>::lowest());
	for (const auto& vertex : mesh.vertices) {
		const Eigen::Vector3f point = VertexPosition(vertex);
		minimum = minimum.cwiseMin(point);
		maximum = maximum.cwiseMax(point);
	}
	return std::max((maximum - minimum).norm(), 1.0f);
}

Plane FitBasePlane(
	const PlyMesh& mesh,
	const std::vector<Eigen::Vector3f>& camera_centers,
	const float scene_diagonal) {
	Plane plane;
	if (mesh.vertices.empty()) {
		if (!camera_centers.empty()) {
			plane.point = camera_centers.back();
		}
		return plane;
	}

	std::vector<Eigen::Vector3f> points;
	points.reserve(mesh.vertices.size());
	for (const auto& vertex : mesh.vertices) {
		points.push_back(VertexPosition(vertex));
	}

	Eigen::Vector3f centroid = Eigen::Vector3f::Zero();
	for (const Eigen::Vector3f& point : points) {
		centroid += point;
	}
	centroid /= static_cast<float>(points.size());
	plane.point = centroid;

	const float distance_threshold = std::max(0.008f * scene_diagonal, 1e-3f);
	std::mt19937 generator(0x4f544653U);
	std::uniform_int_distribution<size_t> distribution(0, points.size() - 1);
	size_t best_inliers = 0;
	float best_error = std::numeric_limits<float>::max();

	if (points.size() >= 3) {
		for (int iteration = 0; iteration < 100; ++iteration) {
			const size_t i0 = distribution(generator);
			const size_t i1 = distribution(generator);
			const size_t i2 = distribution(generator);
			if (i0 == i1 || i0 == i2 || i1 == i2) {
				continue;
			}

			Eigen::Vector3f normal =
				(points[i1] - points[i0]).cross(points[i2] - points[i0]);
			const float length = normal.norm();
			if (length <= kEpsilon) {
				continue;
			}
			normal /= length;

			size_t inliers = 0;
			float error = 0.0f;
			for (const Eigen::Vector3f& point : points) {
				const float distance = std::abs((point - points[i0]).dot(normal));
				if (distance <= distance_threshold) {
					++inliers;
					error += distance;
				}
			}
			if (inliers > best_inliers ||
				(inliers == best_inliers && error < best_error)) {
				best_inliers = inliers;
				best_error = error;
				plane.normal = normal;
				plane.point = points[i0];
			}
		}
	}

	if (best_inliers < 3) {
		Eigen::Matrix3f covariance = Eigen::Matrix3f::Zero();
		for (const Eigen::Vector3f& point : points) {
			const Eigen::Vector3f centered = point - centroid;
			covariance.noalias() += centered * centered.transpose();
		}
		Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(covariance);
		if (solver.info() == Eigen::Success) {
			plane.normal = solver.eigenvectors().col(0).normalized();
		}
		plane.point = centroid;
	}

	if (!camera_centers.empty()) {
		float signed_distance_sum = 0.0f;
		for (const Eigen::Vector3f& camera : camera_centers) {
			signed_distance_sum += (camera - plane.point).dot(plane.normal);
		}
		if (signed_distance_sum < 0.0f) {
			plane.normal = -plane.normal;
		}
	}
	else if (plane.normal.z() < 0.0f) {
		plane.normal = -plane.normal;
	}
	return plane;
}

OrientedBox BuildOrientedBox(
	const PlyMesh& mesh,
	const std::vector<Eigen::Vector3f>& camera_centers,
	const Plane& plane,
	const float scene_diagonal) {
	OrientedBox box;
	box.n = plane.normal.normalized();
	const Eigen::Vector3f reference =
		std::abs(box.n.z()) < 0.9f ? Eigen::Vector3f::UnitZ() : Eigen::Vector3f::UnitX();
	box.u = reference.cross(box.n).normalized();
	box.v = box.n.cross(box.u).normalized();
	box.scene_diagonal = scene_diagonal;

	Eigen::Vector3f aabb_minimum = Eigen::Vector3f::Constant(
		std::numeric_limits<float>::max());
	Eigen::Vector3f aabb_maximum = Eigen::Vector3f::Constant(
		std::numeric_limits<float>::lowest());
	for (const auto& vertex : mesh.vertices) {
		const Eigen::Vector3f point = VertexPosition(vertex);
		aabb_minimum = aabb_minimum.cwiseMin(point);
		aabb_maximum = aabb_maximum.cwiseMax(point);
	}
	for (const Eigen::Vector3f& camera : camera_centers) {
		aabb_minimum = aabb_minimum.cwiseMin(camera);
		aabb_maximum = aabb_maximum.cwiseMax(camera);
	}
	const Eigen::Vector3f aabb_center = 0.5f * (aabb_minimum + aabb_maximum);
	box.origin = aabb_center - (aabb_center - plane.point).dot(box.n) * box.n;

	box.minimum = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
	box.maximum = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());
	auto extend = [&box](const Eigen::Vector3f& point) {
		const Eigen::Vector3f relative = point - box.origin;
		const Eigen::Vector3f local(
			relative.dot(box.u), relative.dot(box.v), relative.dot(box.n));
		box.minimum = box.minimum.cwiseMin(local);
		box.maximum = box.maximum.cwiseMax(local);
	};
	for (const auto& vertex : mesh.vertices) {
		extend(VertexPosition(vertex));
	}
	for (const Eigen::Vector3f& camera : camera_centers) {
		extend(camera);
	}

	const Eigen::Vector3f margin =
		(box.maximum - box.minimum).cwiseMax(Eigen::Vector3f::Constant(scene_diagonal * 0.02f)) * 0.05f;
	box.minimum -= margin;
	box.maximum += margin;
	return box;
}

Eigen::Vector3f ToLocal(const OrientedBox& box, const Eigen::Vector3f& point) {
	const Eigen::Vector3f relative = point - box.origin;
	return Eigen::Vector3f(
		relative.dot(box.u), relative.dot(box.v), relative.dot(box.n));
}

Eigen::Vector3f ToWorld(const OrientedBox& box, const Eigen::Vector3f& point) {
	return box.origin + point.x() * box.u + point.y() * box.v + point.z() * box.n;
}

bool IntersectRayWithBox(
	const OrientedBox& box,
	const Eigen::Vector3f& origin,
	const Eigen::Vector3f& direction,
	Eigen::Vector3f& intersection) {
	if (direction.squaredNorm() <= kEpsilon * kEpsilon) {
		return false;
	}
	const Eigen::Vector3f local_origin = ToLocal(box, origin);
	const Eigen::Vector3f normalized = direction.normalized();
	const Eigen::Vector3f local_direction(
		normalized.dot(box.u), normalized.dot(box.v), normalized.dot(box.n));

	float near_distance = 0.0f;
	float far_distance = std::numeric_limits<float>::max();
	for (int axis = 0; axis < 3; ++axis) {
		if (std::abs(local_direction[axis]) <= kEpsilon) {
			if (local_origin[axis] < box.minimum[axis] ||
				local_origin[axis] > box.maximum[axis]) {
				return false;
			}
			continue;
		}
		float first = (box.minimum[axis] - local_origin[axis]) / local_direction[axis];
		float second = (box.maximum[axis] - local_origin[axis]) / local_direction[axis];
		if (first > second) {
			std::swap(first, second);
		}
		near_distance = std::max(near_distance, first);
		far_distance = std::min(far_distance, second);
		if (near_distance > far_distance) {
			return false;
		}
	}

	const float distance = far_distance;
	if (!std::isfinite(distance) || distance <= kEpsilon) {
		return false;
	}
	intersection = ToWorld(box, local_origin + distance * local_direction);
	return true;
}

std::vector<LowQualityFace> ExtractLowQualityFaces(
	const PlyMesh& mesh,
	const std::vector<double>& quality,
	const std::vector<double>& quantiles,
	const Plane& plane,
	const std::vector<Eigen::Vector3f>& camera_centers) {
	std::vector<LowQualityFace> result;
	const size_t face_count = std::min(mesh.faces.size(), quality.size());
	if (face_count == 0) {
		return result;
	}

	std::vector<double> sorted_quality(quality.begin(), quality.begin() + face_count);
	std::sort(sorted_quality.begin(), sorted_quality.end());
	const size_t threshold_index = std::min(
		static_cast<size_t>(0.10 * static_cast<double>(face_count)),
		face_count - 1);
	const double threshold = sorted_quality[threshold_index];

	Eigen::Vector3f camera_centroid = Eigen::Vector3f::Zero();
	for (const Eigen::Vector3f& camera : camera_centers) {
		camera_centroid += camera;
	}
	if (!camera_centers.empty()) {
		camera_centroid /= static_cast<float>(camera_centers.size());
	}

	result.reserve(threshold_index + 1);
	for (size_t index = 0; index < face_count; ++index) {
		const bool is_low_quantile =
			index < quantiles.size() ? quantiles[index] <= 0.10 : quality[index] <= threshold;
		if (!is_low_quantile && quality[index] > threshold) {
			continue;
		}

		const auto& face = mesh.faces[index];
		if (face.vertex_idx1 >= mesh.vertices.size() ||
			face.vertex_idx2 >= mesh.vertices.size() ||
			face.vertex_idx3 >= mesh.vertices.size()) {
			continue;
		}
		const Eigen::Vector3f p0 = VertexPosition(mesh.vertices[face.vertex_idx1]);
		const Eigen::Vector3f p1 = VertexPosition(mesh.vertices[face.vertex_idx2]);
		const Eigen::Vector3f p2 = VertexPosition(mesh.vertices[face.vertex_idx3]);
		Eigen::Vector3f normal = (p1 - p0).cross(p2 - p0);
		if (normal.squaredNorm() <= kEpsilon * kEpsilon) {
			continue;
		}
		normal.normalize();
		const Eigen::Vector3f center = (p0 + p1 + p2) / 3.0f;
		if (!camera_centers.empty()) {
			if (normal.dot(camera_centroid - center) < 0.0f) {
				normal = -normal;
			}
		}
		else if (normal.dot(plane.normal) < 0.0f) {
			normal = -normal;
		}
		result.push_back(LowQualityFace{
			center,
			normal,
			static_cast<float>(std::max(0.0, threshold - quality[index]) + 1e-6)
		});
	}

	// Bound the quadratic DBSCAN implementation while retaining the faces with
	// the largest quality deficit. This keeps each incremental update responsive.
	constexpr size_t kMaximumClusterFaces = 3000;
	if (result.size() > kMaximumClusterFaces) {
		std::nth_element(
			result.begin(),
			result.begin() + kMaximumClusterFaces,
			result.end(),
			[](const LowQualityFace& lhs, const LowQualityFace& rhs) {
				return lhs.deficit > rhs.deficit;
			});
		result.resize(kMaximumClusterFaces);
	}
	return result;
}

std::map<int, std::vector<int>> ClusterLowQualityFaces(
	const std::vector<LowQualityFace>& faces,
	const float scene_diagonal) {
	std::map<int, std::vector<int>> clusters;
	if (faces.empty()) {
		return clusters;
	}
	std::vector<Eigen::Vector3f> centers;
	centers.reserve(faces.size());
	for (const LowQualityFace& face : faces) {
		centers.push_back(face.center);
	}
	const float epsilon = std::max(0.008f * scene_diagonal, 1e-3f);
	const int minimum_points = std::max(
		3,
		std::min(30, static_cast<int>(faces.size() / 200)));
	DBSCAN dbscan(epsilon, minimum_points);
	const std::vector<int> labels = dbscan.fit(centers);
	for (size_t index = 0; index < labels.size(); ++index) {
		if (labels[index] >= 0) {
			clusters[labels[index]].push_back(static_cast<int>(index));
		}
	}

	// Small or very sparse meshes often contain only DBSCAN noise. Keeping a
	// single fallback cluster still yields actionable online guidance.
	if (clusters.empty()) {
		auto& fallback = clusters[0];
		fallback.resize(faces.size());
		std::iota(fallback.begin(), fallback.end(), 0);
	}
	return clusters;
}

bool IsFlyableView(
	const Eigen::Vector3f& viewpoint,
	const Eigen::Vector3f& target,
	const Plane& plane,
	const float scene_diagonal) {
	const float height = (viewpoint - plane.point).dot(plane.normal);
	if (height < -0.01f * scene_diagonal) {
		return false;
	}
	Eigen::Vector3f direction = target - viewpoint;
	if (direction.squaredNorm() <= kEpsilon * kEpsilon) {
		return false;
	}
	direction.normalize();
	const float sine = std::clamp(std::abs(direction.dot(plane.normal)), 0.0f, 1.0f);
	const float elevation = std::asin(sine) * 180.0f / kPi;
	return elevation >= 12.0f && elevation <= 90.0f;
}

std::vector<CandidateViewpoint> GenerateCandidates(
	const std::vector<LowQualityFace>& faces,
	const std::map<int, std::vector<int>>& clusters,
	const Plane& plane,
	const OrientedBox& box) {
	std::vector<CandidateViewpoint> candidates;
	for (const auto& [cluster_id, indices] : clusters) {
		(void)cluster_id;
		if (indices.empty()) {
			continue;
		}
		Eigen::Vector3f centroid = Eigen::Vector3f::Zero();
		float total_weight = 0.0f;
		for (const int index : indices) {
			const float weight = faces[index].deficit;
			centroid += weight * faces[index].center;
			total_weight += weight;
		}
		centroid /= std::max(total_weight, kEpsilon);

		std::vector<int> ordered = indices;
		std::sort(ordered.begin(), ordered.end(),
			[&faces](const int lhs, const int rhs) {
				return faces[lhs].deficit > faces[rhs].deficit;
			});
		const size_t sample_count = std::min<size_t>(ordered.size(), 128);
		for (size_t sample = 0; sample < sample_count; ++sample) {
			const size_t source = sample * ordered.size() / sample_count;
			const LowQualityFace& face = faces[ordered[source]];
			Eigen::Vector3f direction = face.normal;
			if (direction.dot(plane.normal) < -0.25f) {
				direction = -direction;
			}

			Eigen::Vector3f viewpoint;
			bool intersects = IntersectRayWithBox(box, centroid, direction, viewpoint);
			if (!intersects) {
				intersects = IntersectRayWithBox(box, centroid, -direction, viewpoint);
			}
			if (!intersects || !IsFlyableView(viewpoint, centroid, plane, box.scene_diagonal)) {
				continue;
			}
			candidates.push_back(CandidateViewpoint{
				viewpoint,
				centroid,
				face.deficit
			});
		}
	}
	return candidates;
}

std::vector<CandidateViewpoint> SparsifyCandidates(
	const std::vector<CandidateViewpoint>& candidates,
	const float scene_diagonal) {
	if (candidates.empty()) {
		return {};
	}
	const float minimum_spacing = std::max(0.03f * scene_diagonal, 1e-3f);
	const size_t target_size = std::min<size_t>(
		60,
		std::max<size_t>(12, candidates.size() / 8));

	std::vector<bool> used(candidates.size(), false);
	std::vector<CandidateViewpoint> selected;
	selected.reserve(std::min(target_size, candidates.size()));
	const auto first = std::max_element(
		candidates.begin(), candidates.end(),
		[](const CandidateViewpoint& lhs, const CandidateViewpoint& rhs) {
			return lhs.priority < rhs.priority;
		});
	const size_t first_index = static_cast<size_t>(std::distance(candidates.begin(), first));
	selected.push_back(*first);
	used[first_index] = true;

	while (selected.size() < target_size) {
		size_t best_index = candidates.size();
		float best_distance = minimum_spacing;
		for (size_t index = 0; index < candidates.size(); ++index) {
			if (used[index]) {
				continue;
			}
			float minimum_distance = std::numeric_limits<float>::max();
			for (const CandidateViewpoint& chosen : selected) {
				minimum_distance = std::min(
					minimum_distance,
					(candidates[index].position - chosen.position).norm());
			}
			if (minimum_distance > best_distance) {
				best_distance = minimum_distance;
				best_index = index;
			}
		}
		if (best_index == candidates.size()) {
			break;
		}
		selected.push_back(candidates[best_index]);
		used[best_index] = true;
	}
	return selected;
}

float FlightCost(
	const CandidateViewpoint& from,
	const CandidateViewpoint& to,
	const Eigen::Vector3f& vertical) {
	const Eigen::Vector3f delta = to.position - from.position;
	return delta.norm() + 0.3f * std::abs(delta.dot(vertical));
}

std::vector<size_t> OptimizeRoute(
	const std::vector<CandidateViewpoint>& viewpoints,
	const std::vector<Eigen::Vector3f>& camera_centers,
	const Eigen::Vector3f& vertical) {
	if (viewpoints.empty()) {
		return {};
	}
	std::vector<size_t> route;
	route.reserve(viewpoints.size());
	std::vector<bool> visited(viewpoints.size(), false);

	size_t current = 0;
	if (!camera_centers.empty()) {
		const Eigen::Vector3f& current_camera = camera_centers.back();
		float best_distance = std::numeric_limits<float>::max();
		for (size_t index = 0; index < viewpoints.size(); ++index) {
			const float distance = (viewpoints[index].position - current_camera).norm();
			if (distance < best_distance) {
				best_distance = distance;
				current = index;
			}
		}
	}
	route.push_back(current);
	visited[current] = true;
	while (route.size() < viewpoints.size()) {
		size_t next = viewpoints.size();
		float best_cost = std::numeric_limits<float>::max();
		for (size_t index = 0; index < viewpoints.size(); ++index) {
			if (visited[index]) {
				continue;
			}
			const float cost = FlightCost(viewpoints[current], viewpoints[index], vertical);
			if (cost < best_cost) {
				best_cost = cost;
				next = index;
			}
		}
		if (next == viewpoints.size()) {
			break;
		}
		current = next;
		visited[current] = true;
		route.push_back(current);
	}

	if (route.size() >= 4) {
		for (int iteration = 0; iteration < 100; ++iteration) {
			bool improved = false;
			for (size_t first = 1; first + 2 < route.size(); ++first) {
				for (size_t last = first + 1; last + 1 < route.size(); ++last) {
					const float current_cost =
						FlightCost(viewpoints[route[first - 1]], viewpoints[route[first]], vertical) +
						FlightCost(viewpoints[route[last]], viewpoints[route[last + 1]], vertical);
					const float swapped_cost =
						FlightCost(viewpoints[route[first - 1]], viewpoints[route[last]], vertical) +
						FlightCost(viewpoints[route[first]], viewpoints[route[last + 1]], vertical);
					if (swapped_cost + kEpsilon < current_cost) {
						std::reverse(route.begin() + first, route.begin() + last + 1);
						improved = true;
					}
				}
			}
			if (!improved) {
				break;
			}
		}
	}
	return route;
}

void FillRenderData(
	const std::vector<CandidateViewpoint>& viewpoints,
	const std::vector<size_t>& route,
	RenderData& render_data) {
	constexpr std::array<float, 4> kViewpointColor{1.0f, 0.0f, 1.0f, 1.0f};
	constexpr std::array<float, 4> kPathColor{1.0f, 0.647f, 0.0f, 1.0f};
	constexpr std::array<float, 4> kDirectionColor{0.0f, 1.0f, 0.0f, 1.0f};

	for (const CandidateViewpoint& viewpoint : viewpoints) {
		const auto& position = viewpoint.position;
		render_data.guide_points.push_back(PointPainterData{
			position.x(), position.y(), position.z(),
			kViewpointColor[0], kViewpointColor[1], kViewpointColor[2], kViewpointColor[3]
		});
		const Eigen::Vector3f direction = viewpoint.target - position;
		const Eigen::Vector3f endpoint = position + 0.6f * direction;
		render_data.guide_directions.push_back(LinePainterData{
			position.x(), position.y(), position.z(),
			kDirectionColor[0], kDirectionColor[1], kDirectionColor[2], kDirectionColor[3],
			endpoint.x(), endpoint.y(), endpoint.z(),
			kDirectionColor[0], kDirectionColor[1], kDirectionColor[2], kDirectionColor[3]
		});
	}

	for (size_t index = 1; index < route.size(); ++index) {
		const Eigen::Vector3f& from = viewpoints[route[index - 1]].position;
		const Eigen::Vector3f& to = viewpoints[route[index]].position;
		render_data.guide_lines.push_back(LinePainterData{
			from.x(), from.y(), from.z(),
			kPathColor[0], kPathColor[1], kPathColor[2], kPathColor[3],
			to.x(), to.y(), to.z(),
			kPathColor[0], kPathColor[1], kPathColor[2], kPathColor[3]
		});
	}
}

}  // namespace

bool GeneratePredictivePath(
	const RawData& raw,
	const PlyMesh& mesh,
	const std::vector<double>& quality,
	const std::vector<double>& quantiles,
	RenderData& render_data) {
	render_data.guide_points.clear();
	render_data.guide_lines.clear();
	render_data.guide_directions.clear();
	render_data.base_faces.clear();

	if (mesh.vertices.empty() || mesh.faces.empty() || quality.empty()) {
		return false;
	}

	const float scene_diagonal = SceneDiagonal(mesh);
	const std::vector<Eigen::Vector3f> camera_centers = CameraCenters(raw);
	const Plane base_plane = FitBasePlane(mesh, camera_centers, scene_diagonal);
	const OrientedBox bounding_box = BuildOrientedBox(
		mesh, camera_centers, base_plane, scene_diagonal);
	const std::vector<LowQualityFace> low_quality_faces = ExtractLowQualityFaces(
		mesh, quality, quantiles, base_plane, camera_centers);
	const std::map<int, std::vector<int>> clusters = ClusterLowQualityFaces(
		low_quality_faces, scene_diagonal);
	const std::vector<CandidateViewpoint> candidates = GenerateCandidates(
		low_quality_faces, clusters, base_plane, bounding_box);
	const std::vector<CandidateViewpoint> selected = SparsifyCandidates(
		candidates, scene_diagonal);
	const std::vector<size_t> route = OptimizeRoute(
		selected, camera_centers, base_plane.normal);
	FillRenderData(selected, route, render_data);

	LOG(INFO) << "[Path planning] low-quality faces=" << low_quality_faces.size()
		<< ", clusters=" << clusters.size()
		<< ", candidates=" << candidates.size()
		<< ", selected viewpoints=" << selected.size()
		<< ", path segments=" << render_data.guide_lines.size();
	return !selected.empty();
}
