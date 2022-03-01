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

	if (NeedStartNorms()) {
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

	std::vector<bool> pointVisit(m->nVerts, false);

	if (bConnected && !IResults.empty()) {
		int pickFacet1 = IResults[0].HitFacet;
		float minDist = IResults[0].HitDistance;
		for (unsigned int i = 0; i < mirrorStartInd; ++i) {
			auto& r = IResults[i];
			if (r.HitDistance < minDist) {
				minDist = r.HitDistance;
				pickFacet1 = r.HitFacet;
			}
		}

		if (mergeMirror && mirrorStartInd < IResults.size()) {
			int pickFacet2 = IResults[mirrorStartInd].HitFacet;
			minDist = IResults[mirrorStartInd].HitDistance;
			for (unsigned int i = mirrorStartInd; i < IResults.size(); ++i) {
				auto& r = IResults[i];
				if (r.HitDistance < minDist) {
					minDist = r.HitDistance;
					pickFacet2 = r.HitFacet;
				}
			}
			m->ConnectedPointsInTwoSpheres(meshorigin, meshmirrororigin, meshradius * meshradius, pickFacet1, pickFacet2, pointVisit, resultPoints, outResultCount);
		}
		else
			m->ConnectedPointsInSphere(meshorigin, meshradius * meshradius, pickFacet1, pointVisit, resultPoints, outResultCount);
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

/* ParabolaFit: this function fits a parabola through the values for five
points: pti, p1, p2, op1, and op2.  This parabola fit gives a new value for
pti, which is returned.  */
template<typename ValueFuncType>
float ParabolaFit(Mesh* m, int pti, int p1, int p2, const ValueFuncType& ValueFunc) {
	constexpr float maxDotForDeriv = -0.5f;

	const Vector3& tv = m->verts[pti];
	const Vector3& v1 = m->verts[p1];
	const Vector3& v2 = m->verts[p2];
	float tvw = ValueFunc(pti);
	float v1w = ValueFunc(p1);
	float v2w = ValueFunc(p2);

	// Find derivative of weight at p1 in roughly the direction
	// from p1 to pti.
	int op1 = m->FindOpposingPoint(p1, pti, maxDotForDeriv);
	float deriv1 = 0.0f;
	if (op1 == -1)
		deriv1 = (tvw - v1w) / (tv - v1).length();
	else {
		const Vector3& ov1 = m->verts[op1];
		float ov1w = ValueFunc(op1);
		deriv1 = (tvw - ov1w) / (tv - ov1).length();
	}

	// Find derivative of weight at p2 in roughly the direction
	// from p2 to pti.
	int op2 = m->FindOpposingPoint(p2, pti, maxDotForDeriv);
	float deriv2 = 0.0f;
	if (op2 == -1)
		deriv2 = (tvw - v2w) / (tv - v2).length();
	else {
		const Vector3& ov2 = m->verts[op2];
		float ov2w = ValueFunc(op2);
		deriv2 = (tvw - ov2w) / (tv - ov2).length();
	}

	// Second derivative of weight at pti in roughly the direction
	// from p1 to p2, wrt t, with t=0 at p1 and t=1 at p2.
	float secderiv = -(deriv1 + deriv2) * (v2 - v1).length();

	// Calculate t, which is where pti is along the segment from
	// p1 to p2.
	float t = (tv - v1).dot(v2 - v1) / (v2 - v1).length2();

	// Parabola fit of weight at pti.
	return v1w + (v2w - v1w) * t - 0.5f * secderiv * t * (1 - t);
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
	method = 2;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Mask Smooth";
}

TB_SmoothMask::~TB_SmoothMask() {}

void TB_SmoothMask::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	for (int i = 0; i < nPoints; i++) {
		const std::vector<int>& adjPoints = m->adjVerts[points[i]];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int apt : adjPoints)
			d += Vector3(m->mask[apt], 0.0f, 0.0f);
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

	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int apt : adjPoints)
			d += Vector3(m->mask[apt], 0.0f, 0.0f);
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
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		Vector3 d;
		for (int apt : adjPoints)
			d += b[apt];

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

void TB_SmoothMask::bppfFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv, UndoStateShape& uss) {
	int balPts[Mesh::MaxAdjacentPoints];

	for (int i = 0; i < nPoints; i++) {
		int pti = points[i];

		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		auto wvit = m->weldVerts.find(pti);
		if (wvit != m->weldVerts.end())
			for (unsigned int v = 0; v < wvit->second.size(); v++)
				if (wvit->second[v] < pti)
					skip = true;
		if (skip)
			continue;

		int balCount = m->FindAdjacentBalancedPairs(pti, balPts);
		if (balCount == 0)
			continue;

		float d = 0.0;
		for (int n = 0; n < balCount; n += 2) {
			int p1 = balPts[n];
			int p2 = balPts[n + 1];
			float fitm = ParabolaFit(m, pti, p1, p2, [&uss, &m](int p) {
				auto pssit = uss.pointStartState.find(p);
				if (pssit != uss.pointStartState.end())
					return pssit->second.x;
				return m->mask[p];
			});

			d += fitm;
		}

		wv[pti] = Vector3(d / (balCount / 2), 0.0f, 0.0f);

		if (wvit != m->weldVerts.end()) {
			for (unsigned int v = 0; v < wvit->second.size(); v++) {
				wv[wvit->second[v]] = wv[pti];
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
	else if (method == 1) // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss);
	else	// balanced-pair parabola-fit smooth
		bppfFilter(m, points, nPoints, wv, uss);

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
	method = 2;
	strength = 0.1f;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {}

void TB_Smooth::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	for (int i = 0; i < nPoints; i++) {
		const std::vector<int>& adjPoints = m->adjVerts[points[i]];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int apt : adjPoints)
			d += m->verts[apt];
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

	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		// average adjacent points positions, using values from last iteration.
		Vector3 d;
		for (int apt : adjPoints)
			d += m->verts[apt];
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
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;
		Vector3 d;
		for (int apt : adjPoints)
			d += b[apt];

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[i] -= ((b[i] * hcBeta) + (d * avgB));

		if (m->weldVerts.find(i) != m->weldVerts.end())
			for (unsigned int v = 0; v < m->weldVerts[i].size(); v++)
				wv[m->weldVerts[i][v]] = wv[i];
	}
}

static Vector3 CircleFitMidpoint(const Vector3& p1, const Vector3& p2, const Vector3& n1, const Vector3& n2) {
	// This code is copied from PrepareRefineMesh.
	/* We wish to calculate the midpoint of a circular arc between
	p1 and p2.  We want the normals at p1 and p2 to be perpendicular
	to this arc.  But there's no guarantee that that's possible: the
	normals could have different angles to the arc; and they could
	even be non-coplanar.  So we need to average the angles of the
	normals somehow.  */

	// First, get the unit vector along the segment between p1 and p2
	Vector3 u12 = p2 - p1;	// unit vector from p1 to p2
	float halfseglen = u12.length() * 0.5f;	// half segment length
	u12.Normalize();

	// Calculate the desired normal at the arc midpoint.  We average the
	// two point normals and make the result perpendicular to u12.
	Vector3 n = n1 + n2;
	n -= u12 * u12.dot(n);
	n.Normalize();

	// If we wanted to calculate the angle between one of the two normals and
	// n, in the plane of that normal and the segment (since n isn't
	// necessarily in that plane), we would do asin(u12.dot(ni)).  We want to
	// average this for the two normals and carefully preserve the sign.
	float sina = u12.dot(n2 - n1) * 0.5f;
	float alpha = asin(sina);

	// Alpha is the desired circle angle between the arc midpoint and either
	// of our input points.  It's positive for convex, negative for concave.
	// To figure out how far off of the segment we need to go, we need to take
	// the trigonometric tangent of the correct angle.  It turns out that the
	// correct angle is alpha divided by 2.
	float curveOffsetFactor = tan(alpha * 0.5f);

	// Now calculate the point.  (curveOffsetFactor is between -1 and 1.)
	Vector3 mp = (p1 + p2) * 0.5f;
	mp += n * (curveOffsetFactor * halfseglen);
	return mp;
}

static Vector3 CircleFitNearestPoint(const Vector3& p1, const Vector3& p2, const Vector3& n1, const Vector3& n2, const Vector3& p) {
	// Much of this code is the same as CircleFitMidpoint.  See the comments
	// in that function.
	Vector3 u12 = p2 - p1;	// unit vector from p1 to p2
	float halfseglen = u12.length() * 0.5f;	// half segment length
	u12.Normalize();
	Vector3 n = n1 + n2;
	n -= u12 * u12.dot(n);
	n.Normalize();
	float sina = u12.dot(n2 - n1) * 0.5f;
	float alpha = asin(sina);
	float curveOffsetFactor = tan(alpha * 0.5f);
	Vector3 mp = (p1 + p2) * 0.5f;
	Vector3 amp = mp + n * (curveOffsetFactor * halfseglen);

	// Now we have some relatively simple new stuff.
	float cosa = cos(alpha);
	Vector3 poff = p - mp;
	Vector3 perp = u12.cross(n);
	poff -= perp * perp.dot(poff); // project poff to circle's plane

	// Now things get weird.  These formulas make no sense.  I derived them
	// from straightforward geometric concepts, but the simple geometric
	// formulas had division-by-zero and large-cancellation numerical problems.
	// The formulas used here have those numerical problems removed, at the
	// expense of making them completely obscure and opaque.  Alpha is the
	// (signed) circle angle between p1/p2 and mp.  Beta is the (signed)
	// circle angle between (projected) p and mp.  Note that there could
	// easily be a gross mistake here, and probably no one would ever
	// realize it because the adjustment to amp is tiny.
	Vector3 denvec = sina * poff + (halfseglen * cosa) * n;
	float den = denvec.length();
	float sinboversina = poff.dot(u12) / den;
	float eoff = halfseglen * sinboversina;
	float sinb = sina * sinboversina;
	float beta = asin(sinb);
	float noff = eoff * tan(beta * 0.5f);
	return amp + eoff * u12 - noff * n;
}

void TB_Smooth::bpcfFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, Vector3>& wv) {
	int balPts[Mesh::MaxAdjacentPoints];

	for (int i = 0; i < nPoints; i++) {
		int pti = points[i];

		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		auto wvit = m->weldVerts.find(pti);
		if (wvit != m->weldVerts.end())
			for (unsigned int v = 0; v < wvit->second.size(); v++)
				if (wvit->second[v] < pti)
					skip = true;
		if (skip)
			continue;

		int balCount = m->FindAdjacentBalancedPairs(pti, balPts);
		if (balCount == 0)
			continue;

		// Average the circle-fit results for each balanced pair
		Vector3 d;
		for (int n = 0; n < balCount; n += 2) {
			const Vector3& p1 = m->verts[balPts[n]];
			const Vector3& p2 = m->verts[balPts[n + 1]];
			const Vector3& n1 = cache[m].startNorms[balPts[n]];
			const Vector3& n2 = cache[m].startNorms[balPts[n + 1]];
			if (restrictNormal)
				d += CircleFitNearestPoint(p1, p2, n1, n2, m->verts[pti]);
			else
				d += CircleFitMidpoint(p1, p2, n1, n2);
		}
		wv[pti] = d / (balCount / 2);

		// Update welded points
		if (wvit != m->weldVerts.end()) {
			for (unsigned int v = 0; v < wvit->second.size(); v++) {
				wv[wvit->second[v]] = wv[pti];
			}
		}
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
	else if (method == 1) // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss);
	else	// balanced-pair circle-fit smooth
		bpcfFilter(m, points, nPoints, wv);

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
	method = 2;
	hcAlpha = 0.2f;
	hcBeta = 0.5f;
	bMirror = false;
	bLiveBVH = false;
	bLiveNormals = false;
	brushName = "Weight Smooth";
}

TB_SmoothWeight::~TB_SmoothWeight() {}

void TB_SmoothWeight::lapFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, float>& wv) {
	for (int i = 0; i < nPoints; i++) {
		const std::vector<int>& adjPoints = m->adjVerts[points[i]];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;

		// average adjacent points values, using values from last iteration.
		float d = 0.0;
		for (int apt : adjPoints)
			d += m->weight[apt];

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

	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		int i = points[p];
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;

		// average adjacent points positions, using values from last iteration.
		float d = 0.0;
		for (int apt : adjPoints) {
			if (ubw.find(apt) != ubw.end())
				d += ubw[apt].endVal;
			else if (wPtr && wPtr->find(apt) != wPtr->end())
				d += wPtr->at(apt);
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
		const std::vector<int>& adjPoints = m->adjVerts[i];
		int c = static_cast<int>(adjPoints.size());
		if (c == 0)
			continue;

		float d = 0.0;

		for (int apt : adjPoints)
			d += b[apt];

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

void TB_SmoothWeight::bppfFilter(Mesh* m, const int* points, int nPoints, std::unordered_map<int, float>& wv, UndoStateShape& uss, const int boneInd, const std::unordered_map<uint16_t, float>* wPtr) {
	auto& weights = uss.boneWeights[boneInd].weights;

	int balPts[Mesh::MaxAdjacentPoints];

	for (int i = 0; i < nPoints; i++) {
		int pti = points[i];

		// Check if it's a welded vertex; only do welded vertices once.
		bool skip = false;
		auto wvit = m->weldVerts.find(pti);
		if (wvit != m->weldVerts.end())
			for (unsigned int v = 0; v < wvit->second.size(); v++)
				if (wvit->second[v] < pti)
					skip = true;
		if (skip)
			continue;

		int balCount = m->FindAdjacentBalancedPairs(pti, balPts);
		if (balCount == 0)
			continue;

		float d = 0.0;
		for (int n = 0; n < balCount; n += 2) {
			int p1 = balPts[n];
			int p2 = balPts[n + 1];
			float fitw = ParabolaFit(m, pti, p1, p2, [&weights, &wPtr](int p) {
				auto wit = weights.find(p);
				if (wit != weights.end())
					return wit->second.startVal;
				if (wPtr) {
					auto wpit = wPtr->find(p);
					if (wpit != wPtr->end())
						return wpit->second;
				}
				return 0.0f;
			});

			d += fitw;
		}

		wv[pti] = d / (balCount / 2);

		if (wvit != m->weldVerts.end()) {
			for (unsigned int v = 0; v < wvit->second.size(); v++) {
				wv[wvit->second[v]] = wv[pti];
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
	else if (method == 1) // HC-laplacian smooth
		hclapFilter(m, points, nPoints, wv, uss, 0, animInfo->GetWeightsPtr(m->shapeName, boneNames[0]));
	else // balanced-pair parabola-fit smooth
		bppfFilter(m, points, nPoints, wv, uss, 0, animInfo->GetWeightsPtr(m->shapeName, boneNames[0]));

	if (bXMirrorBone) {
		if (method == 0) // laplacian smooth
			lapFilter(m, points, nPoints, mwv);
		else if (method == 1) // HC-laplacian smooth
			hclapFilter(m, points, nPoints, mwv, uss, 1, animInfo->GetWeightsPtr(m->shapeName, boneNames[1]));
		else // balanced-pair parabola-fit smooth
			bppfFilter(m, points, nPoints, mwv, uss, 1, animInfo->GetWeightsPtr(m->shapeName, boneNames[1]));
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
