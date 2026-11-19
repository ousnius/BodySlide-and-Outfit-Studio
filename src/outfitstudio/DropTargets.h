/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <wx/dnd.h>

class OutfitStudioFrame;

// Files dropped on the window and on the slider list

class DnDFile : public wxFileDropTarget {
public:
	DnDFile(OutfitStudioFrame* pOwner = nullptr) { owner = pOwner; }

	virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileNames);

private:
	OutfitStudioFrame* owner = nullptr;
};

class DnDSliderFile : public wxFileDropTarget {
public:
	DnDSliderFile(OutfitStudioFrame* pOwner = nullptr) { owner = pOwner; }

	virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileNames);
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult);

private:
	OutfitStudioFrame* owner = nullptr;
	wxDragResult lastResult = wxDragNone;
	std::string targetSlider;
};
