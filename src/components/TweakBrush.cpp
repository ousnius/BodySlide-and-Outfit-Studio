/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "TweakBrush.h"
#include "WeightNorm.h"
#include "Anim.h"

using namespace nifly;

std::vector<std::future<void>> TweakStroke::normalUpdates{};

void TweakStroke::beginStroke(TweakPickInfo& pickInfo) {
	const int nMesh = refMeshes.size();
	usp.usss.resize(nMesh);
	for (int mi = 0; mi < nMesh; ++mi) {
		mesh *m = refMeshes[mi];
		usp.usss[mi].shapeName = m->shapeName;

		pts1[m] = std::make_unique<int[]>(m->nVerts);
		if (refBrush->isMirrored())
			pts2[m] = std::make_unique<int[]>(m->nVerts);
	}

	refBrush->strokeInit(refMeshes, pickInfo, usp);
}

void TweakStroke::beginStroke(TweakPickInfo& pickInfo, const std::vector<std::vector<Vector3>>& positionData) {
	beginStroke(pickInfo);
	refBrush->strokeInit(refMeshes, pickInfo, usp, positionData);
}

void TweakStroke::updateStroke(TweakPickInfo& pickInfo) {
	int brushType = refBrush->Type();

	TweakPickInfo mirrorPick = pickInfo;
	mirrorPick.origin.x *= -1.0f;
	mirrorPick.normal.x *= -1.0f;
	mirrorPick.facet = pickInfo.facetM;

	if (!newStroke) {
		if (!refBrush->checkSpacing(lastPoint, pickInfo.origin))
			return;
	}
	else
		newStroke = false;

	// Remove finished tasks
	for (int i = 0; i < normalUpdates.size(); i++) {
		if (normalUpdates[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			normalUpdates.erase(normalUpdates.begin() + i);
			i--;
		}
	}

	const int nMesh = refMeshes.size();
	// Move/transform handles most operations differently than other brushes.
	// Mirroring is done internally, most of the pick info values are ignored.
	if (brushType == TBT_MOVE || brushType == TBT_XFORM) {
		Vector3 originSave = pickInfo.origin;
		Vector3 centerSave = pickInfo.center;
		for (int mi = 0; mi < nMesh; ++mi) {
			mesh *m = refMeshes[mi];
			UndoStateShape &uss = usp.usss[mi];
			int nPts1 = 0;

			// Get rid of model space from collision again
			if (brushType == TBT_MOVE) {
				glm::vec4 morigin = glm::inverse(m->matModel) * glm::vec4(originSave.x, originSave.y, originSave.z, 1.0f);
				pickInfo.origin = Vector3(morigin.x, morigin.y, morigin.z);
			}

			if (!refBrush->queryPoints(m, pickInfo, mirrorPick, nullptr, nPts1, affectedNodes[m]))
				continue;

			refBrush->brushAction(m, pickInfo, nullptr, nPts1, uss);

			if (refBrush->LiveNormals() && normalUpdates.empty()) {
				auto pending = async(std::launch::async, mesh::SmoothNormalsStaticMap, m, uss.pointStartState);
				normalUpdates.push_back(std::move(pending));
			}
		}
		pickInfo.origin = originSave;
		pickInfo.center = centerSave;
	}
	else {
		for (int mi = 0; mi < nMesh; ++mi) {
			mesh *m = refMeshes[mi];
			UndoStateShape &uss = usp.usss[mi];
			int nPts1 = 0;
			int nPts2 = 0;

			if (!refBrush->queryPoints(m, pickInfo, mirrorPick, pts1[m].get(), nPts1, affectedNodes[m]))
				continue;

			if (refBrush->isMirrored() && !refBrush->NeedMirrorMergedQuery())
				refBrush->queryPoints(m, mirrorPick, pickInfo, pts2[m].get(), nPts2, affectedNodes[m]);

			refBrush->brushAction(m, pickInfo, pts1[m].get(), nPts1, uss);

			if (refBrush->isMirrored() && nPts2 > 0)
				refBrush->brushAction(m, mirrorPick, pts2[m].get(), nPts2, uss);

			if (refBrush->LiveNormals() && normalUpdates.empty()) {
				auto pending1 = std::async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts1[m].get(), nPts1);
				normalUpdates.push_back(std::move(pending1));

				if (refBrush->isMirrored() && nPts2 > 0 && normalUpdates.size() <= 1) {
					auto pending2 = std::async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts2[m].get(), nPts2);
					normalUpdates.push_back(std::move(pending2));
				}
			}
		}
	}

	lastPoint = pickInfo.origin;

	if (refBrush->LiveBVH()) {
		for (int mi = 0; mi < nMesh; ++mi) {
			mesh *m = refMeshes[mi];
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}
	}
}

