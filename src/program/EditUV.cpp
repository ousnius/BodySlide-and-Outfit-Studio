/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "EditUV.h"
#include <regex>

extern ConfigurationManager Config;

using namespace nifly;

std::unordered_map<int, Vector2>& EditUVAction::GetStartState() {
	return startState;
}

std::unordered_map<int, Vector2>& EditUVAction::GetEndState() {
	return endState;
}

void EditUVAction::SetStartState(std::unordered_map<int, Vector2>& state) {
	startState = std::move(state);
}

void EditUVAction::SetEndState(std::unordered_map<int, Vector2>& state) {
	endState = std::move(state);
}

void EditUVAction::RestoreStartState() {
	for (auto& stateIt : startState) {
		actionMesh->verts[stateIt.first].x = stateIt.second.u;
		actionMesh->verts[stateIt.first].y = stateIt.second.v;
	}

	actionMesh->QueueUpdate(Mesh::UpdateType::Position);
}

void EditUVAction::RestoreEndState() {
	for (auto& stateIt : endState) {
		actionMesh->verts[stateIt.first].x = stateIt.second.u;
		actionMesh->verts[stateIt.first].y = stateIt.second.v;
	}

	actionMesh->QueueUpdate(Mesh::UpdateType::Position);
}


EditUVHistory::EditUVHistory() {}

EditUVHistory::~EditUVHistory() {
	Clear();
}

void EditUVHistory::Clear() {
	for (unsigned int i = 0; i < actions.size(); i++)
		delete actions[i];

	actions.clear();
	curState = -1;
}

void EditUVHistory::Add(EditUVAction* action) {
	int maxState = actions.size() - 1;
	if (curState < maxState) {
		for (auto strokeIt = actions.begin() + (curState + 1); strokeIt != actions.end(); ++strokeIt)
			delete (*strokeIt);

		actions.erase(actions.begin() + (curState + 1), actions.end());
	}
	else if (actions.size() == EDITUV_MAX_UNDO) {
		delete actions[0];
		actions.erase(actions.begin());
	}

	actions.push_back(action);
	curState = actions.size() - 1;
}

bool EditUVHistory::Back() {
	if (curState > -1) {
		actions[curState]->RestoreStartState();
		curState--;
		return true;
	}

	return false;
}

bool EditUVHistory::Forward() {
	int maxState = actions.size() - 1;
	if (curState < maxState) {
		actions[curState + 1]->RestoreEndState();
		curState++;
		return true;
	}

	return false;
}


wxBEGIN_EVENT_TABLE(EditUV, wxFrame)
	EVT_MENU(XRCID("btnBoxSelection"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnVertexSelection"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnMove"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnScale"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnRotate"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnSeamEdges"), EditUV::OnSeamEdges)
	EVT_MENU(XRCID("editUndo"), EditUV::OnUndo)
	EVT_MENU(XRCID("editRedo"), EditUV::OnRedo)
	EVT_MENU(XRCID("editSelectAll"), EditUV::OnSelectAll)
	EVT_MENU(XRCID("editSelectInvert"), EditUV::OnSelectInvert)
	EVT_MENU(XRCID("editSelectLess"), EditUV::OnSelectLess)
	EVT_MENU(XRCID("editSelectMore"), EditUV::OnSelectMore)
	EVT_BUTTON(wxID_OK, EditUV::OnApply)
	EVT_BUTTON(wxID_CANCEL, EditUV::OnCancel)
	EVT_CLOSE(EditUV::OnClose)
wxEND_EVENT_TABLE()

EditUV::EditUV(wxWindow* parent, NifFile* srcNif, NiShape* srcShape, Mesh* srcMesh, const std::string& srcSliderName) {
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/EditUV.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load EditUV.xrc file!", "Error", wxICON_ERROR);
		return;
	}

	loaded = xrc->LoadFrame(this, parent, "dlgEditUV");
	if (!loaded) {
		wxMessageBox("Failed to load EditUV frame!", "Error", wxICON_ERROR);
		return;
	}

	os = (OutfitStudioFrame*)parent;
	nif = srcNif;
	shape = srcShape;
	shapeMesh = srcMesh;
	sliderName = srcSliderName;

	uvToolBar = xrc->LoadToolBar(this, "uvToolBar");
	uvMenuBar = xrc->LoadMenuBar(this, "uvMenuBar");

	canvas = new EditUVCanvas(this, wxDefaultSize, GLSurface::GetGLAttribs());
	canvas->SetNotifyWindow(this);
	canvas->SetCursor(wxStockCursor::wxCURSOR_CROSS);

	xrc->AttachUnknownControl("uvGLView", canvas, this);
}

EditUV::~EditUV() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/EditUV.xrc");
}

