/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "TweakBrush.h"
#include "Anim.h"
#include "WeightNorm.h"

using namespace nifly;

std::vector<std::future<void>> TweakStroke::normalUpdates{};

void TweakStroke::beginStroke(TweakPickInfo& pickInfo) {
	const int nMesh = refMeshes.size();
	usp.usss.resize(nMesh);
	for (int mi = 0; mi < nMesh; ++mi) {
		Mesh* m = refMeshes[mi];
		usp.usss[mi].shapeName = m->shapeName;

		pts1[m] = std::make_unique<int[]>(m->nVerts);
		if (refBrush->isMirrored())
			pts2[m] = std::make_unique<int[]>(m->nVerts);
	}

	refBrush->strokeInit(refMeshes, pickInfo, usp);
}

void TweakStroke::beginStroke(TweakPickInfo& pickInfo, std::vector<std::vector<Vector3>>& positionData) {
	beginStroke(pickInfo);
	refBrush->strokeInit(refMeshes, pickInfo, usp, positionData);
}

void TweakStroke::updateStroke(TweakPickInfo& pickInfo) {
	TweakBrush::BrushType brushType = refBrush->Type();

	TweakPickInfo mirrorPick = pickInfo;
	mirrorPick.origin.x *= -1.0f;
	mirrorPick.normal.x *= -1.0f;

	if (!newStroke) {
		if (!refBrush->checkSpacing(lastPoint, pickInfo.origin))
			return;
	}
	else
		newStroke = false;

	// Remove finished tasks
	for (size_t i = 0; i < normalUpdates.size(); i++) {
		if (normalUpdates[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			normalUpdates.erase(normalUpdates.begin() + i);
			i--;
		}
	}

	const int nMesh = refMeshes.size();
	// Move/transform handles most operations differently than other brushes.
	// Mirroring is done internally, most of the pick info values are ignored.
	if (brushType == TweakBrush::BrushType::Move || brushType == TweakBrush::BrushType::Transform) {
		for (int mi = 0; mi < nMesh; ++mi) {
			Mesh* m = refMeshes[mi];
			UndoStateShape& uss = usp.usss[mi];
			int nPts1 = 0;

			if (!refBrush->queryPoints(m, pickInfo, mirrorPick, nullptr, nPts1, affectedNodes[m]))
				continue;

			refBrush->brushAction(m, pickInfo, nullptr, nPts1, uss);

			if (refBrush->LiveNormals() && normalUpdates.empty()) {
				auto pending = async(std::launch::async, Mesh::SmoothNormalsStaticMap, m, uss.pointStartState);
				normalUpdates.push_back(std::move(pending));
			}
		}
	}
	else {
		for (int mi = 0; mi < nMesh; ++mi) {
			Mesh* m = refMeshes[mi];
			UndoStateShape& uss = usp.usss[mi];
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
				auto pending1 = std::async(std::launch::async, Mesh::SmoothNormalsStaticArray, m, pts1[m].get(), nPts1);
				normalUpdates.push_back(std::move(pending1));

				if (refBrush->isMirrored() && nPts2 > 0 && normalUpdates.size() <= 1) {
					auto pending2 = std::async(std::launch::async, Mesh::SmoothNormalsStaticArray, m, pts2[m].get(), nPts2);
					normalUpdates.push_back(std::move(pending2));
				}
			}
		}
	}

	lastPoint = pickInfo.origin;

	if (refBrush->LiveBVH()) {
		for (int mi = 0; mi < nMesh; ++mi) {
			Mesh* m = refMeshes[mi];
			for (auto& bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}
	}
}

void TweakStroke::endStroke() {
	const int nMesh = refMeshes.size();
	if (refBrush->Type() == TweakBrush::BrushType::Move) {
		for (int mi = 0; mi < nMesh; ++mi) {
			Mesh* m = refMeshes[mi];
			TweakBrushMeshCache* meshCache = refBrush->getCache(m);
			affectedNodes[m].swap(meshCache->cachedNodes);
			affectedNodes[m].insert(meshCache->cachedNodesM.begin(), meshCache->cachedNodesM.end());
			meshCache->cachedNodes.clear();
			meshCache->cachedNodesM.clear();
		}
	}
	else if (refBrush->Type() == TweakBrush::BrushType::Transform)
		for (int mi = 0; mi < nMesh; ++mi)
			refMeshes[mi]->CreateBVH();

	if (!refBrush->LiveBVH() && refBrush->Type() != TweakBrush::BrushType::Weight)
		for (int mi = 0; mi < nMesh; ++mi) {
			Mesh* m = refMeshes[mi];
			for (auto& bvhNode : affectedNodes[m])
				bvhNode->UpdateAABB();
		}

	for (int mi = 0; mi < nMesh; ++mi) {
		auto pending = std::async(std::launch::async, Mesh::SmoothNormalsStatic, refMeshes[mi]);
		normalUpdates.push_back(std::move(pending));
	}

	bool notReady = true;
	while (notReady) {
		notReady = false;
		for (size_t i = 0; i < normalUpdates.size(); i++) {
			if (normalUpdates[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				normalUpdates.erase(normalUpdates.begin() + i);
				i--;
			}
			else
				notReady = true;
		}
	}
}

TweakBrush::TweakBrush()
	: radius(0.45f)
	, focus(0.50f)
	, strength(0.0015f)
	, spacing(0.015f) {
	bMirror = true;
	bLiveBVH = true;
	bLiveNormals = true;
	bConnected = false;
}

TweakBrush::~TweakBrush() {}

void TweakBrush::strokeInit(const std::vector<Mesh*>& refMeshes, TweakPickInfo&, UndoStateProject&) {
	cache.clear();

	if (restrictPlane || restrictNormal) {
		for (Mesh* m : refMeshes) {
			TweakBrushMeshCache& meshCache = cache[m];
			meshCache.startNorms = std::make_unique<Vector3[]>(m->nVerts);
			std::copy(m->norms.get(), m->norms.get() + m->nVerts, meshCache.startNorms.get());
		}
	}
}

bool TweakBrush::checkSpacing(Vector3& start, Vector3& end) {
	float d = start.DistanceTo(end);
	if (d > spacing)
		return true;
	else
		return false;
}

float TweakBrush::getFalloff(float dist, float meshradius) {
	// Beyond the radius, no strength
	if (dist > meshradius)
		return 0.0;

	// No distance, keep strength
	if (dist == 0.0f)
		return 1.0;

	float p = std::pow(6.0f, focus * 2.0f - 1.0f);
	float x = std::pow(dist / meshradius, p);
	return (2 * x - 3) * x * x + 1;
}

void TweakBrush::applyFalloff(Vector3& deltaVec, float dist, float meshradius) {
	deltaVec *= getFalloff(dist, meshradius);
}

bool TweakBrush::queryPoints(
	Mesh* m, TweakPickInfo& pickInfo, TweakPickInfo& mirrorPick, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes) {
	std::vector<IntersectResult> IResults;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);
	Vector3 meshmirrororigin = m->TransformPosModelToMesh(mirrorPick.origin);

	if (!m->bvh || !m->bvh->IntersectSphere(meshorigin, meshradius, &IResults))
		return false;
	unsigned int mirrorStartInd = IResults.size();
	bool mergeMirror = NeedMirrorMergedQuery();
	if (mergeMirror)
		m->bvh->IntersectSphere(meshmirrororigin, meshradius, &IResults);

	std::unique_ptr<bool[]> pointVisit = std::make_unique<bool[]>(m->nVerts);

	if (bConnected && !IResults.empty()) {
		int pickFacet = IResults[0].HitFacet;
		float minDist = IResults[0].HitDistance;
		for (unsigned int i = 0; i < mirrorStartInd; ++i) {
			auto& r = IResults[i];
			if (r.HitDistance < minDist) {
				minDist = r.HitDistance;
				pickFacet = r.HitFacet;
			}
		}

		std::vector<int> resultFacets;
		m->ConnectedPointsInSphere(meshorigin, meshradius * meshradius, pickFacet, nullptr, pointVisit.get(), resultPoints, outResultCount, resultFacets);
		if (mergeMirror && mirrorStartInd < IResults.size()) {
			pickFacet = IResults[mirrorStartInd].HitFacet;
			minDist = IResults[mirrorStartInd].HitDistance;
			for (unsigned int i = mirrorStartInd; i < IResults.size(); ++i) {
				auto& r = IResults[i];
				if (r.HitDistance < minDist) {
					minDist = r.HitDistance;
					pickFacet = r.HitFacet;
				}
			}
			m->ConnectedPointsInSphere(meshmirrororigin, meshradius * meshradius, pickFacet, nullptr, pointVisit.get(), resultPoints, outResultCount, resultFacets);
		}
	}
	else
		outResultCount = 0;

	for (unsigned int i = 0; i < IResults.size(); i++) {
		if (!bConnected) {
			const Triangle& t = m->tris[IResults[i].HitFacet];
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

TB_Inflate::TB_Inflate() {
	brushType = TweakBrush::BrushType::Inflate;
	brushName = "Inflate Brush";
}

TB_Inflate::~TB_Inflate() {}

void TB_Inflate::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Matrix4 xform;
	float meshstrength = m->TransformDistModelToMesh(strength);
	xform.Translate(m->TransformDirModelToMesh(pickInfo.normal) * meshstrength);
	Vector3 vs;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vs;
		if (restrictNormal) {
			const Vector3& n = cache[m].startNorms[points[i]];
			ve = n * meshstrength;
		}
		else {
			ve = xform * vs;
			ve -= vs;
		}

		applyFalloff(ve, meshorigin.DistanceTo(vs), meshradius);

		ve = ve * (1.0f - m->mask[points[i]]);

		vf = vs + ve;

		endState[points[i]] = m->verts[points[i]] = (vf);
	}

	m->QueueUpdate(Mesh::UpdateType::Position);
}

TB_Mask::TB_Mask() {
	brushType = TweakBrush::BrushType::Mask;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Mask Brush";
	strength = 1.0f;
}

TB_Mask::~TB_Mask() {}

void TB_Mask::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = Vector3(m->mask[points[i]], 0.0f, 0.0f);
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		if (strength < 1.0f)
			ve.x += strength;
		else
			ve.x += 1.0f;
		ve -= vc;

		float originToV = meshorigin.DistanceTo(vs);
		if (originToV > meshradius)
			ve.Zero();
		else if (strength < 1.0f)
			applyFalloff(ve, originToV, meshradius);

		vf = vc + ve;
		if (vf.x > 1.0f)
			vf.x = 1.0f;

		m->mask[points[i]] = vf.x;
		endState[points[i]] = vf;
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}

TB_Unmask::TB_Unmask() {
	brushType = TweakBrush::BrushType::Mask;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Unmask Brush";
	strength = -1.0f;
}

TB_Unmask::~TB_Unmask() {}

void TB_Unmask::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	Vector3 vc;
	Vector3 ve;
	Vector3 vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = Vector3(m->mask[points[i]], 0.0f, 0.0f);
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		ve = vc;
		if (strength > -1.0f)
			ve.x += strength;
		else
			ve.x -= 1.0f;
		ve -= vc;

		float originToV = meshorigin.DistanceTo(vs);
		if (originToV > meshradius)
			ve.Zero();
		else if (strength > -1.0f)
			applyFalloff(ve, originToV, meshradius);

		vf = vc + ve;
		if (vf.x < 0.0f)
			vf.x = 0.0f;

		m->mask[points[i]] = vf.x;
		endState[points[i]] = vf;
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}

