/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Anim.h"
#include "../components/Automorph.h"
#include "../components/Mesh.h"
#include "OutfitStudio.h"

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <wx/arrstr.h>
#include <wx/filename.h>

struct ConformOptions {
	float proximityRadius = 10.0f;
	int maxResults = 10;
	bool smoothResultDeltas = false;
	int smoothIterations = 2;
	float smoothStrength = 0.5f;
	bool noSqueeze = false;
	bool solidMode = false;
	bool axisX = true;
	bool axisY = true;
	bool axisZ = true;
	bool fixClipping = false;
	float fixClippingStrength = 0.5f;
	std::vector<std::string> sliderNames; // If empty, conform all non-zap/non-UV sliders
};

struct SliderDataKey {
	std::string sliderName;
	std::string targetName;
	std::string dataName;
};

struct SliderDataLocation {
	SliderDataKey key;
	std::string shapeName;
	std::string fileName;
	std::string dataFileName;
	std::string dataNameInFile;
	bool local = false;
	bool resolved = false;
	bool isBSD = false;
	std::string resolvedPath;
	std::string candidatePath;
	std::vector<std::string> dataFolders;
};

class OutfitStudioFrame;
class SFMaterialDatabase;
struct UndoStateShape;
struct UndoStateShapeDelete;

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
	// matches has all matched pairs of vertices, including self-matches,
	// in no particular order.  The first vertex index is always less than
	// or equal to the second.  Only one vertex in each weld set is listed:
	// the one with the lowest index.
	std::vector<std::pair<int, int>> matches;
	std::vector<int> unmatched;
};

// VertexAsymmetries: result from the function FindVertexAsymmetries
struct VertexAsymmetries {
	// Indexing of each vector<bool> and vector<float> is the same as
	// SymmetricVertices::matches.
	std::vector<bool> positions;
	std::vector<float> poserr;
	struct Slider {
		std::string sliderName;
		std::vector<bool> aflags;
		std::vector<float> differr;
	};
	std::vector<Slider> sliders;
	std::vector<bool> anyslider;
	struct Bone {
		std::string boneName;
		// We call this bone "bone 1" and its mirror "bone 2".
		// mirroroffset gives the offset in bones of bone 2.
		int mirroroffset = 0;	// -1, 0, or 1
		// aflags[i] will be true if p1's bone 1 weight doesn't match p2's
		// bone 2 weight.  That means not all asymmetries for bone 1 will
		// be listed here: if bone 1 != bone 2 and (p1,p2) is a dual
		// match, bone 2's aflags will indicate whether p2's bone 1 weight
		// doesn't match p1's bone 2 weight, which is also an asymmetry for
		// bone 1.
		std::vector<bool> aflags;
		std::vector<float> weighterr;
	};
	std::vector<Bone> bones;
	std::vector<bool> anybone;
};

struct VertexAsymmetryStats {
	int unmaskedCount = 0;
	int posCount = 0;
	float posAvg = 0.0f;
	int anySliderCount = 0;
	int anyBoneCount = 0;
	std::vector<int> sliderCounts;
	std::vector<float> sliderAvgs;
	std::vector<int> boneCounts;
	std::vector<float> boneAvgs;
};

struct VertexAsymmetryTasks {
	bool doUnmatched = false;
	bool doPos = false;
	std::vector<bool> doSliders;	// Same size as VertexAsymmetries::sliders
	std::vector<bool> doBones;	// Same size as VertexAsymmetries::bones
};

void CalcVertexAsymmetryStats(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const std::vector<bool>& selVerts, VertexAsymmetryStats& stats);
std::vector<bool> CalcVertexListForAsymmetryTasks(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, int nVerts);
void AddWeldedToVertexList(const Mesh::WeldVertsType& welcVerts, std::vector<bool>& selVerts);

class OutfitProject {
	OutfitStudioFrame* owner = nullptr;

	void UpdateProgress(int val, const wxString& msg = "");

	nifly::NifFile workNif;
	AnimInfo workAnim;
	nifly::NiShape* baseShape = nullptr;

	// All cloth data blocks that have been loaded during work
	std::unordered_map<std::string, std::unique_ptr<nifly::BSClothExtraData>> clothData;

	// All HDT-SMP physics links that sat on the root node of a loaded NIF. The
	// game reads only one of them, so a single one is picked for the output.
	std::vector<std::unique_ptr<nifly::NiStringExtraData>> rootPhysicsData;

