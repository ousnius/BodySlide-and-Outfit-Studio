/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

#include <string>
#include <vector>

class wxStateButton;

// The rows of the slider list, and the category headers and tab buttons above them

#define SLIDER_LO 1
#define SLIDER_HI 2

class SliderCategoryUI {
	bool isCreated = false;

public:
	bool isShown = false;
	bool isEnabled = false;
	bool oneSize = false;

	std::string categoryName;
	std::vector<std::string> sliderNames;

	wxPanel* dummyPanel1 = nullptr;
	wxCheckBox* check = nullptr;
	wxStaticText* label = nullptr;
	wxPanel* dummyPanel2 = nullptr;
	wxStateButton* tabButton = nullptr;

	SliderCategoryUI();

	bool IsCreated() { return isCreated; }

	bool Create(wxScrolledWindow* scrollWindow,
				wxSizer* sliderLayout,
				wxSizer* categoryTabSizer,
				const std::string& name,
				const std::vector<std::string>& sliders,
				bool pEnabled = true,
				bool pOneSize = false);
	void Show(bool show = true);
	void Destroy();
};

// How a slider is marked in the slider list
enum class SliderMarker {
	None,
	AddOn,		  // From an add-on
	AutoConformed // Conformed from an add-on of the body when loading
};

class SliderDisplay {
	bool isCreated = false;

public:
	bool isShown = false;
	bool isZap = false; // Shown as a check box, for zaps and toggles
	bool oneSize = false;
	bool isForced = false; // Held at a value by a ticked toggle
	wxString markerTip;

	std::string sliderName;
	std::string displayName;
	std::string categoryName;

	wxStaticText* lblSliderLo = nullptr;
	wxSlider* sliderLo = nullptr;
	wxTextCtrl* sliderReadoutLo = nullptr;
	wxStaticText* lblSliderHi = nullptr;
	wxSlider* sliderHi = nullptr;
	wxTextCtrl* sliderReadoutHi = nullptr;
	wxCheckBox* zapCheckHi = nullptr;
	wxCheckBox* zapCheckLo = nullptr;

	SliderDisplay();

	bool IsCreated() { return isCreated; }

	bool Create(
		wxScrolledWindow* scrollWindow, wxSizer* sliderLayout, const std::string& name, const std::string& category, const std::string& display, int minValue, int maxValue, bool pIsZap, bool pOneSize = false);
	void Show(bool show = true);
	// Colors the labels and gives the slider a tooltip, which pooled sliders take over otherwise
	void SetMarker(SliderMarker marker, const wxString& tip);
	// Holds the slider at the value that the named toggle sets, or gives it back with an empty name
	void SetForcedBy(const wxString& toggleName);
};

class SliderDisplayPool {
	std::vector<SliderDisplay*> pool;

	const size_t MaxPoolSize = 500;

public:
	SliderDisplay* Push();
	void CreatePool(size_t poolSize, wxScrolledWindow* scrollWindow, wxSizer* sliderLayout);
	SliderDisplay* Get(size_t index);
	SliderDisplay* GetNext();
	void Clear();
};
