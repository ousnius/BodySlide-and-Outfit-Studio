/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "wxSubSliderPanel.h"

#include <wx/wx.h>

class wxSliderPanel : public wxWindow {
	bool isCreated = false;
	wxSubSliderPanel* m_subSliderPanel;

	wxBoxSizer* sizer = nullptr;
	wxString sliderReadoutValue;
	float sliderValue;
	bool isChecked;
	bool isEditing;
	bool isFocused;

public:
	wxSliderPanel();
	wxSliderPanel(wxWindow* parent, const wxString& name);

	bool Create(wxWindow* parent, const wxString& name);

	bool IsCreated() { return isCreated; }
	bool IsChecked() { return isChecked; }

	void AttachSubSliderPanel(wxSubSliderPanel* subSliderPanel, size_t index, const wxBitmap& bmpEdit, const wxBitmap& bmpSettings);
	void DetachSubSliderPanel();

	void SetValue(float value);
	void SetChecked(bool checked);
	void SetEditing(bool editing);
	void SetName(const wxString& name) { SetLabel(name);}
	void FocusSlider() {
		//slider->SetFocus();
	}

	DECLARE_DYNAMIC_CLASS(wxSliderPanel)
	DECLARE_EVENT_TABLE()
};

class wxSliderPanelPool {
	std::vector<wxSliderPanel*> pool;

	const size_t MaxPoolSize = 500;

public:
	wxSliderPanel* Push();
	void CreatePool(size_t poolSize, wxWindow* parent);
	wxSliderPanel* Get(size_t index);
	wxSliderPanel* GetNext();
	void Clear();
};
