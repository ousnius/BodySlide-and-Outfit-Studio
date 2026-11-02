/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxBrushSettingsPopup.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {
constexpr int SliderSteps = 1000;

// Size and strength are mostly used at small values, so their sliders map position
// to value on a square curve to give the low end more travel.
float SliderToValue(int pos, bool squared) {
	float p = pos / static_cast<float>(SliderSteps);
	return squared ? p * p : p;
}

int ValueToSlider(float value, bool squared) {
	if (value < 0.0f)
		value = 0.0f;

	float p = squared ? std::sqrt(value) : value;
	return static_cast<int>(std::round(p * SliderSteps));
}
}

wxBrushSettingsPopupBase::wxBrushSettingsPopupBase(OutfitStudioFrame* parent, wxWindow* popupWin) {
	os = parent;
	Setup(popupWin);
}

void wxBrushSettingsPopupBase::Setup(wxWindow* popupWin) {
	auto applySize = [&](float value) {
		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			os->glView->SetBrushSize(value);
			os->CheckBrushBounds();
		}
	};

	auto applyStrength = [&](float value) {
		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setStrength(value);
			os->CheckBrushBounds();
		}
	};

	auto applyFocus = [&](float value) {
		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setFocus(value);
			os->CheckBrushBounds();
		}
	};

	auto applySpacing = [&](float value) {
		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->setSpacing(value);
			os->CheckBrushBounds();
		}
	};

	auto onSliderSize = [&, applySize](wxCommandEvent& WXUNUSED(event)) {
		float slideSize = SliderToValue(brushSize->GetValue(), true);
		brushSizeVal->SetValue(slideSize);
		applySize(slideSize);
	};

	auto onSliderStrength = [&, applyStrength](wxCommandEvent& WXUNUSED(event)) {
		float slideStrength = SliderToValue(brushStrength->GetValue(), true);
		brushStrengthVal->SetValue(slideStrength);
		applyStrength(slideStrength);
	};

	auto onSliderFocus = [&, applyFocus](wxCommandEvent& WXUNUSED(event)) {
		float slideFocus = SliderToValue(brushFocus->GetValue(), false);
		brushFocusVal->SetValue(slideFocus);
		applyFocus(slideFocus);
	};

	auto onSliderSpacing = [&, applySpacing](wxCommandEvent& WXUNUSED(event)) {
		float slideSpacing = SliderToValue(brushSpacing->GetValue(), false);
		brushSpacingVal->SetValue(slideSpacing);
		applySpacing(slideSpacing);
	};

	// Spin events cover the arrows, the mouse wheel and committed text,
	// text events apply values while they are still being typed.
	auto bindValueCtrl = [](wxSpinCtrlDouble* ctrl, wxSlider* slider, bool squared, std::function<void(float)> apply) {
		ctrl->Bind(wxEVT_SPINCTRLDOUBLE, [ctrl, slider, squared, apply](wxSpinDoubleEvent& event) {
			float value = static_cast<float>(event.GetValue());
			slider->SetValue(ValueToSlider(value, squared));
			apply(value);
		});

		ctrl->Bind(wxEVT_TEXT, [ctrl, slider, squared, apply](wxCommandEvent& WXUNUSED(event)) {
			double value = 0.0;
			if (!ctrl->GetTextValue().ToDouble(&value))
				return;

			value = std::clamp(value, ctrl->GetMin(), ctrl->GetMax());
			slider->SetValue(ValueToSlider(static_cast<float>(value), squared));
			apply(static_cast<float>(value));
		});
	};

	auto onButtonReset = [&](wxCommandEvent& WXUNUSED(event)) {
		TweakBrush* brush = os->glView->GetActiveBrush();
		if (brush) {
			brush->resetSettings();
			os->glView->ResetBrushSize();
			os->UpdateBrushSettings();
			os->CheckBrushBounds();
		}
	};

	const int sliderWidth = 250;
	const int valueCtrlWidth = 90;

	auto createValueCtrl = [&](double increment) {
		auto ctrl = new wxSpinCtrlDouble(panel, wxID_ANY, "", wxDefaultPosition, panel->FromDIP(wxSize(valueCtrlWidth, -1)), wxSP_ARROW_KEYS, 0.0, 10.0, 0.0, increment);
		ctrl->SetDigits(4);
		return ctrl;
	};

	panel = new wxPanel(popupWin);
	topSizer = new wxBoxSizer(wxVERTICAL);
	flexGridSizer = new wxFlexGridSizer(3, 0, 0);
	flexGridSizer->AddGrowableCol(1);

	lbBrushSize = new wxStaticText(panel, wxID_ANY, _("Size"));
	flexGridSizer->Add(lbBrushSize, 0, wxALL, 5);

	brushSize = new wxSlider(panel, wxID_ANY, 0, 0, SliderSteps, wxDefaultPosition, panel->FromDIP(wxSize(sliderWidth, -1)), wxSL_HORIZONTAL);
	brushSize->SetToolTip(_("Shortcut: 'S' + mouse wheel, 'Shift+S' + mouse wheel for finer steps"));
	brushSize->Bind(wxEVT_SLIDER, onSliderSize);
	flexGridSizer->Add(brushSize, 1, wxALL, 5);

	brushSizeVal = createValueCtrl(0.001);
	bindValueCtrl(brushSizeVal, brushSize, true, applySize);
	flexGridSizer->Add(brushSizeVal, 0, wxALL, 5);

	lbBrushStrength = new wxStaticText(panel, wxID_ANY, _("Strength"));
	flexGridSizer->Add(lbBrushStrength, 0, wxALL, 5);

	brushStrength = new wxSlider(panel, wxID_ANY, 0, 0, SliderSteps, wxDefaultPosition, panel->FromDIP(wxSize(sliderWidth, -1)), wxSL_HORIZONTAL);
	brushStrength->Bind(wxEVT_SLIDER, onSliderStrength);
	flexGridSizer->Add(brushStrength, 1, wxALL, 5);

	brushStrengthVal = createValueCtrl(0.001);
	bindValueCtrl(brushStrengthVal, brushStrength, true, applyStrength);
	flexGridSizer->Add(brushStrengthVal, 0, wxALL, 5);

	lbBrushFocus = new wxStaticText(panel, wxID_ANY, _("Focus"));
	flexGridSizer->Add(lbBrushFocus, 0, wxALL, 5);

	brushFocus = new wxSlider(panel, wxID_ANY, 0, 0, SliderSteps, wxDefaultPosition, panel->FromDIP(wxSize(sliderWidth, -1)), wxSL_HORIZONTAL);
	brushFocus->Bind(wxEVT_SLIDER, onSliderFocus);
	flexGridSizer->Add(brushFocus, 1, wxALL, 5);

	brushFocusVal = createValueCtrl(0.01);
	bindValueCtrl(brushFocusVal, brushFocus, false, applyFocus);
	flexGridSizer->Add(brushFocusVal, 0, wxALL, 5);

	lbBrushSpacing = new wxStaticText(panel, wxID_ANY, _("Spacing"));
	flexGridSizer->Add(lbBrushSpacing, 0, wxALL, 5);

	brushSpacing = new wxSlider(panel, wxID_ANY, 0, 0, SliderSteps, wxDefaultPosition, panel->FromDIP(wxSize(sliderWidth, -1)), wxSL_HORIZONTAL);
	brushSpacing->Bind(wxEVT_SLIDER, onSliderSpacing);
	flexGridSizer->Add(brushSpacing, 1, wxALL, 5);

	brushSpacingVal = createValueCtrl(0.001);
	bindValueCtrl(brushSpacingVal, brushSpacing, false, applySpacing);
	flexGridSizer->Add(brushSpacingVal, 0, wxALL, 5);

	topSizer->Add(flexGridSizer, 0, wxALL, 0);

	bottomSizer = new wxBoxSizer(wxHORIZONTAL);

	buttonReset = new wxButton(panel, wxID_ANY, _("Reset"));
	buttonReset->Bind(wxEVT_BUTTON, onButtonReset);
	bottomSizer->Add(buttonReset, 0, wxALL, 0);

	brushNameLabel = new wxStaticText(panel, wxID_ANY, "");
	bottomSizer->Add(brushNameLabel, 0, wxLEFT | wxALIGN_CENTER, 10);

	topSizer->Add(bottomSizer, 0, wxALL, 0);

	panel->SetSizer(topSizer);

	topSizer->Fit(panel);
	popupWin->SetClientSize(panel->GetSize());
}

void wxBrushSettingsPopupBase::SetBrushName(const wxString& brushName) {
	brushNameLabel->SetLabel(brushName);
}

void wxBrushSettingsPopupBase::SetBrushSize(float value) {
	brushSize->SetValue(ValueToSlider(value, true));
	brushSizeVal->SetValue(value);
}

void wxBrushSettingsPopupBase::SetBrushStrength(float value) {
	brushStrength->SetValue(ValueToSlider(value, true));
	brushStrengthVal->SetValue(value);
}

void wxBrushSettingsPopupBase::SetBrushFocus(float value) {
	brushFocus->SetValue(ValueToSlider(value, false));
	brushFocusVal->SetValue(value);
}

void wxBrushSettingsPopupBase::SetBrushSpacing(float value) {
	brushSpacing->SetValue(ValueToSlider(value, false));
	brushSpacingVal->SetValue(value);
}


wxBrushSettingsPopupTransient::wxBrushSettingsPopupTransient(OutfitStudioFrame* parent, bool stayOpen)
	: wxPopupTransientWindow(parent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS)
	, wxBrushSettingsPopupBase(parent, this) {
	this->stayOpen = stayOpen;
}
