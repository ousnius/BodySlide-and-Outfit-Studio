/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "TweakBrush.h"

TweakUndo::TweakUndo() : curState(-1) {
}

TweakUndo::~TweakUndo() {
	Clear();
}

void TweakUndo::Clear()	{
	for (unsigned int i = 0; i < strokes.size(); i++) {
		delete strokes[i];
	}
	strokes.clear();
	curState = -1;
}

std::unordered_map<mesh*, Vector3*> TweakStroke::outPositions{};
std::unordered_map<mesh*, int> TweakStroke::outPositionCount{};
int TweakStroke::nStrokes = 0;
std::vector<std::future<void>> TweakStroke::normalUpdates{};

TweakStroke* TweakUndo::CreateStroke(const std::vector<mesh*>& refMeshes, TweakBrush* refBrush) {
	TweakStroke* newStroke = new TweakStroke(refMeshes, refBrush);
	addStroke(newStroke);
	return newStroke;
}

void TweakUndo::addStroke(TweakStroke* stroke) {
	int maxState = strokes.size() - 1;
	if (curState != maxState) {
		for (auto strokeIt = strokes.begin() + (curState + 1); strokeIt != strokes.end(); ++strokeIt)
			delete (*strokeIt);

		strokes.erase(strokes.begin() + (curState + 1), strokes.end());
		curState++;
	}
	else if (strokes.size() == TB_MAX_UNDO) {
		delete strokes[0];
		strokes.erase(strokes.begin());
	}
	else
		curState++;

	strokes.push_back(stroke);
}

bool TweakUndo::backStroke(const std::vector<mesh*>& validMeshes) {
	if (curState > -1) {
		for (auto &m : GetCurStateMeshes()) {
			if (std::find(validMeshes.begin(), validMeshes.end(), m) != validMeshes.end()) {
				strokes[curState]->RestoreStartState(m);
			}
		}
		curState--;
		return true;
	}
	return false;
}

bool TweakUndo::forwardStroke(const std::vector<mesh*>& validMeshes) {
	int maxState = strokes.size() - 1;
	if (curState < maxState) {
		for (auto &m : GetNextStateMeshes()) {
			if (std::find(validMeshes.begin(), validMeshes.end(), m) != validMeshes.end()) {
				strokes[curState + 1]->RestoreEndState(m);
			}
		}
		curState++;
		return true;
	}
	return false;
}

void TweakStroke::RestoreStartState(mesh* m) {
	for (auto &stateIt : pointStartState[m]) {
		if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT)
			m->vcolors[stateIt.first] = stateIt.second;
		else
			m->verts[stateIt.first] = stateIt.second;
	}

	if (refBrush->Type() != TBT_MASK && refBrush->Type() != TBT_WEIGHT) {
		m->SmoothNormals();

		if (startBVH[m] == endBVH[m]) {
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}
		else
			m->bvh = startBVH[m];
	}

	if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT)
		m->QueueUpdate(mesh::UpdateType::VertexColors);
	else
		m->QueueUpdate(mesh::UpdateType::Position);

}

void TweakStroke::RestoreEndState(mesh* m) {
	for (auto &stateIt : pointEndState[m]) {
		if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT)
			m->vcolors[stateIt.first] = stateIt.second;
		else
			m->verts[stateIt.first] = stateIt.second;
	}

	if (refBrush->Type() != TBT_MASK && refBrush->Type() != TBT_WEIGHT) {
		m->SmoothNormals();

		if (startBVH[m] == endBVH[m]) {
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}
		else
			m->bvh = endBVH[m];
	}

	if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT)
		m->QueueUpdate(mesh::UpdateType::VertexColors);
	else
		m->QueueUpdate(mesh::UpdateType::Position);
}

