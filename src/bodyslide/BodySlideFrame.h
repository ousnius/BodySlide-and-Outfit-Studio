/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BodySlideApp.h"
#include "SliderDisplay.h"
#include "ui/wxFavoriteChoice.h"

#include <wx/html/htmlwin.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/wx.h>

#include <string>
#include <unordered_map>
#include <vector>

class PreviewPanel;

#define DELAYLOAD_TIMER 299

class BodySlideFrame : public wxFrame {
public:
	SliderDisplayPool sliderPool;
	std::unordered_map<std::string, SliderDisplay*> sliderDisplays;
	std::unordered_map<std::string, SliderCategoryUI*> sliderCategories;

	wxTimer delayLoad;

	wxFavoriteChoice* outfitChoice = nullptr;
	wxStaticText* lblOutfitBase = nullptr;
	wxStaticText* lblOutfitAddOns = nullptr;
	wxFavoriteChoice* presetChoice = nullptr;
	wxButton* btnFavoriteOutfit = nullptr;
	wxButton* btnFavoritePreset = nullptr;
	wxButton* btnSavePreset = nullptr;
	wxSearchCtrl* search = nullptr;
	wxSearchCtrl* outfitsearch = nullptr;
	wxSearchCtrl* sliderFilter = nullptr;
	wxSearchCtrl* presetFilter = nullptr;
	wxSizer* categoryTabSizer = nullptr;

	wxScrolledWindow* sliderScroll = nullptr;
	wxFlexGridSizer* sliderLayout = nullptr;

	wxListCtrl* batchBuildList = nullptr;

	// Batch build list items that are checked by default, and the ones whose output was last built from them
	enum class BatchBuildState { NotBuilt, BuiltWithPreset, BuiltWithOtherPreset, OtherOutfitBuilt };
	std::vector<bool> batchBuildDefaults;
	std::vector<BatchBuildState> batchBuildStates;

	wxMenu* fileCollisionMenu = nullptr;
	std::vector<std::string> outfitChoiceNames;
	std::vector<std::string> presetChoiceNames;
	bool populatingChoices = false;

	// Splitter and embedded preview
	wxSplitterWindow* splitter = nullptr;
	wxPanel* leftPanel = nullptr;
	PreviewPanel* previewPanel = nullptr;
	bool previewVisible = true;
	bool previewOnLeft = false;
	int savedSashPosition = -1;
	int savedPreviewWidth = 0;

	// Helpers for preview docking/undocking
	void UnsplitPreview();
	void SplitPreview(wxPanel* panel = nullptr);
	void SetPreviewOnLeft(bool onLeft);
	void UpdatePreviewButtonLabel();

	BodySlideFrame(BodySlideApp* app, const wxSize& size);
	~BodySlideFrame() { delete fileCollisionMenu; }

	void HideSlider(SliderDisplay* slider);
	void ShowLowColumn(bool show);
	void AddCategorySliderUI(const std::string& name, const std::vector<std::string>& sliders, bool enabled, bool oneSize);
	void AddSliderGUI(const std::string& name,
					  const std::string& display,
					  const std::string& categoryName,
					  bool isZap,
					  bool oneSize = false,
					  SliderMarker marker = SliderMarker::None,
					  const wxString& markerTip = wxEmptyString);

	SliderDisplay* GetSliderDisplay(const std::string& name) {
		if (sliderDisplays.find(name) != sliderDisplays.end())
			return sliderDisplays[name];

		return nullptr;
	}

	SliderCategoryUI* GetSliderCategory(const std::string& name) {
		if (sliderCategories.find(name) != sliderCategories.end())
			return sliderCategories[name];

		return nullptr;
	}

	void ClearPresetList();
	void ClearOutfitList();
	// Shows under the outfit list what the loaded project is an add-on or variant of
	void ShowOutfitBaseProject(const SliderSet* set);
	// Shows next to it what add-ons the loaded project has sliders of, and which ones it misses
	void ShowOutfitAddOns(const BodySlideApp::ProjectAddOns* addOns);
	void ClearSliderGUI();
	void SetPresetChanged(bool changed = true);

