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

int TweakStroke::nStrokes = 0;
int TweakStroke::outPositionCount = 0;
vec3* TweakStroke::outPositions = NULL;

TweakStroke* TweakUndo::CreateStroke(mesh* refmesh, TweakBrush* refBrush) {
	TweakStroke* newStroke = new TweakStroke(refmesh, refBrush);
	addStroke(newStroke);
	return newStroke;
}

void TweakUndo::addStroke(TweakStroke* stroke) {
	vector<TweakStroke*>::iterator strokeIt;
	int maxState = strokes.size() - 1;
	if (curState != maxState) {
		for (strokeIt = strokes.begin() + (curState + 1); strokeIt != strokes.end(); ++strokeIt) {
			delete (*strokeIt);
		}
		strokes.erase(strokes.begin() + (curState + 1), strokes.end());
		curState++;
	}
	else if (strokes.size() == TB_MAX_UNDO) {
		delete strokes[0];
		strokes.erase(strokes.begin());
		//curState = strokes.size()-1;
	}
	else {
		curState++;
	}
	strokes.push_back(stroke);
}

bool TweakUndo::backStroke(bool skipUpdate) {
	if (curState > -1) {
		if (!skipUpdate)
			strokes[curState]->restoreStartState();
		curState--;
		return true;
	}
	return false;
}

bool TweakUndo::forwardStroke(bool skipUpdate) {
	int maxState = strokes.size() - 1;
	if (curState < maxState) {
		curState++;
		if (!skipUpdate)
			strokes[curState]->restoreEndState();
		return true;
	}
	return false;
}

void TweakStroke::restoreStartState() {
	unordered_map<int, vec3>::iterator stateIt;
	unordered_set<AABBTree::AABBTreeNode*>::iterator bvhNode;
	for (stateIt = pointStartState.begin(); stateIt != pointStartState.end(); ++stateIt){
		if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT) {
			refMesh->vcolors[stateIt->first] = stateIt->second;
		}
		else {
			refMesh->verts[stateIt->first] = stateIt->second;
		}
	}
	if (refBrush->Type() != TBT_MASK &&   refBrush->Type() != TBT_WEIGHT) {
		refMesh->SmoothNormals();
		if (startBVH == endBVH) {
			for (bvhNode = this->affectedNodes.begin(); bvhNode != affectedNodes.end(); ++bvhNode) {
				(*bvhNode)->UpdateAABB();
			}
		}
		else {
			refMesh->bvh = startBVH;
		}

		/*
		if(bvhValid) {
		for(bvhNode = this->affectedNodes.begin();bvhNode != affectedNodes.end(); ++bvhNode) {
		(*bvhNode)->UpdateAABB();

		}
		} else {
		refMesh->CreateBVH();
		}*/

	}
}

void TweakStroke::restoreEndState() {
	unordered_map<int, vec3>::iterator stateIt;
	unordered_set<AABBTree::AABBTreeNode*>::iterator bvhNode;
	for (stateIt = pointEndState.begin(); stateIt != pointEndState.end(); ++stateIt){
		if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT) {
			refMesh->vcolors[stateIt->first] = stateIt->second;
		}
		else {
			refMesh->verts[stateIt->first] = stateIt->second;
		}
	}

	if (refBrush->Type() != TBT_MASK && refBrush->Type() != TBT_WEIGHT) {
		refMesh->SmoothNormals();
		if (startBVH == endBVH) {
			for (bvhNode = this->affectedNodes.begin(); bvhNode != affectedNodes.end(); ++bvhNode) {
				(*bvhNode)->UpdateAABB();
			}
		}
		else {
			refMesh->bvh = endBVH;
		}

		/*
		if(bvhValid) {
		for(bvhNode = this->affectedNodes.begin();bvhNode != affectedNodes.end(); ++bvhNode) {
		(*bvhNode)->UpdateAABB();

		}
		}*/
	}
}

void TweakStroke::beginStroke(TweakPickInfo& pickInfo) {
	refBrush->strokeInit(refMesh, pickInfo);
	startBVH = refMesh->bvh;

	pts1 = (int*)malloc(refMesh->nVerts*sizeof(int));
	if (refBrush->isMirrored())
		pts2 = (int*)malloc(refMesh->nVerts*sizeof(int));
}