void TweakStroke::beginStroke(TweakPickInfo& pickInfo) {
	refBrush->strokeInit(refMeshes, pickInfo);

	for (auto &m : refMeshes) {
		startBVH[m] = m->bvh;

		pts1[m] = (int*)malloc(m->nVerts * sizeof(int));
		if (refBrush->isMirrored())
			pts2[m] = (int*)malloc(m->nVerts * sizeof(int));

		if (m->nVerts > outPositionCount[m]) {
			if (outPositions.find(m) != outPositions.end())
				delete[] outPositions[m];

			outPositions[m] = new Vector3[m->nVerts];
			outPositionCount[m] = m->nVerts;
		}
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
			std::vector<int> facets;
			int nPts1 = 0;

			// Get rid of model space from collision again
			if (brushType == TBT_MOVE) {
				glm::vec4 morigin = glm::inverse(m->matModel) * glm::vec4(originSave.x, originSave.y, originSave.z, 1.0f);
				pickInfo.origin = Vector3(morigin.x, morigin.y, morigin.z);
			}

			if (!refBrush->queryPoints(m, pickInfo, nullptr, nPts1, facets, affectedNodes[m]))
				continue;

			refBrush->brushAction(m, pickInfo, nullptr, nPts1, outPositions[m]);

			int cachedPointIndex = 0;
			for (int i = 0; i < nPts1; i++) {
				cachedPointIndex = refBrush->CachedPointIndex(m, i);
				addPoint(m, cachedPointIndex, outPositions[m][cachedPointIndex]);
			}

			if (refBrush->LiveNormals()) {
				auto pending = async(std::launch::async, mesh::SmoothNormalsStaticMap, m, pointStartState[m]);
				normalUpdates.push_back(std::move(pending));
			}
		}
		pickInfo.origin = originSave;
		pickInfo.center = centerSave;
	}
	else {
		for (auto &m : refMeshes) {
			std::vector<int> facets;
			std::vector<int> facets2;
			int nPts1 = 0;
			int nPts2 = 0;

			if (!refBrush->queryPoints(m, pickInfo, pts1[m], nPts1, facets, affectedNodes[m]))
				continue;

			if (refBrush->isMirrored())
				refBrush->queryPoints(m, mirrorPick, pts2[m], nPts2, facets2, affectedNodes[m]);

			refBrush->brushAction(m, pickInfo, pts1[m], nPts1, outPositions[m]);
			for (int i = 0; i < nPts1; i++)
				addPoint(m, pts1[m][i], outPositions[m][i]);

			if (refBrush->isMirrored())  {
				refBrush->brushAction(m, mirrorPick, pts2[m], nPts2, outPositions[m]);
				for (int i = 0; i < nPts2; i++)
					addPoint(m, pts2[m][i], outPositions[m][i]);
			}

			if (refBrush->LiveNormals() && brushType != TBT_WEIGHT && brushType != TBT_MASK) {
				auto pending1 = std::async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts1[m], nPts1);
				normalUpdates.push_back(std::move(pending1));

				auto pending2 = async(std::launch::async, mesh::SmoothNormalsStaticArray, m, pts2[m], nPts2);
				normalUpdates.push_back(std::move(pending2));
			}
		}
	}

	lastPoint = pickInfo.origin;

	if (refBrush->LiveBVH() && brushType != TBT_WEIGHT && brushType != TBT_MASK) {
		for (auto &m : refMeshes)
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
	}
}