TB_SmoothMask::TB_SmoothMask() {
	brushType = TweakBrush::BrushType::Mask;
	strength = 0.015f;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Mask Smooth";
}

TB_SmoothMask::~TB_SmoothMask() {}

void TB_SmoothMask::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = m->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += Vector3(m->mask[adjPoints[n]], 0.0f, 0.0f);
		wv[points[i]] = d / (float)c;

		if (m->weldVerts.find(points[i]) != m->weldVerts.end()) {
			for (unsigned int v = 0; v < m->weldVerts[points[i]].size(); v++) {
				wv[m->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_SmoothMask::hclapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, UndoStateShape& uss) {
	std::vector<Vector3> b(m->nVerts);

	int adjPoints[1000];
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += Vector3(m->mask[adjPoints[n]], 0.0f, 0.0f);
		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((uss.pointStartState[i] * hcAlpha) + (Vector3(m->mask[i], 0.0f, 0.0f) * (1.0f - hcAlpha)));
	}

	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		if (m->weldVerts.find(i) != m->weldVerts.end())
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++)
				if (m->weldVerts[i][v] < i)
					skip = true;
		if (skip)
			continue;
		// Average 'b' for adjacent points
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (m->weldVerts.find(i) != m->weldVerts.end()) {
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++) {
				wv[m->weldVerts[i][v]] = wv[i];
			}
		}
	}
}

