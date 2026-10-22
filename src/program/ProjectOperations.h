/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "EditorState.h"
#include "OutfitProject.h"
#include "../components/MeshAnalysis.h"
#include "../components/PoseData.h"
#include "../components/SliderPresets.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Parts of the GUI that have to be refreshed after an operation changed the project
enum class RefreshFlags : uint32_t {
	None = 0,
	PendingChanges = 1 << 0, // The project has unsaved changes
	Meshes = 1 << 1,		 // Mesh vertices have to be recalculated from the project
	SliderData = 1 << 2,	 // Which shapes have data for which slider
	SliderSet = 1 << 3,		 // Sliders were added, removed or renamed
	SliderValues = 1 << 4,	 // Slider values changed
	ShapeList = 1 << 5,		 // Shapes were added, removed or renamed
	Anim = 1 << 6,			 // Bones or weights changed
	Pose = 1 << 7,			 // The preview pose changed
	WeightDisplay = 1 << 8,	 // Which bones have weights, and how many
	Shaders = 1 << 9,		 // What the mesh shaders show changed
};

inline RefreshFlags operator|(RefreshFlags a, RefreshFlags b) {
	return static_cast<RefreshFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline RefreshFlags& operator|=(RefreshFlags& a, RefreshFlags b) {
	a = a | b;
	return a;
}

inline bool HasRefresh(RefreshFlags flags, RefreshFlags test) {
	return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(test)) != 0;
}

struct OpResult {
	bool ok = true;
	std::string error;
	RefreshFlags refresh = RefreshFlags::None;

	static OpResult Fail(const std::string& message) {
		OpResult result;
		result.ok = false;
		result.error = message;
		return result;
	}
};

enum class ClippingFixMode { Shapes, Sliders };

struct BoneWeightCopyOptions {
	float proximityRadius = 10.0f;
	int maxResults = 10;
	// Bones to copy. Empty copies all bones of the reference shape.
	std::vector<std::string> selectedBones;
	// Give the shapes the global-to-skin transform of the reference
	bool copySkinTransform = false;
	// Move the geometry along when the skin transform changes, so the shapes keep their place
	bool transformGeometry = false;
};

// Bones a weight brush works on, from the active bone and the normalization settings
struct WeightBrushBones {
	// The active bone first, then the x-mirror bone if there is one, then the bones to normalize with
	std::vector<std::string> boneNames;
	std::vector<std::string> lockedBoneNames;
	bool spreadWeight = false;
	bool xMirrorBone = false;

	// Configures one of the weight brush classes, which share these members
	template<typename Brush>
	void ApplyTo(Brush& brush, AnimInfo* animInfo) const {
		brush.animInfo = animInfo;
		brush.boneNames = boneNames;
		brush.lockedBoneNames = lockedBoneNames;
		brush.bSpreadWeight = spreadWeight;
		brush.bXMirrorBone = xMirrorBone;
	}
};

// Where to load a reference shape from: a NIF, a set of a slider set file, or a reference template
struct ReferenceSource {
	std::string file;
	// The set of a slider set file. Without a set the file is used as a reference template.
	std::string set;
	std::string shape;
	// Templates only: load all shapes of the set, not only the reference shape
	bool loadAll = false;
	// Keep the existing sliders or zaps and add the reference's data to them
	bool mergeSliders = false;
	bool mergeZaps = false;
	bool appendNewSliders = false;
};

struct SaveProjectOptions {
	std::string displayName;
	std::string sliderSetFile;   // Relative to the SliderSets folder
	std::string shapeDataFolder; // Relative to the ShapeData folder
	std::string shapeDataFile;
	std::string outputDataPath;  // Game path of the meshes BodySlide builds
	std::string outputFileName;
	bool genWeights = true;
	bool copyReference = true;
	std::string sfMorphPath;
	std::string sfMorphTargetShape;
};

