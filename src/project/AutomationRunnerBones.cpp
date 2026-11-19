/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: bones, bone weights and poses

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "outfitstudio/OutfitStudioFrame.h"
#include "outfitstudio/GLPanel.h"
#include "outfitstudio/ShapeTreeItems.h"
#include "components/UndoState.h"
#include "utils/StringStuff.h"

#include <NifFile.hpp>
#include <wx/msgdlg.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using namespace nifly;

int AutomationRunner::ExecuteStepCopyBoneWeights(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: CopyBoneWeights - no target shapes found.");
		return 0;
	}

	BoneWeightCopyOptions options;
	options.proximityRadius = step.weightProximityRadius;
	options.maxResults = step.weightMaxResults;
	options.selectedBones = step.weightBoneList;
	options.normalize = step.weightNormalize;

	UndoStateProject usp;
	OpResult result = ProjectOperations(*project, outfitStudio->editor).CopyBoneWeights(shapes, options, usp);
	if (!result.ok) {
		wxLogError("Automation: CopyBoneWeights - %s", result.error);
		return 1;
	}

	outfitStudio->ApplyRefresh(result.refresh);
	return 0;
}

int AutomationRunner::ExecuteStepDeleteBones(const AutomationStep& step) {
	if (step.deleteBoneNames.empty()) {
		wxLogError("Automation: DeleteBones - no bone names specified.");
		return 1;
	}

	if (step.deleteBoneFromProject) {
		for (const auto& boneName : step.deleteBoneNames)
			wxLogMessage("Automation: Deleting bone '%s' from project...", boneName);

		project->DeleteBones(step.deleteBoneNames);
	}
	else {
		auto shapes = ResolveTargetShapes(step);
		if (shapes.empty()) {
			wxLogWarning("Automation: DeleteBones - no target shapes found for weight removal.");
			return 0;
		}
		for (const auto& boneName : step.deleteBoneNames) {
			for (auto* shape : shapes) {
				wxLogMessage("Automation: Removing bone '%s' weights from '%s'...", boneName, shape->name.get());
				project->GetWorkAnim()->RemoveShapeBone(shape->name.get(), boneName);
			}
		}
	}
	return 0;
}

int AutomationRunner::ExecuteStepAddCustomBone(const AutomationStep& step) {
	if (step.addBoneName.empty()) {
		wxLogError("Automation: AddCustomBone - no bone name specified.");
		return 1;
	}

	nifly::MatTransform xform;
	xform.translation = nifly::Vector3(step.addBoneTransX, step.addBoneTransY, step.addBoneTransZ);
	nifly::Vector3 rotVec(step.addBoneRotX, step.addBoneRotY, step.addBoneRotZ);
	xform.rotation = nifly::RotVecToMat(rotVec);

	wxLogMessage("Automation: Adding custom bone '%s' (parent: '%s')...", step.addBoneName, step.addBoneParent);
	project->AddCustomBoneRef(step.addBoneName, step.addBoneParent, xform);
	return 0;
}

int AutomationRunner::ExecuteStepEditBone(const AutomationStep& step) {
	if (step.editBoneName.empty()) {
		wxLogError("Automation: EditBone - no bone name specified.");
		return 1;
	}

	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(step.editBoneName);
	if (!bPtr) {
		wxLogError("Automation: EditBone - bone '%s' not found.", step.editBoneName);
		return 1;
	}
	if (bPtr->isStandardBone) {
		wxLogError("Automation: EditBone - bone '%s' is a standard bone, cannot edit.", step.editBoneName);
		return 1;
	}

	nifly::MatTransform xform;
	xform.translation = nifly::Vector3(step.editBoneTransX, step.editBoneTransY, step.editBoneTransZ);
	nifly::Vector3 rotVec(step.editBoneRotX, step.editBoneRotY, step.editBoneRotZ);
	xform.rotation = nifly::RotVecToMat(rotVec);

	wxLogMessage("Automation: Editing custom bone '%s' (parent: '%s')...", step.editBoneName, step.editBoneParent);
	project->ModifyCustomBone(bPtr, step.editBoneParent, xform);
	return 0;
}

int AutomationRunner::ExecuteStepDuplicateBones(const AutomationStep& step) {
	if (step.dupBoneNames.empty()) {
		wxLogError("Automation: DuplicateBones - no bone names specified.");
		return 1;
	}

	std::string suffix = step.dupBoneSuffix;
	if (suffix.empty())
		suffix = project->NextFreeDuplicateSuffix(step.dupBoneNames, step.dupBoneFind, step.dupBoneReplace);

	std::optional<std::string> newParent;
	if (!step.dupBoneParent.empty())
		newParent = step.dupBoneParent;

	std::vector<std::string> newNames;
	std::string error;
	if (!project->DuplicateBones(step.dupBoneNames, step.dupBoneFind, step.dupBoneReplace, suffix, newParent, newNames, error)) {
		wxLogError("Automation: DuplicateBones - %s", error);
		return 1;
	}

	if (step.dupBoneMirror)
		project->MirrorBonesX(newNames);

	wxLogMessage("Automation: Duplicated %zu bone(s) as '%s'.", newNames.size(), JoinStrings(newNames, "', '"));
	return 0;
}

int AutomationRunner::ExecuteStepMirrorBones(const AutomationStep& step) {
	if (step.mirrorBoneNames.empty()) {
		wxLogError("Automation: MirrorBones - no bone names specified.");
		return 1;
	}

	if (project->MirrorBonesX(step.mirrorBoneNames).empty()) {
		wxLogError("Automation: MirrorBones - none of the bones is a custom bone.");
		return 1;
	}
	return 0;
}