void TweakStroke::endStroke() {
	const int nMesh = refMeshes.size();
	if (refBrush->Type() == TBT_MOVE) {
		for (int mi = 0; mi < nMesh; ++mi) {
			mesh *m = refMeshes[mi];
			TweakBrushMeshCache* meshCache = refBrush->getCache(m);
			affectedNodes[m].swap(meshCache->cachedNodes);
			affectedNodes[m].insert(meshCache->cachedNodesM.begin(), meshCache->cachedNodesM.end());
			meshCache->cachedNodes.clear();
			meshCache->cachedNodesM.clear();
		}
	}
	else if (refBrush->Type() == TBT_XFORM)
		for (int mi = 0; mi < nMesh; ++mi)
			refMeshes[mi]->CreateBVH();

	if (!refBrush->LiveBVH() && refBrush->Type() != TBT_WEIGHT)
		for (int mi = 0; mi < nMesh; ++mi) {
			mesh *m = refMeshes[mi];
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}

	for (int mi = 0; mi < nMesh; ++mi) {
		auto pending = std::async(std::launch::async, mesh::SmoothNormalsStatic, refMeshes[mi]);
		normalUpdates.push_back(std::move(pending));
	}

	bool notReady = true;
	while (notReady) {
		notReady = false;
		for (int i = 0; i < normalUpdates.size(); i++) {
			if (normalUpdates[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				normalUpdates.erase(normalUpdates.begin() + i);
				i--;
			}
			else
				notReady = true;
		}
	}
}

TweakBrush::TweakBrush() : radius(0.45f), focus(1.00f), inset(0.00f), strength(0.0015f), spacing(0.015f) {
	brushType = TBT_STANDARD;
	brushName = "Standard Brush";
	bMirror = true;
	bLiveBVH = true;
	bLiveNormals = true;
	bConnected = false;
}

TweakBrush::~TweakBrush() {
}

bool TweakBrush::checkSpacing(Vector3& start, Vector3& end) {
	float d = start.DistanceTo(end);
	if (d > spacing)
		return true;
	else
		return false;
}

// Standard falloff function
// y = ((cos((pi/2)*x) * sqrt(cos((pi/2)*x))) ^ focus) - inset
// Cancel that -- going for a simpler function (1-x^2)*(1-x^4)
// With focus: (1-(((f-1)-f*x)^2))*(1-(((f-1)-f*x)^4))  f is greater than 1.
// Values between 0 and 1 give a spherical curve, values over 1 give a peaked curve
float TweakBrush::getFalloff(float dist) {
	// Beyond the radius, no strength
	if (dist > radius)
		return 0.0;

	// No distance, keep strength
	if (dist == 0.0f)
		return 1.0;

	float x = dist / radius;
	if (x > (focus - 1.0f) / focus) {
		float p1 = 0.0f;
		float p2 = 0.0f;

		if (focus >= 1.0f) {
			float fx = (focus - 1.0f) - focus * x;
			p1 = 1.0f - fx * fx;
			p2 = 1.0f - std::pow(fx, 4);
		}
		else if (focus > 0.0f) {
			float fx = x / focus;
			p1 = 1.0f - x * x;
			p2 = 1.0f - std::pow(fx, 4);
		}

		float p = p1 * p2;
		if (p < 0.0f)
			p = 0.0f;

		return p;
	}
	return 1.0;
}

void TweakBrush::applyFalloff(Vector3 &deltaVec, float dist) {
	deltaVec *= getFalloff(dist);
}

bool TweakBrush::queryPoints(mesh *refmesh, TweakPickInfo& pickInfo, TweakPickInfo& mirrorPick, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*> &affectedNodes) {
	std::vector<IntersectResult> IResults;

	if (!refmesh->bvh || !refmesh->bvh->IntersectSphere(pickInfo.origin, radius, &IResults))
		return false;
	unsigned int mirrorStartInd = IResults.size();
	bool mergeMirror = NeedMirrorMergedQuery();
	if (mergeMirror)
		refmesh->bvh->IntersectSphere(mirrorPick.origin, radius, &IResults);

	std::unique_ptr<bool[]> pointVisit = std::make_unique<bool[]>(refmesh->nVerts);

	if (bConnected && !IResults.empty()) {
		int pickFacet = IResults[0].HitFacet;
		float minDist = IResults[0].HitDistance;
		for (unsigned int i = 0; i < mirrorStartInd; ++i) {
			auto &r = IResults[i];
			if (r.HitDistance < minDist) {
				minDist = r.HitDistance;
				pickFacet = r.HitFacet;
			}
		}

		std::vector<int> resultFacets;
		refmesh->ConnectedPointsInSphere(pickInfo.origin, radius * radius, pickFacet, nullptr, pointVisit.get(), resultPoints, outResultCount, resultFacets);
		if (mergeMirror && mirrorStartInd < IResults.size()) {
			pickFacet = IResults[mirrorStartInd].HitFacet;
			minDist = IResults[mirrorStartInd].HitDistance;
			for (unsigned int i = mirrorStartInd; i < IResults.size(); ++i) {
				auto &r = IResults[i];
				if (r.HitDistance < minDist) {
					minDist = r.HitDistance;
					pickFacet = r.HitFacet;
				}
			}
			refmesh->ConnectedPointsInSphere(mirrorPick.origin, radius * radius, pickFacet, nullptr, pointVisit.get(), resultPoints, outResultCount, resultFacets);
		}
	}
	else
		outResultCount = 0;

	for (unsigned int i = 0; i < IResults.size(); i++) {
		if (!bConnected) {
			const Triangle &t = refmesh->tris[IResults[i].HitFacet];
			if (!pointVisit[t.p1]) {
				resultPoints[outResultCount++] = t.p1;
				pointVisit[t.p1] = true;
			}
			if (!pointVisit[t.p2]) {
				resultPoints[outResultCount++] = t.p2;
				pointVisit[t.p2] = true;
			}
			if (!pointVisit[t.p3]) {
				resultPoints[outResultCount++] = t.p3;
				pointVisit[t.p3] = true;
			}

		}
		affectedNodes.insert(IResults[i].bvhNode);
	}

	return true;
}

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Matrix4 xform;
	xform.Translate(pickInfo.normal * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vs;
		ve = xform * vs;
		ve -= vs;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vs + ve;

		endState[points[i]] = refmesh->verts[points[i]] = (vf);
	}

	refmesh->QueueUpdate(mesh::UpdateType::Position);
}

