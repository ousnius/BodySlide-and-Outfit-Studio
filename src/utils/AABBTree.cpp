/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AABBTree.h"

using namespace nifly;

AABB::AABB(const Vector3& newMin, const Vector3& newMax) {
	min = newMin;
	max = newMax;
}

AABB::AABB(const Vector3* points, const uint16_t nPoints) {
	if (nPoints <= 0)
		return;

	min = points[0];
	max = points[0];
	for (uint16_t i = 1; i < nPoints; i++) {
		if (points[i].x < min.x)
			min.x = points[i].x;
		if (points[i].y < min.y)
			min.y = points[i].y;
		if (points[i].z < min.z)
			min.z = points[i].z;

		if (points[i].x > max.x)
			max.x = points[i].x;
		if (points[i].y > max.y)
			max.y = points[i].y;
		if (points[i].z > max.z)
			max.z = points[i].z;
	}
}

AABB::AABB(const Vector3* points, const uint16_t* indices, const uint16_t nPoints) {
	if (nPoints <= 0)
		return;

	min = points[indices[0]];
	max = points[indices[0]];
	uint16_t idx;
	for (uint16_t i = 1; i < nPoints; i++) {
		idx = indices[i];
		if (points[idx].x < min.x)
			min.x = points[idx].x;
		if (points[idx].y < min.y)
			min.y = points[idx].y;
		if (points[idx].z < min.z)
			min.z = points[idx].z;

		if (points[idx].x > max.x)
			max.x = points[idx].x;
		if (points[idx].y > max.y)
			max.y = points[idx].y;
		if (points[idx].z > max.z)
			max.z = points[idx].z;
	}
}

void AABB::AddBoxToMesh(std::vector<Vector3>& verts, std::vector<Edge>& edges) {
	Vector3 v;
	Edge e;
	uint16_t s = static_cast<uint16_t>(verts.size());

	v.x = min.x;
	v.y = min.y;
	v.z = min.z;
	verts.push_back(v);
	v.z = max.z;
	verts.push_back(v);
	v.y = max.y;
	verts.push_back(v);
	v.z = min.z;
	verts.push_back(v);

	v.x = max.x;
	verts.push_back(v);
	v.y = min.y;
	verts.push_back(v);
	v.z = max.z;
	verts.push_back(v);
	v.y = max.y;
	verts.push_back(v);

	e.p1 = s;
	e.p2 = s + 1;
	edges.push_back(e);
	e.p1 = s + 1;
	e.p2 = s + 2;
	edges.push_back(e);
	e.p1 = s + 2;
	e.p2 = s + 3;
	edges.push_back(e);
	e.p1 = s + 3;
	e.p2 = s;
	edges.push_back(e);
	e.p1 = s + 3;
	e.p2 = s + 4;
	edges.push_back(e);
	e.p1 = s + 4;
	e.p2 = s + 5;
	edges.push_back(e);
	e.p1 = s + 5;
	e.p2 = s + 6;
	edges.push_back(e);
	e.p1 = s + 6;
	e.p2 = s + 7;
	edges.push_back(e);
	e.p1 = s;
	e.p2 = s + 5;
	edges.push_back(e);
	e.p1 = s + 1;
	e.p2 = s + 6;
	edges.push_back(e);
	e.p1 = s + 2;
	e.p2 = s + 7;
	edges.push_back(e);
	e.p1 = s + 7;
	e.p2 = s + 4;
	edges.push_back(e);
}

void AABB::Merge(const Vector3* points, const uint16_t* indices, const uint16_t nPoints) {
	if (nPoints <= 0)
		return;

	uint16_t idx;
	for (uint16_t i = 0; i < nPoints; i++) {
		idx = indices[i];
		if (points[idx].x < min.x)
			min.x = points[idx].x;
		if (points[idx].y < min.y)
			min.y = points[idx].y;
		if (points[idx].z < min.z)
			min.z = points[idx].z;

		if (points[idx].x > max.x)
			max.x = points[idx].x;
		if (points[idx].y > max.y)
			max.y = points[idx].y;
		if (points[idx].z > max.z)
			max.z = points[idx].z;
	}
}

void AABB::Merge(const AABB& other) {
	if (other.min.x < min.x)
		min.x = other.min.x;
	if (other.min.y < min.y)
		min.y = other.min.y;
	if (other.min.z < min.z)
		min.z = other.min.z;

	if (other.max.x > max.x)
		max.x = other.max.x;
	if (other.max.y > max.y)
		max.y = other.max.y;
	if (other.max.z > max.z)
		max.z = other.max.z;
}

