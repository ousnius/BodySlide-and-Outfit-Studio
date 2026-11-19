/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The BodySlide window: creating and closing it, settings and about

#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "BodySlideApp.h"
#include "ui/AppSetup.h"
#include "ui/SettingsDialogShared.h"
#include "utils/GameUtil.h"

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <algorithm>
#include <utility>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/collpane.h>

using namespace nifly;

wxBEGIN_EVENT_TABLE(BodySlideFrame, wxFrame)
	EVT_MENU(wxID_EXIT, BodySlideFrame::OnExit)
	EVT_CLOSE(BodySlideFrame::OnClose)
	EVT_ACTIVATE(BodySlideFrame::OnActivateFrame)
	EVT_ICONIZE(BodySlideFrame::OnIconizeFrame)
	EVT_COMMAND_SCROLL(wxID_ANY, BodySlideFrame::OnSliderChange)
	EVT_TEXT_ENTER(wxID_ANY, BodySlideFrame::OnSliderReadoutChange)
	EVT_TEXT(XRCID("searchHolder"), BodySlideFrame::OnSearchChange)
	EVT_TEXT(XRCID("outfitsearchHolder"), BodySlideFrame::OnOutfitSearchChange)
	EVT_TEXT_ENTER(XRCID("sliderFilter"), BodySlideFrame::OnSliderFilterChanged)
	EVT_TEXT(XRCID("sliderFilter"), BodySlideFrame::OnSliderFilterChanged)
	EVT_TEXT_ENTER(XRCID("presetFilter"), BodySlideFrame::OnPresetFilterChanged)
	EVT_TEXT(XRCID("presetFilter"), BodySlideFrame::OnPresetFilterChanged)
	EVT_TIMER(DELAYLOAD_TIMER, BodySlideFrame::OnDelayLoad)
	EVT_COMBOBOX(XRCID("outfitChoice"), BodySlideFrame::OnChooseOutfit)
	EVT_COMBOBOX(XRCID("presetChoice"), BodySlideFrame::OnChoosePreset)
	EVT_BUTTON(XRCID("btnFavoriteOutfit"), BodySlideFrame::OnFavoriteOutfit)
	EVT_BUTTON(XRCID("btnFavoritePreset"), BodySlideFrame::OnFavoritePreset)

	EVT_BUTTON(XRCID("btnDeleteProject"), BodySlideFrame::OnDeleteProject)
	EVT_BUTTON(XRCID("btnDeletePreset"), BodySlideFrame::OnDeletePreset)
	EVT_CHECKBOX(XRCID("cbIsOutfitChoice"), BodySlideFrame::OnOutfitChoiceSelect)
	EVT_CHECKBOX(XRCID("cbConformAddOns"), BodySlideFrame::OnConformAddOns)

	EVT_BUTTON(XRCID("btnPreview"), BodySlideFrame::OnPreview)
	EVT_BUTTON(XRCID("btnHighToLow"), BodySlideFrame::OnHighToLow)
	EVT_BUTTON(XRCID("btnLowToHigh"), BodySlideFrame::OnLowToHigh)
	EVT_BUTTON(XRCID("btnBuildBatch"), BodySlideFrame::OnBatchBuild)
	EVT_BUTTON(XRCID("btnBuild"), BodySlideFrame::OnBuildBodies)
	EVT_BUTTON(XRCID("btnOutfitStudio"), BodySlideFrame::OnOutfitStudio)
	EVT_BUTTON(XRCID("btnSettings"), BodySlideFrame::OnSettings)
	EVT_BUTTON(XRCID("btnAbout"), BodySlideFrame::OnAbout)
	EVT_BUTTON(XRCID("btnEditPreset"), BodySlideFrame::OnEditPreset)
	EVT_BUTTON(XRCID("btnSavePreset"), BodySlideFrame::OnSavePreset)
	EVT_BUTTON(XRCID("btnSavePresetAs"), BodySlideFrame::OnSavePresetAs)
	EVT_BUTTON(XRCID("btnGroupManager"), BodySlideFrame::OnGroupManager)
	EVT_BUTTON(XRCID("btnChooseGroups"), BodySlideFrame::OnChooseGroups)
	EVT_BUTTON(XRCID("btnRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
	EVT_BUTTON(XRCID("btnEditProject"), BodySlideFrame::OnEditProject)

	EVT_SLIDER(XRCID("sliderClippingStrength"), BodySlideFrame::OnClippingStrengthChanged)

	EVT_MENU(XRCID("menuChooseGroups"), BodySlideFrame::OnChooseGroups)
	EVT_MENU(XRCID("menuRefreshGroups"), BodySlideFrame::OnRefreshGroups)
	EVT_MENU(XRCID("menuRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
	EVT_MENU(XRCID("menuRegexOutfits"), BodySlideFrame::OnRegexOutfits)
	EVT_MENU(XRCID("menuFilterHasZaps"), BodySlideFrame::OnFilterHasZaps)
	EVT_MENU(XRCID("menuFilterOutputWinners"), BodySlideFrame::OnFilterOutputWinners)
	EVT_MENU(XRCID("menuBrowseOutfitFolder"), BodySlideFrame::OnBrowseOutfitFolder)
	EVT_MENU(XRCID("menuSaveGroups"), BodySlideFrame::OnSaveGroups)

	EVT_MOVE_END(BodySlideFrame::OnMoveWindow)
	EVT_SIZE(BodySlideFrame::OnSetSize)
wxEND_EVENT_TABLE()

BodySlideFrame::BodySlideFrame(BodySlideApp* a, const wxSize& size)
	: delayLoad(this, DELAYLOAD_TIMER) {
	app = a;

	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/BodySlide.xrc");
	if (!loaded) {
		wxMessageBox(_("Failed to load BodySlide.xrc file!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	loaded = xrc->LoadFrame(this, GetParent(), "bodySlideFrame");
	if (!loaded) {
		wxMessageBox(_("Failed to load BodySlide frame!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	SetMinSize(FromDIP(wxSize(850, 400)));

	// --- Embed splitter with preview panel ---
	// Capture the XRC-created sizer and all children, then reparent them
	// into the left side of a splitter window.
	wxSizer* originalSizer = GetSizer();

	splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
	splitter->SetMinimumPaneSize(FromDIP(400));

	leftPanel = new wxPanel(splitter, wxID_ANY);

	// Reparent all XRC children from frame to leftPanel
	wxWindowList children = GetChildren();
	for (auto* child : children) {
		if (child != splitter)
			child->Reparent(leftPanel);
	}

	// Move the XRC sizer to leftPanel
	SetSizer(nullptr, false);
	leftPanel->SetSizer(originalSizer);
	leftPanel->SetBackgroundColour(GetBackgroundColour());
	leftPanel->SetDoubleBuffered(true);

	// Create embedded preview panel
	previewPanel = new PreviewPanel(splitter, app);

	// Read sash position, side and visibility from config
	previewVisible = BodySlideConfig.GetBoolValue("BodySlideFrame.previewVisible", false);
	previewOnLeft = BodySlideConfig.GetBoolValue("BodySlideFrame.previewOnLeft", false);
	savedSashPosition = BodySlideConfig.GetIntValue("BodySlideFrame.sashpos");
	savedPreviewWidth = BodySlideConfig.GetIntValue("BodySlideFrame.previewWidth");

	if (previewVisible) {
		if (previewOnLeft) {
			// The sash position is the preview width when the preview is the first pane
			int previewWidth = savedPreviewWidth;
			if (previewWidth <= 0)
				previewWidth = FromDIP(400);

			splitter->SplitVertically(previewPanel, leftPanel, previewWidth);
		}
		else {
			splitter->SplitVertically(leftPanel, previewPanel, savedSashPosition);
		}
	}
	else {
		splitter->Initialize(leftPanel);
		previewPanel->Hide();
	}

	// Set new top-level sizer for the frame
	wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(splitter, 1, wxEXPAND);
	SetSizer(frameSizer);

	// Connect splitter events
	splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, &BodySlideFrame::OnSashPosChanging, this);
	splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &BodySlideFrame::OnSashPosChanged, this);

	// Listen for pop-out events from the preview panel
	splitter->Bind(EVT_PREVIEW_POPOUT, &BodySlideFrame::OnPreviewPopout, this);

	outfitChoice = (wxFavoriteChoice*)FindWindowByName("outfitChoice", this);
	lblOutfitBase = (wxStaticText*)FindWindowByName("lblOutfitBase", this);
	lblOutfitAddOns = (wxStaticText*)FindWindowByName("lblOutfitAddOns", this);

	if (auto cbConformAddOns = XRCCTRL(*this, "cbConformAddOns", wxCheckBox))
		cbConformAddOns->SetValue(app->GetConformAddOnSliders());
	presetChoice = (wxFavoriteChoice*)FindWindowByName("presetChoice", this);
	btnFavoriteOutfit = (wxButton*)FindWindowByName("btnFavoriteOutfit", this);
	btnFavoritePreset = (wxButton*)FindWindowByName("btnFavoritePreset", this);
	btnSavePreset = (wxButton*)FindWindowByName("btnSavePreset", this);

	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/BatchBuild.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Settings.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/About.xrc");

	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/BodySlide.png", wxBITMAP_TYPE_PNG));
	SetSize(size);

	batchBuildList = nullptr;
	auto srchMenu = xrc->LoadMenu("menuGroupContext");
	auto outfitsrchMenu = xrc->LoadMenu("menuOutfitSrchContext");

	if (outfitsrchMenu) {
		auto menuRegexOutfits = outfitsrchMenu->FindItem(XRCID("menuRegexOutfits"));
		if (menuRegexOutfits) {
			bool regexFilterOutfits = BodySlideConfig.GetBoolValue("RegexFilterOutfits");
			menuRegexOutfits->Check(regexFilterOutfits);
		}
	}
	fileCollisionMenu = xrc->LoadMenu("menuFileCollision");

	search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->ShowCancelButton(true);
	search->SetDescriptiveText(_("Filter groups..."));
	search->SetMenu(srchMenu);

	outfitsearch = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	outfitsearch->ShowSearchButton(true);
	outfitsearch->ShowCancelButton(true);
	outfitsearch->SetDescriptiveText(_("Filter outfits..."));
	outfitsearch->SetMenu(outfitsrchMenu);

	sliderFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sliderFilter->ShowSearchButton(true);
	sliderFilter->ShowCancelButton(true);
	sliderFilter->SetDescriptiveText(_("Filter sliders..."));

	presetFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	presetFilter->ShowSearchButton(true);
	presetFilter->ShowCancelButton(true);
	presetFilter->SetDescriptiveText(_("Filter presets..."));
	presetFilter->SetToolTip(_("Search visible preset names. Presets are first limited by the selected outfit's groups."));

	int categoryTabSizerID = XRCID("categoryTabSizer");
	wxSizerItem* si = leftPanel->GetSizer()->GetItemById(categoryTabSizerID, true);

	categoryTabSizer = si ? si->GetSizer() : nullptr;

	auto conflictLabel = (wxStaticText*)FindWindowByName("conflictLabel", this);
	if (conflictLabel)
		conflictLabel->Bind(wxEVT_RIGHT_DOWN, &BodySlideFrame::OnConflictPopup, this);

	auto conflictInfo = (wxStaticText*)FindWindowByName("conflictInfo", this);
	if (conflictInfo)
		conflictInfo->Bind(wxEVT_RIGHT_DOWN, &BodySlideFrame::OnConflictPopup, this);

	// Takes the rest of the row and gets shortened, so its text doesn't widen the columns and move the buttons
	auto buildInfo = (wxStaticText*)FindWindowByName("buildInfo", this);
	if (buildInfo)
		buildInfo->SetMinSize(wxSize(FromDIP(10), -1));

	xrc->AttachUnknownControl("searchHolder", search, this);
	xrc->AttachUnknownControl("outfitsearchHolder", outfitsearch, this);
	xrc->AttachUnknownControl("sliderFilter", sliderFilter, this);
	xrc->AttachUnknownControl("presetFilter", presetFilter, this);

	sliderScroll = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (sliderScroll) {
		sliderScroll->SetScrollRate(5, 26);
		sliderScroll->SetFocusIgnoringChildren();
		sliderScroll->Bind(wxEVT_ENTER_WINDOW, &BodySlideFrame::OnEnterSliderWindow, this);

		sliderLayout = (wxFlexGridSizer*)sliderScroll->GetSizer();
	}

	wxString val = BodySlideConfig["LastGroupFilter"];
	search->ChangeValue(val);
	val = BodySlideConfig["LastOutfitFilter"];
	outfitsearch->ChangeValue(val);
	val = BodySlideConfig["LastPresetFilter"];
	presetFilter->ChangeValue(val);

	RefreshTargetGameState();

	// Create initial slider pool
	if (sliderScroll && sliderLayout) {
		const size_t minSliderPoolSize = 100;
		sliderPool.CreatePool(minSliderPoolSize, sliderScroll, sliderLayout);
	}

	// Set up accelerator entries
	wxAcceleratorEntry entries[5];
	entries[0].Set(wxACCEL_CTRL, (int)'O', XRCID("btnEditProject"));
	entries[1].Set(wxACCEL_CTRL, (int)'S', XRCID("btnSavePreset"));
	entries[2].Set(wxACCEL_CTRL | wxACCEL_ALT, (int)'S', XRCID("btnSavePresetAs"));
	entries[3].Set(wxACCEL_CTRL, (int)'G', XRCID("btnGroupManager"));
	entries[4].Set(wxACCEL_CTRL, (int)'P', XRCID("btnPreview"));

	wxAcceleratorTable accel(5, entries);
	SetAcceleratorTable(accel);

	// Update preview toggle button label
	auto btnPreview = (wxButton*)FindWindowByName("btnPreview", this);
	if (btnPreview) {
		if (previewVisible)
			btnPreview->SetLabel(_("Hide Preview"));
		else
			btnPreview->SetLabel(_("Show Preview"));
	}

	UpdateFavoriteButtons();
}

void BodySlideFrame::OnLinkClicked(wxHtmlLinkEvent& link) {
	wxLaunchDefaultBrowser(link.GetLinkInfo().GetHref());
}

void BodySlideFrame::OnEnterClose(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_RETURN) {
		wxDialog* parent = (wxDialog*)((wxWindow*)event.GetEventObject())->GetParent();
		if (!parent)
			return;

		parent->Close();
		parent->SetReturnCode(wxID_OK);
	}
	event.Skip();
}

void BodySlideFrame::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close(true);
}

void BodySlideFrame::OnClose(wxCloseEvent& WXUNUSED(event)) {
	bool previewWindowOpen = app->IsPreviewPoppedOut();
	bool previewWasPoppedOut = previewWindowOpen || BodySlideConfig.GetBoolValue("BodySlideFrame.previewPoppedOut", false);
	bool previewWasVisible = previewVisible || previewWindowOpen;

	app->CleanupPreview();
	app->ClosePreview();

	BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", previewWasVisible);
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", previewWasPoppedOut);

	sliderPool.Clear();
	sliderDisplays.clear();

	for (auto& cat : sliderCategories) {
		cat.second->Destroy();
		delete cat.second;
	}

	sliderCategories.clear();

	auto cbMorphs = XRCCTRL(*this, "cbMorphs", wxCheckBox);
	if (cbMorphs && cbMorphs->IsShown())
		BodySlideConfig.SetBoolValue("BuildMorphs", cbMorphs->GetValue());

	auto cbForceBodyNormals = XRCCTRL(*this, "cbForceBodyNormals", wxCheckBox);
	if (cbForceBodyNormals)
		BodySlideConfig.SetBoolValue("ForceBodyNormals", cbForceBodyNormals->GetValue());

	auto menuOutfitSrchContext = outfitsearch->GetMenu();
	if (menuOutfitSrchContext) {
		auto menuRegexOutfits = menuOutfitSrchContext->FindItem(XRCID("menuRegexOutfits"));
		if (menuRegexOutfits)
			BodySlideConfig.SetBoolValue("RegexFilterOutfits", menuRegexOutfits->IsChecked());
	}

	app->SaveFavorites();

	int ret = BodySlideConfig.SaveConfig(Config["AppDir"] + "/BodySlide.xml", "BodySlideConfig");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);

	wxLogMessage("BodySlide closed.");
	Destroy();
}

void BodySlideFrame::OnActivateFrame(wxActivateEvent& event) {
	event.Skip();
	if (event.GetActive()) {
		sliderScroll->SetFocusIgnoringChildren();
	}
}

void BodySlideFrame::OnIconizeFrame(wxIconizeEvent& event) {
	event.Skip();
	if (!event.IsIconized()) {
		lastScroll = sliderScroll->GetScrollPos(wxVERTICAL);
		CallAfter(&BodySlideFrame::PostIconizeFrame);
	}
}

void BodySlideFrame::PostIconizeFrame() {
	sliderScroll->SetFocusIgnoringChildren();
	sliderScroll->Scroll(0, lastScroll);
}

void BodySlideFrame::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	int i = 0;
	i++;
	return;
}

void BodySlideFrame::OnDelayLoad(wxTimerEvent& WXUNUSED(event)) {
	if (app)
		app->LoadData();

	delayLoad.Stop();
}

void BodySlideFrame::OnOutfitStudio(wxCommandEvent& WXUNUSED(event)) {
	app->LaunchOutfitStudio();
}

void BodySlideFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(settings->FromDIP(wxSize(525, -1)));
		settings->SetMinSize(settings->FromDIP(wxSize(525, -1)));
		settings->CenterOnParent();

		wxCollapsiblePane* advancedPane = XRCCTRL(*settings, "advancedPane", wxCollapsiblePane);
		advancedPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&settings](wxCommandEvent&) { settings->Fit(); });

		const std::string prevSkeletonReference = Config["Anim/DefaultSkeletonReference"];

		SettingsDialogShared::CommonSettingsDialogControls commonControls{};
		SettingsDialogShared::InitCommonSettingsDialog(*settings,
			Config,
			BodySlideConfig,
			AppSetup::SupportedLangs.data(),
			AppSetup::SupportedLangs.size(),
			commonControls);

		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);

		wxCheckBox* cbPreviewAlwaysDetached = XRCCTRL(*settings, "cbPreviewAlwaysDetached", wxCheckBox);
		cbPreviewAlwaysDetached->SetValue(BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false));

		wxCheckBox* cbPreviewOnLeft = XRCCTRL(*settings, "cbPreviewOnLeft", wxCheckBox);
		cbPreviewOnLeft->SetValue(BodySlideConfig.GetBoolValue("BodySlideFrame.previewOnLeft", false));

		wxCheckBox* cbConformAddOnSliders = XRCCTRL(*settings, "cbConformAddOnSliders", wxCheckBox);
		cbConformAddOnSliders->SetValue(BodySlideConfig.GetBoolValue("ConformAddOnSliders", false));

		// Hide the single instance setting (only relevant for Outfit Studio)
		XRCCTRL(*settings, "lbSingleInstanceBehavior", wxStaticText)->Hide();
		XRCCTRL(*settings, "choiceSingleInstanceBehavior", wxChoice)->Hide();
		XRCCTRL(*settings, "cbMcpServer", wxCheckBox)->Hide();
		XRCCTRL(*settings, "spinMcpPort", wxSpinCtrl)->Hide();

		AppSetup::FillDataFileList(commonControls.dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		commonControls.choiceTargetGame->Bind(wxEVT_CHOICE, &AppSetup::OnChooseTargetGame);

		if (settings->ShowModal() == wxID_OK) {
			int targetGameSelection = 0;
			bool needsRestart = false;
			SettingsDialogShared::SaveCommonSettingsDialog(
				Config,
				BodySlideConfig,
				GameUtil::TargetGames.data(),
				GameUtil::TargetGames.size(),
				AppSetup::SupportedLangs.data(),
				AppSetup::SupportedLangs.size(),
				commonControls,
				[this]() { app->InitLanguage(); },
				targetGameSelection,
				needsRestart);

			TargetGame targ = (TargetGame)targetGameSelection;

			BodySlideConfig.SetBoolValue("BodySlideFrame.previewAlwaysDetached", cbPreviewAlwaysDetached->IsChecked());
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewOnLeft", cbPreviewOnLeft->IsChecked());
			SetPreviewOnLeft(cbPreviewOnLeft->IsChecked());
			BodySlideConfig.SetBoolValue("ConformAddOnSliders", cbConformAddOnSliders->IsChecked());

			// Only feeds a uniform, so the meshes and their textures stay as they are
			app->ApplyComplexMaterialSetting();

			// This one picks the shader files, so it rebuilds the preview meshes when it changed
			app->ApplyPBRSetting();

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
			app->SaveFavorites();
			// Poses and animations are made for one game's skeleton, and the
			// preview skinning was built on the skeleton loaded before
			const bool skeletonChanged = targ != app->targetGame || Config["Anim/DefaultSkeletonReference"] != prevSkeletonReference;
			app->targetGame = targ;
			if (skeletonChanged)
				app->ResetPreviewPoses();
			app->LoadFavorites();
			GameUtil::InitArchives();
			app->RetryMissingTextures();
			app->LoadAllCategories();
			app->LoadAllGroups();
			app->LoadSliderSets();
			app->LoadData();

			RefreshTargetGameState();
			Layout();

			if (needsRestart) {
				wxMessageBox(_("Settings changed. Please restart the application for changes to take effect."), _("Settings Changed"), wxOK | wxICON_INFORMATION);
			}
		}

		delete settings;
	}
}

void BodySlideFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* about = wxXmlResource::Get()->LoadDialog(this, "dlgAbout");
	if (about) {
		about->SetSize(about->FromDIP(wxSize(625, 375)));
		about->SetMinSize(about->FromDIP(wxSize(625, 375)));
		about->CenterOnParent();
		about->Bind(wxEVT_CHAR_HOOK, &BodySlideFrame::OnEnterClose, this);
		about->Bind(wxEVT_HTML_LINK_CLICKED, &BodySlideFrame::OnLinkClicked, this);
		about->ShowModal();
		delete about;
	}
}

void BodySlideFrame::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = GetPosition();
	BodySlideConfig.SetValue("BodySlideFrame.x", p.x);
	BodySlideConfig.SetValue("BodySlideFrame.y", p.y);
	event.Skip();
}

void BodySlideFrame::OnSetSize(wxSizeEvent& event) {
	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		BodySlideConfig.SetValue("BodySlideFrame.width", p.x);
		BodySlideConfig.SetValue("BodySlideFrame.height", p.y);
	}

	BodySlideConfig.SetBoolValue("BodySlideFrame.maximized", maximized);
	event.Skip();
}

void BodySlideFrame::OnEditProject(wxCommandEvent& WXUNUSED(event)) {
	std::string projectName = BodySlideConfig["SelectedOutfit"];
	app->EditProject(projectName);
}

