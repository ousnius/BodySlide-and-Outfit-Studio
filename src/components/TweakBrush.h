/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

/* Vertex tweaking classes. Patterned off of 3d sculpting applications like ZBrush.
	Process overview:
	1) App enters edit mode.
		- default standard brush is created and selected.
		- undo stack is created.
		- the original shape of the model is saved as a duplicate hidden mesh.
	2) User begins a stroke by pressing down the mouse button while over the active mesh.
		- new stroke is created and the active mesh and brush are saved in the stroke data.
		- Begin Stroke is called to initialize the stroke.
		- Update Stroke is called.
			- Brush queries mesh for vertices in its realm of influence.
			- Stroke saves result BVH facet pointers in the affectednodes set.
			- Stroke saves result set of vertices and their positions in the pointstartstate map.
			- Brush applies transformation to result vertices.
			- Stroke saves transformed vertices to pointcurrentstate map.
		- mesh->updateBVH is called.
		- Window is redrawn.
	3) User continues stroke by dragging mouse with button still down.
		- Update stroke is called.
			- Vertices not already in the pointstartstate map are added with their original positions.
		- BVH is updated and the window is redrawn.
	4) User releases mouse button at end of stroke.
		- stroke is saved to the undo stack. If the stack is full, the oldest state is erased.
	5) User uses the undo function.
		- mesh data is reverted to the stroke's pointstartstate map.
		- the affectedNodes set is used to update the BVH.
		- the undo stack position is decremented.
		- the window is redrawn.
	5) User uses the redo function.
		- mesh data is set to the stroke's pointendstate map.
		- the affectedNodes set is used to update the BVH.
		- the undo stack position is incremented.
		- the window is rerawn.
	6) User performs a new edit after using the undo function.
		- the edit is performed as normal (stroke begin, stroke update, stroke end).
		- the undo stack position indicator is checked against the length of the stack.
		- if there are states after the current position, those states are discarded and a new stroke is added to the stack.
	7) User completes changes and exits edit mode.
		- user is prompted to save the changes.
	8) User chooses to save it.
		- if the user chooses to save as an .obj file, the current mesh is exported in .obj format.
		- if the user chooses to save as a .bsd file,
			- the last stroke's position information is compared with the saved original mesh data to generate
				diff information, which is saved to the .bsd file.
			- a small XML file containing a slider set entry is saved?
	9) App enters view mode (if user turned off editing rather than close the window)
		- undo history is discarded.
		- original mesh data is retained until window exits.
	10) User changes brush with 1-9 keys.
	11) User changes active mesh with ALT + 1-9 keys
*/

#pragma once

#include "Mesh.h"

#include <future>

const int TB_MAX_UNDO = 40;

enum TweakBrushType {
	TBT_STANDARD = 1,
	TBT_MOVE,
	TBT_MASK,
	TBT_WEIGHT,
	TBT_XFORM
};


// Collecton of information that identifies the position and attributes where a brush stroke is taking place.
class TweakPickInfo {
public:
	Vector3 origin;			// Point on the surface of the mesh that was touched.
	Vector3 normal;			// Surface normal at the point of impact.
	Vector3 view;				// View vector.
	Vector3 center;			// Center point for a transform.
	int facet;				// Facet index touched.
	int facetM;				// Mirrored facet index touched (X-axis mirror).
};

class TweakBrushMeshCache {
public:
	int* cachedPoints;
	int nCachedPoints;
	std::vector<int> cachedFacets;
	int* cachedPointsM;
	int nCachedPointsM;
	std::vector<int> cachedFacetsM;
	std::unordered_map<int, Vector3> cachedPositions;
	std::unordered_set<AABBTree::AABBTreeNode*> cachedNodes;
	std::unordered_set<AABBTree::AABBTreeNode*> cachedNodesM;

	TweakBrushMeshCache() {
		cachedPoints = nullptr;
		cachedPointsM = nullptr;

		nCachedPoints = 0;
		nCachedPointsM = 0;
	}

	~TweakBrushMeshCache() {
		if (cachedPoints)
			delete[] cachedPoints;
		if (cachedPointsM)
			delete[] cachedPointsM;
	}
};