bool AABB::IntersectAABB(const AABB& other) {
	if (min.x > other.max.x)
		return false;
	if (min.y > other.max.y)
		return false;
	if (min.z > other.max.z)
		return false;
	if (max.x < other.min.x)
		return false;
	if (max.y < other.min.y)
		return false;
	if (max.z < other.min.z)
		return false;
	return true;
}

bool AABB::IntersectRay(const Vector3& Origin, const Vector3& Direction, Vector3* outCoord) {
	//char side[3];  // side of potential collision plane: 0=left 1=right 2 = middle
	float candidatePlane[3]; // x,y,z values for potential candidate plane intersection.
	Vector3 maxT(-1, -1, -1);
	Vector3 collisionCoord;
	bool inside = true;
	char axis = 0;
	float planeval = -1;

	// X Axis candidacy
	if (Origin.x < min.x) {
		inside = false;
		candidatePlane[0] = min.x;
		if (Direction.x != 0.0f) {
			maxT.x = (min.x - Origin.x) / Direction.x;
		}
	}
	else if (Origin.x > max.x) {
		inside = false;
		candidatePlane[0] = max.x;
		if (Direction.x != 0.0f) {
			maxT.x = (max.x - Origin.x) / Direction.x;
		}
	}
	planeval = maxT.x;
	// Y Axis candidacy
	if (Origin.y < min.y) {
		inside = false;
		candidatePlane[1] = min.y;
		if (Direction.y != 0.0f) {
			maxT.y = (min.y - Origin.y) / Direction.y;
		}
	}
	else if (Origin.y > max.y) {
		inside = false;
		candidatePlane[1] = max.y;
		if (Direction.y != 0.0f) {
			maxT.y = (max.y - Origin.y) / Direction.y;
		}
	}
	if (maxT.y > planeval) {
		planeval = maxT.y;
		axis = 1;
	}
	// Z Axis candidacy
	if (Origin.z < min.z) {
		inside = false;
		candidatePlane[2] = min.z;
		if (Direction.z != 0.0f) {
			maxT.z = (min.z - Origin.z) / Direction.z;
		}
	}
	else if (Origin.z > max.z) {
		inside = false;
		candidatePlane[2] = max.z;
		if (Direction.z != 0.0f) {
			maxT.z = (max.z - Origin.z) / Direction.z;
		}
	}
	if (maxT.z > planeval) {
		planeval = maxT.z;
		axis = 2;
	}

	if (inside) {
		if (outCoord)
			(*outCoord) = Origin;
		return true;
	}
	if (planeval < 0)
		return false;

	if (axis == 0) {
		collisionCoord.x = planeval;
	}
	else {
		collisionCoord.x = Origin.x + planeval * Direction.x;
		if (collisionCoord.x < min.x || collisionCoord.x > max.x)
			return false;
	}
	if (axis == 1) {
		collisionCoord.y = planeval;
	}
	else {
		collisionCoord.y = Origin.y + planeval * Direction.y;
		if (collisionCoord.y < min.y || collisionCoord.y > max.y)
			return false;
	}
	if (axis == 2) {
		collisionCoord.z = planeval;
	}
	else {
		collisionCoord.z = Origin.z + planeval * Direction.z;
		if (collisionCoord.z < min.z || collisionCoord.z > max.z)
			return false;
	}

	if (outCoord)
		(*outCoord) = collisionCoord;
	return true;
}

bool AABB::IntersectSphere(const Vector3& Origin, const float radius) {
	float s, d = 0;

	if (Origin.x < min.x) {
		s = Origin.x - min.x;
		d += s * s;
	}
	else if (Origin.x > max.x) {
		s = Origin.x - max.x;
		d += s * s;
	}
	if (Origin.y < min.y) {
		s = Origin.y - min.y;
		d += s * s;
	}
	else if (Origin.y > max.y) {
		s = Origin.y - max.y;
		d += s * s;
	}
	if (Origin.z < min.z) {
		s = Origin.z - min.z;
		d += s * s;
	}
	else if (Origin.z > max.z) {
		s = Origin.z - max.z;
		d += s * s;
	}

	return d <= radius * radius;
}