void TweakStroke::endStroke() {
	if (refBrush->Type() == TBT_MOVE) {
		for (auto &m : refMeshes) {
			TweakBrushMeshCache* meshCache = refBrush->getCache(m);
			affectedNodes[m].swap(meshCache->cachedNodes);
			affectedNodes[m].insert(meshCache->cachedNodesM.begin(), meshCache->cachedNodesM.end());
			meshCache->cachedNodes.clear();
			meshCache->cachedNodesM.clear();
		}
	}
	else if (refBrush->Type() == TBT_XFORM)
		for (auto &m : refMeshes)
			m->CreateBVH();

	if (refBrush->Type() != TBT_WEIGHT)
		for (auto &m : refMeshes)
			for (auto &bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();

	if (!refBrush->LiveNormals() || refBrush->Type() == TBT_WEIGHT) {
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
		if (pts1.find(m) != pts1.end())
			delete[] pts1[m];
		if (pts2.find(m) != pts2.end())
			delete[] pts2[m];

		pts1.clear();
		pts2.clear();

		endBVH[m] = m->bvh;
	}
}

void TweakStroke::addPoint(mesh* m, int point, Vector3& newPos) {
	auto& startState = pointStartState[m];
	if (startState.find(point) == startState.end())
		startState[point] = newPos;

	if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT)
		pointEndState[m][point] = m->vcolors[point];
	else
		pointEndState[m][point] = m->verts[point];
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

// Standard falloff function, used by most brushes 
//   y = (  (cos((pi/2)*x) * sqrt(cos((pi/2)*x))) ^ focus) - inset
//  cancel that -- going for a simpler function (1-x^2)*(1-x^4)
// with focus :  (1-(((f-1)-f*x)^2))*(1-(((f-1)-f*x)^4))  f is greater than 1.
//				
// values between 0 and 1 give a spherical curve, values over 1 give a peaked curve
void TweakBrush::applyFalloff(Vector3 &deltaVec, float dist) {
	// beyond the radius, always 0 stength.
	if (dist > radius) {
		deltaVec.x = deltaVec.y = deltaVec.z = 0.0f;
		return;
	}

	// at 0, always full strength
	if (dist == 0.0f)
		return;

	float p = 1.0f;

	float x = dist / radius;
	if (x <= (focus - 1.0f) / (focus)) {
		p = 1.0f;
	}
	else {
		float fx;
		float p1 = 0.0f;
		float p2 = 0.0f;
		if (focus >= 1.0f) {
			fx = (focus - 1.0f) - (focus*x);
			p1 = (1.0f - fx*fx);
			p2 = (1.0f - pow(fx, 4));
		}
		else if (focus > 0.0f) {
			fx = x / focus;
			p1 = (1.0f - x*x);
			p2 = (1.0f - pow(fx, 4));
		}

		p = p1*p2;
		if (p < 0.0f) p = 0.0f;
	}
	deltaVec *= p;
}

bool TweakBrush::queryPoints(mesh *refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, std::vector<int>& resultFacets, std::unordered_set<AABBTree::AABBTreeNode*> &affectedNodes) {
	std::vector<IntersectResult> IResults;

	if (!refmesh->bvh->IntersectSphere(pickInfo.origin, radius, &IResults))
		return false;

	bool* pointVisit = (bool*)calloc(refmesh->nVerts, sizeof(bool));

	if (bConnected)
		refmesh->ConnectedPointsInSphere(pickInfo.origin, radius * radius, pickInfo.facet, nullptr, pointVisit, resultPoints, outResultCount, resultFacets);
	else
		outResultCount = 0;

	Triangle t;
	for (unsigned int i = 0; i < IResults.size(); i++) {
		if (!bConnected) {
			resultFacets.push_back(IResults[i].HitFacet);
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

	free(pointVisit);
	return true;
}

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3> &movedpoints) {
	Matrix4 xform;
	xform.Translate(pickInfo.normal * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		movedpoints[points[i]] = vs;
		ve = xform * vs;
		ve -= vs;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vs + ve;

		refmesh->verts[points[i]] = (vf);
	}

	refmesh->QueueUpdate(mesh::UpdateType::Position);
}

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	Matrix4 xform;
	xform.Translate(pickInfo.normal * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		movedpoints[i] = vs;
		ve = xform * vs;
		ve -= vs;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vs + ve;

		refmesh->verts[points[i]] = (vf);
	}

	refmesh->QueueUpdate(mesh::UpdateType::Position);
}

TB_Mask::TB_Mask() :TweakBrush() {
	brushType = TBT_MASK;
	strength = 0.1f;
	focus = 4.75f;
	brushName = "Mask Brush";
}

TB_Mask::~TB_Mask() {
}

void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		if (strength < 0.1f)
			ve.x += strength;
		else
			ve.x += 1.0f;
		ve -= vc;

		if (focus < 5.0f)
			applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		vf = vc + ve;
		if (vf.x > 1.0f)
			vf.x = 1.0f;

		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}
	