	void PopulateOutfitList(const wxArrayString& items, const wxString& selectItem);
	void PopulatePresetList(const wxArrayString& items, const wxString& selectItem);
	std::string GetSelectedOutfitName() const;
	std::string GetSelectedPresetName() const;

	void SetSliderPosition(const wxString& name, float newValue, short HiLo);
	void DoFilterSliders();

	int lastScroll = 0;

private:
	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnActivateFrame(wxActivateEvent& event);
	void OnIconizeFrame(wxIconizeEvent& event);
	void PostIconizeFrame();

	void OnLinkClicked(wxHtmlLinkEvent& link);
	void OnEnterClose(wxKeyEvent& event);

	void OnEnterSliderWindow(wxMouseEvent& event);
	wxString OutfitChoiceSuffix(const std::string& name) const;
	bool SelectChoiceName(wxFavoriteChoice* choice, const std::vector<std::string>& names, const std::string& selectItem) const;
	void SetFavoriteButtonBitmap(wxButton* button, bool favorite) const;
	void UpdateFavoriteButtons();
	void RebuildOutfitChoice(const std::string& selectItem);
	void RebuildPresetChoice(const std::string& selectItem);
	void OnSliderChange(wxScrollEvent& event);
	void OnSliderReadoutChange(wxCommandEvent& event);
	void OnSearchChange(wxCommandEvent& event);
	void OnOutfitSearchChange(wxCommandEvent& event);

	void OnSliderFilterChanged(wxCommandEvent&);
	void OnPresetFilterChanged(wxCommandEvent&);

	void OnZapCheckChanged(wxCommandEvent& event);
	void OnCategoryCheckChanged(wxCommandEvent& event);
	void OnCategoryTabButton(wxCommandEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void OnDelayLoad(wxTimerEvent& event);

	void OnChooseGroups(wxCommandEvent& event);
	void OnRefreshGroups(wxCommandEvent& event);
	void OnBrowseOutfitFolder(wxCommandEvent& event);
	void OnSaveGroups(wxCommandEvent& event);
	void OnRefreshOutfits(wxCommandEvent& event);
	void OnRegexOutfits(wxCommandEvent& event);
	void OnFilterHasZaps(wxCommandEvent& event);
	void OnFilterOutputWinners(wxCommandEvent& event);

	void OnChooseOutfit(wxCommandEvent& event);
	void OnChoosePreset(wxCommandEvent& event);
	void OnFavoriteOutfit(wxCommandEvent& event);
	void OnFavoritePreset(wxCommandEvent& event);

	void OnDeleteProject(wxCommandEvent& event);
	void OnDeletePreset(wxCommandEvent& event);

	void OnEditPreset(wxCommandEvent& event);
	void OnSavePreset(wxCommandEvent& event);
	void OnSavePresetAs(wxCommandEvent& event);
	void OnGroupManager(wxCommandEvent& event);
	void OnConflictPopup(wxMouseEvent& event);
	void OnOutfitChoiceSelect(wxCommandEvent& event);

	void OnPreview(wxCommandEvent& event);
	void OnSashPosChanging(wxSplitterEvent& event);
	void OnSashPosChanged(wxSplitterEvent& event);
	void OnPreviewPopout(wxCommandEvent& event);
	void OnPreviewWindowClosed();

	void OnHighToLow(wxCommandEvent& event);
	void OnLowToHigh(wxCommandEvent& event);
	void OnBuildBodies(wxCommandEvent& event);
	void OnConformAddOns(wxCommandEvent& event);
	void OnBatchBuild(wxCommandEvent& event);
	void OnBatchBuildContext(wxContextMenuEvent& event);
	void OnBatchBuildKey(wxKeyEvent& event);
	void OnBatchBuildSelect(wxCommandEvent& event);
	void OnOutfitStudio(wxCommandEvent& event);
	void OnSettings(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void OnEditProject(wxCommandEvent& event);

	void OnClippingStrengthChanged(wxCommandEvent& event);

	bool OutfitIsEmpty() {
		if (!GetSelectedOutfitName().empty())
			return false;

		return true;
	}

	void RefreshTargetGameState();

	BodySlideApp* app;

	wxDECLARE_EVENT_TABLE();
};