class TweakBrush {
protected:
	TweakBrushType brushType;
	std::string brushName;
	float radius;
	float focus;			// Focus between 1 and 5.
	float strength;
	float inset;			// Normally 0. Values between 0 and 1 increase displacement, values below 0 reduce displacement.
	float spacing;			// Distance between points; movements less than this distance don't update the stroke.
	bool bMirror;			// X-axis mirror function
	bool bLiveBVH;			// Update BVH at each update instead of at stroke completion.
	bool bLiveNormals;		// Update mesh normals at each update instead of at stroke completion.
	bool bConnected;		// Operate on connected vertices only.

	std::unordered_map<mesh*, TweakBrushMeshCache> cache;

public:
	TweakBrush();
	virtual ~TweakBrush();

	int Type() {
		return brushType;
	}
	std::string Name() {
		return brushName;
	}

	TweakBrushMeshCache* getCache(mesh* m) {
		return &cache[m];
	}

	virtual float getRadius() {
		return radius;
	}
	virtual float getStrength() {
		return strength * 10.0f;
	}
	virtual float getFocus() {
		return focus / 5.0f;
	}
	virtual float getSpacing() {
		return spacing;
	}
	virtual void setRadius(float newRadius) {
		radius = newRadius;
	}
	virtual void setFocus(float newFocus) {
		focus = newFocus * 5.0f;
	}
	virtual void setStrength(float newStr) {
		strength = newStr / 10.0f;
	}
	virtual void setSpacing(float newSpacing) {
		spacing = newSpacing;
	}
	virtual void setLiveNormals(bool newLiveNormals = true) {
		bLiveNormals = newLiveNormals;
	}

	virtual int CachedPointIndex(mesh*, int) {
		return 0;
	}

	virtual void setMirror(bool wantMirror = true) {
		bMirror = wantMirror;
	}
	virtual void setConnected(bool wantConnected = true) {
		bConnected = wantConnected;
	}
	virtual bool isMirrored() {
		return bMirror;
	}
	virtual bool LiveBVH() {
		return bLiveBVH;
	}
	virtual bool LiveNormals() {
		return bLiveNormals;
	}

	// Stroke initialization interface, allows a brush to set up initial conditions.
	// Default implementation is to return true. Return false to cancel stroke based on provided data.
	virtual bool strokeInit(std::vector<mesh*>, TweakPickInfo&) {
		return true;
	}

	// Using the start and end points, determine if enough distance has been covered to satisfy the spacing setting.
	virtual bool checkSpacing(Vector3& start, Vector3& end);

	// Standard falloff function, used by most brushes
	// y = (cos((pi/2)*x) * sqrt(cos((pi/2)*x))) ^ focus
	// Focus values between 0 and 1 give a spherical curve, values over 1 give a peaked curve.
	virtual void applyFalloff(Vector3& deltaVec, float dist);

	// Get the list of points, facets and BVH nodes within the brush sphere of influence.
	// Normally, the origin point is used for sphere center and assumed to be an arbitrary point on the surface.
	// Optionally, the operation can use the nearest vertex  on the mesh as the center point, using the provided facet to determine candidate points.
	// Also optionally, the query can return only connected points within the sphere.
	//virtual bool queryPoints (mesh* refmesh, TweakPickInfo& pickInfo, set<int>& resultPoints, vector<int>& resultFacets, set<AABBTree::AABBTreeNode*>& affectedNodes);

	virtual bool queryPoints(mesh *refmesh, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, std::vector<int>& resultFacets, std::unordered_set<AABBTree::AABBTreeNode*> &affectedNodes);

	// Apply the brush effect to the mesh, modifying the points in the set provided.
	// Overridden versions should return the original point positions in the movedpoints map.
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);

	// Version using a pre-allocated array of vectors matching the order of the points array.
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
};

class TB_Mask : public TweakBrush {
public:
	TB_Mask();
	virtual ~TB_Mask();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo&) {
		return true;
	}

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
	virtual bool checkSpacing(Vector3&, Vector3&) {
		return true;
	}
};

class TB_Unmask : public TweakBrush {
public:
	TB_Unmask();
	virtual ~TB_Unmask();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo&) {
		return true;
	}

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
	virtual bool checkSpacing(Vector3&, Vector3&) {
		return true;
	}
};

