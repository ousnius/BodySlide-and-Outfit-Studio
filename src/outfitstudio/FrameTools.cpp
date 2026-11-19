/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the editing tools and brush settings

#include "OutfitStudioFrame.h"
#include "GLPanel.h"
#include "ui/wxStateButton.h"
#include "components/TweakBrush.h"
#include "render/GLSurface.h"
#include "files/HkxFile.h"
#include "ui/outfitstudio/wxBrushSettingsPopup.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <utility>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::SelectTool(ToolID tool) {
	UpdateEdgeSlideToolOptionsUI(tool);

	if (tool == ToolID::Select) {
		glView->SetEditMode(false);
		glView->SetBrushMode(false);
		glView->SetActiveTool(tool);

		menuBar->Check(XRCID("btnSelect"), true);
		toolBarH->ToggleTool(XRCID("btnSelect"), true);

		ReEnableToolOptionsUI();
		ReToggleToolOptionsUI();
		return;
	}

	if (tool == ToolID::Transform) {
		int id = XRCID("btnTransform");
		bool state = !glView->GetTransformMode();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetTransformMode(state);
		return;
	}

	if (tool == ToolID::Pivot) {
		int id = XRCID("btnPivot");
		bool state = !glView->GetPivotMode();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetPivotMode(state);
		return;
	}

	if (tool == ToolID::VertexEdit) {
		int id = XRCID("btnVertexEdit");
		bool state = !glView->GetVertexEdit();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetVertexEdit(state);
		return;
	}

	glView->SetActiveTool(tool);
	glView->SetCursorType(GLSurface::BrushCursor);

	ReEnableToolOptionsUI();
	ReToggleToolOptionsUI();

	if (tool == ToolID::MaskBrush) {
		menuBar->Check(XRCID("btnMaskBrush"), true);
		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
	}
	else if (tool == ToolID::InflateBrush) {
		menuBar->Check(XRCID("btnInflateBrush"), true);
		toolBarH->ToggleTool(XRCID("btnInflateBrush"), true);
	}
	else if (tool == ToolID::DeflateBrush) {
		menuBar->Check(XRCID("btnDeflateBrush"), true);
		toolBarH->ToggleTool(XRCID("btnDeflateBrush"), true);
	}
	else if (tool == ToolID::MoveBrush) {
		menuBar->Check(XRCID("btnMoveBrush"), true);
		toolBarH->ToggleTool(XRCID("btnMoveBrush"), true);
	}
	else if (tool == ToolID::SmoothBrush) {
		menuBar->Check(XRCID("btnSmoothBrush"), true);
		toolBarH->ToggleTool(XRCID("btnSmoothBrush"), true);
	}
	else if (tool == ToolID::UndiffBrush) {
		menuBar->Check(XRCID("btnUndiffBrush"), true);
		toolBarH->ToggleTool(XRCID("btnUndiffBrush"), true);
	}
	else if (tool == ToolID::WeightBrush) {
		menuBar->Check(XRCID("btnWeightBrush"), true);
		toolBarH->ToggleTool(XRCID("btnWeightBrush"), true);
	}
	else if (tool == ToolID::ColorBrush) {
		menuBar->Check(XRCID("btnColorBrush"), true);
		toolBarH->ToggleTool(XRCID("btnColorBrush"), true);

		FindWindowById(XRCID("colorPalette"), colorSettings)->Show();
		FindWindowById(XRCID("btnMaskVertexColor"), colorSettings)->Show();
		FindWindowById(XRCID("clampMaxValue"), colorSettings)->Hide();
		colorSettings->Layout();
		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Alpha"));
	}
	else if (tool == ToolID::AlphaBrush) {
		menuBar->Check(XRCID("btnAlphaBrush"), true);
		toolBarH->ToggleTool(XRCID("btnAlphaBrush"), true);

		FindWindowById(XRCID("colorPalette"), colorSettings)->Hide();
		FindWindowById(XRCID("btnMaskVertexColor"), colorSettings)->Hide();
		FindWindowById(XRCID("clampMaxValue"), colorSettings)->Show();
		colorSettings->Layout();
		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Color"));
	}
	else if (tool == ToolID::CollapseVertex) {
		menuBar->Check(XRCID("btnCollapseVertex"), true);
		toolBarH->ToggleTool(XRCID("btnCollapseVertex"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::VertexCursor);
		return;
	}
	else if (tool == ToolID::FlipEdge) {
		menuBar->Check(XRCID("btnFlipEdgeTool"), true);
		toolBarH->ToggleTool(XRCID("btnFlipEdgeTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::EdgeCursor);
		return;
	}
	else if (tool == ToolID::SplitEdge) {
		menuBar->Check(XRCID("btnSplitEdgeTool"), true);
		toolBarH->ToggleTool(XRCID("btnSplitEdgeTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::EdgeCursor);
		return;
	}
	else if (tool == ToolID::MoveVertex) {
		menuBar->Check(XRCID("btnMoveVertexTool"), true);
		toolBarH->ToggleTool(XRCID("btnMoveVertexTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::VertexCursor);
		return;
	}
	else {
		glView->SetEditMode(false);
		glView->SetBrushMode(false);
		glView->SetTransformMode(false);
		return;
	}

	// One of the brushes was activated
	glView->SetEditMode();
	glView->SetBrushMode();
	glView->SetBrushSize(glView->GetBrushSize());

	CheckBrushBounds();
	UpdateBrushSettings();
}

void OutfitStudioFrame::UpdateEdgeSlideToolOptionsUI(ToolID tool) {
	bool isEdgeSlide = tool == ToolID::MoveVertex && glView->GetToolOptionEdgeSlide();
	if (!cbEdgeSlideCorrectUV)
		return;

	cbEdgeSlideCorrectUV->Show(isEdgeSlide);
	cbEdgeSlideCorrectUV->SetValue(isEdgeSlide && glView->GetToolOptionEdgeSlideUVCorrection());

	if (toolBarH) {
		toolBarH->Layout();
		toolBarH->Refresh();
	}
}

void OutfitStudioFrame::ReEnableToolOptionsUI() {
	bool isBrush = glView->GetActiveBrush() != nullptr;
	ToolID toolid = glView->GetActiveTool();
	bool isMV = toolid == ToolID::MoveVertex;
	bool isIB = toolid == ToolID::InflateBrush || toolid == ToolID::DeflateBrush;
	bool isMB = toolid == ToolID::MoveBrush;
	bool isSB = toolid == ToolID::SmoothBrush;

	UpdateEdgeSlideToolOptionsUI(toolid);

	menuBar->Enable(XRCID("btnXMirror"), isBrush || isMV);
	tbvHider.Show(XRCID("btnXMirror"), isBrush || isMV);
	menuBar->Enable(XRCID("btnConnected"), isBrush);
	tbvHider.Show(XRCID("btnConnected"), isBrush);
	menuBar->Enable(XRCID("btnMerge"), isMV);
	tbvHider.Show(XRCID("btnMerge"), isMV);
	menuBar->Enable(XRCID("btnWeld"), isMV);
	tbvHider.Show(XRCID("btnWeld"), isMV);
	tbvHider.Show(XRCID("btnEdgeSlide"), isMV);
	menuBar->Enable(XRCID("btnRestrictSurface"), isMV);
	tbvHider.Show(XRCID("btnRestrictSurface"), isMV);
	menuBar->Enable(XRCID("btnRestrictPlane"), isMV || isMB || isSB);
	tbvHider.Show(XRCID("btnRestrictPlane"), isMV || isMB || isSB);
	menuBar->Enable(XRCID("btnRestrictNormal"), isMV || isIB || isMB || isSB);
	tbvHider.Show(XRCID("btnRestrictNormal"), isMV || isIB || isMB || isSB);
}

void OutfitStudioFrame::ReToggleToolOptionsUI() {
	bool isBrush = glView->GetActiveBrush() != nullptr;
	ToolID toolid = glView->GetActiveTool();
	bool isMV = toolid == ToolID::MoveVertex;
	bool isIB = toolid == ToolID::InflateBrush || toolid == ToolID::DeflateBrush;
	bool isMB = toolid == ToolID::MoveBrush;
	bool isSB = toolid == ToolID::SmoothBrush;
	bool transformMode = glView->GetTransformMode();
	bool xMirror = glView->GetToolOptionXMirror();
	bool connOnly = glView->GetToolOptionConnectedOnly();
	bool merge = glView->GetToolOptionMerge();
	bool weld = glView->GetToolOptionWeld();
	bool rsurf = glView->GetToolOptionRestrictSurface();
	bool rplane = glView->GetToolOptionRestrictPlane();
	bool rnormal = glView->GetToolOptionRestrictNormal();
	bool edgeSlide = glView->GetToolOptionEdgeSlide();
	bool isES = isMV && edgeSlide;
	bool edgeSlideUVCorrection = glView->GetToolOptionEdgeSlideUVCorrection();

	if (cbEdgeSlideCorrectUV)
		cbEdgeSlideCorrectUV->SetValue(isES && edgeSlideUVCorrection);

	menuBar->Check(XRCID("btnTransform"), transformMode);
	toolBarV->ToggleTool(XRCID("btnTransform"), transformMode);
	menuBar->Check(XRCID("btnXMirror"), (isBrush || isMV) && xMirror);
	toolBarV->ToggleTool(XRCID("btnXMirror"), (isBrush || isMV) && xMirror);
	menuBar->Check(XRCID("btnConnected"), isBrush && connOnly);
	toolBarV->ToggleTool(XRCID("btnConnected"), isBrush && connOnly);
	menuBar->Check(XRCID("btnMerge"), isMV && merge);
	toolBarV->ToggleTool(XRCID("btnMerge"), isMV && merge);
	menuBar->Check(XRCID("btnWeld"), isMV && weld);
	toolBarV->ToggleTool(XRCID("btnWeld"), isMV && weld);
	toolBarV->ToggleTool(XRCID("btnEdgeSlide"), isES);
	menuBar->Check(XRCID("btnRestrictSurface"), isMV && rsurf);
	toolBarV->ToggleTool(XRCID("btnRestrictSurface"), isMV && rsurf);
	menuBar->Check(XRCID("btnRestrictPlane"), (isMV || isMB || isSB) && rplane);
	toolBarV->ToggleTool(XRCID("btnRestrictPlane"), (isMV || isMB || isSB) && rplane);
	menuBar->Check(XRCID("btnRestrictNormal"), (isMV || isIB || isMB || isSB) && rnormal);
	toolBarV->ToggleTool(XRCID("btnRestrictNormal"), (isMV || isIB || isMB || isSB) && rnormal);
}

void OutfitStudioFrame::CloseBrushSettings() {
	if (brushSettingsPopupTransient) {
		brushSettingsPopupTransient->Destroy();
		brushSettingsPopupTransient = nullptr;
	}
}

void OutfitStudioFrame::PopupBrushSettings(wxWindow* popupAt) {
	CloseBrushSettings();

	if (!glView->GetBrushMode())
		return;

	wxPoint popupPos;
	wxSize popupSize;

	if (popupAt) {
		popupPos = popupAt->GetScreenPosition();
		popupSize.y = popupAt->GetSize().y;
	}
	else {
		popupPos = wxGetMousePosition();
		popupSize.x = 10;
		popupSize.y = 10;
	}

	bool stayOpen = popupAt != nullptr;
	brushSettingsPopupTransient = new wxBrushSettingsPopupTransient(this, stayOpen);
	brushSettingsPopupTransient->Position(popupPos, popupSize);
	brushSettingsPopupTransient->Popup();

	UpdateBrushSettings();
}

void OutfitStudioFrame::UpdateBrushSettings() {
	TweakBrush* brush = glView->GetActiveBrush();
	if (!brush)
		return;

	if (brushSettingsPopupTransient) {
		brushSettingsPopupTransient->SetBrushName(wxString::FromUTF8(brush->Name()));
		brushSettingsPopupTransient->SetBrushSize(glView->GetBrushSize());
		brushSettingsPopupTransient->SetBrushStrength(brush->getStrength());
		brushSettingsPopupTransient->SetBrushFocus(brush->getFocus());
		brushSettingsPopupTransient->SetBrushSpacing(brush->getSpacing());
	}
}

void OutfitStudioFrame::CheckBrushBounds() {
	TweakBrush* brush = glView->GetActiveBrush();
	if (!brush)
		return;

	float size = glView->GetBrushSize();
	float strength = brush->getStrength();
	//float focus = brush->getFocus();
	//float spacing = brush->getSpacing();

	if (size >= 1.0f)
		menuBar->Enable(XRCID("btnIncreaseSize"), false);
	else
		menuBar->Enable(XRCID("btnIncreaseSize"), true);

	if (size <= 0.0f)
		menuBar->Enable(XRCID("btnDecreaseSize"), false);
	else
		menuBar->Enable(XRCID("btnDecreaseSize"), true);

	if (strength >= 1.0f)
		menuBar->Enable(XRCID("btnIncreaseStr"), false);
	else
		menuBar->Enable(XRCID("btnIncreaseStr"), true);

	if (strength <= 0.0f)
		menuBar->Enable(XRCID("btnDecreaseStr"), false);
	else
		menuBar->Enable(XRCID("btnDecreaseStr"), true);
}

// The tool each tool button selects
static const std::pair<const char*, ToolID> ToolButtons[] = {
	{"btnSelect", ToolID::Select},
	{"btnTransform", ToolID::Transform},
	{"btnPivot", ToolID::Pivot},
	{"btnVertexEdit", ToolID::VertexEdit},
	{"btnMaskBrush", ToolID::MaskBrush},
	{"btnInflateBrush", ToolID::InflateBrush},
	{"btnDeflateBrush", ToolID::DeflateBrush},
	{"btnMoveBrush", ToolID::MoveBrush},
	{"btnSmoothBrush", ToolID::SmoothBrush},
	{"btnUndiffBrush", ToolID::UndiffBrush},
	{"btnWeightBrush", ToolID::WeightBrush},
	{"btnColorBrush", ToolID::ColorBrush},
	{"btnAlphaBrush", ToolID::AlphaBrush},
	{"btnCollapseVertex", ToolID::CollapseVertex},
	{"btnFlipEdgeTool", ToolID::FlipEdge},
	{"btnSplitEdgeTool", ToolID::SplitEdge},
	{"btnMoveVertexTool", ToolID::MoveVertex},
};

void OutfitStudioFrame::OnSelectTool(wxCommandEvent& event) {
	ToolID tool = ToolID::Any;
	for (auto& button : ToolButtons) {
		if (event.GetId() == XRCID(button.first)) {
			tool = button.second;
			break;
		}
	}

	SelectTool(tool);

	// Remember last standard tool used in regular tabs
	if (meshTabButton->GetCheck() || lightsTabButton->GetCheck()) {
		auto activeBrush = glView->GetActiveBrush();
		if (activeBrush) {
			TweakBrush::BrushType brushType = activeBrush->Type();
			if (brushType == TweakBrush::BrushType::Inflate || brushType == TweakBrush::BrushType::Mask || brushType == TweakBrush::BrushType::Move) {
				glView->SetLastTool(glView->GetActiveTool());
			}
		}
	}
}

void OutfitStudioFrame::OnBrushSettings(wxCommandEvent& WXUNUSED(event)) {
	if (brushSettingsPopupTransient && brushSettingsPopupTransient->IsShown())
		CloseBrushSettings();
	else
		PopupBrushSettings(brushSettings);
}

void OutfitStudioFrame::OnEdgeSlideCorrectUV(wxCommandEvent& event) {
	glView->SetToolOptionEdgeSlideUVCorrection(event.IsChecked());
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnBrushColorChanged(wxColourPickerEvent& event) {
	wxColour color = event.GetColour();
	Vector3 brushColor;
	brushColor.x = color.Red() / 255.0f;
	brushColor.y = color.Green() / 255.0f;
	brushColor.z = color.Blue() / 255.0f;
	glView->SetColorBrush(brushColor);
}

void OutfitStudioFrame::OnColorClampMaxValueSlider(wxCommandEvent& WXUNUSED(event)) {
	wxSlider* slider = (wxSlider*)colorSettings->FindWindowByName("cpClampMaxValueSlider");
	wxTextCtrl* txtControl = (wxTextCtrl*)colorSettings->FindWindowByName("cpClampMaxValueTxt");
	int clampMaxValue = slider->GetValue();
	txtControl->SetValue(wxString::Format("%d", clampMaxValue));

	ClampBrush* clampBrush = dynamic_cast<ClampBrush*>(glView->GetActiveBrush());
	if (clampBrush)
		clampBrush->clampMaxValue = clampMaxValue / 255.0f;
}

void OutfitStudioFrame::OnColorClampMaxValueChanged(wxCommandEvent& WXUNUSED(event)) {
	wxTextCtrl* txtControl = (wxTextCtrl*)colorSettings->FindWindowByName("cpClampMaxValueTxt");
	wxSlider* slider = (wxSlider*)colorSettings->FindWindowByName("cpClampMaxValueSlider");
	int clampMaxValue = std::clamp(atoi(txtControl->GetValue().c_str()), 0, 255);
	slider->SetValue(clampMaxValue);

	if (!glView)
		return;
	ClampBrush* clampBrush = dynamic_cast<ClampBrush*>(glView->GetActiveBrush());
	if (clampBrush)
		clampBrush->clampMaxValue = clampMaxValue / 255.0f;
}

void OutfitStudioFrame::OnSwapBrush(wxCommandEvent& WXUNUSED(event)) {
	ToolID activeTool = glView->GetActiveTool();
	if (activeTool == ToolID::ColorBrush)
		SelectTool(ToolID::AlphaBrush);
	else if (activeTool == ToolID::AlphaBrush)
		SelectTool(ToolID::ColorBrush);
}

void OutfitStudioFrame::OnXMirror(wxCommandEvent& event) {
	glView->SetToolOptionXMirror(event.IsChecked());
	if (event.IsChecked() && glView->GetActiveTool() == ToolID::MoveVertex) {
		glView->SetToolOptionWeld(false);
		glView->SetToolOptionMerge(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnConnectedOnly(wxCommandEvent& event) {
	glView->SetToolOptionConnectedOnly(event.IsChecked());
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionMerge(wxCommandEvent& event) {
	glView->SetToolOptionMerge(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictSurface(true);
		glView->SetToolOptionXMirror(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionWeld(wxCommandEvent& event) {
	glView->SetToolOptionWeld(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictSurface(true);
		glView->SetToolOptionXMirror(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionRestrictSurface(wxCommandEvent& event) {
	glView->SetToolOptionRestrictSurface(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictPlane(false);
		glView->SetToolOptionEdgeSlide(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionRestrictPlane(wxCommandEvent& event) {
	glView->SetToolOptionRestrictPlane(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictSurface(false);
		glView->SetToolOptionRestrictNormal(false);
		glView->SetToolOptionEdgeSlide(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionRestrictNormal(wxCommandEvent& event) {
	glView->SetToolOptionRestrictNormal(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictPlane(false);
		glView->SetToolOptionEdgeSlide(false);
	}
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnToolOptionEdgeSlide(wxCommandEvent& event) {
	glView->SetToolOptionEdgeSlide(event.IsChecked());
	if (event.IsChecked()) {
		glView->SetToolOptionRestrictSurface(false);
		glView->SetToolOptionRestrictPlane(false);
		glView->SetToolOptionRestrictNormal(false);
	}
	ReEnableToolOptionsUI();
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnIncBrush(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetActiveBrush() && glView->GetBrushSize() < 1.0f) {
		float v = glView->IncBrush() / BrushSizeScale;
		if (statusBar)
			statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);

		CheckBrushBounds();
		UpdateBrushSettings();
	}
}

void OutfitStudioFrame::OnDecBrush(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetActiveBrush() && glView->GetBrushSize() > 0.0f) {
		float v = glView->DecBrush() / BrushSizeScale;
		if (statusBar)
			statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);

		CheckBrushBounds();
		UpdateBrushSettings();
	}
}

void OutfitStudioFrame::OnIncStr(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() < 1.0f) {
		float v = glView->IncStr();
		if (statusBar)
			statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);

		CheckBrushBounds();
		UpdateBrushSettings();
	}
}

void OutfitStudioFrame::OnDecStr(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() > 0.0f) {
		float v = glView->DecStr();
		if (statusBar)
			statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);

		CheckBrushBounds();
		UpdateBrushSettings();
	}
}
