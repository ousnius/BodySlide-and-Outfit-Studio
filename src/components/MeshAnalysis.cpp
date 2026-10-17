/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MeshAnalysis.h"
#include "ClippingFixer.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <unordered_set>

using namespace nifly;

namespace MeshAnalysis {

namespace {

std::vector<std::vector<int>> Neighbors(size_t vertexCount, const std::vector<Triangle>& tris) {
	std::vector<std::unordered_set<int>> sets(vertexCount);
	for (auto& t : tris) {
		const uint16_t p[3] = {t.p1, t.p2, t.p3};
		for (int i = 0; i < 3; i++) {
			if (p[i] >= vertexCount)
				continue;
			for (int j = 0; j < 3; j++)
				if (i != j && p[j] < vertexCount)
					sets[p[i]].insert(p[j]);
		}
	}

	std::vector<std::vector<int>> neighbors(vertexCount);
	for (size_t v = 0; v < vertexCount; v++)
		neighbors[v].assign(sets[v].begin(), sets[v].end());
	return neighbors;
}

} // namespace

PenetrationStats AnalyzePenetration(const std::vector<Vector3>& outfitVerts,
									const std::vector<Vector3>& bodyVerts,
									const std::vector<Triangle>& bodyTris,
									float nearDistance,
									float searchRadius) {
	PenetrationStats stats;
	stats.signedDistances = ClippingFixer::SignedDistancesToBody(bodyVerts, bodyTris, outfitVerts, searchRadius);

	double depthSum = 0.0;
	for (float d : stats.signedDistances) {
		if (d == FLT_MAX)
			continue;

		if (d < 0.0f) {
			stats.penetrating++;
			depthSum += -d;
			stats.maxDepth = std::max(stats.maxDepth, -d);
		}
		else if (d < nearDistance)
			stats.nearSurface++;
	}

	if (stats.penetrating > 0)
		stats.meanDepth = static_cast<float>(depthSum / static_cast<double>(stats.penetrating));

	return stats;
}

MorphStats AnalyzeMorph(const std::vector<Triangle>& tris, const std::vector<Vector3>& deltas, float moveThreshold) {
	MorphStats stats;
	const size_t count = deltas.size();
	auto neighbors = Neighbors(count, tris);

	std::vector<bool> moved(count, false);
	double deltaSum = 0.0;
	for (size_t v = 0; v < count; v++) {
		float length = deltas[v].length();
		if (length > moveThreshold) {
			moved[v] = true;
			stats.movedCount++;
			deltaSum += length;
			stats.maxDelta = std::max(stats.maxDelta, length);
		}
	}

	if (stats.movedCount == 0)
		return stats;

	stats.meanDelta = static_cast<float>(deltaSum / static_cast<double>(stats.movedCount));

	double roughnessSum = 0.0;
	for (size_t v = 0; v < count; v++) {
		if (!moved[v] || neighbors[v].empty())
			continue;

		Vector3 mean;
		bool boundary = false;
		for (int n : neighbors[v]) {
			mean += deltas[n];
			if (!moved[n])
				boundary = true;
		}
		mean /= static_cast<float>(neighbors[v].size());

		float roughness = (deltas[v] - mean).length();
		roughnessSum += roughness;
		if (boundary)
			stats.maxBoundaryRoughness = std::max(stats.maxBoundaryRoughness, roughness);
	}

	stats.roughness = static_cast<float>(roughnessSum / static_cast<double>(stats.movedCount));
	if (stats.meanDelta > 0.0f)
		stats.normalizedRoughness = stats.roughness / stats.meanDelta;

	return stats;
}

size_t CountFlippedTriangles(const std::vector<Vector3>& before, const std::vector<Vector3>& after, const std::vector<Triangle>& tris) {
	size_t flipped = 0;
	for (auto& t : tris) {
		if (t.p1 >= before.size() || t.p2 >= before.size() || t.p3 >= before.size() || t.p1 >= after.size() || t.p2 >= after.size() || t.p3 >= after.size())
			continue;

		Vector3 n1 = (before[t.p2] - before[t.p1]).cross(before[t.p3] - before[t.p1]);
		Vector3 n2 = (after[t.p2] - after[t.p1]).cross(after[t.p3] - after[t.p1]);
		if (n1.dot(n2) < 0.0f)
			flipped++;
	}
	return flipped;
}

WeightStats AnalyzeWeights(size_t vertexCount, const std::vector<const std::unordered_map<uint16_t, float>*>& boneWeights, int influenceLimit, float sumTolerance) {
	WeightStats stats;
	std::vector<float> sums(vertexCount, 0.0f);
	std::vector<int> influences(vertexCount, 0);

	for (auto* weights : boneWeights) {
		if (!weights)
			continue;

		for (auto& w : *weights) {
			if (w.first >= vertexCount || w.second <= 0.0f)
				continue;
			sums[w.first] += w.second;
			influences[w.first]++;
		}
	}

	for (size_t v = 0; v < vertexCount; v++) {
		stats.maxInfluences = std::max(stats.maxInfluences, influences[v]);
		if (influences[v] == 0) {
			stats.unweighted++;
			continue;
		}

		if (influences[v] > influenceLimit)
			stats.overInfluenceLimit++;

		float error = std::fabs(sums[v] - 1.0f);
		if (error > sumTolerance)
			stats.badSum++;
		stats.maxSumError = std::max(stats.maxSumError, error);
	}

	return stats;
}

DeltaComparison CompareDeltas(const std::vector<Vector3>& a, const std::vector<Vector3>& b) {
	DeltaComparison result;
	result.compared = std::min(a.size(), b.size());
	if (result.compared == 0)
		return result;

	double sum = 0.0;
	double sumSq = 0.0;
	for (size_t v = 0; v < result.compared; v++) {
		float error = (a[v] - b[v]).length();
		sum += error;
		sumSq += static_cast<double>(error) * error;
		result.maxError = std::max(result.maxError, error);
	}

	result.meanError = static_cast<float>(sum / static_cast<double>(result.compared));
	result.rmsError = static_cast<float>(std::sqrt(sumSq / static_cast<double>(result.compared)));
	return result;
}

Vector3 HeatmapColor(float t) {
	static const Vector3 ramp[] = {
		Vector3(0.0f, 0.0f, 1.0f),
		Vector3(0.0f, 1.0f, 1.0f),
		Vector3(0.0f, 1.0f, 0.0f),
		Vector3(1.0f, 1.0f, 0.0f),
		Vector3(1.0f, 0.0f, 0.0f),
	};
	constexpr int segments = 4;

	if (!(t > 0.0f))
		return ramp[0];
	if (t >= 1.0f)
		return ramp[segments];

	float scaled = t * segments;
	int segment = std::min(static_cast<int>(scaled), segments - 1);
	float f = scaled - segment;
	return ramp[segment] * (1.0f - f) + ramp[segment + 1] * f;
}

} // namespace MeshAnalysis
