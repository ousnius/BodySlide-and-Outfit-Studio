/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BrushStrokeController.h"
#include "../utils/AABBTree.h"
#include "../utils/ConfigurationManager.h"

#include <algorithm>
#include <cfloat>
#include <map>

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

RefreshFlags BrushStrokeController::SmoothMeshes(const std::vector<Mesh*>& meshes, int iterations, float strength, bool keepBorders) {
	if (meshes.empty() || iterations < 1)
		return RefreshFlags::None;

	// One dab of a brush large enough for every vertex, which then has the full strength
	TB_Smooth smooth;
	smooth.setRadius(1.0e6f);
	smooth.setStrength(strength);
	smooth.setMirror(false);

	Mesh* first = meshes.front();
	TweakPickInfo pick;
	pick.origin = first->TransformPosMeshToModel(first->nVerts > 0 ? first->verts[0] : Vector3());
	pick.normal = Vector3(0.0f, 0.0f, 1.0f);
	pick.view = pick.normal;

	// Open edges are masked for the stroke: smoothing pulls them in, opening gaps to what's attached there
	std::vector<std::vector<std::pair<int, float>>> savedMasks(meshes.size());
	if (keepBorders) {
		for (size_t mi = 0; mi < meshes.size(); mi++) {
			Mesh* m = meshes[mi];
			if (!m->mask || !m->tris)
				continue;
			if (!m->bGotWeldVerts)
				m->CalcWeldVerts();

			// Edges of only one triangle, with UV seams welded
			std::map<std::pair<int, int>, int> edgeCount;
			for (int t = 0; t < m->nTris; t++) {
				const Triangle& tri = m->tris[t];
				const int p[3] = {m->LeastWeldedVertexIndex(tri.p1), m->LeastWeldedVertexIndex(tri.p2), m->LeastWeldedVertexIndex(tri.p3)};
				for (int e = 0; e < 3; e++)
					edgeCount[std::minmax(p[e], p[(e + 1) % 3])]++;
			}

			auto pin = [&](int p) {
				m->DoForEachWeldedVertex(p, [&](int w) {
					if (m->mask[w] < 1.0f) {
						savedMasks[mi].emplace_back(w, m->mask[w]);
						m->mask[w] = 1.0f;
					}
				});
				if (m->mask[p] < 1.0f) {
					savedMasks[mi].emplace_back(p, m->mask[p]);
					m->mask[p] = 1.0f;
				}
			};
			for (auto& e : edgeCount) {
				if (e.second == 1) {
					pin(e.first.first);
					pin(e.first.second);
				}
			}
		}
	}

	Begin(meshes, &smooth, pick);
	for (int i = 1; i < iterations; i++)
		Update(pick);
	RefreshFlags refresh = End();

	for (size_t mi = 0; mi < meshes.size(); mi++)
		for (auto& saved : savedMasks[mi])
			meshes[mi]->mask[saved.first] = saved.second;
	return refresh;
}

