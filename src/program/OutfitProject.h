/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Anim.h"
#include "../components/Automorph.h"
#include "../components/Mesh.h"
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

// SymmetricVertices: result from the function MatchSymmetricVertices
struct SymmetricVertices {
	// matches has all matched pairs of vertices, starting with the two-pairs,
	// then the one-pairs, and finally the self matches.  Two-pair matches
	// have the lower index first.  One-pair matches have the unmasked vertex
	// first, the masked vertex second.
	std::vector<std::pair<int, int>> matches;
	std::vector<int> unmatched;
	// weldSkip lists all vertices that were skipped (and are therefore not
	// in matches or unmatched) because they're welded to a vertex with a
	// lower index.
	std::vector<int> weldSkip;
	// Two-pair matches: matches where both vertices were not masked.
	int two_pair_count = 0;
	// One-pair matches: matches where one vertex was not masked and the
	// other vertex was masked.
	int one_pair_count = 0;
	// Self matches: matches where the vertex's mirror was itself.
	int self_count = 0;
};

// VertexAsymmetries: result from the function FindVertexAsymmetries
struct VertexAsymmetries {
	// Indexing of each vector<bool> is the same as SymmetricVertices::matches.
	std::vector<bool> positions;
	int posCount = 0;
	float posAvg = 0.0f;
	struct Slider {
		std::string sliderName;
		std::vector<bool> aflags;
		int count = 0;
		float avg = 0.0f;
	};
	std::vector<Slider> sliders;
	std::vector<bool> anyslider;
	int anyslidercount = 0;
	struct Bone {
		std::string boneName;
		std::vector<bool> aflags;
		int two_pair_count = 0, one_pair_count = 0, self_count = 0;
		int mirroroffset = 0;	// -1, 0, or 1
		float dual_avg = 0.0f, one_pair_avg = 0.0f, all_avg = 0.0f;
	};
	std::vector<Bone> bones;
	std::vector<bool> anybone;
	int anybonecount = 0;
};

struct VertexAsymmetryTasks {
	bool doUnmatched = true;
	bool doPos = true;
	std::vector<bool> doSliders;	// Same size as VertexAsymmetries::sliders
	// doDualBones: do two_pair and sel; doSingleBones: do one_pair
	std::vector<bool> doDualBones, doSingleBones;	// Same size as VertexAsymmetries::bones
};