void TB_SmoothMask::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	Vector3 vc;
	float vm;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vc = Vector3(m->mask[points[i]], 0.0f, 0.0f);
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;
		wv[points[i]] = vc;
	}

	if (method == 0) // laplacian smooth
		lapFilter(m, points, nPoints, wv);
	else // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss);

	Vector3 delta;
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		vs = m->verts[i];
		vm = m->mask[i];
		delta.x = wv[i].x - vm;
		delta.x *= strength;

		applyFalloff(delta, meshorigin.DistanceTo(vs), meshradius);

		vm += delta.x;

		if (vm < EPSILON)
			vm = 0.0f;
		if (vm > 1.0f)
			vm = 1.0f;
		endState[i].x = m->mask[i] = vm;
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
}

TB_Deflate::TB_Deflate() {
	strength = -0.0015f;
	brushName = "Deflate Brush";
}

TB_Deflate::~TB_Deflate() {}

void TB_Deflate::setStrength(float newStr) {
	strength = -(newStr / 10.0f);
}

float TB_Deflate::getStrength() {
	return -strength * 10.0f;
}

TB_Smooth::TB_Smooth() {
	method = 1;
	strength = 0.1f;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {}

void TB_Smooth::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = m->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += m->verts[adjPoints[n]];
		wv[points[i]] = d / (float)c;

		if (m->weldVerts.find(points[i]) != m->weldVerts.end()) {
			for (unsigned int v = 0; v < m->weldVerts[points[i]].size(); v++) {
				wv[m->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_Smooth::hclapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, UndoStateShape& uss) {
	std::vector<Vector3> b(m->nVerts);

	int adjPoints[1000];
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += m->verts[adjPoints[n]];
		wv[i] = d / (float)c;
		// Calculate the difference between the new position and a blend of the original and previous positions
		b[i] = wv[i] - ((uss.pointStartState[i] * hcAlpha) + (m->verts[i] * (1.0f - hcAlpha)));
	}

	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		if (m->weldVerts.find(i) != m->weldVerts.end())
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++)
				if (m->weldVerts[i][v] < i)
					skip = true;
		if (skip)
			continue;
		// Average 'b' for adjacent points
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;
		Vector3 d;
		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (m->weldVerts.find(i) != m->weldVerts.end())
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++)
				wv[m->weldVerts[i][v]] = wv[i];
	}
}