void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		if (strength < 0.1f)
			ve.x += strength;
		else
			ve.x += 1.0f;
		ve -= vc;

		if (focus < 5.0f)
			applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		vf = vc + ve;
		if (vf.x > 1.0f)
			vf.x = 1.0f;

		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Unmask::TB_Unmask() :TweakBrush() {
	brushType = TBT_MASK;
	strength = -0.1f;
	focus = 4.75f;
	brushName = "Unmask Brush";
}

TB_Unmask::~TB_Unmask() {
}

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		if (strength > -0.1f)
			ve.x += strength;
		else
			ve.x -= 1.0f;
		ve -= vc;

		if (focus < 5.0f)
			applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		vf = vc + ve;
		if (vf.x < 0.0f)
			vf.x = 0.0f;

		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		if (strength > -0.1f)
			ve.x += strength;
		else
			ve.x -= 1.0f;
		ve -= vc;

		if (focus < 5.0f)
			applyFalloff(ve, pickInfo.origin.DistanceTo(vs));

		vf = vc + ve;
		if (vf.x < 0.0f)
			vf.x = 0.0f;

		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
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
	lastMesh = nullptr;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {
}

void TB_Smooth::lapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map <int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;
	Vector3 d;
	int adjPoints[1000];
	int c = 0;
	int a;

	for (int i = 0; i < nPoints; i++) {
		c = refmesh->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		// average adjacent points positions.  Since we're storing the changed vertices separately,
		// there's additional complexity involved with making sure we're grabbing a changed vertex 
		// instead of the original. This primarily comes into play when more than one iteration is used.
		for (int n = 0; n < c; n++) {
			a = adjPoints[n];
			mi = wv.find(a);
			if (mi != wv.end()) {
				d += mi->second;
			}
			else {
				d.x += refmesh->verts[a].x;
				d.y += refmesh->verts[a].y;
				d.z += refmesh->verts[a].z;
			}
		}
		wv[points[i]] = d / (float)c;

		if (refmesh->weldVerts.find(points[i]) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[points[i]].size(); v++) {
				wv[refmesh->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_Smooth::hclapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map <int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;

	b.resize(refmesh->nVerts);
	if (refmesh != lastMesh)
		lastMesh = refmesh;
	else
		memset(b.data(), 0, b.size() * sizeof(Vector3));

	Vector3 d;
	Vector3 q;
	int adjPoints[1000];
	int a;
	int c;
	int i;
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		i = points[p];
		c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		// average adjacent points positions.  Since we're storing the changed vertices separately,
		// there's additional complexity involved with making sure we're grabbing a changed vertex 
		// instead of the original. This primarily comes into play when more than one iteration is used.
		for (int n = 0; n < c; n++) {
			a = adjPoints[n];
			mi = wv.find(a);
			if (mi != wv.end()) {
				d += mi->second;
			}
			else {
				d.x += refmesh->verts[a].x;
				d.y += refmesh->verts[a].y;
				d.z += refmesh->verts[a].z;
			}
		}
		// Save the current point's working position (or original if the working value hasn't been calculated
		// yet.)  This is used as part of the blend between original and changed position
		mi = wv.find(i);
		if (mi != wv.end())
			q = wv[i];
		else
			q = refmesh->verts[i];

		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((refmesh->verts[i] * hcAlpha) + (q * (1.0f - hcAlpha)));

		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++) {
				wv[refmesh->weldVerts[i][v]] = wv[i];
			}
		}
	}

	for (int p = 0; p < nPoints; p++) {
		int j = points[p];
		c = refmesh->GetAdjacentPoints(j, adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[j] -= ((b[j] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(j) != refmesh->weldVerts.end())
			for (unsigned int v = 0; v < refmesh->weldVerts[j].size(); v++)
				wv[refmesh->weldVerts[j][v]] = wv[j];
	}
}

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;

	for (int i = 0; i < nPoints; i++) {
		movedpoints[points[i]] = Vector3(refmesh->verts[points[i]]);
		wv[points[i]] = movedpoints[points[i]];
	}


	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0) {			// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		}
		else {					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv);
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
		}

		refmesh->QueueUpdate(mesh::UpdateType::Position);
	}
}

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;

	for (int i = 0; i < nPoints; i++) {
		movedpoints[i] = refmesh->verts[points[i]];
		wv[points[i]] = movedpoints[i];
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0) {			// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		}
		else {					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv);
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

bool TB_Move::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo) {
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
		meshCache->cachedPoints = (int*)malloc(m->nVerts * sizeof(int));

		if (!TweakBrush::queryPoints(m, pick, meshCache->cachedPoints, meshCache->nCachedPoints, meshCache->cachedFacets, meshCache->cachedNodes))
			continue;

		for (int i = 0; i < meshCache->nCachedPoints; i++)
			meshCache->cachedPositions[meshCache->cachedPoints[i]] = m->verts[meshCache->cachedPoints[i]];

		if (bMirror) {
			meshCache->cachedPointsM = (int*)malloc(m->nVerts * sizeof(int));
			meshCache->cachedFacetsM.clear();
			meshCache->cachedNodesM.clear();
			TweakBrush::queryPoints(m, mpick, meshCache->cachedPointsM, meshCache->nCachedPointsM, meshCache->cachedFacetsM, meshCache->cachedNodesM);

			for (int i = 0; i < meshCache->nCachedPointsM; i++)
				meshCache->cachedPositions[meshCache->cachedPointsM[i]] = m->verts[meshCache->cachedPointsM[i]];
		}
	}

	return true;
}

