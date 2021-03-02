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

#include "PreviewWindow.h"
#include "PresetSaveDialog.h"
#include "GroupManager.h"
#include "../components/SliderData.h"
#include "../components/SliderManager.h"
#include "../components/SliderGroup.h"
#include "../components/SliderCategories.h"
#include "../components/BuildSelection.h"
#include "../files/TriFile.h"
#include "../utils/Log.h"
#include "../utils/ConfigurationManager.h"

#include "../FSEngine/FSManager.h"
#include "../FSEngine/FSEngine.h"

#include <wx/wxprec.h>
#include <wx/srchctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/imagpng.h>
#include <wx/dcbuffer.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>
#include <wx/cmdline.h>
#include <wx/html/htmlwin.h>
#include <wx/listctrl.h>
#include <wx/dir.h>
#include <wx/progdlg.h>
#include <wx/intl.h>
#include <wx/clrpicker.h>
#include <wx/collpane.h>
#include <wx/stdpaths.h>


enum TargetGame {
	FO3, FONV, SKYRIM, FO4, SKYRIMSE, FO4VR, SKYRIMVR, FO76
};

class BodySlideFrame;

class BodySlideApp : public wxApp {
	/* UI Managers */
	BodySlideFrame* sliderView = nullptr;
	PreviewWindow* preview = nullptr;

	/* Command-Line Arguments */
	wxString cmdGroupBuild;
	wxString cmdTargetDir;
	wxString cmdPreset;
	bool cmdTri = false;

	/* Localization */
	wxLocale* locale = nullptr;
	int language = 0;

	/* Data Managers */
	SliderManager sliderManager;
	DiffDataSets dataSets;
	SliderSet activeSet;
	Log logger;

	/* Data Items */
	std::map<std::string, std::string, case_insensitive_compare> outfitNameSource;	// All currently defined outfits.
	std::vector<std::string> outfitNameOrder;										// All currently defined outfits, in their order of appearance.
	std::map<std::string, std::vector<std::string>> groupMembers;					// All currently defined groups.
	std::map<std::string, std::string> groupAlias;									// Group name aliases.
	std::vector<std::string> ungroupedOutfits;										// Outfits without a group.
	std::vector<std::string> filteredOutfits;										// Filtered outfit names.
	std::vector<std::string> presetGroups;
	std::vector<std::string> allGroups;
	SliderSetGroupCollection gCollection;

	/* Cache */
	std::map<std::string, nifly::NifFile, case_insensitive_compare> refNormalsCache;	// Cache for reference normals files

	std::map<std::string, std::vector<std::string>, case_insensitive_compare> outFileCount;	// Counts how many sets write to the same output file

	std::string previewBaseName;
	std::string previewSetName;
	nifly::NifFile* previewBaseNif = nullptr;
	nifly::NifFile PreviewMod;

	int CreateSetSliders(const std::string& outfit);

public:
	virtual ~BodySlideApp();
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	virtual bool OnExceptionInMainLoop();
	virtual void OnUnhandledException();
	virtual void OnFatalException();
	
	SliderCategoryCollection cCollection;
	TargetGame targetGame;

	bool SetDefaultConfig();
	bool ShowSetup();
	wxString GetGameDataPath(TargetGame targ);

	std::string GetOutputDataPath() const;
	std::string GetProjectPath() const;

	void InitLanguage();

	void InitArchives();
	void GetArchiveFiles(std::vector<std::string>& outList);

