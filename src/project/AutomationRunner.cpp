/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AutomationRunner.h"

#include "OutfitProject.h"
#include "ProjectOperations.h"
#include "StudioSession.h"

#include "utils/ProjectUtil.h"

#include <NifFile.hpp>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace nifly;
AutomationRunner::AutomationRunner(IStudioSession& inSession)
	: session(&inSession)
	, project(inSession.GetProject()) {}

void AutomationRunner::SyncProject() {
	project = session->GetProject();
}

void AutomationRunner::BeginRun(const std::map<std::string, std::string>& baseVariables) {
	runBaseVariables = baseVariables;
	runtimeVariables.clear();
}

int AutomationRunner::RunStep(const AutomationStep& scriptStep, AutomationStep* executedStep) {
	// Copied so Set Variable steps earlier in the run can still fill in
	// placeholders that were unknown when the script was substituted
	AutomationStep step = scriptStep;
	if (!runtimeVariables.empty())
		SubstituteStepPlaceholders(step, runtimeVariables);

	int err = ExecuteStep(step);
	RefreshAfterStep(step);

	if (executedStep)
		*executedStep = step;
	return err;
}

void AutomationRunner::RefreshAfterStep(const AutomationStep& step) {
	SyncProject();
	session->RefreshGUIFromProj();

	if (StepChangesSliderSet(step.type))
		session->CreateSetSliders();

	session->ApplySliders();
}