void EditUV::OnSelectTool(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == XRCID("btnBoxSelection"))
		SelectTool(EditUVTool::BoxSelection);
	else if (id == XRCID("btnVertexSelection"))
		SelectTool(EditUVTool::VertexSelection);
	else if (id == XRCID("btnMove"))
		SelectTool(EditUVTool::Move);
	else if (id == XRCID("btnScale"))
		SelectTool(EditUVTool::Scale);
	else if (id == XRCID("btnRotate"))
		SelectTool(EditUVTool::Rotate);
}

void EditUV::OnSeamEdges(wxCommandEvent& event) {
	if (canvas->seamEdgesMesh) {
		canvas->seamEdgesMesh->bVisible = event.IsChecked();
		os->glView->Render();
	}
}

void EditUV::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	Undo();
}

void EditUV::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	Redo();
}

void EditUV::OnSelectAll(wxCommandEvent& WXUNUSED(event)) {
	canvas->SelectAll();
}

void EditUV::OnSelectInvert(wxCommandEvent& WXUNUSED(event)) {
	canvas->SelectInvert();
}

void EditUV::OnSelectLess(wxCommandEvent& WXUNUSED(event)) {
	canvas->SelectLess();
}

void EditUV::OnSelectMore(wxCommandEvent& WXUNUSED(event)) {
	canvas->SelectMore();
}

void EditUV::SelectTool(EditUVTool tool) {
	canvas->SetCursorType(GLSurface::None);
	toolSelected = tool;

	switch (toolSelected) {
		case EditUVTool::BoxSelection:
			canvas->SetCursor(wxStockCursor::wxCURSOR_CROSS);
			uvToolBar->ToggleTool(XRCID("btnBoxSelection"), true);
			break;
		case EditUVTool::VertexSelection:
			canvas->SetCursor(wxStockCursor::wxCURSOR_DEFAULT);
			canvas->SetCursorType(GLSurface::PointCursor);
			uvToolBar->ToggleTool(XRCID("btnVertexSelection"), true);
			break;
		case EditUVTool::Move:
			canvas->SetCursor(wxStockCursor::wxCURSOR_SIZING);
			uvToolBar->ToggleTool(XRCID("btnMove"), true);
			break;
		case EditUVTool::Scale:
			canvas->SetCursor(wxStockCursor::wxCURSOR_SIZING);
			uvToolBar->ToggleTool(XRCID("btnScale"), true);
			break;
		case EditUVTool::Rotate:
			canvas->SetCursor(wxStockCursor::wxCURSOR_HAND);
			uvToolBar->ToggleTool(XRCID("btnRotate"), true);
			break;
	}
}

void EditUV::Undo() {
	history.Back();
	canvas->Render();

	UpdateShapeMesh(false);
}

void EditUV::Redo() {
	history.Forward();
	canvas->Render();

	UpdateShapeMesh(false);
}

void EditUV::UpdateShapeMesh(bool apply) {
	std::vector<Vector2> uvs;
	nif->GetUvsForShape(shape, uvs);

	if (!sliderName.empty()) {
		std::unordered_map<uint16_t, Vector3> morphDiff;
		os->project->GetSliderDiffUV(shape, sliderName, uvs);

		for (int i = 0; i < canvas->uvGridMesh->nVerts; i++) {
			Vector3 diff = Vector3((canvas->uvGridMesh->verts[i].x - uvs[i].u) / -10.0f, 0.0f, ((canvas->uvGridMesh->verts[i].y * -1.0f) - uvs[i].v) / 10.0f);
			if (!diff.IsZero(true))
				morphDiff[i] = std::move(diff);

			shapeMesh->texcoord[i].u = canvas->uvGridMesh->verts[i].x;
			shapeMesh->texcoord[i].v = canvas->uvGridMesh->verts[i].y * -1.0f;
		}

		if (apply)
			os->project->UpdateMorphResult(shape, sliderName, morphDiff);
	}
	else {
		for (int i = 0; i < canvas->uvGridMesh->nVerts; i++) {
			uvs[i].u = canvas->uvGridMesh->verts[i].x;
			uvs[i].v = canvas->uvGridMesh->verts[i].y * -1.0f;
			shapeMesh->texcoord[i].u = uvs[i].u;
			shapeMesh->texcoord[i].v = uvs[i].v;
		}

		if (apply)
			nif->SetUvsForShape(shape, uvs);
	}

	shapeMesh->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	os->glView->Render();
}