void TB_Smooth::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	std::unordered_map<int, Vector3> wv;
	Vector3 vs;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vs;
		wv[points[i]] = vs;
	}

	if (method == 0) // laplacian smooth
		lapFilter(m, points, nPoints, wv);
	else // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss);

	Vector3 delta;
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		vs = m->verts[i];
		delta = wv[i] - m->verts[i];
		delta *= strength;
		applyFalloff(delta, meshorigin.DistanceTo(vs), meshradius);

		delta = delta * (1.0f - m->mask[i]);
		Vector3 ve = m->verts[i] + delta;

		if (restrictNormal) {
			const Vector3& n = cache[m].startNorms[i];
			ve = startState[i] + n * (ve - startState[i]).dot(n);
		}

		if (restrictPlane) {
			const Vector3& n = cache[m].startNorms[i];
			ve -= startState[i];
			ve -= n * ve.dot(n);
			ve += startState[i];
		}

		m->verts[i] = ve;
		endState[i] = ve;
	}

	m->QueueUpdate(Mesh::UpdateType::Position);
}

TB_Undiff::TB_Undiff() {
	strength = 0.01f;
	brushName = "Undiff Brush";
	brushType = TweakBrush::BrushType::Undiff;
}

TB_Undiff::~TB_Undiff() {}

void TB_Undiff::strokeInit(const std::vector<Mesh*>& refMeshes, TweakPickInfo& pickInfo, UndoStateProject& usp, std::vector<std::vector<Vector3>>& positionData) {
	TweakBrush::strokeInit(refMeshes, pickInfo, usp);

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		Mesh* m = refMeshes[mi];
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->positionData = std::move(positionData[mi]);
	}
}

void TB_Undiff::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	TweakBrushMeshCache* meshCache = &cache[m];
	std::vector<Vector3>& basePosition = meshCache->positionData;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

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

		if (basePosition.size() > (size_t)p) {
			vs = m->verts[p];
			bp = basePosition[p];

			if (startState.find(p) == startState.end())
				startState[p] = vs;

			dv = bp - vs;
			ve = dv * strength;

			applyFalloff(ve, meshorigin.DistanceTo(vs), meshradius);

			ve = ve * (1.0f - m->mask[p]);
			vf = vs + ve;

			endState[p] = m->verts[p] = (vf);
		}
	}

	m->QueueUpdate(Mesh::UpdateType::Position);
}

