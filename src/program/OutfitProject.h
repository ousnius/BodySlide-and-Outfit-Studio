/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Automorph.h"
#include "../components/Mesh.h"
#include "../components/Anim.h"
#include "OutfitStudio.h"

#include <wx/arrstr.h>

struct ConformOptions {
	float proximityRadius = 10.0f;
	int maxResults = 10;
	bool axisX = true;
	bool axisY = true;
	bool axisZ = true;
};

class OutfitStudioFrame;
struct UndoStateShape;

struct MergeCheckErrors {
	bool canMerge = false;
	bool shapesSame = false;
	bool partitionsMismatch = false;
	bool segmentsMismatch = false;
	bool tooManyVertices = false;
	bool tooManyTriangles = false;
	bool shaderMismatch = false;
	bool textureMismatch = false;
	bool alphaPropMismatch = false;
};

class OutfitProject {
	OutfitStudioFrame* owner = nullptr;

	NifFile workNif;
	AnimInfo workAnim;
	NiShape* baseShape = nullptr;

	// All cloth data blocks that have been loaded during work
	std::unordered_map<std::string, BSClothExtraData*> clothData;

	void ValidateNIF(NifFile& nif);

public:
	std::string outfitName = "New Outfit";
	DiffDataSets baseDiffData;
	SliderSet activeSet;
	Automorph morpher;
	bool morpherInitialized = false;

	std::map<std::string, std::vector<Vector3>> boneScaleOffsets;
	std::map<std::string, std::vector<Vector3>> boneScaleVerts;

	std::unordered_map<std::string, std::vector<std::string>> shapeTextures;
	std::unordered_map<std::string, MaterialFile> shapeMaterialFiles;

	// inOwner is meant to provide access to OutfitStudio for the purposes of reporting process status only.
	OutfitProject(OutfitStudioFrame* inOwner = nullptr);
	~OutfitProject();

	wxString mFileName;
	wxString mOutfitName;
	wxString mDataDir;
	wxString mBaseFile;
	wxString mGamePath;
	wxString mGameFile;
	bool mCopyRef = true;
	bool mGenWeights = false;
	bool bPose = false;

	// Returns a string error message or empty string on success.
	std::string Save(const wxString& strFileName,
		const wxString& strOutfitName,
		const wxString& strDataDir,
		const wxString& strBaseFile,
		const wxString& strGamePath,
		const wxString& strGameFile,
		bool genWeights,
		bool copyRef);

	bool SaveSliderData(const std::string& fileName, bool copyRef = true);

	NifFile* GetWorkNif() { return &workNif; }
	AnimInfo* GetWorkAnim() { return &workAnim; }
	std::unordered_map<std::string, BSClothExtraData*>& GetClothData() { return clothData; }

	NiShape* GetBaseShape() { return baseShape; }
	void SetBaseShape(NiShape* shape, const bool moveData = true);

	bool IsBaseShape(NiShape* shape) {
		return (shape && shape == baseShape);
	}

	std::string SliderSetName();
	std::string SliderSetFileName();
	std::string OutfitName();

	void ReplaceForbidden(std::string& str, const char& replacer = ' ');

	bool ValidSlider(int index);
	bool ValidSlider(const std::string& sliderName);
	bool AllSlidersZero();
	int SliderCount(void);
	std::string GetSliderName(int index);
	void GetSliderList(std::vector<std::string>& sliderNames);
	void AddEmptySlider(const std::string& newName);
	void AddZapSlider(const std::string& newName, std::unordered_map<ushort, float>& verts, NiShape* shape);
	void AddCombinedSlider(const std::string& newName);

	NiShape* CreateNifShapeFromData(const std::string& shapeName, std::vector<Vector3>& v, std::vector<Triangle>& t, std::vector<Vector2>& uv, std::vector<Vector3>* norms = nullptr);

