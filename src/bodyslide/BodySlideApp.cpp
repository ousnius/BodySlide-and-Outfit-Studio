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

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "PreviewWindow.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "FSEngine/FSManager.h"
#include "ui/AppSetup.h"
#include "files/wxDDSImage.h"
#include "ui/SettingsDialogShared.h"
#include "utils/StackTrace.h"
#include "utils/StringStuff.h"
#include "utils/GameUtil.h"

#include <algorithm>
#include <utility>
#include <wx/debugrpt.h>
#include <wx/textctrl.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;
ConfigurationManager Config;
ConfigurationManager BodySlideConfig;

wxIMPLEMENT_APP(BodySlideApp);

BodySlideApp::~BodySlideApp() {
	++previewLoadGeneration;
	if (previewLoadThread.joinable())
		previewLoadThread.join();

	delete locale;
	locale = nullptr;

	FSManager::del();
}

bool BodySlideApp::OnInit() {
	if (!wxApp::OnInit())
		return false;

#ifdef _DEBUG
	std::string dataDir{wxGetCwd().ToUTF8()};
#else
	std::string dataDir{wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8()};
#endif

	Config.LoadConfig(dataDir + "/Config.xml");
	BodySlideConfig.LoadConfig(dataDir + "/BodySlide.xml", "BodySlideConfig");

	Config.SetDefaultValue("AppDir", dataDir);

	logger.Initialize(Config.GetIntValue("LogLevel", 2), dataDir + "/Log_BS.txt");
	wxLogMessage("Initializing BodySlide...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

#ifdef __WXMSW__
	SetAppearance(SettingsDialogShared::GetConfiguredAppearance(Config));
#endif

	wxString appDirUri = wxString::FromUTF8(dataDir);
	appDirUri.Replace("#", "%23");
	wxSetEnv("AppDir", appDirUri);

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);
	xrc->InitAllHandlers();
	wxInitAllImageHandlers();
	wxImage::AddHandler(new wxDDSHandler);

	preview = nullptr;
	previewWindow = nullptr;
	sliderView = nullptr;

	Bind(wxEVT_CHAR_HOOK, &BodySlideApp::CharHook, this);

	wxLogMessage("Working directory: %s", wxGetCwd());
	wxLogMessage("Executable directory: %s", wxString::FromUTF8(dataDir));
	if (!SetDefaultConfig())
		return false;

	conformAddOnSliders = BodySlideConfig.GetBoolValue("ConformAddOnSliders");

	LoadFavorites();

	InitLanguage();

	wxString gameName = "Target game: ";
	switch (targetGame) {
		case FO3: gameName.Append("Fallout 3"); break;
		case FONV: gameName.Append("Fallout New Vegas"); break;
		case SKYRIM: gameName.Append("Skyrim"); break;
		case FO4: gameName.Append("Fallout 4"); break;
		case SKYRIMSE: gameName.Append("Skyrim Special Edition"); break;
		case FO4VR: gameName.Append("Fallout 4 VR"); break;
		case SKYRIMVR: gameName.Append("Skyrim VR"); break;
		case FO76: gameName.Append("Fallout 76"); break;
		case OB: gameName.Append("Oblivion"); break;
		case SF: gameName.Append("Starfield"); break;
		default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	// Handle preview mode - open nif files directly without main frame
	if (cmdPreviewMode && !cmdPreviewNifs.empty()) {
		wxLogMessage("BodySlide preview mode initialized.");

		// The main frame's data loading, which preview mode skips, is what makes the archives available
		// otherwise. Without them the preview misses every texture, material and mesh kept in one.
		GameUtil::InitArchives();

		// Resolve the preset of a preset file given on the command line before the projects are loaded.
		LoadCmdPresetFile();

		ShowPreview();
		if (preview) {
			preview->SetReadOnlyMode(true);
			LoadPreviewNifs(cmdPreviewNifs);
		}
		return true;
	}

	int x = BodySlideConfig.GetIntValue("BodySlideFrame.x");
	int y = BodySlideConfig.GetIntValue("BodySlideFrame.y");
	int w = BodySlideConfig.GetIntValue("BodySlideFrame.width");
	int h = BodySlideConfig.GetIntValue("BodySlideFrame.height");
	std::string maximized = BodySlideConfig["BodySlideFrame.maximized"];
	bool savedPreviewVisible = BodySlideConfig.GetBoolValue("BodySlideFrame.previewVisible", false);
	bool savedPreviewPoppedOut = BodySlideConfig.GetBoolValue("BodySlideFrame.previewPoppedOut", false);
	bool previewAlwaysDetached = BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false);
	bool restorePreviewPoppedOut = savedPreviewPoppedOut || previewAlwaysDetached;

	wxLogMessage("Loading BodySlide frame at X:%d Y:%d with W:%d H:%d...", x, y, w, h);
	sliderView = new BodySlideFrame(this, wxSize(w, h));
	sliderView->SetPosition(wxPoint(x, y));
	if (maximized == "true")
		sliderView->Maximize();

	// Set preview pointer to the embedded panel before any data loading
	InitPreviewPanel();
	if (savedPreviewVisible && savedPreviewPoppedOut && sliderView->previewVisible) {
		// Do not auto-open detached preview at startup; convert persisted open state to hidden.
		sliderView->UnsplitPreview();
		sliderView->previewVisible = false;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", false);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
		sliderView->UpdatePreviewButtonLabel();
	}
	else if (restorePreviewPoppedOut && sliderView->previewVisible) {
		PopOutPreview();
	}

	sliderView->Show();
	SetTopWindow(sliderView);

	if (!GetOutputDataPath().empty()) {
		bool dirWritable = wxFileName::IsDirWritable(GetOutputDataPath());
		bool dirReadable = wxFileName::IsDirReadable(GetOutputDataPath());
		if ((!dirWritable || !dirReadable) && cmdQuiet)
			wxLogWarning("No read/write permission for game data path '%s'.", GetOutputDataPath());
		else if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!Config["ProjectPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["ProjectPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["ProjectPath"]);
		if ((!dirWritable || !dirReadable) && cmdQuiet)
			wxLogWarning("No read/write permission for project path '%s'.", Config["ProjectPath"]);
		else if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for project path!\n\nPlease launch the program with admin elevation and make sure the project path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	LoadAllCategories();
	LoadAllGroups();
	LoadSliderSets();

	wxLogMessage("BodySlide initialized.");

	if (HasCmdLineBuild())
		CommandLineBuild();
	else
		sliderView->delayLoad.Start(100, true);

	return true;
}

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {{wxCMD_LINE_OPTION, "gbuild", "groupbuild", "builds the specified group on launch", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"b",
													"build",
													"builds the specified outfits on launch, a single outfit name or a list of them separated by ',', ';' or '|'",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"f",
													"filter",
													"builds all outfits matching the specified filter on launch, works like the outfit filter box",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_SWITCH, "regex", "regexfilter", "treats the value of the filter option as a regular expression"},
												   {wxCMD_LINE_OPTION, "t", "targetdir", "build target directory, defaults to game data path", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"p",
													"preset",
													"preset to load on launch, use for the build or apply in preview mode, either a preset name or the "
													"path to a preset XML file optionally followed by '?' and a preset name, defaults to last used preset",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_SWITCH, "tri", "trimorphs", "enables tri morph output for the specified build"},
												   {wxCMD_LINE_SWITCH,
													"conformaddons",
													"conformaddons",
													"conforms the sliders of the body's add-ons to outfits that don't have them, for the specified build"},
												   {wxCMD_LINE_SWITCH,
													"q",
													"quiet",
													"builds without dialogs: conflicting outputs take the saved or first choice without saving it, failures are only logged"},
												   {wxCMD_LINE_OPTION, "preview", "preview", "open the specified nif files in preview mode", wxCMD_LINE_VAL_STRING},
												   wxCMD_LINE_DESC_END};

void BodySlideApp::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
}

