/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MaskTransfer.h"
#include "../utils/AABBTree.h"

#include <algorithm>

using namespace nifly;

void MaskTransfer::CopyMask(const std::vector<Vector3>& sourceVerts,
							const std::vector<Triangle>& sourceTris,
							const std::vector<float>& sourceMask,
							const std::vector<Vector3>& targetVerts,
							float maxDistance,
							std::vector<float>& outMask,
							std::vector<bool>& outMatched) {
	outMask.assign(targetVerts.size(), 0.0f);
	outMatched.assign(targetVerts.size(), false);

	const size_t nSourceVerts = sourceVerts.size();
	if (nSourceVerts == 0 || sourceMask.size() < nSourceVerts || maxDistance < 0.0f)
		return;

	// Triangles referencing vertices out of range would make the tree read past the vertex array
	std::vector<Triangle> tris;
	tris.reserve(sourceTris.size());
	for (const Triangle& t : sourceTris)
		if (t.p1 < nSourceVerts && t.p2 < nSourceVerts && t.p3 < nSourceVerts)
			tris.push_back(t);

	if (tris.empty())
		return;

	// The tree stays empty with fewer facets than its minimum leaf size
	const uint32_t nTris = static_cast<uint32_t>(tris.size());
	AABBTree bvh(const_cast<Vector3*>(sourceVerts.data()), tris.data(), nTris, 100, std::min<uint32_t>(nTris, 2));
	std::vector<AABBTriangleDistanceData> triDistanceData;
	AABBTree::BuildTriangleDistanceData(sourceVerts.data(), tris.data(), nTris, triDistanceData);

	for (size_t i = 0; i < targetVerts.size(); i++) {
		Vector3 origin = targetVerts[i];
		uint32_t facet = 0;
		Vector3 closestPoint;
		if (!bvh.ClosestFacetInSphere(origin, maxDistance, facet, &closestPoint, &triDistanceData) || facet >= tris.size())
			continue;

		const Triangle& t = tris[facet];
		uint16_t corner = t.p1;
		float cornerDist = closestPoint.DistanceSquaredTo(sourceVerts[t.p1]);
		for (uint16_t p : {t.p2, t.p3}) {
			float dist = closestPoint.DistanceSquaredTo(sourceVerts[p]);
			if (dist < cornerDist) {
				cornerDist = dist;
				corner = p;
			}
		}

		outMask[i] = sourceMask[corner];
		outMatched[i] = true;
	}
}
