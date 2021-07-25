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
	wxStaticText* lbBrushSizeVal = nullptr;

	wxStaticText* lbBrushStrength = nullptr;
	wxSlider* brushStrength = nullptr;
	wxStaticText* lbBrushStrengthVal = nullptr;

	wxStaticText* lbBrushFocus = nullptr;
	wxSlider* brushFocus = nullptr;
	wxStaticText* lbBrushFocusVal = nullptr;

	wxStaticText* lbBrushSpacing = nullptr;
	wxSlider* brushSpacing = nullptr;
	wxStaticText* lbBrushSpacingVal = nullptr;

	void Setup(wxWindow* popupWin);

public:
	wxBrushSettingsPopupBase(OutfitStudioFrame* parent, wxWindow* popupWin);

	void SetBrushSize(float value);
	void SetBrushStrength(float value);
	void SetBrushFocus(float value);
	void SetBrushSpacing(float value);
};

class wxBrushSettingsPopup : public wxPopupWindow, public wxBrushSettingsPopupBase {
public:
	wxBrushSettingsPopup(OutfitStudioFrame* parent);
};

class wxBrushSettingsPopupTransient : public wxPopupTransientWindow, public wxBrushSettingsPopupBase {
public:
	wxBrushSettingsPopupTransient(OutfitStudioFrame* parent);
};