AABBTree::AABBTreeNode::AABBTreeNode(
	std::vector<uint32_t>& facetIndices, const uint32_t start, const uint32_t end, AABBTree* treeRef, AABBTreeNode* parentRef, const uint32_t depth) {
	int axis;
	Vector3 axis_avg;
	tree = treeRef;
	parent = parentRef;
	uint32_t moreStart = 0;

	if (facetIndices.size() < treeRef->MinFacets())
		return;

	// calculate this node's containing AABB and the geometric average
	treeRef->CalcAABBandGeoAvg(facetIndices, start, end, mBB, axis_avg);

	// Force a leaf if the facet count gets below a certain threshold or the depth becomes too large.
	if ((end - start + 1) <= treeRef->MinFacets() || depth > treeRef->MaxDepth()) {
		nFacets = end - start + 1;
		mIFacets = std::make_unique<uint32_t[]>(nFacets);
		int p = 0;
		for (uint32_t f = start; f <= end; f++)
			mIFacets[p++] = facetIndices[f];

		return;
	}

	// determine split axis
	Vector3 diag = mBB.max - mBB.min;
	axis = 0;
	float curmax = diag.x;
	if (diag.y > curmax) {
		axis = 1;
		curmax = diag.y;
	}

	if (diag.z > curmax)
		axis = 2;

	uint32_t l = start;
	float lval;

	uint32_t r = end;
	float rval;

	switch (axis) {
		case 0:
			lval = treeRef->triRef[facetIndices[l]].AxisMidPointX(treeRef->vertexRef);
			rval = treeRef->triRef[facetIndices[r]].AxisMidPointX(treeRef->vertexRef);
			while (l < r) {
				while (lval < axis_avg.x && l != r) {
					lval = treeRef->triRef[facetIndices[++l]].AxisMidPointX(treeRef->vertexRef);
				}
				while (rval >= axis_avg.x && r != l) {
					rval = treeRef->triRef[facetIndices[--r]].AxisMidPointX(treeRef->vertexRef);
				}
				moreStart = r;
				if (r == l) {
					break;
				}
				std::swap(facetIndices[l], facetIndices[r]);
				lval = treeRef->triRef[facetIndices[++l]].AxisMidPointX(treeRef->vertexRef);
				rval = treeRef->triRef[facetIndices[--r]].AxisMidPointX(treeRef->vertexRef);
			}
			break;

		case 1:
			lval = treeRef->triRef[facetIndices[l]].AxisMidPointY(treeRef->vertexRef);
			rval = treeRef->triRef[facetIndices[r]].AxisMidPointY(treeRef->vertexRef);
			while (l < r) {
				while (lval < axis_avg.y && l != r) {
					lval = treeRef->triRef[facetIndices[++l]].AxisMidPointY(treeRef->vertexRef);
				}
				while (rval >= axis_avg.y && r != l) {
					rval = treeRef->triRef[facetIndices[--r]].AxisMidPointY(treeRef->vertexRef);
				}
				moreStart = r;
				if (r == l) {
					break;
				}
				std::swap(facetIndices[l], facetIndices[r]);
				lval = treeRef->triRef[facetIndices[++l]].AxisMidPointY(treeRef->vertexRef);
				rval = treeRef->triRef[facetIndices[--r]].AxisMidPointY(treeRef->vertexRef);
			}
			break;

		case 2:
			lval = treeRef->triRef[facetIndices[l]].AxisMidPointZ(treeRef->vertexRef);
			rval = treeRef->triRef[facetIndices[r]].AxisMidPointZ(treeRef->vertexRef);
			while (l < r) {
				while (lval < axis_avg.z && l != r) {
					lval = treeRef->triRef[facetIndices[++l]].AxisMidPointZ(treeRef->vertexRef);
				}
				while (rval >= axis_avg.z && r != l) {
					rval = treeRef->triRef[facetIndices[--r]].AxisMidPointZ(treeRef->vertexRef);
				}
				moreStart = r;
				if (r == l)
					break;

				std::swap(facetIndices[l], facetIndices[r]);
				lval = treeRef->triRef[facetIndices[++l]].AxisMidPointZ(treeRef->vertexRef);
				rval = treeRef->triRef[facetIndices[--r]].AxisMidPointZ(treeRef->vertexRef);
			}
			break;
	}

	if (moreStart == start)
		moreStart = (end + start) / 2;

	P = std::make_unique<AABBTreeNode>(facetIndices, moreStart, end, treeRef, this, depth + 1);
	N = std::make_unique<AABBTreeNode>(facetIndices, start, moreStart - 1, treeRef, this, depth + 1);
}

