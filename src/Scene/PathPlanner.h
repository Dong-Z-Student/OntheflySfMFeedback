#pragma once

#include "../Base/ply.h"
#include "RenderData.h"
#include "rawio.h"

#include <vector>

// Recomputes the quality-aware predictive route for the current incremental
// mesh snapshot. The output uses the existing guide painters:
//   guide_points     - selected viewpoints
//   guide_lines      - optimized flight path
//   guide_directions - viewpoint-to-low-quality-region viewing directions
bool GeneratePredictivePath(
	const RawData& raw,
	const PlyMesh& mesh,
	const std::vector<double>& quality,
	const std::vector<double>& quantiles,
	RenderData& render_data);
