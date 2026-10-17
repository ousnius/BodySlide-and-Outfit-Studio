/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "EditorState.h"
#include "OutfitProject.h"
#include "../components/MeshAnalysis.h"
#include "../components/PoseData.h"

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

	// Sets all sliders that aren't clamped to zero
	RefreshFlags ZeroSliders();

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

	std::unique_ptr<EditSnapshot> CaptureSnapshot();

	// Restores the project data. The shape meshes have to be rebuilt afterwards, then RestoreMasks applies the masks.
	RefreshFlags RestoreSnapshot(const EditSnapshot& snapshot);
	void RestoreMasks(const EditSnapshot& snapshot);

	// Conforms the sliders of the shapes to the reference shape. Without slider names in the
	// options, all sliders that aren't zaps or UV sliders are conformed.
	OpResult ConformSliders(const std::vector<nifly::NiShape*>& shapes, const ConformOptions& options);

	// Pushes the shapes out of the reference shape. Shapes mode fixes the base geometry,
	// sliders mode fixes the morph of each slider (all non-zap, non-UV sliders without names).
	OpResult FixClipping(const std::vector<nifly::NiShape*>& shapes, ClippingFixMode mode, float strength, const std::vector<std::string>& sliderNames = {});

	// Bones for painting weights of the active bone, optionally mirrored to xMirrorBone
	WeightBrushBones GetWeightBrushBones(const std::string& xMirrorBone);

	// Writes the vertex positions of the shape's mesh back to the shape
	void UpdateShapeFromMesh(nifly::NiShape* shape);

	// Copies bone weights from the reference shape to the masked-out vertices of the shapes.
	// The changes are recorded in usp, so the caller can keep them as an undo step.
	OpResult CopyBoneWeights(const std::vector<nifly::NiShape*>& shapes, const BoneWeightCopyOptions& options, UndoStateProject& usp);

private:
	OutfitProject& project;
	EditorState& editor;

	Mesh* GetMesh(const std::string& shapeName) { return project.GetHost().GetShapeMesh(shapeName); }
};
