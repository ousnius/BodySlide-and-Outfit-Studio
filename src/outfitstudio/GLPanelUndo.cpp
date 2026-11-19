/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: undo and redo

#include "GLPanel.h"
#include "OutfitStudioFrame.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

bool wxGLPanel::RestoreMode(UndoStateProject* usp) {
	bool modeChanged = false;
	UndoType undoType = usp->undoType;
	if (undoType == UndoType::VertexPosition && os->editor.activeSlider != usp->sliderName) {
		os->EnterSliderEdit(usp->sliderName);
		modeChanged = true;
	}
	if ((undoType != UndoType::VertexPosition || usp->sliderName.empty()) && os->editor.bEditSlider) {
		os->ExitSliderEdit();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && !usp->sliderName.empty() && usp->sliderscale != os->project->SliderValue(usp->sliderName)) {
		os->SetSliderValue(usp->sliderName, usp->sliderscale * 100);
		os->ApplySliders();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && usp->sliderName.empty() && !os->project->AllSlidersZero()) {
		os->ZeroSliders();
		modeChanged = true;
	}
	return modeChanged;
}

void wxGLPanel::ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender) {
	switch (usp->undoType) {
		case UndoType::Weight: ApplyUndoWeights(usp, bUndo); break;
		case UndoType::Mask: ApplyUndoPoints(usp, bUndo, Mesh::UpdateType::Mask, [](Mesh& m, int i, const Vector3& v) { m.mask[i] = v.x; }); break;
		case UndoType::Color:
			ApplyUndoPoints(usp, bUndo, Mesh::UpdateType::VertexColors, [](Mesh& m, int i, const Vector3& v) { m.vcolors[i] = v; });
			break;
		case UndoType::Alpha:
			ApplyUndoPoints(usp, bUndo, Mesh::UpdateType::VertexAlpha, [](Mesh& m, int i, const Vector3& v) { m.valpha[i] = v.x; });
			os->ActiveShapesUpdated(usp, bUndo);
			break;
		case UndoType::VertexPosition: ApplyUndoVertexPositions(usp, bUndo); break;
		case UndoType::Mesh: ApplyUndoMesh(usp, bUndo); break;
		case UndoType::ShapeDelete: ApplyUndoShapeDelete(usp, bUndo); break;
		case UndoType::Mirror: ApplyUndoMirror(usp); break;
		case UndoType::NodeTransform: ApplyUndoNodeTransform(usp, bUndo); break;
		case UndoType::BoneEdit: ApplyUndoBoneEdit(usp, bUndo); break;
	}

	if (bRender) {
		if (transformMode)
			ShowTransformTool();
		else
			Render();
	}
}

void wxGLPanel::ApplyUndoWeights(UndoStateProject* usp, bool bUndo) {
	for (auto& uss : usp->usss) {
		Mesh* m = GetMesh(uss.shapeName);
		if (!m)
			continue;

		for (auto& bw : uss.boneWeights) {
			if (bw.boneName != os->GetActiveBone())
				continue;

			for (auto& wIt : bw.weights)
				m->weight[wIt.first] = bUndo ? wIt.second.startVal : wIt.second.endVal;
		}

		m->QueueUpdate(Mesh::UpdateType::Weight);
	}

	os->ActiveShapesUpdated(usp, bUndo);
}

void wxGLPanel::ApplyUndoPoints(UndoStateProject* usp, bool bUndo, Mesh::UpdateType updateType, void (*setPoint)(Mesh&, int, const Vector3&)) {
	for (auto& uss : usp->usss) {
		Mesh* m = GetMesh(uss.shapeName);
		if (!m)
			continue;

		for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
			setPoint(*m, pit.first, pit.second);

		m->QueueUpdate(updateType);
	}
}

void wxGLPanel::ApplyUndoUVs(UndoStateProject* usp, UndoStateShape& uss, bool bUndo) {
	if (!usp->sliderName.empty())
		return;

	const auto& uvState = bUndo ? uss.uvStartState : uss.uvEndState;
	if (uvState.empty())
		return;

	Mesh* m = GetMesh(uss.shapeName);
	if (m && m->texcoord) {
		for (auto& uvIt : uvState) {
			if (uvIt.first >= 0 && uvIt.first < m->nVerts)
				m->texcoord[uvIt.first] = uvIt.second;
		}

		m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	}

	NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
	if (!shape)
		return;

	std::vector<Vector2> uvs;
	if (!os->project->GetWorkNif()->GetUvsForShape(shape, uvs))
		return;

	for (auto& uvIt : uvState) {
		if (uvIt.first >= 0 && uvIt.first < static_cast<int>(uvs.size()))
			uvs[uvIt.first] = uvIt.second;
	}

	os->project->GetWorkNif()->SetUvsForShape(shape, uvs);
}

void wxGLPanel::UpdateMeshFromLiveVerts(NiShape* shape) {
	std::vector<Vector3> verts;
	os->project->GetLiveVerts(shape, verts);
	UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
}

void wxGLPanel::UpdateMeshesFromLiveVerts(UndoStateProject* usp) {
	for (auto& uss : usp->usss) {
		auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (shape)
			UpdateMeshFromLiveVerts(shape);
	}
}