	void LoadData();
	void CharHook(wxKeyEvent& event) {
		wxWindow* w = (wxWindow*)event.GetEventObject();
		if (!w) {
			event.Skip();
			return;
		}

		wxString nm = w->GetName();
		if (event.GetKeyCode() == wxKeyCode::WXK_F5) {
			if (nm == "outfitChoice") {
				RefreshOutfitList();
			}
			event.Skip();
			return;
		}

#ifdef _WINDOWS
		std::string stupidkeys = "0123456789-";
		bool stupidHack = false;
		if (event.GetKeyCode() < 256 && stupidkeys.find(event.GetKeyCode()) != std::string::npos)
			stupidHack = true;

		if (stupidHack && nm.EndsWith("|readout")) {
			wxTextCtrl* e = (wxTextCtrl*)w;
			HWND hwndEdit = e->GetHandle();
			::SendMessage(hwndEdit, WM_CHAR, event.GetKeyCode(), event.GetRawKeyFlags());
		}
		else
#endif
		{
			event.Skip();
		}
	}

	void LoadAllCategories();

	void SetPresetGroups(const std::string& setName);
	void LoadAllGroups();
	void GetAllGroupNames(std::vector<std::string>& outGroups);
	int SaveGroupList(const std::string& filename, const std::string& groupname);

	void ApplyOutfitFilter();
	int GetOutfits(std::vector<std::string>& outList);
	int GetFilteredOutfits(std::vector<std::string>& outList);

	void LoadPresets(const std::string& sliderSet);
	void PopulatePresetList(const std::string& select);
	void PopulateOutfitList(const std::string& select);
	void DisplayActiveSet();

	int LoadSliderSets();
	void RefreshOutfitList();

	void RefreshSliders();

	void ActivateOutfit(const std::string& outfitName);
	void ActivatePreset(const std::string& presetName, const bool updatePreview = true);

	void DeleteOutfit(const std::string& outfitName);
	void DeletePreset(const std::string& presetName);

	void EditProject(const std::string& projectName);
	void LaunchOutfitStudio(const wxString& args = "");

	void ApplySliders(const std::string& targetShape, std::vector<Slider>& sliderSet, std::vector<nifly::Vector3>& verts, std::vector<uint16_t>& zapidx, std::vector<nifly::Vector2>* uvs = nullptr);
	bool WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, nifly::NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices);

	void CopySliderValues(bool toHigh);
	void ShowPreview();
	void InitPreview();
	void CleanupPreview();
	void ClosePreview() {
		// Calling Close() will cause PreviewClosed() to be called,
		// where we reset the preview window pointer to null
		if (preview)
			preview->Close();
	}
	void PreviewClosed() {
		preview = nullptr;
	}

	void UpdatePreview();
	void RebuildPreviewMeshes();
	void UpdateMeshesFromSet();
	void ApplyReferenceNormals(nifly::NifFile& nif);

	int BuildBodies(bool localPath = false, bool clean = false, bool tri = false, bool forceNormals = false);
	int BuildListBodies(std::vector<std::string>& outfitList, std::map<std::string, std::string>& failedOutfits, bool remove = false, bool tri = false, bool forceNormals = false, const std::string& custPath = "");
	void GroupBuild(const std::string& group);

	void AddTriData(nifly::NifFile& nif, const std::string& shapeName, const std::string& triPath, bool toRoot = false);

	float GetSliderValue(const wxString& sliderName, bool isLo);
	bool IsUVSlider(const wxString& sliderName);
	std::vector<std::string> GetSliderZapToggles(const wxString& sliderName);
	void SetSliderValue(const wxString& sliderName, bool isLo, float val);
	void SetSliderChanged(const wxString& sliderName, bool isLo);

	int UpdateSliderPositions(const std::string& presetName);
	int SaveSliderPositions(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups);
};

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
	{ wxCMD_LINE_OPTION, "gbuild", "groupbuild", "builds the specified group on launch", wxCMD_LINE_VAL_STRING },
	{ wxCMD_LINE_OPTION, "t", "targetdir", "build target directory, defaults to game data path", wxCMD_LINE_VAL_STRING },
	{ wxCMD_LINE_OPTION, "p", "preset", "preset used for the build, defaults to last used preset", wxCMD_LINE_VAL_STRING },
	{ wxCMD_LINE_SWITCH, "tri", "trimorphs", "enables tri morph output for the specified build" },
	{ wxCMD_LINE_NONE }
};

