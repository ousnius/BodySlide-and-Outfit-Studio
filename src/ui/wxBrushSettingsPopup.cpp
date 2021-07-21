/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxBrushSettingsPopup.h"

wxBrushSettingsPopup::wxBrushSettingsPopup(OutfitStudioFrame* parent)
	: wxPopupTransientWindow(parent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS) {
	os = parent;

	panel = new wxPanel(this);
	topSizer = new wxBoxSizer(wxVERTICAL);
	flexGridSizer = new wxFlexGridSizer(3, 0, 0);
	flexGridSizer->AddGrowableCol(1);

	lbBrushSize = new wxStaticText(panel, wxID_ANY, _("Size"));
	flexGridSizer->Add(lbBrushSize, 0, wxALL, 5);

	brushSize = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(150, -1), wxSL_BOTH | wxSL_HORIZONTAL);
	brushSize->SetToolTip(_("Shortcut: 'S' + mouse wheel"));
	brushSize->Bind(wxEVT_SLIDER, &wxBrushSettingsPopup::OnBrushSettingsSlider, this);
	flexGridSizer->Add(brushSize, 1, wxALL, 5);

	lbBrushSizeVal = new wxStaticText(panel, wxID_ANY, "0.000");
	flexGridSizer->Add(lbBrushSizeVal, 0, wxALL, 5);

	lbBrushStrength = new wxStaticText(panel, wxID_ANY, _("Strength"));
	flexGridSizer->Add(lbBrushStrength, 0, wxALL, 5);

	brushStrength = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(150, -1), wxSL_BOTH | wxSL_HORIZONTAL);
	brushStrength->Bind(wxEVT_SLIDER, &wxBrushSettingsPopup::OnBrushSettingsSlider, this);
	flexGridSizer->Add(brushStrength, 1, wxALL, 5);

	lbBrushStrengthVal = new wxStaticText(panel, wxID_ANY, "0.000");
	flexGridSizer->Add(lbBrushStrengthVal, 0, wxALL, 5);

	lbBrushFocus = new wxStaticText(panel, wxID_ANY, _("Focus"));
	flexGridSizer->Add(lbBrushFocus, 0, wxALL, 5);

	brushFocus = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(150, -1), wxSL_BOTH | wxSL_HORIZONTAL);
	brushFocus->Bind(wxEVT_SLIDER, &wxBrushSettingsPopup::OnBrushSettingsSlider, this);
	flexGridSizer->Add(brushFocus, 1, wxALL, 5);

	lbBrushFocusVal = new wxStaticText(panel, wxID_ANY, "0.000");
	flexGridSizer->Add(lbBrushFocusVal, 0, wxALL, 5);

	lbBrushSpacing = new wxStaticText(panel, wxID_ANY, _("Spacing"));
	flexGridSizer->Add(lbBrushSpacing, 0, wxALL, 5);

	brushSpacing = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(150, -1), wxSL_BOTH | wxSL_HORIZONTAL);
	brushSpacing->Bind(wxEVT_SLIDER, &wxBrushSettingsPopup::OnBrushSettingsSlider, this);
	flexGridSizer->Add(brushSpacing, 1, wxALL, 5);

	lbBrushSpacingVal = new wxStaticText(panel, wxID_ANY, "0.000");
	flexGridSizer->Add(lbBrushSpacingVal, 0, wxALL, 5);

	topSizer->Add(flexGridSizer, 0, wxALL, 0);
	panel->SetSizer(topSizer);

	topSizer->Fit(panel);
	SetClientSize(panel->GetSize());
}

wxBrushSettingsPopup::~wxBrushSettingsPopup() {
}

void wxBrushSettingsPopup::OnBrushSettingsSlider(wxCommandEvent& WXUNUSED(event)) {
	float slideSize = brushSize->GetValue() / 1000.0f;
	float slideStrength = brushStrength->GetValue() / 1000.0f;
	float slideFocus = brushFocus->GetValue() / 1000.0f;
	float slideSpacing = brushSpacing->GetValue() / 1000.0f;

	wxString valSizeStr = wxString::Format("%0.3f", slideSize);
	wxString valStrengthStr = wxString::Format("%0.3f", slideStrength);
	wxString valFocusStr = wxString::Format("%0.3f", slideFocus);
	wxString valSpacingStr = wxString::Format("%0.3f", slideSpacing);

	lbBrushSizeVal->SetLabel(valSizeStr);
	lbBrushStrengthVal->SetLabel(valStrengthStr);
	lbBrushFocusVal->SetLabel(valFocusStr);
	lbBrushSpacingVal->SetLabel(valSpacingStr);

	TweakBrush* brush = os->glView->GetActiveBrush();
	if (brush) {
		os->glView->SetBrushSize(slideSize);
		brush->setStrength(slideStrength);
		brush->setFocus(slideFocus);
		brush->setSpacing(slideSpacing);
		os->CheckBrushBounds();
	}
}

void wxBrushSettingsPopup::SetBrushSize(float value) {
	brushSize->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	lbBrushSizeVal->SetLabel(valStr);
}

void wxBrushSettingsPopup::SetBrushStrength(float value) {
	brushStrength->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	lbBrushStrengthVal->SetLabel(valStr);
}

void wxBrushSettingsPopup::SetBrushFocus(float value) {
	brushFocus->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	lbBrushFocusVal->SetLabel(valStr);
}

void wxBrushSettingsPopup::SetBrushSpacing(float value) {
	brushSpacing->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	lbBrushSpacingVal->SetLabel(valStr);
}