AABBTree::AABBTreeNode::AABBTreeNode(std::vector<uint32_t>& facetIndices, AABBTree* treeRef, AABBTreeNode* parentRef, const uint32_t depth) {
	Vector3 axis_avg;
	tree = treeRef;
	parent = parentRef;

	// Force a leaf if the facet count gets below a certain threshold or the depth becomes too large.
	if (facetIndices.size() <= treeRef->MinFacets() || depth > treeRef->MaxDepth()) {
		treeRef->CalcAABBandGeoAvg(facetIndices, mBB, axis_avg);
		nFacets = static_cast<uint32_t>(facetIndices.size());
		mIFacets = std::make_unique<uint32_t[]>(nFacets);
		for (uint32_t f = 0; f < nFacets; f++)
			mIFacets[f] = facetIndices[f];

		return;
	}

	// less and more indx lists
	std::vector<uint32_t> less;
	std::vector<uint32_t> more;

	// calculate this node's containing AABB and the geometric average
	treeRef->CalcAABBandGeoAvg(facetIndices, mBB, axis_avg);

	// determine split axis
	Vector3 diag = mBB.max - mBB.min;
	uint32_t axis = 0;
	float curmax = diag.x;
	if (diag.y > curmax) {
		axis = 1;
		curmax = diag.y;
	}

	if (diag.z > curmax)
		axis = 2;

	int facetnum;
	switch (axis) {
		case 0:
			for (uint32_t i = 0; i < static_cast<uint32_t>(facetIndices.size()); i++) {
				facetnum = facetIndices[i];
				if (treeRef->triRef[facetnum].AxisMidPointX(treeRef->vertexRef) < axis_avg.x) {
					less.push_back(facetnum);
				}
				else
					more.push_back(facetnum);
			}

			break;

		case 1:
			for (uint32_t i = 0; i < static_cast<uint32_t>(facetIndices.size()); i++) {
				facetnum = facetIndices[i];
				if (treeRef->triRef[facetnum].AxisMidPointY(treeRef->vertexRef) < axis_avg.y) {
					less.push_back(facetnum);
				}
				else
					more.push_back(facetnum);
			}
			break;

		case 2:
			for (uint32_t i = 0; i < static_cast<uint32_t>(facetIndices.size()); i++) {
				facetnum = facetIndices[i];
				if (treeRef->triRef[facetnum].AxisMidPointZ(treeRef->vertexRef) < axis_avg.z) {
					less.push_back(facetnum);
				}
				else
					more.push_back(facetnum);
			}
			break;
	}

	//  Split lists when all midpoints fall on one side of axis or the other
	uint32_t sz = static_cast<uint32_t>(more.size());
	uint32_t numFacets = static_cast<uint32_t>(facetIndices.size());
	if (sz == 0 || sz == numFacets) {
		if (sz > less.size()) {
			sz = sz / 2;
			less.assign(more.begin(), more.begin() + sz);
			more.erase(more.begin(), more.begin() + sz);
		}
		else {
			sz = static_cast<uint32_t>(less.size() / 2);
			more.assign(less.begin(), less.begin() + sz);
			less.erase(less.begin(), less.begin() + sz);
		}
	}

	P = std::make_unique<AABBTreeNode>(more, treeRef, this, depth + 1);
	N = std::make_unique<AABBTreeNode>(less, treeRef, this, depth + 1);
}

Vector3 AABBTree::AABBTreeNode::Center() {
	return ((mBB.max + mBB.min) / 2);
}

void AABBTree::AABBTreeNode::AddDebugFrames(std::vector<Vector3>& verts, std::vector<Edge>& edges, const uint32_t maxdepth, const uint32_t curdepth) {
	if (curdepth <= maxdepth) {
		mBB.AddBoxToMesh(verts, edges);
	} // else return;

	if (P)
		P->AddDebugFrames(verts, edges, maxdepth, curdepth + 1);
	if (N)
		N->AddDebugFrames(verts, edges, maxdepth, curdepth + 1);
}

void AABBTree::AABBTreeNode::AddRayIntersectFrames(Vector3& origin, Vector3& direction, std::vector<Vector3>& verts, std::vector<Edge>& edges) {
	bool collision = mBB.IntersectRay(origin, direction, nullptr);
	if (collision)
		mBB.AddBoxToMesh(verts, edges);
	else
		return;

	if (P)
		P->AddRayIntersectFrames(origin, direction, verts, edges);
	if (N)
		N->AddRayIntersectFrames(origin, direction, verts, edges);
}

