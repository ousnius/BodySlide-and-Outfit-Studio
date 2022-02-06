/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "wxSliderPanel.h"

#include <wx/wx.h>

class wxDummySliderPanel : public wxWindow {
	bool isCreated = false;
	wxSliderPanel* m_sliderPanel = nullptr;
	wxBoxSizer* sizer = nullptr;
	wxString m_sliderReadoutValue;
	int m_sliderValue;
	bool m_isChecked;
	bool m_isEditing;

public:
	wxDummySliderPanel();
	wxDummySliderPanel(wxWindow* parent, const wxString& name);

	bool Create(wxWindow* parent, const wxString& name);

	bool IsCreated() { return isCreated; }
	bool IsChecked() { return m_isChecked; }
	bool IsInUse() { return IsShown() || m_sliderPanel; }
	bool IsVisible() { return m_sliderPanel != nullptr; }

	void AttachSubSliderPanel(wxSliderPanel* sliderPanel, wxSizer* sliderScrollSizer, int index);
	wxSliderPanel* DetachSubSliderPanel(wxSizer* sliderScrollSizer, int index);

	void SetValue(int value);
	void SetChecked(bool checked);
	void SetEditing(bool editing);
	void SetName(const wxString& name);
	void FocusSlider();

	DECLARE_DYNAMIC_CLASS(wxSliderPanel)
	DECLARE_EVENT_TABLE()
};

class wxDummySliderPanelPool {
	std::vector<wxDummySliderPanel*> pool;

	const size_t MaxPoolSize = 500;

public:
	wxDummySliderPanel* Push();
	void CreatePool(size_t poolSize, wxWindow* parent);
	wxDummySliderPanel* Get(size_t index);
	wxDummySliderPanel* GetNext();
	void Clear();
};
