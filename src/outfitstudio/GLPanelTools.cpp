/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: the editing tools - brush strokes, vertex and edge tools, physics grab and brush settings

#include "GLPanel.h"
#include "SegmentPartitionEditor.h"
#include "PhysicsPreview.h"
#include "OutfitStudioFrame.h"
#include "ShapeTreeItems.h"
#include "ui/wxStateButton.h"
#include "utils/ConfigurationManager.h"
#include "physics/Controller.h"
#include "components/TweakBrush.h"
#include "components/UndoHistory.h"
#include "render/GLSurface.h"
#include "project/BrushStrokeController.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

void wxGLPanel::SetActiveTool(ToolID brushID) {
	activeTool = brushID;

	switch (brushID) {
		case ToolID::MaskBrush: activeBrush = &maskBrush; break;
		case ToolID::InflateBrush: activeBrush = &inflateBrush; break;
		case ToolID::DeflateBrush: activeBrush = &deflateBrush; break;
		case ToolID::MoveBrush: activeBrush = &moveBrush; break;
		case ToolID::SmoothBrush: activeBrush = &smoothBrush; break;
		case ToolID::UndiffBrush: activeBrush = &undiffBrush; break;
		case ToolID::WeightBrush: activeBrush = &weightBrush; break;
		case ToolID::ColorBrush: activeBrush = &colorBrush; break;
		case ToolID::AlphaBrush: activeBrush = &alphaBrush; break;
		default: activeBrush = nullptr; break;
	}

	gls.SetXMirrorCursor(GetToolOptionXMirror());
}

void wxGLPanel::SetLastTool(ToolID tool) {
	lastTool = tool;
}

std::vector<TweakBrush*> wxGLPanel::GetBrushList() {
	std::vector<TweakBrush*> brushList{&inflateBrush,
									   &deflateBrush,
									   &moveBrush,
									   &smoothBrush,
									   &undiffBrush,
									   &maskBrush,
									   &weightBrush,
									   &colorBrush,
									   &alphaBrush};

	return brushList;
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	// Check if brush strokes are currently allowed
	if (activeBrush == &weightBrush) {
		std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;
	}

	TweakPickInfo tpi;
	Mesh* hitMesh = nullptr;
	int hitTri = -1;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
	if (!hit || !hitMesh)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
	tpi.normal.Normalize();
	tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);

	Vector3 v;
	Vector3 vo;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

	v = v * -1.0f;
	tpi.view = v;

	savedBrush = activeBrush;

	if (wxGetKeyState(WXK_CONTROL)) {
		if (wxGetKeyState(WXK_ALT) && !segmentMode) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			UnMaskBrush.setFocus(maskBrush.getFocus());
			UnMaskBrush.setSpacing(maskBrush.getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		WeightBrushBones bones = ProjectOperations(*os->project, os->editor).GetWeightBrushBones(os->GetXMirrorBone());
		AnimInfo* animInfo = os->project->GetWorkAnim();

		if (wxGetKeyState(WXK_ALT)) {
			bones.ApplyTo(unweightBrush, animInfo);
			unweightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			unweightBrush.setStrength(-weightBrush.getStrength());
			unweightBrush.setFocus(weightBrush.getFocus());
			unweightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &unweightBrush;
		}
		else if (wxGetKeyState(WXK_SHIFT)) {
			bones.ApplyTo(smoothWeightBrush, animInfo);
			smoothWeightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			smoothWeightBrush.setStrength(weightBrush.getStrength() * 15.0f);
			smoothWeightBrush.setFocus(weightBrush.getFocus());
			smoothWeightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &smoothWeightBrush;
		}
		else {
			bones.ApplyTo(weightBrush, animInfo);
		}
	}
	else if (wxGetKeyState(WXK_ALT) && !segmentMode) {
		if (activeBrush == &inflateBrush) {
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			activeBrush = &inflateBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-activeBrush->getStrength());
			UnMaskBrush.setFocus(activeBrush->getFocus());
			UnMaskBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else if (activeBrush == &colorBrush) {
			uncolorBrush.setStrength(-activeBrush->getStrength());
			uncolorBrush.setFocus(activeBrush->getFocus());
			uncolorBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &uncolorBrush;
		}
		else if (activeBrush == &alphaBrush) {
			unalphaBrush.setStrength(-activeBrush->getStrength());
			unalphaBrush.setFocus(activeBrush->getFocus());
			unalphaBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &unalphaBrush;
		}
	}
	else if (activeBrush == &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothMaskBrush.setStrength(activeBrush->getStrength());
		smoothMaskBrush.setFocus(activeBrush->getFocus());
		smoothMaskBrush.setSpacing(activeBrush->getSpacing());
		activeBrush = &smoothMaskBrush;
	}
	else if (activeBrush != &weightBrush && activeBrush != &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		activeBrush = &smoothBrush;
	}

	activeBrush->setRadius(brushSize);
	activeBrush->setMirror(GetToolOptionXMirror());
	activeBrush->setConnected(toolOptionConnectedOnly);
	activeBrush->setRestrictPlane(toolOptionRestrictPlane);
	activeBrush->setRestrictNormal(toolOptionRestrictNormal);

	brushStroke = std::make_unique<BrushStrokeController>(*os->project, os->editor);

	if (activeBrush->Type() == TweakBrush::BrushType::Weight) {
		std::vector<NiShape*> shapes;
		for (auto& sel : os->GetSelectedItems())
			shapes.push_back(sel->GetShape());

		brushStroke->PrepareWeightStroke(shapes);
	}

	bool applyFirstDab = activeBrush->Type() != TweakBrush::BrushType::Move && !segmentMode;
	brushStroke->Begin(gls.GetActiveMeshes(), activeBrush, tpi, applyFirstDab);

	if (activeBrush->Type() != TweakBrush::BrushType::Move && segmentMode) {
		{
			bool changed = os->segPartEditor->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->segPartEditor->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->segPartEditor->ShowSegment();
				os->segPartEditor->ShowPartition();
			}
		}
	}

	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	TweakPickInfo tpi;

	if (brushStroke && brushStroke->IsActive()) {
		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true);
		Mesh* hitMesh = nullptr;
		int hitTri = -1;

		if (activeBrush->Type() == TweakBrush::BrushType::Move) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			if (!hit)
				return;

			hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
			if (!hit || !hitMesh)
				return;

			tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
			tpi.normal.Normalize();
			tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);
		}

		Vector3 v;
		Vector3 vo;
		gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

		v = v * -1.0f;
		tpi.view = v;
		if (segmentMode) {
			bool changed = os->segPartEditor->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->segPartEditor->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->segPartEditor->ShowSegment();
				os->segPartEditor->ShowPartition();
			}
		}
		else {
			os->ApplyRefresh(brushStroke->Update(tpi));
		}

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->segPartEditor->ShowSegment();
			os->segPartEditor->ShowPartition();
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (brushStroke && brushStroke->IsActive()) {
		os->ApplyRefresh(brushStroke->End());
		brushStroke.reset();
		activeBrush = savedBrush;

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->segPartEditor->ShowSegment();
			os->segPartEditor->ShowPartition();

			if (os->currentTabButton)
				os->currentTabButton->SetPendingChanges();
		}

		os->UpdateUndoTools();
	}
}

