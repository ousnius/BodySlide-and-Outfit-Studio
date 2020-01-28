/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "TweakBrush.h"

TweakUndo::TweakUndo() {
}

TweakUndo::~TweakUndo() {
	Clear();
}

void TweakUndo::Clear()	{
	strokes.clear();
	curState = -1;
}

std::vector<std::future<void>> TweakStroke::normalUpdates{};

TweakStroke* TweakUndo::CreateStroke(const std::vector<mesh*>& refMeshes, TweakBrush* refBrush) {
	int nStrokes = strokes.size();
	if (curState < nStrokes - 1) {
		strokes.resize(curState + 1);
	}
	else if (nStrokes == TB_MAX_UNDO) {
		strokes.erase(strokes.begin());
	}

	TweakStroke* newStroke = new TweakStroke(refMeshes, refBrush);
	strokes.push_back(std::unique_ptr<TweakStroke>(newStroke));
	curState = strokes.size() - 1;
	return newStroke;
}

bool TweakUndo::backStroke(const std::vector<mesh*>& validMeshes) {
	if (curState > -1) {
		for (auto &m : strokes[curState]->GetRefMeshes()) {
			if (std::find(validMeshes.begin(), validMeshes.end(), m) != validMeshes.end())
				strokes[curState]->tsh.RestoreStartState(m);
		}
		curState--;
		return true;
	}
	return false;
}

bool TweakUndo::forwardStroke(const std::vector<mesh*>& validMeshes) {
	int nStrokes = strokes.size();
	if (curState < nStrokes - 1) {
		++curState;
		for (auto &m : strokes[curState]->GetRefMeshes()) {
			if (std::find(validMeshes.begin(), validMeshes.end(), m) != validMeshes.end())
				strokes[curState]->tsh.RestoreEndState(m);
		}
		return true;
	}
	return false;
}

void TweakStateHolder::RestoreStartState(mesh* m) {
	TweakState &ts = tss[m];
	for (auto &stateIt : ts.pointStartState) {
		if (brushType == TBT_MASK || brushType == TBT_WEIGHT || brushType == TBT_COLOR)
			m->vcolors[stateIt.first] = stateIt.second;
		else if (brushType == TBT_ALPHA)
			m->valpha[stateIt.first] = stateIt.second.x;
		else
			m->verts[stateIt.first] = stateIt.second;
	}

	if (brushType != TBT_MASK && brushType != TBT_WEIGHT && brushType != TBT_COLOR) {
		m->SmoothNormals();

		if (ts.startBVH == ts.endBVH) {
			for (auto &bvhNode : ts.affectedNodes)
				bvhNode->UpdateAABB();
		}
		else
			m->bvh = ts.startBVH;
	}

	if (brushType == TBT_MASK || brushType == TBT_WEIGHT || brushType == TBT_COLOR)
		m->QueueUpdate(mesh::UpdateType::VertexColors);
	else if (brushType == TBT_ALPHA)
		m->QueueUpdate(mesh::UpdateType::VertexAlpha);
	else
		m->QueueUpdate(mesh::UpdateType::Position);

}

void TweakStateHolder::RestoreEndState(mesh* m) {
	TweakState &ts = tss[m];
	for (auto &stateIt : ts.pointEndState) {
		if (brushType == TBT_MASK || brushType == TBT_WEIGHT || brushType == TBT_COLOR)
			m->vcolors[stateIt.first] = stateIt.second;
		else if (brushType == TBT_ALPHA)
			m->valpha[stateIt.first] = stateIt.second.x;
		else
			m->verts[stateIt.first] = stateIt.second;
	}

	if (brushType != TBT_MASK && brushType != TBT_WEIGHT && brushType != TBT_COLOR) {
		m->SmoothNormals();

		if (ts.startBVH == ts.endBVH) {
			for (auto &bvhNode : ts.affectedNodes)
				bvhNode->UpdateAABB();
		}
		else
			m->bvh = ts.endBVH;
	}

	if (brushType == TBT_MASK || brushType == TBT_WEIGHT || brushType == TBT_COLOR)
		m->QueueUpdate(mesh::UpdateType::VertexColors);
	else if (brushType == TBT_ALPHA)
		m->QueueUpdate(mesh::UpdateType::VertexAlpha);
	else
		m->QueueUpdate(mesh::UpdateType::Position);
}

