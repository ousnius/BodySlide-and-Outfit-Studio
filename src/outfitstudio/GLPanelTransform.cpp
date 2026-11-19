/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: the transform tool, the pivot and the rotation center

#include "GLPanel.h"
#include "OutfitStudioFrame.h"
#include "components/TweakBrush.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
	if (!hit)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.center = xformCenter;
	xformCenterInitial = xformCenter;

	std::string mname = hitMesh->shapeName;
	if (mname.find("Move") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
			case 'X':
				tpi.view = Vector3(1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Y':
				tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Z':
				tpi.view = Vector3(0.0f, 0.0f, 1.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
		}
	}
	else if (mname.find("Rotate") != std::string::npos) {
		translateBrush.SetXFormType(1);
		switch (mname[0]) {
			case 'X':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
				//tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
			case 'Y':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
				//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 1.0f, 0.0f);
				break;
			case 'Z':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
				//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
		}
	}
	else if (mname.find("Scale") != std::string::npos) {
		if (mname.find("Uniform") == std::string::npos) {
			translateBrush.SetXFormType(2);
			switch (mname[0]) {
				case 'X':
					tpi.view = Vector3(1.0f, 0.0f, 0.0f);
					tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
					break;
				case 'Y':
					tpi.view = Vector3(0.0f, 1.0f, 0.0f);
					tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
					break;
				case 'Z':
					tpi.view = Vector3(0.0f, 0.0f, 1.0f);
					tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
					break;
			}
		}
		else {
			translateBrush.SetXFormType(3);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	else
		return false;

	if (!nodesMode && !bonesMode) {
		activeStroke = std::make_unique<TweakStroke>(gls.GetActiveMeshes(), &translateBrush, *os->editor.undoHistory.PushState());

		if (os->editor.bEditSlider) {
			activeStroke->usp.sliderName = os->editor.activeSlider;

			float sliderscale = os->project->SliderValue(os->editor.activeSlider);
			if (sliderscale == 0.0)
				sliderscale = 1.0;

			activeStroke->usp.sliderscale = sliderscale;
		}

		activeStroke->beginStroke(tpi);
	}
	else {
		// Bones/nodes-mode transform: edit the active bone/node directly.
		// The TweakStroke machinery isn't used here because we don't need
		// to modify any mesh vertices through a brush -- we change the
		// bone's local transform and let skinning update the meshes.
		const std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;

		// Only translate and rotate are supported for bones/nodes.
		if (mname.find("Move") != std::string::npos)
			boneXformType = 0;
		else if (mname.find("Rotate") != std::string::npos)
			boneXformType = 1;
		else
			return false;

		boneXformPickStart = tpi.origin;
		boneXformPlaneNormalModel = tpi.normal;
		boneXformAxisModel = tpi.view;
		boneXformPlaneDist = tpi.origin.dot(tpi.normal);

		// Pose mode: edit poseTranVec/poseRotVec only (no NIF writes, no
		// undo).  Only AnimBones have pose data; plain NIF nodes do not.
		boneXformPoseMode = false;
		AnimBone* poseBPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (os->project->bPose && poseBPtr) {
			boneXformPoseMode = true;
			xformInitialLocalToParent = poseBPtr->xformToParent;
			// M = parent_pose_to_global ∘ xformToParent
			MatTransform parentPoseToGlobal;
			if (poseBPtr->parent)
				parentPoseToGlobal = poseBPtr->parent->xformPoseToGlobal;
			xformInitialParentToGlobal = parentPoseToGlobal.ComposeTransforms(poseBPtr->xformToParent);
			boneXformPoseInitialTran = poseBPtr->poseTranVec;
			boneXformPoseInitialRot = poseBPtr->poseRotVec;
		}
		else {
			// Capture the initial transforms of the active bone/node.
			xformInitialLocalToParent = MatTransform();
			xformInitialParentToGlobal = MatTransform();

			const AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
			if (bPtr) {
				xformInitialLocalToParent = bPtr->xformToParent;
				if (bPtr->parent)
					xformInitialParentToGlobal = bPtr->parent->xformToGlobal;
			}
			else if (auto* nif = os->project->GetWorkNif()) {
				if (NiNode* node = nif->FindBlockByName<NiNode>(activeBone)) {
					xformInitialLocalToParent = node->GetTransformToParent();
					// Compose the transforms of all ancestor nodes to build
					// parent-to-global.
					NiNode* p = nif->GetParentNode(node);
					while (p) {
						xformInitialParentToGlobal = p->GetTransformToParent().ComposeTransforms(xformInitialParentToGlobal);
						p = nif->GetParentNode(p);
					}
				}
				else {
					// No AnimBone and no NiNode -- nothing to edit.
					return false;
				}
			}
			else
				return false;

			// Push an undo state.  nodeEndXformToParent is filled in in
			// EndTransform; until then it equals nodeStartXformToParent.
			UndoStateProject* usp = os->editor.undoHistory.PushState();
			usp->undoType = UndoType::NodeTransform;
			usp->boneName = activeBone;
			usp->nodeStartXformToParent = xformInitialLocalToParent;
			usp->nodeEndXformToParent = xformInitialLocalToParent;
		}
	}

	XMoveMesh->bVisible = false;
	YMoveMesh->bVisible = false;
	ZMoveMesh->bVisible = false;
	XRotateMesh->bVisible = false;
	YRotateMesh->bVisible = false;
	ZRotateMesh->bVisible = false;
	XScaleMesh->bVisible = false;
	YScaleMesh->bVisible = false;
	ZScaleMesh->bVisible = false;
	ScaleUniformMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdateTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	if (!nodesMode && !bonesMode) {
		translateBrush.GetWorkingPlane(pn, pd);
		gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		activeStroke->updateStroke(tpi);
		ShowTransformTool();
		return;
	}

	// Bones/nodes-mode transform: compute the bone/node's new local transform
	// directly from the drag and apply it.  Meshes update live via skinning.
	const std::string activeBone = os->GetActiveBone();
	if (activeBone.empty()) {
		ShowTransformTool();
		return;
	}

	// Intersect the mouse ray with the pick plane captured in StartTransform.
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, boneXformPlaneNormalModel, boneXformPlaneDist);

	if (boneXformPoseMode) {
		AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (!bPtr) {
			ShowTransformTool();
			return;
		}

		// M = parent_pose_to_global ∘ xformToParent  (captured in
		// xformInitialParentToGlobal).  Edits go into poseTranVec/poseRotVec
		// which are applied in the bone's own CS before xformToParent.
		if (boneXformType == 0) {
			const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
			const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
			const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
			const Vector3 localOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
			bPtr->poseTranVec = boneXformPoseInitialTran + localOffset;
		}
		else if (boneXformType == 1) {
			const Vector3 a = boneXformPickStart - xformCenterInitial;
			const Vector3 b = tpi.origin - xformCenterInitial;
			if (a.length() > EPSILON && b.length() > EPSILON) {
				const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
				const float cosA = a.dot(b);
				const float angle = std::atan2(sinA, cosA);

				Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
				Vector3 axisLocal = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
				const float axisLen = axisLocal.length();
				if (axisLen > EPSILON) {
					axisLocal /= axisLen;
					const Matrix3 rot = RotVecToMat(axisLocal * angle);
					const Matrix3 newPoseRot = rot * RotVecToMat(boneXformPoseInitialRot);
					bPtr->poseRotVec = RotMatToVec(newPoseRot);
				}
			}
		}

		bPtr->UpdatePoseTransform();

		// Live mesh update to show the pose change immediately.
		if (auto* nif = os->project->GetWorkNif()) {
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		// Keep the pose-sliders GUI in sync with the new values.
		os->PoseToGUI();

		UpdateBones();
		UpdateNodes();
		ShowTransformTool();
		return;
	}

	MatTransform newToParent = xformInitialLocalToParent;

	if (boneXformType == 0) {
		// Translate along the gizmo's axis.  Project the drag vector onto
		// the axis in model space, convert to nif distance, then map the
		// resulting global offset into parent space (so scale/rotation of
		// ancestor nodes is handled correctly).
		const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
		const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
		const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
		const Vector3 parentOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
		newToParent.translation = xformInitialLocalToParent.translation + parentOffset;
	}
	else if (boneXformType == 1) {
		// Rotate about the gizmo ring's axis.  Angle is the signed angle
		// between the initial and current pick vectors in the rotation plane.
		const Vector3 a = boneXformPickStart - xformCenterInitial;
		const Vector3 b = tpi.origin - xformCenterInitial;
		if (a.length() > EPSILON && b.length() > EPSILON) {
			const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
			const float cosA = a.dot(b);
			const float angle = std::atan2(sinA, cosA);

			// Convert axis from model to global, then to parent.
			Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
			Vector3 axisParent = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
			const float axisLen = axisParent.length();
			if (axisLen > EPSILON) {
				axisParent /= axisLen;
				const Matrix3 rot = RotVecToMat(axisParent * angle);
				newToParent.rotation = rot * xformInitialLocalToParent.rotation;
			}
		}
	}

	// Apply the new transform to the AnimBone (if any) and to the NIF node.
	// We deliberately do not recalculate xformSkinToBone: that way the mesh
	// follows the bone through skinning, the active pose updates live, and
	// the "check for bad bones" residual correctly flags the new mismatch.
	auto* nif = os->project->GetWorkNif();
	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
	if (bPtr)
		bPtr->SetTransformBoneToParent(newToParent);
	if (nif)
		nif->SetNodeTransformToParent(activeBone, newToParent);

	// Record the end state on the open undo entry so an in-progress drag is
	// already undo-able if the stroke is canceled.
	if (UndoStateProject* usp = os->editor.undoHistory.GetCurState()) {
		if (usp->undoType == UndoType::NodeTransform && usp->boneName == activeBone)
			usp->nodeEndXformToParent = newToParent;
	}

	// Live mesh update (so skinned vertices follow the bone).
	if (nif) {
		for (auto& s : nif->GetShapes()) {
			std::vector<Vector3> verts;
			os->project->GetLiveVerts(s, verts);
			UpdateMeshVertices(s->name.get(), &verts, true, true, false);
		}
	}

	UpdateBones();
	UpdateNodes();
	ShowTransformTool();
}

void wxGLPanel::EndTransform() {
	if (!nodesMode && !bonesMode) {
		activeStroke->endStroke();
		activeStroke = nullptr;

		os->ActiveShapesUpdated(os->editor.undoHistory.GetCurState());
		if (!os->editor.bEditSlider) {
			{
				UndoStateProject* usp = os->editor.undoHistory.GetCurState();
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						os->project->ComputeUndoRestDiffs(shape, uss);
				}
			}

			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				os->UpdateShapeSource(s);
				os->project->RefreshMorphShape(s);
			}

			if (os->project->bPose) {
				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(s, verts);
					UpdateMeshVertices(s->name.get(), &verts, true, true, false);
				}
			}
		}
		else if (os->project->bPose) {
			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}
	}
	else {
		// Bones/nodes-mode: sync the AnimInfo's bone transforms out to the
		// NIF, refresh the bone-tree bad-bone icons, and mark the project
		// as modified.  In pose mode we only edited poseTranVec/poseRotVec
		// on the AnimBone -- no NIF writes, no undo, no pending changes.
		if (!boneXformPoseMode) {
			if (auto* nif = os->project->GetWorkNif()) {
				if (auto* workAnim = os->project->GetWorkAnim())
					workAnim->WriteNodesToNif(nif);
			}
			os->RefreshBoneTreeBadBoneIcons();
			os->SetPendingChanges();
		}
		boneXformPoseMode = false;
	}

	ShowTransformTool();
	os->UpdateUndoTools();
}