bool AABBTree::AABBTreeNode::IntersectRay(Vector3& origin, Vector3& direction, std::vector<IntersectResult>* results) {
	IntersectResult r;
	bool collision = mBB.IntersectRay(origin, direction, nullptr);
	if (!collision)
		return false;

	if (!P && !N) {
		for (uint32_t i = 0; i < nFacets; i++) {
			uint32_t f = mIFacets[i];
			if (tree->triRef[f].IntersectRay(tree->vertexRef, origin, direction, &r.HitDistance, &r.HitCoord)) {
				if (results) {
					r.HitFacet = mIFacets[i];
					r.bvhNode = this;
					results->push_back(r);
				}
				return true;
			}
		}
		return false;
	}

	bool Pcollide = false;
	bool Ncollide = false;
	if (P)
		Pcollide = P->IntersectRay(origin, direction, results);

	if (N)
		Ncollide = N->IntersectRay(origin, direction, results);

	return Pcollide || Ncollide;
}

bool AABBTree::AABBTreeNode::IntersectSphere(Vector3& origin, const float radius, std::vector<IntersectResult>* results) {
	IntersectResult r;
	bool collision = mBB.IntersectSphere(origin, radius);
	if (!collision)
		return false;

	if (!P && !N) {
		bool found = false;
		for (uint32_t i = 0; i < nFacets; i++) {
			uint32_t f = mIFacets[i];
			if (tree->triRef[f].IntersectSphere(tree->vertexRef, origin, radius, &r.HitDistance)) {
				if (results) {
					r.HitFacet = mIFacets[i];
					r.bvhNode = this;
					results->push_back(r);
					found = true;
				}
				else {
					return true;
				}
			}
		}
		return found;
	}

	bool Pcollide = false;
	bool Ncollide = false;
	if (P)
		Pcollide = P->IntersectSphere(origin, radius, results);
	if (N)
		Ncollide = N->IntersectSphere(origin, radius, results);

	return Pcollide || Ncollide;
}

void AABBTree::AABBTreeNode::UpdateAABB(const AABB* childBB) {
	if (!childBB) {
		if (nFacets > 0) {
			Vector3 bogus;
			tree->CalcAABBandGeoAvg(mIFacets.get(), 0, nFacets - 1, mBB, bogus);
		}
	}
	else
		mBB.Merge((*childBB));

	if (parent)
		parent->UpdateAABB(&mBB);
}

AABBTree::AABBTree(Vector3* vertices, Triangle* facets, const uint32_t nFacets, const uint32_t maxDepth, const uint32_t minFacets) {
	triRef = facets;
	vertexRef = vertices;
	max_depth = maxDepth;
	min_facets = minFacets;

	std::vector<uint32_t> facetIndices(nFacets, 0);
	for (uint32_t i = 0; i < nFacets; i++)
		facetIndices[i] = i;

	uint32_t start = 0;
	uint32_t depth = 0;
	root = std::make_unique<AABBTreeNode>(facetIndices, start, nFacets - 1, this, nullptr, depth);
}

uint32_t AABBTree::MinFacets() {
	return min_facets;
}
uint32_t AABBTree::MaxDepth() {
	return max_depth;
}

Vector3 AABBTree::Center() {
	return root->Center();
}

void AABBTree::CalcAABBandGeoAvg(std::vector<uint32_t>& forFacets, AABB& outBB, Vector3& outAxisAvg) {
	if (forFacets.empty())
		return;

	outAxisAvg = Vector3(0.0f, 0.0f, 0.0f);

	Vector3 mid;
	AABB tmpBB(vertexRef, (uint16_t*)(&triRef[forFacets[0]]), 3);

	uint32_t forFacetsSize = static_cast<uint32_t>(forFacets.size());
	for (uint32_t i = 0; i < forFacetsSize; i++) {
		triRef[forFacets[i]].midpoint(vertexRef, mid);
		outAxisAvg += (mid / forFacetsSize);
		tmpBB.Merge(vertexRef, (uint16_t*)&triRef[forFacets[i]], 3);
	}
	outBB = tmpBB;
}

void AABBTree::CalcAABBandGeoAvg(std::vector<uint32_t>& forFacets, const uint32_t start, const uint32_t end, AABB& outBB, Vector3& outAxisAvg) {
	if (forFacets.empty())
		return;

	Vector3 mid;
	//float ytot = 0;
	//float yaxis = 0;
	outAxisAvg = Vector3(0.0f, 0.0f, 0.0f);

	AABB tmpBB(vertexRef, (uint16_t*)(&triRef[forFacets[start]]), 3);
	uint32_t fcount = end - start + 1;
	for (uint32_t i = start; i <= end; i++) {
		triRef[forFacets[i]].midpoint(vertexRef, mid);
		outAxisAvg.x += mid.x;
		outAxisAvg.y += mid.y;
		outAxisAvg.z += mid.z;
		tmpBB.Merge(vertexRef, (uint16_t*)&triRef[forFacets[i]], 3);
	}

	outAxisAvg /= fcount;
	outBB = tmpBB;
}