bool BodySlideApp::OnCmdLineParsed(wxCmdLineParser& parser) {
	wxString gbuild;
	parser.Found("gbuild", &gbuild);

	wxStringTokenizer tokenizer(gbuild, ",");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken().Trim();
		if (!token.IsEmpty()) {
			std::string groupName = token.ToUTF8().data();
			cmdGroupBuild.push_back(groupName);
		}
	}

	wxString buildOutfits;
	parser.Found("b", &buildOutfits);

	// Outfit names can contain commas, so semicolons and pipes are accepted as separators as well.
	wxStringTokenizer outfitTokenizer(buildOutfits, ",;|");
	while (outfitTokenizer.HasMoreTokens()) {
		wxString token = outfitTokenizer.GetNextToken().Trim(true).Trim(false);
		if (!token.IsEmpty()) {
			std::string outfitName = token.ToUTF8().data();
			cmdBuildOutfits.push_back(outfitName);
		}
	}

	wxString buildFilter;
	if (parser.Found("f", &buildFilter)) {
		buildFilter.Trim(true).Trim(false);
		cmdBuildFilter = buildFilter.ToUTF8().data();
	}

	cmdBuildFilterRegex = parser.Found("regex");

	wxString targetDir;
	parser.Found("t", &targetDir);

	if (!targetDir.IsEmpty() && !targetDir.EndsWith(PathSepChar))
		targetDir.Append(PathSepChar);

	cmdTargetDir = targetDir.ToUTF8().data();

	wxString preset;
	if (parser.Found("p", &preset)) {
		preset.Trim(true).Trim(false);

		// The preset can either be given by name or as the path to a preset XML file.
		// A preset file can be followed by '?' and the name of one of its presets, same as for the preview option.
		wxString presetPath = preset;
		wxString presetName;

		int nameSep = preset.Find('?');
		if (nameSep != wxNOT_FOUND) {
			presetPath = preset.Left(nameSep).Trim(true).Trim(false);
			presetName = preset.Mid(nameSep + 1).Trim(true).Trim(false);
		}

		if (presetPath.Lower().EndsWith(".xml")) {
			wxFileName presetFileName(presetPath);
			presetFileName.MakeAbsolute();
			cmdPresetFile = presetFileName.GetFullPath().ToUTF8().data();
			cmdPreset = presetName.ToUTF8().data();
		}
		else
			cmdPreset = preset.ToUTF8().data();

		cmdPresetPending = !cmdPreset.empty() || !cmdPresetFile.empty();
	}

	cmdTri = parser.Found("tri");
	cmdQuiet = parser.Found("q");
	cmdConformAddOns = parser.Found("conformaddons");

	wxString previewFiles;
	if (parser.Found("preview", &previewFiles)) {
		cmdPreviewMode = true;
		wxStringTokenizer previewTokenizer(previewFiles, ",;|");
		while (previewTokenizer.HasMoreTokens()) {
			wxString token = previewTokenizer.GetNextToken().Trim();
			if (!token.IsEmpty()) {
				std::string filePath = token.ToUTF8().data();
				cmdPreviewNifs.push_back(filePath);
			}
		}
	}

	return true;
}