class TB_Deflate : public TweakBrush {
public:
	TB_Deflate();
	virtual ~TB_Deflate();
	virtual void setStrength(float newStr);
	virtual float getStrength();
};


// Smooth brush implementing a laplacian smooth function with HC-Smooth modifier.
// Default is 2 iterations but can be configured for more or less.
class TB_Smooth : public TweakBrush {
	int iterations;
	byte method;	// 0 for laplacian, 1 for HC-Smooth.
	float hcAlpha;			// Blending constants.
	float hcBeta;

	Vector3* b;				// Scratch space used in the hc-lap smooth filter.
	mesh* lastMesh;			// Last mesh smoothed, used to sync the hc-lap smooth scratch space.

	// Laplacian smoothing filter. Points are the set of point indices into refmesh to smooth.
	// wv is the current position of those points. This function can be called iteratively, reusing wv.
	void lapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv);

	// Improved laplacian smoothing filter (HC-Smooth) points are the set of point indices into refmesh to smooth.
	// wv is the current position of those points. This function can be called iteratively, reusing wv.
	// This algo is much slower than lap, but tries to maintain mesh volume.
	void hclapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv);

public:
	TB_Smooth();
	virtual ~TB_Smooth();

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
	virtual bool checkSpacing(Vector3&, Vector3&) {
		return true;
	}
};

// Move brush behavior is significantly different from other brush types.
// The largest difference is that the brush itself caches the initial set of vertices and their positions.
// The cached info is reused on each brush update. Additionally, there is no spacing.
class TB_Move : public TweakBrush {
	TweakPickInfo pick;
	TweakPickInfo mpick;
	float d;				// Plane dist.
	float md;

public:
	TB_Move();
	virtual ~TB_Move();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo& pickInfo);
	virtual bool queryPoints(mesh* m, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, std::vector<int>& resultFacets, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes);
	virtual void brushAction(mesh* m, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* m, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
	virtual bool checkSpacing(Vector3&, Vector3&) {
		return true;
	}

	void GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist);
	int CachedPointIndex(mesh* m, int query) {
		TweakBrushMeshCache* meshCache = &cache[m];
		if (query >= meshCache->nCachedPoints)
			return meshCache->cachedPointsM[query - meshCache->nCachedPoints];
		else
			return meshCache->cachedPoints[query];
	}
};

class TB_XForm : public TweakBrush {
	TweakPickInfo pick;
	float d;				// Plane dist,
	int xformType;			// 0 = Move, 1 = Rotate, 2 = Scale, 3 = Uniform Scale

public:
	TB_XForm();
	virtual ~TB_XForm();

	void GetWorkingPlane(Vector3& outPlaneNormal, float& outPlaneDist);
	int CachedPointIndex(mesh*, int query) {
		return query;
	}
	void SetXFormType(int type) {
		xformType = type;
	}

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo& pickInfo);
	virtual bool queryPoints(mesh* m, TweakPickInfo& pickInfo, int* resultPoints, int& outResultCount, std::vector<int>& resultFacets, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes);
	virtual void brushAction(mesh* m, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* m, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
	virtual bool checkSpacing(Vector3&, Vector3&) {
		return true;
	}
};

class TB_Weight : public TweakBrush {
public:
	std::string refBone;
	bool bFixedWeight;

	TB_Weight();
	virtual ~TB_Weight();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo&) {
		return true;
	}

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
};

class TB_Unweight : public TweakBrush {
public:
	std::string refBone;

	TB_Unweight();
	virtual ~TB_Unweight();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo&) {
		return true;
	}

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
};

class TB_SmoothWeight : public TweakBrush {
public:
	std::string refBone;
	int iterations;
	byte method;			// 0 for laplacian, 1 for HC-Smooth.
	float hcAlpha;			// Blending constants.
	float hcBeta;

	Vector3* b;				// Scratch space used in the hc-lap smooth filter.
	mesh* lastMesh;			// Last mesh smoothed, used to sync the hc-lap smooth scratch space.