void TweakStroke::beginStroke(TweakPickInfo& pickInfo) {
	refBrush->strokeInit(refMeshes, pickInfo, tsh);

	for (auto &m : refMeshes) {
		tsh.tss[m].startBVH = m->bvh;

		pts1[m] = std::make_unique<int[]>(m->nVerts);
		if (refBrush->isMirrored())
			pts2[m] = std::make_unique<int[]>(m->nVerts);
	}
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

	// Move/transform handles most operations differently than other brushes.
	// Mirroring is done internally, most of the pick info values are ignored.
	if (brushType == TBT_MOVE || brushType == TBT_XFORM) {
		Vector3 originSave = pickInfo.origin;
		Vector3 centerSave = pickInfo.center;
		for (auto &m : refMeshes) {
			int nPts1 = 0;

			// Get rid of model space from collision again
			if (brushType == TBT_MOVE) {
				glm::vec4 morigin = glm::inverse(m->matModel) * glm::vec4(originSave.x, originSave.y, originSave.z, 1.0f);
				pickInfo.origin = Vector3(morigin.x, morigin.y, morigin.z);
			}

			if (!refBrush->queryPoints(m, pickInfo, nullptr, nPts1, tsh.tss[m].affectedNodes))
				continue;

			refBrush->brushAction(m, pickInfo, nullptr, nPts1, tsh.tss[m]);

			if (refBrush->LiveNormals()) {
				auto pending = async(std::launch::async, mesh::SmoothNormalsStaticMap, m, tsh.tss[m].pointStartState);
				normalUpdates.push_back(std::move(pending));
			}
		}
		pickInfo.origin = originSave;
		pickInfo.center = centerSave;
	}
	else {
		for (auto &m : refMeshes) {
			int nPts1 = 0;
			int nPts2 = 0;

			if (!refBrush->queryPoints(m, pickInfo, pts1[m].get(), nPts1, tsh.tss[m].affectedNodes))
				continue;

			if (refBrush->isMirrored())
				refBrush->queryPoints(m, mirrorPick, pts2[m].get(), nPts2, tsh.tss[m].affectedNodes);

			refBrush->brushAction(m, pickInfo, pts1[m].get(), nPts1, tsh.tss[m]);

			if (refBrush->isMirrored())
				refBrush->brushAction(m, mirrorPick, pts2[m].get(), nPts2, tsh.tss[m]);

			if (refBrush->LiveNormals()) {
				auto pending1 = std::async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts1[m].get(), nPts1);
				normalUpdates.push_back(std::move(pending1));
				if (refBrush->isMirrored()) {
					auto pending2 = std::async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts2[m].get(), nPts2);
					normalUpdates.push_back(std::move(pending2));
				}
			}
		}
	}

	lastPoint = pickInfo.origin;

	if (refBrush->LiveBVH()) {
		for (auto &m : refMeshes)
			for (auto &bvhNode : tsh.tss[m].affectedNodes)
				bvhNode->UpdateAABB();
	}
}

