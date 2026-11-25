/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: shapes, their geometry, transforms and partitions

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "ProjectOperations.h"
#include "StudioSession.h"
#include "BrushStrokeController.h"
#include "components/UndoHistory.h"
#include "components/UndoState.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <NifFile.hpp>
#include <wx/log.h>
#include <wx/msgdlg.h>

using namespace nifly;

namespace {
bool ParseAutomationPartitionID(const std::string& value, int& partitionID) {
	std::string trimmedValue = TrimString(value);
	if (trimmedValue.empty())
		return false;

	std::smatch match;
	// A slot number, alone or as the partition choices show it ("32 Body")
	static const std::regex partitionChoicePattern("^([0-9]+)(\\s+.+)?$");
	if (!std::regex_match(trimmedValue, match, partitionChoicePattern))
		return false;

	try {
		long parsedValue = std::stol(match[1].str());
		if (parsedValue < std::numeric_limits<int>::min() || parsedValue > std::numeric_limits<int>::max())
			return false;

		partitionID = static_cast<int>(parsedValue);
		return true;
	}
	catch (...) {
		return false;
	}
}
}

int AutomationRunner::ExecuteStepTransformShape(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: TransformShape - no target shapes found.");
		return 0;
	}

	nifly::Vector3 move(step.moveX, step.moveY, step.moveZ);
	nifly::Vector3 rotate(step.rotateX, step.rotateY, step.rotateZ);
	nifly::Vector3 scale(step.scaleX, step.scaleY, step.scaleZ);

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Transforming shape '%s'...", shape->name.get());
		if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f)
			project->OffsetShape(shape, move);
		if (rotate.x != 0.0f || rotate.y != 0.0f || rotate.z != 0.0f)
			project->RotateShape(shape, rotate);
		if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f)
			project->ScaleShape(shape, scale);
	}

	// Inflate: move vertices along their normals
	nifly::Vector3 inflate(step.inflateX, step.inflateY, step.inflateZ);
	if (inflate.x != 0.0f || inflate.y != 0.0f || inflate.z != 0.0f) {
		for (auto* shape : shapes) {
			wxLogMessage("Automation: Inflating shape '%s'...", shape->name.get());
			const std::vector<nifly::Vector3>* verts = project->GetWorkNif()->GetVertsForShape(shape);
			const std::vector<nifly::Vector3>* norms = project->GetWorkNif()->GetNormalsForShape(shape);
			if (!verts || !norms || verts->size() != norms->size())
				continue;

			std::vector<nifly::Vector3> newVerts = *verts;
			for (size_t i = 0; i < newVerts.size(); i++) {
				nifly::Vector3 diff = (*norms)[i].ComponentMultiply(inflate);
				newVerts[i] += diff;
			}
			project->GetWorkNif()->SetVertsForShape(shape, newVerts);
		}
	}

	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepInvertUVs(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: InvertUVs - no target shapes found.");
		return 0;
	}

	if (!step.invertU && !step.invertV) {
		wxLogWarning("Automation: InvertUVs - neither U nor V inversion selected.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Inverting UVs for '%s' (U=%d, V=%d)...", shape->name.get(), step.invertU, step.invertV);
		project->GetWorkNif()->InvertUVsForShape(shape, step.invertU, step.invertV);
	}
	return 0;
}

int AutomationRunner::ExecuteStepDeleteShape(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: DeleteShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Deleting shape '%s'...", shape->name.get());
		project->DeleteShape(shape);
	}
	return 0;
}

int AutomationRunner::ExecuteStepRenameShape(const AutomationStep& step) {
	if (step.renameOldName.empty() || step.renameNewName.empty()) {
		wxLogError("Automation: RenameShape - old or new name not specified.");
		return 1;
	}

	NiShape* shape = FindShapeByName(step.renameOldName);
	if (!shape) {
		if (step.renameSkipIfMissing) {
			wxLogMessage("Automation: RenameShape - shape '%s' not found, skipping.", step.renameOldName);
			return 0;
		}

		wxLogError("Automation: RenameShape - shape '%s' not found.", step.renameOldName);
		return 1;
	}

	if (project->activeSet.IsAddOn()) {
		wxLogError("Automation: RenameShape - the shapes of an add-on belong to its base project and can't be renamed.");
		return 1;
	}

	size_t disconnected = project->RenameDisconnectsFromBase("", step.renameOldName).size();
	if (disconnected > 0)
		wxLogMessage("Automation: RenameShape - %zu slider data entries of shape '%s' are taken from the base project and become this project's own.", disconnected, step.renameOldName);

	wxLogMessage("Automation: Renaming shape '%s' to '%s'...", step.renameOldName, step.renameNewName);
	project->RenameShape(shape, step.renameNewName);
	session->RenameMesh(step.renameOldName, step.renameNewName);
	return 0;
}

