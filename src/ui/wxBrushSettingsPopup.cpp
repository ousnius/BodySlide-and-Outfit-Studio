/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxBrushSettingsPopup.h"

#include <wx/valnum.h>

wxBrushSettingsPopupBase::wxBrushSettingsPopupBase(OutfitStudioFrame* parent, wxWindow* popupWin) {
	os = parent;
	Setup(popupWin);
}

void wxBrushSettingsPopupBase::Setup(wxWindow* popupWin) {
	auto onBrushSettingsSlider = [&](wxCommandEvent& WXUNUSED(event)) {
		float slideSize = brushSize->GetValue() / 1000.0f;
		float slideStrength = brushStrength->GetValue() / 1000.0f;
		float slideFocus = brushFocus->GetValue() / 1000.0f;
		float slideSpacing = brushSpacing->GetValue() / 1000.0f;

		wxString valSizeStr = wxString::Format("%0.3f", slideSize);
		wxString valStrengthStr = wxString::Format("%0.3f", slideStrength);
		wxString valFocusStr = wxString::Format("%0.3f", slideFocus);
		wxString valSpacingStr = wxString::Format("%0.3f", slideSpacing);

		brushSizeVal->ChangeValue(valSizeStr);
		brushStrengthVal->ChangeValue(valStrengthStr);
		brushFocusVal->ChangeValue(valFocusStr);
		brushSpacingVal->ChangeValue(valSpacingStr);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			os->glView->SetBrushSize(slideSize);
			brush->setStrength(slideStrength);
			brush->setFocus(slideFocus);
			brush->setSpacing(slideSpacing);
			os->CheckBrushBounds();
		}
	};

	auto onBrushSettingsTextChanged = [&](wxCommandEvent& WXUNUSED(event)) {
		float textSize = atof(brushSizeVal->GetValue().c_str());
		float textStrength = atof(brushStrengthVal->GetValue().c_str());
		float textFocus = atof(brushFocusVal->GetValue().c_str());
		float textSpacing = atof(brushSpacingVal->GetValue().c_str());

		brushSize->SetValue(textSize * 1000);
		brushStrength->SetValue(textStrength * 1000);
		brushFocus->SetValue(textFocus * 1000);
		brushSpacing->SetValue(textSpacing * 1000);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			os->glView->SetBrushSize(textSize);
			brush->setStrength(textStrength);
			brush->setFocus(textFocus);
			brush->setSpacing(textSpacing);
			os->CheckBrushBounds();
		}
	};

	const int sliderWidth = 250;
	const int textCtrlWidth = 50;

	wxFloatingPointValidator<float> floatValidator(3);
	floatValidator.SetRange(0.0, 1.0);

	panel = new wxPanel(popupWin);
	topSizer = new wxBoxSizer(wxVERTICAL);
	flexGridSizer = new wxFlexGridSizer(3, 0, 0);
	flexGridSizer->AddGrowableCol(1);

	lbBrushSize = new wxStaticText(panel, wxID_ANY, _("Size"));
	flexGridSizer->Add(lbBrushSize, 0, wxALL, 5);

	brushSize = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushSize->SetToolTip(_("Shortcut: 'S' + mouse wheel"));
	brushSize->Bind(wxEVT_SLIDER, onBrushSettingsSlider);
	flexGridSizer->Add(brushSize, 1, wxALL, 5);

	brushSizeVal = new wxTextCtrl(panel, wxID_ANY, "0.000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushSizeVal->SetValidator(floatValidator);
	brushSizeVal->Bind(wxEVT_TEXT, onBrushSettingsTextChanged);
	flexGridSizer->Add(brushSizeVal, 0, wxALL, 5);

	lbBrushStrength = new wxStaticText(panel, wxID_ANY, _("Strength"));
	flexGridSizer->Add(lbBrushStrength, 0, wxALL, 5);

	brushStrength = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushStrength->Bind(wxEVT_SLIDER, onBrushSettingsSlider);
	flexGridSizer->Add(brushStrength, 1, wxALL, 5);

	brushStrengthVal = new wxTextCtrl(panel, wxID_ANY, "0.000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushStrengthVal->SetValidator(floatValidator);
	brushStrengthVal->Bind(wxEVT_TEXT, onBrushSettingsTextChanged);
	flexGridSizer->Add(brushStrengthVal, 0, wxALL, 5);

	lbBrushFocus = new wxStaticText(panel, wxID_ANY, _("Focus"));
	flexGridSizer->Add(lbBrushFocus, 0, wxALL, 5);

	brushFocus = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushFocus->Bind(wxEVT_SLIDER, onBrushSettingsSlider);
	flexGridSizer->Add(brushFocus, 1, wxALL, 5);

	brushFocusVal = new wxTextCtrl(panel, wxID_ANY, "0.000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushFocusVal->SetValidator(floatValidator);
	brushFocusVal->Bind(wxEVT_TEXT, onBrushSettingsTextChanged);
	flexGridSizer->Add(brushFocusVal, 0, wxALL, 5);

	lbBrushSpacing = new wxStaticText(panel, wxID_ANY, _("Spacing"));
	flexGridSizer->Add(lbBrushSpacing, 0, wxALL, 5);

	brushSpacing = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushSpacing->Bind(wxEVT_SLIDER, onBrushSettingsSlider);
	flexGridSizer->Add(brushSpacing, 1, wxALL, 5);

	brushSpacingVal = new wxTextCtrl(panel, wxID_ANY, "0.000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushSpacingVal->SetValidator(floatValidator);
	brushSpacingVal->Bind(wxEVT_TEXT, onBrushSettingsTextChanged);
	flexGridSizer->Add(brushSpacingVal, 0, wxALL, 5);

	topSizer->Add(flexGridSizer, 0, wxALL, 0);
	panel->SetSizer(topSizer);

	topSizer->Fit(panel);
	popupWin->SetClientSize(panel->GetSize());
}

void wxBrushSettingsPopupBase::SetBrushSize(float value) {
	brushSize->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	brushSizeVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushStrength(float value) {
	brushStrength->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	brushStrengthVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushFocus(float value) {
	brushFocus->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	brushFocusVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushSpacing(float value) {
	brushSpacing->SetValue(value * 1000.0f);
	wxString valStr = wxString::Format("%0.3f", value);
	brushSpacingVal->ChangeValue(valStr);
}


wxBrushSettingsPopupTransient::wxBrushSettingsPopupTransient(OutfitStudioFrame* parent, bool stayOpen)
	: wxPopupTransientWindow(parent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS),
	wxBrushSettingsPopupBase(parent, this) {
	this->stayOpen = stayOpen;
}