void TweakStroke::endStroke() {
	if (refBrush->Type() == TBT_MOVE) {
		for (auto &m : refMeshes) {
			TweakBrushMeshCache* meshCache = refBrush->getCache(m);
			tsh.tss[m].affectedNodes.swap(meshCache->cachedNodes);
			tsh.tss[m].affectedNodes.insert(meshCache->cachedNodesM.begin(), meshCache->cachedNodesM.end());
			meshCache->cachedNodes.clear();
			meshCache->cachedNodesM.clear();
		}
	}
	else if (refBrush->Type() == TBT_XFORM)
		for (auto &m : refMeshes)
			m->CreateBVH();

	if (!refBrush->LiveBVH() && refBrush->Type() != TBT_WEIGHT)
		for (auto &m : refMeshes)
			for (auto &bvhNode : tsh.tss[m].affectedNodes)
				bvhNode->UpdateAABB();

	if (!refBrush->LiveNormals()) {
		for (auto &m : refMeshes) {
			auto pending = std::async(std::launch::async, mesh::SmoothNormalsStatic, m);
			normalUpdates.push_back(std::move(pending));
		}
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

	for (auto &m : refMeshes) {
		tsh.tss[m].endBVH = m->bvh;
	}
}

TweakBrush::TweakBrush() : radius(0.45f), focus(1.00f), inset(0.00f), strength(0.0015f), spacing(0.015f) {
	brushType = TBT_STANDARD;
	brushName = "Standard Brush";
	bMirror = false;
	bLiveBVH = true;
	bLiveNormals = true;
	bConnected = true;
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
void TweakBrush::applyFalloff(Vector3 &deltaVec, float dist) {
	// Beyond the radius, no strength
	if (dist > radius) {
		deltaVec.Zero();
		return;
	}

	// No distance, keep strength
	if (dist == 0.0f)
		return;

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

		deltaVec *= p;
	}
}

bool TweakBrush::queryPoints(mesh *refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*> &affectedNodes) {
	std::vector<IntersectResult> IResults;

	if (!refmesh->bvh || !refmesh->bvh->IntersectSphere(pickInfo.origin, radius, &IResults))
		return false;

	std::unique_ptr<bool[]> pointVisit = std::make_unique<bool[]>(refmesh->nVerts);

	if (bConnected && !IResults.empty()) {
		int pickFacet = IResults[0].HitFacet;
		float minDist = IResults[0].HitDistance;
		for (auto &r : IResults) {
			if (r.HitDistance < minDist) {
				minDist = r.HitDistance;
				pickFacet = r.HitFacet;
			}
		}

		std::vector<int> resultFacets;
		refmesh->ConnectedPointsInSphere(pickInfo.origin, radius * radius, pickFacet, nullptr, pointVisit.get(), resultPoints, outResultCount, resultFacets);
	}
	else
		outResultCount = 0;

	Triangle t;
	for (unsigned int i = 0; i < IResults.size(); i++) {
		if (!bConnected) {
			t = refmesh->tris[IResults[i].HitFacet];
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

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Matrix4 xform;
	xform.Translate(pickInfo.normal * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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

void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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
	iterations = 1;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bLiveBVH = false;
	bLiveNormals = false;
	bMirror = false;
	brushName = "Mask Smooth";
}

TB_SmoothMask::~TB_SmoothMask() {
}

void TB_SmoothMask::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;
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

void TB_SmoothMask::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, TweakState &ts) {
	std::unordered_map<int, Vector3>::iterator mi;

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
		b[i] = wv[i] - ((ts.pointStartState[i] * hcAlpha) + (refmesh->vcolors[i] * (1.0f - hcAlpha)));
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

void TB_SmoothMask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;
		wv[points[i]] = vc;
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0)		// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		else					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv, ts);
	}

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
	iterations = 1;
	method = 1;
	strength = 0.01f;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bMirror = false;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {
}

void TB_Smooth::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map <int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;
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

void TB_Smooth::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map <int, Vector3>& wv, TweakState &ts) {
	std::unordered_map<int, Vector3>::iterator mi;

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
		b[i] = wv[i] - ((ts.pointStartState[i] * hcAlpha) + (refmesh->verts[i] * (1.0f - hcAlpha)));
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

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vs;
		wv[points[i]] = vs;
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0) {			// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		}
		else {					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv, ts);
		}
	}
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

TB_Move::TB_Move() : TweakBrush() {
	brushType = TBT_MOVE;
	brushName = "Move Brush";
	bLiveBVH = false;
	focus = 2.0f;
	strength = 0.1f;
}

TB_Move::~TB_Move() {
}

void TB_Move::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo, TweakStateHolder &tsh) {
	pick = pickInfo;
	mpick = pickInfo;
	mpick.origin.x = -mpick.origin.x;
	mpick.view.x = -mpick.view.x;
	mpick.facet = pickInfo.facetM;
	d = pick.origin.dot(pick.view);
	md = mpick.origin.dot(mpick.view);

	cache.clear();

	for (auto &m : refMeshes) {
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = 0;
		meshCache->nCachedPointsM = 0;
		auto& startState = tsh.tss[m].pointStartState;
		meshCache->cachedPoints.resize(m->nVerts);

		if (bMirror)
			meshCache->cachedPointsM.resize(m->nVerts);

		if (!TweakBrush::queryPoints(m, pick, &meshCache->cachedPoints.front(), meshCache->nCachedPoints, meshCache->cachedNodes))
			continue;

		for (int i = 0; i < meshCache->nCachedPoints; i++) {
			int vi = meshCache->cachedPoints[i];
			startState[vi] = m->verts[vi];
		}

		if (bMirror) {
			TweakBrush::queryPoints(m, mpick, &meshCache->cachedPointsM.front(), meshCache->nCachedPointsM, meshCache->cachedNodesM);

			for (int i = 0; i < meshCache->nCachedPointsM; i++) {
				int vi = meshCache->cachedPointsM[i];
				startState[vi] = m->verts[vi];
			}
		}
	}
}