bool BodySlideApp::OnExceptionInMainLoop() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	if (sliderView)
		sliderView->delayLoad.Stop();

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unexpected exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unexpected exception has occurred: %s, the program will terminate."), error), _("Unexpected exception"), wxICON_ERROR);
	return false;
}

void BodySlideApp::OnUnhandledException() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	if (sliderView)
		sliderView->delayLoad.Stop();

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unhandled exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unhandled exception has occurred: %s, the program will terminate."), error), _("Unhandled exception"), wxICON_ERROR);
}

void BodySlideApp::OnFatalException() {
	if (sliderView)
		sliderView->delayLoad.Stop();

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Fatal exception has occurred, the program will terminate.");
	LogStackTraceFromException();

	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);

	wxDebugReport report;
	report.AddExceptionContext();
	report.Process();
}

void BodySlideApp::LoadData() {
	if (!sliderView)
		return;

	wxLogMessage("Loading initial data...");
	GameUtil::InitArchives();

	std::string activeOutfit = BodySlideConfig["SelectedOutfit"];
	projectCatalog.IsStackedAddOn(activeOutfit, &activeOutfit);
	if (!activeOutfit.empty() && !OutfitExists(activeOutfit)) {
		wxLogMessage("Previously selected outfit '%s' no longer exists, clearing.", activeOutfit);
		activeOutfit.clear();
		BodySlideConfig.SetValue("SelectedOutfit", activeOutfit);
	}
	if (activeOutfit.empty() && !outfitNameOrder.empty()) {
		activeOutfit = outfitNameOrder.front();
		BodySlideConfig.SetValue("SelectedOutfit", activeOutfit);
	}

	sliderView->Freeze();
	sliderView->ClearOutfitList();
	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	if (preview)
		CleanupPreview();

	if (previewWindow)
		ClosePreview();

	sliderManager.ClearSliders();
	sliderManager.ClearPresets();

	if (!activeOutfit.empty()) {
		wxLogMessage("Setting up set '%s'...", activeOutfit);

		int error = CreateSetSliders(activeOutfit);
		if (error)
			wxLogError("Failed to load set '%s' from slider set list (%d).", activeOutfit, error);

		SetPresetGroups(activeOutfit);
		LoadPresets(activeOutfit);

		std::string activePreset = BodySlideConfig["SelectedPreset"];

		if (cmdPresetPending) {
			// A preset was specified on the command line, apply it instead of the last used one.
			cmdPresetPending = false;

			if (GetPresetFileName(cmdPreset).empty())
				wxLogWarning("Preset '%s' from the command line is not available for set '%s', keeping preset '%s'.", cmdPreset, activeOutfit, activePreset);
			else {
				wxLogMessage("Applying preset '%s' from the command line.", cmdPreset);
				activePreset = cmdPreset;

				// Make sure a filter of the last session doesn't hide the preset in the list.
				if (sliderView->presetFilter)
					sliderView->presetFilter->ChangeValue("");
			}
		}

		PopulatePresetList(activePreset);
		ActivatePreset(activePreset);

		wxLogMessage("Finished setting up '%s'.", activeOutfit);
	}

	PopulateOutfitList(activeOutfit);

	sliderView->Thaw();
	sliderView->Layout();
	if (sliderView->leftPanel)
		sliderView->leftPanel->Layout();

	// Trigger initial preview load for embedded panel
	if (preview && preview->IsGLInitialized() && !previewWindow && !activeOutfit.empty()) {
		InitPreview();
	}
}