TB_Mask::TB_Mask() :TweakBrush() {
	brushType = TBT_MASK;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Mask Brush";
	strength = 0.1f;
}

TB_Mask::~TB_Mask() {
}

void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		if (strength < 0.1f)
			ve.x += strength;
		else
			ve.x += 1.0f;
		ve -= vc;

		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV > radius)
			ve.Zero();
		else if (strength < 0.1f && focus < 5.0f)
			applyFalloff(ve, originToV);

		vf = vc + ve;
		if (vf.x > 1.0f)
			vf.x = 1.0f;

		endState[points[i]] = refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Unmask::TB_Unmask() :TweakBrush() {
	brushType = TBT_MASK;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Unmask Brush";
	strength = -0.1f;
}

TB_Unmask::~TB_Unmask() {
}

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		if (strength > -0.1f)
			ve.x += strength;
		else
			ve.x -= 1.0f;
		ve -= vc;

		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV > radius)
			ve.Zero();
		else if (strength > -0.1f && focus < 5.0f)
			applyFalloff(ve, originToV);

		vf = vc + ve;
		if (vf.x < 0.0f)
			vf.x = 0.0f;

		endState[points[i]] = refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_SmoothMask::TB_SmoothMask() :TweakBrush() {
	brushType = TBT_MASK;
	strength = 0.015f;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Mask Smooth";
}

TB_SmoothMask::~TB_SmoothMask() {
}

void TB_SmoothMask::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = refmesh->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0) continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += refmesh->vcolors[adjPoints[n]];
		wv[points[i]] = d / (float)c;

		if (refmesh->weldVerts.find(points[i]) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[points[i]].size(); v++) {
				wv[refmesh->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_SmoothMask::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, UndoStateShape &uss) {
	std::vector<Vector3> b(refmesh->nVerts);

	int adjPoints[1000];
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += refmesh->vcolors[adjPoints[n]];
		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((uss.pointStartState[i] * hcAlpha) + (refmesh->vcolors[i] * (1.0f - hcAlpha)));
	}

	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end())
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++)
				if (refmesh->weldVerts[i][v] < i)
					skip = true;
		if (skip) continue;
		// Average 'b' for adjacent points
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++) {
				wv[refmesh->weldVerts[i][v]] = wv[i];
			}
		}
	}
}