void wxGLPanel::ApplyUndoVertexPositions(UndoStateProject* usp, bool bUndo) {
	bool hasRestDiffs = false;
	for (auto& uss : usp->usss) {
		if (!uss.restDiffs.empty()) {
			hasRestDiffs = true;
			break;
		}
	}

	if (hasRestDiffs) {
		// Pose-independent undo/redo: use stored rest-space NIF diffs
		if (usp->sliderName.empty()) {
			for (auto& uss : usp->usss) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
				if (!shape)
					continue;

				std::vector<Vector3> restVerts;
				os->project->GetWorkNif()->GetVertsForShape(shape, restVerts);

				for (auto& rd : uss.restDiffs) {
					if (bUndo)
						restVerts[rd.first] -= rd.second;
					else
						restVerts[rd.first] += rd.second;
				}

				os->project->GetWorkNif()->SetVertsForShape(shape, restVerts);
			}
		}

		os->ActiveShapesUpdated(usp, bUndo);

		for (auto& uss : usp->usss) {
			auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			UpdateMeshFromLiveVerts(shape);

			Mesh* m = GetMesh(uss.shapeName);
			if (m)
				m->CalcWeldVerts();

			ApplyUndoUVs(usp, uss, bUndo);
		}
		return;
	}

	for (auto& uss : usp->usss) {
		Mesh* m = GetMesh(uss.shapeName);
		if (!m)
			continue;

		for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
			m->verts[pit.first] = pit.second;

		m->CalcWeldVerts();
		m->SmoothNormals();
		BVHUpdateQueue.insert(m);

		m->QueueUpdate(Mesh::UpdateType::Position);

		ApplyUndoUVs(usp, uss, bUndo);
	}

	os->ActiveShapesUpdated(usp, bUndo);

	// Without a slider the edit changed the shape itself, which then follows the mesh
	if (usp->sliderName.empty()) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (shape)
				os->project->UpdateShapeFromMesh(shape, m);
		}
	}

	if (os->project->bPose)
		UpdateMeshesFromLiveVerts(usp);
}

void wxGLPanel::RestoreOrDeleteShapes(UndoStateProject* usp, bool restore) {
	if (restore) {
		for (auto& ds : usp->deletedShapes)
			os->project->RestoreDeletedShape(ds);
	}
	else {
		for (auto& ds : usp->deletedShapes) {
			auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(ds.shapeName);
			if (shape)
				os->project->DeleteShape(shape);
		}
	}
}

void wxGLPanel::ApplyUndoMesh(UndoStateProject* usp, bool bUndo) {
	std::unordered_map<std::string, std::vector<float>> maskStash = StashMasks();

	// Undo: restore deleted shapes before processing vertex mesh undos
	if (bUndo)
		RestoreOrDeleteShapes(usp, true);

	for (auto& uss : usp->usss) {
		NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;

		os->project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, bUndo);
	}

	// Redo: re-delete shapes after processing vertex mesh redos
	if (!bUndo)
		RestoreOrDeleteShapes(usp, false);

	os->RefreshGUIFromProj(false, false);
	UnstashMasks(maskStash);
	os->ApplySliders();
}

void wxGLPanel::ApplyUndoShapeDelete(UndoStateProject* usp, bool bUndo) {
	RestoreOrDeleteShapes(usp, bUndo);
	os->RefreshGUIFromProj(false, false);
	os->ApplySliders();
}

void wxGLPanel::ApplyUndoMirror(UndoStateProject* usp) {
	for (auto& uss : usp->usss) {
		NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;

		os->project->GetWorkNif()->MirrorShape(shape, usp->mirrorX, usp->mirrorY, usp->mirrorZ);
		if (usp->swapBonesX)
			os->project->GetWorkAnim()->SwapBonesLR(uss.shapeName);
	}

	os->RefreshGUIFromProj(false);
}

void wxGLPanel::ApplyUndoNodeTransform(UndoStateProject* usp, bool bUndo) {
	const MatTransform& target = bUndo ? usp->nodeStartXformToParent : usp->nodeEndXformToParent;

	if (AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(usp->boneName))
		bPtr->SetTransformBoneToParent(target);

	auto* nif = os->project->GetWorkNif();
	if (nif) {
		nif->SetNodeTransformToParent(usp->boneName, target);
		if (auto* workAnim = os->project->GetWorkAnim())
			workAnim->WriteNodesToNif(nif);

		// Re-skin all shapes so poses and bind-pose display both follow
		// the restored bone transform.
		for (auto& s : nif->GetShapes())
			UpdateMeshFromLiveVerts(s);
	}

	UpdateAfterSkeletonUndo();
}

void wxGLPanel::ApplyUndoBoneEdit(UndoStateProject* usp, bool bUndo) {
	os->project->ApplyBoneXforms(usp->boneXforms, bUndo);

	// The skin binds were recalculated, so this only changes posed meshes
	if (auto* nif = os->project->GetWorkNif()) {
		for (auto& s : nif->GetShapes())
			UpdateMeshFromLiveVerts(s);
	}

	UpdateAfterSkeletonUndo();
}

void wxGLPanel::UpdateAfterSkeletonUndo() {
	UpdateBones();
	UpdateNodes();
	os->RefreshBoneTreeBadBoneIcons();
	os->SetPendingChanges();
}

bool wxGLPanel::UndoStroke() {
	UndoStateProject* curState = os->editor.undoHistory.GetCurState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!os->editor.undoHistory.BackStepHistory())
		return false;

	ApplyUndoState(curState, true);

	return true;
}

bool wxGLPanel::RedoStroke() {
	UndoStateProject* curState = os->editor.undoHistory.GetNextState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!os->editor.undoHistory.ForwardStepHistory())
		return false;

	ApplyUndoState(curState, false);

	return true;
}