int AutomationRunner::ExecuteStepRefineMesh(const AutomationStep& step) {
	wxLogMessage("Automation: Refining meshes...");

	auto workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: RefineMesh - no work NIF loaded.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: RefineMesh - no target shapes found.");
		return 0;
	}

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = workNif->GetTriangleLimit();

	for (auto* shape : shapes) {
		size_t nverts = shape->GetNumVertices();

		// Determine unmasked vertices
		std::unordered_map<uint16_t, float> mask;
		session->GetShapeUnmasked(mask, shape->name.get());
		std::vector<bool> pincs(nverts, false);
		for (auto& m : mask)
			pincs[m.first] = true;

		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		size_t nedges = 0;
		for (Triangle& tri : tris) {
			int ntripts = pincs[tri.p1] + pincs[tri.p2] + pincs[tri.p3];
			if (ntripts == 3)
				nedges += 3;
			else if (ntripts == 2)
				nedges += 1;
		}

		if (nverts + nedges > maxVertIndex || shape->GetNumTriangles() + nedges > maxTriIndex) {
			wxLogWarning("Automation: RefineMesh - shape '%s' would exceed vertex/triangle limits, skipping.", shape->name.get());
			continue;
		}

		Mesh* m = session->GetMesh(shape->name.get());
		if (!m)
			continue;

		UndoStateShape uss;
		uss.shapeName = shape->name.get();
		if (!project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, false)) {
			wxLogWarning("Automation: RefineMesh - shape '%s' has orientation issues, skipping.", shape->name.get());
			continue;
		}

		std::vector<float> emptyMask;
		project->ApplyShapeMeshUndo(shape, emptyMask, uss, false);
	}

	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepApplyTransforms(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: ApplyTransforms - no target shapes found.");
		return 0;
	}

	int appliedCount = 0;
	for (auto* shape : shapes) {
		if (!project->ApplyShapeTransformToGeometry(shape))
			continue;

		wxLogMessage("Automation: Applied the transform of shape '%s' to its geometry.", shape->name.get());
		appliedCount++;
	}

	if (appliedCount == 0) {
		wxLogMessage("Automation: ApplyTransforms - none of the target shapes had a transform to apply.");
		return 0;
	}

	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepResetTransforms(const AutomationStep&) {
	wxLogMessage("Automation: Resetting transforms...");
	project->ResetTransforms();
	return 0;
}