bool wxGLPanel::StartPivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
	if (!hit)
		return false;

	tpi.center = pivotPosition;

	std::string mname = hitMesh->shapeName;
	if (mname.find("PivotMesh") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
			case 'X':
				tpi.view = Vector3(1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Y':
				tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Z':
				tpi.view = Vector3(0.0f, 0.0f, 1.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
		}
	}
	else
		return false;

	std::vector<Mesh*> strokeMeshes{hitMesh};
	activeStroke = std::make_unique<TweakStroke>(strokeMeshes, &translateBrush, *os->editor.undoHistory.PushState());
	activeStroke->beginStroke(tpi);

	XPivotMesh->bVisible = false;
	YPivotMesh->bVisible = false;
	ZPivotMesh->bVisible = false;
	PivotCenterMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdatePivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	translateBrush.GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);

	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndPivotPosition() {
	activeStroke->endStroke();

	std::vector<Mesh*> refMeshes = activeStroke->GetRefMeshes();
	if (refMeshes.size() > 0) {
		UndoStateShape& uss = activeStroke->usp.usss[0];
		if (uss.pointStartState.size() > 0 && uss.pointEndState.size() > 0) {
			Vector3& pivotStartStatePos = uss.pointStartState[0];
			Vector3& pivotEndStatePos = uss.pointEndState[0];
			Vector3 pivotDiff = pivotEndStatePos - pivotStartStatePos;
			pivotPosition += pivotDiff;
		}
	}

	activeStroke = nullptr;
	ShowPivot();
}