TB_Move::TB_Move() {
	brushType = TweakBrush::BrushType::Move;
	brushName = "Move Brush";
	bLiveBVH = false;
	focus = 0.5f;
	strength = 1.0f;
}

TB_Move::~TB_Move() {}

void TB_Move::strokeInit(const std::vector<Mesh*>& refMeshes, TweakPickInfo& pickInfo, UndoStateProject& usp) {
	pick = pickInfo;
	mpick = pickInfo;
	mpick.origin.x = -mpick.origin.x;
	mpick.view.x = -mpick.view.x;
	d = pick.origin.dot(pick.view);
	md = mpick.origin.dot(mpick.view);

	TweakBrush::strokeInit(refMeshes, pickInfo, usp);

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		Mesh* m = refMeshes[mi];
		UndoStateShape& uss = usp.usss[mi];
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

bool TB_Move::queryPoints(Mesh* m, TweakPickInfo&, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
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

void TB_Move::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int*, int, UndoStateShape& uss) {
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 start = m->TransformPosModelToMesh(pick.origin);
	Vector3 mirrorstart = m->TransformPosModelToMesh(mpick.origin);

	TweakBrushMeshCache* meshCache = &cache[m];
	if (bMirror) {
		Vector3 lmo;
		lmo.x = -pickInfo.origin.x;
		lmo.y = pickInfo.origin.y;
		lmo.z = pickInfo.origin.z;
		float dist = lmo.dot(mpick.view) - md;
		Vector3 v = lmo - mpick.view * dist;
		Vector3 dv = (v - mpick.origin) * strength;
		Vector3 meshdv = m->TransformDiffModelToMesh(dv);

		for (int p = 0; p < meshCache->nCachedPointsM; p++) {
			int i = meshCache->cachedPointsM[p];
			Vector3 vs = startState[i];
			if (mirrorstart.DistanceTo(vs) > start.DistanceTo(vs))
				continue;
			Vector3 ve = meshdv;
			applyFalloff(ve, mirrorstart.DistanceTo(vs), meshradius);

			if (m->mask)
				ve = ve * (1.0f - m->mask[i]);

			if (restrictNormal) {
				const Vector3& n = cache[m].startNorms[i];
				ve = n * ve.dot(n);
			}

			if (restrictPlane) {
				const Vector3& n = cache[m].startNorms[i];
				ve -= n * ve.dot(n);
			}

			Vector3 vf = vs + ve;

			endState[i] = m->verts[i] = (vf);
		}
	}

	float dist = pickInfo.origin.dot(pick.view) - d;
	Vector3 v = pickInfo.origin - pick.view * dist;
	Vector3 dv = (v - pick.origin) * strength;
	Vector3 meshdv = m->TransformDiffModelToMesh(dv);

	for (int p = 0; p < meshCache->nCachedPoints; p++) {
		int i = meshCache->cachedPoints[p];
		Vector3 vs = startState[i];
		if (bMirror && start.DistanceTo(vs) > mirrorstart.DistanceTo(vs))
			continue;
		Vector3 ve = meshdv;
		applyFalloff(ve, start.DistanceTo(vs), meshradius);

		if (m->mask)
			ve = ve * (1.0f - m->mask[i]);

		if (restrictNormal) {
			const Vector3& n = cache[m].startNorms[i];
			ve = n * ve.dot(n);
		}

		if (restrictPlane) {
			const Vector3& n = cache[m].startNorms[i];
			ve -= n * ve.dot(n);
		}

		Vector3 vf = vs + ve;

		endState[i] = m->verts[i] = (vf);
	}

	m->QueueUpdate(Mesh::UpdateType::Position);
}

void TB_Move::GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.view;
	outPlaneDist = d;
}

TB_XForm::TB_XForm() {
	brushType = TweakBrush::BrushType::Transform;
	brushName = "Transform Brush";
	bLiveBVH = false;
	focus = 0.5f;
	strength = 1.0f;
}

TB_XForm::~TB_XForm() {}

void TB_XForm::GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.normal;
	outPlaneDist = pick.origin.dot(pick.normal);
}