int AutomationRunner::ExecuteStepDuplicateShape(const AutomationStep& step) {
	if (step.dupNewName.empty()) {
		wxLogError("Automation: DuplicateShape - no new name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: DuplicateShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		std::string newName = step.dupNewName;
		// If duplicating multiple shapes, append original name to avoid duplicates
		if (shapes.size() > 1)
			newName = step.dupNewName + "_" + shape->name.get();

		if (project->IsValidShape(newName)) {
			wxLogWarning("Automation: DuplicateShape - shape '%s' already exists, skipping.", newName);
			continue;
		}

		wxLogMessage("Automation: Duplicating shape '%s' as '%s'...", shape->name.get(), newName);
		project->DuplicateShape(shape, newName);
	}
	return 0;
}

int AutomationRunner::ExecuteStepChangePartitions(const AutomationStep& step) {
	int sourcePartitionID = 0;
	int destinationPartitionID = 0;
	if (!ParseAutomationPartitionID(step.partitionSource, sourcePartitionID)) {
		wxLogError("Automation: ChangePartitions - invalid source partition '%s', expected a slot number like '32' or '32 Body'.", step.partitionSource);
		return 1;
	}

	if (!ParseAutomationPartitionID(step.partitionDestination, destinationPartitionID)) {
		wxLogError("Automation: ChangePartitions - invalid destination partition '%s', expected a slot number like '52' or '52 Pelvis'.", step.partitionDestination);
		return 1;
	}

	if (sourcePartitionID == destinationPartitionID) {
		wxLogWarning("Automation: ChangePartitions - source and destination partitions are both %d; nothing to do.", sourcePartitionID);
		return 0;
	}

	NifFile* workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: ChangePartitions - no work NIF loaded.");
		return 1;
	}

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: ChangePartitions - no target shapes found.");
		return 0;
	}

	int updatedShapes = 0;
	int affectedTriangles = 0;
	int skippedShapes = 0;
	NiShape* activeShape = session->GetActiveShape();
	bool activeShapeUpdated = false;

	for (auto* shape : targetShapes) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		std::vector<int> trianglePartitions;
		if (!workNif->GetShapePartitions(shape, partitionInfo, trianglePartitions)) {
			wxLogWarning("Automation: ChangePartitions - shape '%s' has no partitions; skipping.", shape->name.get());
			skippedShapes++;
			continue;
		}

		std::vector<int> sourcePartitionIndices;
		int destinationPartitionIndex = -1;
		for (size_t partitionIndex = 0; partitionIndex < partitionInfo.size(); partitionIndex++) {
			int bodyPartID = partitionInfo[partitionIndex].partID;
			if (bodyPartID == sourcePartitionID)
				sourcePartitionIndices.push_back(static_cast<int>(partitionIndex));
			else if (bodyPartID == destinationPartitionID && destinationPartitionIndex < 0)
				destinationPartitionIndex = static_cast<int>(partitionIndex);
		}

		if (sourcePartitionIndices.empty()) {
			wxLogWarning("Automation: ChangePartitions - shape '%s' has no partition %d; skipping.", shape->name.get(), sourcePartitionID);
			skippedShapes++;
			continue;
		}

		auto isSourcePartitionIndex = [&sourcePartitionIndices](int partitionIndex) {
			return std::find(sourcePartitionIndices.begin(), sourcePartitionIndices.end(), partitionIndex) != sourcePartitionIndices.end();
		};

		int shapeAffectedTriangles = 0;
		for (int partitionIndex : trianglePartitions) {
			if (isSourcePartitionIndex(partitionIndex))
				shapeAffectedTriangles++;
		}

		bool changed = false;
		if (destinationPartitionIndex < 0) {
			destinationPartitionIndex = sourcePartitionIndices.front();
			partitionInfo[destinationPartitionIndex].partID = destinationPartitionID;
			changed = true;
		}

		for (int sourcePartitionIndex : sourcePartitionIndices) {
			if (sourcePartitionIndex == destinationPartitionIndex)
				continue;

			changed = true;
			for (int& trianglePartition : trianglePartitions) {
				if (trianglePartition == sourcePartitionIndex)
					trianglePartition = destinationPartitionIndex;
			}
		}

		if (!changed)
			continue;

		workNif->SetShapePartitions(shape, partitionInfo, trianglePartitions);
		workNif->RemoveEmptyPartitions(shape);
		session->MeshFromProj(shape, true);
		if (shape == activeShape)
			activeShapeUpdated = true;

		updatedShapes++;
		affectedTriangles += shapeAffectedTriangles;
		wxLogMessage("Automation: ChangePartitions - changed %d triangle(s) on '%s' from partition %d to %d.",
			shapeAffectedTriangles,
			shape->name.get(),
			sourcePartitionID,
			destinationPartitionID);
	}

	if (updatedShapes > 0) {
		if (activeShapeUpdated && session->GetActiveShape() == activeShape)
			session->RefreshPartitionTree();

		session->SetPendingChanges();
		session->Render();
		wxLogMessage("Automation: ChangePartitions - updated %d shape(s), changed %d triangle assignment(s), skipped %d shape(s).",
			updatedShapes,
			affectedTriangles,
			skippedShapes);
	}
	else {
		wxLogWarning("Automation: ChangePartitions - no matching partition assignments were changed.");
	}

	return 0;
}