void EditUV::OnApply(wxCommandEvent& WXUNUSED(event)) {
	UpdateShapeMesh();
	os->SetPendingChanges();
	Close();
}

void EditUV::OnCancel(wxCommandEvent& WXUNUSED(event)) {
	os->ApplySliders(false);
	Close();
}

void EditUV::OnClose(wxCloseEvent& WXUNUSED(event)) {
	if (canvas)
		delete canvas;

	Destroy();
}


wxBEGIN_EVENT_TABLE(EditUVCanvas, wxGLCanvas)
	EVT_PAINT(EditUVCanvas::OnPaint)
	EVT_SIZE(EditUVCanvas::OnSize)
	EVT_MOUSEWHEEL(EditUVCanvas::OnMouseWheel)
	EVT_MOTION(EditUVCanvas::OnMouseMove)
	EVT_LEFT_DOWN(EditUVCanvas::OnLeftDown)
	EVT_LEFT_DCLICK(EditUVCanvas::OnLeftDown)
	EVT_LEFT_UP(EditUVCanvas::OnLeftUp)
	EVT_MIDDLE_DOWN(EditUVCanvas::OnMiddleDown)
	EVT_MIDDLE_UP(EditUVCanvas::OnMiddleUp)
	EVT_RIGHT_DOWN(EditUVCanvas::OnRightDown)
	EVT_RIGHT_UP(EditUVCanvas::OnRightUp)
	EVT_KEY_DOWN(EditUVCanvas::OnKeyDown)
wxEND_EVENT_TABLE()

EditUVCanvas::EditUVCanvas(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs)
	: wxGLCanvas(parent, attribs, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE) {
	context = std::make_unique<wxGLContext>(this, nullptr, &GLSurface::GetGLContextAttribs());
}