void wxGLPanel::ShowRotationCenter(bool show) {
	if (show) {
		RotationCenterMesh = gls.AddVis3dSphere(gls.camRotOffset, 0.08f, Vector3(1.0f, 0.0f, 1.0f), "RotationCenterMesh");
		RotationCenterMesh->prop.alpha = 0.25f;
		RotationCenterMesh->bVisible = true;

		RotationCenterMeshRingX = gls.AddVis3dRing(gls.camRotOffset, Vector3(1.0f, 0.0f, 0.0f), 0.25f, 0.01f, Vector3(1.0f, 0.0f, 0.0f), "RotationCenterMeshRingX");
		RotationCenterMeshRingX->prop.alpha = 0.25f;
		RotationCenterMeshRingX->bVisible = true;

		RotationCenterMeshRingY = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 1.0f, 0.0f), 0.25f, 0.01f, Vector3(0.0f, 1.0f, 0.0f), "RotationCenterMeshRingY");
		RotationCenterMeshRingY->prop.alpha = 0.25f;
		RotationCenterMeshRingY->bVisible = true;

		RotationCenterMeshRingZ = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 0.0f, 1.0f), 0.25f, 0.01f, Vector3(0.0f, 0.0f, 1.0f), "RotationCenterMeshRingZ");
		RotationCenterMeshRingZ->prop.alpha = 0.25f;
		RotationCenterMeshRingZ->bVisible = true;
	}
	else {
		if (RotationCenterMesh) {
			RotationCenterMesh->bVisible = false;
			RotationCenterMeshRingX->bVisible = false;
			RotationCenterMeshRingY->bVisible = false;
			RotationCenterMeshRingZ->bVisible = false;
		}
	}
}

