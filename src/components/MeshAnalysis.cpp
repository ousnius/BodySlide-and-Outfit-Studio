/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MeshAnalysis.h"
#include "ClippingFixer.h"
#include "../utils/AABBTree.h"
#include "../utils/ParallelFor.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <functional>
#include <map>
#include <set>
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
	return FlippedTriangles(before, after, tris).size();
}

std::vector<size_t> FlippedTriangles(const std::vector<Vector3>& before, const std::vector<Vector3>& after, const std::vector<Triangle>& tris) {
	std::vector<size_t> flipped;
	for (size_t ti = 0; ti < tris.size(); ti++) {
		const Triangle& t = tris[ti];
		if (t.p1 >= before.size() || t.p2 >= before.size() || t.p3 >= before.size() || t.p1 >= after.size() || t.p2 >= after.size() || t.p3 >= after.size())
			continue;

		Vector3 n1 = (before[t.p2] - before[t.p1]).cross(before[t.p3] - before[t.p1]);
		Vector3 n2 = (after[t.p2] - after[t.p1]).cross(after[t.p3] - after[t.p1]);

		// Degenerate triangles have no facing to flip
		constexpr float minArea2 = 1e-8f;
		if (n1.length() < minArea2 || n2.length() < minArea2)
			continue;

		if (n1.dot(n2) < 0.0f)
			flipped.push_back(ti);
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

std::vector<float> VertexRoughness(const std::vector<Triangle>& tris, const std::vector<Vector3>& deltas, float moveThreshold) {
	const size_t count = deltas.size();
	auto neighbors = Neighbors(count, tris);

	std::vector<float> roughness(count, 0.0f);
	for (size_t v = 0; v < count; v++) {
		if (deltas[v].length() <= moveThreshold || neighbors[v].empty())
			continue;

		Vector3 mean;
		for (int n : neighbors[v])
			mean += deltas[n];
		mean /= static_cast<float>(neighbors[v].size());
		roughness[v] = (deltas[v] - mean).length();
	}
	return roughness;
}

std::vector<std::vector<int>> CoincidentGroups(const std::vector<Vector3>& points, float tolerance) {
	std::vector<std::vector<int>> groups;
	if (points.empty())
		return groups;

	const float cell = std::max(tolerance, 1e-6f);
	auto key = [cell](const Vector3& p) {
		return std::array<int64_t, 3>{static_cast<int64_t>(std::floor(p.x / cell)), static_cast<int64_t>(std::floor(p.y / cell)), static_cast<int64_t>(std::floor(p.z / cell))};
	};

	std::map<std::array<int64_t, 3>, std::vector<int>> grid;
	for (size_t i = 0; i < points.size(); i++)
		grid[key(points[i])].push_back(static_cast<int>(i));

	const float toleranceSq = tolerance * tolerance;
	std::vector<int> groupOf(points.size(), -1);
	for (size_t i = 0; i < points.size(); i++) {
		if (groupOf[i] >= 0)
			continue;

		std::vector<int> group = {static_cast<int>(i)};
		auto k = key(points[i]);
		for (int64_t dx = -1; dx <= 1; dx++) {
			for (int64_t dy = -1; dy <= 1; dy++) {
				for (int64_t dz = -1; dz <= 1; dz++) {
					auto it = grid.find({k[0] + dx, k[1] + dy, k[2] + dz});
					if (it == grid.end())
						continue;
					for (int j : it->second)
						if (j != static_cast<int>(i) && groupOf[j] < 0 && points[i].DistanceSquaredTo(points[j]) <= toleranceSq)
							group.push_back(j);
				}
			}
		}

		if (group.size() < 2)
			continue;

		for (int j : group)
			groupOf[j] = static_cast<int>(groups.size());
		groups.push_back(std::move(group));
	}

	return groups;
}

std::vector<int> ConnectedParts(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float weldDistance, const std::vector<bool>* include, int* outPartCount) {
	const size_t count = verts.size();
	auto included = [&](size_t v) { return v < count && (!include || (v < include->size() && (*include)[v])); };

	std::vector<int> parent(count);
	for (size_t v = 0; v < count; v++)
		parent[v] = static_cast<int>(v);

	auto find = [&](int v) {
		while (parent[v] != v) {
			parent[v] = parent[parent[v]];
			v = parent[v];
		}
		return v;
	};
	auto join = [&](int a, int b) {
		a = find(a);
		b = find(b);
		if (a != b)
			parent[b] = a;
	};

	for (auto& t : tris) {
		if (!included(t.p1) || !included(t.p2) || !included(t.p3))
			continue;
		join(t.p1, t.p2);
		join(t.p1, t.p3);
	}

	if (weldDistance > 0.0f) {
		std::vector<int> indices;
		std::vector<Vector3> points;
		for (size_t v = 0; v < count; v++) {
			if (included(v)) {
				indices.push_back(static_cast<int>(v));
				points.push_back(verts[v]);
			}
		}

		for (auto& group : CoincidentGroups(points, weldDistance))
			for (size_t i = 1; i < group.size(); i++)
				join(indices[group[0]], indices[group[i]]);
	}

	// Number the parts by size, largest first, and parts of the same size by their first vertex
	std::unordered_map<int, size_t> sizes;
	std::unordered_map<int, int> firstVertex;
	for (size_t v = 0; v < count; v++) {
		if (!included(v))
			continue;
		int root = find(static_cast<int>(v));
		sizes[root]++;
		firstVertex.emplace(root, static_cast<int>(v));
	}

	std::vector<std::pair<size_t, int>> order;
	for (auto& s : sizes)
		order.emplace_back(s.second, s.first);
	std::sort(order.begin(), order.end(), [&](const auto& a, const auto& b) {
		return a.first != b.first ? a.first > b.first : firstVertex[a.second] < firstVertex[b.second];
	});

	std::unordered_map<int, int> number;
	for (size_t i = 0; i < order.size(); i++)
		number[order[i].second] = static_cast<int>(i);

	std::vector<int> parts(count, -1);
	for (size_t v = 0; v < count; v++)
		if (included(v))
			parts[v] = number[find(static_cast<int>(v))];

	if (outPartCount)
		*outPartCount = static_cast<int>(order.size());
	return parts;
}

Vector3 SimilarityTransform::Apply(const Vector3& p) const {
	double out[3];
	for (int r = 0; r < 3; r++)
		out[r] = scale * (rotation[r][0] * p.x + rotation[r][1] * p.y + rotation[r][2] * p.z);
	return Vector3(static_cast<float>(out[0]), static_cast<float>(out[1]), static_cast<float>(out[2])) + translation;
}

namespace {

// Eigenvector of the largest eigenvalue of a symmetric 4x4 matrix, by Jacobi rotations
void LargestEigenvector(double a[4][4], double outVector[4]) {
	double v[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

	for (int sweep = 0; sweep < 50; sweep++) {
		double offDiagonal = 0.0;
		double diagonal = 0.0;
		for (int p = 0; p < 4; p++) {
			diagonal += a[p][p] * a[p][p];
			for (int q = p + 1; q < 4; q++)
				offDiagonal += a[p][q] * a[p][q];
		}
		if (offDiagonal <= 1e-24 * diagonal || offDiagonal < 1e-300)
			break;

		for (int p = 0; p < 4; p++) {
			for (int q = p + 1; q < 4; q++) {
				if (a[p][q] == 0.0)
					continue;

				double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
				double t = (theta >= 0.0 ? 1.0 : -1.0) / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
				double c = 1.0 / std::sqrt(t * t + 1.0);
				double s = t * c;

				for (int k = 0; k < 4; k++) {
					double akp = a[k][p];
					double akq = a[k][q];
					a[k][p] = c * akp - s * akq;
					a[k][q] = s * akp + c * akq;
				}
				for (int k = 0; k < 4; k++) {
					double apk = a[p][k];
					double aqk = a[q][k];
					a[p][k] = c * apk - s * aqk;
					a[q][k] = s * apk + c * aqk;
				}
				for (int k = 0; k < 4; k++) {
					double vkp = v[k][p];
					double vkq = v[k][q];
					v[k][p] = c * vkp - s * vkq;
					v[k][q] = s * vkp + c * vkq;
				}
			}
		}
	}

	int largest = 0;
	for (int i = 1; i < 4; i++)
		if (a[i][i] > a[largest][largest])
			largest = i;
	for (int k = 0; k < 4; k++)
		outVector[k] = v[k][largest];
}

// Barycentric coordinates of a point on the plane of a triangle, clamped onto the triangle
void Barycentric(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c, float& u, float& v, float& w) {
	Vector3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = v0.dot(v0), d01 = v0.dot(v1), d11 = v1.dot(v1), d20 = v2.dot(v0), d21 = v2.dot(v1);
	float denom = d00 * d11 - d01 * d01;
	if (std::fabs(denom) < 1e-20f) {
		u = 1.0f;
		v = w = 0.0f;
		return;
	}
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;

	u = std::max(0.0f, u);
	v = std::max(0.0f, v);
	w = std::max(0.0f, w);
	float sum = u + v + w;
	if (sum > 0.0f) {
		u /= sum;
		v /= sum;
		w /= sum;
	}
}

} // namespace

SimilarityTransform BestFitTransform(const std::vector<Vector3>& from, const std::vector<Vector3>& to, const std::vector<float>* weights, bool allowScale) {
	SimilarityTransform result;
	const size_t count = std::min(from.size(), to.size());
	auto weightOf = [weights](size_t i) { return weights && i < weights->size() ? static_cast<double>((*weights)[i]) : 1.0; };

	double weightSum = 0.0;
	double cf[3] = {0, 0, 0};
	double ct[3] = {0, 0, 0};
	for (size_t i = 0; i < count; i++) {
		double w = weightOf(i);
		if (w <= 0.0)
			continue;
		weightSum += w;
		cf[0] += w * from[i].x;
		cf[1] += w * from[i].y;
		cf[2] += w * from[i].z;
		ct[0] += w * to[i].x;
		ct[1] += w * to[i].y;
		ct[2] += w * to[i].z;
	}
	if (weightSum <= 0.0)
		return result;
	for (int k = 0; k < 3; k++) {
		cf[k] /= weightSum;
		ct[k] /= weightSum;
	}

	// Cross covariance of the centered points, and the spread of the from points for the scale
	double s[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	double spread = 0.0;
	for (size_t i = 0; i < count; i++) {
		double w = weightOf(i);
		if (w <= 0.0)
			continue;
		double p[3] = {from[i].x - cf[0], from[i].y - cf[1], from[i].z - cf[2]};
		double q[3] = {to[i].x - ct[0], to[i].y - ct[1], to[i].z - ct[2]};
		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 3; c++)
				s[r][c] += w * p[r] * q[c];
		spread += w * (p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
	}

	// Horn's method: the rotation is the unit quaternion of the largest eigenvalue of this matrix
	double n[4][4] = {
		{s[0][0] + s[1][1] + s[2][2], s[1][2] - s[2][1], s[2][0] - s[0][2], s[0][1] - s[1][0]},
		{s[1][2] - s[2][1], s[0][0] - s[1][1] - s[2][2], s[0][1] + s[1][0], s[2][0] + s[0][2]},
		{s[2][0] - s[0][2], s[0][1] + s[1][0], -s[0][0] + s[1][1] - s[2][2], s[1][2] + s[2][1]},
		{s[0][1] - s[1][0], s[2][0] + s[0][2], s[1][2] + s[2][1], -s[0][0] - s[1][1] + s[2][2]},
	};

	double quat[4];
	LargestEigenvector(n, quat);
	double length = std::sqrt(quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3]);
	if (length > 0.0) {
		double qw = quat[0] / length, qx = quat[1] / length, qy = quat[2] / length, qz = quat[3] / length;
		double(&r)[3][3] = result.rotation;
		r[0][0] = 1 - 2 * (qy * qy + qz * qz);
		r[0][1] = 2 * (qx * qy - qw * qz);
		r[0][2] = 2 * (qx * qz + qw * qy);
		r[1][0] = 2 * (qx * qy + qw * qz);
		r[1][1] = 1 - 2 * (qx * qx + qz * qz);
		r[1][2] = 2 * (qy * qz - qw * qx);
		r[2][0] = 2 * (qx * qz - qw * qy);
		r[2][1] = 2 * (qy * qz + qw * qx);
		r[2][2] = 1 - 2 * (qx * qx + qy * qy);
	}

	if (allowScale && spread > 1e-12) {
		// The sum of q . (R p) over the spread of p
		double dot = 0.0;
		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 3; c++)
				dot += result.rotation[r][c] * s[c][r];
		if (dot > 0.0)
			result.scale = dot / spread;
	}

	double moved[3];
	for (int r = 0; r < 3; r++)
		moved[r] = result.scale * (result.rotation[r][0] * cf[0] + result.rotation[r][1] * cf[1] + result.rotation[r][2] * cf[2]);
	result.translation = Vector3(static_cast<float>(ct[0] - moved[0]), static_cast<float>(ct[1] - moved[1]), static_cast<float>(ct[2] - moved[2]));
	return result;
}

std::vector<Vector3> RigidifyDeltas(const std::vector<Vector3>& verts, const std::vector<Vector3>& deltas, const std::vector<int>& parts, const std::vector<float>& influence, bool allowScale) {
	std::vector<Vector3> result = deltas;
	const size_t count = std::min({verts.size(), deltas.size(), parts.size(), influence.size()});

	std::map<int, std::vector<size_t>> members;
	for (size_t v = 0; v < count; v++)
		if (parts[v] >= 0 && influence[v] > 0.0f)
			members[parts[v]].push_back(v);

	for (auto& part : members) {
		std::vector<Vector3> from, to;
		std::vector<float> weights;
		for (size_t v : part.second) {
			from.push_back(verts[v]);
			to.push_back(verts[v] + deltas[v]);
			weights.push_back(influence[v]);
		}

		SimilarityTransform fit = BestFitTransform(from, to, &weights, allowScale);
		for (size_t v : part.second) {
			Vector3 rigid = fit.Apply(verts[v]) - verts[v];
			const float f = std::min(influence[v], 1.0f);
			result[v] = deltas[v] * (1.0f - f) + rigid * f;
		}
	}

	return result;
}

std::vector<SurfacePoint> ClosestSurfacePoints(const std::vector<Vector3>& surfaceVerts, const std::vector<Triangle>& surfaceTris, const std::vector<Vector3>& points, float searchRadius) {
	std::vector<SurfacePoint> result(points.size());
	if (surfaceVerts.empty() || surfaceTris.empty() || points.empty())
		return result;

	AABBTree tree(const_cast<Vector3*>(surfaceVerts.data()), const_cast<Triangle*>(surfaceTris.data()), static_cast<uint32_t>(surfaceTris.size()), 100, 2);
	std::vector<AABBTriangleDistanceData> distanceData;
	AABBTree::BuildTriangleDistanceData(surfaceVerts.data(), surfaceTris.data(), static_cast<uint32_t>(surfaceTris.size()), distanceData);

	ParallelFor(points.size(), 256, [&](size_t startIndex, size_t endIndex) {
		for (size_t i = startIndex; i < endIndex; i++) {
			uint32_t facet = 0;
			Vector3 closest;
			Vector3 point = points[i];
			if (!tree.ClosestFacetInSphere(point, searchRadius, facet, &closest, &distanceData) || facet >= surfaceTris.size())
				continue;

			const Triangle& t = surfaceTris[facet];
			if (t.p1 >= surfaceVerts.size() || t.p2 >= surfaceVerts.size() || t.p3 >= surfaceVerts.size())
				continue;

			SurfacePoint& sp = result[i];
			sp.triangle = static_cast<int>(facet);
			Barycentric(closest, surfaceVerts[t.p1], surfaceVerts[t.p2], surfaceVerts[t.p3], sp.u, sp.v, sp.w);
			sp.distance = points[i].DistanceTo(closest);
		}
	});

	return result;
}

std::vector<float> WeightDeviation(const std::vector<SurfacePoint>& closest, const std::vector<Triangle>& bodyTris, const BoneWeightMap& bodyWeights, const BoneWeightMap& pointWeights) {
	auto weightOf = [](const std::unordered_map<uint16_t, float>* weights, uint16_t v) {
		if (!weights)
			return 0.0f;
		auto it = weights->find(v);
		return it != weights->end() ? it->second : 0.0f;
	};

	std::set<std::string> bones;
	for (auto& b : bodyWeights)
		bones.insert(b.first);
	for (auto& b : pointWeights)
		bones.insert(b.first);

	std::vector<float> deviation(closest.size(), -1.0f);
	for (size_t i = 0; i < closest.size(); i++) {
		const SurfacePoint& sp = closest[i];
		if (sp.triangle < 0 || static_cast<size_t>(sp.triangle) >= bodyTris.size())
			continue;

		const Triangle& t = bodyTris[sp.triangle];
		float difference = 0.0f;
		for (auto& bone : bones) {
			float bodyWeight = 0.0f;
			auto body = bodyWeights.find(bone);
			if (body != bodyWeights.end())
				bodyWeight = weightOf(body->second, t.p1) * sp.u + weightOf(body->second, t.p2) * sp.v + weightOf(body->second, t.p3) * sp.w;

			float pointWeight = 0.0f;
			auto point = pointWeights.find(bone);
			if (point != pointWeights.end())
				pointWeight = weightOf(point->second, static_cast<uint16_t>(i));

			difference += std::fabs(bodyWeight - pointWeight);
		}
		deviation[i] = std::min(1.0f, difference * 0.5f);
	}

	return deviation;
}

StrainStats AnalyzeStrain(const std::vector<Vector3>& before, const std::vector<Vector3>& after, const std::vector<Triangle>& tris, float threshold) {
	StrainStats stats;
	const size_t count = std::min(before.size(), after.size());
	stats.vertexStrain.assign(before.size(), 0.0f);

	std::set<std::pair<uint16_t, uint16_t>> edges;
	for (auto& t : tris) {
		const uint16_t p[3] = {t.p1, t.p2, t.p3};
		for (int i = 0; i < 3; i++) {
			uint16_t a = p[i];
			uint16_t b = p[(i + 1) % 3];
			if (a >= count || b >= count || a == b)
				continue;
			edges.insert({std::min(a, b), std::max(a, b)});
		}
	}

	double sum = 0.0;
	for (auto& e : edges) {
		const float lengthBefore = before[e.first].DistanceTo(before[e.second]);
		const float lengthAfter = after[e.first].DistanceTo(after[e.second]);
		// Edges without length have no direction to stretch in
		if (lengthBefore < 1e-5f || lengthAfter < 1e-5f)
			continue;

		const float strain = std::max(lengthAfter / lengthBefore, lengthBefore / lengthAfter) - 1.0f;
		stats.edges++;
		sum += strain;
		if (strain > threshold)
			stats.stretched++;
		stats.maxStrain = std::max(stats.maxStrain, strain);
		stats.vertexStrain[e.first] = std::max(stats.vertexStrain[e.first], strain);
		stats.vertexStrain[e.second] = std::max(stats.vertexStrain[e.second], strain);
	}
	stats.meanStrain = stats.edges ? static_cast<float>(sum / stats.edges) : 0.0f;
	return stats;
}

DenseWeights SmoothWeights(const DenseWeights& weights,
						   const std::vector<std::vector<int>>& neighbors,
						   const std::vector<float>& influence,
						   int iterations,
						   float strength,
						   const std::vector<bool>* bones) {
	DenseWeights current = weights;
	if (current.empty())
		return current;

	const size_t boneCount = current.front().size();
	auto smoothed = [&](size_t b) { return !bones || (b < bones->size() && (*bones)[b]); };
	auto total = [&](const std::vector<float>& w) {
		float sum = 0.0f;
		for (size_t b = 0; b < boneCount; b++)
			if (smoothed(b))
				sum += w[b];
		return sum;
	};

	// Each vertex keeps the total it had on the blended bones, so the others aren't crowded out
	std::vector<float> totals(current.size());
	for (size_t v = 0; v < current.size(); v++)
		totals[v] = total(current[v]);

	std::vector<float> mean(boneCount);
	for (int i = 0; i < iterations; i++) {
		DenseWeights next = current;
		for (size_t v = 0; v < current.size() && v < neighbors.size(); v++) {
			const float f = strength * (influence.empty() ? 1.0f : (v < influence.size() ? influence[v] : 0.0f));
			if (f <= 0.0f || totals[v] <= 0.0f)
				continue;

			// Unweighted neighbors have nothing to blend in
			std::fill(mean.begin(), mean.end(), 0.0f);
			int used = 0;
			for (int n : neighbors[v]) {
				if (n < 0 || static_cast<size_t>(n) >= current.size())
					continue;
				const float neighborTotal = total(current[n]);
				if (neighborTotal <= 0.0f)
					continue;
				// Scaled to this vertex's total, so a neighbor with more weight elsewhere doesn't count less
				for (size_t b = 0; b < boneCount; b++)
					if (smoothed(b))
						mean[b] += current[n][b] * totals[v] / neighborTotal;
				used++;
			}
			if (used == 0)
				continue;

			for (size_t b = 0; b < boneCount; b++)
				if (smoothed(b))
					next[v][b] = current[v][b] + (mean[b] / used - current[v][b]) * f;
		}
		current.swap(next);
	}
	return current;
}

bool LimitWeights(std::vector<float>& weights, int maxInfluences, float pruneBelow) {
	const std::vector<float> original = weights;

	size_t largest = 0;
	for (size_t b = 0; b < weights.size(); b++)
		if (weights[b] > weights[largest])
			largest = b;
	if (weights.empty() || weights[largest] <= 0.0f)
		return false;

	for (size_t b = 0; b < weights.size(); b++)
		if (weights[b] < pruneBelow && b != largest)
			weights[b] = 0.0f;

	if (maxInfluences > 0) {
		std::vector<size_t> order;
		for (size_t b = 0; b < weights.size(); b++)
			if (weights[b] > 0.0f)
				order.push_back(b);
		if (order.size() > static_cast<size_t>(maxInfluences)) {
			std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return weights[a] > weights[b]; });
			for (size_t i = maxInfluences; i < order.size(); i++)
				weights[order[i]] = 0.0f;
		}
	}

	float sum = 0.0f;
	for (float w : weights)
		sum += std::max(w, 0.0f);
	for (auto& w : weights)
		w = std::max(w, 0.0f) / sum;

	for (size_t b = 0; b < weights.size(); b++)
		if (std::fabs(weights[b] - original[b]) > 1e-6f)
			return true;
	return false;
}

std::vector<float> WeightJump(const DenseWeights& weights, const std::vector<std::vector<int>>& neighbors) {
	std::vector<float> jump(weights.size(), 0.0f);
	for (size_t v = 0; v < weights.size() && v < neighbors.size(); v++) {
		for (int n : neighbors[v]) {
			if (n < 0 || static_cast<size_t>(n) >= weights.size())
				continue;
			float difference = 0.0f;
			for (size_t b = 0; b < weights[v].size() && b < weights[n].size(); b++)
				difference += std::fabs(weights[v][b] - weights[n][b]);
			jump[v] = std::max(jump[v], std::min(1.0f, difference * 0.5f));
		}
	}
	return jump;
}

} // namespace MeshAnalysis