void AABBTree::CalcAABBandGeoAvg(const uint32_t forFacets[], const uint32_t start, const uint32_t end, AABB& outBB, Vector3& outAxisAvg) {
	Vector3 mid;
	//float ytot = 0;
	//float yaxis = 0;
	outAxisAvg = Vector3(0.0f, 0.0f, 0.0f);

	AABB tmpBB(vertexRef, (uint16_t*)(&triRef[forFacets[start]]), 3);
	uint32_t fcount = end - start + 1;
	for (uint32_t i = start; i <= end; i++) {
		triRef[forFacets[i]].midpoint(vertexRef, mid);
		outAxisAvg.x += mid.x;
		outAxisAvg.y += mid.y;
		outAxisAvg.z += mid.z;
		tmpBB.Merge(vertexRef, (uint16_t*)&triRef[forFacets[i]], 3);
	}
	outAxisAvg /= fcount;
	outBB = tmpBB;
}

void AABBTree::BuildDebugFrames(Vector3** outVerts, uint16_t* outNumVerts, Edge** outEdges, uint32_t* outNumEdges) {
	std::vector<Vector3> v;
	std::vector<Edge> e;

	root->AddDebugFrames(v, e, 8);

	//	m->rendermode = 1;
	//	m->color = Vector3(0,.2,0);
	//	m->nVerts =  v.size();
	//	m->verts = new Vector3[m->nVerts];

	//	m->nEdges = e.size();
	//	m->edges = new Edge[m->nEdges];

	auto vc = static_cast<uint16_t>(v.size());
	(*outNumVerts) = vc;
	(*outVerts) = new Vector3[vc];

	auto ec = static_cast<uint32_t>(e.size());
	(*outNumEdges) = ec;
	(*outEdges) = new Edge[ec];

	for (uint16_t i = 0; i < vc; i++) {
		//m->verts[i] = v[i];
		(*outVerts)[i] = v[i];
	}

	for (uint32_t i = 0; i < ec; i++) {
		//m->edges[i].p1 = e[i].p1;
		//m->edges[i].p2 = e[i].p2;
		(*outEdges)[i].p1 = e[i].p1;
		(*outEdges)[i].p2 = e[i].p2;
	}
}

void AABBTree::BuildRayIntersectFrames(Vector3& origin, Vector3& direction, Vector3** outVerts, uint16_t* outNumVerts, Edge** outEdges, uint32_t* outNumEdges) {
	std::vector<Vector3> v;
	std::vector<Edge> e;
	bFlag = false;
	root->AddRayIntersectFrames(origin, direction, v, e);

	//m->rendermode = 1;
	//m->color = Vector3(.8,.8,0);
	//
	//m->nVerts =  v.size();
	//m->verts = new Vector3[m->nVerts];

	//m->nEdges = e.size();
	//m->edges = new Edge[m->nEdges];

	//for(int i =0 ;i< m->nVerts; i++ ){
	//	m->verts[i] = v[i];
	//}
	//for(int i=0;i<m->nEdges; i++) {
	//	m->edges[i].p1 = e[i].p1;
	//	m->edges[i].p2 = e[i].p2;
	//}

	auto vc = static_cast<uint16_t>(v.size());
	(*outNumVerts) = vc;
	(*outVerts) = new Vector3[vc];

	auto ec = static_cast<uint32_t>(e.size());
	(*outNumEdges) = ec;
	(*outEdges) = new Edge[ec];

	for (uint16_t i = 0; i < vc; i++)
		(*outVerts)[i] = v[i];

	for (uint32_t i = 0; i < ec; i++) {
		(*outEdges)[i].p1 = e[i].p1;
		(*outEdges)[i].p2 = e[i].p2;
	}
}

bool AABBTree::IntersectRay(Vector3& origin, Vector3& direction, std::vector<IntersectResult>* results) {
	return root->IntersectRay(origin, direction, results);
}

bool AABBTree::IntersectSphere(Vector3& origin, const float radius, std::vector<IntersectResult>* results) {
	return root->IntersectSphere(origin, radius, results);
}