void TB_XForm::strokeInit(const std::vector<Mesh*>& refMeshes, TweakPickInfo& pickInfo, UndoStateProject& usp) {
	pick = pickInfo;

	TweakBrush::strokeInit(refMeshes, pickInfo, usp);

	const int nMesh = refMeshes.size();
	for (int mi = 0; mi < nMesh; ++mi) {
		Mesh* m = refMeshes[mi];
		UndoStateShape& uss = usp.usss[mi];
		TweakBrushMeshCache* meshCache = &cache[m];
		meshCache->nCachedPoints = m->nVerts;
		auto& startState = uss.pointStartState;
		for (int i = 0; i < meshCache->nCachedPoints; i++)
			startState[i] = m->verts[i];
	}
}

bool TB_XForm::queryPoints(Mesh* m, TweakPickInfo&, TweakPickInfo&, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>&) {
	TweakBrushMeshCache* meshCache = &cache[m];
	if (meshCache->nCachedPoints == 0)
		return false;

	if (resultPoints) {
		for (int i = 0; i < m->nVerts; i++) {
			resultPoints[i] = i;
		}
	}

	outResultCount = m->nVerts;
	return true;
}

void TB_XForm::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int*, int, UndoStateShape& uss) {
	Vector3 v = pickInfo.origin;
	Vector3 dv = v - pick.origin;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;

	// Project dv onto movement axis (which is stored in pick.view)
	dv.x *= pick.view.x;
	dv.y *= pick.view.y;
	dv.z *= pick.view.z;

	Vector3 center = m->TransformPosModelToMesh(pick.center);

	Matrix4 xform;
	if (xformType == 0) {
		xform.Translate(m->TransformDiffModelToMesh(dv) * strength);
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

		Vector3 a = m->TransformPosModelToMesh(pick.origin) - center;
		Vector3 b = m->TransformPosModelToMesh(pickInfo.origin) - center;
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

		if (m->mask)
			ve = ve * (1.0f - m->mask[p]);

		vf = vs + ve;

		endState[p] = m->verts[p] = (vf);
	}

	m->QueueUpdate(Mesh::UpdateType::Position);
}

static inline void ClampWeight(float& w) {
	if (w > 1.0f)
		w = 1.0f;
	if (w < EPSILON)
		w = 0.0f;
}

TB_Weight::TB_Weight() {
	brushType = TweakBrush::BrushType::Weight;
	strength = 0.015f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	bFixedWeight = false;
	brushName = "Weight Paint";
}

TB_Weight::~TB_Weight() {}

void TB_Weight::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, m->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;
	mOrigin = m->TransformPosModelToMesh(mOrigin);

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = {true, false};
		float orDist = meshorigin.DistanceTo(m->verts[i]);
		float morDist = mOrigin.DistanceTo(m->verts[i]);
		float falloff = getFalloff(orDist, meshradius);
		float mFalloff = getFalloff(morDist, meshradius);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float sw = uss.boneWeights[0].weights[i].endVal;
		float str = bFixedWeight ? strength - sw : strength;
		float maskF = 1.0f - m->mask[i];

		uss.boneWeights[0].weights[i].endVal += str * maskF * b0falloff;

		if (!bNormalizeWeights)
			ClampWeight(uss.boneWeights[0].weights[i].endVal);

		if (bXMirrorBone) {
			float b1falloff = mFalloff;
			if (bMirror && orDist < morDist)
				b1falloff = falloff;

			adjFlag[1] = b1falloff > 0.0;
			sw = uss.boneWeights[1].weights[i].endVal;
			str = bFixedWeight ? strength - sw : strength;
			uss.boneWeights[1].weights[i].endVal += str * maskF * b1falloff;

			if (!bNormalizeWeights)
				ClampWeight(uss.boneWeights[1].weights[i].endVal);
		}

		if (bNormalizeWeights)
			nzer.AdjustWeights(i, adjFlag);

		m->weight[i] = uss.boneWeights[0].weights[i].endVal;
	}

	m->QueueUpdate(Mesh::UpdateType::Weight);
}

TB_Unweight::TB_Unweight() {
	brushType = TweakBrush::BrushType::Weight;
	strength = -0.015f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Erase";
}

TB_Unweight::~TB_Unweight() {}

void TB_Unweight::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, m->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;
	mOrigin = m->TransformPosModelToMesh(mOrigin);

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = {true, false};
		float orDist = meshorigin.DistanceTo(m->verts[i]);
		float morDist = mOrigin.DistanceTo(m->verts[i]);
		float falloff = getFalloff(orDist, meshradius);
		float mFalloff = getFalloff(morDist, meshradius);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float maskF = 1.0f - m->mask[i];

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

		m->weight[i] = uss.boneWeights[0].weights[i].endVal;
	}

	m->QueueUpdate(Mesh::UpdateType::Weight);
}

