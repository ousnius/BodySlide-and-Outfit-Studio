/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "EditUV.h"
#include "../render/GLOffscreenBuffer.h"

#include <regex>

extern ConfigurationManager Config;

using namespace nifly;

namespace {
constexpr float RopeSelectMinPointDistSq = 0.000025f;
constexpr size_t RopeSelectMaxPoints = 2048;
}

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
	EVT_MENU(XRCID("btnRopeSelection"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnVertexSelection"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnMove"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnScale"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnRotate"), EditUV::OnSelectTool)
	EVT_MENU(XRCID("btnSeamEdges"), EditUV::OnSeamEdges)
	EVT_MENU(XRCID("fileExportUVTemplate"), EditUV::OnExportUVTemplate)
	EVT_MENU(XRCID("editUndo"), EditUV::OnUndo)
	EVT_MENU(XRCID("editRedo"), EditUV::OnRedo)
	EVT_MENU(XRCID("editSelectAll"), EditUV::OnSelectAll)
	EVT_MENU(XRCID("editSelectInvert"), EditUV::OnSelectInvert)
	EVT_MENU(XRCID("editSelectLess"), EditUV::OnSelectLess)
	EVT_MENU(XRCID("editSelectMore"), EditUV::OnSelectMore)
	EVT_MENU(XRCID("maskSelection"), EditUV::OnMaskSelection)
	EVT_MENU(XRCID("editTranslate"), EditUV::OnTranslate)
	EVT_MENU(XRCID("editRotate"), EditUV::OnRotate)
	EVT_MENU(XRCID("editScale"), EditUV::OnScale)
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
#ifdef _WINDOWS
	canvas->MSWDisableComposited(); // Fix stuttering from composited flag?
#endif
}

EditUV::~EditUV() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/EditUV.xrc");
}

void EditUV::OnSelectTool(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == XRCID("btnBoxSelection"))
		SelectTool(EditUVTool::BoxSelection);
	else if (id == XRCID("btnRopeSelection"))
		SelectTool(EditUVTool::RopeSelection);
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

void EditUV::OnMaskSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!shapeMesh->mask)
		return;

	if (shapeMesh->nVerts != canvas->uvGridMesh->nVerts)
		return;

	for (int i = 0; i < canvas->uvGridMesh->nVerts; i++) {
		if (canvas->uvGridMesh->vcolors[i] == Vector3(1.0f, 1.0f, 0.0f))
			shapeMesh->mask[i] = 1.0f;
		else
			shapeMesh->mask[i] = 0.0f;
	}

	shapeMesh->QueueUpdate(Mesh::UpdateType::Mask);
	os->glView->Render();
}

void EditUV::OnExportUVTemplate(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgExportUV"))
		return;

	wxChoice* choiceResolution = XRCCTRL(dlg, "choiceResolution", wxChoice);
	wxColourPickerCtrl* cpWireColor = XRCCTRL(dlg, "cpWireColor", wxColourPickerCtrl);
	wxColourPickerCtrl* cpBackgroundColor = XRCCTRL(dlg, "cpBackgroundColor", wxColourPickerCtrl);
	wxCheckBox* cbTransparentBG = XRCCTRL(dlg, "cbTransparentBG", wxCheckBox);
	wxCheckBox* cbIncludeTexture = XRCCTRL(dlg, "cbIncludeTexture", wxCheckBox);

	auto onTransparentToggle = [&](wxCommandEvent&) {
		cpBackgroundColor->Enable(!cbTransparentBG->IsChecked());
	};

	cbTransparentBG->Bind(wxEVT_CHECKBOX, onTransparentToggle);
	cpBackgroundColor->Enable(!cbTransparentBG->IsChecked());

	if (dlg.ShowModal() != wxID_OK)
		return;

	int resolutions[] = {512, 1024, 2048, 4096};
	int resIndex = choiceResolution->GetSelection();
	if (resIndex < 0 || resIndex > 3)
		resIndex = 2;
	int resolution = resolutions[resIndex];

	wxColour wireColor = cpWireColor->GetColour();
	wxColour bgColor = cpBackgroundColor->GetColour();
	bool transparentBG = cbTransparentBG->IsChecked();
	bool includeTexture = cbIncludeTexture->IsChecked();

	wxChoice* choiceWrapMode = XRCCTRL(dlg, "choiceWrapMode", wxChoice);
	wxCheckBox* cbAntiAliasing = XRCCTRL(dlg, "cbAntiAliasing", wxCheckBox);
	bool clampUVs = choiceWrapMode->GetSelection() == 1;
	bool antiAliasing = cbAntiAliasing->IsChecked();

	wxFileDialog saveDialog(this,
		_("Save UV template image"),
		wxEmptyString,
		wxEmptyString,
		"PNG Files (*.png)|*.png|TGA Files (*.tga)|*.tga|BMP Files (*.bmp)|*.bmp|JPG Files (*.jpg)|*.jpg|DDS Files (*.dds)|*.dds",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() != wxID_OK)
		return;

	std::string filename = saveDialog.GetPath().ToUTF8().data();

	if (!canvas->ExportUVTemplate(filename, resolution, wireColor, bgColor, transparentBG, includeTexture, clampUVs, antiAliasing))
		wxMessageBox(_("Failed to export UV template."), _("Error"), wxICON_ERROR, this);
}