std::vector<UndoStateShape> BrushStrokeController::FollowEdit(const UndoStateProject& edit, const std::vector<Mesh*>& editedMeshes,
															  const std::vector<Mesh*>& followers, float radius) {
	// Each edited mesh before and after the edit, to find the surface under a follower vertex in both
	struct EditedSurface {
		Mesh* mesh = nullptr;
		std::vector<Vector3> before; // Mesh space
		std::unique_ptr<AABBTree> treeBefore;
		std::unique_ptr<AABBTree> treeAfter;
		Vector3 lo, hi; // Model space box around the moved vertices, grown by the radius
	};

	std::vector<EditedSurface> surfaces;
	for (Mesh* m : editedMeshes) {
		auto uss = std::find_if(edit.usss.begin(), edit.usss.end(), [&](const UndoStateShape& u) { return u.shapeName == m->shapeName; });
		if (uss == edit.usss.end() || !m->tris || m->nTris == 0)
			continue;

		EditedSurface s;
		s.mesh = m;
		s.before.assign(m->verts.get(), m->verts.get() + m->nVerts);
		s.lo = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		s.hi = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		bool moved = false;
		for (auto& start : uss->pointStartState) {
			auto end = uss->pointEndState.find(start.first);
			if (start.first >= m->nVerts || end == uss->pointEndState.end() || end->second == start.second)
				continue;

			s.before[start.first] = start.second;
			for (const Vector3& p : {start.second, end->second}) {
				Vector3 pos = m->TransformPosMeshToModel(p);
				s.lo = Vector3(std::min(s.lo.x, pos.x), std::min(s.lo.y, pos.y), std::min(s.lo.z, pos.z));
				s.hi = Vector3(std::max(s.hi.x, pos.x), std::max(s.hi.y, pos.y), std::max(s.hi.z, pos.z));
			}
			moved = true;
		}
		if (!moved)
			continue;

		s.lo -= Vector3(radius, radius, radius);
		s.hi += Vector3(radius, radius, radius);
		s.treeBefore = std::make_unique<AABBTree>(s.before.data(), m->tris.get(), m->nTris, 100, 2);
		s.treeAfter = std::make_unique<AABBTree>(m->verts.get(), m->tris.get(), m->nTris, 100, 2);
		surfaces.push_back(std::move(s));
	}

	std::vector<UndoStateShape> result;
	if (surfaces.empty())
		return result;

	auto faceNormal = [](const std::vector<Vector3>& verts, const Triangle& t) {
		Vector3 n = (verts[t.p2] - verts[t.p1]).cross(verts[t.p3] - verts[t.p1]);
		n.Normalize();
		return n;
	};

	for (Mesh* m : followers) {
		if (std::find(editedMeshes.begin(), editedMeshes.end(), m) != editedMeshes.end())
			continue;

		UndoStateShape uss;
		uss.shapeName = m->shapeName;
		for (int i = 0; i < m->nVerts; i++) {
			const float keep = m->mask ? 1.0f - m->mask[i] : 1.0f;
			if (keep <= 0.0f)
				continue;

			// The vertex moves as far as the surface under it, across the surface only: the closest
			// point after the edit can be a different one, so it doesn't slide along with the surface
			// vertices or follow open edges the edit moved
			Vector3 pos = m->TransformPosMeshToModel(m->verts[i]);
			float bestDist = FLT_MAX;
			Vector3 move;
			for (auto& s : surfaces) {
				if (pos.x < s.lo.x || pos.y < s.lo.y || pos.z < s.lo.z || pos.x > s.hi.x || pos.y > s.hi.y || pos.z > s.hi.z)
					continue;

				Mesh* sm = s.mesh;
				Vector3 q = sm->TransformPosModelToMesh(pos);
				const float meshRadius = sm->TransformDistModelToMesh(radius);
				uint32_t facetBefore = 0;
				uint32_t facetAfter = 0;
				Vector3 closestBefore, closestAfter;
				if (!s.treeBefore->ClosestFacetInSphere(q, meshRadius, facetBefore, &closestBefore))
					continue;
				if (!s.treeAfter->ClosestFacetInSphere(q, meshRadius * 2.0f, facetAfter, &closestAfter))
					continue;

				const float dist = q.DistanceTo(closestBefore);
				if (dist >= bestDist)
					continue;

				const Vector3 normalBefore = faceNormal(s.before, sm->tris[facetBefore]);
				const float lift = (closestAfter - closestBefore).dot(normalBefore);
				move = sm->TransformDiffMeshToModel(normalBefore * lift);
				bestDist = dist;
			}

			if (bestDist == FLT_MAX || move.length() * keep < 1.0e-6f)
				continue;

			move = m->TransformDiffModelToMesh(move * keep);
			uss.pointStartState[i] = m->verts[i];
			uss.pointEndState[i] = m->verts[i] + move;
		}

		if (!uss.pointEndState.empty())
			result.push_back(std::move(uss));
	}

	return result;
}