	// Slider data can have a separate name from the shape target.
	std::string SliderShapeDataName(int index, const std::string& shapeName);
	bool SliderClamp(int index);
	bool SliderZap(int index);
	bool SliderUV(int index);
	wxArrayString SliderZapToggles(int index);
	bool SliderInvert(int index);
	bool SliderHidden(int index);
	int SliderIndexFromName(const std::string& sliderName);

	void SetSliderZap(int index, bool zap);
	void SetSliderZapToggles(int index, const wxArrayString& toggles);
	void SetSliderInvert(int index, bool inv);
	void SetSliderUV(int index, bool uv);
	void SetSliderHidden(int index, bool hidden);
	void SetSliderDefault(int index, int val, bool isHi);
	void SetSliderName(int index, const std::string& newName);

	void NegateSlider(const std::string& sliderName, NiShape* shape);
	void MaskAffected(const std::string& sliderName, NiShape* shape);

	void SetSliderFromBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName);
	bool SetSliderFromOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName);
	bool SetSliderFromFBX(const std::string& sliderName, NiShape* shape, const std::string& fileName);
	void SetSliderFromDiff(const std::string& sliderName, NiShape* shape, std::unordered_map<ushort, Vector3>& diff);
	int SaveSliderBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName);
	int SaveSliderOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName, const bool onlyDiff = false);
	bool WriteMorphTRI(const std::string& triPath);
	bool WriteHeadTRI(NiShape* shape, const std::string& triPath);

	float& SliderValue(int index);
	float& SliderValue(const std::string& name);
	float SliderDefault(int index, bool hi);

	void InitConform();
	void ConformShape(NiShape* shape, const ConformOptions& options = ConformOptions());

	const std::string& ShapeToTarget(const std::string& shapeName);
	int GetVertexCount(NiShape* shape);
	void GetLiveVerts(NiShape* shape, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs = nullptr);
	void GetSliderDiff(NiShape* shape, const std::string& sliderName, std::vector<Vector3>& outVerts);
	void GetSliderDiffUV(NiShape* shape, const std::string& sliderName, std::vector<Vector2>& outUVs);
	void GetActiveBones(std::vector<std::string>& outBoneNames);

	std::vector<std::string> GetShapeTextures(NiShape* shape);
	bool GetShapeMaterialFile(NiShape* shape, MaterialFile& outMatFile);

	void SetTextures();
	void SetTextures(const std::vector<std::string>& textureFiles);
	void SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles = std::vector<std::string>());

	bool IsValidShape(const std::string& shapeName);

	bool& SliderShow(int index);
	bool& SliderShow(const std::string& sliderName);

	void RefreshMorphShape(NiShape* shape);
	void UpdateShapeFromMesh(NiShape* shape, const mesh* m);
	void UpdateMorphResult(NiShape* shape, const std::string& sliderName, std::unordered_map<ushort, Vector3>& vertUpdates);
	void ScaleMorphResult(NiShape* shape, const std::string& sliderName, float scaleValue);
	void MoveVertex(NiShape* shape, const Vector3& pos, const int& id);
	void OffsetShape(NiShape* shape, const Vector3& xlate, std::unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(NiShape* shape, const Vector3& scale, std::unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(NiShape* shape, const Vector3& angle, std::unordered_map<ushort, float>* mask = nullptr);
	void ApplyTransformToShapeGeometry(NiShape* shape, const MatTransform& t);

	// Uses the AutoMorph class to generate proximity values for bone weights.
	// This is done by creating several virtual sliders that contain weight offsets for each vertex per bone.
	// These data sets are then temporarily linked to the AutoMorph class and result 'diffs' are generated.
	// The resulting data is then written back to the outfit shape as the green color channel.
	void CopyBoneWeights(NiShape* shape, const float proximityRadius, const int maxResults, std::unordered_map<ushort, float>& mask, const std::vector<std::string>& boneList, int nCopyBones, const std::vector<std::string> &lockedBones, UndoStateShape &uss, bool bSpreadWeight);
	// Transfers the weights of the selected bones from reference to chosen shape 1:1. Requires same vertex count and order.
	void TransferSelectedWeights(NiShape* shape, std::unordered_map<ushort, float>* mask = nullptr, std::vector<std::string>* inBoneList = nullptr);
	bool HasUnweighted(std::vector<std::string>* shapeNames = nullptr);

	void InvalidateBoneScaleCache();
	void ApplyBoneScale(const std::string& bone, int sliderPos, bool clear = false);
	void ClearBoneScale(bool clear = true);

	void AddBoneRef(const std::string& boneName);
	void AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const MatTransform &xformToParent);
	void ModifyCustomBone(AnimBone *bPtr, const std::string& parentBone, const MatTransform &xformToParent);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(NiShape* shape, const std::string& sliderName);
	void ClearUnmaskedDiff(NiShape* shape, const std::string& sliderName, std::unordered_map<ushort, float>* mask);
	void DeleteSlider(const std::string& sliderName);

	int LoadSkeletonReference(const std::string& skeletonFileName);
	int LoadReferenceTemplate(const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll = false, bool mergeSliders = false);
	int LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders = false);
	int LoadReference(const std::string& fileName, const std::string& setName, bool mergeSliders = false, const std::string& shapeName = "");

	int LoadFromSliderSet(const std::string& fileName, const std::string& setName, std::vector<std::string>* origShapeOrder = nullptr);
	int AddFromSliderSet(const std::string& fileName, const std::string& setName, const bool newDataLocal = true);

	void CollectVertexData(NiShape *shape, UndoStateShape &uss, const std::vector<int> &indices);
	void CollectTriangleData(NiShape *shape, UndoStateShape &uss, const std::vector<int> &indices);
	bool PrepareDeleteVerts(NiShape* shape, const std::unordered_map<ushort, float>& mask, UndoStateShape &uss);
	void ApplyShapeMeshUndo(NiShape* shape, const UndoStateShape &uss, bool bUndo);

	bool PrepareCollapseVertex(NiShape* shape, UndoStateShape &uss, const std::vector<int> &indices);
	bool PrepareFlipEdge(NiShape* shape, UndoStateShape &uss, const Edge &edge);
	bool PrepareSplitEdge(NiShape* shape, UndoStateShape &uss, const std::vector<int> &p1s, const std::vector<int> &p2s);

	void CheckMerge(const std::string &sourceName, const std::string &targetName, MergeCheckErrors &e);
	void PrepareCopyGeo(NiShape *source, NiShape *target, UndoStateShape &uss);

	NiShape* DuplicateShape(NiShape* sourceShape, const std::string& destShapeName);
	void DeleteShape(NiShape* shape);

	void DeleteBone(const std::string& boneName) {
		if (workNif.IsValid()) {
			for (auto &s : workNif.GetShapeNames())
				workAnim.RemoveShapeBone(s, boneName);

			int blockID = workNif.GetBlockID(workNif.FindBlockByName<NiNode>(boneName));
			if (blockID >= 0)
				workNif.GetHeader().DeleteBlock(blockID);
		}
	}

	void RenameShape(NiShape* shape, const std::string& newShapeName);
	void UpdateNifNormals(NifFile* nif, const std::vector<mesh*>& shapemeshes);

	void ChooseClothData(NifFile& nif);
	void ResetTransforms();
	void RemoveSkinning();

	int ImportNIF(const std::string& fileName, bool clear = true, const std::string& inOutfitName = "", std::map<std::string, std::string>* renamedShapes = nullptr);
	int ExportNIF(const std::string& fileName, const std::vector<mesh*>& modMeshes, bool withRef = false);
	int ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes);

	int ImportOBJ(const std::string& fileName, const std::string& shapeName = "", NiShape* mergeShape = nullptr);
	int ExportOBJ(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal, const Vector3& scale = Vector3(1.0f, 1.0f, 1.0f), const Vector3& offset = Vector3());

	int ImportFBX(const std::string& fileName, const std::string& shapeName = "", NiShape* mergeShape = nullptr);
	int ExportFBX(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal);
};
