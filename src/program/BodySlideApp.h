/*
BodySlide and Outfit Studio

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "../components/BuildSelection.h"
#include "../components/ClippingFixer.h"
#include "../components/SliderCategories.h"
#include "../components/SliderData.h"
#include "../components/SliderGroup.h"
#include "../components/SliderManager.h"
#include "../files/TriFile.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/Log.h"
#include "GroupManager.h"
#include "PresetSaveDialog.h"
#include "PreviewWindow.h"
#include "../ui/PreviewPanel.h"
#include "../ui/wxStateButton.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <wx/clrpicker.h>
#include <wx/cmdline.h>
#include <wx/collpane.h>
#include <wx/dcbuffer.h>
#include <wx/dir.h>
#include <wx/html/htmlwin.h>
#include <wx/imagpng.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/treelist.h>
#include <wx/progdlg.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/statline.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/wxprec.h>
#include <wx/xrc/xmlres.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>


enum TargetGame { FO3, FONV, SKYRIM, FO4, SKYRIMSE, FO4VR, SKYRIMVR, FO76, OB, SF };

struct ShapePreviewData {
	std::string name;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Vector2> uvs;
	std::vector<uint16_t> zapIdx;
	int projectIdx = 0;
};

class BodySlideFrame;

class BodySlideApp : public wxApp {
	/* UI Managers */
	BodySlideFrame* sliderView = nullptr;
	PreviewPanel* preview = nullptr;
	PreviewWindow* previewWindow = nullptr;
	bool dockPoppedOutOnClose = true;

	/* Command-Line Arguments */
	std::vector<std::string> cmdGroupBuild;
	std::string cmdTargetDir;
	std::string cmdPreset;
	std::string cmdPresetFile;
	bool cmdPresetResolved = false;
	bool cmdPresetPending = false;
	bool cmdTri = false;
	std::vector<std::string> cmdPreviewNifs;
	bool cmdPreviewMode = false;

public:
	/* Clipping Fix */
	float clippingFixStrength = 0.0f; // 0-100 scale, 0 disables clipping fix
private:

	/* Reference shape loaded from external project for clipping fix */
	std::unique_ptr<nifly::NifFile> referenceNif;
	SliderSet referenceSliderSet;
	DiffDataSets referenceDiffData;
	std::string referenceShapeName;

	/* Localization */
	wxLocale* locale = nullptr;
	int language = 0;

	/* Data Managers */
	SliderManager sliderManager;
	Log logger;

	/* Data Items */
	std::map<std::string, std::string, case_insensitive_compare> outfitNameSource; // All currently defined outfits.
	std::vector<std::string> outfitNameOrder;									   // All currently defined outfits, in their order of appearance.
	std::vector<std::string> outfitHasZaps;										   // All currently defined outfits that have visible zaps.
	std::map<std::string, std::vector<std::string>> groupMembers;				   // All currently defined groups.
	std::map<std::string, std::string> groupAlias;								   // Group name aliases.
	std::vector<std::string> ungroupedOutfits;									   // Outfits without a group.
	std::vector<std::string> filteredOutfits;									   // Filtered outfit names.
	std::vector<std::string> favoriteOutfits;
	std::vector<std::string> favoritePresets;
	std::unordered_set<std::string> favoriteOutfitSet;
	std::unordered_set<std::string> favoritePresetSet;
	std::vector<std::string> presetGroups;
	std::vector<std::string> allGroups;
	SliderSetGroupCollection gCollection;

	/* Cache */
	std::map<std::string, nifly::NifFile, case_insensitive_compare> refNormalsCache; // Cache for reference normals files

	struct ProjectData {
		SliderSet sliderSet;
		DiffDataSets dataSets;
		nifly::NifFile* baseNif = nullptr;
		nifly::NifFile modNif;
		std::string setName;
		std::string inputFileName;

		~ProjectData() { delete baseNif; }
		ProjectData() = default;
		ProjectData(const ProjectData&) = delete;
		ProjectData& operator=(const ProjectData&) = delete;
	};
	std::vector<std::unique_ptr<ProjectData>> projects;
	bool multiProjectMode = false;

	int CreateSetSliders(const std::string& outfit);
	std::string GetFavoriteConfigKey(const std::string& listName) const;
	std::string SerializeFavoriteNames(const std::vector<std::string>& names) const;
	std::vector<std::string> DeserializeFavoriteNames(const std::string& value) const;
	void SetFavoriteList(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::vector<std::string>& names);
	bool RemoveFavoriteName(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::string& name);
	void SortFavoritesFirst(std::vector<std::string>& names, const std::unordered_set<std::string>& favorites) const;