int AutomationRunner::ExecuteStep(const AutomationStep& step) {
	using ExecuteFn = int (AutomationRunner::*)(const AutomationStep&);
	struct Binding {
		AutomationStepType type;
		ExecuteFn execute;
	};

	static const Binding bindings[] = {
		{AutomationStepType::AddCustomBone, &AutomationRunner::ExecuteStepAddCustomBone},
		{AutomationStepType::CopyBoneWeights, &AutomationRunner::ExecuteStepCopyBoneWeights},
		{AutomationStepType::DeleteBones, &AutomationRunner::ExecuteStepDeleteBones},
		{AutomationStepType::EditBone, &AutomationRunner::ExecuteStepEditBone},
		{AutomationStepType::DuplicateBones, &AutomationRunner::ExecuteStepDuplicateBones},
		{AutomationStepType::MirrorBones, &AutomationRunner::ExecuteStepMirrorBones},
		{AutomationStepType::RemoveSkinning, &AutomationRunner::ExecuteStepRemoveSkinning},
		{AutomationStepType::ExportFile, &AutomationRunner::ExecuteStepExportFile},
		{AutomationStepType::ExportSFMorphs, &AutomationRunner::ExecuteStepExportSFMorphs},
		{AutomationStepType::SaveProject, &AutomationRunner::ExecuteStepSaveProject},
		{AutomationStepType::ImportFile, &AutomationRunner::ExecuteStepImportFile},
		{AutomationStepType::ImportFromGame, &AutomationRunner::ExecuteStepImportFromGame},
		{AutomationStepType::ImportSliderData, &AutomationRunner::ExecuteStepImportSliderData},
		{AutomationStepType::AddProject, &AutomationRunner::ExecuteStepAddProject},
		{AutomationStepType::ClearProject, &AutomationRunner::ExecuteStepClearProject},
		{AutomationStepType::ClearReference, &AutomationRunner::ExecuteStepClearReference},
		{AutomationStepType::LoadReference, &AutomationRunner::ExecuteStepLoadReference},
		{AutomationStepType::SetBaseShape, &AutomationRunner::ExecuteStepSetBaseShape},
		{AutomationStepType::SetReferenceShape, &AutomationRunner::ExecuteStepSetReferenceShape},
		{AutomationStepType::ApplyPose, &AutomationRunner::ExecuteStepApplyPose},
		{AutomationStepType::DeleteShape, &AutomationRunner::ExecuteStepDeleteShape},
		{AutomationStepType::DuplicateShape, &AutomationRunner::ExecuteStepDuplicateShape},
		{AutomationStepType::ChangePartitions, &AutomationRunner::ExecuteStepChangePartitions},
		{AutomationStepType::FixBadBones, &AutomationRunner::ExecuteStepFixBadBones},
		{AutomationStepType::FixClipping, &AutomationRunner::ExecuteStepFixClipping},
		{AutomationStepType::InvertUVs, &AutomationRunner::ExecuteStepInvertUVs},
		{AutomationStepType::MirrorShape, &AutomationRunner::ExecuteStepMirrorShape},
		{AutomationStepType::RecalcNormals, &AutomationRunner::ExecuteStepRecalcNormals},
		{AutomationStepType::RefineMesh, &AutomationRunner::ExecuteStepRefineMesh},
		{AutomationStepType::RenameShape, &AutomationRunner::ExecuteStepRenameShape},
		{AutomationStepType::ApplyTransforms, &AutomationRunner::ExecuteStepApplyTransforms},
		{AutomationStepType::ResetTransforms, &AutomationRunner::ExecuteStepResetTransforms},
		{AutomationStepType::TransformShape, &AutomationRunner::ExecuteStepTransformShape},
		{AutomationStepType::SetGeometryProperties, &AutomationRunner::ExecuteStepSetGeometryProperties},
		{AutomationStepType::SetExtraData, &AutomationRunner::ExecuteStepSetExtraData},
		{AutomationStepType::DeleteExtraData, &AutomationRunner::ExecuteStepDeleteExtraData},
		{AutomationStepType::ConformSliders, &AutomationRunner::ExecuteStepConformSliders},
		{AutomationStepType::DeleteSlider, &AutomationRunner::ExecuteStepDeleteSlider},
		{AutomationStepType::SetSliderValues, &AutomationRunner::ExecuteStepSetSliderValues},
		{AutomationStepType::SetSliderProperties, &AutomationRunner::ExecuteStepSetSliderProperties},
		{AutomationStepType::SetShaderProperties, &AutomationRunner::ExecuteStepSetShaderProperties},
		{AutomationStepType::SetTexturePaths, &AutomationRunner::ExecuteStepSetTexturePaths},
		{AutomationStepType::ClearMask, &AutomationRunner::ExecuteStepClearMask},
		{AutomationStepType::LoadMask, &AutomationRunner::ExecuteStepLoadMask},
		{AutomationStepType::RemoveUnusedNodes, &AutomationRunner::ExecuteStepRemoveUnusedNodes},
		{AutomationStepType::AddBone, &AutomationRunner::ExecuteStepAddBone},
		{AutomationStepType::TransferWeights, &AutomationRunner::ExecuteStepTransferWeights},
		{AutomationStepType::SetBoneTransform, &AutomationRunner::ExecuteStepSetBoneTransform},
		{AutomationStepType::MakeConversionRef, &AutomationRunner::ExecuteStepMakeConversionRef},
		{AutomationStepType::CopySegPart, &AutomationRunner::ExecuteStepCopySegPart},
		{AutomationStepType::DeleteVertices, &AutomationRunner::ExecuteStepDeleteVertices},
		{AutomationStepType::SeparateVertices, &AutomationRunner::ExecuteStepSeparateVertices},
		{AutomationStepType::MergeGeometry, &AutomationRunner::ExecuteStepMergeGeometry},
		{AutomationStepType::SymmetrizeVertices, &AutomationRunner::ExecuteStepSymmetrizeVertices},
		{AutomationStepType::SmoothVertices, &AutomationRunner::ExecuteStepSmoothVertices},
		{AutomationStepType::ClearSliderData, &AutomationRunner::ExecuteStepClearSliderData},
		{AutomationStepType::CloneSlider, &AutomationRunner::ExecuteStepCloneSlider},
		{AutomationStepType::NegateSlider, &AutomationRunner::ExecuteStepNegateSlider},
		{AutomationStepType::RigidifySlider, &AutomationRunner::ExecuteStepRigidifySlider},
		{AutomationStepType::NewCombinedSlider, &AutomationRunner::ExecuteStepNewCombinedSlider},
		{AutomationStepType::NewZapSlider, &AutomationRunner::ExecuteStepNewZapSlider},
		{AutomationStepType::GrowShrinkMask, &AutomationRunner::ExecuteStepGrowShrinkMask},
		{AutomationStepType::InvertMask, &AutomationRunner::ExecuteStepInvertMask},
		{AutomationStepType::MaskAsymmetric, &AutomationRunner::ExecuteStepMaskAsymmetric},
		{AutomationStepType::MaskBox, &AutomationRunner::ExecuteStepMaskBox},
		{AutomationStepType::MaskCovered, &AutomationRunner::ExecuteStepMaskCovered},
		{AutomationStepType::MaskBoneWeighted, &AutomationRunner::ExecuteStepMaskBoneWeighted},
		{AutomationStepType::MaskSliderAffected, &AutomationRunner::ExecuteStepMaskSliderAffected},
		{AutomationStepType::MaskWeighted, &AutomationRunner::ExecuteStepMaskWeighted},
		{AutomationStepType::SaveMask, &AutomationRunner::ExecuteStepSaveMask},
		{AutomationStepType::SmoothMask, &AutomationRunner::ExecuteStepSmoothMask},
		{AutomationStepType::SetVariable, &AutomationRunner::ExecuteStepSetVariable},
		{AutomationStepType::LogMessage, &AutomationRunner::ExecuteStepLogMessage},
	};

	static_assert(std::size(bindings) == AutomationStepTypeCount, "every AutomationStepType needs a binding");

	SyncProject();
	for (const auto& binding : bindings)
		if (binding.type == step.type)
			return (this->*binding.execute)(step);

	return 0;
}

