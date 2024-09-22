/*
BodySlide and Outfit Studio
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
#include "UndoState.h"

#include <future>

// Collecton of information that identifies the position and attributes where a brush stroke is taking place.
class TweakPickInfo {
public:
	// All vectors are in model space, not mesh
	nifly::Vector3 origin; // Point on the surface of the mesh that was touched.
	nifly::Vector3 normal; // Surface normal at the point of impact.
	nifly::Vector3 view;   // View vector.
	nifly::Vector3 center; // Center point for a transform.
};

class TweakBrushMeshCache {
public:
	std::vector<int> cachedPoints;
	std::vector<int> cachedPointsM;
	int nCachedPoints = 0;
	int nCachedPointsM = 0;
	std::unordered_set<AABBTree::AABBTreeNode*> cachedNodes;
	std::unordered_set<AABBTree::AABBTreeNode*> cachedNodesM;
	std::vector<nifly::Vector3> positionData;
	std::unique_ptr<nifly::Vector3[]> startNorms;
};


class TweakBrush {
public:
	enum class BrushType { Inflate = 1, Move, Mask, Weight, Color, Alpha, Transform, Undiff };

protected:
	BrushType brushType;
	std::string brushName;
	float radius;
	float focus; // Focus between 0 and 1.
	float strength;
	float spacing;	   // Distance between points; movements less than this distance don't update the stroke.
	bool bMirror;	   // X-axis mirror enabled
	bool bLiveBVH;	   // Update BVH at each update instead of at stroke completion.
	bool bLiveNormals; // Update mesh normals at each update instead of at stroke completion.
	bool bConnected;   // Operate on connected vertices only.
	bool restrictPlane = false;
	bool restrictNormal = false;

	std::unordered_map<Mesh*, TweakBrushMeshCache> cache;

public:
	TweakBrush();
	virtual ~TweakBrush();

	BrushType Type() { return brushType; }
	std::string Name() { return brushName; }

	virtual UndoType GetUndoType() { return UndoType::VertexPosition; }

	TweakBrushMeshCache* getCache(Mesh* m) { return &cache[m]; }

	virtual float getRadius() { return radius; }
	virtual float getStrength() { return strength; }
	virtual float getFocus() { return focus; }
	virtual float getSpacing() { return spacing; }
	virtual void setRadius(float newRadius) { radius = newRadius; }
	virtual void setFocus(float newFocus) { focus = newFocus; }
	virtual void setStrength(float newStr) { strength = newStr; }
	virtual void setSpacing(float newSpacing) { spacing = newSpacing; }

	virtual void resetSettings() {
		radius = 0.45f;
		focus = 0.5f;
		strength = 0.0015f;
		spacing = 0.015f;
	};

	virtual int CachedPointIndex(Mesh*, int) { return 0; }

	virtual bool isMirrored() { return bMirror; }
	virtual void setMirror(bool wantMirror = true) { bMirror = wantMirror; }

	virtual bool isConnected() { return bConnected; }
	virtual void setConnected(bool wantConnected = true) { bConnected = wantConnected; }
	virtual void setRestrictPlane(bool on) { restrictPlane = on; }
	virtual void setRestrictNormal(bool on) { restrictNormal = on; }
	virtual bool LiveBVH() { return bLiveBVH; }
	virtual bool LiveNormals() { return bLiveNormals; }

	virtual bool NeedMirrorMergedQuery() { return false; }
	virtual bool NeedStartNorms() { return restrictPlane || restrictNormal; }

	// Stroke initialization interface, allows a brush to set up initial conditions.
	virtual void strokeInit(const std::vector<Mesh*>&, TweakPickInfo&, UndoStateProject&);
	virtual void strokeInit(const std::vector<Mesh*>&, TweakPickInfo&, UndoStateProject&, std::vector<std::vector<nifly::Vector3>>&) {}

	// Using the start and end points, determine if enough distance has been covered to satisfy the spacing setting.
	virtual bool checkSpacing(nifly::Vector3& start, nifly::Vector3& end);

	virtual float getFalloff(float dist, float meshradius);
	virtual void applyFalloff(nifly::Vector3& deltaVec, float dist, float meshradius);

	// Get the list of points, facets and BVH nodes within the brush sphere of influence.
	// Normally, the origin point is used for sphere center and assumed to be an arbitrary point on the surface.
	// Optionally, the operation can use the nearest vertex  on the mesh as the center point, using the provided facet to determine candidate points.
	// Also optionally, the query can return only connected points within the sphere.

	virtual bool queryPoints(
		Mesh* refmesh, TweakPickInfo& pickInfo, TweakPickInfo& mirrorPick, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes);

	// Apply the brush effect to the mesh
	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss) = 0;
};

class ClampBrush {
public:
	float clampMaxValue = 0.0f;
};

class TB_Inflate : public TweakBrush {
public:
	TB_Inflate();
	virtual ~TB_Inflate();

	virtual float getStrength() { return strength * 10.0f; }
	virtual void setStrength(float newStr) { strength = newStr / 10.0f; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Mask : public TweakBrush {
public:
	TB_Mask();
	virtual ~TB_Mask();

	virtual UndoType GetUndoType() { return UndoType::Mask; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 1.0f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }
};

class TB_Unmask : public TweakBrush {
public:
	TB_Unmask();
	virtual ~TB_Unmask();

	virtual UndoType GetUndoType() { return UndoType::Mask; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = -1.0f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }
};

class TB_SmoothMask : public TweakBrush {
public:
	uint8_t method; // 0 for laplacian, 1 for HC-Smooth.
	float hcAlpha;	// Blending constants.
	float hcBeta;

	void lapFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv);
	void hclapFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv, UndoStateShape& uss);
	// Balanced-pair parabola-fit smoothing filter.  This smoothing filter
	// uses only balanced pairs of neighboring vertices.  It fits a parabola
	// through each balanced pair to determine the destination.
	void bppfFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv, UndoStateShape& uss);

	TB_SmoothMask();
	virtual ~TB_SmoothMask();

	virtual UndoType GetUndoType() { return UndoType::Mask; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.2f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Deflate : public TB_Inflate {
public:
	TB_Deflate();
	virtual ~TB_Deflate();

	virtual void setStrength(float newStr);
	virtual float getStrength();

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = -0.0015f;
	}
};


// Smooth brush implementing a laplacian smooth function with HC-Smooth modifier.
class TB_Smooth : public TweakBrush {
	uint8_t method; // 0 for laplacian, 1 for HC-Smooth.
	float hcAlpha;	// Blending constants.
	float hcBeta;

	// Laplacian smoothing filter. Points are the set of point indices into refmesh to smooth.
	// wv is the current position of those points. This function can be called iteratively, reusing wv.
	void lapFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv);

	// Improved laplacian smoothing filter (HC-Smooth) points are the set of point indices into refmesh to smooth.
	// wv is the current position of those points. This function can be called iteratively, reusing wv.
	// This algo is much slower than lap, but tries to maintain mesh volume.
	void hclapFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv, UndoStateShape& uss);

	// Balanced-pair circle-fit smoothing filter.  This smoothing filter
	// uses only balanced pairs of neighboring vertices and tries
	// to fit a circle through each pair to determine the destination of the
	// point (using normals).
	void bpcfFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, nifly::Vector3>& wv);

public:
	TB_Smooth();
	virtual ~TB_Smooth();

	virtual bool NeedStartNorms() { return true; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.1f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }
};


// Undiff brush moving points towards their base position
class TB_Undiff : public TweakBrush {
public:
	TB_Undiff();
	virtual ~TB_Undiff();

	virtual void strokeInit(const std::vector<Mesh*>&, TweakPickInfo&, UndoStateProject&, std::vector<std::vector<nifly::Vector3>>&);
	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }

	virtual float getStrength() { return strength * 10.0f; }
	virtual void setStrength(float newStr) { strength = newStr / 10.0f; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.01f;
	}
};

// Move brush behavior is significantly different from other brush types.
// The largest difference is that the brush itself caches the initial set of vertices and their positions.
// The cached info is reused on each brush update. Additionally, there is no spacing.
class TB_Move : public TweakBrush {
	TweakPickInfo pick;
	TweakPickInfo mpick;
	float d = 0.0f; // Plane dist.
	float md = 0.0f;

public:
	TB_Move();
	virtual ~TB_Move();

	virtual void strokeInit(const std::vector<Mesh*>&, TweakPickInfo&, UndoStateProject&);

	virtual bool queryPoints(
		Mesh* m, TweakPickInfo& pickInfo, TweakPickInfo& mirrorPick, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes);
	virtual void brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 1.0f;
	}

	void GetWorkingPlane(nifly::Vector3& outPlaneNormal, float& outPlaneDist);
	int CachedPointIndex(Mesh* m, int query) {
		TweakBrushMeshCache* meshCache = &cache[m];
		if (query >= meshCache->nCachedPoints)
			return meshCache->cachedPointsM[query - meshCache->nCachedPoints];
		else
			return meshCache->cachedPoints[query];
	}
};

class TB_XForm : public TweakBrush {
	TweakPickInfo pick;
	int xformType = 0; // 0 = Move, 1 = Rotate, 2 = Scale, 3 = Uniform Scale

public:
	TB_XForm();
	virtual ~TB_XForm();

	void GetWorkingPlane(nifly::Vector3& outPlaneNormal, float& outPlaneDist);
	int CachedPointIndex(Mesh*, int query) { return query; }
	void SetXFormType(int type) { xformType = type; }

	virtual void strokeInit(const std::vector<Mesh*>&, TweakPickInfo&, UndoStateProject&);
	virtual bool queryPoints(
		Mesh* m, TweakPickInfo& pickInfo, TweakPickInfo& mirrorPick, int* resultPoints, int& outResultCount, std::unordered_set<AABBTree::AABBTreeNode*>& affectedNodes);
	virtual void brushAction(Mesh* m, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 1.0f;
	}
};

class AnimInfo;

class TB_Weight : public TweakBrush {
public:
	bool bFixedWeight;
	AnimInfo* animInfo;
	// boneNames: first is bone being edited; second is x-mirror bone if
	// bXMirrorBone is true; the rest are normalize bones
	std::vector<std::string> boneNames, lockedBoneNames;
	// bSpreadWeight: if true, leftover weight is spread across normalize bones.
	bool bSpreadWeight;
	// bXMirrorBone:  if true, boneNames[1] is the x-mirror bone
	bool bXMirrorBone;
	bool bNormalizeWeights = false;

	TB_Weight();
	virtual ~TB_Weight();

	virtual UndoType GetUndoType() { return UndoType::Weight; }
	virtual bool NeedMirrorMergedQuery() { return bMirror || bXMirrorBone; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.015f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Unweight : public TweakBrush {
public:
	AnimInfo* animInfo;
	// boneNames: first is bone being edited; second is x-mirror bone if
	// bXMirrorBone is true; the rest are normalize bones
	std::vector<std::string> boneNames, lockedBoneNames;
	// bSpreadWeight: if true, leftover weight is spread across normalize bones.
	bool bSpreadWeight;
	// bXMirrorBone:  if true, boneNames[1] is the x-mirror bone
	bool bXMirrorBone;
	bool bNormalizeWeights = false;

	TB_Unweight();
	virtual ~TB_Unweight();

	virtual UndoType GetUndoType() { return UndoType::Weight; }
	virtual bool NeedMirrorMergedQuery() { return bMirror || bXMirrorBone; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = -0.015f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_SmoothWeight : public TweakBrush {
public:
	AnimInfo* animInfo;
	// boneNames: first is bone being edited; second is x-mirror bone if
	// bXMirrorBone is true; the rest are normalize bones
	std::vector<std::string> boneNames, lockedBoneNames;
	// bSpreadWeight: if true, leftover weight is spread across normalize bones.
	bool bSpreadWeight;
	// bXMirrorBone:  if true, boneNames[1] is the x-mirror bone
	bool bXMirrorBone;
	bool bNormalizeWeights = false;
	uint8_t method; // 0 for laplacian, 1 for HC-Smooth.
	float hcAlpha;	// Blending constants.
	float hcBeta;

	void lapFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, float>& wv);
	void hclapFilter(
		Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, float>& wv, UndoStateShape& uss, const int boneInd, const std::unordered_map<uint16_t, float>* wPtr);
	// Balanced-pair parabola-fit smoothing filter.  This smoothing filter
	// uses only balanced pairs of neighboring vertices.  It fits a parabola
	// through those balanced pairs to determine the destination.
	void bppfFilter(Mesh* refmesh, const int* points, int nPoints, std::unordered_map<int, float>& wv, UndoStateShape& uss, const int boneInd, const std::unordered_map<uint16_t, float>* wPtr);

	TB_SmoothWeight();
	virtual ~TB_SmoothWeight();

	virtual UndoType GetUndoType() { return UndoType::Weight; }
	virtual bool NeedMirrorMergedQuery() { return bMirror || bXMirrorBone; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.15f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Color : public TweakBrush {
public:
	nifly::Vector3 color;

	TB_Color();
	virtual ~TB_Color();

	virtual UndoType GetUndoType() { return UndoType::Color; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.03f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Uncolor : public TweakBrush {
public:
	TB_Uncolor();
	virtual ~TB_Uncolor();

	virtual UndoType GetUndoType() { return UndoType::Color; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = -0.03f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
};

class TB_Alpha : public TweakBrush, public ClampBrush {
public:
	TB_Alpha();
	virtual ~TB_Alpha();

	virtual UndoType GetUndoType() { return UndoType::Alpha; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = 0.03f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }
};

class TB_Unalpha : public TweakBrush {
public:
	TB_Unalpha();
	virtual ~TB_Unalpha();

	virtual UndoType GetUndoType() { return UndoType::Alpha; }

	virtual void resetSettings() {
		TweakBrush::resetSettings();
		strength = -0.03f;
	}

	virtual void brushAction(Mesh* refmesh, TweakPickInfo& pickInfo, const int* points, int nPoints, UndoStateShape& uss);
	virtual bool checkSpacing(nifly::Vector3&, nifly::Vector3&) { return true; }
};

class TweakStroke {
	std::vector<Mesh*> refMeshes;
	TweakBrush* refBrush;
	bool newStroke = true;
	nifly::Vector3 lastPoint;

	static std::vector<std::future<void>> normalUpdates;
	std::unordered_map<Mesh*, std::unique_ptr<int[]>> pts1;
	std::unordered_map<Mesh*, std::unique_ptr<int[]>> pts2;

	std::unordered_map<Mesh*, std::unordered_set<AABBTree::AABBTreeNode*>> affectedNodes;

public:
	UndoStateProject& usp;

	TweakStroke(const std::vector<Mesh*>& meshes, TweakBrush* theBrush, UndoStateProject& uspi)
		: usp(uspi) {
		refMeshes = meshes;
		refBrush = theBrush;
		usp.undoType = refBrush->GetUndoType();
	}

	void beginStroke(TweakPickInfo& pickInfo);
	void beginStroke(TweakPickInfo& pickInfo, std::vector<std::vector<nifly::Vector3>>& positionData);
	void updateStroke(TweakPickInfo& pickInfo);
	void endStroke();

	TweakBrush::BrushType BrushType() { return refBrush->Type(); }
	std::string BrushName() { return refBrush->Name(); }
	TweakBrush* GetRefBrush() { return refBrush; }
	std::vector<Mesh*> GetRefMeshes() { return refMeshes; }
};
