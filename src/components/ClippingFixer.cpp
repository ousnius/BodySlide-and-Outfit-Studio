/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ClippingFixer.h"

#include "../utils/AABBTree.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <unordered_set>

using namespace nifly;

NiShape* ClippingFixer::FindReferenceShape(NifFile& nif) {
	for (auto& shape : nif.GetShapes()) {
		NiShader* shader = nif.GetShader(shape);
		if (shader && shader->IsSkinTinted())
			return shape;
	}
	return nullptr;
}

std::vector<Vector3> ClippingFixer::ComputeVertexNormals(const std::vector<Vector3>& verts,
														 const std::vector<Triangle>& tris) {
	std::vector<Vector3> normals(verts.size());

	for (auto& tri : tris) {
		if (tri.p1 >= verts.size() || tri.p2 >= verts.size() || tri.p3 >= verts.size())
			continue;

		Vector3 faceNormal = tri.trinormal(verts.data());
		normals[tri.p1] += faceNormal;
		normals[tri.p2] += faceNormal;
		normals[tri.p3] += faceNormal;
	}

	for (auto& n : normals)
		n.Normalize();

	return normals;
}

float ClippingFixer::SignedDistanceToTriangle(const Vector3& point,
											  const Triangle& tri,
											  const Vector3* bodyVerts,
												  Vector3& outClosestPoint) {
	const Vector3& v1 = bodyVerts[tri.p1];
	const Vector3& v2 = bodyVerts[tri.p2];
	const Vector3& v3 = bodyVerts[tri.p3];

	Vector3 triNormal = tri.trinormal(bodyVerts);
	triNormal.Normalize();

	// Project point onto triangle plane
	Vector3 toPoint = point - v1;
	float planeDist = toPoint.dot(triNormal);

	// Project point onto the plane
	Vector3 projected = point - triNormal * planeDist;

	// Check if projected point is inside the triangle using edge tests
	Vector3 edge1 = v2 - v1;
	Vector3 edge2 = v3 - v2;
	Vector3 edge3 = v1 - v3;

	bool inside1 = (projected - v1).dot(edge1.cross(triNormal)) <= 0;
	bool inside2 = (projected - v2).dot(edge2.cross(triNormal)) <= 0;
	bool inside3 = (projected - v3).dot(edge3.cross(triNormal)) <= 0;

	if (inside1 && inside2 && inside3) {
		outClosestPoint = projected;
		return planeDist;
	}

	// Point projects outside — find closest point on edges/vertices
	float minDist = FLT_MAX;
	auto ClosestPointOnSegment = [](const Vector3& p, const Vector3& a, const Vector3& b) -> Vector3 {
		Vector3 ab = b - a;
		float t = (p - a).dot(ab) / ab.dot(ab);
		t = std::max(0.0f, std::min(1.0f, t));
		return a + ab * t;
	};

	Vector3 cp1 = ClosestPointOnSegment(point, v1, v2);
	Vector3 cp2 = ClosestPointOnSegment(point, v2, v3);
	Vector3 cp3 = ClosestPointOnSegment(point, v3, v1);

	float d1 = point.DistanceTo(cp1);
	float d2 = point.DistanceTo(cp2);
	float d3 = point.DistanceTo(cp3);

	if (d1 < minDist) { minDist = d1; outClosestPoint = cp1; }
	if (d2 < minDist) { minDist = d2; outClosestPoint = cp2; }
	if (d3 < minDist) { minDist = d3; outClosestPoint = cp3; }

	return planeDist;
}

void ClippingFixer::BarycentricCoords(const Vector3& p,
									  const Vector3& v1,
									  const Vector3& v2,
									  const Vector3& v3,
									  float& u, float& v, float& w) {
	Vector3 e0 = v2 - v1;
	Vector3 e1 = v3 - v1;
	Vector3 ep = p - v1;
	float d00 = e0.dot(e0);
	float d01 = e0.dot(e1);
	float d11 = e1.dot(e1);
	float d20 = ep.dot(e0);
	float d21 = ep.dot(e1);
	float denom = d00 * d11 - d01 * d01;
	if (std::abs(denom) < 1e-12f) {
		u = v = w = 1.0f / 3.0f;
		return;
	}
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
}

std::vector<std::vector<int>> ClippingFixer::BuildAdjacency(int nVerts, const std::vector<Triangle>& tris) {
	std::vector<std::unordered_set<int>> adjSets(nVerts);
	for (auto& tri : tris) {
		int a = tri.p1, b = tri.p2, c = tri.p3;
		if (a < nVerts && b < nVerts && c < nVerts) {
			adjSets[a].insert(b); adjSets[a].insert(c);
			adjSets[b].insert(a); adjSets[b].insert(c);
			adjSets[c].insert(a); adjSets[c].insert(b);
		}
	}
	std::vector<std::vector<int>> adjacency(nVerts);
	for (int i = 0; i < nVerts; i++)
		adjacency[i].assign(adjSets[i].begin(), adjSets[i].end());
	return adjacency;
}