public:
	virtual ~BodySlideApp();
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	virtual bool OnExceptionInMainLoop();
	virtual void OnUnhandledException();
	virtual void OnFatalException();

	SliderCategoryCollection cCollection;
	TargetGame targetGame = TargetGame::FO3;
	std::map<std::string, std::vector<std::string>, case_insensitive_compare> outFileCount; // Counts how many sets write to the same output file

	bool SetDefaultConfig();
	bool ShowSetup();

	std::string GetOutputDataPath() const;
	SliderSet& GetActiveSet() { return projects[0]->sliderSet; }
	DiffDataSets& GetActiveDataSets() { return projects[0]->dataSets; }
	bool HasActiveProject() const { return !projects.empty(); }

	int AddProjectSliders(const std::string& projectFile, const std::string& setName);

	void InitLanguage();

	void LoadData();
	void CharHook(wxKeyEvent& event);

	bool OutfitExists(const std::string& name) const { return outfitNameSource.find(name) != outfitNameSource.end(); }
	bool PresetExists(const std::string& name);

	void LoadFavorites();
	void SaveFavorites();
	bool IsFavoriteOutfit(const std::string& name) const;
	bool IsFavoritePreset(const std::string& name) const;
	void SortOutfitNamesForDisplay(std::vector<std::string>& names) const;
	void SortPresetNamesForDisplay(std::vector<std::string>& names) const;
	void ToggleFavoriteOutfit(const std::string& name);
	void ToggleFavoritePreset(const std::string& name);
	void RemoveFavoriteOutfit(const std::string& name);
	void RemoveFavoritePreset(const std::string& name);

	void LoadAllCategories();

	void SetPresetGroups(const std::string& setName);
	void LoadAllGroups();
	void GetAllGroupNames(std::vector<std::string>& outGroups);
	int SaveGroupList(const std::string& filename, const std::string& groupname);

	void PopulateFilterData();
	void ApplyOutfitFilter();
	std::vector<std::string> ApplyPresetFilter(const std::vector<std::string>& presetNames);
	int GetOutfits(std::vector<std::string>& outList);
	int GetFilteredOutfits(std::vector<std::string>& outList);

	void LoadPresets(const std::string& sliderSet);
	void LoadCmdPresetFile();
	void GetPresetNames(std::vector<std::string>& outNames);
	std::string GetPresetFileName(const std::string& presetName);
	void GetPresetGroups(const std::string& presetName, std::vector<std::string>& outGroups);
	void InitializeSliders(const std::string& presetName = "");
	void RefreshPresetsForCurrentOutfit();
	void PopulatePresetList(const std::string& select);
	void PopulateOutfitList(const std::string& select);
	void DisplayActiveSet();

	void GetBuildSelection(BuildSelectionFile& file, BuildSelection& buildSel);

	void UpdateConflictManager();
	void SetDefaultBuildSelection();

	bool UpdateZapChoices();
	void SetZapChoice(const std::string& zap, bool choice);

	int LoadSliderSets();
	void RefreshOutfitList();

	void RefreshSliders();

	void ActivateOutfit(const std::string& outfitName);
	void ActivatePreset(const std::string& presetName, const bool updatePreview = true);

	std::vector<std::string> GetConflictingOutfits() {
		if (projects.empty())
			return {};
		return outFileCount.find(GetActiveSet().GetOutputFilePath())->second;
	}

	void DeleteOutfit(const std::string& outfitName);
	void DeletePreset(const std::string& presetName);

	void EditProject(const std::string& projectName);
	void LaunchOutfitStudio(const wxString& args = "");

	void ApplySliders(const std::string& targetShape,
					  std::vector<Slider>& sliderSet,
					  DiffDataSets& dataSets,
					  std::vector<nifly::Vector3>& verts,
					  std::vector<uint16_t>& zapidx,
					  std::vector<nifly::Vector2>* uvs = nullptr);
	bool WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, nifly::NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices);
	bool WriteSFMorphFile(const std::string& morphFolder, SliderSet& sliderSet, nifly::NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices);

	void CopySliderValues(bool toHigh);
	void CopyPreviewWeightToSliders();
	void ShowPreview();
	void InitPreviewPanel();
	void BuildPreviewMesh(ProjectData* pp, bool freshLoad);
	void InitPreview();
	bool IsPreviewRenderable() const;
	void CleanupPreview();
	void LoadPreviewNifs(const std::vector<std::string>& nifFilePaths);
	void ClosePreview();
	void PreviewClosed();
	void PopOutPreview();
	void DockPreview(bool attachToMain = true, bool preservePoppedOutState = false);
	bool IsPreviewPoppedOut() const { return previewWindow != nullptr; }
	void SetDockPoppedOutOnClose(bool dock) { dockPoppedOutOnClose = dock; }
	bool ShouldDockPoppedOutOnClose() const { return dockPoppedOutOnClose; }

	/* Async preview loading */
	std::atomic<uint64_t> previewLoadGeneration{0};
	std::thread previewLoadThread;
	bool previewLoading = false;

	void ApplyClippingFix(nifly::NifFile& nif,
						const std::vector<nifly::Vector3>& bodyVerts,
						const std::vector<nifly::Triangle>& bodyTris,
						std::unordered_map<std::string, std::vector<nifly::Vector3>*>& shapeVerts);
	bool LoadExternalReference(const SliderSet& sliderSet);
	void UpdateReferenceCheckboxState();
	void UpdatePreview();
	void RebuildPreviewMeshes();
	std::vector<ShapePreviewData> ComputeMorphedShapeData(int weight);
	void PostProcessPreview(std::vector<ShapePreviewData>& shapeData, int weight);
	void UpdateExternalReferenceMesh(int weight, std::vector<nifly::Vector3>* outVerts = nullptr);
	void UpdateMeshesFromSet(SliderSet& set);
	void ApplyReferenceNormals(nifly::NifFile& nif);

	int BuildBodies(bool localPath = false, bool clean = false, bool tri = false, bool forceNormals = false);
	int BuildListBodies(std::vector<std::string>& outfitList,
						std::map<std::string, std::string>& failedOutfits,
						bool remove = false,
						bool tri = false,
						bool forceNormals = false,
						const std::string& custPath = "");
	int ShowBuildOverrideWithPreview(wxDialog* dlg, wxTreeListCtrl* treeListCtrl);
	void GroupBuild(const std::vector<std::string>& groupNames);

	void AddTriData(nifly::NifFile& nif, const std::string& shapeName, const std::string& triPath, bool toRoot = false);

	float GetSliderValue(const wxString& sliderName, bool isLo);
	bool IsUVSlider(const wxString& sliderName);
	std::vector<std::string> GetSliderZapToggles(const wxString& sliderName);
	void SetSliderValue(const wxString& sliderName, bool isLo, float val);
	void SetSliderChanged(const wxString& sliderName, bool isLo);

	int UpdateSliderPositions(const std::string& presetName);
	int SaveSliderPositions(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups);
	int SavePresetGroups(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups);
};

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {{wxCMD_LINE_OPTION, "gbuild", "groupbuild", "builds the specified group on launch", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION, "t", "targetdir", "build target directory, defaults to game data path", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"p",
													"preset",
													"preset to load on launch, use for the build or apply in preview mode, either a preset name or the "
													"path to a preset XML file optionally followed by '?' and a preset name, defaults to last used preset",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_SWITCH, "tri", "trimorphs", "enables tri morph output for the specified build"},
												   {wxCMD_LINE_OPTION, "preview", "preview", "open the specified nif files in preview mode", wxCMD_LINE_VAL_STRING},
												   wxCMD_LINE_DESC_END};