	std::unique_ptr<std::istream> GetExternalGeometryStream(const std::string& dir, const std::string& path, const std::string& nifFilePath = std::string()) const;
	bool GetSFMaterialJSON(const std::string& matPath, std::string& jsonOutput);
	void ValidateNIF(nifly::NifFile& nif, const std::string& nifFilePath = std::string());

	// Records the HDT-SMP links on the root node of a NIF that is about to be
	// merged into the work NIF, for the choice made on save
	void CaptureRootPhysicsData(nifly::NifFile& srcNif);

	// One block MergeRootExtraData could bring over: the block in the loaded NIF
	// and, when the project already has one of the same type and name, the block
	// on the work root that merging it would replace.
	struct RootExtraDataCandidate {
		nifly::NiExtraData* source = nullptr;
		nifly::NiExtraData* replaces = nullptr;
	};

	// Copies the extra data of a NIF's root node onto the work NIF's root node.
	// CloneShape only brings over the shape and its bones, so without this every
	// file merged after the first loses whatever sat on its root.
	void MergeRootExtraData(nifly::NifFile& srcNif);

	// Lets the user narrow the blocks to merge down. Returns false on cancel.
	bool ChooseRootExtraData(std::vector<RootExtraDataCandidate>& merging);

	std::string SliderDataTargetForShape(nifly::NiShape* shape);
	std::string ShapeTargetOrDefault(const std::string& shapeName);
	bool TargetNameInUse(const std::string& targetName, const std::string& exceptShapeName);
	std::string UniqueTargetNameForShape(const std::string& shapeName, const std::set<std::string>& reservedTargets);
	void RetargetShapeData(const std::string& shapeName, const std::string& newTarget);
	void ResolveTargetConflictsForIncomingShapes(const std::vector<std::pair<std::string, std::string>>& incomingShapeTargets);
	bool ResolveSliderDataEntry(const SliderDataKey& key, size_t& sliderIndex, size_t& dataIndex);
	bool ShapeSliderDataIsLocalOnly(const std::string& shapeName);

	std::vector<std::unique_ptr<SFMaterialDatabase>> sfMaterialDbs;
	std::vector<std::string> sfMaterialDbContents;
	std::vector<std::unique_ptr<std::istringstream>> sfMaterialDbStreams;
	bool sfMaterialDbsLoaded = false;

	// Applies the inverse of the blended pose transform to a NIF-space diff
	// vector for a single vertex, converting it from posed space to rest space.
	nifly::Vector3 InversePoseDiff(int vertIndex, const nifly::Vector3& diffNif, AnimSkin& animSkin, const nifly::MatTransform& globalToSkin);
	struct ClippingCorrectionCache {
		std::vector<nifly::Triangle> bodyTris;
		std::vector<nifly::Triangle> outfitTris;
		std::vector<nifly::Vector3> neutralBodyVerts;
		std::vector<nifly::Vector3> neutralOutfitVerts;
		std::vector<nifly::Vector3> fixedNeutralVerts;
		bool valid = false;
	};
	bool BuildClippingCorrectionCache(nifly::NiShape* shape, float strength, ClippingCorrectionCache& cache);
	void CalcSliderClippingCorrection(nifly::NiShape* shape,
									  const std::string& sliderName,
									  float strength,
									  TargetDataDiffs& outMorphDiffs,
									  const std::unordered_set<uint16_t>* allowedVerts,
									  const ClippingCorrectionCache* cache);
	void ApplyClippingFixToConformedSlider(nifly::NiShape* shape, const std::string& sliderName, float strength);

public:
	std::string outfitName = "New Outfit";
	DiffDataSets baseDiffData;
	SliderSet activeSet;
	Automorph morpher;
	bool morpherInitialized = false;

	std::unordered_map<std::string, std::vector<std::string>> shapeTextures;
	std::unordered_map<std::string, MaterialFile> shapeMaterialFiles;

	// Physics XML files ("HDT Skinned Mesh Physics Object" extra data) that
	// were linked to each shape by the NIF it came from. Only the first NIF
	// loaded keeps its node hierarchy in the work NIF; every later file
	// contributes shapes alone, so the link - which the game stores on the
	// root node - has to be captured before the merge or it is lost.
	std::unordered_map<std::string, std::vector<std::string>> shapePhysicsFiles;

	// Records the physics XML files linked to the given shapes of a NIF that
	// is about to be merged into the work NIF, keyed by shape name.
	void CapturePhysicsFiles(nifly::NifFile& nif, const std::vector<nifly::NiShape*>& shapes);