bool TB_Move::queryPoints(mesh* m, TweakPickInfo&, int* resultPoints, int& outResultCount, std::vector<int>&, std::unordered_set<AABBTree::AABBTreeNode*>&) {
	TweakBrushMeshCache* meshCache = &cache[m];
	if (meshCache->nCachedPoints == 0)
		return false;

	if (resultPoints) {
		memcpy(resultPoints, meshCache->cachedPoints, meshCache->nCachedPoints * sizeof(int));
		memcpy(resultPoints + meshCache->nCachedPoints, meshCache->cachedPointsM, meshCache->nCachedPointsM * sizeof(int));
	}

	outResultCount = meshCache->nCachedPoints + meshCache->nCachedPointsM;

	return true;
}

void TB_Move::brushAction(mesh* m, TweakPickInfo& pickInfo, int*, int, std::unordered_map<int, Vector3>& movedpoints){
	float dist = pickInfo.origin.dot(pick.view) - d;
	Vector3 v = pickInfo.origin - pick.view*dist;
	Vector3 dv = v - pick.origin;

	Matrix4 xform;
	xform.Translate(dv*strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

	TweakBrushMeshCache* meshCache = &cache[m];
	if (bMirror) {
		Vector3 lmo;
		lmo.x = -pickInfo.origin.x;	lmo.y = pickInfo.origin.y; lmo.z = pickInfo.origin.z;
		dist = lmo.dot(mpick.view) - md;
		v = lmo - mpick.view*dist;
		dv = v - mpick.origin;
		Matrix4 xformMirror;
		xformMirror.Translate(dv*strength);

		for (int p = 0; p < meshCache->nCachedPointsM; p++) {
			int i = meshCache->cachedPointsM[p];
			vs = meshCache->cachedPositions[i];
			movedpoints[i] = vs;
			ve = xformMirror * vs;
			ve -= vs;
			applyFalloff(ve, mpick.origin.DistanceTo(vs));

			ve = ve * (1.0f - m->vcolors[i].x);
			vf = vs + ve;

			m->verts[i] = (vf);
		}
	}

	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		int i = meshCache->cachedPoints[p];
		vs = meshCache->cachedPositions[i];
		movedpoints[i] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pick.origin.DistanceTo(vs));

		ve = ve * (1.0f - m->vcolors[i].x);
		vf = vs + ve;

		m->verts[i] = (vf);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

void TB_Move::brushAction(mesh* m, TweakPickInfo& pickInfo, int*, int, Vector3* movedpoints) {
	float dist = pickInfo.origin.dot(pick.view) - d;
	Vector3 v = pickInfo.origin - pick.view*dist;
	Vector3 dv = v - pick.origin;

	Matrix4 xform;
	xform.Translate(dv * strength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

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
			vs = meshCache->cachedPositions[i];
			movedpoints[i] = vs;
			ve = xformMirror * vs;
			ve -= vs;
			applyFalloff(ve, mpick.origin.DistanceTo(vs));

			ve = ve * (1.0f - m->vcolors[i].x);
			vf = vs + ve;

			m->verts[i] = (vf);
		}
	}

	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		int i = meshCache->cachedPoints[p];
		vs = meshCache->cachedPositions[i];
		movedpoints[i] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pick.origin.DistanceTo(vs));

		ve = ve * (1.0f - m->vcolors[i].x);
		vf = vs + ve;

		m->verts[i] = (vf);
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

