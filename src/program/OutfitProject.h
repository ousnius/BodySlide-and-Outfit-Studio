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
#include <wx/filename.h>

struct ConformOptions {
	float proximityRadius = 10.0f;
	int maxResults = 10;
	bool noSqueeze = false;
	bool solidMode = false;
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

	nifly::NifFile workNif;
	AnimInfo workAnim;
	nifly::NiShape* baseShape = nullptr;

	// All cloth data blocks that have been loaded during work
	std::unordered_map<std::string, std::unique_ptr<nifly::BSClothExtraData>> clothData;

	void ValidateNIF(nifly::NifFile& nif);

public:
	std::string outfitName = "New Outfit";
	DiffDataSets baseDiffData;
	SliderSet activeSet;
	Automorph morpher;
	bool morpherInitialized = false;

	std::map<std::string, std::vector<nifly::Vector3>> boneScaleOffsets;
	std::map<std::string, std::vector<nifly::Vector3>> boneScaleVerts;

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
	std::string Save(const wxFileName& sliderSetFile,
		const wxString& strOutfitName,
		const wxString& strDataDir,
		const wxString& strBaseFile,
		const wxString& strGamePath,
		const wxString& strGameFile,
		bool genWeights,
		bool copyRef);

	bool SaveSliderData(const std::string& fileName, bool copyRef = true);

	nifly::NifFile* GetWorkNif() { return &workNif; }
	AnimInfo* GetWorkAnim() { return &workAnim; }
	std::unordered_map<std::string, std::unique_ptr<nifly::BSClothExtraData>>& GetClothData() { return clothData; }

	nifly::NiShape* GetBaseShape() { return baseShape; }
	void SetBaseShape(nifly::NiShape* shape, const bool moveData = true);

	bool IsBaseShape(nifly::NiShape* shape) {
		return (shape && shape == baseShape);
	}

	std::string SliderSetName();
	std::string SliderSetFileName();
	std::string OutfitName();

	void ReplaceForbidden(std::string& str, const char& replacer = ' ');

	bool ValidSlider(const size_t index);
	bool ValidSlider(const std::string& sliderName);
	bool AllSlidersZero();
	size_t SliderCount();
	std::string GetSliderName(const size_t index);
	void GetSliderList(std::vector<std::string>& sliderNames);
	void AddEmptySlider(const std::string& newName);
	void AddZapSlider(const std::string& newName, std::unordered_map<uint16_t, float>& verts, nifly::NiShape* shape);
	void AddCombinedSlider(const std::string& newName);

	nifly::NiShape* CreateNifShapeFromData(const std::string& shapeName,
										   const std::vector<nifly::Vector3>* v,
										   const std::vector<nifly::Triangle>* t = nullptr,
										   const std::vector<nifly::Vector2>* uv = nullptr,
										   const std::vector<nifly::Vector3>* norms = nullptr);

	// Slider data can have a separate name from the shape target.
	std::string SliderShapeDataName(const size_t index, const std::string& shapeName);
	bool SliderClamp(const size_t index);
	bool SliderZap(const size_t index);
	bool SliderUV(const size_t index);
	wxArrayString SliderZapToggles(const size_t index);
	bool SliderInvert(const size_t index);
	bool SliderHidden(const size_t index);
	bool SliderIndexFromName(const std::string& sliderName, size_t& index);

	void SetSliderZap(const size_t index, const bool zap);
	void SetSliderZapToggles(const size_t index, const wxArrayString& toggles);
	void SetSliderInvert(const size_t index, const bool inv);
	void SetSliderUV(const size_t index, const bool uv);
	void SetSliderHidden(const size_t index, const bool hidden);
	void SetSliderDefault(const size_t index, const int val, const bool isHi);
	void SetSliderName(const size_t index, const std::string& newName);

	void NegateSlider(const std::string& sliderName, nifly::NiShape* shape);
	void MaskAffected(const std::string& sliderName, nifly::NiShape* shape);

