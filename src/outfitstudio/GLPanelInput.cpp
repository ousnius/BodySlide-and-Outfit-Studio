/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Viewport: keyboard and mouse input

#include "GLPanel.h"
#include "PhysicsPreview.h"
#include "AnimationPlayer.h"
#include "OutfitStudioFrame.h"
#include "ui/outfitstudio/wxBrushSettingsPopup.h"
#include "utils/ConfigurationManager.h"
#include "render/GLSurface.h"
#include "files/HkxFile.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	// No editing shortcuts while an animation is playing.
	if (os && os->animation->IsPlaying()) {
		event.Skip();
		return;
	}

	if (event.GetKeyCode() == WXK_F12 && event.ControlDown() && event.AltDown() && event.ShiftDown()) {
		// Hidden: saves an offscreen render of the view, to check rendering without screen captures
		wxString fileName = wxString::FromUTF8(Config["AppDir"]) + "/ViewportCapture.png";
		if (SaveViewImage(fileName, 1024, 1024))
			wxLogMessage("Saved view image to '%s'.", fileName);
		return;
	}

	if (event.GetKeyCode() == WXK_SPACE && event.GetModifiers() == wxMOD_CONTROL) {
		if (!os->editor.activeSlider.empty()) {
			os->ExitSliderEdit();
		}
		else {
			os->EnterSliderEdit();
			os->ScrollToActiveSlider();
		}
	}
	else if (!event.HasAnyModifiers()) {
		if (event.GetUnicodeKey() == 'V') {
			wxPoint cursorPos(event.GetPosition());

			int vertIndex;
			Mesh* outHitMesh;
			if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex, nullptr, &outHitMesh))
				return;

			if (os->currentTabButton == os->colorsTabButton) {
				auto cpBrushColor = (wxColourPickerCtrl*)os->FindWindowById(XRCID("cpBrushColor"));
				if (!cpBrushColor)
					return;

				if (outHitMesh->vcolors) {
					// Set color for brush to vertex color
					Vector3 vcolor = outHitMesh->vcolors[vertIndex];
					SetColorBrush(vcolor);

					// Set color for picker to vertex color
					wxColour color(vcolor.x * 255.0f, vcolor.y * 255.0f, vcolor.z * 255.0f);
					cpBrushColor->SetColour(color);
				}
			}
			else {
				// Find shape for hit mesh name
				NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(outHitMesh->shapeName);
				if (!shape)
					return;

				os->CloseBrushSettings();

				wxDialog dlg;
				if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(shape, verts);

					Vector3 oldPos = verts[vertIndex];
					XRCCTRL(dlg, "posX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.x));
					XRCCTRL(dlg, "posY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.y));
					XRCCTRL(dlg, "posZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.z));

					if (dlg.ShowModal() == wxID_OK) {
						Vector3 newPos;
						newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
						newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
						newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

						// Move vertex in shape directly
						if (!os->editor.bEditSlider)
							os->project->MoveVertex(shape, newPos, vertIndex);

						// To mesh coordinates
						oldPos = Mesh::TransformPosNifToMesh(oldPos);
						newPos = Mesh::TransformPosNifToMesh(newPos);

						UndoStateShape uss;
						uss.shapeName = shape->name.get();
						uss.pointStartState[vertIndex] = oldPos;
						uss.pointEndState[vertIndex] = newPos;

						// Push changes onto undo stack and execute
						UndoStateProject* usp = GetUndoHistory()->PushState();
						usp->undoType = UndoType::VertexPosition;
						usp->usss.push_back(std::move(uss));

						if (os->editor.bEditSlider) {
							usp->sliderName = os->editor.activeSlider;

							float sliderscale = os->project->SliderValue(os->editor.activeSlider);
							if (sliderscale == 0.0)
								sliderscale = 1.0;

							usp->sliderscale = sliderscale;
						}

						ApplyUndoState(usp, false);
						os->UpdateUndoTools();
					}

					if (transformMode)
						ShowTransformTool();
				}
			}
		}
		else if (event.GetUnicodeKey() == '0' && os->menuBar->IsEnabled(XRCID("btnSelect")))
			os->SelectTool(ToolID::Select);
		else if (event.GetUnicodeKey() == '1' && os->menuBar->IsEnabled(XRCID("btnMaskBrush")))
			os->SelectTool(ToolID::MaskBrush);
		else if (event.GetUnicodeKey() == '2' && os->menuBar->IsEnabled(XRCID("btnInflateBrush")))
			os->SelectTool(ToolID::InflateBrush);
		else if (event.GetUnicodeKey() == '3' && os->menuBar->IsEnabled(XRCID("btnDeflateBrush")))
			os->SelectTool(ToolID::DeflateBrush);
		else if (event.GetUnicodeKey() == '4' && os->menuBar->IsEnabled(XRCID("btnMoveBrush")))
			os->SelectTool(ToolID::MoveBrush);
		else if (event.GetUnicodeKey() == '5' && os->menuBar->IsEnabled(XRCID("btnSmoothBrush")))
			os->SelectTool(ToolID::SmoothBrush);
		else if (event.GetUnicodeKey() == '6' && os->menuBar->IsEnabled(XRCID("btnUndiffBrush")))
			os->SelectTool(ToolID::UndiffBrush);
		else if (event.GetUnicodeKey() == '7' && os->menuBar->IsEnabled(XRCID("btnWeightBrush")))
			os->SelectTool(ToolID::WeightBrush);
		else if (event.GetUnicodeKey() == '8' && os->menuBar->IsEnabled(XRCID("btnColorBrush")))
			os->SelectTool(ToolID::ColorBrush);
		else if (event.GetUnicodeKey() == '9' && os->menuBar->IsEnabled(XRCID("btnAlphaBrush")))
			os->SelectTool(ToolID::AlphaBrush);
		else if (event.GetKeyCode() == WXK_SPACE) {
			if (os->brushSettingsPopupTransient && os->brushSettingsPopupTransient->IsShown()) {
				os->CloseBrushSettings();
			}
			else {
				bool brushSettingsNearCursor = Config.GetBoolValue("Input/BrushSettingsNearCursor");
				if (brushSettingsNearCursor)
					os->PopupBrushSettings();
				else
					os->PopupBrushSettings(os->brushSettings);
			}
		}
		else if (event.GetKeyCode() == WXK_ESCAPE) {
			if (isMovingVertex) {
				CancelMoveVertex();
				isMovingVertex = false;
			}
			if (isSlidingEdge) {
				CancelEdgeSlide();
				isSlidingEdge = false;
			}
			os->CloseBrushSettings();
		}
	}

	event.Skip();
}

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	int delt = event.GetWheelRotation();

	// Switching the edited slider and resizing the brush are both locked out
	// during playback; zooming falls through.
	const bool playing = os && os->animation->IsPlaying();

	if (!playing && event.ControlDown()) {
		std::string sliderName = os->editor.lastActiveSlider;

		if (sliderName.empty())
			sliderName = os->project->GetSliderName(0);

		if (sliderName.empty())
			return;

		if (!os->editor.activeSlider.empty())
			os->ExitSliderEdit();

		size_t sliderCount = os->project->SliderCount();
		size_t sliderIndex = 0;
		if (os->project->SliderIndexFromName(sliderName, sliderIndex)) {
			if (delt < 0) {
				++sliderIndex;

				if (sliderIndex == sliderCount)
					sliderIndex = 0;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
			else {
				if (sliderIndex == 0)
					sliderIndex = sliderCount - 1;
				else
					--sliderIndex;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
		}
	}
	else if (!playing && wxGetKeyState(wxKeyCode('S'))) {
		wxPoint p = event.GetPosition();

		if (brushMode) {
			// Adjust brush size, in finer steps with Shift held
			float step = wxGetKeyState(WXK_SHIFT) ? 0.001f : 0.010f;
			if (delt < 0)
				DecBrush(step);
			else
				IncBrush(step);

			os->CheckBrushBounds();
			os->UpdateBrushSettings();
			gls.UpdateCursor(p.x, p.y, true);
			gls.RenderOneFrame();
		}
	}
	else {
		gls.DollyCamera(delt);
		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	if (os->IsActive())
		SetFocus();

	// Mouse motion floods the message queue, which starves both the idle events
	// and the WM_TIMER that drive playback and physics, so tick them from here
	// as well.
	os->animation->Pump();
	os->physicsPreview->Pump();

	bool cursorExists = false;
	int x;
	int y;
	event.GetPosition(&x, &y);

	mouseHasMovedSinceStart = true;

	ShowRotationCenter(false);

	if (mbuttonDown) {
		isMDragging = true;
		if (wxGetKeyState(wxKeyCode::WXK_SHIFT))
			gls.DollyCamera(y - lastY);
		else
			gls.PanCamera(x - lastX, y - lastY);

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (rbuttonDown) {
		// A right-click that barely moved still opens the bone context menu
		if (!isRDragging && std::abs(x - rDownPos.x) + std::abs(y - rDownPos.y) > FromDIP(3))
			isRDragging = true;

		if (wxGetKeyState(WXK_ALT)) {
			if (lastHitResult.hoverPoint != -1) {
				rotationCenterMode = RotationCenterMode::Picked;
				gls.SetRotationCenter(lastHitResult.hoverRealCoord);
			}
		}

		if (wxGetKeyState(WXK_SHIFT)) {
			gls.PanCamera(x - lastX, y - lastY);
		}
		else {
			float yawDegrees = gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
			os->physicsPreview->InjectCameraYaw(yawDegrees);
			ShowRotationCenter();
		}

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (lbuttonDown || isMovingVertex || isSlidingEdge) {
		isLDragging = true;
		if (isPhysicsGrabbing) {
			UpdatePhysicsGrab(event.GetPosition());
		}
		else if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else if (isMovingPivot) {
			UpdatePivotPosition(event.GetPosition());
		}
		else if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isSelecting) {
			SelectVertex(event.GetPosition());
		}
		else if (isPickingVertex) {
			UpdatePickVertex(event.GetPosition());
		}
		else if (isPickingEdge) {
			UpdatePickEdge(event.GetPosition());
		}
		else if (isMovingVertex) {
			UpdateMoveVertex(event.GetPosition() - mouseDownOffset);
		}
		else if (isSlidingEdge) {
			UpdateEdgeSlide(event.GetPosition());
		}
		else if (isPickingBone) {
			// Dragging off a clicked bone does nothing
		}
		else {
			if (Config.MatchValue("Input/LeftMousePan", "true")) {
				gls.PanCamera(x - lastX, y - lastY);
				UpdateTransformTool();
				UpdatePivot();
			}
		}

		gls.RenderOneFrame();
	}

	if (!rbuttonDown && !lbuttonDown && !isMovingVertex && !isSlidingEdge) {
		GLSurface::CursorHitResult hitResult{};

		// Shown bones can be hovered and clicked with any tool. The handles of the
		// transform and pivot tools take precedence where they overlap a bone.
		std::string boneUnderCursor = PickBone(x, y);
		if (!boneUnderCursor.empty() && (transformMode || pivotMode)) {
			Vector3 handleOrigin, handleNormal;
			Mesh* handleMesh = nullptr;
			if (gls.CollideOverlay(x, y, handleOrigin, handleNormal, &handleMesh) && handleMesh && handleMesh != PivotCenterMesh)
				boneUnderCursor.clear();
		}
		SetHoverBone(boneUnderCursor);

		// The brush cursor is hidden during playback: the brushes are locked out
		// anyway, and it would hit-test against the mesh BVHs that playback
		// leaves stale, reporting vertices that are not where it draws them.
		// It is also hidden over a bone, as a click there selects the bone.
		if (editMode && !(os && os->animation->IsPlaying()) && hoverBone.empty()) {
			cursorExists = gls.UpdateCursor(x, y, true, &hitResult);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if (activeTool == ToolID::MoveVertex && hoverBone.empty()) {
			mouseDownMirrorPoint = -1;
			if (GetToolOptionXMirror()) {
				Vector3 hitPt, hitNrm;
				Mesh* mmesh = nullptr;
				int triInd = 0;
				if (gls.CollideMeshes(x, y, hitPt, hitNrm, true, &mmesh, true, &triInd) && mmesh == hitResult.hitMesh) {
					int hitPti = mmesh->tris[triInd].ClosestVertex(mmesh->verts.get(), hitPt);
					if (hitPti != hitResult.hoverPoint) {
						// Should we also check if the mirror point's location
						// is close to the mirror of the hover point's location?
						mouseDownMirrorPoint = hitPti;
						gls.ShowMirrorPointCursor(mmesh->verts[mouseDownMirrorPoint], mmesh);
					}
				}
			}
		}

		lastHitResult = hitResult;

		if ((transformMode || pivotMode) && !isTransforming && !isMovingPivot) {
			if (XMoveMesh) {
				XMoveMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YMoveMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZMoveMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XRotateMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YRotateMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZRotateMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XScaleMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YScaleMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZScaleMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				ScaleUniformMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			if (XPivotMesh) {
				XPivotMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YPivotMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZPivotMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				PivotCenterMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			Vector3 outOrigin, outNormal;
			Mesh* hitMesh = nullptr;
			if (gls.CollideOverlay(x, y, outOrigin, outNormal, &hitMesh)) {
				if (hitMesh && hitMesh != PivotCenterMesh) {
					hitMesh->color = Vector3(1.0f, 1.0f, 0.0f);
					gls.ShowCursor(false);
				}
			}
		}

		gls.RenderOneFrame();

		if (os->statusBar) {
			if (!hoverBone.empty()) {
				os->statusBar->SetStatusText(wxString::Format("Bone: %s", wxString::FromUTF8(hoverBone)), 1);
			}
			else if (cursorExists) {
				if (activeTool == ToolID::MaskBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", hitResult.hoverPoint, hitResult.hoverMask), 1);
				else if (activeTool == ToolID::WeightBrush) {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hitResult.hoverWeight, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z), 1);
				}
				else if (activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Color: %g, %g, %g, Alpha: %g",
																  hitResult.hoverPoint,
																  hitResult.hoverColor.x,
																  hitResult.hoverColor.y,
																  hitResult.hoverColor.z,
																  hitResult.hoverAlpha),
												 1);
				else {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z),
												 1);
				}
			}
			else {
				os->statusBar->SetStatusText("", 1);
			}
		}
	}

	lastX = x;
	lastY = y;
}

void wxGLPanel::OnLeftDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	lbuttonDown = true;

	// No tool may start editing while an animation plays; the click still falls
	// through to camera navigation.
	if (os && os->animation->IsPlaying())
		return;

	// A grab feeds the running simulation and never touches the mesh data, so
	// while it is armed it takes the click ahead of the editing tools. Missing
	// the simulated mesh leaves the click to them as if it were not.
	if (os->physicsPreview->IsGrabEnabled()) {
		bool meshHit = StartPhysicsGrab(event.GetPosition());
		if (meshHit) {
			isPhysicsGrabbing = true;
			return;
		}
	}

	if (transformMode) {
		bool meshHit = StartTransform(event.GetPosition());
		if (meshHit) {
			isTransforming = true;
			SetHoverBone(std::string());
			return;
		}
	}

	if (pivotMode) {
		bool meshHit = StartPivotPosition(event.GetPosition());
		if (meshHit) {
			isMovingPivot = true;
			SetHoverBone(std::string());
			return;
		}
	}

	// A click on a shown bone selects it instead of using the active tool.
	// Ctrl adds it to or removes it from the selection.
	std::string pickedBone = PickBone(event.GetX(), event.GetY());
	if (!pickedBone.empty()) {
		isPickingBone = true;
		os->SelectBoneFromViewport(pickedBone, event.ControlDown());
		gls.RenderOneFrame();
		return;
	}

	if (brushMode) {
		bool meshHit = StartBrushStroke(event.GetPosition());
		if (meshHit)
			isPainting = true;
	}
	else if (activeTool == ToolID::CollapseVertex) {
		bool meshHit = StartPickVertex();
		if (meshHit)
			isPickingVertex = true;
	}
	else if (activeTool == ToolID::FlipEdge || activeTool == ToolID::SplitEdge) {
		bool meshHit = StartPickEdge();
		if (meshHit)
			isPickingEdge = true;
	}
	else if (activeTool == ToolID::MoveVertex) {
		if (GetToolOptionEdgeSlide()) {
			if (isSlidingEdge) {
				EndEdgeSlide();
				isSlidingEdge = false;
			}
			else {
				bool meshHit = StartEdgeSlide(event.GetPosition());
				if (meshHit)
					isSlidingEdge = true;
			}
		}
		else if (isMovingVertex) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else {
			bool meshHit = StartMoveVertex(event.GetPosition());
			if (meshHit)
				isMovingVertex = true;
		}
	}
	else if (vertexEdit) {
		bool meshHit = SelectVertex(event.GetPosition());
		if (meshHit)
			isSelecting = true;
	}
}

void wxGLPanel::OnMiddleDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	mbuttonDown = true;
}