std::vector<bool> CalcCombinedVertexAsymmetryTasks(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks);

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

	bool IsBaseShape(nifly::NiShape* shape) { return (shape && shape == baseShape); }

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
	const std::string& TargetToShape(const std::string& targetName);
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
	void UpdateShapeFromMesh(nifly::NiShape* shape, const Mesh* m);
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
	void CopyBoneWeights(nifly::NiShape* shape,
						 const float proximityRadius,
						 const int maxResults,
						 std::unordered_map<uint16_t, float>& mask,
						 const std::vector<std::string>& boneList,
						 int nCopyBones,
						 const std::vector<std::string>& lockedBones,
						 UndoStateShape& uss,
						 bool bSpreadWeight);
	// Transfers the weights of the selected bones from reference to chosen shape 1:1. Requires same vertex count and order.
	void TransferSelectedWeights(nifly::NiShape* shape, std::unordered_map<uint16_t, float>* mask = nullptr, std::vector<std::string>* inBoneList = nullptr);
	bool HasUnweighted(std::vector<std::string>* shapeNames = nullptr);

	void InvalidateBoneScaleCache();
	void ApplyBoneScale(const std::string& bone, int sliderPos, bool clear = false);
	void ClearBoneScale(bool clear = true);

	void AddBoneRef(const std::string& boneName);
	void AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const nifly::MatTransform& xformToParent);
	void ModifyCustomBone(AnimBone* bPtr, const std::string& parentBone, const nifly::MatTransform& xformToParent);

	nifly::MatTransform GetTransformShapeToGlobal(nifly::NiShape* shape);
	// CopySegPart returns the number of failed triangles, or 0 for success.
	int CopySegPart(nifly::NiShape* shape);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(nifly::NiShape* shape, const std::string& sliderName);
	void ClearUnmaskedDiff(nifly::NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, float>* mask);
	void DeleteSlider(const std::string& sliderName);

	int LoadSkeletonReference(const std::string& skeletonFileName);
	int LoadReferenceTemplate(
		const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll = false, bool mergeSliders = false, bool mergeZaps = false);
	int LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders = false, bool mergeZaps = false);
	int LoadReference(const std::string& fileName, const std::string& setName, const std::string& shapeName = "", bool mergeSliders = false, bool mergeZaps = false);

	int LoadFromSliderSet(const std::string& fileName, const std::string& setName, std::vector<std::string>* origShapeOrder = nullptr);
	int AddFromSliderSet(const std::string& fileName, const std::string& setName, const bool newDataLocal = true);

	std::unordered_map<uint16_t, nifly::Vector3>* GetDiffSet(SliderData& silderData, nifly::NiShape* shape);

	void CollectVertexData(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices);
	void CollectTriangleData(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint32_t>& indices);
	bool PrepareDeleteVerts(nifly::NiShape* shape, const std::unordered_map<uint16_t, float>& mask, UndoStateShape& uss);
	void ApplyShapeMeshUndo(nifly::NiShape* shape, std::vector<float>& mask, const UndoStateShape& uss, bool bUndo);

	bool PrepareCollapseVertex(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices);
	bool PrepareFlipEdge(nifly::NiShape* shape, UndoStateShape& uss, const nifly::Edge& edge);
	bool PrepareRefineMesh(nifly::NiShape* shape, UndoStateShape& uss, std::vector<bool>& pincs, const std::unordered_map<int, std::vector<int>>& weldVerts);

	bool IsVertexOnBoundary(nifly::NiShape* shape, int vi);
	bool PointsHaveDifferingWeightsOrDiffs(nifly::NiShape* shape1, int p1, nifly::NiShape* shape2, int p2);
	void PrepareMergeVertex(nifly::NiShape* shape, UndoStateShape& uss, int selVert, int targVert);
	void PrepareWeldVertex(nifly::NiShape* shape, UndoStateShape& uss, int selVert, int targVert, nifly::NiShape* targShape);

	void CheckMerge(const std::string& sourceName, const std::string& targetName, MergeCheckErrors& e);
	void PrepareCopyGeo(nifly::NiShape* source, nifly::NiShape* target, UndoStateShape& uss);

	nifly::NiShape* DuplicateShape(nifly::NiShape* sourceShape, const std::string& destShapeName);
	void DeleteShape(nifly::NiShape* shape);

	void DeleteBone(const std::string& boneName) {
		if (workNif.IsValid()) {
			for (auto& s : workNif.GetShapeNames())
				workAnim.RemoveShapeBone(s, boneName);

			int blockID = workNif.GetBlockID(workNif.FindBlockByName<nifly::NiNode>(boneName));
			if (blockID >= 0)
				workNif.GetHeader().DeleteBlock(blockID);
		}
	}

	void RenameShape(nifly::NiShape* shape, const std::string& newShapeName);
	void UpdateNifNormals(nifly::NifFile* nif, const std::vector<Mesh*>& shapemeshes);

	void MatchSymmetricVertices(nifly::NiShape* shape, const float* mask, const std::unordered_map<int, std::vector<int>>& weldVerts, SymmetricVertices& r);
	void MatchSymmetricBoneNames(std::vector<std::pair<std::string, std::string>>& pairs, std::vector<std::string>& singles);
	void FindVertexAsymmetries(nifly::NiShape* shape, const SymmetricVertices& symverts, const std::unordered_map<int, std::vector<int>>& weldVerts, VertexAsymmetries& r);
	void PrepareSymmetrizeVertices(nifly::NiShape* shape, UndoStateShape& uss, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, const std::unordered_map<int, std::vector<int>>& weldVerts, const std::vector<std::string>& userNormBones, const std::vector<std::string>& userNotNormBones);
	std::vector<bool> CalculateAsymmetricTriangleVertexMask(nifly::NiShape* shape, const std::unordered_map<int, std::vector<int>>& weldVerts);

	void ChooseClothData(nifly::NifFile& nif);
	void ResetTransforms();
	void RemoveSkinning();

	int ImportNIF(const std::string& fileName, bool clear = true, const std::string& inOutfitName = "", std::map<std::string, std::string>* renamedShapes = nullptr);
	int ExportNIF(const std::string& fileName, const std::vector<Mesh*>& modMeshes, bool withRef = false);
	int ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes);

	int ImportOBJ(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportOBJ(const std::string& fileName,
				  const std::vector<nifly::NiShape*>& shapes,
				  bool transToGlobal,
				  const nifly::Vector3& scale = nifly::Vector3(1.0f, 1.0f, 1.0f),
				  const nifly::Vector3& offset = nifly::Vector3());

	int ImportFBX(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportFBX(const std::string& fileName, const std::vector<nifly::NiShape*>& shapes, bool transToGlobal);
};