void TB_SmoothMask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;
		wv[points[i]] = vc;
	}

	if (method == 0)		// laplacian smooth
		lapFilter(refmesh, points, nPoints, wv);
	else					// HC-laplacian smooth
		hclapFilter(refmesh, points, nPoints, wv, uss);

	Vector3 delta;
	if (strength != 1.0f) {
		for (int p = 0; p < nPoints; p++) {
			int i = points[p];
			vs = refmesh->verts[i];
			vc = refmesh->vcolors[i];
			delta.x = wv[i].x - vc.x;
			delta.x *= strength;

			applyFalloff(delta, pickInfo.origin.DistanceTo(vs));

			vc.x += delta.x;

			if (vc.x < EPSILON) vc.x = 0.0f;
			if (vc.x > 1.0f) vc.x = 1.0f;
			endState[i].x = refmesh->vcolors[i].x = vc.x;
		}

		refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
	}
}
	
TB_Deflate::TB_Deflate() :TweakBrush() {
	strength = -0.0015f;
	brushName = "Deflate Brush";
}

TB_Deflate::~TB_Deflate() {
}

void TB_Deflate::setStrength(float newStr) {
	strength = -(newStr / 10.0f);
}

float TB_Deflate::getStrength() {
	return -strength * 10.0f;
}

TB_Smooth::TB_Smooth() :TweakBrush() {
	method = 1;
	strength = 0.01f;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {
}

void TB_Smooth::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map <int, Vector3>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = refmesh->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0) continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += refmesh->verts[adjPoints[n]];
		wv[points[i]] = d / (float)c;

		if (refmesh->weldVerts.find(points[i]) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[points[i]].size(); v++) {
				wv[refmesh->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_Smooth::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map <int, Vector3>& wv, UndoStateShape &uss) {
	std::vector<Vector3> b(refmesh->nVerts);

	int adjPoints[1000];
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += refmesh->verts[adjPoints[n]];
		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((uss.pointStartState[i] * hcAlpha) + (refmesh->verts[i] * (1.0f - hcAlpha)));
	}

	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end())
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++)
				if (refmesh->weldVerts[i][v] < i)
					skip = true;
		if (skip) continue;
		// Average 'b' for adjacent points
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end())
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++)
				wv[refmesh->weldVerts[i][v]] = wv[i];
	}
}

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vs;
		wv[points[i]] = vs;
	}

	if (method == 0)			// laplacian smooth
		lapFilter(refmesh, points, nPoints, wv);
	else					// HC-laplacian smooth
		hclapFilter(refmesh, points, nPoints, wv, uss);

	Vector3 delta;
	if (strength != 1.0f) {
		for (int p = 0; p < nPoints; p++) {
			int i = points[p];
			vs = refmesh->verts[i];
			delta = wv[i] - refmesh->verts[i];
			delta *= strength;
			applyFalloff(delta, pickInfo.origin.DistanceTo(vs));

			delta = delta * (1.0f - refmesh->vcolors[i].x);
			refmesh->verts[i] += delta;
			endState[i] = refmesh->verts[i];
		}

		refmesh->QueueUpdate(mesh::UpdateType::Position);
	}
}

TB_Undiff::TB_Undiff() :TweakBrush() {
	strength = 0.01f;
	brushName = "Undiff Brush";
	brushType = TBT_UNDIFF;
}

TB_Undiff::~TB_Undiff() {
}

void TB_Undiff::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo&, UndoStateProject&, const std::vector<std::vector<Vector3>>& positionData) {
	cache.clear();

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		mesh* m = refMeshes[mi];
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->positionData = std::move(positionData[mi]);
	}
}

void TB_Undiff::brushAction(mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	TweakBrushMeshCache* meshCache = &cache[m];
	std::vector<Vector3>& basePosition = meshCache->positionData;

	int p;
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	Vector3 bp;
	Vector3 dv;

	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		p = points[i];

		if (basePosition.size() > p) {
			vs = m->verts[p];
			bp = basePosition[p];

			if (startState.find(p) == startState.end())
				startState[p] = vs;

			dv = bp - vs;
			ve = dv * strength;

			applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

			ve = ve * (1.0f - m->vcolors[p].x);
			vf = vs + ve;

			endState[p] = m->verts[p] = (vf);
		}
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

TB_Move::TB_Move() : TweakBrush() {
	brushType = TBT_MOVE;
	brushName = "Move Brush";
	bLiveBVH = false;
	focus = 2.0f;
	strength = 0.1f;
}

TB_Move::~TB_Move() {
}

void TB_Move::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo, UndoStateProject &usp) {
	pick = pickInfo;
	mpick = pickInfo;
	mpick.origin.x = -mpick.origin.x;
	mpick.view.x = -mpick.view.x;
	mpick.facet = pickInfo.facetM;
	d = pick.origin.dot(pick.view);
	md = mpick.origin.dot(mpick.view);

	cache.clear();

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		mesh *m = refMeshes[mi];
		UndoStateShape &uss = usp.usss[mi];
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = 0;
		meshCache->nCachedPointsM = 0;
		auto& startState = uss.pointStartState;
		meshCache->cachedPoints.resize(m->nVerts);

		if (bMirror)
			meshCache->cachedPointsM.resize(m->nVerts);

		if (!TweakBrush::queryPoints(m, pick, mpick, &meshCache->cachedPoints.front(), meshCache->nCachedPoints, meshCache->cachedNodes))
			continue;

		for (int i = 0; i < meshCache->nCachedPoints; i++) {
			int vi = meshCache->cachedPoints[i];
			startState[vi] = m->verts[vi];
		}

		if (bMirror) {
			TweakBrush::queryPoints(m, mpick, pick, &meshCache->cachedPointsM.front(), meshCache->nCachedPointsM, meshCache->cachedNodesM);

			for (int i = 0; i < meshCache->nCachedPointsM; i++) {
				int vi = meshCache->cachedPointsM[i];
				startState[vi] = m->verts[vi];
			}
		}
	}
}

