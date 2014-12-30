#pragma once

#include "resource.h"
#include "slidermanager.h"
#include <wx/wxprec.h>
#include <wx/srchctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/imagpng.h>
#include <wx/dcbuffer.h>
#include <wx/statline.h>

#include <vector>
#include <string>
#include <map>
#include <tuple>

#include "Commctrl.h"
#include <string.h>
#include "PreviewWindow.h"
#include "DiffData.h"
#include "SliderData.h"
#include "OutfitStudio.h"
#include "SliderGroup.h"
#include "SliderCategories.h"


class BodySlideFrame;

class BodySlideApp :
	public wxApp
{
	/* UI Managers */ 
	PreviewWindow* preview0;
	PreviewWindow* preview1;

	BodySlideFrame* sliderView;
	OutfitStudio* outfitStudio;

	/* Data Managers */
	SliderManager sliderManager;
	DiffDataSets dataSets;
	SliderSet activeSet;

	/* Data Items */
	map<string, string> outfitNameSource;		// all currently defined outfits
	vector<string> outfitNameOrder;				// all currently defined outfits, in their order of appearance
	map<string, vector<string>> groupMembers;	// all currently defined groups
	map<string, string> groupAlias;				// group name aliases 
	vector<string> ungroupedOutfits;			// outfits without a group
	vector<string> filteredOutfits;				// filtered outfit names
	vector<string> presetGroups;
	vector<string> allGroups;
	SliderSetGroupCollection gCollection;

	string curOutfit;

	string previewBaseName;
	NifFile* previewBaseNif;
	NifFile PreviewMod;

	/* Data Load/Setup */ 
	void setupOutfit(const string& outfitName);
	int createSliders(const string& outfit = "CBBE Body", bool hideAll = false);
	int createSetSliders(const string& outfit, bool hideAll = false);
	int saveDiffData(const string& filename, map<int,vector3>& data);
	int loadDiffData(const string& filename, map<int,vector3>& data);

public:
	virtual ~BodySlideApp();
	virtual bool OnInit();
	
	SliderCategoryCollection cCollection;

	void SetDefaultConfig();
	void LoadData();
	void CharHook (wxKeyEvent& event) {
		wxWindow* w = (wxWindow*)event.GetEventObject();
		if(!w) {
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

		string stupidkeys = "0123456789-";
		bool stupidHack = false;
		if (event.GetKeyCode() < 256 && stupidkeys.find(event.GetKeyCode()) != string::npos)
			stupidHack = true;

		if (stupidHack && nm.EndsWith("|readout")) {
			wxTextCtrl* e = (wxTextCtrl*)w;
			HWND hwndEdit = e->GetHandle();
			::SendMessage(hwndEdit, WM_CHAR, event.GetKeyCode(), event.GetRawKeyFlags());
		} else {
			event.Skip();
		}
	}
	/*
	int FilterEvent(wxEvent& event) {
		if(event.GetEventType() == wxEVT_CHAR && outfitStudio) {
			wxMessageBox("filterevent");
		}
		return -1;
	}*/

	void LoadAllCategories();

	void SetPresetGroups(const string& setName);
	void LoadAllGroups();
	void GetAllGroupNames(vector<string>& outGroups);
	int SaveGroupList(const string& filename, const string& groupname);

	void ApplyOutfitFilter();
	int GetFilteredOutfits(vector<string>& outList);

	void LoadPresets(const string& sliderSet);
	void PopulatePresetList(const string& select);
	void PopulateOutfitList(const string& select);
	void DisplayActiveSet();

	int LoadSliderSets();
	void RefreshOutfitList();
	
	void RefreshSliders();

	void ActivateOutfit(const string& outfitName);
	void ActivatePreset(const string& presetName);

	void LaunchOutfitStudio();

	void ApplySliders(const string& targetShape, vector<Slider>& sliderSet, vector<vec3>& verts, vector<ushort>& zapidx, vector<vec2>* uvs = NULL);

	void ShowPreview(char PreviewType = SMALL_PREVIEW);
	void ClosePreview(char PreviewType = SMALL_PREVIEW) {
		if (PreviewType == SMALL_PREVIEW) {
			if (preview0)
				delete preview0;
			preview0 = NULL;
			return;
		}
		if (preview1)
			delete preview1;
		preview1 = NULL;
	}

	void CloseOutfitStudio() {
		outfitStudio = NULL;
	}

	void UpdatePreview(char PreviewType = SMALL_PREVIEW);

	void RebuildPreviewMeshes(char PreviewType);

	int CreateControls();

	void SetMessage(string msg);

	void MakeDiffFile(string shapesFile, string baseShapeName, string outDiffPath);

	void MakeDiffFromObj(string baseShapeName, string objFile);

	int BlenderNifToSkyrim();
	
	int BuildBodies(bool localPath = false, bool clean = false);
	int BuildListBodies(const vector<string>& outfitList, map<string,string>& failedOutfits, bool remove = false, const string& custPath = "");
	
	float GetSliderValue(const wxString& sliderName, bool isLo);
	void SetSliderValue(const wxString& sliderName, bool isLo, float val);
	void SetSliderChanged(const wxString& sliderName, bool isLo);
	int SaveSliderPositions(const string& outputFile, const string& presetName, vector<string>& groups);
};

#define DELAYLOAD_TIMER 299
#define SLIDER_LO 1
#define SLIDER_HI 2

class BodySlideFrame : public wxFrame
{
public:
	class SliderDisplay {
	public:
		bool isZap;
		bool oneSize;
		string sliderName;
		wxStaticText* lblSliderLo;
		wxSlider* sliderLo;
		wxTextCtrl* sliderReadoutLo;
		wxStaticText* lblSliderHi;
		wxSlider* sliderHi;
		wxTextCtrl* sliderReadoutHi;
		wxCheckBox* zapCheckHi;
		wxCheckBox* zapCheckLo;
		SliderDisplay() {
			isZap = false;
			oneSize = false;
			lblSliderLo = NULL; sliderLo = NULL; sliderReadoutLo = NULL; lblSliderLo = NULL;
			lblSliderHi = NULL; sliderHi = NULL; sliderReadoutLo = NULL; lblSliderLo = NULL;
			zapCheckHi = NULL; zapCheckLo = NULL;
		}
	};

	unordered_map<string, SliderDisplay*> sliderDisplays;

	wxTimer delayLoad;
	wxSearchCtrl* search;
	wxSearchCtrl* outfitsearch;
	wxCheckListBox* batchBuildList;

	BodySlideFrame(BodySlideApp* app, const wxString& title, const wxPoint& pos, const wxSize& size);
	~BodySlideFrame() {
		ClearSliderGUI();
	}

	void ShowLowColumn(bool show);
	void AddCategorySliderUI(const wxString& name, bool show, bool oneSize);
	void AddSliderGUI(const wxString& name, bool isZap, bool oneSize = false);

	BodySlideFrame::SliderDisplay* GetSliderDisplay(const string& name) {
		if (sliderDisplays.find(name) != sliderDisplays.end())
			return sliderDisplays[name];
		return NULL;
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
	void OnActivateFrame(wxActivateEvent& event);
	void OnIconizeFrame(wxIconizeEvent& event);
	void PostIconizeFrame();
	
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

	void OnSavePreset(wxCommandEvent& event);
	void OnPreviewHi(wxCommandEvent& event);
	void OnPreviewLo(wxCommandEvent& event);
	void OnBuildBodies(wxCommandEvent& event);
	void OnBatchBuild(wxCommandEvent& event);
	void OnBatchBuildContext(wxMouseEvent& event);
	void OnBatchBuildSelect(wxCommandEvent& event);
	void OnOutfitStudio(wxCommandEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	long MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

	BodySlideApp* app;

	DECLARE_EVENT_TABLE();
};
enum {
	ID_Hello = 1
};