#define DELAYLOAD_TIMER 299
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

class SliderDisplay {
	bool isCreated = false;

public:
	bool isShown = false;
	bool isZap = false;
	bool oneSize = false;

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

class BodySlideFrame : public wxFrame {
public:
	SliderDisplayPool sliderPool;
	std::unordered_map<std::string, SliderDisplay*> sliderDisplays;
	std::unordered_map<std::string, SliderCategoryUI*> sliderCategories;

	wxTimer delayLoad;

	wxChoice* outfitChoice = nullptr;
	wxChoice* presetChoice = nullptr;
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

	wxCheckListBox* batchBuildList = nullptr;
	wxMenu* fileCollisionMenu = nullptr;
	std::vector<std::string> outfitChoiceNames;
	std::vector<std::string> presetChoiceNames;
	bool populatingChoices = false;

	// Splitter and embedded preview
	wxSplitterWindow* splitter = nullptr;
	wxPanel* leftPanel = nullptr;
	PreviewPanel* previewPanel = nullptr;
	bool previewVisible = true;
	int savedSashPosition = -1;
	int savedPreviewWidth = 0;

	// Helpers for preview docking/undocking
	void UnsplitPreview();
	void SplitPreview(wxPanel* panel = nullptr);
	void UpdatePreviewButtonLabel();

	BodySlideFrame(BodySlideApp* app, const wxSize& size);
	~BodySlideFrame() { delete fileCollisionMenu; }

	void HideSlider(SliderDisplay* slider);
	void ShowLowColumn(bool show);
	void AddCategorySliderUI(const std::string& name, const std::vector<std::string>& sliders, bool enabled, bool oneSize);
	void AddSliderGUI(const std::string& name, const std::string& display, const std::string& categoryName, bool isZap, bool oneSize = false);

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
	wxString FavoriteChoiceLabel(const std::string& name, bool favorite) const;
	bool SelectChoiceName(wxChoice* choice, const std::vector<std::string>& names, const std::string& selectItem) const;
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
	void OnBatchBuild(wxCommandEvent& event);
	void OnBatchBuildContext(wxMouseEvent& event);
	void OnBatchBuildSelect(wxCommandEvent& event);
	void OnOutfitStudio(wxCommandEvent& event);
	void SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame);
	void OnSettings(wxCommandEvent& event);
	void OnChooseTargetGame(wxCommandEvent& event);
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