NiShape* AutomationRunner::FindShapeByName(const std::string& name) {
	SyncProject();
	auto shapes = project->GetWorkNif()->GetShapes();
	for (auto* shape : shapes) {
		if (shape->name.get() == name)
			return shape;
	}
	return nullptr;
}
std::vector<NiShape*> AutomationRunner::ResolveTargetShapes(const AutomationStep& step, bool includeBaseShapeOnEmpty, bool patternsMatchBaseShape) {
	SyncProject();
	if (step.targetMeshes.empty()) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		std::vector<NiShape*> result;
		auto* baseShape = project->GetBaseShape();
		for (auto* shape : allShapes) {
			if (includeBaseShapeOnEmpty || shape != baseShape)
				result.push_back(shape);
		}
		return result;
	}

	std::vector<NiShape*> result;
	auto addUniqueShape = [&result](NiShape* shape) {
		if (shape && std::find(result.begin(), result.end(), shape) == result.end())
			result.push_back(shape);
	};

	if (step.targetRegex) {
		// Patterns leave the reference out, so ".*" for every outfit shape doesn't clear or mask the body's slider
		// data too. A step that is meant for the reference names it without TargetRegex or sets TargetReference,
		// and steps that work on the reference when empty (MaskCovered) let patterns match it as well.
		auto allShapes = project->GetWorkNif()->GetShapes();
		auto* baseShape = (step.targetReference || patternsMatchBaseShape) ? nullptr : project->GetBaseShape();
		for (const auto& pattern : step.targetMeshes) {
			try {
				std::regex re(pattern, std::regex::icase);
				bool matched = false;
				for (auto* shape : allShapes) {
					if (shape != baseShape && std::regex_search(shape->name.get(), re)) {
						addUniqueShape(shape);
						matched = true;
					}
				}
				if (!matched)
					wxLogWarning("Automation: Target shape regex '%s' matched no shapes.", pattern);
			}
			catch (const std::regex_error& e) {
				wxLogWarning("Automation: Invalid target shape regex '%s' (%s); skipping pattern.", pattern, e.what());
			}
		}
	}
	else {
		for (const auto& name : step.targetMeshes) {
			NiShape* shape = FindShapeByName(name);
			if (shape)
				addUniqueShape(shape);
			else
				wxLogWarning("Automation: Target shape '%s' not found.", name);
		}
	}

	return result;
}
void AutomationRunner::ResetAndClearProject() {
	SyncProject();
	project->SetBaseShape(nullptr, false);
	// One by one, so what the project keeps by shape name (the reference's source, textures, materials) goes with
	// them instead of passing to a shape of the same name in the next project
	for (auto* shape : project->GetWorkNif()->GetShapes())
		project->DeleteShape(shape);
	project->GetWorkAnim()->Clear();
	project->GetWorkNif()->Clear();
	project->ClearLoadedFileData();
	session->ResetProject();
	SyncProject();
}
wxString AutomationRunner::MakeAbsoluteToProject(const wxString& path) {
	if (path.empty())
		return path;

	wxFileName fn(path);
	if (!fn.IsRelative())
		return path;

	fn.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));
	return fn.GetFullPath();
}

bool AutomationRunner::StepChangesSliderSet(AutomationStepType type) {
	switch (type) {
		case AutomationStepType::ClearProject:
		case AutomationStepType::LoadReference:
		case AutomationStepType::AddProject:
		case AutomationStepType::DeleteSlider:
		case AutomationStepType::ClearReference:
		case AutomationStepType::SetBaseShape:
		case AutomationStepType::ImportSliderData:
		case AutomationStepType::ImportFile:
		case AutomationStepType::ImportFromGame:
		case AutomationStepType::CloneSlider:
		case AutomationStepType::MakeConversionRef:
		case AutomationStepType::NewCombinedSlider:
		case AutomationStepType::NewZapSlider:
			return true;
		default:
			return false;
	}
}

void AutomationRunner::RefreshMeshesWithMasks(std::unordered_map<std::string, std::vector<float>>& maskStash) {
	// The stashed masks were already remapped to the new vertex lists by
	// ApplyShapeMeshUndo while the render meshes still carry the old ones, so the
	// rebuild has to happen without stashing again.
	session->RefreshGUIFromProj(false, false);
	session->UnstashMasks(maskStash);
}

int AutomationRunner::ExecuteStepSetVariable(const AutomationStep& step) {
	if (step.variableName.empty()) {
		wxLogError("Automation: SetVariable - no variable name specified.");
		return 1;
	}

	// The placeholder variables are substituted once before the run, so a name
	// that is defined there has already been replaced everywhere.
	if (runBaseVariables.find(step.variableName) != runBaseVariables.end())
		wxLogWarning("Automation: SetVariable - '%s' is also a placeholder variable; those are substituted before the run and take precedence.", step.variableName);

	runtimeVariables[step.variableName] = step.variableValue;
	wxLogMessage("Automation: {{%s}} = '%s'.", step.variableName, step.variableValue);
	return 0;
}

int AutomationRunner::ExecuteStepLogMessage(const AutomationStep& step) {
	wxString message = wxString::FromUTF8(step.logMessageText);

	switch (step.logMessageLevel) {
		case 2: wxLogError("%s", message); break;
		case 1: wxLogWarning("%s", message); break;
		default: wxLogMessage("%s", message); break;
	}

	return 0;
}