bool wxGLPanel::SelectVertex(const wxPoint& screenPos) {
	int vertIndex;
	if (!gls.GetCursorVertex(screenPos.x, screenPos.y, &vertIndex))
		return false;

	if (os->activeItem) {
		Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
		if (m) {
			double newval = m->mask[vertIndex];
			if (wxGetKeyState(WXK_CONTROL))
				newval = 1.0f;
			else if (!segmentMode)
				newval = 0.0f;

			if (m->mask[vertIndex] != newval) {
				UndoStateProject* usp = GetUndoHistory()->PushState();
				usp->undoType = UndoType::Mask;
				usp->usss.emplace_back();
				UndoStateShape& uss = usp->usss.back();
				uss.shapeName = m->shapeName;
				uss.pointStartState[vertIndex].x = m->mask[vertIndex];
				uss.pointEndState[vertIndex].x = newval;
				ApplyUndoState(usp, false);

				if (!Config.GetBoolValue("Input/MaskHistory"))
					GetUndoHistory()->PopState();

				os->UpdateUndoTools();
			}
		}
	}

	if (transformMode)
		ShowTransformTool();

	return true;
}

bool wxGLPanel::StartPickVertex() {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	return true;
}

void wxGLPanel::UpdatePickVertex(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || hitResult.hoverPoint != mouseDownPoint)
		gls.HidePointCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickVertex() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || lastHitResult.hoverPoint != mouseDownPoint)
		return;

	// Clear PickVertex state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HidePointCursor();

	if (activeTool == ToolID::CollapseVertex)
		ClickCollapseVertex();
}