int AutomationRunner::ExecuteStepMirrorShape(const AutomationStep& step) {
	if (!step.mirrorX && !step.mirrorY && !step.mirrorZ) {
		wxLogWarning("Automation: MirrorShape - no mirror axis selected.");
		return 0;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MirrorShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Mirroring shape '%s' (X=%d, Y=%d, Z=%d, SwapBones=%d)...",
			shape->name.get(), step.mirrorX, step.mirrorY, step.mirrorZ, step.mirrorSwapBonesX);
		project->GetWorkNif()->MirrorShape(shape, step.mirrorX, step.mirrorY, step.mirrorZ);
		if (step.mirrorSwapBonesX)
			project->GetWorkAnim()->SwapBonesLR(shape->name.get());
	}
	return 0;
}

int AutomationRunner::ExecuteStepRecalcNormals(const AutomationStep& step) {
	auto* workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: RecalcNormals - no work NIF loaded.");
		return 1;
	}

	// Empty targets include the reference shape on purpose: recalculating the
	// reference's normals is legitimate and the project save skips it otherwise.
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: RecalcNormals - no target shapes found.");
		return 0;
	}

	int recalcCount = 0;

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();

		// Persist the settings into the slider set before recalculating. The render
		// meshes are rebuilt from the project after every step, so setting these on
		// the mesh alone would not stick.
		if (step.normalsSeamSmooth >= 0)
			project->activeSet.SetSmoothSeamNormals(shapeName, step.normalsSeamSmooth != 0);

		if (step.normalsSeamAngle >= 0.0f)
			project->activeSet.SetSmoothSeamNormalsAngle(shapeName, step.normalsSeamAngle);

		bool smooth = project->activeSet.GetSmoothSeamNormals(shapeName);
		float angle = project->activeSet.GetSmoothSeamNormalsAngle(shapeName);

		if (project->activeSet.GetLockNormals(shapeName) && !step.normalsForce) {
			wxLogMessage("Automation: RecalcNormals - '%s' has locked normals, skipping.", shapeName);
		}
		else {
			wxLogMessage("Automation: Recalculating normals for '%s' (seams=%d, angle=%0.2f)...", shapeName, smooth, angle);

			// Honors LOCKEDNORM vertices; force also overrides the model space shader skip on SK/SSE
			workNif->CalcNormalsForShape(shape, step.normalsForce, smooth, angle);
			workNif->CalcTangentsForShape(shape);
			recalcCount++;
		}

		// Applied after the recalculation so that force + lock is a usable combination
		if (step.normalsLock >= 0)
			project->activeSet.SetLockNormals(shapeName, step.normalsLock != 0);
	}

	if (recalcCount > 0)
		session->SetPendingChanges();

	wxLogMessage("Automation: RecalcNormals - recalculated normals on %d shape(s).", recalcCount);
	return 0;
}

int AutomationRunner::ExecuteStepRemoveUnusedNodes(const AutomationStep&) {
	wxLogMessage("Automation: Removing unused nodes...");
	int deletionCount = 0;
	auto workNif = project->GetWorkNif();
	if (workNif)
		workNif->DeleteUnreferencedNodes(&deletionCount);
	wxLogMessage("Automation: %d unreferenced nodes removed.", deletionCount);
	return 0;
}