void BodySlideApp::CharHook(wxKeyEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w) {
		event.Skip();
		return;
	}

	wxString nm = w->GetName();
	int keyCode = event.GetKeyCode();

	if (event.ControlDown()) {
		if (event.ShiftDown()) {
			if (keyCode == (int)'A') {
				if (sliderView) {
					if (sliderView->outfitsearch)
						sliderView->outfitsearch->Clear();

					if (sliderView->search)
						sliderView->search->Clear();

					if (sliderView->sliderFilter)
						sliderView->sliderFilter->Clear();

					if (sliderView->presetFilter)
						sliderView->presetFilter->Clear();
				}
				return;
			}
		}

		if (keyCode == wxKeyCode::WXK_PAGEUP) {
			if (sliderView->outfitChoice) {
				int curSel = sliderView->outfitChoice->GetSelection();
				if (curSel > 0) {
					sliderView->outfitChoice->SetSelection(curSel - 1);
					std::string outfitName = sliderView->GetSelectedOutfitName();
					if (!outfitName.empty())
						ActivateOutfit(outfitName);
				}
			}
			return;
		}

		if (keyCode == wxKeyCode::WXK_PAGEDOWN) {
			if (sliderView->outfitChoice) {
				int curSel = sliderView->outfitChoice->GetSelection();
				int curCount = sliderView->outfitChoice->GetCount();
				if (curCount > 0 && curSel < curCount - 1) {
					sliderView->outfitChoice->Select(curSel + 1);
					std::string outfitName = sliderView->GetSelectedOutfitName();
					if (!outfitName.empty())
						ActivateOutfit(outfitName);
				}
			}
			return;
		}
	}
	else {
		if (keyCode == wxKeyCode::WXK_F5) {
			if (nm == "outfitChoice")
				RefreshOutfitList();
			return;
		}

		if (keyCode == wxKeyCode::WXK_PAGEUP) {
			if (sliderView->presetChoice) {
				int curSel = sliderView->presetChoice->GetSelection();
				if (curSel > 0) {
					sliderView->presetChoice->SetSelection(curSel - 1);
					ActivatePreset(sliderView->GetSelectedPresetName());
				}
			}
			return;
		}

		if (keyCode == wxKeyCode::WXK_PAGEDOWN) {
			if (sliderView->presetChoice) {
				int curSel = sliderView->presetChoice->GetSelection();
				int curCount = sliderView->presetChoice->GetCount();
				if (curCount > 0 && curSel < curCount - 1) {
					sliderView->presetChoice->Select(curSel + 1);
					ActivatePreset(sliderView->GetSelectedPresetName());
				}
			}
			return;
		}
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

bool BodySlideApp::SetDefaultConfig() {
	int xborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_X);
	if (xborder < 0)
		xborder = 0;
	int yborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_Y);
	if (yborder < 0)
		yborder = 0;

	int currentTarget = AppSetup::SetSharedConfigDefaults();
	BodySlideConfig.SetDefaultValue("SelectedOutfit", "");
	BodySlideConfig.SetDefaultValue("SelectedPreset", "");
	BodySlideConfig.SetDefaultBoolValue("BuildMorphs", false);
	BodySlideConfig.SetDefaultBoolValue("ConformAddOnSliders", false);
	BodySlideConfig.SetDefaultBoolValue("RegexFilterOutfits", false);
	const wxSize bodySlideFrameSize = wxWindow::FromDIP(wxSize(1040, 840), nullptr);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.width", bodySlideFrameSize.GetWidth());
	BodySlideConfig.SetDefaultValue("BodySlideFrame.height", bodySlideFrameSize.GetHeight());
	BodySlideConfig.SetDefaultValue("BodySlideFrame.x", 100);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.y", 100);
	const int bodySlideFrameSashPos = wxWindow::FromDIP(980, nullptr);
	const int bodySlideFramePreviewWidth = wxWindow::FromDIP(620, nullptr);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.sashpos", bodySlideFrameSashPos);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.previewWidth", bodySlideFramePreviewWidth);
	BodySlideConfig.SetDefaultBoolValue("BodySlideFrame.previewVisible", false);
	BodySlideConfig.SetDefaultBoolValue("BodySlideFrame.previewPoppedOut", false);
	BodySlideConfig.SetDefaultBoolValue("BodySlideFrame.previewAlwaysDetached", false);
	BodySlideConfig.SetDefaultBoolValue("BodySlideFrame.previewOnLeft", false);

	const wxSize previewSize = wxWindow::FromDIP(wxSize(720 + xborder * 2, 720 + yborder * 2), nullptr);
	BodySlideConfig.SetDefaultValue("PreviewFrame.width", previewSize.GetWidth());
	BodySlideConfig.SetDefaultValue("PreviewFrame.height", previewSize.GetHeight());
	BodySlideConfig.SetDefaultValue("PreviewFrame.x", 100);
	BodySlideConfig.SetDefaultValue("PreviewFrame.y", 100);

	AppSetup::SetGameRegistryDefaults();

	// Target game not set, show setup dialog
	if (currentTarget == -1)
		if (!ShowSetup())
			return false;

	targetGame = (TargetGame)Config.GetIntValue("TargetGame");