void TweakStroke::updateStroke(TweakPickInfo& pickInfo) {
	//set<int> points;
	//set<int> points2;
	//set<int>::iterator pI;
	vector<int> facets;
	vector<int> facets2;
	unordered_map<int, vec3> movedpoints;

	int nPts1 = 0;
	int nPts2 = 0;

	int brushType = refBrush->Type();

	TweakPickInfo mirrorPick = pickInfo;;
	//mirrorPick.origin = curOrigin;
	//mirrorPick.normal = originNormal;
	//mirrororigin.x *= -1;
	//mirrornormal.x *= -1;
	mirrorPick.origin.x *= -1;
	mirrorPick.normal.x *= -1;
	mirrorPick.facet = pickInfo.facetM;

	if (!newStroke) {
		if (!refBrush->checkSpacing(lastPoint, pickInfo.origin))
			return;
	}
	else {
		newStroke = false;
	}


	if (brushType == TBT_MOVE || brushType == TBT_XFORM) {
		// Move brush handles most operations differently than other brushes.  Mirroring, for instance, is done internally. 
		// most of the pick info values are ignored.
		if (!refBrush->queryPoints(refMesh, pickInfo, NULL, nPts1, facets, affectedNodes))
			return;

		if (nPts1 > outPositionCount) {
			if (outPositions != NULL) {
				delete[] outPositions;
			}
			outPositions = new vec3[nPts1];
			outPositionCount = nPts1;
		}
		refBrush->brushAction(refMesh, pickInfo, NULL, nPts1, outPositions);

		/*for(auto mP : movedpoints) {
			addPoint(mP.first,mP.second,0);
			}*/


		for (int i = 0; i < nPts1; i++){
			addPoint(refBrush->CachedPointIndex(i), outPositions[i], brushType);
		}


		/*for (pI = points.begin();pI!=points.end();++pI) {
			addPoint((*pI), movedpoints[(*pI)], 0);
			}*/
	}
	else {/*
	 if(!refBrush->queryPoints(refMesh, pickInfo, points,facets, affectedNodes))
	 return;
	 if(refBrush->isMirrored()) {
	 refBrush->queryPoints(refMesh, mirrorPick, points2,facets2,affectedNodes);
	 }
	 */
		if (!refBrush->queryPoints(refMesh, pickInfo, pts1, nPts1, facets, affectedNodes))
			return;
		if (refBrush->isMirrored()) {
			refBrush->queryPoints(refMesh, mirrorPick, pts2, nPts2, facets2, affectedNodes);
			//refBrush->queryPoints(refMesh, mirrorPick, pts1+nPts1, nPts2, facets2,affectedNodes);	
		}

		/*refBrush->brushAction(refMesh, pickInfo, pts1, nPts1, movedpoints);
		for(auto mP : movedpoints) {
		addPoint(mP.first,mP.second,0);
		}
		if(refBrush->isMirrored())  {
		refBrush->brushAction(refMesh, mirrorPick, pts2, nPts2, movedpoints);

		for(auto mP : movedpoints) {
		addPoint(mP.first,mP.second,0);
		}
		}*/

		int totalpoints = max(nPts1, nPts2);
		if (totalpoints > outPositionCount) {
			if (outPositions != NULL) {
				delete[] outPositions;
			}
			outPositions = new vec3[totalpoints];
			outPositionCount = totalpoints;
		}

		refBrush->brushAction(refMesh, pickInfo, pts1, nPts1, outPositions);
		for (int i = 0; i < nPts1; i++){
			addPoint(pts1[i], outPositions[i], brushType);
		}
		if (refBrush->isMirrored())  {
			refBrush->brushAction(refMesh, mirrorPick, pts2, nPts2, outPositions);
			for (int i = 0; i < nPts2; i++){
				addPoint(pts2[i], outPositions[i], brushType);
			}
		}

	}
	lastPoint = pickInfo.origin;

	if (refBrush->LiveNormals()) {
		refMesh->SmoothNormals();
	}

	if (refBrush->LiveBVH()) {
		unordered_set<AABBTree::AABBTreeNode*>::iterator bvhNode;
		for (bvhNode = affectedNodes.begin(); bvhNode != affectedNodes.end(); ++bvhNode)
			(*bvhNode)->UpdateAABB();
	}
}