	// Set while an unattended script drives the project. Prompts that would
	// otherwise stall the run fall back to their default answer instead.
	bool suppressPrompts = false;

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
	bool bPreventMorphFile = false;
	bool bKeepZappedShapes = false;
	wxString mSFMorphPath;
	wxString mSFMorphTargetShape;
	bool bPose = false;

	// Physics preview: replacement pose-to-global transforms for bones driven
	// by the simulation, or nullptr while physics is off. Owned by the
	// physics controller; consumed by GetLiveVerts' skinning.
	const AnimPoseOverrideMap* physicsPose = nullptr;

	// Reference source info (remembered when reference is loaded from an OSP)
	std::string mRefProjectFile;    // OSP file path relative to project dir
	std::string mRefProjectName;    // Slider set name in the OSP file
	std::string mRefShapeName;      // Shape name in the project

	// Returns a string error message or empty string on success.
	std::string Save(const wxFileName& sliderSetFile,
					 const wxString& strOutfitName,
					 const wxString& strDataDir,
					 const wxString& strBaseFile,
					 const wxString& strGamePath,
					 const wxString& strGameFile,
					 bool genWeights,
					 bool copyRef,
					 bool preventMorphFile,
					 bool keepZappedShapes,
					 const wxString& strSFMorphPath = "",
					 const wxString& strSFMorphTargetShape = "");

	bool SaveSliderData(const std::string& fileName, bool copyRef = true);

	nifly::NifFile* GetWorkNif() { return &workNif; }
	AnimInfo* GetWorkAnim() { return &workAnim; }

	// Resolves a physics XML path referenced by a "HDT Skinned Mesh Physics
	// Object" extra data to a readable stream (loose game data folder file,
	// relative to the project's input NIF, or from loaded archives).
	std::unique_ptr<std::istream> GetPhysicsXmlStream(const std::string& xmlPath);
	std::unordered_map<std::string, std::unique_ptr<nifly::BSClothExtraData>>& GetClothData() { return clothData; }

	nifly::NiShape* GetBaseShape() { return baseShape; }
	void SetBaseShape(nifly::NiShape* shape, const bool moveData = true);
	const std::string& GetReferenceProjectFile() const { return mRefProjectFile; }
	const std::string& GetReferenceProjectName() const { return mRefProjectName; }

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
	void GetSliderDataLocations(std::vector<SliderDataLocation>& outLocations, const std::string& sliderName = "");
	bool SliderDataIsExternal(const std::string& sliderName, nifly::NiShape* shape);
	std::string EnsureSliderDataLocal(const std::string& sliderName, nifly::NiShape* shape);
	bool SetSliderDataLocal(const SliderDataKey& key, std::string* errorMessage = nullptr);
	bool SetSliderDataLocal(const size_t sliderIndex, const size_t dataIndex, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const size_t sliderIndex, const size_t dataIndex, const std::vector<std::string>& dataFolders, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const size_t sliderIndex, const size_t dataIndex, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const std::vector<SliderDataKey>& dataKeys, const std::vector<std::string>& dataFolders, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const std::vector<SliderDataKey>& dataKeys, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const std::vector<std::pair<size_t, size_t>>& dataEntries, const std::vector<std::string>& dataFolders, std::string* errorMessage = nullptr);
	bool SetSliderDataExternal(const std::vector<std::pair<size_t, size_t>>& dataEntries, const std::vector<std::string>& dataFolders, const std::string& osdFileName, std::string* errorMessage = nullptr);
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

	void CloneSlider(const std::string& sliderName, const std::string& cloneName);
	void NegateSlider(const std::string& sliderName, nifly::NiShape* shape);
	void MaskAffected(const std::string& sliderName, nifly::NiShape* shape);