void wxGLPanel::ClickCollapseVertex() {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Make list of this vertex and its welded vertices.
	std::vector<uint16_t> verts;
	m->GetWeldSet(mouseDownPoint, verts);

	std::sort(verts.begin(), verts.end());

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareCollapseVertex(shape, uss, verts)) {
		wxMessageBox(_("The vertex picked has more than three connections."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

struct EdgeSlideCandidate {
	Edge edge;
	int targetPoint = -1;
	float t = 0.0f;
	float distanceSquared = 0.0f;
};

static float ClampEdgeSlideT(float t) {
	if (t < 0.0f)
		return 0.0f;

	if (t > 1.0f)
		return 1.0f;

	return t;
}

static bool FindEdgeSlideCandidate(GLSurface& gls, Mesh* m, int slidePoint, const Vector3& startPosition, const wxPoint& screenPos, EdgeSlideCandidate& outCandidate) {
	if (!m || slidePoint < 0 || slidePoint >= m->nVerts)
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges)
		return false;

	int startX = 0;
	int startY = 0;
	gls.ProjectPointToScreen(m->TransformPosMeshToModel(startPosition), startX, startY);

	bool foundCandidate = false;
	std::unordered_set<int> seenTargets;
	for (int edgeIndex : m->vertEdges[slidePoint]) {
		if (edgeIndex < 0 || edgeIndex >= m->nEdges)
			continue;

		const Edge& edge = m->edges[edgeIndex];
		int targetPoint = -1;
		if (edge.p1 == slidePoint)
			targetPoint = edge.p2;
		else if (edge.p2 == slidePoint)
			targetPoint = edge.p1;
		else
			continue;

		if (targetPoint < 0 || targetPoint >= m->nVerts || !seenTargets.insert(targetPoint).second)
			continue;

		int endX = 0;
		int endY = 0;
		gls.ProjectPointToScreen(m->TransformPosMeshToModel(m->verts[targetPoint]), endX, endY);

		float dx = static_cast<float>(endX - startX);
		float dy = static_cast<float>(endY - startY);
		float edgeLengthSquared = dx * dx + dy * dy;
		if (edgeLengthSquared <= 0.0001f)
			continue;

		float cursorDx = static_cast<float>(screenPos.x - startX);
		float cursorDy = static_cast<float>(screenPos.y - startY);
		float t = ClampEdgeSlideT((cursorDx * dx + cursorDy * dy) / edgeLengthSquared);
		float closestX = static_cast<float>(startX) + dx * t;
		float closestY = static_cast<float>(startY) + dy * t;
		float distX = static_cast<float>(screenPos.x) - closestX;
		float distY = static_cast<float>(screenPos.y) - closestY;
		float distanceSquared = distX * distX + distY * distY;

		if (!foundCandidate || distanceSquared < outCandidate.distanceSquared) {
			outCandidate.edge = edge;
			outCandidate.targetPoint = targetPoint;
			outCandidate.t = t;
			outCandidate.distanceSquared = distanceSquared;
			foundCandidate = true;
		}
	}

	return foundCandidate;
}

bool wxGLPanel::StartEdgeSlide(const wxPoint& WXUNUSED(screenPos)) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges || m->vertEdges[lastHitResult.hoverPoint].empty())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	edgeSlideStartPosition = lastHitResult.hoverMeshCoord;
	edgeSlideStartUV = Vector2();
	edgeSlideCurrentEdge = Edge();
	edgeSlideTarget = -1;
	edgeSlideHasUV = !os->editor.bEditSlider && GetToolOptionEdgeSlideUVCorrection() && m->texcoord != nullptr;
	if (edgeSlideHasUV)
		edgeSlideStartUV = m->texcoord[mouseDownPoint];

	UndoStateProject* usp = os->editor.undoHistory.PushState();
	usp->undoType = UndoType::VertexPosition;
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = edgeSlideStartPosition;
	if (edgeSlideHasUV)
		usp->usss[0].uvEndState[mouseDownPoint] = usp->usss[0].uvStartState[mouseDownPoint] = edgeSlideStartUV;

	if (os->editor.bEditSlider) {
		usp->sliderName = os->editor.activeSlider;
		float sliderscale = os->project->SliderValue(os->editor.activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateEdgeSlide(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];

	EdgeSlideCandidate candidate;
	if (!FindEdgeSlideCandidate(gls, m, mouseDownPoint, edgeSlideStartPosition, screenPos, candidate)) {
		gls.HideSegCursor();
		return;
	}

	edgeSlideCurrentEdge = candidate.edge;
	edgeSlideTarget = candidate.targetPoint;

	Vector3 targetPosition = m->verts[edgeSlideTarget];
	Vector3 newPosition = edgeSlideStartPosition + (targetPosition - edgeSlideStartPosition) * candidate.t;
	uss.pointEndState[mouseDownPoint] = newPosition;
	m->verts[mouseDownPoint] = newPosition;
	m->QueueUpdate(Mesh::UpdateType::Position);

	if (edgeSlideHasUV && m->texcoord) {
		Vector2 targetUV = m->texcoord[edgeSlideTarget];
		Vector2 newUV = edgeSlideStartUV + (targetUV - edgeSlideStartUV) * candidate.t;
		uss.uvEndState[mouseDownPoint] = newUV;
		m->texcoord[mouseDownPoint] = newUV;
		m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	}

	Vector3 newPositionModel = m->TransformPosMeshToModel(newPosition);
	gls.SetPointCursor(newPositionModel);
	gls.SetCenterCursor(newPositionModel);
	gls.AddVisSeg(m->TransformPosMeshToModel(edgeSlideStartPosition), m->TransformPosMeshToModel(targetPosition), "seghilite");
	gls.ShowCursor(true);
}

void wxGLPanel::EndEdgeSlide() {
	isSlidingEdge = false;
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];
	bool positionChanged = uss.pointEndState[mouseDownPoint] != uss.pointStartState[mouseDownPoint];
	bool uvChanged = false;
	auto uvStart = uss.uvStartState.find(mouseDownPoint);
	auto uvEnd = uss.uvEndState.find(mouseDownPoint);
	if (uvStart != uss.uvStartState.end() && uvEnd != uss.uvEndState.end())
		uvChanged = uvStart->second != uvEnd->second;

	if (!positionChanged && !uvChanged) {
		CancelEdgeSlide();
		return;
	}

	if (!os->editor.bEditSlider) {
		NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		if (shape)
			os->project->ComputeUndoRestDiffs(shape, uss);
	}

	ApplyUndoState(usp, false);
	gls.HideSegCursor();

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelEdgeSlide() {
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	os->editor.undoHistory.PopState();
	gls.HideSegCursor();
}

bool wxGLPanel::StartPhysicsGrab(const wxPoint& screenPos) {
	const auto& affectedShapes = os->physicsPreview->GetAffectedShapes();
	if (affectedShapes.empty())
		return false;

	// The pump skips the BVH update every frame, so the trees still describe
	// the pose the simulation started from. Fit them to where the mesh is now,
	// or the grab misses the cloth that has swung away since.
	os->physicsPreview->RefitBVH();

	// Any shape the simulation moves can be grabbed, whether or not it is one
	// of the shapes picked for editing: what is being grabbed is the
	// simulation, not the project.
	Vector3 viewDir;
	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, viewDir, viewOrigin);

	Mesh* hitMesh = nullptr;
	Vector3 hitMeshPos;
	float hitDistance = std::numeric_limits<float>::max();

	for (auto* m : gls.GetMeshes()) {
		if (!m->bVisible || !m->bvh || affectedShapes.count(m->shapeName) == 0)
			continue;

		Vector3 rayOrigin = m->TransformPosModelToMesh(viewOrigin);
		Vector3 rayDir = m->TransformDirModelToMesh(viewDir);

		std::vector<IntersectResult> results;
		if (!m->bvh->IntersectRay(rayOrigin, rayDir, &results))
			continue;

		for (auto& result : results) {
			const float distance = result.HitCoord.DistanceTo(rayOrigin);
			if (distance >= hitDistance)
				continue;

			hitDistance = distance;
			hitMesh = m;
			hitMeshPos = result.HitCoord;
		}
	}

	if (!hitMesh)
		return false;

	std::vector<Physics::GrabTarget> targets;
	if (!CollectPhysicsGrabTargets(hitMesh, hitMeshPos, targets))
		return false;

	if (!os->physicsPreview->BeginGrab(targets))
		return false;

	// The simulation pulls the grabbed patch out from under the cursor right
	// away, so the drag reads against a plane through the grabbed spot facing
	// the camera rather than against the mesh - the move brush freezes a plane
	// for the same reason.
	physicsGrabStart = hitMesh->TransformPosMeshToModel(hitMeshPos);
	physicsGrabPlaneNormal = viewDir * -1.0f;
	physicsGrabPlaneDist = physicsGrabStart.dot(physicsGrabPlaneNormal);

	ShowPhysicsGrabMarker(physicsGrabStart);
	return true;
}

bool wxGLPanel::CollectPhysicsGrabTargets(Mesh* m, const Vector3& meshPos, std::vector<Physics::GrabTarget>& outTargets) {
	AnimInfo* anim = os->project->GetWorkAnim();
	auto skinIt = anim->shapeSkinning.find(m->shapeName);
	if (skinIt == anim->shapeSkinning.end())
		return false;

	// A patch the size of the brush is taken hold of rather than a single
	// vertex, so that a grab catches a piece of cloth instead of whichever
	// bone happens to skin the one vertex under the cursor.
	const float radius = m->TransformDistModelToMesh(brushSize);
	if (!(radius > 0.0f))
		return false;

	Vector3 patchCenter = meshPos;
	std::vector<IntersectResult> results;
	m->bvh->IntersectSphere(patchCenter, radius, &results);

	// Falloff towards the rim of the patch, so the pull stays centered on the
	// cursor instead of dragging the whole bone chain the patch reaches into
	std::unordered_map<uint16_t, float> patch;
	for (auto& result : results) {
		const Triangle& tri = m->tris[result.HitFacet];
		for (const uint16_t point : {tri.p1, tri.p2, tri.p3}) {
			if (patch.count(point) != 0)
				continue;

			const float distance = m->verts[point].DistanceTo(meshPos) / radius;
			if (distance >= 1.0f)
				continue;

			patch[point] = 1.0f - distance * distance * (3.0f - 2.0f * distance);
		}
	}

	if (patch.empty())
		return false;

	// Hand the patch over to the bones skinning it: each one gets the share of
	// it its weights hold, anchored at the center of that share. Anchoring per
	// bone rather than all of them at the cursor keeps the lever arms short,
	// so pulling the hem of a skirt bends it instead of spinning its bones.
	AnimSkin& skin = skinIt->second;
	float strongest = 0.0f;

	for (auto& boneName : skin.boneNames) {
		AnimWeight& boneWeight = skin.boneWeights[boneName.second];

		Vector3 anchor;
		float share = 0.0f;

		for (auto& patchPoint : patch) {
			auto weightIt = boneWeight.weights.find(patchPoint.first);
			if (weightIt == boneWeight.weights.end())
				continue;

			const float weight = patchPoint.second * weightIt->second;
			if (!(weight > 0.0f))
				continue;

			// The simulation works in NIF global space, model space is that
			// same space in render axes and units
			anchor += Mesh::TransformPosMeshToNif(m->TransformPosMeshToModel(m->verts[patchPoint.first])) * weight;
			share += weight;
		}

		if (!(share > 0.0f))
			continue;

		outTargets.push_back({boneName.first, anchor / share, share});
		strongest = std::max(strongest, share);
	}

	if (outTargets.empty())
		return false;

	// Shares are relative to the bone holding the most of the patch: a patch
	// spread thin over many bones must not pull harder than a compact one.
	// Bones barely touching it only add drag, so they are dropped.
	constexpr float minShare = 0.05f;
	for (auto& target : outTargets)
		target.weight /= strongest;

	outTargets.erase(std::remove_if(outTargets.begin(), outTargets.end(), [minShare](const Physics::GrabTarget& target) { return target.weight < minShare; }),
					 outTargets.end());

	return !outTargets.empty();
}

void wxGLPanel::UpdatePhysicsGrab(const wxPoint& screenPos) {
	Vector3 target;
	if (!gls.CollidePlane(screenPos.x, screenPos.y, target, physicsGrabPlaneNormal, physicsGrabPlaneDist))
		return;

	os->physicsPreview->UpdateGrab(Mesh::TransformDiffMeshToNif(target - physicsGrabStart));
	ShowPhysicsGrabMarker(target);
}

void wxGLPanel::EndPhysicsGrab() {
	isPhysicsGrabbing = false;
	HidePhysicsGrabMarker();
	os->physicsPreview->EndGrab();
}

void wxGLPanel::ShowPhysicsGrabMarker(const Vector3& modelPos) {
	// Marks where the cursor is pulling to, which is not where the mesh is:
	// how far the mesh trails behind the marker shows how hard its physics
	// setup is resisting the grab. Same circle the brush cursor uses, so the
	// radius it covers reads the same way.
	Mesh* marker = gls.AddVisCircle(modelPos, physicsGrabPlaneNormal, brushSize, "physicsgrabcircle");
	if (marker)
		marker->color = Vector3(1.0f, 0.75f, 0.2f);

	gls.AddVisPoint(modelPos, "physicsgrabcenter")->color = Vector3(1.0f, 0.75f, 0.2f);
}

void wxGLPanel::HidePhysicsGrabMarker() {
	gls.DeleteOverlay("physicsgrabcircle");
	gls.DeleteOverlay("physicsgrabcenter");
}

void wxGLPanel::ShowPhysicsProbe(const Vector3& nifPos, float nifRadius) {
	// A mesh rather than an overlay: overlays are drawn after the depth buffer is
	// cleared, so the ball would float in front of the cloth it is pressed into
	// instead of sinking behind it, and how deep it has gone is the whole point.
	// It carries bPrimitive, so nothing treats it as a shape of the project.
	// Model space is NIF global space in render axes and units.
	Mesh* m = gls.AddVis3dSphere(Mesh::TransformPosNifToMesh(nifPos), Mesh::TransformDistNifToMesh(nifRadius), Vector3(1.0f, 0.75f, 0.2f), "physicsprobeball", true);
	if (!m)
		return;

	// Shaded rather than flat tinted, which the other primitives are. A ball drawn in one
	// colour is a disc: nothing in it says where its near side is, and reading how far it
	// has been pushed into the cloth is the whole reason it is on screen. The sphere is
	// built with per-vertex normals and a tangent space already, so this is all it takes.
	m->rendermode = Mesh::RenderMode::LitSolid;
}

void wxGLPanel::HidePhysicsProbe() {
	gls.DeleteMesh("physicsprobeball");
}

bool wxGLPanel::StartMoveVertex(const wxPoint& screenPos) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	mouseDownPointNormal = m->TransformDirMeshToModel(m->norms[mouseDownPoint]);
	moveVertexOperation = MoveVertexOperation::None;

	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, mouseDownViewDir, viewOrigin);
	mouseDownViewDir *= -1.0f;

	// snapDistance: shortest edge length of triangle under pointer
	const Triangle& tri = m->tris[lastHitResult.hoverTri];
	Vector3 gtp1 = m->TransformPosMeshToModel(m->verts[tri.p1]);
	Vector3 gtp2 = m->TransformPosMeshToModel(m->verts[tri.p2]);
	Vector3 gtp3 = m->TransformPosMeshToModel(m->verts[tri.p3]);
	snapDistance = gtp1.DistanceTo(gtp2);
	float elen = gtp2.DistanceTo(gtp3);
	if (snapDistance > elen)
		snapDistance = elen;
	elen = gtp3.DistanceTo(gtp1);
	if (snapDistance > elen)
		snapDistance = elen;

	UndoStateProject* usp = os->editor.undoHistory.PushState();
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = lastHitResult.hoverMeshCoord;

	if (mouseDownMirrorPoint != -1)
		usp->usss[0].pointEndState[mouseDownMirrorPoint] = usp->usss[0].pointStartState[mouseDownMirrorPoint] = m->verts[mouseDownMirrorPoint];

	if (os->editor.bEditSlider) {
		usp->sliderName = os->editor.activeSlider;
		float sliderscale = os->project->SliderValue(os->editor.activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	wxPoint p;
	gls.ProjectPointToScreen(lastHitResult.hoverRealCoord, p.x, p.y);
	mouseDownOffset = screenPos - p;

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateMoveVertex(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;
	UndoStateShape& uss = usp->usss[0];

	// Restore original position in m so CollideMeshes and IntersectSphere
	// will work.
	Vector3 mesholdpos = uss.pointStartState[mouseDownPoint];
	m->verts[mouseDownPoint] = mesholdpos;
	if (mouseDownMirrorPoint != -1)
		m->verts[mouseDownMirrorPoint] = uss.pointStartState[mouseDownMirrorPoint];
	Vector3 oldpos = m->TransformPosMeshToModel(mesholdpos);

	// Since the pointer can be offset from screenPos, calculate a point
	// to display at screenPos.  We do this by intersecting the
	// screenPos ray with the plane perpendicular to mouseDownViewDir at oldpos.
	// This will also be our default for newpos if all tool options are off.
	Vector3 pointerPoint;
	gls.CollidePlane(screenPos.x, screenPos.y, pointerPoint, mouseDownViewDir, mouseDownViewDir.dot(oldpos));

	// Determining newpos: 1. intersect screenPos-ray with plane or surface.
	Vector3 newpos = pointerPoint;
	moveVertexOperation = MoveVertexOperation::Move;
	if (toolOptionRestrictSurface) {
		Vector3 hitpt, hitnormal;
		Mesh* hitmesh = nullptr;
		bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, hitpt, hitnormal, false, &hitmesh);
		if (!hit || !hitmesh) {
			newpos = oldpos;
			moveVertexOperation = MoveVertexOperation::None;
		}
		else
			newpos = hitmesh->TransformPosMeshToModel(hitpt);
	}
	else if (toolOptionRestrictPlane) {
		gls.CollidePlane(screenPos.x, screenPos.y, newpos, mouseDownPointNormal, mouseDownPointNormal.dot(oldpos));
	}

	// 2. Project onto normal
	if (toolOptionRestrictNormal && moveVertexOperation != MoveVertexOperation::None) {
		newpos = oldpos + mouseDownPointNormal * mouseDownPointNormal.dot(newpos - oldpos);
	}

	// 3. Snap to nearest valid point
	if ((toolOptionMerge || toolOptionWeld) && moveVertexOperation != MoveVertexOperation::None) {
		// Get adjacent points (not including welded points)
		std::unordered_set<int> adjPts;
		if (m->vertEdges && m->edges) {
			for (int ei : m->vertEdges[mouseDownPoint]) {
				const Edge& edge = m->edges[ei];
				if (edge.p1 != mouseDownPoint)
					adjPts.insert(edge.p1);
				if (edge.p2 != mouseDownPoint)
					adjPts.insert(edge.p2);
			}
		}

		int closestPoint = -1;
		Mesh* closestMesh = nullptr;
		float closestDist = snapDistance;

		for (Mesh* tm : gls.GetActiveMeshes()) {
			Vector3 viewDir, viewOrigin;
			gls.GetPickRay(screenPos.x, screenPos.y, tm, viewDir, viewOrigin);

			Vector3 tmnewpos = tm->TransformPosModelToMesh(newpos);
			float tmSnapDistance = tm->TransformDistModelToMesh(snapDistance);

			std::vector<IntersectResult> iresults;
			if (tm->bvh && tm->bvh->IntersectSphere(tmnewpos, tmSnapDistance, &iresults)) {
				for (const IntersectResult& ir : iresults) {
					const Triangle& t = tm->tris[ir.HitFacet];
					for (int tvi = 0; tvi < 3; ++tvi) {
						int tp = t[tvi];
						if (tm == m && tp == mouseDownPoint)
							continue;
						if (tm == m && adjPts.count(tp) != 0)
							continue;

						// Calculate distance in plane perpendicular to view
						Vector3 diff = tm->verts[tp] - tmnewpos;
						diff -= viewDir * viewDir.dot(diff);
						float d = tm->TransformDistMeshToModel(diff.length());
						if (d >= closestDist)
							continue;

						closestPoint = tp;
						closestMesh = tm;
						closestDist = d;
					}
				}
			}
		}

		if (closestPoint != -1) {
			NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
			NiShape* s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(closestMesh->shapeName);
			bool canWeld = toolOptionWeld && s1 && s2 && os->project->PointsHaveDifferingWeightsOrDiffs(s1, mouseDownPoint, s2, closestPoint);
			bool canMerge = toolOptionMerge && m == closestMesh;
			if (canWeld || canMerge) {
				newpos = closestMesh->TransformPosMeshToModel(closestMesh->verts[closestPoint]);
				if (canWeld)
					moveVertexOperation = MoveVertexOperation::Weld;
				else if (canMerge)
					moveVertexOperation = MoveVertexOperation::Merge;
				moveVertexTarget = closestPoint;
				moveVertexWeldTargetMeshName = closestMesh->shapeName;
			}
		}
	}

	Vector3 meshnewpos = m->TransformPosModelToMesh(newpos);
	uss.pointEndState[mouseDownPoint] = meshnewpos;
	m->verts[mouseDownPoint] = meshnewpos;
	gls.SetPointCursor(newpos);
	gls.SetCenterCursor(pointerPoint);
	m->QueueUpdate(Mesh::UpdateType::Position);
	gls.ShowCursor(true);

	if (mouseDownMirrorPoint != -1) {
		Vector3 mp = newpos;
		mp.x = -mp.x;
		mp = m->TransformPosModelToMesh(mp);
		uss.pointEndState[mouseDownMirrorPoint] = mp;
		m->verts[mouseDownMirrorPoint] = mp;
		gls.ShowMirrorPointCursor(mp, m);
	}
}

void wxGLPanel::EndMoveVertex() {
	isMovingVertex = false;
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	if (moveVertexOperation == MoveVertexOperation::None) {
		CancelMoveVertex();
		return;
	}

	bool isWeld = moveVertexOperation == MoveVertexOperation::Weld;
	bool isMerge = moveVertexOperation == MoveVertexOperation::Merge;

	if (isWeld || isMerge) {
		NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		NiShape* s2 = s1;
		if (isWeld)
			s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(moveVertexWeldTargetMeshName);

		bool p1b = os->project->IsVertexOnBoundary(s1, mouseDownPoint);
		bool p2b = os->project->IsVertexOnBoundary(s2, moveVertexTarget);

		if (!p1b || !p2b) {
			int response = wxMessageBox(
				!p1b ? !p2b ? _("Neither the selected nor target vertices are on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
							: _("The selected vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
					 : _("The target vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?"),
				_("Weld/Merge Non-Boundary Vertices"),
				wxOK | wxCANCEL | wxCANCEL_DEFAULT,
				os);

			if (response == wxCANCEL) {
				CancelMoveVertex();
				return;
			}
		}

		usp->undoType = UndoType::Mesh;
		usp->sliderName.clear();
		usp->usss[0].pointStartState.clear();
		usp->usss[0].pointEndState.clear();

		if (isWeld)
			os->project->PrepareWeldVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget, s2);

		if (isMerge)
			os->project->PrepareMergeVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget);
	}

	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelMoveVertex() {
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	os->editor.undoHistory.PopState();
}

bool wxGLPanel::StartPickEdge() {
	if (lastHitResult.hitMeshName.empty() || (lastHitResult.hoverEdge.p1 == 0 && lastHitResult.hoverEdge.p2 == 0))
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownEdge = lastHitResult.hoverEdge;
	return true;
}

void wxGLPanel::UpdatePickEdge(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || !hitResult.hoverEdge.CompareIndices(mouseDownEdge))
		gls.HideSegCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickEdge() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || !lastHitResult.hoverEdge.CompareIndices(mouseDownEdge))
		return;

	// Clear PickEdge state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HideSegCursor();

	if (activeTool == ToolID::FlipEdge)
		ClickFlipEdge();
	if (activeTool == ToolID::SplitEdge)
		ClickSplitEdge();
}