#define DELAYLOAD_TIMER 299
#define SLIDER_LO 1
#define SLIDER_HI 2

class BodySlideFrame : public wxFrame {
public:
	class SliderDisplay {
	public:
		bool isZap = false;
		bool oneSize = false;
		std::string sliderName;
		wxStaticText* lblSliderLo = nullptr;
		wxSlider* sliderLo = nullptr;
		wxTextCtrl* sliderReadoutLo = nullptr;
		wxStaticText* lblSliderHi = nullptr;
		wxSlider* sliderHi = nullptr;
		wxTextCtrl* sliderReadoutHi = nullptr;
		wxCheckBox* zapCheckHi = nullptr;
		wxCheckBox* zapCheckLo = nullptr;

		SliderDisplay() {}
	};

	std::unordered_map<std::string, SliderDisplay*> sliderDisplays;

	wxTimer delayLoad;
	wxSearchCtrl* search = nullptr;
	wxSearchCtrl* outfitsearch = nullptr;
	wxCheckListBox* batchBuildList = nullptr;

	BodySlideFrame(BodySlideApp* app, const wxSize& size);
	~BodySlideFrame() {
		ClearSliderGUI();
	}

	void ShowLowColumn(bool show);
	void AddCategorySliderUI(const wxString& name, bool show, bool oneSize);
	void AddSliderGUI(const std::string& name, const std::string& display, bool isZap, bool oneSize = false);

	BodySlideFrame::SliderDisplay* GetSliderDisplay(const std::string& name) {
		if (sliderDisplays.find(name) != sliderDisplays.end())
			return sliderDisplays[name];

		return nullptr;
	}

	void ClearPresetList();
	void ClearOutfitList();
	void ClearSliderGUI();

	void PopulateOutfitList(const wxArrayString& items, const wxString& selectItem);
	void PopulatePresetList(const wxArrayString& items, const wxString& selectItem);

	void SetSliderPosition(const wxString& name, float newValue, short HiLo);

	int lastScroll;
	int rowCount;

private:
	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnActivateFrame(wxActivateEvent& event);
	void OnIconizeFrame(wxIconizeEvent& event);
	void PostIconizeFrame();

	void OnLinkClicked(wxHtmlLinkEvent& link);
	void OnEnterClose(wxKeyEvent& event);

	void OnEnterSliderWindow(wxMouseEvent& event);
	void OnSliderChange(wxScrollEvent& event);
	void OnSliderReadoutChange(wxCommandEvent& event);
	void OnSearchChange(wxCommandEvent& event);
	void OnOutfitSearchChange(wxCommandEvent& event);

	void OnZapCheckChanged(wxCommandEvent& event);
	void OnCategoryCheckChanged(wxCommandEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void OnDelayLoad(wxTimerEvent& event);

	void OnChooseGroups(wxCommandEvent& event);
	void OnRefreshGroups(wxCommandEvent& event);
	void OnSaveGroups(wxCommandEvent& event);
	void OnRefreshOutfits(wxCommandEvent& event);

	void OnChooseOutfit(wxCommandEvent& event);
	void OnChoosePreset(wxCommandEvent& event);

	void OnDeleteProject(wxCommandEvent& event);
	void OnDeletePreset(wxCommandEvent& event);

	void OnSavePreset(wxCommandEvent& event);
	void OnSavePresetAs(wxCommandEvent& event);
	void OnGroupManager(wxCommandEvent& event);

	void OnPreview(wxCommandEvent& event);
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

	bool OutfitIsEmpty() {
		wxChoice* outfitChoice = (wxChoice*)FindWindowByName("outfitChoice");
		if (outfitChoice && !outfitChoice->GetStringSelection().empty())
			return false;

		return true;
	}

	BodySlideApp* app;

	wxDECLARE_EVENT_TABLE();
};