bool TB_Move::queryPoints(mesh* m, TweakPickInfo&, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
	TweakBrushMeshCache* meshCache = &cache[m];
	if (meshCache->nCachedPoints == 0)
		return false;

	if (resultPoints) {
		std::memcpy(resultPoints, &meshCache->cachedPoints.front(), meshCache->nCachedPoints * sizeof(int));
		std::memcpy(resultPoints + meshCache->nCachedPoints, &meshCache->cachedPointsM.front(), meshCache->nCachedPointsM * sizeof(int));
	}

	outResultCount = meshCache->nCachedPoints + meshCache->nCachedPointsM;

	return true;
}

void TB_Move::brushAction(mesh* m, TweakPickInfo& pickInfo, const int*, int, UndoStateShape &uss) {
	float dist = pickInfo.origin.dot(pick.view) - d;
	Vector3 v = pickInfo.origin - pick.view*dist;
	Vector3 dv = v - pick.origin;

	Matrix4 xform;
	xform.Translate(dv * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	TweakBrushMeshCache* meshCache = &cache[m];
	if (bMirror) {
		Vector3 lmo;
		lmo.x = -pickInfo.origin.x;
		lmo.y = pickInfo.origin.y;
		lmo.z = pickInfo.origin.z;
		dist = lmo.dot(mpick.view) - md;
		v = lmo - mpick.view*dist;
		dv = v - mpick.origin;
		Matrix4 xformMirror;
		xformMirror.Translate(dv * strength);

		for (int p = 0; p < meshCache->nCachedPointsM; p++) {
			int i = meshCache->cachedPointsM[p];
			vs = startState[i];
			if (mpick.origin.DistanceTo(vs) > pick.origin.DistanceTo(vs))
				continue;
			ve = xformMirror * vs;
			ve -= vs;
			applyFalloff(ve, mpick.origin.DistanceTo(vs));

			if (m->vcolors)
				ve = ve * (1.0f - m->vcolors[i].x);

			vf = vs + ve;

			endState[i] = m->verts[i] = (vf);
		}
	}

	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		int i = meshCache->cachedPoints[p];
		vs = startState[i];
		if (bMirror && pick.origin.DistanceTo(vs) > mpick.origin.DistanceTo(vs))
			continue;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pick.origin.DistanceTo(vs));

		if (m->vcolors)
			ve = ve * (1.0f - m->vcolors[i].x);

		vf = vs + ve;

		endState[i] = m->verts[i] = (vf);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

void TB_Move::GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.view;
	outPlaneDist = d;
}

TB_XForm::TB_XForm() {
	brushType = TBT_XFORM;
	brushName = "Transform Brush";
	bLiveBVH = false;
	focus = 1.0f;
	strength = 1.0f;
}

TB_XForm::~TB_XForm() {
}

void TB_XForm::GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.normal;
	outPlaneDist = pick.origin.dot(pick.normal);
}

void TB_XForm::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo, UndoStateProject &usp) {
	pick = pickInfo;

	cache.clear();

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		mesh *m = refMeshes[mi];
		UndoStateShape &uss = usp.usss[mi];
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = m->nVerts;
		auto& startState = uss.pointStartState;
		for (int i = 0; i < meshCache->nCachedPoints; i++)
			startState[i] = m->verts[i];
	}
}

bool TB_XForm::queryPoints(mesh* m, TweakPickInfo&, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
	TweakBrushMeshCache* meshCache = &cache[m];
	if (meshCache->nCachedPoints == 0)
		return false;

	if (resultPoints) {
		for (int i = 0; i < m->nVerts; i++)	{
			resultPoints[i] = i;
		}
	}

	outResultCount = m->nVerts;
	return true;
}