void EditUV::OnTranslate(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgTranslate")) {
		wxSlider* sliderU = XRCCTRL(dlg, "sliderU", wxSlider);
		wxSlider* sliderV = XRCCTRL(dlg, "sliderV", wxSlider);
		wxTextCtrl* textU = XRCCTRL(dlg, "textU", wxTextCtrl);
		wxTextCtrl* textV = XRCCTRL(dlg, "textV", wxTextCtrl);

		auto sliderMoved = [&](wxCommandEvent&) {
			float u = sliderU->GetValue() / 1000.0f;
			float v = sliderV->GetValue() / 1000.0f;

			textU->ChangeValue(wxString::Format("%0.5f", u));
			textV->ChangeValue(wxString::Format("%0.5f", v));
		};

		auto textChanged = [&](wxCommandEvent&) {
			float u = atof(textU->GetValue().c_str());
			float v = atof(textV->GetValue().c_str());

			sliderU->SetValue(u * 1000);
			sliderV->SetValue(v * 1000);
		};

		sliderU->Bind(wxEVT_SLIDER, sliderMoved);
		sliderV->Bind(wxEVT_SLIDER, sliderMoved);

		textU->Bind(wxEVT_TEXT, textChanged);
		textV->Bind(wxEVT_TEXT, textChanged);

		if (dlg.ShowModal() != wxID_OK)
			return;

		canvas->StartMeshAction();

		Vector3 translation;
		translation.x = atof(textU->GetValue().c_str());
		translation.y = atof(textV->GetValue().c_str());

		Matrix4 mat;
		mat.PushTranslate(translation);

		canvas->ApplyMeshTransform(mat);
		canvas->EndMeshAction();
	}
}

void EditUV::OnRotate(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotate")) {
		wxSlider* slider = XRCCTRL(dlg, "slider", wxSlider);
		wxTextCtrl* text = XRCCTRL(dlg, "text", wxTextCtrl);

		auto sliderMoved = [&](wxCommandEvent&) {
			float sliderValue = slider->GetValue() / 100.0f;
			text->ChangeValue(wxString::Format("%0.4f", sliderValue));
		};

		auto textChanged = [&](wxCommandEvent&) {
			float changedValue = atof(text->GetValue().c_str());
			slider->SetValue(changedValue * 100);
		};

		slider->Bind(wxEVT_SLIDER, sliderMoved);
		text->Bind(wxEVT_TEXT, textChanged);

		if (dlg.ShowModal() != wxID_OK)
			return;

		Vector3 currentCenter = canvas->CalcSelectionCenter();
		canvas->StartMeshAction();

		float rotation = atof(text->GetValue().c_str());

		Matrix4 mat;
		mat.PushTranslate(currentCenter);
		mat.PushRotate(rotation * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
		mat.PushTranslate(currentCenter * -1.0f);

		canvas->ApplyMeshTransform(mat);
		canvas->EndMeshAction();
	}
}