	void lapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv);
	void hclapFilter(mesh* refmesh, int* points, int nPoints, std::unordered_map<int, Vector3>& wv);

	TB_SmoothWeight();
	virtual ~TB_SmoothWeight();

	virtual bool strokeInit(std::vector<mesh*> refMeshes, TweakPickInfo&) {
		return true;
	}

	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, std::unordered_map<int, Vector3>& movedpoints);
	virtual void brushAction(mesh* refmesh, TweakPickInfo& pickInfo, int* points, int nPoints, Vector3* movedpoints);
};

class TweakStroke {
	std::vector<mesh*> refMeshes;
	TweakBrush* refBrush;
	bool newStroke;
	Vector3 lastPoint;

	std::unordered_map<mesh*, std::shared_ptr<AABBTree>> startBVH;
	std::unordered_map<mesh*, std::shared_ptr<AABBTree>> endBVH;

	static std::unordered_map<mesh*, Vector3*> outPositions;
	static std::unordered_map<mesh*, int> outPositionCount;
	static int nStrokes;

	static std::vector<std::future<void>> normalUpdates;

	std::unordered_map<mesh*, int*> pts1;
	std::unordered_map<mesh*, int*> pts2;

	// When the mesh BVH is recalculated, historical BVH nodes are broken.
	// This lets us keep the undo history at the cost of forcing a full recalc for each undo/redo.
	bool bvhValid;

public:
	TweakStroke(std::vector<mesh*> meshes, TweakBrush* theBrush) {
		newStroke = true;
		refMeshes = meshes;
		refBrush = theBrush;
		bvhValid = true;
		nStrokes++;
	}

	~TweakStroke() {
		nStrokes--;
		if (nStrokes == 0) {
			for (auto &pos : outPositions)
				delete[] pos.second;

			outPositions.clear();
			outPositionCount.clear();
		}
	}

	std::unordered_map<mesh*, std::unordered_map<int, Vector3>> pointStartState;
	std::unordered_map<mesh*, std::unordered_map<int, Vector3>> pointEndState;
	std::unordered_map<mesh*, std::unordered_set<AABBTree::AABBTreeNode*>> affectedNodes;

	void addPoint(mesh* m, int point, Vector3& newPos);

	void InvalidateBVH() {
		bvhValid = false;
	}
	void PushBVH(std::vector<mesh*> meshes) {
		for (auto &m : meshes)
			endBVH[m] = m->bvh;
	}
	void RestoreStartState(mesh* m);
	void RestoreEndState(mesh* m);

	void beginStroke(TweakPickInfo& pickInfo);
	void updateStroke(TweakPickInfo& pickInfo);
	void endStroke();

	int BrushType() {
		return refBrush->Type();
	}
	std::string BrushName() {
		return refBrush->Name();
	}
	TweakBrush* GetRefBrush() {
		return refBrush;
	}
	std::vector<mesh*> GetRefMeshes() {
		return refMeshes;
	}
};

class TweakUndo {
	int curState = -1;
	std::vector<TweakStroke*> strokes;

public:
	TweakUndo();
	~TweakUndo();

	TweakStroke* CreateStroke(std::vector<mesh*> refMeshes, TweakBrush* refBrush);
	void addStroke(TweakStroke* stroke);
	bool backStroke(const std::vector<mesh*>& validMeshes);
	bool forwardStroke(const std::vector<mesh*>& validMeshes);
	void Clear();

	TweakStroke* GetCurStateStroke() {
		if (curState == -1)
			return nullptr;

		return strokes[curState];
	}

	std::vector<mesh*> GetCurStateMeshes() {
		std::vector<mesh*> meshes;
		if (curState == -1)
			return meshes;

		meshes = strokes[curState]->GetRefMeshes();
		return meshes;
	}

	std::vector<mesh*> GetNextStateMeshes() {
		std::vector<mesh*> meshes;
		int sz = strokes.size();
		sz -= 1;

		if (curState >= sz)
			return meshes;

		meshes = strokes[curState + 1]->GetRefMeshes();
		return meshes;
	}

	void PushBVH() {
		TweakStroke* curstroke = GetCurStateStroke();
		if (curstroke)
			curstroke->PushBVH(curstroke->GetRefMeshes());
	}

	void InvalidateHistoricalBVH() {
		for (auto &s : strokes)
			s->InvalidateBVH();
	}
};