#ifdef _WINDOWS
	if (Config["GameDataPath"].empty()) {
		wxString installPath = GameUtil::GetRegistryDataPath(targetGame);
		if (!installPath.empty()) {
			Config.SetDefaultValue("GameDataPath", installPath.ToUTF8().data());
			wxLogMessage("Registry game data path: %s", installPath);
		}
	}
#endif

	if (Config["GameDataPath"].empty()) {
		if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
			if (!cmdQuiet)
				wxMessageBox(_("Failed to find game install path registry key or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
		}
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);

	if (!Config["OutputDataPath"].empty()) {
		wxLogMessage("Output data path in config: %s", Config["OutputDataPath"]);
	}

	if (!Config["ProjectPath"].empty()) {
		wxLogMessage("Project path in config: %s", Config["ProjectPath"]);
	}

	return true;
}

void BodySlideApp::ApplyComplexMaterialSetting() {
	if (preview)
		preview->SetComplexMaterialEnabled(BodySlideConfig.GetBoolValue("Rendering/ComplexMaterial", true));
}

void BodySlideApp::RetryMissingTextures() {
	if (preview)
		preview->RetryMissingTextures();
}

void BodySlideApp::ApplyPBRSetting() {
	if (!preview)
		return;

	// True PBR decides which shader files a shape is given rather than feeding a uniform, and the pair
	// is picked while the textures are assigned, so the meshes have to be built again to pick it up.
	const bool pbrEnabled = BodySlideConfig.GetBoolValue("Rendering/TruePBR", true);
	if (pbrEnabled == preview->IsPBREnabled())
		return;

	preview->SetPBREnabled(pbrEnabled);
	RebuildPreviewMeshes();
}

bool BodySlideApp::ShowSetup() {
	return AppSetup::ShowSetupDialog();
}

void BodySlideApp::InitLanguage() {
	AppSetup::InitLanguage(locale);
}
