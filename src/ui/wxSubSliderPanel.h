/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

class wxSubSliderPanel : public wxWindow {
	bool isCreated = false;
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

	bool editing = false;

	wxSubSliderPanel();
	wxSubSliderPanel(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);

	bool Create(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);
	bool IsCreated() { return isCreated; }

	DECLARE_DYNAMIC_CLASS(wxSubSliderPanel)
	DECLARE_EVENT_TABLE()
};

class wxSubSliderPanelPool {
	std::vector<wxSubSliderPanel*> pool;

	const size_t MaxPoolSize = 500;

public:
	wxSubSliderPanel* Push();
	void CreatePool(size_t poolSize, wxWindow* parent, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);
	wxSubSliderPanel* Get(size_t index);
	wxSubSliderPanel* GetNext();
	void Clear();
};