	bool SetSliderFromNIF(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	void SetSliderFromBSD(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	bool SetSliderFromOBJ(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
#ifdef USE_FBXSDK
	bool SetSliderFromFBX(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
#endif
	void SetSliderFromDiff(const std::string& sliderName, nifly::NiShape* shape, const TargetDataDiffs& diff);
	int SaveSliderNIF(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	int SaveSliderBSD(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName);
	int SaveSliderOBJ(const std::string& sliderName, nifly::NiShape* shape, const std::string& fileName, const bool onlyDiff = false);
	bool WriteMorphTRI(const std::string& triPath);
	bool WriteHeadTRI(nifly::NiShape* shape, const std::string& triPath);
	// Imports a FaceGen head TRI file (mesh with morphs) as a new shape. The shape is named
	// after the file if no name is given and made unique if that name is already taken.
	// With withSliders, the morphs are loaded as slider data and the names of the added
	// sliders are put into newSliders, otherwise only the mesh is imported.
	// Returns the created shape or nullptr if the file couldn't be loaded.
	nifly::NiShape* ImportHeadTRI(const std::string& triPath, const std::string& shapeName = "", bool withSliders = true, std::vector<std::string>* newSliders = nullptr);
	bool WriteSFMorphs(nifly::NiShape* shape, const std::string& morphPath);

	float& SliderValue(const size_t index);
	float& SliderValue(const std::string& name);
	float SliderDefault(const size_t index, const bool hi);

	void InitConform();
	void GetConformSliderNames(const ConformOptions& options, std::vector<std::string>& outSliderNames);
	void ConformShape(nifly::NiShape* shape, const ConformOptions& options = ConformOptions());
	void CalcSliderClippingCorrection(nifly::NiShape* shape,
									  const std::string& sliderName,
									  float strength,
									  TargetDataDiffs& outMorphDiffs,
									  const std::unordered_set<uint16_t>* allowedVerts = nullptr);

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
	void UpdateMorphResult(nifly::NiShape* shape, const std::string& sliderName, const TargetDataDiffs& vertUpdates);

	// Converts per-vertex diffs from posed mesh space to rest mesh space.
	// Only has effect when bPose is true; otherwise diffs are unchanged.
	void UndoPoseDiffs(nifly::NiShape* shape, std::unordered_map<uint16_t, nifly::Vector3>& diffs);

	// Computes and stores rest-space NIF diffs in the undo state for
	// pose-independent undo/redo. Only has effect when bPose is true.
	void ComputeUndoRestDiffs(nifly::NiShape* shape, UndoStateShape& uss);
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

	void AddBoneRef(const std::string& boneName);
	void AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const nifly::MatTransform& xformToParent);
	void ModifyCustomBone(AnimBone* bPtr, const std::string& parentBone, const nifly::MatTransform& xformToParent);

	// CopySegPart returns the number of failed triangles, or 0 for success.
	int CopySegPart(nifly::NiShape* shape);

	void ClearWorkSliders();
	void ClearReference();
	void ClearOutfit();
	void ClearSlider(nifly::NiShape* shape, const std::string& sliderName);
	void ClearUnmaskedDiff(nifly::NiShape* shape, const std::string& sliderName, std::unordered_map<uint16_t, float>* mask);
	void DeleteSlider(const std::string& sliderName);

	int LoadSkeletonReference(const std::string& skeletonFileName);
	int LoadReferenceTemplate(const std::string& sourceFile,
							  const std::string& set,
							  const std::string& shape,
							  bool loadAll = false,
							  bool mergeSliders = false,
							  bool mergeZaps = false,
							  bool appendNewSliders = true);
	int LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders = false, bool mergeZaps = false);
	int LoadReference(
		const std::string& fileName, const std::string& setName, const std::string& shapeName = "", bool mergeSliders = false, bool mergeZaps = false, bool appendNewSliders = true);

	int LoadFromSliderSet(const std::string& fileName, const std::string& setName, std::vector<std::string>* origShapeOrder = nullptr);
	int AddFromSliderSet(const std::string& fileName,
						 const std::string& setName,
						 const bool newDataLocal = false,
						 const bool appendNewSliders = true,
						 const bool setAsReference = true);

	TargetDataDiffs* GetDiffSet(SliderData& silderData, nifly::NiShape* shape);

	void CollectVertexData(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices);
	void CollectTriangleData(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint32_t>& indices);
	bool PrepareDeleteVerts(nifly::NiShape* shape, const std::unordered_map<uint16_t, float>& mask, UndoStateShape& uss);
	void ApplyShapeMeshUndo(nifly::NiShape* shape, std::vector<float>& mask, const UndoStateShape& uss, bool bUndo);

	bool PrepareCollapseVertex(nifly::NiShape* shape, UndoStateShape& uss, const std::vector<uint16_t>& indices);
	bool PrepareFlipEdge(nifly::NiShape* shape, UndoStateShape& uss, const nifly::Edge& edge);
	bool PrepareRefineMesh(nifly::NiShape* shape, UndoStateShape& uss, std::vector<bool>& pincs, const Mesh::WeldVertsType& weldVerts, const bool noCurveOffset, std::vector<nifly::Edge>* badEdges = nullptr);

	bool IsVertexOnBoundary(nifly::NiShape* shape, int vi);
	bool PointsHaveDifferingWeightsOrDiffs(nifly::NiShape* shape1, int p1, nifly::NiShape* shape2, int p2);
	void PrepareMergeVertex(nifly::NiShape* shape, UndoStateShape& uss, int selVert, int targVert);
	void PrepareWeldVertex(nifly::NiShape* shape, UndoStateShape& uss, int selVert, int targVert, nifly::NiShape* targShape);

	void CheckMerge(const std::string& sourceName, const std::string& targetName, MergeCheckErrors& e);
	void PrepareCopyGeo(nifly::NiShape* source, nifly::NiShape* target, UndoStateShape& uss);

	nifly::NiShape* DuplicateShape(nifly::NiShape* sourceShape, const std::string& destShapeName);
	void DeleteShape(nifly::NiShape* shape);
	void CaptureShapeDeleteState(nifly::NiShape* shape, UndoStateShapeDelete& state);
	nifly::NiShape* RestoreDeletedShape(UndoStateShapeDelete& state);

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

	void MatchSymmetricVertices(nifly::NiShape* shape, const Mesh::WeldVertsType& weldVerts, SymmetricVertices& r);
	void MatchSymmetricBoneNames(std::vector<std::pair<std::string, std::string>>& pairs, std::vector<std::string>& singles);
	void FindVertexAsymmetries(nifly::NiShape* shape, const SymmetricVertices& symverts, const Mesh::WeldVertsType& weldVerts, VertexAsymmetries& r);
	void PrepareSymmetrizeVertices(nifly::NiShape* shape, UndoStateShape& uss, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, const Mesh::WeldVertsType& weldVerts, const std::vector<bool>& selVerts, const std::vector<std::string>& userNormBones, const std::vector<std::string>& userNotNormBones);
	std::vector<bool> CalculateAsymmetricTriangleVertexMask(nifly::NiShape* shape, const Mesh::WeldVertsType& weldVerts);

	void ChooseClothData(nifly::NifFile& nif);
	void ChoosePhysicsData(nifly::NifFile& nif);
	void ResetTransforms();

	void CreateSkinning(nifly::NiShape* s);
	void RemoveSkinning(nifly::NiShape* s);
	void RemoveSkinning();

	bool CheckForBadBones(bool interactive = true);
	bool ShapeHasBadBones(nifly::NiShape* s);

	void GetAllPoseTransforms(nifly::NiShape* s, std::vector<nifly::MatTransform>& ts);
	void ApplyTransformToOneVertexGeometry(UndoStateVertex& usv, const nifly::MatTransform& t);
	void ApplyPoseTransformsToShapeGeometry(nifly::NiShape* s, UndoStateShape& uss);
	void ApplyPoseTransformsToAllShapeGeometry(UndoStateProject& usp);

	int ImportNIF(const std::string& fileName, bool clear = true, const std::string& inOutfitName = "", std::map<std::string, std::string>* renamedShapes = nullptr);
	int ExportNIF(const std::string& fileName, const std::vector<Mesh*>& modMeshes, bool withRef = false, std::optional<bool> useInternalGeom = std::nullopt);
	int ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes, std::optional<bool> useInternalGeom = std::nullopt);

	// Force internal geometry (flag 0x200) on all BSGeometry shapes in a Starfield NIF.
	void ForceInternalGeometry(nifly::NifFile& nif);

	// Prompt the user to choose internal or external geometry for Starfield NIF export.
	void ConfigureInternalGeometry(nifly::NifFile& nif, const std::string& nifFileName, std::optional<bool> useInternalGeom = std::nullopt);

	// Save external .mesh files for Starfield BSGeometry shapes alongside the NIF.
	bool SaveExternalMeshes(nifly::NifFile& nif, const std::string& nifFileName);

	// Generate mesh-shader meshlets + cull data for any Starfield BSGeometry shape that lacks them
	// No-op for non-Starfield NIFs.
	void GenerateStarfieldMeshlets(nifly::NifFile& nif);

	int ImportOBJ(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportOBJ(const std::string& fileName,
				  const std::vector<nifly::NiShape*>& shapes,
				  bool transToGlobal,
				  const nifly::Vector3& scale = nifly::Vector3(1.0f, 1.0f, 1.0f),
				  const nifly::Vector3& offset = nifly::Vector3());

#ifdef USE_FBXSDK
	int ImportFBX(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
	int ExportFBX(const std::string& fileName, const std::vector<nifly::NiShape*>& shapes, bool transToGlobal);
#endif
};