void TweakStroke::endStroke() {
	unordered_set<AABBTree::AABBTreeNode*>::iterator bvhNode;
	if (refBrush->Type() == TBT_MOVE) {
		TB_Move* br = dynamic_cast<TB_Move*> (refBrush);
		if (br) {
			affectedNodes.swap(((TB_Move*)refBrush)->cachedNodes);
			affectedNodes.insert(((TB_Move*)refBrush)->cachedNodesM.begin(), ((TB_Move*)refBrush)->cachedNodesM.end()); /*
			for(bvhNode = ((TB_Move*) refBrush)->cachedNodes.begin(); bvhNode!=((TB_Move*) refBrush)->cachedNodes.end(); ++bvhNode )
			(*bvhNode)->UpdateAABB();
			for(bvhNode = ((TB_Move*) refBrush)->cachedNodesM.begin(); bvhNode!=((TB_Move*) refBrush)->cachedNodesM.end(); ++bvhNode )
			(*bvhNode)->UpdateAABB();
			*/
			((TB_Move*)refBrush)->cachedNodes.clear();
			((TB_Move*)refBrush)->cachedNodesM.clear();
		}
	}
	else if (refBrush->Type() == TBT_XFORM) {
		// recalc BVH 
		refMesh->CreateBVH();
		TB_XForm* br = dynamic_cast<TB_XForm*> (refBrush);
		if (br) {
		}
		//InvalidateBVH();
	}

	for (auto bvhNode : affectedNodes) {
		bvhNode->UpdateAABB();
	}
	//}
	if (!refBrush->LiveNormals()) {
		refMesh->SmoothNormals();
	}
	if (pts1) free(pts1);
	if (pts2) free(pts2);

	endBVH = refMesh->bvh;
}

void TweakStroke::addPoint(int point, vec3& newPos, int strokeType) {
	if (pointStartState.find(point) == pointStartState.end()) {
		pointStartState[point] = newPos;
	}
	if (refBrush->Type() == TBT_MASK || refBrush->Type() == TBT_WEIGHT) {
		pointEndState[point] = refMesh->vcolors[point];
	}
	else {
		pointEndState[point] = refMesh->verts[point];
	}
}

TweakBrush::TweakBrush(void) : radius(0.45f), focus(1.00f), inset(0.00f), strength(0.0015f), spacing(0.015f) {
	brushType = TBT_STANDARD;
	brushName = "Standard Brush";
	bMirror = false;
	bLiveBVH = true;
	bLiveNormals = true;
	bConnected = true;
}

TweakBrush::~TweakBrush(void) {
}