void EditUV::OnScale(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScale")) {
		wxSlider* sliderU = XRCCTRL(dlg, "sliderU", wxSlider);
		wxSlider* sliderV = XRCCTRL(dlg, "sliderV", wxSlider);
		wxTextCtrl* textU = XRCCTRL(dlg, "textU", wxTextCtrl);
		wxTextCtrl* textV = XRCCTRL(dlg, "textV", wxTextCtrl);

		auto sliderMoved = [&](wxCommandEvent& event) {
			Vector3 scale(1.0f, 1.0f, 1.0f);

			bool uniform = XRCCTRL(dlg, "cbUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / 1000.0f;
				scale = Vector3(uniformValue, uniformValue, uniformValue);

				XRCCTRL(dlg, "sliderU", wxSlider)->SetValue(scale.x * 1000);
				XRCCTRL(dlg, "sliderV", wxSlider)->SetValue(scale.y * 1000);
			}
			else {
				scale.x = XRCCTRL(dlg, "sliderU", wxSlider)->GetValue() / 1000.0f;
				scale.y = XRCCTRL(dlg, "sliderV", wxSlider)->GetValue() / 1000.0f;
			}

			XRCCTRL(dlg, "textU", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
			XRCCTRL(dlg, "textV", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
		};

		auto textChanged = [&](wxCommandEvent& event) {
			Vector3 scale(1.0f, 1.0f, 1.0f);

			bool uniform = XRCCTRL(dlg, "cbUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
				scale = Vector3(uniformValue, uniformValue, uniformValue);

				XRCCTRL(dlg, "textU", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
				XRCCTRL(dlg, "textV", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
			}
			else {
				scale.x = atof(XRCCTRL(dlg, "textU", wxTextCtrl)->GetValue().c_str());
				scale.y = atof(XRCCTRL(dlg, "textV", wxTextCtrl)->GetValue().c_str());
			}

			if (scale.x < 0.01f) {
				scale.x = 0.01f;
				XRCCTRL(dlg, "textU", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
			}

			if (scale.y < 0.01f) {
				scale.y = 0.01f;
				XRCCTRL(dlg, "textV", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
			}

			XRCCTRL(dlg, "sliderU", wxSlider)->SetValue(scale.x * 1000);
			XRCCTRL(dlg, "sliderV", wxSlider)->SetValue(scale.y * 1000);
		};

		sliderU->Bind(wxEVT_SLIDER, sliderMoved);
		sliderV->Bind(wxEVT_SLIDER, sliderMoved);

		textU->Bind(wxEVT_TEXT, textChanged);
		textV->Bind(wxEVT_TEXT, textChanged);

		if (dlg.ShowModal() != wxID_OK)
			return;

		Vector3 currentCenter = canvas->CalcSelectionCenter();
		canvas->StartMeshAction();

		float scaleU = atof(textU->GetValue().c_str());
		float scaleV = atof(textV->GetValue().c_str());

		Matrix4 mat;
		mat.PushTranslate(currentCenter);
		mat.PushScale(scaleU, scaleV, 1.0f);
		mat.PushTranslate(currentCenter * -1.0f);

		canvas->ApplyMeshTransform(mat);
		canvas->EndMeshAction();
	}
}

void EditUV::SelectTool(EditUVTool tool) {
	canvas->SetCursorType(GLSurface::None);
	toolSelected = tool;

	switch (toolSelected) {
		case EditUVTool::BoxSelection:
			canvas->SetCursor(wxStockCursor::wxCURSOR_CROSS);
			uvToolBar->ToggleTool(XRCID("btnBoxSelection"), true);
			break;
		case EditUVTool::RopeSelection:
			canvas->SetCursor(wxStockCursor::wxCURSOR_CROSS);
			uvToolBar->ToggleTool(XRCID("btnRopeSelection"), true);
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
	Render();
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
		else if (activeTool == EditUVTool::RopeSelection) {
			AddRopeSelectPoint(current);

			if (!wxGetKeyState(wxKeyCode::WXK_ALT))
				ropeSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
			else
				ropeSelectMesh->color = Vector3(0.0f, 1.0f, 0.0f);
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
		case EditUVTool::VertexSelection: break;

		case EditUVTool::BoxSelection:
			boxSelectMesh->verts[0] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[1] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[2] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->verts[3] = Vector3(click.x, click.y, 0.0f);
			boxSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
			boxSelectMesh->QueueUpdate(Mesh::UpdateType::Position);
			boxSelectMesh->bVisible = true;
			break;

		case EditUVTool::RopeSelection:
			ropeSelectPoints.clear();
			AddRopeSelectPoint(click, true);
			ropeSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
			ropeSelectMesh->bVisible = true;
			break;

		case EditUVTool::Move:
		case EditUVTool::Scale:
		case EditUVTool::Rotate:
			StartMeshAction();
			CalcSelectionCenter();
			lastAngle = std::atan2(currentCenter.y - click.y, currentCenter.x - click.x);
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

		case EditUVTool::RopeSelection: {
			Vector3 up;
			Vector3 d;
			uvSurface.GetPickRay(upX, upY, nullptr, d, up);
			AddRopeSelectPoint(up);
			ApplyRopeSelection(wxGetKeyState(wxKeyCode::WXK_ALT));
			ropeSelectMesh->bVisible = false;
			ropeSelectPoints.clear();
			break;
		}

		case EditUVTool::VertexSelection: SelectVertex(p, wxGetKeyState(wxKeyCode::WXK_ALT)); break;

		case EditUVTool::Move:
		case EditUVTool::Scale:
		case EditUVTool::Rotate: EndMeshAction(); break;
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
			case '2': editUV->SelectTool(EditUVTool::RopeSelection); break;
			case '3': editUV->SelectTool(EditUVTool::VertexSelection); break;
			case '4': editUV->SelectTool(EditUVTool::Move); break;
			case '5': editUV->SelectTool(EditUVTool::Scale); break;
			case '6': editUV->SelectTool(EditUVTool::Rotate); break;
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
			std::unordered_set<int> adjacentPoints;
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
	std::unordered_set<int> adjacentPoints;
	for (int i = 0; i < uvGridMesh->nVerts; i++)
		if (uvGridMesh->vcolors[i].x > 0.0f)
			uvGridMesh->GetAdjacentPoints(i, adjacentPoints);

	for (auto& adj : adjacentPoints)
		uvGridMesh->vcolors[adj].x = 1.0f;

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
	uvSurface.RenderOneFrame();
}

Vector3 EditUVCanvas::CalcSelectionCenter() {
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

	return currentCenter;
}

void EditUVCanvas::StartMeshAction() {
	std::unordered_map<int, Vector2> state;
	state.reserve(uvGridMesh->nVerts);

	for (int i = 0; i < uvGridMesh->nVerts; i++)
		state[i] = Vector2(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y);

	auto action = new EditUVAction();
	action->SetActionMesh(uvGridMesh);
	action->SetStartState(state);
	editUV->history.Add(action);
}

void EditUVCanvas::EndMeshAction() {
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
}

void EditUVCanvas::ApplyMeshTransform(const Matrix4& mat) {
	Mesh* m = editUV->shapeMesh;

	for (int i = 0; i < uvGridMesh->nVerts; i++) {
		if (uvGridMesh->vcolors[i] == Vector3(1.0f, 1.0f, 0.0f)) {
			uvGridMesh->verts[i] = mat * uvGridMesh->verts[i];

			m->texcoord[i].u = uvGridMesh->verts[i].x;
			m->texcoord[i].v = uvGridMesh->verts[i].y * -1.0f;
		}
	}

	uvGridMesh->QueueUpdate(Mesh::UpdateType::Position);

	m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	editUV->os->glView->Render();
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

	uvGridMesh->BuildVertexAdjacency();
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

	ropeSelectMesh = new Mesh();
	ropeSelectMesh->color = Vector3(1.0f, 1.0f, 0.0f);
	ropeSelectMesh->prop.alpha = 0.25f;
	ropeSelectMesh->alphaFlags = 4333;

	ropeSelectMaterial = GLMaterial(Config["AppDir"] + "/res/shaders/primitive.vert", Config["AppDir"] + "/res/shaders/primitive.frag");
	ropeSelectMesh->material = &ropeSelectMaterial;

	ropeSelectMesh->shapeName = "RopeSelect";
	ropeSelectMesh->doublesided = true;
	ropeSelectMesh->bVisible = false;

	uvSurface.AddOverlay(ropeSelectMesh);
	uvSurface.UpdateShaders(ropeSelectMesh);
}

bool EditUVCanvas::AddRopeSelectPoint(const Vector3& point, bool force) {
	if (ropeSelectPoints.size() >= RopeSelectMaxPoints)
		return false;

	Vector2 ropePoint(point.x, point.y);
	if (!force && !ropeSelectPoints.empty()) {
		Vector2 diff = ropePoint - ropeSelectPoints.back();
		if (diff.u * diff.u + diff.v * diff.v < RopeSelectMinPointDistSq)
			return false;
	}

	ropeSelectPoints.push_back(ropePoint);
	UpdateRopeSelectMesh();
	return true;
}

void EditUVCanvas::UpdateRopeSelectMesh() {
	if (!ropeSelectMesh)
		return;

	int pointCount = static_cast<int>(ropeSelectPoints.size());
	if (pointCount < 3) {
		ropeSelectMesh->bVisible = false;
		return;
	}

	if (!uvSurface.SetContext())
		return;

	ropeSelectMesh->nVerts = pointCount;
	ropeSelectMesh->nTris = pointCount - 2;
	ropeSelectMesh->verts = std::make_unique<Vector3[]>(ropeSelectMesh->nVerts);
	ropeSelectMesh->tris = std::make_unique<Triangle[]>(ropeSelectMesh->nTris);

	for (int i = 0; i < pointCount; i++)
		ropeSelectMesh->verts[i] = Vector3(ropeSelectPoints[i].u, ropeSelectPoints[i].v, 0.0f);

	for (int i = 0; i < ropeSelectMesh->nTris; i++)
		ropeSelectMesh->tris[i] = Triangle(0, i + 1, i + 2);

	ropeSelectMesh->bVisible = true;
	ropeSelectMesh->CreateBuffers();
}

void EditUVCanvas::ApplyRopeSelection(bool unselect) {
	if (ropeSelectPoints.size() < 3)
		return;

	bool additive = wxGetKeyState(wxKeyCode::WXK_SHIFT);
	for (int i = 0; i < uvGridMesh->nVerts; i++) {
		Vector2 uvPoint(uvGridMesh->verts[i].x, uvGridMesh->verts[i].y);
		if (PointInRopeSelection(uvPoint)) {
			uvGridMesh->vcolors[i].x = unselect ? 0.0f : 1.0f;
			uvGridMesh->vcolors[i].y = 1.0f;
			uvGridMesh->vcolors[i].z = 0.0f;
		}
		else if (!unselect && !additive) {
			uvGridMesh->vcolors[i].x = 0.0f;
			uvGridMesh->vcolors[i].y = 1.0f;
			uvGridMesh->vcolors[i].z = 0.0f;
		}
	}

	uvGridMesh->QueueUpdate(Mesh::UpdateType::VertexColors);
}

bool EditUVCanvas::PointInRopeSelection(const Vector2& point) const {
	bool inside = false;
	size_t pointCount = ropeSelectPoints.size();
	for (size_t i = 0, j = pointCount - 1; i < pointCount; j = i++) {
		const Vector2& pi = ropeSelectPoints[i];
		const Vector2& pj = ropeSelectPoints[j];
		bool crosses = ((pi.v > point.v) != (pj.v > point.v)) && (point.u < (pj.u - pi.u) * (point.v - pi.v) / (pj.v - pi.v) + pi.u);
		if (crosses)
			inside = !inside;
	}

	return inside;
}

bool EditUVCanvas::ExportUVTemplate(const std::string& filename, int resolution, const wxColour& wireColor, const wxColour& bgColor, bool transparentBG, bool includeTexture, bool clampUVs, bool antiAliasing) {
	if (!uvGridMesh)
		return false;

	uvSurface.SetContext();

	// Determine save format from file extension
	int saveType = SOIL_SAVE_TYPE_PNG;
	std::string ext = filename.substr(filename.find_last_of('.') + 1);
	if (ext == "tga" || ext == "TGA")
		saveType = SOIL_SAVE_TYPE_TGA;
	else if (ext == "bmp" || ext == "BMP")
		saveType = SOIL_SAVE_TYPE_BMP;
	else if (ext == "jpg" || ext == "JPG" || ext == "jpeg" || ext == "JPEG")
		saveType = SOIL_SAVE_TYPE_JPG;
	else if (ext == "dds" || ext == "DDS")
		saveType = SOIL_SAVE_TYPE_DDS;

	// Formats without alpha support: pick a contrasting background based on wire color brightness
	bool formatSupportsAlpha = (saveType == SOIL_SAVE_TYPE_PNG || saveType == SOIL_SAVE_TYPE_TGA || saveType == SOIL_SAVE_TYPE_DDS);
	bool useTransparency = transparentBG && formatSupportsAlpha;

	// MSAA: use 4x multisampling when anti-aliasing is enabled
	int samples = antiAliasing ? 4 : 0;

	// Save current state
	Vector3 savedCamPos = uvSurface.camPos;
	Vector3 savedCamOffset = uvSurface.camOffset;
	Vector3 savedCamRot = uvSurface.camRot;
	Vector3 savedCamRotOffset = uvSurface.camRotOffset;
	Vector3 savedBgColor = uvSurface.GetBackgroundColor();
	Vector3 savedGridColor = uvGridMesh->color;
	bool savedVertexColors = uvGridMesh->vertexColors;

	// Reset camera to default UV view (shows full 0-1 UV space)
	uvSurface.camPos = Vector3(-0.5f, 0.5f, -1.0f);
	uvSurface.camOffset = Vector3();
	uvSurface.camRot = Vector3();
	uvSurface.camRotOffset = Vector3();

	// Set up the projection for a square 1:1 viewport
	uint32_t savedW, savedH;
	uvSurface.GetSize(savedW, savedH);
	uvSurface.SetSize(resolution, resolution);
	uvSurface.UpdateProjection();

	// Create offscreen buffer (with optional MSAA)
	GLOffScreenBuffer offscreen(&uvSurface, resolution, resolution, 1, {}, samples);
	offscreen.Start();

	// Clear with background color
	if (useTransparency) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (transparentBG && !formatSupportsAlpha) {
		float luminance = wireColor.Red() / 255.0f * 0.299f + wireColor.Green() / 255.0f * 0.587f + wireColor.Blue() / 255.0f * 0.114f;
		float bg = luminance > 0.5f ? 0.0f : 1.0f;
		glClearColor(bg, bg, bg, 1.0f);
	}
	else {
		glClearColor(bgColor.Red() / 255.0f, bgColor.Green() / 255.0f, bgColor.Blue() / 255.0f, 1.0f);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Clamp: use scissor test to restrict rendering to 0-1 UV range
	if (clampUVs) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(0, 0, resolution, resolution);
	}

	// Render texture plane if requested
	if (includeTexture && planeMesh && planeMesh->material) {
		uvSurface.RenderMesh(planeMesh);
	}

	// Render UV wireframe with custom color (disable vertex colors, override mesh color)
	uvGridMesh->color = Vector3(wireColor.Red() / 255.0f, wireColor.Green() / 255.0f, wireColor.Blue() / 255.0f);
	uvGridMesh->vertexColors = false;
	uvSurface.UpdateShaders(uvGridMesh);
	uvSurface.RenderMesh(uvGridMesh);

	if (clampUVs) {
		glDisable(GL_SCISSOR_TEST);
	}

	// Resolve MSAA (no-op if samples == 0) and read pixels
	offscreen.Resolve();

	std::unique_ptr<GLubyte[]> pixels(new GLubyte[resolution * resolution * 4]);
	glReadPixels(0, 0, resolution, resolution, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

	offscreen.End();

	// Restore mesh state
	uvGridMesh->color = savedGridColor;
	uvGridMesh->vertexColors = savedVertexColors;
	uvSurface.UpdateShaders(uvGridMesh);

	// Restore camera and viewport
	uvSurface.camPos = savedCamPos;
	uvSurface.camOffset = savedCamOffset;
	uvSurface.camRot = savedCamRot;
	uvSurface.camRotOffset = savedCamRotOffset;
	uvSurface.SetBackgroundColor(savedBgColor);
	uvSurface.SetSize(savedW, savedH);

	// Flip vertically (glReadPixels reads bottom-to-top, all save formats expect top-to-bottom)
	{
		int rowBytes = resolution * 4;
		std::unique_ptr<GLubyte[]> rowBuf(new GLubyte[rowBytes]);
		for (int y = 0; y < resolution / 2; y++) {
			GLubyte* topRow = pixels.get() + y * rowBytes;
			GLubyte* botRow = pixels.get() + (resolution - 1 - y) * rowBytes;
			memcpy(rowBuf.get(), topRow, rowBytes);
			memcpy(topRow, botRow, rowBytes);
			memcpy(botRow, rowBuf.get(), rowBytes);
		}
	}

	int result = SOIL_save_image(filename.c_str(), saveType, resolution, resolution, 4, pixels.get());

	// Re-render the canvas
	Render();

	return result != 0;
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