void BodySlideFrame::RefreshTargetGameState() {
	auto cbMorphs = XRCCTRL(*this, "cbMorphs", wxCheckBox);
	if (cbMorphs) {
		bool buildMorphsDef = BodySlideConfig.GetBoolValue("BuildMorphs");

		switch (app->targetGame) {
			case SKYRIM:
			case FO4:
			case FO4VR:
			case SKYRIMSE:
			case SKYRIMVR:
			case SF:
				cbMorphs->SetValue(buildMorphsDef);
				cbMorphs->Show();
				break;
			default:
				cbMorphs->SetValue(false);
				cbMorphs->Hide();
				break;
		}
	}

	auto cbForceBodyNormals = XRCCTRL(*this, "cbForceBodyNormals", wxCheckBox);
	if (cbForceBodyNormals) {
		if (Config.GetBoolValue("ShowForceBodyNormals")) {
			bool forceBodyNormalsDef = BodySlideConfig.GetBoolValue("ForceBodyNormals");
			cbForceBodyNormals->SetValue(forceBodyNormalsDef);

			switch (app->targetGame) {
				case SKYRIMSE:
				case SKYRIMVR: cbForceBodyNormals->Show(); break;
				default: break;
			}
		}
		else
			cbForceBodyNormals->Hide();
	}
}
