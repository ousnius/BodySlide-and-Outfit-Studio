/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../program/OutfitStudio.h"

#include <wx/popupwin.h>

class wxBrushSettingsPopup : public wxPopupTransientWindow {
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

	void OnBrushSettingsSlider(wxCommandEvent& event);

public:
	wxBrushSettingsPopup(OutfitStudioFrame* parent);
	virtual ~wxBrushSettingsPopup();

	void SetBrushSize(float value);
	void SetBrushStrength(float value);
	void SetBrushFocus(float value);
	void SetBrushSpacing(float value);
};