bool TweakBrush::checkSpacing(vec3& start, vec3& end) {
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
void TweakBrush::applyFalloff(vec3 &deltaVec, float dist) {
	// beyond the radius, always 0 stength.
	if (dist > radius) {
		deltaVec.x = deltaVec.y = deltaVec.z = 0.0f;
		return;
	}
	// at 0, always full strength
	if (dist == 0)
		return;

	float p = 1.0f;

	float x = dist / radius; // (radius - dist)/dist;
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

bool TweakBrush::queryPoints(mesh *refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, vector<int>& resultFacets, std::unordered_set<AABBTree::AABBTreeNode*> &affectedNodes) {
	vector<IntersectResult> IResults;

	if (!refmesh->bvh->IntersectSphere(pickInfo.origin, radius, &IResults))
		return false;

	bool* pointVisit = (bool*)calloc(refmesh->nVerts, sizeof(bool));

	if (bConnected) {
		bool* triVisit = (bool*)calloc(refmesh->nTris, sizeof(bool));
		refmesh->ConnectedPointsInSphere(pickInfo.origin, radius*radius, pickInfo.facet, triVisit, pointVisit, resultPoints, outResultCount, resultFacets);
		free(triVisit);
	}
	else {
		outResultCount = 0;
	}

	//unordered_set<int> resultPoints;
	tri t;
	for (unsigned int i = 0; i < IResults.size(); i++) {
		if (!bConnected) {
			resultFacets.push_back(IResults[i].HitFacet);
			t = refmesh->tris[IResults[i].HitFacet];
			if (!pointVisit[t.p1]){
				resultPoints[outResultCount++] = t.p1;
				pointVisit[t.p1] = true;
			}
			if (!pointVisit[t.p2]){
				resultPoints[outResultCount++] = t.p2;
				pointVisit[t.p2] = true;
			}
			if (!pointVisit[t.p3]){
				resultPoints[outResultCount++] = t.p3;
				pointVisit[t.p3] = true;
			}

		}
		affectedNodes.insert(IResults[i].bvhNode);
	}

	free(pointVisit);
	return true;
}

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3> &movedpoints) {
	unordered_set<int>::iterator pointIt;
	Mat4 xform;
	xform.Translate(pickInfo.normal * strength);
	vec3 vs;
	vec3 ve;
	vec3 vf;

	//for(pointIt = points.begin(); pointIt != points.end(); ++pointIt) {
	for (int i = 0; i < nPoints; i++) {
		//vs = refmesh->verts[(*pointIt)];
		//movedpoints[(*pointIt)] = vs;
		vs = refmesh->verts[points[i]];
		movedpoints[points[i]] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vs + ve;
		//refmesh->verts[(*pointIt)]=( vf );
		refmesh->verts[points[i]] = (vf);
	}
}

void TweakBrush::brushAction(mesh *refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_set<int>::iterator pointIt;
	Mat4 xform;
	xform.Translate(pickInfo.normal * strength);
	vec3 vs;
	vec3 ve;
	vec3 vf;

	//for(pointIt = points.begin(); pointIt != points.end(); ++pointIt) {
	for (int i = 0; i < nPoints; i++) {
		//vs = refmesh->verts[(*pointIt)];
		//movedpoints[(*pointIt)] = vs;
		vs = refmesh->verts[points[i]];
		movedpoints[i] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[points[i]].x);
		vf = vs + ve;
		//refmesh->verts[(*pointIt)]=( vf );
		refmesh->verts[points[i]] = (vf);
	}
}

TB_Mask::TB_Mask() :TweakBrush() {
	brushType = TBT_MASK;
	strength = 0.1f;
	focus = 5.0f;
	brushName = "Mask Brush";
}

TB_Mask::~TB_Mask() {
}

void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

	for (int i = 0; i<nPoints; i++) {

		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		ve.x += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		vf = vc + ve;
		if (vf.x > 1.0f) vf.x = 1.0f;
		refmesh->vcolors[points[i]] = (vf);
	}
}
	
void TB_Mask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

	for (int i = 0; i<nPoints; i++) {

		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		ve.x += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		vf = vc + ve;
		if (vf.x > 1.0f) vf.x = 1.0f;
		refmesh->vcolors[points[i]] = (vf);
	}
}

TB_Unmask::TB_Unmask() :TweakBrush() {
	brushType = TBT_MASK;
	strength = -0.1f;
	focus = 5.0f;
	brushName = "Unmask Brush";
}

TB_Unmask::~TB_Unmask() {
}

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

	for (int i = 0; i < nPoints; i++) {

		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[points[i]] = vc;

		ve = vc;
		ve.x += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		vf = vc + ve;
		if (vf.x < 0.0f) vf.x = 0.0f;
		refmesh->vcolors[points[i]] = (vf);
	}
}

void TB_Unmask::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

	for (int i = 0; i < nPoints; i++) {
		vs = refmesh->verts[points[i]];
		vc = refmesh->vcolors[points[i]];
		movedpoints[i] = vc;

		ve = vc;
		ve.x += strength;
		ve -= vc;

		applyFalloff(ve, pickInfo.origin.DistanceTo(vs));
		vf = vc + ve;
		if (vf.x < 0.0f) vf.x = 0.0f;
		refmesh->vcolors[points[i]] = (vf);
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
	b = NULL;
	lastMesh = NULL;
	brushName = "Smooth Brush";
}

TB_Smooth::~TB_Smooth() {
	if (b) {
		free(b);
		b = NULL;
	}
}