int AutomationRunner::ExecuteStepCopySegPart(const AutomationStep& step) {
	if (!project->GetBaseShape()) {
		wxLogError("Automation: CopySegPart - there is no reference shape.");
		return 1;
	}

	// Empty targets exclude the reference: it is the source of the copy
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: CopySegPart - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		if (project->IsBaseShape(shape))
			continue;

		wxLogMessage("Automation: Copying segments/partitions to '%s'...", shape->name.get());
		int failCount = project->CopySegPart(shape);
		if (failCount)
			wxLogWarning("Automation: CopySegPart - %d triangle(s) of '%s' could not be matched.", failCount, shape->name.get());
	}

	session->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepDeleteVertices(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: DeleteVertices - no target shapes found.");
		return 0;
	}

	// Prepare everything first: PrepareDeleteVerts reports whole shapes that would
	// lose all of their triangles instead of filling in an undo state.
	std::vector<std::string> emptiedShapes;
	std::vector<UndoStateShape> usss;

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();

		std::unordered_map<uint16_t, float> verts;
		if (step.deleteVertsMasked)
			session->GetShapeMask(verts, shapeName);
		else
			session->GetShapeUnmasked(verts, shapeName);

		if (verts.empty()) {
			wxLogWarning("Automation: DeleteVertices - '%s' has no %s vertices, skipping.",
				shapeName, step.deleteVertsMasked ? "masked" : "unmasked");
			continue;
		}

		UndoStateShape uss;
		uss.shapeName = shapeName;
		if (project->PrepareDeleteVerts(shape, verts, uss))
			emptiedShapes.push_back(shapeName);
		else
			usss.push_back(std::move(uss));
	}

	if (usss.empty() && emptiedShapes.empty())
		return 0;

	std::unordered_map<std::string, std::vector<float>> maskStash = session->StashMasks();

	// Whole shapes go first and everything is looked up by name again, so that
	// deleting a shape can't leave a stale pointer behind.
	for (const auto& shapeName : emptiedShapes) {
		if (!step.deleteVertsDeleteEmpty) {
			wxLogWarning("Automation: DeleteVertices - '%s' would lose all of its triangles, skipping.", shapeName);
			continue;
		}

		NiShape* shape = project->GetWorkNif()->FindBlockByName<NiShape>(shapeName);
		if (!shape)
			continue;

		wxLogMessage("Automation: Deleting shape '%s', none of its triangles are left...", shapeName);
		project->DeleteShape(shape);
	}

	for (auto& uss : usss) {
		NiShape* shape = project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;

		wxLogMessage("Automation: Deleting the %s vertices of '%s'...", step.deleteVertsMasked ? "masked" : "unmasked", uss.shapeName);
		project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, false);
	}

	project->GetWorkAnim()->CleanupBones();

	RefreshMeshesWithMasks(maskStash);
	session->SetPendingChanges();
	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepSeparateVertices(const AutomationStep& step) {
	if (step.separateNewName.empty()) {
		wxLogError("Automation: SeparateVertices - no name for the new shape specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SeparateVertices - no target shapes found.");
		return 0;
	}

	if (shapes.size() > 1) {
		wxLogError("Automation: SeparateVertices - %zu target shapes; the new shape name only fits one.", shapes.size());
		return 1;
	}

	if (project->IsValidShape(step.separateNewName)) {
		wxLogError("Automation: SeparateVertices - shape '%s' already exists.", step.separateNewName);
		return 1;
	}

	NiShape* shape = shapes.front();
	std::string shapeName = shape->name.get();

	std::unordered_map<uint16_t, float> masked;
	session->GetShapeMask(masked, shapeName);
	if (masked.empty()) {
		wxLogError("Automation: SeparateVertices - '%s' has no masked vertices to separate.", shapeName);
		return 1;
	}

	wxLogMessage("Automation: Separating the masked vertices of '%s' into '%s'...", shapeName, step.separateNewName);

	NiShape* newShape = project->DuplicateShape(shape, step.separateNewName);
	if (!newShape) {
		wxLogError("Automation: SeparateVertices - failed to duplicate '%s'.", shapeName);
		return 1;
	}

	// The masked vertices go to the new shape, everything else stays behind
	std::unordered_map<uint16_t, float> unmasked = masked;
	session->InvertMaskTris(unmasked, shapeName);

	UndoStateShape ussSource;
	ussSource.shapeName = shapeName;
	UndoStateShape ussNew;
	ussNew.shapeName = step.separateNewName;

	project->PrepareDeleteVerts(shape, masked, ussSource);
	project->PrepareDeleteVerts(newShape, unmasked, ussNew);

	std::unordered_map<std::string, std::vector<float>> maskStash = session->StashMasks();

	project->ApplyShapeMeshUndo(shape, maskStash[ussSource.shapeName], ussSource, false);
	project->ApplyShapeMeshUndo(newShape, maskStash[ussNew.shapeName], ussNew, false);

	project->SetTextures();

	RefreshMeshesWithMasks(maskStash);
	session->SetPendingChanges();
	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepMergeGeometry(const AutomationStep& step) {
	if (step.mergeSourceShape.empty() || step.mergeTargetShape.empty()) {
		wxLogError("Automation: MergeGeometry - both a source and a target shape are required.");
		return 1;
	}

	NiShape* sourceShape = FindShapeByName(step.mergeSourceShape);
	if (!sourceShape) {
		if (step.mergeSkipIfMissing) {
			wxLogMessage("Automation: MergeGeometry - source shape '%s' not found, skipping.", step.mergeSourceShape);
			return 0;
		}

		wxLogError("Automation: MergeGeometry - source shape '%s' not found.", step.mergeSourceShape);
		return 1;
	}

	NiShape* targetShape = FindShapeByName(step.mergeTargetShape);
	if (!targetShape) {
		if (step.mergeSkipIfMissing) {
			wxLogMessage("Automation: MergeGeometry - target shape '%s' not found, skipping.", step.mergeTargetShape);
			return 0;
		}

		wxLogError("Automation: MergeGeometry - target shape '%s' not found.", step.mergeTargetShape);
		return 1;
	}

	MergeCheckErrors e;
	project->CheckMerge(step.mergeSourceShape, step.mergeTargetShape, e);

	if (!e.canMerge) {
		if (e.shapesSame)
			wxLogError("Automation: MergeGeometry - the target has to be different from the source.");
		if (e.tooManyVertices)
			wxLogError("Automation: MergeGeometry - the resulting shape would have too many vertices.");
		if (e.tooManyTriangles)
			wxLogError("Automation: MergeGeometry - the resulting shape would have too many triangles.");
		if (e.shaderMismatch)
			wxLogError("Automation: MergeGeometry - the shaders of '%s' and '%s' do not match.", step.mergeSourceShape, step.mergeTargetShape);
		if (e.alphaPropMismatch)
			wxLogError("Automation: MergeGeometry - the alpha properties of '%s' and '%s' do not match.", step.mergeSourceShape, step.mergeTargetShape);
		return 1;
	}

	if (e.partitionsMismatch)
		wxLogWarning("Automation: MergeGeometry - partitions do not match; matching slots are reconciled and missing ones created.");
	if (e.segmentsMismatch)
		wxLogWarning("Automation: MergeGeometry - segments do not match; matching IDs are reconciled and missing ones created.");
	if (e.textureMismatch)
		wxLogWarning("Automation: MergeGeometry - base textures do not match; the texture paths of '%s' are kept.", step.mergeTargetShape);
	if (e.transformsMismatch)
		wxLogWarning("Automation: MergeGeometry - transforms do not match; they are applied to the geometry of both shapes and cleared.");

	wxLogMessage("Automation: Merging '%s' into '%s'...", step.mergeSourceShape, step.mergeTargetShape);

	UndoStateShape uss;
	uss.shapeName = step.mergeTargetShape;
	project->PrepareCopyGeo(sourceShape, targetShape, uss);

	std::unordered_map<std::string, std::vector<float>> maskStash = session->StashMasks();

	project->ApplyShapeMeshUndo(targetShape, maskStash[uss.shapeName], uss, false);

	if (e.textureMismatch)
		project->SetTextures(targetShape);

	if (step.mergeDeleteSource) {
		wxLogMessage("Automation: Deleting the merged source shape '%s'...", step.mergeSourceShape);
		project->DeleteShape(sourceShape);
	}

	RefreshMeshesWithMasks(maskStash);
	session->SetPendingChanges();
	session->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepSymmetrizeVertices(const AutomationStep& step) {
	if (!step.asymDoPositions && !step.asymDoUnmatched && !step.asymDoSliders && !step.asymDoBones) {
		wxLogWarning("Automation: SymmetrizeVertices - nothing selected to symmetrize.");
		return 0;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SymmetrizeVertices - no target shapes found.");
		return 0;
	}

	std::vector<std::string> normBones, notNormBones;
	session->GetNormalizeBones(&normBones, &notNormBones);

	UndoStateProject* usp = session->Editor().undoHistory.PushState();
	usp->undoType = UndoType::Mesh;

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		// Masked vertices are left alone, matching the vertex asymmetry dialog
		std::vector<bool> selVerts(m->nVerts);
		for (int i = 0; i < m->nVerts; i++)
			selVerts[i] = m->mask[i] == 0.0f;

		SymmetricVertices symVerts;
		project->MatchSymmetricVertices(shape, m->weldVerts, symVerts);

		VertexAsymmetries asyms;
		project->FindVertexAsymmetries(shape, symVerts, m->weldVerts, asyms);

		VertexAsymmetryTasks tasks;
		tasks.doPos = step.asymDoPositions;
		tasks.doUnmatched = step.asymDoUnmatched;
		tasks.doSliders.resize(asyms.sliders.size(), step.asymDoSliders);
		tasks.doBones.resize(asyms.bones.size(), step.asymDoBones);

		wxLogMessage("Automation: Symmetrizing the vertices of '%s' (%zu unmatched, %zu slider(s), %zu bone(s))...",
			shapeName, symVerts.unmatched.size(), asyms.sliders.size(), asyms.bones.size());

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = shapeName;
		project->PrepareSymmetrizeVertices(shape, uss, symVerts, asyms, tasks, m->weldVerts, selVerts, normBones, notNormBones);
	}

	if (usp->usss.empty()) {
		session->Editor().undoHistory.PopState();
		wxLogWarning("Automation: SymmetrizeVertices - none of the target shapes have a mesh to work on.");
		return 0;
	}

	session->ApplyUndoState(usp, false);
	session->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepSmoothVertices(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SmoothVertices - no target shapes found.");
		return 0;
	}

	std::vector<Mesh*> meshes;
	for (auto* shape : shapes)
		if (Mesh* m = session->GetMesh(shape->name.get()))
			meshes.push_back(m);
	if (meshes.empty()) {
		wxLogWarning("Automation: SmoothVertices - none of the target shapes have a mesh to work on.");
		return 0;
	}

	// Shapes lying on the smoothed ones, by name or by pattern like the target shapes
	std::vector<Mesh*> followers;
	auto addFollower = [&](NiShape* shape) {
		Mesh* m = shape ? session->GetMesh(shape->name.get()) : nullptr;
		if (m && std::find(meshes.begin(), meshes.end(), m) == meshes.end() && std::find(followers.begin(), followers.end(), m) == followers.end())
			followers.push_back(m);
	};
	for (const auto& name : step.smoothFollowShapes) {
		if (step.targetRegex) {
			try {
				std::regex re(name, std::regex::icase);
				for (auto* shape : project->GetWorkNif()->GetShapes())
					if (std::regex_search(shape->name.get(), re))
						addFollower(shape);
			}
			catch (const std::regex_error& e) {
				wxLogWarning("Automation: SmoothVertices - invalid follow shape regex '%s' (%s).", name, e.what());
			}
		}
		else
			addFollower(FindShapeByName(name));
	}

	wxLogMessage("Automation: Smoothing %zu shape(s) %d time(s) at strength %.3f, %zu shape(s) following...", meshes.size(), step.smoothIterations,
				 step.smoothStrength, followers.size());

	BrushStrokeController controller(*project, session->Editor());
	RefreshFlags refresh = controller.SmoothMeshes(meshes, step.smoothIterations, step.smoothStrength, step.smoothKeepBorders);
	session->ApplyRefresh(refresh);

	if (!followers.empty()) {
		UndoHistory* history = &session->Editor().undoHistory;
		auto moves = BrushStrokeController::FollowEdit(*history->GetCurState(), meshes, followers, Mesh::TransformDistNifToMesh(step.smoothFollowRadius));
		if (!moves.empty()) {
			UndoStateProject* usp = history->PushState();
			usp->undoType = UndoType::VertexPosition;
			usp->usss = std::move(moves);
			if (session->Editor().bEditSlider) {
				usp->sliderName = session->Editor().activeSlider;
				float sliderscale = project->SliderValue(usp->sliderName);
				usp->sliderscale = sliderscale == 0.0f ? 1.0f : sliderscale;
			}
			session->ApplyUndoState(usp, false);
		}
	}

	session->SetPendingChanges();
	session->Render();
	return 0;
}
