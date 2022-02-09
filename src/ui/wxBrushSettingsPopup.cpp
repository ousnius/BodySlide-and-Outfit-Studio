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
	auto onSliderSize = [&](wxCommandEvent& WXUNUSED(event)) {
		float slideSize = brushSize->GetValue() / 1000.0f;

		wxString valSizeStr = wxString::Format("%0.4f", slideSize);
		brushSizeVal->ChangeValue(valSizeStr);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			os->glView->SetBrushSize(slideSize);
			os->CheckBrushBounds();
		}
	};

	auto onSliderStrength = [&](wxCommandEvent& WXUNUSED(event)) {
		float slideStrength = brushStrength->GetValue() / 1000.0f;

		wxString valStrengthStr = wxString::Format("%0.4f", slideStrength);
		brushStrengthVal->ChangeValue(valStrengthStr);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setStrength(slideStrength);
			os->CheckBrushBounds();
		}
	};

	auto onSliderFocus = [&](wxCommandEvent& WXUNUSED(event)) {
		float slideFocus = brushFocus->GetValue() / 1000.0f;

		wxString valFocusStr = wxString::Format("%0.4f", slideFocus);
		brushFocusVal->ChangeValue(valFocusStr);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setFocus(slideFocus);
			os->CheckBrushBounds();
		}
	};

	auto onSliderSpacing = [&](wxCommandEvent& WXUNUSED(event)) {
		float slideSpacing = brushSpacing->GetValue() / 1000.0f;

		wxString valSpacingStr = wxString::Format("%0.4f", slideSpacing);
		brushSpacingVal->ChangeValue(valSpacingStr);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setSpacing(slideSpacing);
			os->CheckBrushBounds();
		}
	};

	auto onSliderTextChangedSize = [&](wxCommandEvent& WXUNUSED(event)) {
		float textSize = atof(brushSizeVal->GetValue().c_str());
		brushSize->SetValue(textSize * 1000);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			os->glView->SetBrushSize(textSize);
			os->CheckBrushBounds();
		}
	};

	auto onSliderTextChangedStrength = [&](wxCommandEvent& WXUNUSED(event)) {
		float textStrength = atof(brushStrengthVal->GetValue().c_str());
		brushStrength->SetValue(textStrength * 1000);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setStrength(textStrength);
			os->CheckBrushBounds();
		}
	};

	auto onSliderTextChangedFocus = [&](wxCommandEvent& WXUNUSED(event)) {
		float textFocus = atof(brushFocusVal->GetValue().c_str());
		brushFocus->SetValue(textFocus * 1000);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setFocus(textFocus);
			os->CheckBrushBounds();
		}
	};

	auto onSliderTextChangedSpacing = [&](wxCommandEvent& WXUNUSED(event)) {
		float textSpacing = atof(brushSpacingVal->GetValue().c_str());
		brushSpacing->SetValue(textSpacing * 1000);

		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setSpacing(textSpacing);
			os->CheckBrushBounds();
		}
	};

	const int sliderWidth = 250;
	const int textCtrlWidth = 50;

	wxFloatingPointValidator<float> floatValidator(4);
	floatValidator.SetRange(0.0, 10.0);

	panel = new wxPanel(popupWin);
	topSizer = new wxBoxSizer(wxVERTICAL);
	flexGridSizer = new wxFlexGridSizer(3, 0, 0);
	flexGridSizer->AddGrowableCol(1);

	lbBrushSize = new wxStaticText(panel, wxID_ANY, _("Size"));
	flexGridSizer->Add(lbBrushSize, 0, wxALL, 5);

	brushSize = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushSize->SetToolTip(_("Shortcut: 'S' + mouse wheel"));
	brushSize->Bind(wxEVT_SLIDER, onSliderSize);
	flexGridSizer->Add(brushSize, 1, wxALL, 5);

	brushSizeVal = new wxTextCtrl(panel, wxID_ANY, "0.0000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushSizeVal->SetValidator(floatValidator);
	brushSizeVal->Bind(wxEVT_TEXT, onSliderTextChangedSize);
	flexGridSizer->Add(brushSizeVal, 0, wxALL, 5);

	lbBrushStrength = new wxStaticText(panel, wxID_ANY, _("Strength"));
	flexGridSizer->Add(lbBrushStrength, 0, wxALL, 5);

	brushStrength = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushStrength->Bind(wxEVT_SLIDER, onSliderStrength);
	flexGridSizer->Add(brushStrength, 1, wxALL, 5);

	brushStrengthVal = new wxTextCtrl(panel, wxID_ANY, "0.0000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushStrengthVal->SetValidator(floatValidator);
	brushStrengthVal->Bind(wxEVT_TEXT, onSliderTextChangedStrength);
	flexGridSizer->Add(brushStrengthVal, 0, wxALL, 5);

	lbBrushFocus = new wxStaticText(panel, wxID_ANY, _("Focus"));
	flexGridSizer->Add(lbBrushFocus, 0, wxALL, 5);

	brushFocus = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushFocus->Bind(wxEVT_SLIDER, onSliderFocus);
	flexGridSizer->Add(brushFocus, 1, wxALL, 5);

	brushFocusVal = new wxTextCtrl(panel, wxID_ANY, "0.0000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushFocusVal->SetValidator(floatValidator);
	brushFocusVal->Bind(wxEVT_TEXT, onSliderTextChangedFocus);
	flexGridSizer->Add(brushFocusVal, 0, wxALL, 5);

	lbBrushSpacing = new wxStaticText(panel, wxID_ANY, _("Spacing"));
	flexGridSizer->Add(lbBrushSpacing, 0, wxALL, 5);

	brushSpacing = new wxSlider(panel, wxID_ANY, 0, 0, 1000, wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL);
	brushSpacing->Bind(wxEVT_SLIDER, onSliderSpacing);
	flexGridSizer->Add(brushSpacing, 1, wxALL, 5);

	brushSpacingVal = new wxTextCtrl(panel, wxID_ANY, "0.0000", wxDefaultPosition, wxSize(textCtrlWidth, -1));
	brushSpacingVal->SetValidator(floatValidator);
	brushSpacingVal->Bind(wxEVT_TEXT, onSliderTextChangedSpacing);
	flexGridSizer->Add(brushSpacingVal, 0, wxALL, 5);

	topSizer->Add(flexGridSizer, 0, wxALL, 0);
	panel->SetSizer(topSizer);

	topSizer->Fit(panel);
	popupWin->SetClientSize(panel->GetSize());
}

void wxBrushSettingsPopupBase::SetBrushSize(float value) {
	wxString valStr = wxString::Format("%0.4f", value);
	brushSize->SetValue(std::atof(valStr.c_str()) * 1000.0f);
	brushSizeVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushStrength(float value) {
	wxString valStr = wxString::Format("%0.4f", value);
	brushStrength->SetValue(std::atof(valStr.c_str()) * 1000.0f);
	brushStrengthVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushFocus(float value) {
	wxString valStr = wxString::Format("%0.4f", value);
	brushFocus->SetValue(std::atof(valStr.c_str()) * 1000.0f);
	brushFocusVal->ChangeValue(valStr);
}

void wxBrushSettingsPopupBase::SetBrushSpacing(float value) {
	wxString valStr = wxString::Format("%0.4f", value);
	brushSpacing->SetValue(std::atof(valStr.c_str()) * 1000.0f);
	brushSpacingVal->ChangeValue(valStr);
}


wxBrushSettingsPopupTransient::wxBrushSettingsPopupTransient(OutfitStudioFrame* parent, bool stayOpen)
	: wxPopupTransientWindow(parent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS)
	, wxBrushSettingsPopupBase(parent, this) {
	this->stayOpen = stayOpen;
}