struct ExportOptions {
	// NIF, OBJ and FBX: include the reference shape
	bool withReference = false;
	// OBJ and FBX: positions in global space instead of the space of each shape
	bool global = false;
	// Only these shapes, like exporting the selected shapes in the window. All outfit shapes when empty.
	std::vector<std::string> shapes;
	// TRI: the head morphs of the one shape in shapes instead of the body morphs of the project
	bool headTri = false;
};

// Differences between the shapes of the open project and the same shapes in another project
struct ProjectComparison {
	struct SliderDifference {
		std::string shape;
		std::string slider;
		bool onlyHere = false;  // The other project has no data for it, compared with no morph
		bool onlyThere = false; // This project has no data for it
		MeshAnalysis::DeltaComparison deltas;
	};

	struct WeightDifference {
		std::string shape;
		std::string bone;
		size_t vertices = 0; // Vertices weighted to the bone in either project
		float meanError = 0.0f;
		float maxError = 0.0f;
	};

	std::vector<SliderDifference> sliders;
	std::vector<WeightDifference> weights;
	// Shapes that couldn't be compared, with the reason
	std::vector<std::pair<std::string, std::string>> skippedShapes;
};

// How RigidifySlider fits the parts of a shape
struct RigidifyOptions {
	// Let the parts also grow or shrink evenly with the morph, instead of only moving and turning
	bool allowScale = false;
	// Fit each connected part of the unmasked vertices on its own, e.g. every buckle. Otherwise all
	// unmasked vertices of a shape move as one.
	bool perPart = true;
	// Vertices closer than this count as joined, like the copies of a vertex along a split seam
	float weldDistance = 0.001f;
	// Keep the parts this far outside the reference with the slider at 1, by moving each part out as a whole where
	// it would sink in (a rigid buckle can't follow a valley the slider makes). A part that's closer or inside with
	// all sliders at 0 only has to stay as it is there. Negative leaves the parts where the fit puts them.
	float clearance = -1.0f;
};

// Zap sliders of a shape, to give another shape the same zaps by proximity, e.g. the new body when the
// reference changes
struct ZapSource {
	struct Zap {
		std::vector<bool> zapped;
		float defSmallValue = 0.0f;
		float defBigValue = 0.0f;
		bool hidden = false;
	};

	std::string shapeName;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::map<std::string, Zap> zaps; // By slider name, only zaps with data on the shape
};

struct ZapTransfer {
	std::string slider;
	size_t sourceZapped = 0; // Vertices the zap removes on the source shape
	size_t zapped = 0;		 // and on the target shape now
	bool created = false;	 // The project didn't have the slider yet
};

// Project data and shape masks, to return to after trying out a change
struct EditSnapshot {
	std::unique_ptr<OutfitProject::Snapshot> project;
	std::unordered_map<std::string, std::vector<float>> masks;
};

// Edits a project the same way no matter who asks: the window, automation or an external API.
// Operations change the project and the shape meshes only, and report what the GUI has to refresh.
class ProjectOperations {
public:
	ProjectOperations(OutfitProject& inProject, EditorState& inEditor)
		: project(inProject)
		, editor(inEditor) {}

	// Applies the per-vertex changes recorded in an undo state (slider morphs, weights,
	// vertex colors or alpha) to the project, or reverts them.
	RefreshFlags ApplyShapeEdits(UndoStateProject& usp, bool undo);