void TB_XForm::brushAction(mesh* m, TweakPickInfo& pickInfo, const int*, int, UndoStateShape &uss) {
	Vector3 v = pickInfo.origin;
	Vector3 dv = v - pick.origin;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	dv.x *= pick.view.x;
	dv.y *= pick.view.y;
	dv.z *= pick.view.z;

	glm::vec4 mcenter = glm::inverse(m->matModel) * glm::vec4(pick.center.x, pick.center.y, pick.center.z, 1.0f);
	Vector3 center = Vector3(mcenter.x, mcenter.y, mcenter.z);

	Matrix4 xform;
	if (xformType == 0) {
		xform.Translate(dv * strength);
	}
	else if (xformType == 1) {
		xform.PushTranslate(center);

		Vector3 a = pick.origin - center;
		Vector3 b = pickInfo.origin - center;
		Vector3 dir = a.cross(b);
		float sign = dir.dot(pick.normal);
		float angle = a.angle(pickInfo.origin - center);
		if (sign < 0)
			angle = -angle;

		xform.PushRotate(angle, pick.normal);
		xform.PushTranslate(center * -1.0f);
	}
	else if (xformType == 2) {
		xform.PushTranslate(center);

		Vector3 dist(1.0f, 1.0f, 1.0f);
		if (fabs(dv.x) > EPSILON)
			dist.x = fabs(1.0f + dv.x / 10.0f);
		else if (fabs(dv.y) > EPSILON)
			dist.y = fabs(1.0f + dv.y / 10.0f);
		else if (fabs(dv.z) > EPSILON)
			dist.z = fabs(1.0f + dv.z / 10.0f);

		xform.PushScale(dist.x, dist.y, dist.z);
		xform.PushTranslate(center * -1.0f);
	}
	else if (xformType == 3) {
		xform.PushTranslate(center);

		glm::vec4 morigin1 = glm::inverse(m->matModel) * glm::vec4(pick.origin.x, pick.origin.y, pick.origin.z, 1.0f);
		glm::vec4 morigin2 = glm::inverse(m->matModel) * glm::vec4(pickInfo.origin.x, pickInfo.origin.y, pickInfo.origin.z, 1.0f);
		Vector3 a = Vector3(morigin1.x, morigin1.y, morigin1.z) - center;
		Vector3 b = Vector3(morigin2.x, morigin2.y, morigin2.z) - center;
		Vector3 dist = a + b;

		float scale = (dist.x + dist.y) / 2.0f;
		scale = fabs(1.0f + scale / 10.0f);
		if (scale > EPSILON)
			xform.PushScale(scale, scale, scale);

		xform.PushTranslate(center * -1.0f);
	}

	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

	TweakBrushMeshCache* meshCache = &cache[m];
	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		vs = startState[p];
		ve = xform * vs;
		ve -= vs;

		if (m->vcolors)
			ve = ve * (1.0f - m->vcolors[p].x);

		vf = vs + ve;

		endState[p] = m->verts[p] = (vf);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

static inline void ClampWeight(float &w) {
	if (w > 1.0f) w = 1.0f;
	if (w < EPSILON) w = 0.0f;
}

TB_Weight::TB_Weight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.0015f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	bFixedWeight = false;
	brushName = "Weight Paint";
}

TB_Weight::~TB_Weight() {
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, refmesh->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = { true, false };
		float orDist = pickInfo.origin.DistanceTo(refmesh->verts[i]);
		float morDist = mOrigin.DistanceTo(refmesh->verts[i]);
		float falloff = getFalloff(orDist);
		float mFalloff = getFalloff(morDist);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float sw = uss.boneWeights[0].weights[i].endVal;
		float str = bFixedWeight ? strength * 10.0f - sw : strength;
		float maskF = 1.0f - refmesh->vcolors[i].x;

		uss.boneWeights[0].weights[i].endVal += str * maskF * b0falloff;

		if (!bNormalizeWeights)
			ClampWeight(uss.boneWeights[0].weights[i].endVal);

		if (bXMirrorBone) {
			float b1falloff = mFalloff;
			if (bMirror && orDist < morDist)
				b1falloff = falloff;

			adjFlag[1] = b1falloff > 0.0;
			sw = uss.boneWeights[1].weights[i].endVal;
			str = bFixedWeight ? strength * 10.0f - sw : strength;
			uss.boneWeights[1].weights[i].endVal += str * maskF * b1falloff;

			if (!bNormalizeWeights)
				ClampWeight(uss.boneWeights[1].weights[i].endVal);
		}

		if (bNormalizeWeights)
			nzer.AdjustWeights(i, adjFlag);

		refmesh->vcolors[i].y = uss.boneWeights[0].weights[i].endVal;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Unweight::TB_Unweight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = -0.0015f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Erase";
}