TB_SmoothWeight::TB_SmoothWeight() {
	brushType = TweakBrush::BrushType::Weight;
	strength = 0.15f;
	method = 1;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Smooth";
}

TB_SmoothWeight::~TB_SmoothWeight() {}

void TB_SmoothWeight::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, float>& wv) {
	int adjPoints[1000];

	for (int i = 0; i < nPoints; i++) {
		int c = m->GetAdjacentPoints(points[i], adjPoints, 1000);
		if (c == 0)
			continue;

		// average adjacent points values, using values from last iteration.
		float d = 0.0;
		for (int n = 0; n < c; n++)
			d += m->weight[adjPoints[n]];

		wv[points[i]] = d / (float)c;

		if (m->weldVerts.find(points[i]) != m->weldVerts.end()) {
			for (unsigned int v = 0; v < m->weldVerts[points[i]].size(); v++) {
				wv[m->weldVerts[points[i]][v]] = wv[points[i]];
			}
		}
	}
}

void TB_SmoothWeight::hclapFilter(
	Mesh* m, const int* points, int nPoints, std::unordered_map<int, float>& wv, UndoStateShape& uss, const int boneInd, const std::unordered_map<uint16_t, float>* wPtr) {
	auto& ubw = uss.boneWeights[boneInd].weights;
	std::vector<float> b(m->nVerts);

	int adjPoints[1000];

	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
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
		if (m->weldVerts.find(i) != m->weldVerts.end())
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++)
				if (m->weldVerts[i][v] < i)
					skip = true;

		if (skip)
			continue;

		// Average 'b' for adjacent points
		int c = m->GetAdjacentPoints(i, adjPoints, 1000);
		if (c == 0)
			continue;

		float d = 0.0;

		for (int n = 0; n < c; n++)
			d += b[adjPoints[n]];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (m->weldVerts.find(i) != m->weldVerts.end()) {
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++) {
				wv[m->weldVerts[i][v]] = wv[i];
			}
		}
	}
}

void TB_SmoothWeight::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, animInfo, m->shapeName, boneNames, lockedBoneNames, bXMirrorBone ? 2 : 1, bSpreadWeight);
	nzer.GrabStartingWeights(points, nPoints);

	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	Vector3 mOrigin = pickInfo.origin;
	mOrigin.x = -mOrigin.x;
	mOrigin = m->TransformPosModelToMesh(mOrigin);

	// Copy previous iteration's results into wv
	std::unordered_map<int, float> wv, mwv;
	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		wv[i] = uss.boneWeights[0].weights[i].endVal;

		if (bXMirrorBone)
			mwv[i] = uss.boneWeights[1].weights[i].endVal;
	}

	if (method == 0) // laplacian smooth
		lapFilter(m, points, nPoints, wv);
	else // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss, 0, animInfo->GetWeightsPtr(m->shapeName, boneNames[0]));

	if (bXMirrorBone) {
		if (method == 0) // laplacian smooth
			lapFilter(m, points, nPoints, mwv);
		else // HC-laplacian smooth
			hclapFilter(m, points, nPoints, mwv, uss, 1, animInfo->GetWeightsPtr(m->shapeName, boneNames[1]));
	}

	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		bool adjFlag[2] = {true, false};
		float orDist = meshorigin.DistanceTo(m->verts[i]);
		float morDist = mOrigin.DistanceTo(m->verts[i]);
		float falloff = getFalloff(orDist, meshradius);
		float mFalloff = getFalloff(morDist, meshradius);
		float b0falloff = falloff;

		if (bMirror && morDist < orDist)
			b0falloff = mFalloff;

		adjFlag[0] = b0falloff > 0.0;

		float str = wv[i] - uss.boneWeights[0].weights[i].endVal;
		float maskF = 1.0f - m->mask[i];

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

		m->weight[i] = uss.boneWeights[0].weights[i].endVal;
	}

	m->QueueUpdate(Mesh::UpdateType::Weight);
}

TB_Color::TB_Color() {
	brushType = TweakBrush::BrushType::Color;
	strength = 0.03f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Paint";
}

TB_Color::~TB_Color() {}