EditUVCanvas::~EditUVCanvas() {
	editUV->os->glView->gls.DeleteMesh(seamEdgesMesh);
	editUV->os->glView->Render();

	uvSurface.Cleanup();
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::OnShown() {
	if (!context->IsOK()) {
		wxLogError("Outfit Studio: OpenGL context is not OK.");
		wxMessageBox(_("Outfit Studio: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR, editUV);
	}

	uvSurface.Initialize(this, context.get());

	auto size = GetSize();
	uvSurface.SetStartingView(Vector3(-0.5f, 0.5f, -1.0f), Vector3(), size.GetWidth(), size.GetHeight());
	uvSurface.SetPerspective(false);

	uvSurface.ToggleLighting();
	uvSurface.SetVertexColors();

	InitMeshes();
}

void EditUVCanvas::OnPaint(wxPaintEvent& event) {
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		OnShown();
	}

	uvSurface.RenderOneFrame();
	event.Skip();
}

void EditUVCanvas::OnSize(wxSizeEvent& event) {
	wxSize sz = event.GetSize();
	uvSurface.SetSize(sz.GetX(), sz.GetY());
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::OnMouseWheel(wxMouseEvent& event) {
	int delt = event.GetWheelRotation() / 10;
	uvSurface.DollyCamera(delt);
	uvSurface.ClampCameraPosition('Z', -10.0f, -0.1f);
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::OnMouseMove(wxMouseEvent& event) {
	if (editUV->IsActive())
		SetFocus();

	int x = 0;
	int y = 0;
	event.GetPosition(&x, &y);
	wxPoint p(x, y);

	EditUVTool activeTool = editUV->GetActiveTool();

	if (mbuttonDown) {
		isMDragging = true;
		if (wxGetKeyState(wxKeyCode::WXK_SHIFT)) {
			uvSurface.DollyCamera(y - lastY);
			uvSurface.ClampCameraPosition('Z', -10.0f, -0.1f);
		}
		else
			uvSurface.PanCamera(x - lastX, y - lastY);

		uvSurface.RenderOneFrame();
	}

	if (rbuttonDown) {
		isRDragging = true;
		//uvSurface.RenderOneFrame();
	}

	if (lbuttonDown) {
		isLDragging = true;

		Vector3 start;
		Vector3 current;
		Vector3 last;
		Vector3 d;
		uvSurface.GetPickRay(clickX, clickY, nullptr, d, start);
		uvSurface.GetPickRay(x, y, nullptr, d, current);
		uvSurface.GetPickRay(lastX, lastY, nullptr, d, last);

		Rect rect;
		Mesh* m = editUV->shapeMesh;

		if (activeTool == EditUVTool::BoxSelection) {
			// Draw normalized rectangle from start to current
			rect.SetTopLeft(Vector2(start.x, start.y));
			rect.SetBottomRight(Vector2(current.x, current.y));
			rect = rect.Normalized();

			boxSelectMesh->verts[0].x = rect.GetTopLeft().u;
			boxSelectMesh->verts[0].y = rect.GetTopLeft().v;

			boxSelectMesh->verts[1].x = rect.GetTopRight().u;
			boxSelectMesh->verts[1].y = rect.GetTopRight().v;

			boxSelectMesh->verts[2].x = rect.GetBottomRight().u;
			boxSelectMesh->verts[2].y = rect.GetBottomRight().v;

			boxSelectMesh->verts[3].x = rect.GetBottomLeft().u;
			boxSelectMesh->verts[3].y = rect.GetBottomLeft().v;

			if (!wxGetKeyState(wxKeyCode::WXK_ALT)) {
				boxSelectMesh->color.x = 1.0f;
				boxSelectMesh->color.y = 1.0f;
				boxSelectMesh->color.z = 0.0f;
			}
			else {
				boxSelectMesh->color.x = 0.0f;
				boxSelectMesh->color.y = 1.0f;
				boxSelectMesh->color.z = 0.0f;
			}

			boxSelectMesh->QueueUpdate(Mesh::UpdateType::Position);
		}
		else if (activeTool == EditUVTool::VertexSelection) {
			SelectVertex(p, wxGetKeyState(wxKeyCode::WXK_ALT));
		}
		else if (activeTool == EditUVTool::Move) {
			// Move alongside cursor
			for (int i = 0; i < uvGridMesh->nVerts; i++) {
				if (uvGridMesh->vcolors[i] == Vector3(1.0f, 1.0f, 0.0f)) {
					uvGridMesh->verts[i].x += current.x - last.x;
					uvGridMesh->verts[i].y += current.y - last.y;

					m->texcoord[i].u = uvGridMesh->verts[i].x;
					m->texcoord[i].v = uvGridMesh->verts[i].y * -1.0f;
				}
			}

			uvGridMesh->QueueUpdate(Mesh::UpdateType::Position);

			m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
			editUV->os->glView->Render();
		}
		else if (activeTool == EditUVTool::Scale) {
			// Store the initial direction
			if (lastDirection == EDITUV_DIRECTION_NONE) {
				if (current.x > currentCenter.x)
					lastDirection |= EDITUV_DIRECTION_RIGHT;
				else
					lastDirection |= EDITUV_DIRECTION_LEFT;

				if (current.y > currentCenter.y)
					lastDirection |= EDITUV_DIRECTION_DOWN;
				else
					lastDirection |= EDITUV_DIRECTION_UP;
			}

			float angle = std::atan2(currentCenter.y - current.y, currentCenter.x - current.x) * 180.0f / PI;
			float angleAbs = std::fabs(angle);

			// Set cursor depending on the angle to the center
			if ((angle >= -22.5f && angle < 22.5f) || (angleAbs >= 157.5f && angleAbs <= 180.0f))
				SetCursor(wxStockCursor::wxCURSOR_SIZEWE);
			else if ((angle >= 22.5f && angle < 67.5f) || (angle >= -157.5f && angle < -112.5))
				SetCursor(wxStockCursor::wxCURSOR_SIZENESW);
			else if (angleAbs >= 67.5f && angleAbs < 112.5f)
				SetCursor(wxStockCursor::wxCURSOR_SIZENS);
			else if ((angle >= 112.5f && angle < 157.5f) || (angle >= -67.5f && angle < -22.5f))
				SetCursor(wxStockCursor::wxCURSOR_SIZENWSE);

			// Scale up or down depending on the initial direction
			Vector3 scale(1.0f, 1.0f, 0.0f);

			if (lastDirection & EDITUV_DIRECTION_RIGHT)
				scale.x += current.x - start.x;
			else
				scale.x += start.x - current.x;

			if (lastDirection & EDITUV_DIRECTION_DOWN)
				scale.y += current.y - start.y;
			else
				scale.y += start.y - current.y;

			// Shift enables uniform scaling
			if (wxGetKeyState(wxKeyCode::WXK_SHIFT)) {
				if (scale.x > scale.y)
					scale.y = scale.x;
				else
					scale.x = scale.y;
			}

			auto curState = editUV->history.GetCurState();
			if (curState) {
				auto& startState = curState->GetStartState();

				// Scale around the selection center
				for (auto& s : startState) {
					if (uvGridMesh->vcolors[s.first] == Vector3(1.0f, 1.0f, 0.0f)) {
						Vector3 startPos(s.second.u, s.second.v, 0.0f);
						uvGridMesh->verts[s.first] = (startPos - currentCenter).ComponentMultiply(scale) + currentCenter;

						m->texcoord[s.first].u = uvGridMesh->verts[s.first].x;
						m->texcoord[s.first].v = uvGridMesh->verts[s.first].y * -1.0f;
					}
				}

				uvGridMesh->QueueUpdate(Mesh::UpdateType::Position);

				m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
				editUV->os->glView->Render();
			}
		}
		else if (activeTool == EditUVTool::Rotate) {
			float angle = std::atan2(currentCenter.y - current.y, currentCenter.x - current.x);
			float angleDiff = angle - lastAngle;
			float angleSin = std::sin(angleDiff);
			float angleCos = std::cos(angleDiff);

			auto curState = editUV->history.GetCurState();
			if (curState) {
				auto& startState = curState->GetStartState();

				// Rotate around the selection center
				for (auto& s : startState) {
					if (uvGridMesh->vcolors[s.first] == Vector3(1.0f, 1.0f, 0.0f)) {
						auto& vert = uvGridMesh->verts[s.first];
						Vector3 pos(s.second.u, s.second.v, 0.0f);
						pos -= currentCenter;

						vert.x = pos.x * angleCos - pos.y * angleSin + currentCenter.x;
						vert.y = pos.x * angleSin + pos.y * angleCos + currentCenter.y;

						m->texcoord[s.first].u = vert.x;
						m->texcoord[s.first].v = vert.y * -1.0f;
					}
				}

				uvGridMesh->QueueUpdate(Mesh::UpdateType::Position);

				m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
				editUV->os->glView->Render();
			}
		}

		uvSurface.RenderOneFrame();
	}

	if ((!lbuttonDown && !rbuttonDown && !mbuttonDown) || activeTool == EditUVTool::VertexSelection) {
		UpdateCursor(x, y, "UVGrid");
		uvSurface.RenderOneFrame();
	}

	lastX = x;
	lastY = y;
}

void EditUVCanvas::OnLeftDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	currentCenter.Zero();
	lastDirection = EDITUV_DIRECTION_NONE;

	event.GetPosition(&clickX, &clickY);

	Vector3 click;
	Vector3 d;
	uvSurface.GetPickRay(clickX, clickY, nullptr, d, click);

	editUV->StartTool();

	switch (editUV->GetActiveTool()) {
		case EditUVTool::VertexSelection:
			break;

		case EditUVTool::BoxSelection:
			boxSelectMesh->verts[0] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[1] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[2] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[3] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
			boxSelectMesh->QueueUpdate(Mesh::UpdateType::Position);
			boxSelectMesh->bVisible = true;
			break;

		case EditUVTool::Move:
		case EditUVTool::Scale:
		case EditUVTool::Rotate:
			// Calculate the selection center
			int count = 0;

			for (int i = 0; i < uvGridMesh->nVerts; i++) {
				if (uvGridMesh->vcolors[i] == Vector3(1.0f, 1.0f, 0.0f)) {
					currentCenter = currentCenter + uvGridMesh->verts[i];
					count++;
				}
			}

			if (count > 0)
				currentCenter = currentCenter / count;
			else
				currentCenter.Zero();

			lastAngle = std::atan2(currentCenter.y - click.y, currentCenter.x - click.x);

			std::unordered_map<int, Vector2> state;
			state.reserve(uvGridMesh->nVerts);

			for (int i = 0; i < uvGridMesh->nVerts; i++)
				state[i] = Vector2(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y);

			auto action = new EditUVAction();
			action->SetActionMesh(uvGridMesh);
			action->SetStartState(state);
			editUV->history.Add(action);
			break;
	}

	lbuttonDown = true;
}

void EditUVCanvas::OnLeftUp(wxMouseEvent& event) {
	if (HasCapture())
		ReleaseMouse();

	event.GetPosition(&upX, &upY);
	wxPoint p(upX, upY);

	Rect rect;

	switch (editUV->GetActiveTool()) {
		case EditUVTool::BoxSelection:
			rect.SetTopLeft(Vector2(boxSelectMesh->verts[0].x, boxSelectMesh->verts[0].y));
			rect.SetBottomRight(Vector2(boxSelectMesh->verts[2].x, boxSelectMesh->verts[2].y));

			if (!wxGetKeyState(wxKeyCode::WXK_ALT)) {
				for (int i = 0; i < uvGridMesh->nVerts; i++) {
					if (rect.Contains(Vector2(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y))) {
						uvGridMesh->vcolors[i].x = 1.0f;
						uvGridMesh->vcolors[i].y = 1.0f;
						uvGridMesh->vcolors[i].z = 0.0f;
					}
					else {
						if (!wxGetKeyState(wxKeyCode::WXK_SHIFT)) {
							uvGridMesh->vcolors[i].x = 0.0f;
							uvGridMesh->vcolors[i].y = 1.0f;
							uvGridMesh->vcolors[i].z = 0.0f;
						}
					}
				}
			}
			else {
				for (int i = 0; i < uvGridMesh->nVerts; i++) {
					if (rect.Contains(Vector2(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y))) {
						uvGridMesh->vcolors[i].x = 0.0f;
						uvGridMesh->vcolors[i].y = 1.0f;
						uvGridMesh->vcolors[i].z = 0.0f;
					}
				}
			}

			uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
			boxSelectMesh->bVisible = false;
			break;

		case EditUVTool::VertexSelection: SelectVertex(p, wxGetKeyState(wxKeyCode::WXK_ALT)); break;

		case EditUVTool::Move:
		case EditUVTool::Scale:
		case EditUVTool::Rotate:
			std::unordered_map<int, Vector2> state;
			state.reserve(uvGridMesh->nVerts);

			for (int i = 0; i < uvGridMesh->nVerts; i++)
				state[i] = Vector2(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y);

			auto action = editUV->history.GetCurState();
			if (action)
				action->SetEndState(state);

			if (editUV->GetActiveTool() == EditUVTool::Rotate)
				SetCursor(wxStockCursor::wxCURSOR_HAND);
			else
				SetCursor(wxStockCursor::wxCURSOR_SIZING);
			break;
	}

	isLDragging = false;
	lbuttonDown = false;

	uvSurface.RenderOneFrame();
}

void EditUVCanvas::OnMiddleDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	mbuttonDown = true;
}

void EditUVCanvas::OnMiddleUp(wxMouseEvent& WXUNUSED(event)) {
	if (HasCapture())
		ReleaseMouse();

	isMDragging = false;
	mbuttonDown = false;
}

void EditUVCanvas::OnRightDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	rbuttonDown = true;
}

void EditUVCanvas::OnRightUp(wxMouseEvent& WXUNUSED(event)) {
	if (HasCapture())
		ReleaseMouse();

	rbuttonDown = false;
}

void EditUVCanvas::OnKeyDown(wxKeyEvent& event) {
	if (!lbuttonDown && !rbuttonDown && !mbuttonDown) {
		switch (event.GetKeyCode()) {
			case '1': editUV->SelectTool(EditUVTool::BoxSelection); break;
			case '2': editUV->SelectTool(EditUVTool::VertexSelection); break;
			case '3': editUV->SelectTool(EditUVTool::Move); break;
			case '4': editUV->SelectTool(EditUVTool::Scale); break;
			case '5': editUV->SelectTool(EditUVTool::Rotate); break;
		}
	}
}

void EditUVCanvas::SetNotifyWindow(wxWindow* win) {
	editUV = dynamic_cast<EditUV*>(win);
}

void EditUVCanvas::SelectAll() {
	for (int i = 0; i < uvGridMesh->nVerts; i++) {
		uvGridMesh->vcolors[i].x = 1.0f;
		uvGridMesh->vcolors[i].y = 1.0f;
		uvGridMesh->vcolors[i].z = 0.0f;
	}

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::SelectInvert() {
	for (int i = 0; i < uvGridMesh->nVerts; i++)
		uvGridMesh->vcolors[i].x = uvGridMesh->vcolors[i].x == 1.0f ? 0.0f : 1.0f;

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::SelectLess() {
	std::set<int> unselectPoints;
	for (int i = 0; i < uvGridMesh->nVerts; i++) {
		if (uvGridMesh->vcolors[i].x > 0.0f) {
			std::set<int> adjacentPoints;
			uvGridMesh->GetAdjacentPoints(i, adjacentPoints);

			for (auto& adj : adjacentPoints) {
				if (uvGridMesh->vcolors[adj].x == 0.0f) {
					unselectPoints.insert(i);
					break;
				}
			}
		}
	}

	for (auto& up : unselectPoints)
		uvGridMesh->vcolors[up].x = 0.0f;

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::SelectMore() {
	std::set<int> adjacentPoints;
	for (int i = 0; i < uvGridMesh->nVerts; i++)
		if (uvGridMesh->vcolors[i].x > 0.0f)
			uvGridMesh->GetAdjacentPoints(i, adjacentPoints);

	for (auto& adj : adjacentPoints)
		uvGridMesh->vcolors[adj].x = 1.0f;

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	uvSurface.RenderOneFrame();
}

void EditUVCanvas::InitMeshes() {
	auto project = editUV->os->project;
	auto glView = editUV->os->glView;

	auto seamEdgesRefMesh = glView->GetMesh(editUV->shape->name.get());
	if (seamEdgesRefMesh) {
		seamEdgesMesh = glView->gls.AddVisSeamEdges(seamEdgesRefMesh, true);
		glView->Render();
	}

	planeMesh = uvSurface.AddVisPlane(Matrix4(), Vector2(64.0f, 64.0f), 64.0f);
	if (planeMesh) {
		std::string texFile;
		editUV->nif->GetTextureSlot(editUV->shape, texFile, 0);

		// Replace all backward slashes with one forward slash
		texFile = std::regex_replace(texFile, std::regex("\\\\+"), "/");

		// Remove everything before the first occurence of "/textures/"
		texFile = std::regex_replace(texFile, std::regex("^(.*?)/textures/", std::regex_constants::icase), "");

		// Remove all slashes from the front
		texFile = std::regex_replace(texFile, std::regex("^/+"), "");

		// If the path doesn't start with "textures/", add it to the front
		texFile = std::regex_replace(texFile, std::regex("^(?!^textures/)", std::regex_constants::icase), "textures/");

		std::string texturesDir = Config["GameDataPath"];
		texFile = texturesDir + texFile;

		std::vector<std::string> textures(1, texFile);
		std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
		std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";
		planeMesh->material = uvSurface.GetResourceLoader()->AddMaterial(textures, vShader, fShader);
		uvSurface.UpdateShaders(planeMesh);
	}

	std::vector<Vector3> verts(editUV->shape->GetNumVertices());
	std::vector<Vector2> uvs(editUV->shape->GetNumVertices());
	editUV->nif->GetUvsForShape(editUV->shape, uvs);

	if (uvs.size() != verts.size()) {
		editUV->Close();
		return;
	}

	if (!editUV->sliderName.empty())
		project->GetSliderDiffUV(editUV->shape, editUV->sliderName, uvs);

	std::vector<Triangle> tris;
	editUV->shape->GetTriangles(tris);

	uvGridMesh = new Mesh();
	uvGridMesh->nVerts = verts.size();
	uvGridMesh->nTris = tris.size();

	if (uvGridMesh->nVerts > 0) {
		uvGridMesh->verts = std::make_unique<Vector3[]>(uvGridMesh->nVerts);
		uvGridMesh->vcolors = std::make_unique<Vector3[]>(uvGridMesh->nVerts);
	}

	if (uvGridMesh->nTris > 0)
		uvGridMesh->tris = std::make_unique<Triangle[]>(uvGridMesh->nTris);

	for (int v = 0; v < uvGridMesh->nVerts; v++) {
		uvGridMesh->verts[v].x = uvs[v].u;
		uvGridMesh->verts[v].y = uvs[v].v * -1.0f;
		uvGridMesh->vcolors[v] = Vector3(0.0f, 1.0f, 0.0f);
	}

	for (int t = 0; t < uvGridMesh->nTris; t++)
		uvGridMesh->tris[t] = tris[t];

	uvGridMesh->rendermode = Mesh::RenderMode::LitWire;
	uvGridMesh->color = Vector3(1.0f, 0.0f, 0.0f);
	uvGridMesh->vertexColors = true;

	uvGridMaterial = GLMaterial(Config["AppDir"] + "/res/shaders/primitive.vert", Config["AppDir"] + "/res/shaders/primitive.frag");
	uvGridMesh->material = &uvGridMaterial;
	uvGridMesh->shapeName = "UVGrid";

	uvGridMesh->BuildTriAdjacency();
	uvGridMesh->BuildEdgeList();
	uvGridMesh->CreateBVH();
	uvGridMesh->CreateBuffers();
	uvSurface.AddOverlay(uvGridMesh);
	uvSurface.UpdateShaders(uvGridMesh);

	boxSelectMesh = new Mesh();
	boxSelectMesh->nVerts = 4;
	boxSelectMesh->nTris = 2;

	boxSelectMesh->verts = std::make_unique<Vector3[]>(boxSelectMesh->nVerts);
	boxSelectMesh->tris = std::make_unique<Triangle[]>(boxSelectMesh->nTris);

	boxSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
	boxSelectMesh->prop.alpha = 0.25f;
	boxSelectMesh->alphaFlags = 4333;

	boxSelectMesh->tris[0] = Triangle(0, 1, 2);
	boxSelectMesh->tris[1] = Triangle(2, 3, 0);

	boxSelectMaterial = GLMaterial(Config["AppDir"] + "/res/shaders/primitive.vert", Config["AppDir"] + "/res/shaders/primitive.frag");
	boxSelectMesh->material = &boxSelectMaterial;

	boxSelectMesh->shapeName = "BoxSelect";
	boxSelectMesh->doublesided = true;
	boxSelectMesh->bVisible = false;

	boxSelectMesh->CreateBuffers();
	uvSurface.AddOverlay(boxSelectMesh);
	uvSurface.UpdateShaders(boxSelectMesh);
}

void EditUVCanvas::UpdateCursor(int ScreenX, int ScreenY, const std::string& meshName) {
	hoverPoint = -1;

	auto m = uvSurface.GetOverlay(meshName);
	if (!m)
		return;

	Vector3 o;
	Vector3 d;
	uvSurface.GetPickRay(ScreenX, ScreenY, m, d, o);

	o.z = 0.0f;
	//d = d * -1.0f;

	std::vector<IntersectResult> results;
	if (m->bvh && m->bvh->IntersectRay(o, d, &results)) {
		if (results.size() > 0) {
			size_t min_i = 0;
			float minDist = results[0].HitDistance;
			for (size_t i = 1; i < results.size(); i++) {
				if (results[i].HitDistance < minDist) {
					minDist = results[i].HitDistance;
					min_i = i;
				}
			}

			Vector3 origin = results[min_i].HitCoord;

			Triangle t = m->tris[results[min_i].HitFacet];

			Vector3 hilitepoint = m->verts[t.p1];
			float closestdist = fabs(m->verts[t.p1].DistanceTo(origin));
			float nextdist = fabs(m->verts[t.p2].DistanceTo(origin));
			int pointid = t.p1;

			if (nextdist < closestdist) {
				closestdist = nextdist;
				hilitepoint = m->verts[t.p2];
				pointid = t.p2;
			}
			nextdist = fabs(m->verts[t.p3].DistanceTo(origin));
			if (nextdist < closestdist) {
				hilitepoint = m->verts[t.p3];
				pointid = t.p3;
			}

			hoverPoint = pointid;

			Vector3 visPoint = m->TransformPosMeshToModel(hilitepoint);
			auto visPointMesh = uvSurface.AddVisPoint(visPoint, "pointhilite");
			if (visPointMesh)
				visPointMesh->color = Vector3(1.0f, 0.0f, 0.0f);
		}
	}

	uvSurface.ShowCursor(hoverPoint >= 0 ? true : false);
}

bool EditUVCanvas::SelectVertex(const wxPoint& screenPos, bool unselect) {
	int vertIndex;
	if (!uvSurface.GetCursorVertex(screenPos.x, screenPos.y, &vertIndex, uvGridMesh))
		return false;

	if (unselect)
		uvGridMesh->vcolors[vertIndex].x = 0.0f;
	else
		uvGridMesh->vcolors[vertIndex].x = 1.0f;

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	return true;
}
