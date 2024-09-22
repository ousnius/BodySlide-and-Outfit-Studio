/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../program/OutfitStudio.h"

#include <wx/popupwin.h>

class wxBrushSettingsPopupBase {
protected:
	OutfitStudioFrame* os = nullptr;

	wxPanel* panel = nullptr;
	wxBoxSizer* topSizer = nullptr;
	wxFlexGridSizer* flexGridSizer = nullptr;

	wxStaticText* lbBrushSize = nullptr;
	wxSlider* brushSize = nullptr;
	wxTextCtrl* brushSizeVal = nullptr;

	wxStaticText* lbBrushStrength = nullptr;
	wxSlider* brushStrength = nullptr;
	wxTextCtrl* brushStrengthVal = nullptr;

	wxStaticText* lbBrushFocus = nullptr;
	wxSlider* brushFocus = nullptr;
	wxTextCtrl* brushFocusVal = nullptr;

	wxStaticText* lbBrushSpacing = nullptr;
	wxSlider* brushSpacing = nullptr;
	wxTextCtrl* brushSpacingVal = nullptr;

	wxBoxSizer* bottomSizer = nullptr;
	wxButton* buttonReset = nullptr;
	wxStaticText* brushNameLabel = nullptr;

	void Setup(wxWindow* popupWin);

public:
	wxBrushSettingsPopupBase(OutfitStudioFrame* parent, wxWindow* popupWin);

	void SetBrushName(const wxString& brushName);
	void SetBrushSize(float value);
	void SetBrushStrength(float value);
	void SetBrushFocus(float value);
	void SetBrushSpacing(float value);
};

class wxBrushSettingsPopupTransient : public wxPopupTransientWindow, public wxBrushSettingsPopupBase {
private:
	bool stayOpen = false;

public:
	wxBrushSettingsPopupTransient(OutfitStudioFrame* parent, bool stayOpen);

#ifdef _WINDOWS
	// IsMouseInWindow is only available on Windows
	void Dismiss() override {
		if (!stayOpen || !GetParent()->IsMouseInWindow())
			Hide();
	}

	void MSWDismissUnfocusedPopup() override {
		if (stayOpen)
			Dismiss();
	}
#endif
};
