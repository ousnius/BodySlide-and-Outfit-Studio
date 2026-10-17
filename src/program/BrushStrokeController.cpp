/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BrushStrokeController.h"
#include "../utils/ConfigurationManager.h"

extern ConfigurationManager Config;

using namespace nifly;

namespace {

// Brushes that change the shape geometry rather than weights or colors
bool ChangesGeometry(TweakBrush::BrushType type) {
	return type != TweakBrush::BrushType::Mask && type != TweakBrush::BrushType::Weight && type != TweakBrush::BrushType::Color
		&& type != TweakBrush::BrushType::Alpha;
}

} // namespace

BrushStrokeController::BrushStrokeController(OutfitProject& inProject, EditorState& inEditor)
	: project(inProject)
	, editor(inEditor) {}

void BrushStrokeController::PrepareWeightStroke(const std::vector<NiShape*>& shapes) {
	for (auto* shape : shapes) {
		project.CreateSkinning(shape);

		int boneIndex = project.GetWorkAnim()->GetShapeBoneIndex(shape->name.get(), editor.activeBone);
		if (boneIndex < 0)
			project.AddBoneRef(editor.activeBone);
	}
}

void BrushStrokeController::Begin(const std::vector<Mesh*>& meshes, TweakBrush* inBrush, TweakPickInfo pick, bool applyFirstDab) {
	brush = inBrush;
	stroke = std::make_unique<TweakStroke>(meshes, brush, *editor.undoHistory.PushState());

	if (editor.bEditSlider) {
		stroke->usp.sliderName = editor.activeSlider;
		float sliderscale = project.SliderValue(editor.activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		stroke->usp.sliderscale = sliderscale;
	}

	if (brush->Type() == TweakBrush::BrushType::Undiff) {
		// The undiff brush moves towards the base vertex positions, not the current mesh positions
		std::vector<Mesh*> refMeshes = stroke->GetRefMeshes();
		std::vector<std::vector<Vector3>> positionData(refMeshes.size());

		for (size_t i = 0; i < refMeshes.size(); i++) {
			std::vector<Vector3> basePosition;

			auto workNif = project.GetWorkNif();
			auto shape = workNif->FindBlockByName<NiShape>(refMeshes[i]->shapeName);
			if (shape)
				workNif->GetVertsForShape(shape, basePosition);

			for (auto& p : basePosition)
				p = Mesh::TransformPosNifToMesh(p);

			positionData[i] = std::move(basePosition);
		}

		stroke->beginStroke(pick, positionData);
	}
	else
		stroke->beginStroke(pick);

	if (applyFirstDab)
		stroke->updateStroke(pick);
}

RefreshFlags BrushStrokeController::Update(TweakPickInfo pick) {
	if (!stroke)
		return RefreshFlags::None;

	stroke->updateStroke(pick);

	if (brush->Type() == TweakBrush::BrushType::Weight && !editor.activeBone.empty())
		return ProjectOperations(project, editor).ApplyShapeEdits(*editor.undoHistory.GetCurState(), false);

	return RefreshFlags::None;
}

RefreshFlags BrushStrokeController::End() {
	if (!stroke)
		return RefreshFlags::None;

	stroke->endStroke();

	RefreshFlags refresh = RefreshFlags::None;
	TweakBrush::BrushType brushType = stroke->BrushType();

	if (brushType != TweakBrush::BrushType::Mask) {
		ProjectOperations ops(project, editor);
		UndoStateProject* usp = editor.undoHistory.GetCurState();
		refresh |= ops.ApplyShapeEdits(*usp, false);

		if (brushType == TweakBrush::BrushType::Weight && !editor.activeBone.empty())
			refresh |= RefreshFlags::WeightDisplay;

		if (ChangesGeometry(brushType)) {
			if (!editor.bEditSlider) {
				for (auto& uss : usp->usss) {
					auto shape = project.GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						project.ComputeUndoRestDiffs(shape, uss);
				}

				for (auto& s : project.GetWorkNif()->GetShapes()) {
					ops.UpdateShapeFromMesh(s);
					project.RefreshMorphShape(s);
				}
			}

			// Posed meshes have to be skinned again with the changed geometry or morph
			if (project.bPose)
				refresh |= RefreshFlags::Meshes;
		}
	}
	else if (!Config.GetBoolValue("Input/MaskHistory"))
		editor.undoHistory.PopState();

	stroke.reset();
	brush = nullptr;
	return refresh;
}

TweakPickInfo BrushStrokeController::PickFromMeshHit(Mesh* mesh, const Vector3& meshOrigin, const Vector3& meshNormal, const Vector3& view) {
	TweakPickInfo pick;
	pick.origin = mesh->TransformPosMeshToModel(meshOrigin);

	Vector3 normal = meshNormal;
	normal.Normalize();
	pick.normal = mesh->TransformDirMeshToModel(normal);
	pick.view = view;
	return pick;
}

TweakPickInfo BrushStrokeController::PickAtVertex(Mesh* mesh, int vertex) {
	Vector3 normal = mesh->norms ? mesh->norms[vertex] : Vector3(0.0f, 0.0f, 1.0f);
	TweakPickInfo pick = PickFromMeshHit(mesh, mesh->verts[vertex], normal, Vector3());
	pick.view = pick.normal;
	return pick;
}