void wxGLPanel::ShowTransformTool(bool show) {
	UpdateTransformCenter();

	if (show) {
		XMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		XMoveMesh->prop.alpha = 0.6f;
		YMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		YMoveMesh->prop.alpha = 0.6f;
		ZMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");
		ZMoveMesh->prop.alpha = 0.6f;

		XRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 1.25f, 0.04f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		XRotateMesh->prop.alpha = 0.6f;
		YRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 1.25f, 0.04f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		YRotateMesh->prop.alpha = 0.6f;
		ZRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 1.25f, 0.04f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");
		ZRotateMesh->prop.alpha = 0.6f;

		XScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.75f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), 0.12f, Vector3(1.0f, 0.0f, 0.0f), "XScaleMesh");
		YScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.75f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.12f, Vector3(0.0f, 1.0f, 0.0f), "YScaleMesh");
		ZScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.0f, 0.75f), Vector3(0.0f, 0.0f, 1.0f), 0.12f, Vector3(0.0f, 0.0f, 1.0f), "ZScaleMesh");
		ScaleUniformMesh = gls.AddVis3dCube(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.15f, Vector3(0.0f, 0.0f, 0.0f), "ScaleUniformMesh");

		lastCenterDistance = 0.0f;

		XMoveMesh->bVisible = true;
		YMoveMesh->bVisible = true;
		ZMoveMesh->bVisible = true;
		XRotateMesh->bVisible = true;
		YRotateMesh->bVisible = true;
		ZRotateMesh->bVisible = true;
		XScaleMesh->bVisible = true;
		YScaleMesh->bVisible = true;
		ZScaleMesh->bVisible = true;
		ScaleUniformMesh->bVisible = true;

		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}

		if (nodesMode || bonesMode) {
			// Only move and rotate gizmos are supported for bones/nodes.
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}
	}
	else {
		if (XMoveMesh) {
			XMoveMesh->bVisible = false;
			YMoveMesh->bVisible = false;
			ZMoveMesh->bVisible = false;
			XRotateMesh->bVisible = false;
			YRotateMesh->bVisible = false;
			ZRotateMesh->bVisible = false;
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}

		if (XPivotMesh && pivotMode) {
			XPivotMesh->bVisible = true;
			YPivotMesh->bVisible = true;
			ZPivotMesh->bVisible = true;
			PivotCenterMesh->bVisible = true;
		}
	}

	UpdateTransformTool();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdateTransformCenter() {
	bool centerSet = false;

	if (pivotMode) {
		xformCenter = pivotPosition;
		centerSet = true;
	}
	else if (nodesMode || bonesMode) {
		std::string activeBone = os->GetActiveBone();
		if (!activeBone.empty()) {
			auto boneMesh = gls.GetOverlay("BP_" + activeBone);
			if (boneMesh) {
				xformCenter = boneMesh->verts[0];
				centerSet = true;
			}
			else {
				auto nodeMesh = gls.GetOverlay("P_" + activeBone);
				if (nodeMesh) {
					xformCenter = nodeMesh->verts[0];
					centerSet = true;
				}
			}
		}
	}

	if (!centerSet)
		xformCenter = gls.GetActiveCenter();
}