void TB_Color::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = m->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		float originToV = meshorigin.DistanceTo(vs);
		if (originToV <= meshradius) {
			Vector3 falloff(strength, strength, strength);
			applyFalloff(falloff, originToV, meshradius);

			vc.x = color.x * falloff.x + vc.x * (1.0f - falloff.x);
			vc.y = color.y * falloff.y + vc.y * (1.0f - falloff.y);
			vc.z = color.z * falloff.z + vc.z * (1.0f - falloff.z);
		}

		if (vc.x > 1.0f)
			vc.x = 1.0f;
		else if (vc.x < 0.0f)
			vc.x = 0.0f;

		if (vc.y > 1.0f)
			vc.y = 1.0f;
		else if (vc.y < 0.0f)
			vc.y = 0.0f;

		if (vc.z > 1.0f)
			vc.z = 1.0f;
		else if (vc.z < 0.0f)
			vc.z = 0.0f;

		endState[points[i]] = m->vcolors[points[i]] = vc;
	}

	m->QueueUpdate(Mesh::UpdateType::VertexColors);
}

TB_Uncolor::TB_Uncolor() {
	brushType = TweakBrush::BrushType::Color;
	strength = -0.03f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Color Erase";
}

TB_Uncolor::~TB_Uncolor() {}

void TB_Uncolor::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	Vector3 vc;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = m->vcolors[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]] = vc;

		float originToV = meshorigin.DistanceTo(vs);
		if (originToV <= meshradius) {
			Vector3 falloff(-strength, -strength, -strength);
			applyFalloff(falloff, originToV, meshradius);

			vc.x = falloff.x + vc.x * (1.0f - falloff.x);
			vc.y = falloff.y + vc.y * (1.0f - falloff.y);
			vc.z = falloff.z + vc.z * (1.0f - falloff.z);
		}

		if (vc.x > 1.0f)
			vc.x = 1.0f;
		else if (vc.x < 0.0f)
			vc.x = 0.0f;

		if (vc.y > 1.0f)
			vc.y = 1.0f;
		else if (vc.y < 0.0f)
			vc.y = 0.0f;

		if (vc.z > 1.0f)
			vc.z = 1.0f;
		else if (vc.z < 0.0f)
			vc.z = 0.0f;

		endState[points[i]] = m->vcolors[points[i]] = vc;
	}

	m->QueueUpdate(Mesh::UpdateType::VertexColors);
}

TB_Alpha::TB_Alpha() {
	brushType = TweakBrush::BrushType::Alpha;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Alpha Brush";
	strength = 0.03f;
}

TB_Alpha::~TB_Alpha() {}

void TB_Alpha::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = m->valpha[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]].x = vc;

		ve = vc;
		if (strength < 1.0f)
			ve -= strength;
		else
			ve -= 1.0f;
		ve -= vc;

		Vector3 vev(ve, ve, ve);
		float originToV = meshorigin.DistanceTo(vs);
		if (originToV > meshradius)
			vev.Zero();
		else if (strength < 1.0f)
			applyFalloff(vev, originToV, meshradius);

		vf = vc + vev.x;
		if (vf < clampMaxValue)
			vf = clampMaxValue;

		endState[points[i]].x = m->valpha[points[i]] = vf;
	}

	m->QueueUpdate(Mesh::UpdateType::VertexAlpha);
}

TB_Unalpha::TB_Unalpha() {
	brushType = TweakBrush::BrushType::Alpha;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Unalpha Brush";
	strength = -0.03f;
}

TB_Unalpha::~TB_Unalpha() {}

void TB_Unalpha::brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) {
	Vector3 vs;
	float vc;
	float ve;
	float vf;
	auto& startState = uss.pointStartState;
	auto& endState = uss.pointEndState;
	float meshradius = m->TransformDistModelToMesh(radius);
	Vector3 meshorigin = m->TransformPosModelToMesh(pickInfo.origin);

	for (int i = 0; i < nPoints; i++) {
		vs = m->verts[points[i]];
		vc = m->valpha[points[i]];
		if (startState.find(points[i]) == startState.end())
			startState[points[i]].x = vc;

		ve = vc;
		if (strength > -1.0f)
			ve -= strength;
		else
			ve += 1.0f;
		ve -= vc;

		Vector3 vev(ve, ve, ve);
		float originToV = meshorigin.DistanceTo(vs);
		if (originToV > meshradius)
			vev.Zero();
		else if (strength > -1.0f)
			applyFalloff(vev, originToV, meshradius);

		vf = vc + vev.x;
		if (vf > 1.0f)
			vf = 1.0f;

		endState[points[i]].x = m->valpha[points[i]] = vf;
	}

	m->QueueUpdate(Mesh::UpdateType::VertexAlpha);
}