bool TB_XForm::strokeInit(const std::vector<mesh*>& refMeshes, TweakPickInfo& pickInfo) {
	pick = pickInfo;
	d = pick.origin.dot(pick.normal);

	cache.clear();

	for (auto &m : refMeshes) {
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = m->nVerts;
		for (int i = 0; i < meshCache->nCachedPoints; i++)
			meshCache->cachedPositions[i] = m->verts[i];
	}
	return true;
}

bool TB_XForm::queryPoints(mesh* m, TweakPickInfo&, int* resultPoints, int& outResultCount, std::vector<int>&, std::unordered_set<AABBTree::AABBTreeNode*>&) {
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

void TB_XForm::brushAction(mesh* m, TweakPickInfo& pickInfo, int*, int, std::unordered_map<int, Vector3>& movedpoints) {
	float dist = pickInfo.origin.dot(pick.normal) - d;
	Vector3 v = pickInfo.origin;
	v -= pick.view * dist;
	Vector3 dv = v - pick.origin;
	dv.x *= pick.view.x;
	dv.y *= pick.view.y;
	dv.z *= pick.view.z;

	Matrix4 xform;
	xform.Rotate(2.0f * PI * 10.0f / dv.x, pick.normal.x, pick.normal.y, pick.normal.z);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;

	TweakBrushMeshCache* meshCache = &cache[m];
	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		vs = meshCache->cachedPositions[p];
		movedpoints[p] = vs;
		ve = xform * vs;
		ve -= vs;

		ve = ve * (1.0f - m->vcolors[p].x);
		vf = vs + ve;

		m->verts[p] = (vf);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

void TB_XForm::brushAction(mesh* m, TweakPickInfo& pickInfo, int*, int, Vector3* movedpoints) {
	Vector3 v = pickInfo.origin;
	Vector3 dv = v - pick.origin;

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
		vs = meshCache->cachedPositions[p];
		movedpoints[p] = vs;
		ve = xform * vs;
		ve -= vs;

		ve = ve * (1.0f - m->vcolors[p].x);
		vf = vs + ve;

		m->verts[p] = (vf);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
}

TB_Weight::TB_Weight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.0015f;
	bFixedWeight = false;
	brushName = "Weight Paint";
}

TB_Weight::~TB_Weight() {
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y > 1.0f) vf.y = 1.0f;
		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		bFixedWeight ? ve.y = strength * 10.0f : ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y > 1.0f) vf.y = 1.0f;
		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

TB_Unweight::TB_Unweight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = -0.0015f;
	brushName = "Weight Erase";
}