void TB_Smooth::lapFilter(mesh* refmesh, int* points, int nPoints, unordered_map <int, vec3>& wv) {
	//unordered_set<int> adjPoints;	
	unordered_map<int, vec3>::iterator mi;
	vec3 d;
	int adjPoints[1000];
	int c = 0;
	int a;

	for (int i = 0; i < nPoints; i++) {
		//adjPoints.clear();
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

void TB_Smooth::hclapFilter(mesh* refmesh, int* points, int nPoints, unordered_map <int, vec3>& wv) {
	//unordered_set<int> adjPoints;	
	unordered_map<int, vec3>::iterator mi;
	//unordered_map<int, vec3> b;

	if (refmesh != lastMesh) {
		if (b) free(b);
		b = (vec3*)calloc(refmesh->nVerts, sizeof(vec3));
		lastMesh = refmesh;
	}
	else {
		memset(b, 0, refmesh->nVerts * sizeof(vec3));
	}

	vec3 d;
	vec3 q;
	int adjPoints[1000];
	int a;
	int c;
	int i;
	// First step is to calculate the laplacian
	for (int p = 0; p < nPoints; p++) {
		i = points[p];
		//adjPoints.clear();
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
		//adjPoints.clear();
		c = refmesh->GetAdjacentPoints(j, adjPoints, 1000);
		if (c == 0) continue;
		d.x = d.y = d.z = 0;
		for (int n = 0; n < c; n++) {
			//if(b.find(adjPoints[n]) == b.end()) {
			//		continue;
			//}
			d += b[adjPoints[n]];
			//d.x += b[n].x;
			//d.y += b[n].y;
			//d.z += b[n].z;
		}

		// blend the new position and the average of the distance moved
		float avgB = (1 - hcBeta) / (float)c;
		wv[j] -= ((b[j] * hcBeta) + (d * avgB));

		if (refmesh->weldVerts.find(j) != refmesh->weldVerts.end()) {
			for (unsigned int v = 0; v < refmesh->weldVerts[j].size(); v++) {
				wv[refmesh->weldVerts[j][v]] = wv[j];
			}
		}

		//wv[j].x -= (hcBeta * b[j].x) + (avgB) * d.x;			
		//wv[j].y -= (hcBeta * b[j].y) + (avgB) * d.y;
		//wv[j].z -= (hcBeta * b[j].z) + (avgB) * d.z;
	}
}

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	unordered_map<int, vec3> wv;
	vec3 vs;
	// p := o

	for (int i = 0; i < nPoints; i++) {
		movedpoints[points[i]] = vec3(refmesh->verts[points[i]]);
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
	vec3 delta;
	if (strength != 1.0f) {
		for (int p = 0; p < nPoints; p++) {
			int i = points[p];
			vs = refmesh->verts[i];
			delta = wv[i] - refmesh->verts[i];
			delta *= strength;
			applyFalloff(delta, pickInfo.origin.DistanceTo(vs));

			if (refmesh->vcolors)
				delta = delta * (1.0f - refmesh->vcolors[i].x);
			refmesh->verts[i] += delta;
		}
	}
}

void TB_Smooth::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_map<int, vec3> wv;
	vec3 vs;
	// p := o

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
	vec3 delta;
	if (strength != 1.0f) {
		for (int p = 0; p < nPoints; p++) {
			int i = points[p];
			vs = refmesh->verts[i];
			delta = wv[i] - refmesh->verts[i];
			delta *= strength;
			applyFalloff(delta, pickInfo.origin.DistanceTo(vs));

			if (refmesh->vcolors)
				delta = delta * (1.0f - refmesh->vcolors[i].x);
			refmesh->verts[i] += delta;
		}
	}
}

TB_Move::TB_Move(void) : TweakBrush() {
	brushType = TBT_MOVE;
	brushName = "Move Brush";
	bLiveBVH = false;
	focus = 2.0f;
	strength = 0.1f;
	cachedPoints = NULL;
	cachedPointsM = NULL;
}

TB_Move::~TB_Move(void) {
	if (cachedPoints)
		free(cachedPoints);
	if (cachedPointsM)
		free(cachedPointsM);
}

bool TB_Move::strokeInit(mesh* refmesh, TweakPickInfo& pickInfo) {
	/*
	o = origin;
	mo = origin;
	mo.x = -mo.x;
	p = view;
	mp = view;
	mp.x = -mp.x;
	d = o.dot(p);
	md = mo.dot(mp);
	*/
	pick = pickInfo;
	mpick = pickInfo;
	mpick.origin.x = -mpick.origin.x;
	mpick.view.x = -mpick.view.x;
	mpick.facet = pickInfo.facetM;
	d = pick.origin.dot(pick.view);
	md = mpick.origin.dot(mpick.view);
	//cachedPoints.clear();
	cachedFacets.clear();
	cachedNodes.clear();
	cachedPositions.clear();
	nCachedPoints = 0;
	nCachedPointsM = 0;

	if (cachedPoints) {
		free(cachedPoints);
	}
	cachedPoints = (int*)malloc(refmesh->nVerts * sizeof(int));

	if (!TweakBrush::queryPoints(refmesh, pick, cachedPoints, nCachedPoints, cachedFacets, cachedNodes))
		return false;

	unordered_set<int>::iterator pointIt;

	//for(pointIt = cachedPoints.begin(); pointIt != cachedPoints.end(); ++pointIt) {
	for (int i = 0; i < nCachedPoints; i++)  {
		cachedPositions[cachedPoints[i]] = refmesh->verts[cachedPoints[i]];
	}

	if (bMirror) {

		if (cachedPointsM) {
			free(cachedPointsM);
		}
		cachedPointsM = (int*)malloc(refmesh->nVerts * sizeof(int));
		//cachedPointsM.clear();
		cachedFacetsM.clear();
		cachedNodesM.clear();
		TweakBrush::queryPoints(refmesh, mpick, cachedPointsM, nCachedPointsM, cachedFacetsM, cachedNodesM);

		//for(pointIt = cachedPointsM.begin(); pointIt != cachedPointsM.end(); ++pointIt) {
		for (int i = 0; i < nCachedPointsM; i++) {
			cachedPositions[cachedPointsM[i]] = refmesh->verts[cachedPointsM[i]];
		}
	}

	return true;
}

bool TB_Move::queryPoints(mesh* refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, vector<int>& resultFacets, unordered_set<AABBTree::AABBTreeNode*>& affectedNodes) {
	if (nCachedPoints == 0)
		return false;
	if (resultPoints != NULL) {
		memcpy(resultPoints, cachedPoints, nCachedPoints*sizeof(int));
		memcpy(resultPoints + nCachedPoints, cachedPointsM, nCachedPointsM*sizeof(int));
	}

	outResultCount = nCachedPoints + nCachedPointsM;

	//	if(bMirror) 
	//	resultPoints.insert(cachedPointsM.begin(), cachedPointsM.end());
	//	resultFacets = cachedFacets;
	//	affectedNodes = cachedNodes;

	return true;
}

void TB_Move::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints){
	unordered_set<int>::iterator pointIt;
	//origin = vec3(10,10,1);
	//origin = o + vec3(3,0,0);
	//vec3 v = origin - o;

	float dist = pickInfo.origin.dot(pick.view) - d;
	vec3 v = pickInfo.origin - pick.view*dist;
	vec3 dv = v - pick.origin;

	Mat4 xform;
	xform.Translate(dv*strength);
	vec3 vs;
	vec3 ve;
	vec3 vf;

	if (bMirror) {
		vec3 lmo;
		lmo.x = -pickInfo.origin.x;	lmo.y = pickInfo.origin.y; lmo.z = pickInfo.origin.z;
		dist = lmo.dot(mpick.view) - md;
		v = lmo - mpick.view*dist;
		dv = v - mpick.origin;
		Mat4 xformMirror;
		xformMirror.Translate(dv*strength);

		//for(pointIt = cachedPointsM.begin(); pointIt != cachedPointsM.end(); ++pointIt) {
		for (int p = 0; p < nCachedPointsM; p++) {
			int i = cachedPointsM[p];
			vs = cachedPositions[i];// refmesh->verts[(*pointIt)];
			movedpoints[i] = vs;
			ve = xformMirror * vs;
			ve -= vs;
			applyFalloff(ve, mpick.origin.DistanceTo(vs));
			if (refmesh->vcolors)
				ve = ve * (1.0f - refmesh->vcolors[i].x);
			vf = vs + ve;
			refmesh->verts[i] = (vf);
		}
	}

	//for(pointIt = cachedPoints.begin(); pointIt != cachedPoints.end(); ++pointIt) {	
	for (int p = 0; p < nCachedPoints; p++) {
		int i = cachedPoints[p];
		vs = cachedPositions[i];// refmesh->verts[(*pointIt)];
		movedpoints[i] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pick.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[i].x);
		vf = vs + ve;
		refmesh->verts[i] = (vf);
	}
}