void wxGLPanel::OnMiddleUp(wxMouseEvent& WXUNUSED(event)) {
	if (GetCapture() == this)
		ReleaseMouse();

	isMDragging = false;
	mbuttonDown = false;
}

void wxGLPanel::OnLeftUp(wxMouseEvent& event) {
	if (GetCapture() == this)
		ReleaseMouse();

	// OnLeftDown bails out during playback, so none of the edit states below can
	// be set; only the click-to-select path would still run, and it would pick
	// against the mesh BVHs that playback leaves stale.
	const bool playing = os && os->animation->IsPlaying();

	if (!playing && !isLDragging && !isPainting && !isPhysicsGrabbing && !isPickingBone && activeTool == ToolID::Select) {
		int x, y;
		event.GetPosition(&x, &y);

		Mesh* m = gls.PickMesh(x, y);
		if (m)
			os->SelectShape(m->shapeName);
	}

	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		if (mouseHasMovedSinceStart) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else
			mouseHasMovedSinceStart = true;
	}

	if (isSlidingEdge) {
		if (mouseHasMovedSinceStart) {
			EndEdgeSlide();
			isSlidingEdge = false;
		}
		else
			mouseHasMovedSinceStart = true;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isPickingBone = false;
	isLDragging = false;
	lbuttonDown = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		CancelMoveVertex();
		isMovingVertex = false;
	}

	if (isSlidingEdge) {
		CancelEdgeSlide();
		isSlidingEdge = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isPickingBone = false;
	isLDragging = false;
	lbuttonDown = false;

	isMDragging = false;
	mbuttonDown = false;

	rbuttonDown = false;
	isRDragging = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnRightDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	rbuttonDown = true;
	isRDragging = false;
	rDownPos = event.GetPosition();
}

void wxGLPanel::OnRightUp(wxMouseEvent& event) {
	if (HasCapture())
		ReleaseMouse();

	const bool wasDragging = isRDragging;
	rbuttonDown = false;
	isRDragging = false;

	// Right-clicking a shown bone without turning the camera opens the bone
	// context menu for it, to add bones as its children among other things.
	if (wasDragging || lbuttonDown || !os)
		return;

	// Connector bones only get the menu to add bones below them
	const std::string bone = PickBone(event.GetX(), event.GetY(), true);
	if (bone.empty())
		return;

	// The menu acts on the clicked bone: it becomes the active one, the parent
	// of added bones. A multi-selection it is part of stays, for deleting.
	if (bone != os->GetActiveBone())
		os->SelectBoneFromViewport(bone, false, true);

	SetHoverBone(std::string());
	gls.RenderOneFrame();

	os->ShowBoneContextMenu(bone, this, event.GetPosition());
}

void wxGLPanel::OnLeaveWindow(wxMouseEvent& event) {
	event.Skip();

	if (lbuttonDown || rbuttonDown || hoverBone.empty())
		return;

	SetHoverBone(std::string());
	gls.RenderOneFrame();
}