int AutomationRunner::ExecuteStepRemoveSkinning(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: RemoveSkinning - no target shapes found.");
	}
	else {
		for (auto* shape : shapes) {
			wxLogMessage("Automation: Removing skinning from '%s'...", shape->name.get());
			project->RemoveSkinning(shape);
		}
		project->GetWorkNif()->DeleteUnreferencedNodes();
	}
	return 0;
}

int AutomationRunner::ExecuteStepApplyPose(const AutomationStep& step) {
	if (step.poseName.empty()) {
		wxLogError("Automation: ApplyPose - no pose name specified.");
		return 1;
	}

	// Find the pose by name
	PoseData* posePtr = nullptr;
	for (auto& pd : outfitStudio->poseDataCollection.poseData) {
		if (pd.name == step.poseName) {
			posePtr = &pd;
			break;
		}
	}
	if (!posePtr) {
		wxLogError("Automation: ApplyPose - pose '%s' not found.", step.poseName);
		return 1;
	}

	wxLogMessage("Automation: Applying pose '%s' to meshes...", step.poseName);

	// Set bone poses from PoseData
	std::vector<std::string> boneNames;
	AnimSkeleton::getInstance().GetActiveBoneNames(boneNames);
	for (const auto& boneName : boneNames) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		auto it = std::find_if(posePtr->boneData.begin(), posePtr->boneData.end(),
			[&](const PoseBoneData& pbd) { return pbd.name == boneName; });

		if (it != posePtr->boneData.end()) {
			bone->poseRotVec = it->rotation;
			bone->poseTranVec = it->translation;
			bone->poseScale = it->scale;
		}
		else {
			bone->poseRotVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
		}
		bone->UpdatePoseTransform();
	}

	// Enable pose and apply to mesh geometry
	project->bPose = true;

	UndoStateProject usp;
	project->ApplyPoseTransformsToAllShapeGeometry(usp);

	// Reset bone poses after applying
	for (const auto& boneName : boneNames) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;
		bone->poseRotVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
		bone->poseTranVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
		bone->poseScale = 1.0f;
		bone->UpdatePoseTransform();
	}

	outfitStudio->PoseToGUI();
	outfitStudio->glView->UpdateBones();
	outfitStudio->glView->ApplyUndoState(&usp, false);
	project->bPose = false;
	return 0;
}

int AutomationRunner::ExecuteStepFixBadBones(const AutomationStep& WXUNUSED(step)) {
	wxLogMessage("Automation: Fixing bad bones...");

	if (!project->CheckForBadBones())
		wxLogMessage("Automation: No bad bones found.");

	return 0;
}

int AutomationRunner::ExecuteStepAddBone(const AutomationStep& step) {
	if (step.addBoneRefNames.empty()) {
		wxLogError("Automation: AddBone - no bone name specified.");
		return 1;
	}

	int addedCount = 0;
	for (const auto& boneName : step.addBoneRefNames) {
		if (!AnimSkeleton::getInstance().GetBonePtr(boneName)) {
			wxLogWarning("Automation: AddBone - '%s' is not in the loaded skeleton, skipping.", boneName);
			continue;
		}

		wxLogMessage("Automation: Adding bone '%s' to the project...", boneName);
		project->AddBoneRef(boneName);
		addedCount++;
	}

	if (addedCount > 0)
		outfitStudio->SetPendingChanges();

	return 0;
}

int AutomationRunner::ExecuteStepTransferWeights(const AutomationStep& step) {
	auto* baseShape = project->GetBaseShape();
	if (!baseShape) {
		wxLogError("Automation: TransferWeights - there is no reference shape.");
		return 1;
	}

	// Empty targets exclude the reference: weights can't be transferred onto itself
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: TransferWeights - no target shapes found.");
		return 0;
	}

	int baseVertCount = project->GetVertexCount(baseShape);
	std::vector<std::string> boneList = step.transferWeightBones;

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		if (project->IsBaseShape(shape))
			continue;

		if (project->GetVertexCount(shape) != baseVertCount) {
			wxLogWarning("Automation: TransferWeights - '%s' has a different vertex count than the reference, skipping.", shapeName);
			continue;
		}

		// The mask is a per vertex blend factor here: fully masked vertices keep
		// the weights they already have.
		std::unordered_map<uint16_t, float> mask;
		if (step.transferWeightUseMask)
			outfitStudio->glView->GetShapeMask(mask, shapeName);

		wxLogMessage("Automation: Transferring bone weights to '%s'...", shapeName);
		project->TransferSelectedWeights(shape, mask.empty() ? nullptr : &mask, boneList.empty() ? nullptr : &boneList);
	}

	outfitStudio->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepSetBoneTransform(const AutomationStep& step) {
	if (step.boneXformNames.empty()) {
		wxLogError("Automation: SetBoneTransform - no bone name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SetBoneTransform - no target shapes found.");
		return 0;
	}

	auto* workAnim = project->GetWorkAnim();
	if (!workAnim) {
		wxLogError("Automation: SetBoneTransform - no project loaded.");
		return 1;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		for (const auto& boneName : step.boneXformNames) {
			if (step.boneXformMode == 0) {
				wxLogMessage("Automation: Recalculating the skin transform of bone '%s' for '%s'...", boneName, shapeName);
				workAnim->RecalcXFormSkinToBone(shapeName, boneName);
			}
			else {
				wxLogMessage("Automation: Recalculating the node transform of bone '%s' from the skin of '%s'...", boneName, shapeName);
				workAnim->RecalcCustomBoneXFormsFromSkin(shapeName, boneName);
			}
		}
	}

	outfitStudio->SetPendingChanges();
	return 0;
}