void TB_Move::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints){
	unordered_set<int>::iterator pointIt;
	//origin = vec3(10,10,1);
	//origin = o + vec3(3,0,0);
	//vec3 v = origin - o;

	float dist = pickInfo.origin.dot(pick.view) - d;
	vec3 v = pickInfo.origin - pick.view*dist;
	vec3 dv = v - pick.origin;

	Mat4 xform;
	xform.Translate(dv*strength);
	vec3 vs;
	vec3 ve;
	vec3 vf;

	if (bMirror) {
		vec3 lmo;
		lmo.x = -pickInfo.origin.x;	lmo.y = pickInfo.origin.y; lmo.z = pickInfo.origin.z;
		dist = lmo.dot(mpick.view) - md;
		v = lmo - mpick.view*dist;
		dv = v - mpick.origin;
		Mat4 xformMirror;
		xformMirror.Translate(dv*strength);

		//for(pointIt = cachedPointsM.begin(); pointIt != cachedPointsM.end(); ++pointIt) {
		for (int p = 0; p < nCachedPointsM; p++) {
			int i = cachedPointsM[p];
			vs = cachedPositions[i];// refmesh->verts[(*pointIt)];
			movedpoints[p + nCachedPoints] = vs;
			ve = xformMirror * vs;
			ve -= vs;
			applyFalloff(ve, mpick.origin.DistanceTo(vs));
			if (refmesh->vcolors)
				ve = ve * (1.0f - refmesh->vcolors[i].x);
			vf = vs + ve;
			refmesh->verts[i] = (vf);
		}
	}

	//for(pointIt = cachedPoints.begin(); pointIt != cachedPoints.end(); ++pointIt) {	
	for (int p = 0; p < nCachedPoints; p++) {
		int i = cachedPoints[p];
		vs = cachedPositions[i];// refmesh->verts[(*pointIt)];
		movedpoints[p] = vs;
		ve = xform * vs;
		ve -= vs;
		applyFalloff(ve, pick.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[i].x);
		vf = vs + ve;
		refmesh->verts[i] = (vf);
	}
}