	bool SetSliderFromNIF(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	void SetSliderFromBSD(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	bool SetSliderFromOBJ(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	bool SetSliderFromFBX(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	void SetSliderFromDiff(const std::string& sliderName, nifly::NiShape* shape, std::unordered_map<uint16_t, nifly::Vector3>& diff);
	int SaveSliderNIF(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	int SaveSliderBSD(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	int SaveSliderOBJ(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName, const bool onlyDiff = false);
	bool WriteMorphTRI(const std::string& triPath);
	bool WriteHeadTRI(nifly::NiShape* shape, const std::string& triPath);

	float& SliderValue(const size_t index);
	float& SliderValue(const std::string& name);
	float SliderDefault(const size_t index, const bool hi);

	void InitConform();
	void ConformShape(nifly::NiShape* shape, const ConformOptions& options = ConformOptions());

	const std::string& ShapeToTarget(const std::string& shapeName);
	int GetVertexCount(nifly::NiShape* shape);
	void GetLiveVerts(nifly::NiShape* shape, std::vector<nifly::Vector3>& outVerts, std::vector<nifly::Vector2>* outUVs = nullptr);
	void GetSliderDiff(nifly::NiShape* shape, const std::string& sliderName, std::vector<nifly::Vector3>& outVerts);
	void GetSliderDiffUV(nifly::NiShape* shape, const std::string& sliderName, std::vector<nifly::Vector2>& outUVs);
	size_t GetActiveBoneCount();
	void GetActiveBones(std::vector<std::string>& outBoneNames);

	std::vector<std::string> GetShapeTextures(nifly::NiShape* shape);
	bool GetShapeMaterialFile(nifly::NiShape* shape, MaterialFile& outMatFile);

	void SetTextures();
	void SetTextures(const std::vector<std::string>& textureFiles);
	void SetTextures(nifly::NiShape* shape, const std::vector<std::string>& textureFiles = std::vector<std::string>());

	bool IsValidShape(const std::string& shapeName);

	bool& SliderShow(const size_t index);
	bool& SliderShow(const std::string& sliderName);

	void RefreshMorphShape(nifly::NiShape* shape);
	void UpdateShapeFromMesh(nifly::NiShape* shape, const mesh* m);
	void UpdateMorphResult(nifly::NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, nifly::Vector3>& vertUpdates);
	void ScaleMorphResult(nifly::NiShape* shape, const std::string& sliderName, float scaleValue);
	void MoveVertex(nifly::NiShape* shape, const nifly::Vector3& pos, const int& id);
	void OffsetShape(nifly::NiShape* shape, const nifly::Vector3& xlate, std::unordered_map<uint16_t, float>* mask = nullptr);
	void ScaleShape(nifly::NiShape* shape, const nifly::Vector3& scale, std::unordered_map<uint16_t, float>* mask = nullptr);
	void RotateShape(nifly::NiShape* shape, const nifly::Vector3& angle, std::unordered_map<uint16_t, float>* mask = nullptr);
	void ApplyTransformToShapeGeometry(nifly::NiShape* shape, const nifly::MatTransform& t);

	// Uses the AutoMorph class to generate proximity values for bone weights.
	// This is done by creating several virtual sliders that contain weight offsets for each vertex per bone.
	// These data sets are then temporarily linked to the AutoMorph class and result 'diffs' are generated.
	// The resulting data is then written back to the outfit shape as the green color channel.
	void CopyBoneWeights(nifly::NiShape* shape, const float proximityRadius, const int maxResults, std::unordered_map<uint16_t, float>& mask, const std::vector<std::string>& boneList, int nCopyBones, const std::vector<std::string> &lockedBones, UndoStateShape &uss, bool bSpreadWeight, int minProgress = 1, int maxProgress = 90);
	// Transfers the weights of the selected bones from reference to chosen shape 1:1. Requires same vertex count and order.
	void TransferSelectedWeights(nifly::NiShape* shape, std::unordered_map<uint16_t, float>* mask = nullptr, std::vector<std::string>* inBoneList = nullptr);
	bool HasUnweighted(std::vector<std::string>* shapeNames = nullptr);

	void InvalidateBoneScaleCache();
	void ApplyBoneScale(const std::string& bone, int sliderPos, bool clear = false);
	void ClearBoneScale(bool clear = true);

	void AddBoneRef(const std::string& boneName);
	void AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const nifly::MatTransform &xformToParent);
	void ModifyCustomBone(AnimBone *bPtr, const std::string& parentBone, const nifly::MatTransform &xformToParent);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(nifly::NiShape* shape, const std::string& sliderName);
	void ClearUnmaskedDiff(nifly::NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, float>* mask);
	void DeleteSlider(const std::string& sliderName);

	int LoadSkeletonReference(const std::string& skeletonFileName);
	int LoadReferenceTemplate(const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll = false, bool mergeSliders = false, bool keepZaps = false, bool silent = false);
	int LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders = false, bool keepZaps = false);
	int LoadReference(const std::string& fileName, const std::string& setName, bool mergeSliders = false, const std::string& shapeName = "", bool keepZaps = false);

	int LoadFromSliderSet(const std::string& fileName, const std::string& setName, std::vector<std::string>* origShapeOrder = nullptr);
	int AddFromSliderSet(const std::string& fileName, const std::string& setName, const bool newDataLocal = true, bool silent = false);

	void CollectVertexData(nifly::NiShape *shape, UndoStateShape &uss, const std::vector<uint16_t> &indices);
	void CollectTriangleData(nifly::NiShape *shape, UndoStateShape &uss, const std::vector<uint32_t> &indices);
	bool PrepareDeleteVerts(nifly::NiShape* shape, const std::unordered_map<uint16_t, float>& mask, UndoStateShape &uss);
	void ApplyShapeMeshUndo(nifly::NiShape* shape, const UndoStateShape &uss, bool bUndo);

	bool PrepareCollapseVertex(nifly::NiShape* shape, UndoStateShape &uss, const std::vector<uint16_t> &indices);
	bool PrepareFlipEdge(nifly::NiShape* shape, UndoStateShape &uss, const nifly::Edge &edge);
	bool PrepareSplitEdge(nifly::NiShape* shape, UndoStateShape &uss, const std::vector<uint16_t> &p1s, const std::vector<uint16_t> &p2s);

	void CheckMerge(const std::string &sourceName, const std::string &targetName, MergeCheckErrors &e);
	void PrepareCopyGeo(nifly::NiShape *source, nifly::NiShape *target, UndoStateShape &uss);

	nifly::NiShape* DuplicateShape(nifly::NiShape* sourceShape, const std::string& destShapeName);
	void DeleteShape(nifly::NiShape* shape);

	void DeleteBone(const std::string& boneName) {
		if (workNif.IsValid()) {
			for (auto &s : workNif.GetShapeNames())
				workAnim.RemoveShapeBone(s, boneName);

			int blockID = workNif.GetBlockID(workNif.FindBlockByName<nifly::NiNode>(boneName));
			if (blockID >= 0)
				workNif.GetHeader().DeleteBlock(blockID);
		}
	}

	void RenameShape(nifly::NiShape* shape, const std::string& newShapeName);
	void UpdateNifNormals(nifly::NifFile* nif, const std::vector<mesh*>& shapemeshes);

	void ChooseClothData(nifly::NifFile& nif);
	void ResetTransforms();
	void RemoveSkinning();

	int ImportNIF(const std::string& fileName, bool clear = true, const std::string& inOutfitName = "", std::map<std::string, std::string>* renamedShapes = nullptr);
	int ExportNIF(const std::string& fileName, const std::vector<mesh*>& modMeshes, bool withRef = false);
	int ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes);

	int ImportOBJ(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportOBJ(const std::string& fileName, const std::vector<nifly::NiShape*>& shapes, bool transToGlobal, const nifly::Vector3& scale = nifly::Vector3(1.0f, 1.0f, 1.0f), const nifly::Vector3& offset = nifly::Vector3());

	int ImportFBX(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportFBX(const std::string& fileName, const std::vector<nifly::NiShape*>& shapes, bool transToGlobal);
};