void ClippingFixer::FixClipping(const std::vector<Vector3>& bodyVerts,
								const std::vector<Triangle>& bodyTris,
								std::vector<Vector3>& outfitVerts,
								const std::vector<Triangle>& outfitTris,
								const ClippingFixOptions& options) {
	if (bodyVerts.empty() || bodyTris.empty() || outfitVerts.empty())
		return;

	float strength = std::max(0.0f, std::min(1.0f, options.strength));
	if (strength <= 0.0f)
		return;

	// Inflation distance: how far the body "inflates" outward
	float inflationDist = 0.05f + strength * 0.25f;

	// Extra margin beyond inflated surface for smooth falloff
	float influenceMargin = inflationDist * 0.5f;
	float totalInfluence = inflationDist + influenceMargin;

	// Determine body normal direction (outward vs inward).
	// trinormal() direction depends on winding order which varies across NIF files.
	Vector3 bodyCentroid;
	for (auto& bv : bodyVerts)
		bodyCentroid += bv;
	bodyCentroid /= static_cast<float>(bodyVerts.size());

	int outwardCount = 0;
	int inwardCount = 0;
	for (auto& tri : bodyTris) {
		if (tri.p1 >= bodyVerts.size() || tri.p2 >= bodyVerts.size() || tri.p3 >= bodyVerts.size())
			continue;
		Vector3 triCenter = (bodyVerts[tri.p1] + bodyVerts[tri.p2] + bodyVerts[tri.p3]) / 3.0f;
		Vector3 toOutward = triCenter - bodyCentroid;
		Vector3 normal = tri.trinormal(bodyVerts.data());
		if (toOutward.dot(normal) > 0.0f)
			outwardCount++;
		else
			inwardCount++;
	}
	float normalSign = (outwardCount >= inwardCount) ? 1.0f : -1.0f;

	// Compute body vertex normals (the "inflation direction" at each vertex)
	std::vector<Vector3> bodyNormals = ComputeVertexNormals(bodyVerts, bodyTris);
	for (auto& n : bodyNormals)
		n = n * normalSign;

	// Build AABBTree for the body mesh
	std::vector<Vector3> bodyVertsCopy = bodyVerts;
	std::vector<Triangle> bodyTrisCopy = bodyTris;
	AABBTree bodyBVH(bodyVertsCopy.data(), bodyTrisCopy.data(), static_cast<uint32_t>(bodyTrisCopy.size()), 100, 2);

	float searchRadius = 2.0f + strength * 3.0f;
	int nOutfitVerts = static_cast<int>(outfitVerts.size());

	// First pass: compute displacement for each outfit vertex (don't apply yet)
	std::vector<Vector3> displacements(nOutfitVerts);

	for (int i = 0; i < nOutfitVerts; i++) {
		Vector3 origin = outfitVerts[i];

		std::vector<IntersectResult> results;
		if (!bodyBVH.IntersectSphere(origin, searchRadius, &results))
			continue;

		// Find the closest body triangle
		float bestDist = FLT_MAX;
		Vector3 bestClosestPoint;
		float bestPlaneDist = 0.0f;
		uint32_t bestFacet = 0;
		bool foundClose = false;

		for (auto& result : results) {
			if (result.HitFacet >= bodyTris.size())
				continue;

			Vector3 closestPoint;
			float planeDist = SignedDistanceToTriangle(origin, bodyTris[result.HitFacet],
												   bodyVerts.data(), closestPoint);
			float dist = origin.DistanceTo(closestPoint);
			if (dist < bestDist) {
				bestDist = dist;
				bestClosestPoint = closestPoint;
				bestPlaneDist = planeDist;
				bestFacet = result.HitFacet;
				foundClose = true;
			}
		}

		if (!foundClose)
			continue;

		float signedDist = bestPlaneDist * normalSign;

		if (signedDist < totalInfluence) {
			// Interpolate the body normal at the closest point using barycentric coords.
			// This gives the smooth "inflation direction" from the body surface.
			const Triangle& closestTri = bodyTris[bestFacet];
			float bu, bv, bw;
			BarycentricCoords(bestClosestPoint,
							  bodyVerts[closestTri.p1],
							  bodyVerts[closestTri.p2],
							  bodyVerts[closestTri.p3],
							  bu, bv, bw);

			// Clamp barycentric coords (closest point may be on edge/vertex)
			bu = std::max(0.0f, bu);
			bv = std::max(0.0f, bv);
			bw = std::max(0.0f, bw);
			float bsum = bu + bv + bw;
			if (bsum > 0.0f) { bu /= bsum; bv /= bsum; bw /= bsum; }

			Vector3 inflationDir = bodyNormals[closestTri.p1] * bu
								 + bodyNormals[closestTri.p2] * bv
								 + bodyNormals[closestTri.p3] * bw;
			inflationDir.Normalize();

			float pushDist;
			if (signedDist < inflationDist) {
				// Inside or too close: push to inflated surface
				pushDist = inflationDist - signedDist;
			}
			else {
				// Influence margin: smooth quadratic falloff
				float t = (signedDist - inflationDist) / influenceMargin;
				float falloff = (1.0f - t) * (1.0f - t);
				pushDist = falloff * inflationDist * 0.3f;
			}

			displacements[i] = inflationDir * pushDist;
		}
	}

	// Propagate displacements through outfit mesh connectivity.
	// This preserves the thickness of thin structures like straps:
	// when inner vertices get pushed, the push carries to outer vertices
	// through mesh edges, moving the whole structure together.
	auto adjacency = BuildAdjacency(nOutfitVerts, outfitTris);
	constexpr int propagationRings = 4;
	constexpr float propagationDecay = 0.85f;

	for (int ring = 0; ring < propagationRings; ring++) {
		std::vector<Vector3> newDisplacements = displacements;

		for (int i = 0; i < nOutfitVerts; i++) {
			float currentLen = displacements[i].length();

			// Find the neighbor with the strongest displacement
			Vector3 bestNeighborDisp;
			float bestNeighborLen = 0.0f;

			for (int adj : adjacency[i]) {
				float adjLen = displacements[adj].length();
				if (adjLen > bestNeighborLen) {
					bestNeighborLen = adjLen;
					bestNeighborDisp = displacements[adj];
				}
			}

			// If a neighbor's decayed displacement exceeds ours, adopt it
			float propagatedLen = bestNeighborLen * propagationDecay;
			if (propagatedLen > currentLen + 0.001f)
				newDisplacements[i] = bestNeighborDisp * propagationDecay;
		}

		displacements = std::move(newDisplacements);
	}

	// Unify displacements for co-located ("welded") vertices.
	// Meshes often have duplicate vertices at the same position with different
	// UVs or normals. Without this, one copy gets displaced while the other
	// stays put, tearing the mesh apart at seams.
	{
		// Sort vertex indices by x coordinate for efficient neighbor search
		std::vector<int> sortedInds(nOutfitVerts);
		for (int i = 0; i < nOutfitVerts; i++)
			sortedInds[i] = i;

		std::sort(sortedInds.begin(), sortedInds.end(),
				  [&outfitVerts](int a, int b) { return outfitVerts[a].x < outfitVerts[b].x; });

		// Determine scale-relative epsilon (same approach as SortingMatcher)
		float scale = 0.0f;
		for (int i = 0; i < nOutfitVerts; i++)
			scale = std::max(scale, std::max(std::fabs(outfitVerts[i].x),
							 std::max(std::fabs(outfitVerts[i].y), std::fabs(outfitVerts[i].z))));
		float epsilon = 0.0001f * 0.01f * scale;

		std::vector<bool> used(nOutfitVerts, false);
		for (int si = 0; si < nOutfitVerts; si++) {
			if (used[si])
				continue;

			// Collect co-located vertices and find strongest displacement
			int bestIdx = sortedInds[si];
			float bestLen = displacements[bestIdx].length();
			std::vector<int> group;

			for (int mi = si + 1; mi < nOutfitVerts; mi++) {
				if (outfitVerts[sortedInds[mi]].x - outfitVerts[sortedInds[si]].x >= epsilon)
					break;
				if (used[mi])
					continue;
				if (std::fabs(outfitVerts[sortedInds[si]].y - outfitVerts[sortedInds[mi]].y) >= epsilon)
					continue;
				if (std::fabs(outfitVerts[sortedInds[si]].z - outfitVerts[sortedInds[mi]].z) >= epsilon)
					continue;

				// Found a co-located vertex
				if (group.empty())
					group.push_back(sortedInds[si]);

				group.push_back(sortedInds[mi]);
				used[mi] = true;

				float len = displacements[sortedInds[mi]].length();
				if (len > bestLen) {
					bestLen = len;
					bestIdx = sortedInds[mi];
				}
			}

			// Apply the strongest displacement to all vertices in the group
			if (!group.empty()) {
				for (int idx : group)
					displacements[idx] = displacements[bestIdx];
			}
		}
	}

	// Apply final displacements
	for (int i = 0; i < nOutfitVerts; i++)
		outfitVerts[i] = outfitVerts[i] + displacements[i];
}
