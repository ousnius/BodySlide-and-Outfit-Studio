/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitProject.h"

#include <functional>
#include <vector>

#include <wx/dialog.h>
#include <wx/string.h>

class OutfitStudioFrame;
class wxButton;
class wxChoice;
class wxListCtrl;
class wxTextCtrl;
class wxWindow;

class SliderDataList {
public:
	static wxString JoinDataFolders(const std::vector<std::string>& dataFolders);
	static void Configure(wxListCtrl* list, bool includeSlider);
	static void Populate(wxListCtrl* list, const std::vector<SliderDataLocation>& rows, const std::vector<size_t>* visibleRows, bool includeSlider);
	static size_t GetSelected(wxListCtrl* list, const std::vector<SliderDataLocation>& rows, std::vector<SliderDataLocation>& outLocations);
	static bool AllHaveSource(const std::vector<SliderDataLocation>& locations, bool local);
	static void BindSelectAll(wxWindow* window, wxListCtrl* list, const std::function<void()>& updateButtons);
	static bool MakeLocal(wxWindow* parent, OutfitProject* project, const std::vector<SliderDataLocation>& locations);
	static bool EditFolders(wxWindow* parent, OutfitProject* project, const std::vector<SliderDataLocation>& locations);
};

class SliderDataLocationsDialog : public wxDialog {
public:
	SliderDataLocationsDialog(OutfitStudioFrame* owner, OutfitProject* project);

private:
	void UpdateButtons();
	void ApplyFilters();
	void RefreshRows();
	void MarkChangedAndRefresh();

	OutfitStudioFrame* owner = nullptr;
	OutfitProject* project = nullptr;
	wxTextCtrl* filterText = nullptr;
	wxChoice* sourceChoice = nullptr;
	wxChoice* statusChoice = nullptr;
	wxListCtrl* sliderDataList = nullptr;
	wxButton* btnMakeLocal = nullptr;
	wxButton* btnMakeExternal = nullptr;
	wxButton* btnEditFolders = nullptr;
	std::vector<SliderDataLocation> rows;
	std::vector<size_t> visibleRows;
};