void TB_Move::GetWorkingPlane(vec3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.view;
	outPlaneDist = d;
}

TB_XForm::TB_XForm() {
	//	origin.Zero();
	//	axis.Zero();
	//	delta.Zero();
	brushType = TBT_XFORM;
	brushName = "Transform Brush";
	bLiveBVH = false;
	bLiveNormals = false;
	focus = 1.0f;
	strength = 1.0f;
	cachedPositions = NULL;
	xformType = 0;
}

TB_XForm::~TB_XForm() {
	if (cachedPositions) {
		free(cachedPositions);
	}
}

void TB_XForm::GetWorkingPlane(vec3& outPlaneNormal, float& outPlaneDist) {
	outPlaneNormal = pick.normal;
	outPlaneDist = pick.origin.dot(pick.normal);
}

bool TB_XForm::strokeInit(mesh* refmesh, TweakPickInfo& pickInfo) {
	pick = pickInfo;
	d = pick.origin.dot(pick.normal);

	cachedPositions;

	if (cachedPositions) {
		free(cachedPositions);
	}
	//cachedPoints = (int*) malloc(refmesh->nVerts * sizeof(int));
	cachedPositions = (vec3*)malloc(refmesh->nVerts * sizeof(vec3));
	nCachedPoints = refmesh->nVerts;
	for (int i = 0; i < nCachedPoints; i++)	{
		cachedPositions[i] = refmesh->verts[i];
	}
	return true;
}

bool TB_XForm::queryPoints(mesh* refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, vector<int>& resultFacets, unordered_set<AABBTree::AABBTreeNode*>& affectedNodes) {
	if (nCachedPoints == 0)
		return false;
	if (resultPoints != NULL) {
		for (int i = 0; i < refmesh->nVerts; i++)	{
			resultPoints[i] = i;
		}
	}
	outResultCount = refmesh->nVerts;
	return true;
}