void wxGLPanel::ClickFlipEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareFlipEdge(shape, uss, mouseDownEdge)) {
		wxMessageBox(_("The edge picked is on the surface boundary.  Pick an interior edge."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::ClickSplitEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	bool shiftDown = wxGetKeyState(WXK_SHIFT);

	constexpr uint16_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	uint32_t maxTriIndex = std::numeric_limits<uint16_t>().max();

	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	if (shape->GetNumVertices() > maxVertIndex - 2) {
		wxMessageBox(_("The shape has reached the vertex count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	if (shape->GetNumTriangles() > maxTriIndex - 2) {
		wxMessageBox(_("The shape has reached the triangle count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	std::vector<bool> pincs(shape->GetNumVertices(), false);
	pincs[mouseDownEdge.p1] = true;
	pincs[mouseDownEdge.p2] = true;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	bool noCurveOffset = shiftDown;
	if (!os->project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, noCurveOffset)) {
		wxMessageBox(_("The edge picked has multiple triangles of the same orientation.  Correct the orientations before splitting."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

bool wxGLPanel::GetToolOptionXMirror() {
	if (activeTool == ToolID::WeightBrush)
		return toolOptionXMirrorWeight;
	return toolOptionXMirror;
}

void wxGLPanel::SetToolOptionXMirror(bool on) {
	if (activeTool == ToolID::WeightBrush)
		toolOptionXMirrorWeight = on;
	else
		toolOptionXMirror = on;
	gls.SetXMirrorCursor(on);
}

void wxGLPanel::SetBrushSize(float val) {
	val *= BrushSizeScale;
	brushSize = val;
	gls.SetCursorSize(val);
}

void wxGLPanel::ResetBrushSize() {
	brushSize = 0.45f;
	gls.SetCursorSize(brushSize);
}

float wxGLPanel::IncBrush(float step) {
	gls.SetCursorSize(gls.GetCursorSize() + step);
	brushSize += step;
	LimitBrushSize();
	return brushSize;
}

float wxGLPanel::DecBrush(float step) {
	gls.SetCursorSize(gls.GetCursorSize() - step);
	brushSize -= step;
	LimitBrushSize();
	return brushSize;
}

void wxGLPanel::LimitBrushSize() {
	if (brushSize < 0.000f) {
		gls.SetCursorSize(0.000f);
		brushSize = 0.000f;
	}
	else if (brushSize > BrushSizeScale) {
		gls.SetCursorSize(BrushSizeScale);
		brushSize = BrushSizeScale;
	}
}

float wxGLPanel::IncStr() {
	if (!activeBrush)
		return 0.0f;

	float str = activeBrush->getStrength();
	str += 0.010f;
	activeBrush->setStrength(str);
	return str;
}

float wxGLPanel::DecStr() {
	if (!activeBrush)
		return 0.0f;

	float str = activeBrush->getStrength();
	str -= 0.010f;
	activeBrush->setStrength(str);
	return str;
}