void wxGLPanel::UpdateTransformTool() {
	if (!transformMode)
		return;

	if (!XMoveMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterDistance != 0.0f) {
		float factor = 1.0f / lastCenterDistance;
		XMoveMesh->ScaleVertices(xformCenter, factor);
		YMoveMesh->ScaleVertices(xformCenter, factor);
		ZMoveMesh->ScaleVertices(xformCenter, factor);

		XRotateMesh->ScaleVertices(xformCenter, factor);
		YRotateMesh->ScaleVertices(xformCenter, factor);
		ZRotateMesh->ScaleVertices(xformCenter, factor);

		XScaleMesh->ScaleVertices(xformCenter, factor);
		YScaleMesh->ScaleVertices(xformCenter, factor);
		ZScaleMesh->ScaleVertices(xformCenter, factor);
		ScaleUniformMesh->ScaleVertices(xformCenter, factor);
	}

	lastCenterDistance = unprojected.DistanceTo(xformCenter) / 15.0f;

	XMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ScaleUniformMesh->ScaleVertices(xformCenter, lastCenterDistance);
}

void wxGLPanel::ShowPivot(bool show) {
	if (show) {
		XPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(1.0f, 0.0f, 0.0f), "XPivotMesh");
		YPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 1.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 1.0f, 0.0f), "YPivotMesh");
		ZPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 0.0f, 1.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 0.0f, 1.0f), "ZPivotMesh");
		PivotCenterMesh = gls.AddVis3dCube(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.075f, Vector3(0.0f, 0.0f, 0.0f), "PivotCenterMesh");

		XPivotMesh->bVisible = true;
		YPivotMesh->bVisible = true;
		ZPivotMesh->bVisible = true;
		PivotCenterMesh->bVisible = true;

		lastCenterPivotDistance = 0.0f;
	}
	else {
		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}
	}

	UpdatePivot();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdatePivot() {
	if (!pivotMode)
		return;

	if (!XPivotMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterPivotDistance != 0.0f) {
		float factor = 1.0f / lastCenterPivotDistance;
		XPivotMesh->ScaleVertices(pivotPosition, factor);
		YPivotMesh->ScaleVertices(pivotPosition, factor);
		ZPivotMesh->ScaleVertices(pivotPosition, factor);
		PivotCenterMesh->ScaleVertices(pivotPosition, factor);
	}

	lastCenterPivotDistance = unprojected.DistanceTo(pivotPosition) / 15.0f;

	XPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	YPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	ZPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	PivotCenterMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
}

void wxGLPanel::SetTransformMode(bool on) {
	transformMode = on;
	ShowTransformTool(on);
}

void wxGLPanel::SetPivotMode(bool on) {
	pivotMode = on;
	ShowPivot(on);

	if (transformMode)
		ShowTransformTool();
}