void TB_XForm::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	float dist = pickInfo.origin.dot(pick.normal) - d;
	vec3 v = pickInfo.origin;
	//v.x *= pick.view.x;
	//v.y *= pick.view.y;
	//v.z *= pick.view.z;
	v -= pick.view * dist;
	vec3 dv = v - pick.origin;
	dv.x *= pick.view.x;
	dv.y *= pick.view.y;
	dv.z *= pick.view.z;
	//pick.view * dist; //

	Mat4 xform;
	//xform.Translate(dv * strength);
	xform.Rotate(2 * (float)PI * 10 / dv.x, pick.normal.x, pick.normal.y, pick.normal.z);
	vec3 vs;
	vec3 ve;
	vec3 vf;

	for (int p = 0; p < nCachedPoints; p++) {
		//int i = cachedPoints[p];
		vs = cachedPositions[p]; // refmesh->verts[(*pointIt)];
		movedpoints[p] = vs;
		ve = xform * vs;
		ve -= vs;
		//applyFalloff(ve, pick.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[p].x);
		vf = vs + ve;
		refmesh->verts[p] = (vf);
	}
}

void TB_XForm::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	//float dist = pickInfo.origin.dot(pick.normal) - d;
	//if (dist > .0010f) 
	//	dist = 1.0f;

	vec3 v = pickInfo.origin;
	//v.x *= pick.view.x;
	//v.y *= pick.view.y;
	//v.z *= pick.view.z;
	//v -= pick.view * dist;
	vec3 dv = v - pick.origin;

	dv.x *= pick.view.x;
	dv.y *= pick.view.y;
	dv.z *= pick.view.z;
	//pick.view * dist; //

	Mat4 xform;
	float dist = 0.0f;
	if (fabs(dv.x) > EPSILON) {
		dist = dv.x;
	}
	else if (fabs(dv.y) > EPSILON) {
		dist = dv.y;
	}
	else if (fabs(dv.z) > EPSILON) {
		dist = dv.z;
	}


	if (xformType == 0) {
		xform.Translate(dv * strength);
	}
	else if (xformType == 1) {
		xform.PushTranslate(pick.center);
		vec3 a = pick.origin - pick.center;
		vec3 b = pickInfo.origin - pick.center;
		vec3 dir = a.cross(b);
		float sign = dir.dot(pick.normal);
		float angle = a.angle(pickInfo.origin - pick.center);
		if (sign < 0)
			angle = -angle;

		xform.PushRotate(angle, pick.normal);
		xform.PushTranslate(pick.center * -1.0f);
	}
	else if (xformType == 2) {
		dist = fabs(1.0f + dist / 10.0f);
		xform.Scale(dist, dist, dist);
	}

	vec3 vs;
	vec3 ve;
	vec3 vf;

	for (int p = 0; p < nCachedPoints; p++) {
		//int i = cachedPoints[p];
		vs = cachedPositions[p];// refmesh->verts[(*pointIt)];
		movedpoints[p] = vs;
		ve = xform * vs;
		ve -= vs;
		//applyFalloff(ve, pick.origin.DistanceTo(vs));
		if (refmesh->vcolors)
			ve = ve * (1.0f - refmesh->vcolors[p].x);
		vf = vs + ve;
		refmesh->verts[p] = (vf);
	}
}

TB_Weight::TB_Weight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = 0.0015f;
	brushName = "Weight Paint";
}

TB_Weight::~TB_Weight() {
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

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
}

void TB_Weight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

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
		if (vf.y > 1.0f) vf.y = 1.0f;
		refmesh->vcolors[points[i]] = vf;
	}
}

TB_Unweight::TB_Unweight() :TweakBrush() {
	brushType = TBT_WEIGHT;
	strength = -0.0015f;
	brushName = "Weight Erase";
}

TB_Unweight::~TB_Unweight() {
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, unordered_map<int, vec3>& movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

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
}

void TB_Unweight::brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, vec3* movedpoints) {
	unordered_set<int>::iterator pointIt;
	vec3 vs;
	vec3 vc;
	vec3 ve;
	vec3 vf;

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
}