bool TB_Move::queryPoints(mesh* m, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
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

void TB_Move::brushAction(mesh* m, TweakPickInfo& pickInfo, const int*, int, TweakState &ts) {
	float dist = pickInfo.origin.dot(pick.view) - d;
	Vector3 v = pickInfo.origin - pick.view*dist;
	Vector3 dv = v - pick.origin;

	Matrix4 xform;
	xform.Translate(dv * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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

void TB_XForm::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo, TweakStateHolder &tsh) {
	pick = pickInfo;

	cache.clear();

	for (auto &m : refMeshes) {
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = m->nVerts;
		auto& startState = tsh.tss[m].pointStartState;
		for (int i = 0; i < meshCache->nCachedPoints; i++)
			startState[i] = m->verts[i];
	}
}

bool TB_XForm::queryPoints(mesh* m, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
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

void TB_XForm::brushAction(mesh* m, TweakPickInfo& pickInfo, const int*, int, TweakState &ts) {
	Vector3 v = pickInfo.origin;
	Vector3 dv = v - pick.origin;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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

TB_Weight::TB_Weight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.0015f;
	bLiveBVH = false;
	bLiveNormals = false;
	bFixedWeight = false;
	brushName = "Weight Paint";
}

TB_Weight::~TB_Weight() {
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		bFixedWeight ? ve.y = strength * 10.0f : ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y > 1.0f) vf.y = 1.0f;
		endState[points[i]] = refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Unweight::TB_Unweight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = -0.0015f;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Erase";
}

TB_Unweight::~TB_Unweight() {
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y < 0.0f) vf.y = 0.0f;
		endState[points[i]] = refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_SmoothWeight::TB_SmoothWeight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.015f;
	iterations = 1;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bLiveBVH = false;
	bLiveNormals = false;
	bMirror = false;
	brushName = "Weight Smooth";
}

TB_SmoothWeight::~TB_SmoothWeight() {
}

void TB_SmoothWeight::lapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;
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

void TB_SmoothWeight::hclapFilter(mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, TweakState &ts) {
	std::unordered_map<int, Vector3>::iterator mi;

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
		b[i] = wv[i] - ((ts.pointStartState[i] * hcAlpha) + (refmesh->vcolors[i] * (1.0f - hcAlpha)));
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

void TB_SmoothWeight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

	for (int i = 0; i < nPoints; i++) {
		vc = refmesh->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;
		wv[points[i]] = vc;
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0)		// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		else					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv, ts);
	}

	Vector3 delta;
	if (strength != 1.0f) {
		for (int p = 0; p < nPoints; p++) {
			int i = points[p];
			vs = refmesh->verts[i];
			vc = refmesh->vcolors[i];
			delta.y = wv[i].y - vc.y;
			delta.y *= strength;

			applyFalloff(delta, pickInfo.origin.DistanceTo(vs));

			delta.y *= 1.0f - refmesh->vcolors[i].x;
			vc.y += delta.y;

			if (vc.y < EPSILON) vc.y = 0.0f;
			if (vc.y > 1.0f) vc.y = 1.0f;
			endState[i].y = refmesh->vcolors[i].y = vc.y;
		}

		refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
	}
}

TB_Color::TB_Color() :TweakBrush() {
	brushType = TBT_COLOR;
	strength = 0.003f;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Paint";
}

TB_Color::~TB_Color() {
}

void TB_Color::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Erase";
}

TB_Uncolor::~TB_Uncolor() {
}

void TB_Uncolor::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Alpha Brush";
	strength = 0.003f;
}

TB_Alpha::~TB_Alpha() {
}

void TB_Alpha::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Unalpha Brush";
	strength = -0.003f;
}

TB_Unalpha::~TB_Unalpha() {
}

void TB_Unalpha::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, TweakState &ts) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = ts.pointStartState;
	auto& endState = ts.pointEndState;

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