	// Splits the bones of the selected shapes by whether they take part in weight normalization
	void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones);

	OpResult LoadReference(const ReferenceSource& source);
	// Adds the shapes and sliders of a set of a slider set file to the project
	OpResult AddProject(const std::string& file, const std::string& set, bool appendNewSliders);
	OpResult SaveProject(const SaveProjectOptions& options);
	// The format comes from the extension: nif, obj, fbx, osd (slider data) or tri (morphs)
	OpResult ExportFile(const std::string& fileName, const ExportOptions& options);

	// Sets all sliders that aren't clamped to zero
	RefreshFlags ZeroSliders();

	// Slider values (0 to 1) of a body preset, for the high (big) or low (small) weight body, the way the
	// window applies presets: sliders the preset doesn't have take their default, zap sliders without a
	// value are turned off, and clamp sliders are left out.
	std::map<std::string, float> GetPresetValues(PresetCollection& presets, const std::string& preset, bool high);

	// Sets slider values by name, 0 to 1. Fails on the first unknown slider without changing any.
	OpResult SetSliderValues(const std::map<std::string, float>& values);

	// Morph of a slider on a shape in NIF coordinates, one delta per vertex
	OpResult GetSliderDeltas(nifly::NiShape* shape, const std::string& sliderName, std::vector<nifly::Vector3>& outDeltas);

	// Replaces the deltas of the given vertices, or adds to them. Deltas are in NIF coordinates.
	OpResult SetSliderDeltas(nifly::NiShape* shape, const std::string& sliderName, const std::unordered_map<uint16_t, nifly::Vector3>& deltas, bool add);

	OpResult ScaleSliderDeltas(nifly::NiShape* shape, const std::string& sliderName, float factor);

	// Smooths the morph of an outfit shape. Fully masked vertices keep their deltas.
	OpResult SmoothSliderDeltas(nifly::NiShape* shape, const std::string& sliderName, int iterations, float strength);

	// Poses the skeleton to preview the shapes with it. The shape geometry stays unposed.
	RefreshFlags SetPreviewPose(const PoseData& pose);
	RefreshFlags ClearPreviewPose();
	// Returns all bones of the skeleton to their unposed transforms
	void ResetPose();

	// Morph quality of one slider on a shape, measured with only that slider applied
	OpResult AnalyzeSlider(nifly::NiShape* shape, const std::string& sliderName, MeshAnalysis::MorphStats& outStats);

	// How far a shape goes into the reference shape with the current slider values. Like the
	// clipping fix, this compares the shapes in NIF coordinates.
	OpResult AnalyzeClipping(nifly::NiShape* shape, MeshAnalysis::PenetrationStats& outStats, float nearDistance = 0.1f);

	OpResult AnalyzeWeights(nifly::NiShape* shape, MeshAnalysis::WeightStats& outStats, int influenceLimit = 4);

	// Compares the slider data and weights of the outfit shapes with a set of a slider set file, e.g. a
	// finished conversion of the same outfit. shapeMap maps names of this project to the other one, other
	// shapes are matched by name. Shapes are only compared when their vertex counts match.
	OpResult CompareToProject(const std::string& file,
							  const std::string& set,
							  const std::map<std::string, std::string>& shapeMap,
							  ProjectComparison& outComparison);

	std::unique_ptr<EditSnapshot> CaptureSnapshot();

	// Restores the project data. The shape meshes have to be rebuilt afterwards, then RestoreMasks applies the masks.
	RefreshFlags RestoreSnapshot(const EditSnapshot& snapshot);
	void RestoreMasks(const EditSnapshot& snapshot);

	// Conforms the sliders of the shapes to the reference shape. Without slider names in the
	// options, all sliders that aren't zaps or UV sliders are conformed.
	OpResult ConformSliders(const std::vector<nifly::NiShape*>& shapes, const ConformOptions& options);

	// Pushes the shapes out of the reference shape. Shapes mode fixes the base geometry,
	// sliders mode fixes the morph of each slider (all non-zap, non-UV sliders without names).
	// Each vertex takes the fix scaled by how unmasked it is, so masked vertices keep their
	// place and a smoothed mask border blends into the fix.
	OpResult FixClipping(const std::vector<nifly::NiShape*>& shapes, ClippingFixMode mode, float strength, const std::vector<std::string>& sliderNames = {});

	// Fixes the clipping of a slider combination (the other sliders at zero) that no slider has on its own, e.g.
	// a body preset. The push out of the reference is shared among the receiving sliders (all sliders of the
	// combination with a value by default) by how far each moves the vertex, and scaled by its value, so the
	// combination comes out fixed and each slider takes its part. Sliders below 0.5 are scaled as if at 0.5, so a
	// slider set low doesn't get a spike, and the combination comes out partly fixed then. Masked vertices are left
	// alone like FixClipping.
	OpResult FixClippingCombination(const std::vector<nifly::NiShape*>& shapes,
									const std::map<std::string, float>& values,
									const std::vector<std::string>& receivers,
									float strength,
									size_t* outChanged = nullptr);

	// Shows the weights of a bone as weight colors on the meshes of the shapes and of the reference.
	// The other meshes are cleared. Weight colors have to be turned on to see them.
	void FillWeightColors(const std::string& bone, const std::vector<nifly::NiShape*>& shapes);

	// Colors the mesh of a shape by one value per vertex, from minValue (blue) to maxValue (red).
	// Vertices with FLT_MAX get grey. Shows with vertex colors turned on; the window's FillVertexColors
	// puts the shape's own colors back.
	OpResult ShowHeatmap(nifly::NiShape* shape, const std::vector<float>& values, float minValue, float maxValue);

	// Bones for painting weights of the active bone, optionally mirrored to xMirrorBone
	WeightBrushBones GetWeightBrushBones(const std::string& xMirrorBone);

	// Writes the vertex positions of the shape's mesh back to the shape
	void UpdateShapeFromMesh(nifly::NiShape* shape);

	// Copies bone weights from the reference shape to the masked-out vertices of the shapes.
	// The changes are recorded in usp, so the caller can keep them as an undo step.
	OpResult CopyBoneWeights(const std::vector<nifly::NiShape*>& shapes, const BoneWeightCopyOptions& options, UndoStateProject& usp);

	// Adds changes (NIF coordinates) to the morph of a slider on an outfit shape. They are recorded in usp
	// like a brush stroke in slider edit mode, so the caller can keep them as an undo step; all changes
	// recorded in one usp have to be for the same slider.
	OpResult ChangeSliderDeltas(nifly::NiShape* shape, const std::string& sliderName, const std::unordered_map<uint16_t, nifly::Vector3>& changes, UndoStateProject& usp);

	// Makes the unmasked parts of the shapes follow the morph of a slider as a whole, without bending: each
	// part gets the motion that best fits its deltas, e.g. so buckles and armor plates keep their shape.
	// Partly masked vertices blend between the morph and that motion. The changes are recorded in usp.
	OpResult RigidifySlider(const std::vector<nifly::NiShape*>& shapes,
							const std::string& sliderName,
							const RigidifyOptions& options,
							UndoStateProject& usp,
							size_t* outParts = nullptr);

	// The zap sliders with data on a shape of a project (the named ones, or all), with the shape's rest geometry
	static ZapSource CaptureZaps(OutfitProject& fromProject, nifly::NiShape* shape, const std::vector<std::string>& sliderNames = {});

	// The same for a shape of a set of another slider set file, by shape or target name
	OpResult CaptureZapsFromProject(const std::string& file,
									const std::string& set,
									const std::string& shapeName,
									const std::vector<std::string>& sliderNames,
									ZapSource& outSource);

	// Zaps the vertices of a shape whose closest point on the source shape's surface (within maxDistance) is
	// nearest to a zapped source vertex, replacing the shape's data of those zap sliders. Zap sliders the
	// project doesn't have are created with the source's settings.
	OpResult ApplyZaps(const ZapSource& source, nifly::NiShape* target, float maxDistance, std::vector<ZapTransfer>& outTransfers);

private:
	OutfitProject& project;
	EditorState& editor;

	Mesh* GetMesh(const std::string& shapeName) { return project.GetHost().GetShapeMesh(shapeName); }
};
