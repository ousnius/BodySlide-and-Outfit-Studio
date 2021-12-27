/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

class wxSliderPanel : public wxPanel {
	wxBoxSizer* sizer = nullptr;

public:
	wxBitmapButton* btnSliderEdit = nullptr;
	wxBitmapButton* btnSliderProp = nullptr;
	wxButton* btnMinus = nullptr;
	wxButton* btnPlus = nullptr;
	wxCheckBox* sliderCheck = nullptr;
	wxStaticText* sliderName = nullptr;
	wxSlider* slider = nullptr;
	wxTextCtrl* sliderReadout = nullptr;

	bool highlighted = false;

	wxSliderPanel();
	wxSliderPanel(wxWindow* parent, const wxString& name, size_t id, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);

	DECLARE_DYNAMIC_CLASS(wxSliderPanel)
	DECLARE_EVENT_TABLE()
};
