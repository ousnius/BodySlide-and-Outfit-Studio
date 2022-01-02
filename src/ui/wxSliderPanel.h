/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

class wxSliderPanel : public wxPanel {
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

	wxSliderPanel();
	wxSliderPanel(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);

	bool Create(wxWindow* parent, const wxString& name, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);
	bool IsCreated() {
		return isCreated;
	}

	DECLARE_DYNAMIC_CLASS(wxSliderPanel)
	DECLARE_EVENT_TABLE()
};

class wxSliderPanelPool {
	std::vector<wxSliderPanel*> pool;

	const size_t MaxPoolSize = 500;

public:
	void IncreaseSize(size_t poolSize);
	void CreatePool(wxWindow* parent, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);
	size_t GetSize() {
		return pool.size();
	}
	wxSliderPanel* Get(size_t index);
	void Clear();
};