TB_Unweight::~TB_Unweight() {
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y < 0.0f) vf.y = 0.0f;
		refmesh->vcolors[points[i]] = vf;
	}

	refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		ve.y += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vc + ve;
		if (vf.y < 0.0f) vf.y = 0.0f;
		refmesh->vcolors[points[i]] = vf;
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
	bMirror = false;
	lastMesh = nullptr;
	brushName = "Weight Smooth";
}

TB_SmoothWeight::~TB_SmoothWeight() {
}

void TB_SmoothWeight::lapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;
	Vector3 d;
	int adjPoints[1000];
	int c = 0;
	int a;

	for (int i = 0; i < nPoints; i++) {
		c = refmesh->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		// average adjacent points positions.  Since we're storing the changed vertices separately,
		// there's additional complexity involved with making sure we're grabbing a changed vertex 
		// instead of the original. This primarily comes into play when more than one iteration is used.
		for (int n = 0; n < c; n++) {
			a = adjPoints[n];
			mi = wv.find(a);
			if (mi != wv.end()) {
				d += mi->second;
			}
			else {
				d += refmesh->vcolors[a];
			}
		}
		wv[points[i]] = d / (float)c;

		if (refmesh->weldVerts.find(points[i]) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[points[i]].size(); v++) {
				wv[refmesh->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_SmoothWeight::hclapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	std::unordered_map<int, Vector3>::iterator mi;

	b.resize(refmesh->nVerts);
	if (refmesh != lastMesh)
		lastMesh = refmesh;
	else
		memset(b.data(), 0, b.size() * sizeof(Vector3));

	Vector3 d;
	Vector3 q;
	int adjPoints[1000];
	int a;
	int c;
	int i;
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		i = points[p];
		c = refmesh->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		// average adjacent points positions.  Since we're storing the changed vertices separately,
		// there's additional complexity involved with making sure we're grabbing a changed vertex 
		// instead of the original. This primarily comes into play when more than one iteration is used.
		for (int n = 0; n < c; n++) {
			a = adjPoints[n];
			mi = wv.find(a);
			if (mi != wv.end()) {
				d += mi->second;
			}
			else {
				d += refmesh->vcolors[a];
			}
		}
		// Save the current point's working position (or original if the working value hasn't been calculated
		// yet.)  This is used as part of the blend between original and changed position
		mi = wv.find(i);
		if (mi != wv.end())
			q = wv[i];
		else
			q = refmesh->vcolors[i];

		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((refmesh->vcolors[i] * hcAlpha) + (q * (1.0f - hcAlpha)));

		if (refmesh->weldVerts.find(i) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[i].size(); v++) {
				wv[refmesh->weldVerts[i][v]] = wv[i];
			}
		}
	}

	for (int p = 0; p < nPoints; p++) {
		int j = points[p];
		c = refmesh->GetAdjacentPoints(j, adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[j] -= ((b[j] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(j) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[j].size(); v++) {
				wv[refmesh->weldVerts[j][v]] = wv[j];
			}
		}
	}
}

void TB_SmoothWeight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;

	for (int i = 0; i < nPoints; i++) {
		movedpoints[points[i]] = refmesh->vcolors[points[i]];
		wv[points[i]] = movedpoints[points[i]];
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0)		// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		else					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv);
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
			refmesh->vcolors[i].y = vc.y;
		}

		refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
	}
}

void TB_SmoothWeight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;

	for (int i = 0; i < nPoints; i++) {
		movedpoints[i] = refmesh->vcolors[points[i]];
		wv[points[i]] = movedpoints[i];
	}

	for (int iter = 0; iter < iterations; iter++) {
		if (method == 0)		// laplacian smooth
			lapFilter(refmesh, points, nPoints, wv);
		else					// HC-laplacian smooth
			hclapFilter(refmesh, points, nPoints, wv);
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
			refmesh->vcolors[i].y = vc.y;
		}

		refmesh->QueueUpdate(mesh::UpdateType::VertexColors);
	}
}
