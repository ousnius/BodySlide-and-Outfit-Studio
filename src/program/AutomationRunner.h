/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Automation.h"
#include "../files/ObjFile.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <wx/string.h>

class OutfitStudioFrame;
class OutfitProject;

namespace nifly {
class NiShape;
}

// Executes automation steps on the project of an Outfit Studio window. Used by the
// automation dialog, headless automation and anything else that runs steps.
class AutomationRunner {
public:
	explicit AutomationRunner(OutfitStudioFrame* outfitStudio);

	// Unattended runs import with default options instead of asking for them
	void SetHeadless(bool headless) { headlessMode = headless; }
	// Options of unattended OBJ imports
	void SetObjImportOptions(const ObjImportOptions& options) { objImportOptions = options; }

	// Starts a run with the script's variables. Set Variable steps add to them during the run.
	void BeginRun(const std::map<std::string, std::string>& baseVariables);

	// Fills in variables set earlier in the run, executes the step and refreshes the GUI.
	// Returns 0 on success.
	// executedStep receives the step with the variables filled in.
	int RunStep(const AutomationStep& step, AutomationStep* executedStep = nullptr);

	// Executes a step as it is, without refreshing the GUI. Returns 0 on success.
	int ExecuteStep(const AutomationStep& step);
	void RefreshAfterStep(const AutomationStep& step);

	void ResetAndClearProject();

	// Imports with default options when headless, otherwise after asking for the options.
	int ImportOBJFile(const std::string& fileName);
#ifdef USE_FBXSDK
	int ImportFBXFile(const std::string& fileName);
#endif


	nifly::NiShape* FindShapeByName(const std::string& name);
	std::vector<nifly::NiShape*> ResolveTargetShapes(const AutomationStep& step, bool includeBaseShapeOnEmpty = true);

	static wxString MakeAbsoluteToProject(const wxString& path);
	static bool StepChangesSliderSet(AutomationStepType type);

private:
	OutfitStudioFrame* outfitStudio = nullptr;
	OutfitProject* project = nullptr;
	bool headlessMode = false;
	ObjImportOptions objImportOptions;

	// Variables defined by Set Variable steps while a script runs. Substituted into
	// every following step, on top of the up front pass over the placeholder table.
	std::map<std::string, std::string> runtimeVariables;
	std::map<std::string, std::string> runBaseVariables;

	// Steps can replace the project of the window, so it is looked up again before each one
	void SyncProject();

	// Rebuilds the render meshes after a step changed vertex or triangle counts,
	// carrying the remapped masks over to the new meshes.
	void RefreshMeshesWithMasks(std::unordered_map<std::string, std::vector<float>>& maskStash);

	int ImportSliderDataFromTRI(const AutomationStep& step, const std::string& filePath);

	int ExecuteStepClearProject(const AutomationStep& step);
	int ExecuteStepLoadReference(const AutomationStep& step);
	int ExecuteStepAddProject(const AutomationStep& step);
	int ExecuteStepSetSliderValues(const AutomationStep& step);
	int ExecuteStepConformSliders(const AutomationStep& step);
	int ExecuteStepCopyBoneWeights(const AutomationStep& step);
	int ExecuteStepSetBaseShape(const AutomationStep& step);
	int ExecuteStepClearReference(const AutomationStep& step);
	int ExecuteStepTransformShape(const AutomationStep& step);
	int ExecuteStepInvertUVs(const AutomationStep& step);
	int ExecuteStepDeleteBones(const AutomationStep& step);
	int ExecuteStepAddCustomBone(const AutomationStep& step);
	int ExecuteStepEditBone(const AutomationStep& step);
	int ExecuteStepRemoveSkinning(const AutomationStep& step);
	int ExecuteStepApplyPose(const AutomationStep& step);
	int ExecuteStepImportSliderData(const AutomationStep& step);
	int ExecuteStepImportFile(const AutomationStep& step);
	int ExecuteStepDeleteShape(const AutomationStep& step);
	int ExecuteStepRenameShape(const AutomationStep& step);
	int ExecuteStepSaveProject(const AutomationStep& step);
	int ExecuteStepExportFile(const AutomationStep& step);
	int ExecuteStepExportSFMorphs(const AutomationStep& step);
	int ExecuteStepRefineMesh(const AutomationStep& step);
	int ExecuteStepDeleteSlider(const AutomationStep& step);
	int ExecuteStepSetReferenceShape(const AutomationStep& step);
	int ExecuteStepApplyTransforms(const AutomationStep& step);
	int ExecuteStepResetTransforms(const AutomationStep& step);
	int ExecuteStepDuplicateShape(const AutomationStep& step);
	int ExecuteStepChangePartitions(const AutomationStep& step);
	int ExecuteStepMirrorShape(const AutomationStep& step);
	int ExecuteStepRecalcNormals(const AutomationStep& step);
	int ExecuteStepLoadMask(const AutomationStep& step);
	int ExecuteStepClearMask(const AutomationStep& step);
	int ExecuteStepSetSliderProperties(const AutomationStep& step);
	int ExecuteStepSetShaderProperties(const AutomationStep& step);
	int ExecuteStepSetGeometryProperties(const AutomationStep& step);
	int ExecuteStepSetExtraData(const AutomationStep& step);
	int ExecuteStepDeleteExtraData(const AutomationStep& step);
	int ExecuteStepSetTexturePaths(const AutomationStep& step);
	int ExecuteStepRemoveUnusedNodes(const AutomationStep& step);
	int ExecuteStepFixClipping(const AutomationStep& step);
	int ExecuteStepFixBadBones(const AutomationStep& step);
	int ExecuteStepAddBone(const AutomationStep& step);
	int ExecuteStepTransferWeights(const AutomationStep& step);
	int ExecuteStepSetBoneTransform(const AutomationStep& step);
	int ExecuteStepMakeConversionRef(const AutomationStep& step);
	int ExecuteStepCopySegPart(const AutomationStep& step);
	int ExecuteStepDeleteVertices(const AutomationStep& step);
	int ExecuteStepSeparateVertices(const AutomationStep& step);
	int ExecuteStepMergeGeometry(const AutomationStep& step);
	int ExecuteStepSymmetrizeVertices(const AutomationStep& step);
	int ExecuteStepClearSliderData(const AutomationStep& step);
	int ExecuteStepCloneSlider(const AutomationStep& step);
	int ExecuteStepNegateSlider(const AutomationStep& step);
	int ExecuteStepNewCombinedSlider(const AutomationStep& step);
	int ExecuteStepNewZapSlider(const AutomationStep& step);
	int ExecuteStepGrowShrinkMask(const AutomationStep& step);
	int ExecuteStepInvertMask(const AutomationStep& step);
	int ExecuteStepMaskAsymmetric(const AutomationStep& step);
	int ExecuteStepMaskBoneWeighted(const AutomationStep& step);
	int ExecuteStepMaskSliderAffected(const AutomationStep& step);
	int ExecuteStepMaskWeighted(const AutomationStep& step);
	int ExecuteStepSaveMask(const AutomationStep& step);
	int ExecuteStepSetVariable(const AutomationStep& step);
	int ExecuteStepLogMessage(const AutomationStep& step);
};