TB_Unweight::~TB_Unweight() {
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, refmesh->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = { true, false };
		float orDist = pickInfo.origin.DistanceTo(refmesh->verts[i]);
		float morDist = mOrigin.DistanceTo(refmesh->verts[i]);
		float falloff = getFalloff(orDist);
		float mFalloff = getFalloff(morDist);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float maskF = 1.0f - refmesh->vcolors[i].x;

		uss.boneWeights[0].weights[i].endVal += strength * maskF * b0falloff;

		if (!bNormalizeWeights)
			ClampWeight(uss.boneWeights[0].weights[i].endVal);

		if (bXMirrorBone) {
			float b1falloff = mFalloff;
			if (bMirror && orDist < morDist)
				b1falloff = falloff;

			adjFlag[1] = b1falloff > 0.0;
			uss.boneWeights[1].weights[i].endVal += strength * maskF * b1falloff;

			if (!bNormalizeWeights)
				ClampWeight(uss.boneWeights[1].weights[i].endVal);
		}

		if (bNormalizeWeights)
			nzer.AdjustWeights(i, adjFlag);

		refmesh->vcolors[i].y = uss.boneWeights[0].weights[i].endVal;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_SmoothWeight::TB_SmoothWeight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.015f;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Smooth";
}

TB_SmoothWeight::~TB_SmoothWeight() {
}

void TB_SmoothWeight::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, float>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = refmesh->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0)
			continue;

		// average adjacent points values, using values from last iteration.
		float d = 0.0;
		for (int n = 0; n < c; n++)
			d += refmesh->vcolors[adjPoints[n]].y;

		wv[points[i]] = d / (float)c;

		if (refmesh->weldVerts.find(points[i]) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[points[i]].size(); v++) {
				wv[refmesh->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_SmoothWeight::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, float>& wv, UndoStateShape &uss, const int boneInd, const std::unordered_map<uint16_t, float> *wPtr) {
	auto &ubw = uss.boneWeights[boneInd].weights;
	std::vector<float> b(refmesh->nVerts);

	int adjPoints[1000];

	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;

		// average adjacent points positions, using values from last iteration.
		float d = 0.0;
		for (int n = 0; n < c; n++) {
			int ai = adjPoints[n];
			if (ubw.find(ai) != ubw.end())
				d += ubw[ai].endVal;
			else if (wPtr && wPtr->find(ai) != wPtr->end())
				d += wPtr->at(ai);
			// otherwise the previous weight is zero
		}

		wv[i] = d / (float)c;

		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((ubw[i].startVal * hcAlpha) + (ubw[i].endVal * (1.0f - hcAlpha)));
	}

	for (int p = 0; p < nPoints; p++) {
		int i = points[p];

		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end())
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++)
				if (refmesh->weldVerts[i][v] < i)
					skip = true;

		if (skip)
			continue;

		// Average 'b' for adjacent points
		int c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;

		float d = 0.0;

		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++) {
				wv[refmesh->weldVerts[i][v]] = wv[i];
			}
		}
	}
}

void TB_SmoothWeight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, refmesh->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;

	// Copy previous iteration's results into wv
	std::unordered_map<int, float> wv, mwv;
	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		wv[i] = uss.boneWeights[0].weights[i].endVal;

		if (bXMirrorBone)
			mwv[i] = uss.boneWeights[1].weights[i].endVal;
	}

	if (method == 0)		// laplacian smooth
		lapFilter(refmesh, points, nPoints, wv);
	else					// HC-laplacian smooth
		hclapFilter(refmesh, points, nPoints, wv, uss, 0, animInfo->GetWeightsPtr(refmesh->shapeName, boneNames[0]));

	if (bXMirrorBone) {
		if (method == 0)		// laplacian smooth
			lapFilter(refmesh, points, nPoints, mwv);
		else					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, mwv, uss, 1, animInfo->GetWeightsPtr(refmesh->shapeName, boneNames[1]));
	}

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = { true, false };
		float orDist = pickInfo.origin.DistanceTo(refmesh->verts[i]);
		float morDist = mOrigin.DistanceTo(refmesh->verts[i]);
		float falloff = getFalloff(orDist);
		float mFalloff = getFalloff(morDist);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float str = wv[i] - uss.boneWeights[0].weights[i].endVal;
		float maskF = 1.0f - refmesh->vcolors[i].x;

		uss.boneWeights[0].weights[i].endVal += str * maskF * b0falloff;

		if (!bNormalizeWeights)
			ClampWeight(uss.boneWeights[0].weights[i].endVal);

		if (bXMirrorBone) {
			float b1falloff = mFalloff;

			if (bMirror && orDist < morDist)
				b1falloff = falloff;

			adjFlag[1] = b1falloff > 0.0;
			str = mwv[i] - uss.boneWeights[1].weights[i].endVal;
			uss.boneWeights[1].weights[i].endVal += str * maskF * b1falloff;

			if (!bNormalizeWeights)
				ClampWeight(uss.boneWeights[1].weights[i].endVal);
		}

		if (bNormalizeWeights)
			nzer.AdjustWeights(i, adjFlag);

		refmesh->vcolors[i].y = uss.boneWeights[0].weights[i].endVal;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Color::TB_Color() :TweakBrush() {
	brushType = TBT_COLOR;
	strength = 0.003f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Paint";
}

TB_Color::~TB_Color() {
}

void TB_Color::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV <= radius) {
			Vector3 falloff(strength * 10.0f, strength * 10.0f, strength * 10.0f);
			applyFalloff(falloff, originToV);

			vc.x = color.x * falloff.x + vc.x * (1.0f - falloff.x);
			vc.y = color.y * falloff.y + vc.y * (1.0f - falloff.y);
			vc.z = color.z * falloff.z + vc.z * (1.0f - falloff.z);
		}

		if (vc.x > 1.0f) vc.x = 1.0f;
		else if (vc.x < 0.0f) vc.x = 0.0f;

		if (vc.y > 1.0f) vc.y = 1.0f;
		else if (vc.y < 0.0f) vc.y = 0.0f;

		if (vc.z > 1.0f) vc.z = 1.0f;
		else if (vc.z < 0.0f) vc.z = 0.0f;

		endState[points[i]] = refmesh->vcolors[points[i]] = vc;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Uncolor::TB_Uncolor() :TweakBrush() {
	brushType = TBT_COLOR;
	strength = -0.003f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Erase";
}

TB_Uncolor::~TB_Uncolor() {
}

void TB_Uncolor::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV <= radius) {
			Vector3 falloff(-strength * 10.0f, -strength * 10.0f, -strength * 10.0f);
			applyFalloff(falloff, originToV);

			vc.x = falloff.x + vc.x * (1.0f - falloff.x);
			vc.y = falloff.y + vc.y * (1.0f - falloff.y);
			vc.z = falloff.z + vc.z * (1.0f - falloff.z);
		}

		if (vc.x > 1.0f) vc.x = 1.0f;
		else if (vc.x < 0.0f) vc.x = 0.0f;

		if (vc.y > 1.0f) vc.y = 1.0f;
		else if (vc.y < 0.0f) vc.y = 0.0f;

		if (vc.z > 1.0f) vc.z = 1.0f;
		else if (vc.z < 0.0f) vc.z = 0.0f;

		endState[points[i]] = refmesh->vcolors[points[i]] = vc;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Alpha::TB_Alpha() :TweakBrush() {
	brushType = TBT_ALPHA;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Alpha Brush";
	strength = 0.003f;
}

TB_Alpha::~TB_Alpha() {
}

void TB_Alpha::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->valpha[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]].x = vc;

		ve = vc;
		if (strength < 0.1f)
			ve -= strength;
		else
			ve -= 1.0f;
		ve -= vc;

		Vector3 vev(ve, ve, ve);
		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV > radius)
			vev.Zero();
		else if (strength < 0.1f && focus < 5.0f)
			applyFalloff(vev, originToV);

		vf = vc + vev.x;
		if (vf < 0.0f)
			vf = 0.0f;

		endState[points[i]].x = refmesh->valpha[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexAlpha);
}

TB_Unalpha::TB_Unalpha() :TweakBrush() {
	brushType = TBT_ALPHA;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Unalpha Brush";
	strength = -0.003f;
}

TB_Unalpha::~TB_Unalpha() {
}

void TB_Unalpha::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape &uss) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->valpha[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]].x = vc;

		ve = vc;
		if (strength > -0.1f)
			ve -= strength;
		else
			ve += 1.0f;
		ve -= vc;

		Vector3 vev(ve, ve, ve);
		float originToV = pickInfo.origin.DistanceTo(vs);
		if (originToV > radius)
			vev.Zero();
		else if (strength > -0.1f && focus < 5.0f)
			applyFalloff(vev, originToV);

		vf = vc + vev.x;
		if (vf > 1.0f)
			vf = 1.0f;

		endState[points[i]].x = refmesh->valpha[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexAlpha);
}
