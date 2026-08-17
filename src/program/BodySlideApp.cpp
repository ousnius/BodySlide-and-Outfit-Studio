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
#include "../components/ClippingFixer.h"
#include "../components/Mesh.h"
#include "../files/GameDataStream.h"
#include "../files/SFMorphFile.h"
#include "../files/wxDDSImage.h"
#include "../utils/SettingsDialogShared.h"
#include "../utils/PlatformUtil.h"
#include "../utils/ParallelFor.h"
#include "../utils/StackTrace.h"
#include "../utils/StringStuff.h"
#include "../utils/ProjectUtil.h"
#include "../utils/GameUtil.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <regex>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <wx/wrapsizer.h>
#include <wx/debugrpt.h>
#include <wx/textctrl.h>

using namespace nifly;

ConfigurationManager Config;
ConfigurationManager BodySlideConfig;

namespace {
constexpr const char* FavoriteStar = "\xE2\x98\x85";
constexpr char FavoriteSeparator = ';';
constexpr char FavoriteEscape = '\\';
constexpr const char* FavoriteStarIcon = "/res/images/FavoriteStar.png";
constexpr const char* FavoriteStarEmptyIcon = "/res/images/FavoriteStarEmpty.png";
constexpr int MinBodySlideLeftPaneWidthDip = 850;
}

const std::array<wxLanguage, 37> SupportedLangs = {wxLANGUAGE_ENGLISH,	  wxLANGUAGE_AFRIKAANS,		   wxLANGUAGE_ARABIC,  wxLANGUAGE_CATALAN,	  wxLANGUAGE_CZECH,
												   wxLANGUAGE_DANISH,	  wxLANGUAGE_GERMAN,		   wxLANGUAGE_GREEK,   wxLANGUAGE_SPANISH,	  wxLANGUAGE_BASQUE,
												   wxLANGUAGE_FINNISH,	  wxLANGUAGE_FRENCH,		   wxLANGUAGE_HINDI,   wxLANGUAGE_HUNGARIAN,  wxLANGUAGE_INDONESIAN,
												   wxLANGUAGE_ITALIAN,	  wxLANGUAGE_JAPANESE,		   wxLANGUAGE_KOREAN,  wxLANGUAGE_LITHUANIAN, wxLANGUAGE_LATVIAN,
												   wxLANGUAGE_MALAY,	  wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI,  wxLANGUAGE_DUTCH,	  wxLANGUAGE_POLISH,
												   wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN,		   wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK,	  wxLANGUAGE_SLOVENIAN,
												   wxLANGUAGE_ALBANIAN,	  wxLANGUAGE_SWEDISH,		   wxLANGUAGE_TAMIL,   wxLANGUAGE_TURKISH,	  wxLANGUAGE_UKRAINIAN,
												   wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE};

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
	EVT_CHOICE(XRCID("outfitChoice"), BodySlideFrame::OnChooseOutfit)
	EVT_CHOICE(XRCID("presetChoice"), BodySlideFrame::OnChoosePreset)
	EVT_BUTTON(XRCID("btnFavoriteOutfit"), BodySlideFrame::OnFavoriteOutfit)
	EVT_BUTTON(XRCID("btnFavoritePreset"), BodySlideFrame::OnFavoritePreset)

	EVT_BUTTON(XRCID("btnDeleteProject"), BodySlideFrame::OnDeleteProject)
	EVT_BUTTON(XRCID("btnDeletePreset"), BodySlideFrame::OnDeletePreset)
	EVT_CHECKBOX(XRCID("cbIsOutfitChoice"), BodySlideFrame::OnOutfitChoiceSelect)

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
		if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!Config["ProjectPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["ProjectPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["ProjectPath"]);
		if (!dirWritable || !dirReadable)
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

int BodySlideApp::CreateSetSliders(const std::string& outfit) {
	wxLogMessage("Creating sliders...");
	if (outfitNameSource.find(outfit) == outfitNameSource.end())
		return 1;

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfitNameSource[outfit]);
	if (!sliderDoc.fail()) {
		projects.clear();
		projects.push_back(std::make_unique<ProjectData>());
		auto& activeSet = GetActiveSet();
		activeSet.Clear();
		sliderManager.ClearSliders();
		if (!sliderDoc.GetSet(outfit, activeSet)) {
			activeSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
			sliderManager.AddSlidersInSet(activeSet);
			DisplayActiveSet();
		}
		else
			return 3;
	}
	else
		return 2;

	return 0;
}

int BodySlideApp::AddProjectSliders(const std::string& projectFile, const std::string& setName) {
	wxLogMessage("Adding project sliders from '%s' set '%s'...", projectFile, setName);

	if (projects.empty()) {
		// First project: clear shared data managers
		sliderManager.ClearSliders();
		sliderManager.ClearPresets();
		multiProjectMode = true;
	}

	auto pp = std::make_unique<ProjectData>();

	SliderSetFile sliderDoc;
	sliderDoc.Open(projectFile);
	if (sliderDoc.fail())
		return 2;

	if (sliderDoc.GetSet(setName, pp->sliderSet))
		return 3;

	pp->sliderSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
	pp->setName = setName;

	// Add sliders from this set (additive)
	sliderManager.AddSlidersInSet(pp->sliderSet);

	projects.push_back(std::move(pp));
	return 0;
}

std::string BodySlideApp::GetOutputDataPath() const {
	std::string res = Config["OutputDataPath"];
	return res.empty() ? Config["GameDataPath"] : res;
}


bool BodySlideApp::PresetExists(const std::string& name) {
	if (name.empty())
		return false;

	std::vector<std::string> presetNames;
	sliderManager.GetPresetNames(presetNames);
	return std::find(presetNames.begin(), presetNames.end(), name) != presetNames.end();
}

std::string BodySlideApp::GetFavoriteConfigKey(const std::string& listName) const {
	return "Favorites/" + GameUtil::TargetGames[targetGame].ToStdString() + "/" + listName;
}

std::string BodySlideApp::SerializeFavoriteNames(const std::vector<std::string>& names) const {
	std::string result;
	for (size_t i = 0; i < names.size(); i++) {
		if (i > 0)
			result.push_back(FavoriteSeparator);

		for (char c : names[i]) {
			if (c == FavoriteSeparator || c == FavoriteEscape)
				result.push_back(FavoriteEscape);

			result.push_back(c);
		}
	}

	return result;
}

std::vector<std::string> BodySlideApp::DeserializeFavoriteNames(const std::string& value) const {
	std::vector<std::string> names;
	std::string current;
	bool escaped = false;

	for (char c : value) {
		if (escaped) {
			current.push_back(c);
			escaped = false;
		}
		else if (c == FavoriteEscape)
			escaped = true;
		else if (c == FavoriteSeparator) {
			if (!current.empty())
				names.push_back(current);
			current.clear();
		}
		else
			current.push_back(c);
	}

	if (escaped)
		current.push_back(FavoriteEscape);

	if (!current.empty())
		names.push_back(current);

	return names;
}

void BodySlideApp::SetFavoriteList(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::vector<std::string>& names) {
	list.clear();
	set.clear();

	for (const auto& name : names) {
		if (!name.empty() && set.insert(name).second)
			list.push_back(name);
	}
}

bool BodySlideApp::RemoveFavoriteName(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::string& name) {
	if (set.erase(name) == 0)
		return false;

	list.erase(std::remove(list.begin(), list.end(), name), list.end());
	return true;
}

void BodySlideApp::SortFavoritesFirst(std::vector<std::string>& names, const std::unordered_set<std::string>& favorites) const {
	std::sort(names.begin(), names.end(), case_insensitive_compare());
	std::stable_partition(names.begin(), names.end(), [&favorites](const std::string& name) { return favorites.find(name) != favorites.end(); });
}

void BodySlideApp::LoadFavorites() {
	SetFavoriteList(favoriteOutfits, favoriteOutfitSet, DeserializeFavoriteNames(BodySlideConfig[GetFavoriteConfigKey("Outfits")]));
	SetFavoriteList(favoritePresets, favoritePresetSet, DeserializeFavoriteNames(BodySlideConfig[GetFavoriteConfigKey("Presets")]));
}

void BodySlideApp::SaveFavorites() {
	BodySlideConfig.SetValue(GetFavoriteConfigKey("Outfits"), SerializeFavoriteNames(favoriteOutfits));
	BodySlideConfig.SetValue(GetFavoriteConfigKey("Presets"), SerializeFavoriteNames(favoritePresets));
}

bool BodySlideApp::IsFavoriteOutfit(const std::string& name) const {
	return favoriteOutfitSet.find(name) != favoriteOutfitSet.end();
}

bool BodySlideApp::IsFavoritePreset(const std::string& name) const {
	return favoritePresetSet.find(name) != favoritePresetSet.end();
}

void BodySlideApp::SortOutfitNamesForDisplay(std::vector<std::string>& names) const {
	SortFavoritesFirst(names, favoriteOutfitSet);
}

void BodySlideApp::SortPresetNamesForDisplay(std::vector<std::string>& names) const {
	SortFavoritesFirst(names, favoritePresetSet);
}

void BodySlideApp::ToggleFavoriteOutfit(const std::string& name) {
	if (name.empty() || !OutfitExists(name))
		return;

	if (!RemoveFavoriteName(favoriteOutfits, favoriteOutfitSet, name)) {
		favoriteOutfitSet.insert(name);
		favoriteOutfits.push_back(name);
	}

	SortOutfitNamesForDisplay(filteredOutfits);

	SaveFavorites();
}

void BodySlideApp::ToggleFavoritePreset(const std::string& name) {
	if (name.empty() || !PresetExists(name))
		return;

	if (!RemoveFavoriteName(favoritePresets, favoritePresetSet, name)) {
		favoritePresetSet.insert(name);
		favoritePresets.push_back(name);
	}

	SaveFavorites();
}

void BodySlideApp::RemoveFavoriteOutfit(const std::string& name) {
	if (RemoveFavoriteName(favoriteOutfits, favoriteOutfitSet, name))
		SaveFavorites();
}

void BodySlideApp::RemoveFavoritePreset(const std::string& name) {
	if (RemoveFavoriteName(favoritePresets, favoritePresetSet, name))
		SaveFavorites();
}

void BodySlideApp::RefreshOutfitList() {
	LoadSliderSets();
	PopulateOutfitList("");
}

int BodySlideApp::LoadSliderSets() {
	wxLogMessage("Loading all slider sets...");
	outfitNameSource.clear();
	outfitNameOrder.clear();
	outfitHasZaps.clear();
	outFileCount.clear();

	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.osp");
	wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.xml");

	bool filterHasZaps = false;

	auto menuOutfitSrchContext = sliderView->outfitsearch->GetMenu();
	if (menuOutfitSrchContext) {
		auto menuFilterHasZaps = menuOutfitSrchContext->FindItem(XRCID("menuFilterHasZaps"));
		if (menuFilterHasZaps)
			filterHasZaps = menuFilterHasZaps->IsChecked();
	}

	for (auto& file : files) {
		std::string fileName{file.ToUTF8()};

		SliderSetFile sliderDoc;
		sliderDoc.Open(fileName);
		if (sliderDoc.fail())
			continue;

		std::string outFilePath;
		std::vector<std::string> outfitNames;
		sliderDoc.GetSetNamesUnsorted(outfitNames, false);
		for (auto& o : outfitNames) {
			if (outfitNameSource.find(o) != outfitNameSource.end())
				continue;

			outfitNameSource[o] = fileName;
			outfitNameOrder.push_back(o);

			if (filterHasZaps) {
				bool hasZaps = false;

				SliderSet sliderSet;
				if (sliderDoc.GetSet(o, sliderSet) == 0) {
					for (size_t s = 0; s < sliderSet.size(); s++) {
						auto& slider = sliderSet[s];
						if (slider.bZap && !slider.bHidden) {
							hasZaps = true;
							break;
						}
					}
				}

				if (hasZaps)
					outfitHasZaps.push_back(o);
			}

			sliderDoc.GetSetOutputFilePath(o, outFilePath);
			if (!outFilePath.empty())
				outFileCount[outFilePath].push_back(o);
		}
	}

	ungroupedOutfits.clear();
	for (auto& o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	bool favoritesChanged = false;
	for (auto it = favoriteOutfits.begin(); it != favoriteOutfits.end();) {
		if (OutfitExists(*it))
			++it;
		else {
			favoriteOutfitSet.erase(*it);
			it = favoriteOutfits.erase(it);
			favoritesChanged = true;
		}
	}

	if (favoritesChanged)
		SaveFavorites();

	return 0;
}

void BodySlideApp::ActivateOutfit(const std::string& outfitName) {
	wxLogMessage("Activating set '%s'...", outfitName);

	BodySlideConfig.SetValue("SelectedOutfit", outfitName);
	sliderView->Freeze();

	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	CleanupPreview();

	std::string activePreset = BodySlideConfig["SelectedPreset"];

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(activePreset);

	int error = CreateSetSliders(outfitName);
	if (error)
		wxLogError("Failed to load set '%s' from slider set list (%d).", outfitName, error);

	PopulateOutfitList(outfitName);

	ActivatePreset(activePreset, false);

	InitPreview();

	sliderView->Layout();
	sliderView->Refresh();
	sliderView->Thaw();
	wxLogMessage("Finished activating set '%s'.", outfitName);
}

void BodySlideApp::ActivatePreset(const std::string& presetName, const bool updatePreview) {
	wxLogMessage("Applying preset '%s' to sliders.", presetName);

	BodySlideConfig.SetValue("SelectedPreset", presetName);
	sliderManager.InitializeSliders(presetName);

	bool zapChanged = false;
	Slider* sliderSmall = nullptr;
	Slider* sliderBig = nullptr;
	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		sliderSmall = &sliderManager.slidersSmall[i];
		sliderBig = &sliderManager.slidersBig[i];

		if (sliderBig->zap && !sliderBig->uv && !zapChanged) {
			auto* sd = sliderView->GetSliderDisplay(sliderBig->name);
			if (sd) {
				float zapValueUI = sd->zapCheckHi->IsChecked() ? 0.01f : 0.0f;
				if (sliderBig->value != zapValueUI)
					zapChanged = true;
			}
		}

		sliderView->SetSliderPosition(sliderSmall->name.c_str(), sliderSmall->value, SLIDER_LO);
		sliderView->SetSliderPosition(sliderBig->name.c_str(), sliderBig->value, SLIDER_HI);
	}

	sliderView->SetPresetChanged(false);

	if (UpdateZapChoices())
		zapChanged = true;

	if (preview && updatePreview)
		zapChanged ? RebuildPreviewMeshes() : UpdatePreview();
}

void BodySlideApp::DeleteOutfit(const std::string& outfitName) {
	auto outfit = outfitNameSource.find(outfitName);
	if (outfit == outfitNameSource.end())
		return;

	int select = wxNOT_FOUND;
	if (sliderView->outfitChoice)
		select = sliderView->outfitChoice->GetSelection();

	wxLogMessage("Loading project file '%s'...", outfit->second);

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfit->second);
	if (!sliderDoc.fail()) {
		wxLogMessage("Deleting project '%s'...", outfitName);

		if (!sliderDoc.DeleteSet(outfit->first)) {
			if (sliderDoc.Save()) {
				RemoveFavoriteOutfit(outfitName);
				RefreshOutfitList();

				if (sliderView->outfitChoice) {
					int count = sliderView->outfitChoice->GetCount();
					if (count > select)
						sliderView->outfitChoice->Select(select);
					else if (count > select - 1)
						sliderView->outfitChoice->Select(select - 1);

					std::string selectedOutfit = sliderView->GetSelectedOutfitName();
					if (!selectedOutfit.empty())
						ActivateOutfit(selectedOutfit);
				}
			}
			else
				wxLogMessage("Failed to delete the slider set!");
		}
		else
			wxLogMessage("Failed to delete the slider set!");
	}
	else
		wxLogMessage("Failed to load the project file!");
}

void BodySlideApp::DeletePreset(const std::string& presetName) {
	std::string outputFile = sliderManager.GetPresetFileNames(presetName);
	if (outputFile.empty())
		return;

	int select = wxNOT_FOUND;
	if (sliderView->presetChoice)
		select = sliderView->presetChoice->GetSelection();

	wxLogMessage("Deleting preset '%s'...", presetName);
	if (!sliderManager.DeletePreset(outputFile, presetName)) {
		RemoveFavoritePreset(presetName);
		LoadPresets("");
		PopulatePresetList(presetName);

		if (sliderView->presetChoice) {
			int count = sliderView->presetChoice->GetCount();
			if (count > select)
				sliderView->presetChoice->Select(select);
			else if (count > select - 1)
				sliderView->presetChoice->Select(select - 1);

			ActivatePreset(sliderView->GetSelectedPresetName());
		}
	}
	else
		wxLogMessage("Failed to delete preset!");
}

void BodySlideApp::RefreshSliders() {
	Slider* slider = nullptr;
	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		slider = &sliderManager.slidersBig[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_HI);

		slider = &sliderManager.slidersSmall[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_LO);
	}

	if (preview)
		UpdatePreview();
}

void BodySlideApp::PopulatePresetList(const std::string& select) {
	std::string myselect = BodySlideConfig["SelectedPreset"];
	if (!select.empty())
		myselect = select;

	std::vector<std::string> presets;

	wxArrayString items;
	sliderManager.GetPresetNames(presets);

	std::vector<std::string> filteredPresets = ApplyPresetFilter(presets);
	items.reserve(filteredPresets.size());
	for (size_t i = 0; i < filteredPresets.size(); i++)
		items.Add(wxString::FromUTF8(filteredPresets[i]));

	sliderView->PopulatePresetList(items, wxString::FromUTF8(myselect));
}

void BodySlideApp::PopulateOutfitList(const std::string& select) {
	std::string myselect = BodySlideConfig["SelectedOutfit"];
	if (!select.empty())
		myselect = select;

	wxArrayString items;

	size_t n = outfitNameSource.size();
	if (n == 0)
		return;

	ApplyOutfitFilter();

	items.reserve(filteredOutfits.size());
	for (auto& fo : filteredOutfits)
		items.Add(wxString::FromUTF8(fo));

	sliderView->PopulateOutfitList(items, wxString::FromUTF8(myselect));
}

void BodySlideApp::DisplayActiveSet() {
	if (projects.empty())
		return;

	auto& activeSet = GetActiveSet();
	if (activeSet.GenWeights())
		sliderView->ShowLowColumn(true);
	else
		sliderView->ShowLowColumn(false);

	// Category name, slider names, hidden flag
	std::vector<std::tuple<std::string, std::vector<std::string>, bool>> sliderCategories;
	std::vector<std::string> cats;
	cCollection.GetAllCategories(cats);

	// Populate category data
	for (auto& cat : cats) {
		std::vector<std::string> catSliders;
		cCollection.GetCategorySliders(cat, catSliders);

		if (catSliders.size() > 0) {
			bool catHidden = cCollection.GetCategoryHidden(cat);
			sliderCategories.push_back(make_tuple(cat, catSliders, !catHidden));
		}
		else
			continue;
	}

	// Loop slider set
	std::vector<std::vector<int>> catSliders;
	for (size_t i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].bHidden)
			continue;

		// Find the category in the list
		bool regularSlider = true;
		int iter = 0;
		for (auto& cat : sliderCategories) {
			catSliders.push_back(std::vector<int>());
			if (std::find(std::get<1>(cat).begin(), std::get<1>(cat).end(), activeSet[i].name) != std::get<1>(cat).end()) {
				catSliders[iter].push_back(i);
				regularSlider = false;
				break;
			}
			iter++;
		}

		// Not in a category
		if (regularSlider)
			sliderView->AddSliderGUI(activeSet[i].name, activeSet[i].name, "", activeSet[i].bZap, !activeSet.GenWeights());
	}

	// Create category UI
	size_t iter = 0;
	if (catSliders.size() > 0) {
		for (auto& cat : sliderCategories) {
			std::string categoryName = std::get<0>(cat);
			std::vector<std::string> sliderNames = std::get<1>(cat);
			bool show = std::get<2>(cat);
			std::string displayName;

			if (catSliders.size() > iter && catSliders[iter].size() > 0) {
				sliderView->AddCategorySliderUI(categoryName, sliderNames, show, !activeSet.GenWeights());

				for (auto& s : catSliders[iter]) {
					displayName = cCollection.GetSliderDisplayName(categoryName, activeSet[s].name);
					if (displayName.empty())
						displayName = activeSet[s].name;

					sliderView->AddSliderGUI(activeSet[s].name, displayName, categoryName, activeSet[s].bZap, !activeSet.GenWeights());
				}
			}
			iter++;
		}
	}

	sliderView->DoFilterSliders();
	UpdateConflictManager();
}

void BodySlideApp::GetBuildSelection(BuildSelectionFile& file, BuildSelection& buildSel) {
	const std::string buildSelFileName = Config["AppDir"] + PathSepStr + "BuildSelection.xml";

	file.Open(buildSelFileName);

	if (file.GetError())
		file.New(buildSelFileName);

	file.Get(buildSel);
}

void BodySlideApp::UpdateConflictManager() {
	if (projects.empty())
		return;

	// Populate Conflict UI
	auto& activeSet = GetActiveSet();
	auto conflictCheckBox = (wxCheckBox*)sliderView->FindWindowByName("cbIsOutfitChoice");
	auto conflictLabel = (wxStaticText*)sliderView->FindWindowByName("conflictLabel");
	auto conflictInfo = (wxStaticText*)sliderView->FindWindowByName("conflictInfo");

	auto outputFilePath = activeSet.GetOutputFilePath();
	auto& col = outFileCount[outputFilePath];

	bool isOutputChoice = true;
	std::string textColourName = "#C8C8C8";

	if (1 < col.size()) {
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		std::string outputChoice = buildSelection.GetOutputChoice(outputFilePath);
		isOutputChoice = outputChoice == activeSet.GetName();
		textColourName = isOutputChoice ? "#00FFFF" : "#FFD769";
	}

	conflictCheckBox->SetValue(isOutputChoice);
	conflictCheckBox->Show();

	if (activeSet.GenWeights())
		conflictLabel->SetLabel(wxString::FromUTF8(outputFilePath) + "_0.nif (and _1.nif)");
	else
		conflictLabel->SetLabel(wxString::FromUTF8(outputFilePath) + ".nif");

	conflictLabel->SetForegroundColour(wxColour(textColourName));
	conflictLabel->Show();

	if (1 < col.size())
		conflictInfo->Show();
	else
		conflictInfo->Hide();

	if (sliderView->leftPanel)
		sliderView->leftPanel->Layout();
}

void BodySlideApp::SetDefaultBuildSelection() {
	if (projects.empty())
		return;

	auto& activeSet = GetActiveSet();
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	auto outputFilePath = activeSet.GetOutputFilePath();
	std::string choiceName = activeSet.GetName();

	bool willSet = 0 != choiceName.compare(buildSelection.GetOutputChoice(outputFilePath));
	if (willSet) {
		buildSelection.SetOutputChoice(outputFilePath, activeSet.GetName());
		buildSelFile.UpdateOutputChoices(buildSelection);
	}
	else {
		buildSelection.SetOutputChoice(outputFilePath, "");
		buildSelFile.RemoveOutputChoice(outputFilePath);
	}

	buildSelFile.Save();

	UpdateConflictManager();
	sliderView->Refresh();
}

bool BodySlideApp::UpdateZapChoices() {
	if (projects.empty())
		return false;

	auto& activeSet = GetActiveSet();
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	bool zapChanged = false;

	for (size_t s = 0; s < activeSet.size(); s++) {
		if (!activeSet[s].bZap || activeSet[s].bHidden)
			continue;

		std::string project = activeSet.GetName();
		std::string zap = activeSet[s].name;
		if (buildSelection.HasZapChoice(project, zap)) {
			bool zapChoice = buildSelection.GetZapChoice(project, zap);

			SliderDisplay* sd = sliderView->GetSliderDisplay(zap);
			if (sd && sd->isZap) {
				if (sd->zapCheckHi->IsChecked() != zapChoice) {
					sd->zapCheckHi->SetValue(zapChoice);

					// Trigger checkbox event
					wxEvtHandler* handler = sd->zapCheckHi->GetEventHandler();
					wxCommandEvent event(wxEVT_COMMAND_CHECKBOX_CLICKED, sd->zapCheckHi->GetId());
					event.SetEventObject(sd->zapCheckHi);
					event.SetInt(zapChoice ? 1 : 0);
					handler->ProcessEvent(event);

					zapChanged = true;
				}
			}
		}
	}

	return zapChanged;
}

void BodySlideApp::SetZapChoice(const std::string& zap, bool choice) {
	if (projects.empty())
		return;

	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	std::string project = GetActiveSet().GetName();

	buildSelection.SetZapChoice(project, zap, choice);
	buildSelFile.UpdateZapChoices(buildSelection);

	buildSelFile.Save();
}

void BodySlideApp::EditProject(const std::string& projectName) {
	auto project = outfitNameSource.find(projectName);
	if (project == outfitNameSource.end())
		return;

	wxLogMessage("Launching Outfit Studio with project file '%s' and project '%s'...", project->second, project->first);
	LaunchOutfitStudio(wxString::Format("-proj \"%s\" \"%s\"", wxString::FromUTF8(project->first), wxString::FromUTF8(project->second)));
}

void BodySlideApp::LaunchOutfitStudio(const wxString& args) {
#ifdef _WIN32
	const wxString osExec = "OutfitStudio.exe";
#else
	const wxString osExec = "OutfitStudio";
#endif

	wxFileName osExecFile(wxString::FromUTF8(Config["AppDir"]), osExec);
	wxString osExecCmd = wxString::Format("\"%s\" %s", osExecFile.GetFullPath(), args);

	if (!wxExecute(osExecCmd, wxEXEC_ASYNC)) {
		wxLogError("Failed to execute '%s' process.", osExecCmd);
		wxMessageBox(_("Failed to launch Outfit Studio executable!"), _("Error"), wxICON_ERROR);
	}
}

void BodySlideApp::ApplySliders(
	const std::string& targetShape, std::vector<Slider>& sliderSet, DiffDataSets& dataSets, std::vector<Vector3>& verts, std::vector<uint16_t>& ZapIdx, std::vector<Vector2>* uvs) {
	for (auto& slider : sliderSet) {
		float val = slider.value;
		if (slider.zap && !slider.uv) {
			if (val > 0)
				for (size_t j = 0; j < slider.linkedDataSets.size(); j++)
					dataSets.GetDiffIndices(slider.linkedDataSets[j], targetShape, ZapIdx);
		}
		else {
			if (slider.invert)
				val = 1.0f - val;

			for (size_t j = 0; j < slider.linkedDataSets.size(); j++) {
				if (slider.uv) {
					if (uvs)
						dataSets.ApplyUVDiff(slider.linkedDataSets[j], targetShape, val, uvs);
				}
				else
					dataSets.ApplyDiff(slider.linkedDataSets[j], targetShape, val, &verts);
			}
		}
	}

	for (auto& slider : sliderSet)
		if (slider.clamp && slider.value > 0)
			for (size_t j = 0; j < slider.linkedDataSets.size(); j++)
				dataSets.ApplyClamp(slider.linkedDataSets[j], targetShape, &verts);
}

struct ContinuousRange {
	uint16_t index = 0;
	size_t length = 0;
};

static std::vector<ContinuousRange> FindContinuousRanges(const std::vector<uint16_t>& source) {
	std::vector<ContinuousRange> ranges;
	if (source.size() == 0) {
		return ranges;
	}

	size_t startIndex = 0;
	size_t endIndex = 1;
	int lastValue = (int)source[0]; // Cast to int to avoid potential uint16_t overflow in comparison below

	for (; endIndex < source.size(); ++endIndex) {
		int value = (int)source[endIndex];
		if (value != lastValue + 1) {
			ranges.emplace_back(ContinuousRange{ source[startIndex], endIndex - startIndex });
			startIndex = endIndex;
		}
		lastValue = value;
	}

	ranges.emplace_back(ContinuousRange{ source[startIndex], endIndex - startIndex });

	return ranges;
}

bool BodySlideApp::WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices) {
	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	std::string triFilePath = triPath + ".tri";

	for (auto targetShape = sliderSet.ShapesBegin(); targetShape != sliderSet.ShapesEnd(); ++targetShape) {
		auto shape = nif.FindBlockByName<NiShape>(targetShape->first);
		if (!shape)
			continue;

		const std::vector<uint16_t>& shapeZapIndices = zapIndices[targetShape->first];

		int shapeVertCount = shape->GetNumVertices();
		shapeVertCount += shapeZapIndices.size();

		if (shapeVertCount <= 0)
			continue;

		if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= shapeVertCount)
			continue;

		auto zapRanges = FindContinuousRanges(shapeZapIndices);

		for (size_t s = 0; s < sliderSet.size(); s++) {
			std::string dn = sliderSet[s].TargetDataName(targetShape->second.targetShape);
			std::string target = targetShape->second.targetShape;
			if (dn.empty())
				continue;

			if (!sliderSet[s].bClamp && !sliderSet[s].bZap) {
				MorphDataPtr morph = std::make_shared<MorphData>();
				morph->name = sliderSet[s].name;

				if (sliderSet[s].bUV) {
					morph->type = MORPHTYPE_UV;

					std::vector<Vector2> uvs;
					uvs.resize(shapeVertCount);

					currentDiffs.ApplyUVDiff(dn, target, 1.0f, &uvs);

					for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
						const auto start = uvs.cbegin() + range->index;
						uvs.erase(start, start + range->length);
					}

					int i = 0;
					for (auto& uv : uvs) {
						Vector3 v(uv.u, uv.v, 0.0f);
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}
				else {
					morph->type = MORPHTYPE_POSITION;

					std::vector<Vector3> verts;
					verts.resize(shapeVertCount);

					currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);

					for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
						const auto start = verts.cbegin() + range->index;
						verts.erase(start, start + range->length);
					}

					int i = 0;
					for (auto& v : verts) {
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(targetShape->first, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

bool BodySlideApp::WriteSFMorphFile(const std::string& morphFolder, SliderSet& sliderSet, NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices) {
	std::string targetShapeName = sliderSet.GetSFMorphTargetShape();
	if (targetShapeName.empty()) {
		wxLogMessage("No morph target shape designated, skipping morph.dat.");
		return false;
	}

	wxLogMessage("Writing Starfield morph.dat for shape '%s' to '%s'...", targetShapeName, morphFolder);

	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	// Find the designated shape in the slider set
	std::string targetDataShape;
	for (auto it = sliderSet.ShapesBegin(); it != sliderSet.ShapesEnd(); ++it) {
		if (it->first == targetShapeName) {
			targetDataShape = it->second.targetShape;
			break;
		}
	}

	if (targetDataShape.empty()) {
		wxLogMessage("Morph target shape '%s' not found in slider set, skipping morph.dat.", targetShapeName);
		return false;
	}

	auto shape = nif.FindBlockByName<NiShape>(targetShapeName);
	if (!shape) {
		wxLogMessage("Shape '%s' not found in NIF, skipping morph.dat.", targetShapeName);
		return false;
	}

	const std::vector<uint16_t>& shapeZapIndices = zapIndices[targetShapeName];

	int shapeVertCount = shape->GetNumVertices();
	shapeVertCount += shapeZapIndices.size();

	if (shapeVertCount <= 0)
		return false;

	if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= shapeVertCount)
		return false;

	auto zapRanges = FindContinuousRanges(shapeZapIndices);

	std::vector<Vector3> baseVerts;
	std::vector<Vector2> baseUVs;
	std::vector<Triangle> baseTris;
	nif.GetVertsForShape(shape, baseVerts);
	nif.GetUvsForShape(shape, baseUVs);
	shape->GetTriangles(baseTris);

	SFMorphFile morphFile;
	morphFile.SetVertexCount(shapeVertCount - static_cast<int>(shapeZapIndices.size()));

	for (size_t s = 0; s < sliderSet.size(); s++) {
		std::string dn = sliderSet[s].TargetDataName(targetDataShape);
		if (dn.empty())
			continue;

		if (sliderSet[s].bClamp || sliderSet[s].bZap || sliderSet[s].bUV)
			continue;

		std::vector<Vector3> diffs;
		diffs.resize(shapeVertCount);

		currentDiffs.ApplyDiff(dn, targetDataShape, 1.0f, &diffs);

		for (auto range = zapRanges.rbegin(); range != zapRanges.rend(); ++range) {
			const auto start = diffs.cbegin() + range->index;
			diffs.erase(start, start + range->length);
		}

		std::unordered_map<uint16_t, Vector3> morphOffsets;
		int i = 0;
		for (auto& d : diffs) {
			if (!d.IsZero(true))
				morphOffsets.emplace(i, d);
			i++;
		}

		if (morphOffsets.empty())
			continue;

		std::unordered_map<uint16_t, Vector3> morphNormals;
		std::unordered_map<uint16_t, Vector3> morphTangents;

		std::vector<Vector3> morphedVerts = baseVerts;

		if (morphedVerts.size() == diffs.size()) {
			for (size_t v = 0; v < morphedVerts.size(); v++)
				morphedVerts[v] += diffs[v];
		}

		int nVerts = static_cast<int>(morphedVerts.size());
		int nTris = static_cast<int>(baseTris.size());

		if (nVerts > 0 && nTris > 0) {
			Mesh tmpMesh{};
			tmpMesh.nVerts = nVerts;
			tmpMesh.nTris = nTris;
			tmpMesh.verts = std::make_unique<Vector3[]>(nVerts);
			tmpMesh.norms = std::make_unique<Vector3[]>(nVerts);
			tmpMesh.tangents = std::make_unique<Vector3[]>(nVerts);
			tmpMesh.bitangents = std::make_unique<Vector3[]>(nVerts);
			tmpMesh.texcoord = std::make_unique<Vector2[]>(nVerts);

			if (nTris > 0)
				tmpMesh.tris = std::make_unique<Triangle[]>(nTris);

			for (int v = 0; v < nVerts; v++)
				tmpMesh.verts[v] = Mesh::TransformPosNifToMesh(morphedVerts[v]);

			if (!baseUVs.empty()) {
				for (int v = 0; v < nVerts && v < static_cast<int>(baseUVs.size()); v++) {
					tmpMesh.texcoord[v].u = baseUVs[v].u;
					tmpMesh.texcoord[v].v = baseUVs[v].v;
				}
			}

			for (int t = 0; t < nTris; t++)
				tmpMesh.tris[t] = baseTris[t];

			tmpMesh.SmoothNormals();

			for (const auto& morphOffset : morphOffsets) {
				const int v = morphOffset.first;
				if (v >= 0 && v < nVerts) {
					morphNormals[v] = Mesh::TransformDirMeshToNif(tmpMesh.norms[v]);
					morphTangents[v] = Mesh::TransformDirMeshToNif(tmpMesh.tangents[v]);
				}
			}
		}

		if (morphFile.morphOffsetsCache.size() >= SFMaxShapeKeys) {
			wxLogWarning("Starfield morph.dat supports at most 128 morphs; '%s' and any remaining morphs were skipped.", sliderSet[s].name);
			break;
		}

		morphFile.AddMorph(sliderSet[s].name, morphOffsets, {}, morphNormals, morphTangents);
	}

	if (morphFile.morphOffsetsCache.empty()) {
		wxLogMessage("No morphs found for shape '%s', skipping morph.dat.", targetShapeName);
		return false;
	}

	wxLogMessage("Writing %zu morph(s) for shape '%s'...", morphFile.morphOffsetsCache.size(), targetShapeName);

	wxFileName::Mkdir(wxString::FromUTF8(morphFolder), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	std::string shapeFilePath = morphFolder + PathSepStr + "morph.dat";

	morphFile.CacheToFileData();

	if (!morphFile.Write(shapeFilePath)) {
		wxLogError("Failed to write morph.dat file to '%s'!", shapeFilePath);
		wxMessageBox(wxString().Format(_("Failed to write morph.dat file to the following location\n\n%s"), shapeFilePath),
					 _("Unable to process"), wxOK | wxICON_ERROR);
		return false;
	}

	wxLogMessage("Successfully wrote morph.dat to '%s'.", shapeFilePath);
	return true;
}

void BodySlideApp::CopySliderValues(bool toHigh) {
	wxLogMessage("Copying slider values to %s weight.", toHigh ? "high" : "low");

	if (toHigh) {
		for (size_t i = 0; i < sliderManager.slidersSmall.size(); i++) {
			Slider* slider = &sliderManager.slidersSmall[i];
			if (slider->zap || slider->clamp)
				continue;

			if (sliderView->GetSliderDisplay(slider->name)) {
				sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_HI);
				SetSliderValue(slider->name, false, slider->value);
				SetSliderChanged(slider->name, false);
			}
		}
	}
	else {
		for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
			Slider* slider = &sliderManager.slidersBig[i];
			if (slider->zap || slider->clamp)
				continue;

			if (sliderView->GetSliderDisplay(slider->name)) {
				sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_LO);
				SetSliderValue(slider->name, true, slider->value);
				SetSliderChanged(slider->name, true);
			}
		}
	}

	sliderView->SetPresetChanged();

	if (preview)
		UpdatePreview();
}

void BodySlideApp::CopyPreviewWeightToSliders() {
	if (!preview || !sliderView)
		return;

	int weight = preview->GetWeight();
	wxLogMessage("Copying preview shape at weight %d to both high and low sliders.", weight);

	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		Slider* sliderBig = &sliderManager.slidersBig[i];
		Slider* sliderSmall = &sliderManager.slidersSmall[i];

		if (sliderBig->zap || sliderBig->clamp)
			continue;

		// Calculate the interpolated slider value at the current weight
		float effectiveValue = (sliderBig->value * weight + sliderSmall->value * (100.0f - weight)) / 100.0f;

		if (sliderView->GetSliderDisplay(sliderBig->name)) {
			sliderView->SetSliderPosition(sliderBig->name.c_str(), effectiveValue, SLIDER_HI);
			SetSliderValue(sliderBig->name, false, effectiveValue);
			SetSliderChanged(sliderBig->name, false);

			sliderView->SetSliderPosition(sliderSmall->name.c_str(), effectiveValue, SLIDER_LO);
			SetSliderValue(sliderSmall->name, true, effectiveValue);
			SetSliderChanged(sliderSmall->name, true);
		}
	}

	sliderView->SetPresetChanged();
	UpdatePreview();
}

void BodySlideApp::ShowPreview() {
	// For standalone mode (cmd preview, or when no main frame exists)
	if (preview)
		return;

	int x = BodySlideConfig.GetIntValue("PreviewFrame.x");
	int y = BodySlideConfig.GetIntValue("PreviewFrame.y");
	int w = BodySlideConfig.GetIntValue("PreviewFrame.width");
	int h = BodySlideConfig.GetIntValue("PreviewFrame.height");
	std::string maximized = BodySlideConfig["PreviewFrame.maximized"];

	previewWindow = new PreviewWindow(wxPoint(x, y), wxSize(w, h), this);
	if (maximized == "true")
		previewWindow->Maximize();

	preview = previewWindow->GetPanel();

	// Set base data path for texture loading
	std::string baseGamePath = Config["GameDataPath"];
	preview->SetBaseDataPath(baseGamePath);
}

void BodySlideApp::InitPreviewPanel() {
	// Set up the embedded preview panel in the main frame
	if (sliderView && sliderView->previewPanel) {
		preview = sliderView->previewPanel;
		std::string baseGamePath = Config["GameDataPath"];
		preview->SetBaseDataPath(baseGamePath);
	}
}

void BodySlideApp::ClosePreview() {
	if (previewWindow) {
		previewWindow->Close();
		// PreviewClosed() will be called from OnClose
	}
}

void BodySlideApp::PreviewClosed() {
	// Called when the standalone preview window is closed
	previewWindow = nullptr;
	// If the main frame has an embedded panel, keep using it
	if (sliderView && sliderView->previewPanel) {
		preview = sliderView->previewPanel;
	}
	else {
		preview = nullptr;
	}
}

void BodySlideApp::PopOutPreview() {
	if (!sliderView || !sliderView->previewPanel || previewWindow)
		return;

	PreviewPanel* panel = sliderView->previewPanel;

	int x = BodySlideConfig.GetIntValue("PreviewFrame.x");
	int y = BodySlideConfig.GetIntValue("PreviewFrame.y");
	int w = BodySlideConfig.GetIntValue("PreviewFrame.width");
	int h = BodySlideConfig.GetIntValue("PreviewFrame.height");
	std::string maximized = BodySlideConfig["PreviewFrame.maximized"];

	// Unsplit: remove panel from splitter and shrink the main frame
	sliderView->UnsplitPreview();

	// Reparent into standalone window
	previewWindow = new PreviewWindow(wxPoint(x, y), wxSize(w, h), this, panel);
	if (maximized == "true")
		previewWindow->Maximize();

	preview = panel;
	bool previewAlwaysDetached = BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false);
	panel->SetPopoutButtonDetachedState(true);
	panel->ShowPopoutButton(!previewAlwaysDetached);

	sliderView->previewVisible = true;
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
	dockPoppedOutOnClose = false;
	sliderView->UpdatePreviewButtonLabel();
}

void BodySlideApp::DockPreview(bool attachToMain, bool preservePoppedOutState) {
	if (!previewWindow || !sliderView)
		return;

	PreviewPanel* panel = previewWindow->ReleasePanel();
	if (!panel)
		return;

	// Reparent back into splitter
	panel->Reparent(sliderView->splitter);
	sliderView->previewPanel = panel;
	preview = panel;

	if (attachToMain) {
		sliderView->SplitPreview(panel);
		sliderView->previewVisible = true;
		panel->SetPopoutButtonDetachedState(false);
		panel->ShowPopoutButton(true);
	}
	else {
		panel->Hide();
		sliderView->previewVisible = false;
		panel->SetPopoutButtonDetachedState(false);
		panel->ShowPopoutButton(true);
	}

	BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", sliderView->previewVisible);
	BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", !attachToMain && preservePoppedOutState);
	sliderView->UpdatePreviewButtonLabel();
	panel->Layout();

	previewWindow->Destroy();
	previewWindow = nullptr;
	dockPoppedOutOnClose = false;
}

bool BodySlideApp::IsPreviewRenderable() const {
	if (!preview)
		return false;

	if (previewWindow)
		return previewWindow->IsShownOnScreen() && preview->IsShownOnScreen();

	if (sliderView)
		return sliderView->previewVisible && preview->IsShownOnScreen();

	return preview->IsShownOnScreen();
}

void BodySlideApp::InitPreview() {
	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	// Cancel any in-flight load
	uint64_t gen = ++previewLoadGeneration;

	if (previewLoadThread.joinable())
		previewLoadThread.join();

	preview->ShowLoadingIndicator(true);
	previewLoading = true;

	// Capture per-project info for the background thread
	struct ProjectInfo {
		size_t index;
		std::string inputFileName;
		std::string setName;
		SliderSet sliderSetCopy;
		bool needsReload;
	};

	auto projectInfos = std::make_shared<std::vector<ProjectInfo>>();
	for (size_t i = 0; i < projects.size(); ++i) {
		auto& pp = projects[i];
		ProjectInfo info;
		info.index = i;
		info.inputFileName = pp->sliderSet.GetInputFileName();
		info.setName = pp->sliderSet.GetName();
		info.sliderSetCopy = pp->sliderSet;

		if (multiProjectMode) {
			// Multi-project always reloads
			info.needsReload = true;
		}
		else {
			// Single-project: skip if NIF hasn't changed
			info.needsReload = !pp->baseNif || pp->inputFileName != info.inputFileName || pp->setName != info.setName || sliderManager.NeedReload();
		}

		projectInfos->push_back(std::move(info));
	}

	auto extraNifPaths = multiProjectMode ? preview->GetExtraNifPaths() : std::vector<std::string>{};
	bool isMultiProject = multiProjectMode;
	PreviewPanel* targetPreview = preview;

	previewLoadThread = std::thread([this, gen, projectInfos, extraNifPaths, isMultiProject, targetPreview]() {
		struct ProjectResult {
			size_t index;
			nifly::NifFile* baseNif = nullptr;
			nifly::NifFile modNif;
			bool loaded = false;
		};

		std::vector<ProjectResult> results(projectInfos->size());

		for (size_t i = 0; i < projectInfos->size(); ++i) {
			if (previewLoadGeneration.load() != gen) {
				for (size_t j = 0; j < i; ++j)
					delete results[j].baseNif;
				return;
			}

			auto& info = (*projectInfos)[i];
			auto& result = results[i];
			result.index = info.index;

			if (!info.needsReload)
				continue;

			result.baseNif = new nifly::NifFile();
			std::fstream file;
			PlatformUtil::OpenFileStream(file, info.inputFileName, std::ios::in | std::ios::binary);
			if (result.baseNif->Load(file)) {
				delete result.baseNif;
				result.baseNif = nullptr;
				continue;
			}

			result.modNif.CopyFrom(*result.baseNif);

			DiffDataSets dataSets;
			info.sliderSetCopy.LoadSetDiffData(dataSets);

			// Build preview mesh (apply sliders, zap verts)
			bool keepZappedShapes = info.sliderSetCopy.KeepZappedShapes();
			std::vector<nifly::Vector3> verts;
			std::vector<nifly::Vector2> uvs;
			std::vector<uint16_t> zapIdx;
			for (auto it = info.sliderSetCopy.ShapesBegin(); it != info.sliderSetCopy.ShapesEnd(); ++it) {
				zapIdx.clear();
				auto shape = result.baseNif->FindBlockByName<nifly::NiShape>(it->first);
				if (!result.baseNif->GetVertsForShape(shape, verts))
					continue;

				result.baseNif->GetUvsForShape(shape, uvs);
				ApplySliders(it->second.targetShape, sliderManager.slidersBig, dataSets, verts, zapIdx, &uvs);

				shape = result.modNif.FindBlockByName<nifly::NiShape>(it->first);
				if (zapIdx.size() > 0) {
					result.modNif.SetVertsForShape(shape, verts);
					result.modNif.SetUvsForShape(shape, uvs);
					if (result.modNif.DeleteVertsForShape(shape, zapIdx) && !keepZappedShapes)
						result.modNif.DeleteShape(shape);
				}
				else {
					result.modNif.SetVertsForShape(shape, verts);
					result.modNif.SetUvsForShape(shape, uvs);
				}
			}
			result.loaded = true;
		}

		if (previewLoadGeneration.load() != gen) {
			for (auto& r : results)
				delete r.baseNif;
			return;
		}

		CallAfter([this, gen, targetPreview, results = std::move(results), projectInfos, extraNifPaths, isMultiProject]() mutable {
			if (previewLoadGeneration.load() != gen) {
				for (auto& r : results)
					delete r.baseNif;
				return;
			}

			// Verify the target panel is still the active preview.
			// Prevents stale callbacks from writing to the wrong panel
			// (e.g. conflicts preview closed while async load was in-flight).
			if (!IsPreviewRenderable() || !preview || preview != targetPreview) {
				for (auto& r : results)
					delete r.baseNif;
				previewLoading = false;
				return;
			}

			bool anyGenWeights = false;
			std::string baseGamePath = Config["GameDataPath"];
			preview->SetBaseDataPath(baseGamePath);

			for (size_t i = 0; i < results.size() && i < projects.size(); ++i) {
				auto& result = results[i];
				auto& pp = projects[result.index];
				auto& info = (*projectInfos)[i];

				if (result.loaded) {
					delete pp->baseNif;
					pp->baseNif = result.baseNif;
					result.baseNif = nullptr; // Ownership transferred
					pp->modNif = std::move(result.modNif);
					pp->inputFileName = info.inputFileName;
					pp->setName = info.setName;
				}

				if (pp->sliderSet.GenWeights())
					anyGenWeights = true;

				pp->sliderSet.LoadSetDiffData(pp->dataSets);
				preview->AddMeshFromNif(&pp->modNif);

				for (auto& s : pp->modNif.GetShapeNames())
					preview->AddNifShapeTextures(&pp->modNif, s);

				UpdateMeshesFromSet(pp->sliderSet);
			}

			if (!isMultiProject)
				sliderManager.FlagReload(false);

			preview->ShowWeight(anyGenWeights);
			if (sliderView && !preview->IsReadOnlyMode())
				preview->ShowLockShapeButton(anyGenWeights);

			if (isMultiProject) {
				preview->LoadNifFiles(extraNifPaths);
			}
			else if (!projects.empty()) {
				// Single-project extras: normals gen layers, reference shape
				auto* pp = projects[0].get();
				preview->SetNormalsGenerationLayers(pp->sliderSet.GetNormalsGenLayers());

				bool hasBuiltInRef = false;
				std::string refInfoShape = pp->sliderSet.GetReferenceShapeName();
				if (!refInfoShape.empty() && pp->baseNif && pp->baseNif->FindBlockByName<nifly::NiShape>(refInfoShape))
					hasBuiltInRef = true;
				if (!hasBuiltInRef && pp->baseNif && ClippingFixer::FindReferenceShape(*pp->baseNif))
					hasBuiltInRef = true;

				if (hasBuiltInRef) {
					preview->ShowReferenceCheckbox(false);
					referenceNif.reset();
				}
				else {
					if (!referenceNif)
						LoadExternalReference(pp->sliderSet);

					if (referenceNif) {
						bool hasClippingFix = clippingFixStrength > 0.0f;
						preview->ShowReferenceCheckbox(true);
						if (hasClippingFix)
							preview->SetReferenceCheckboxState(true, false);
						else
							preview->SetReferenceCheckboxState(false, true);

						preview->AddMeshFromNif(referenceNif.get(), const_cast<char*>(referenceShapeName.c_str()));
						preview->AddNifShapeTextures(referenceNif.get(), referenceShapeName);
						preview->SetMeshVisibility(referenceShapeName, preview->IsShowReferenceChecked());
					}
					else {
						preview->ShowReferenceCheckbox(false);
					}
				}

				if (!hasBuiltInRef && !referenceNif) {
					preview->ShowReferenceCheckbox(false);
					referenceNif.reset();
				}
			}

			previewLoading = false;
			UpdatePreview();
			UpdatePreviewPhysicsAvailability();
			preview->ShowLoadingIndicator(false);
		});
	});
}

void BodySlideApp::LoadPreviewNifs(const std::vector<std::string>& filePaths) {
	if (!preview)
		return;

	// Parse entries: split each by '?' to get file path and optional set names
	struct ParsedEntry {
		std::string filePath;
		std::vector<std::string> setNames;
		bool isOsp = false;
	};

	std::vector<ParsedEntry> entries;
	bool hasExplicitSets = false;

	for (auto& rawPath : filePaths) {
		ParsedEntry entry;

		// Split by '?'
		size_t qPos = rawPath.find('?');
		if (qPos != std::string::npos) {
			entry.filePath = rawPath.substr(0, qPos);
			std::string remainder = rawPath.substr(qPos + 1);
			// Split set names by '?'
			size_t pos = 0;
			while (pos < remainder.size()) {
				size_t nextQ = remainder.find('?', pos);
				std::string setName;
				if (nextQ != std::string::npos) {
					setName = remainder.substr(pos, nextQ - pos);
					pos = nextQ + 1;
				}
				else {
					setName = remainder.substr(pos);
					pos = remainder.size();
				}
				if (!setName.empty())
					entry.setNames.push_back(setName);
			}
			hasExplicitSets = true;
		}
		else {
			entry.filePath = rawPath;
		}

		// Detect OSP extension
		size_t dotPos = entry.filePath.find_last_of('.');
		if (dotPos != std::string::npos) {
			std::string ext = ToLower(entry.filePath.substr(dotPos + 1));
			entry.isOsp = (ext == "osp");
		}

		entries.push_back(entry);
	}

	// Collect non-OSP file paths for extra NIF loading alongside projects
	std::vector<std::string> nifPaths;
	for (auto& e : entries) {
		if (!e.isOsp)
			nifPaths.push_back(e.filePath);
	}

	// Multi-project mode: explicit set names specified with '?'
	if (hasExplicitSets) {
		std::vector<PreviewProjectEntry> projEntries;

		for (auto& e : entries) {
			if (e.isOsp) {
				if (!e.setNames.empty()) {
					for (auto& setName : e.setNames)
						projEntries.push_back({e.filePath, setName});
				}
				else {
					// OSP without specific sets: ignore when other files have explicit sets
					wxLogWarning("Ignoring OSP file without specified set names: %s", e.filePath);
				}
			}
		}

		if (!projEntries.empty()) {
			wxLogMessage("Loading %zu combined project(s) in preview mode...", projEntries.size());
			if (!nifPaths.empty())
				preview->SetExtraNifPaths(nifPaths);
			preview->SetProjectData(projEntries, true, cmdPreset);
			return;
		}
	}

	// OSP files without explicit sets: show dropdown with all sets from all OSP files
	{
		std::vector<PreviewProjectEntry> projEntries;
		bool anyOsp = false;

		for (auto& e : entries) {
			if (!e.isOsp)
				continue;

			anyOsp = true;
			SliderSetFile sliderDoc;
			sliderDoc.Open(e.filePath);
			if (sliderDoc.fail()) {
				wxLogError("Failed to load BodySlide project file: %s", e.filePath);
				continue;
			}

			std::vector<std::string> setNames;
			sliderDoc.GetSetNamesUnsorted(setNames, false);
			for (auto& setName : setNames)
				projEntries.push_back({e.filePath, setName});
		}

		if (!projEntries.empty()) {
			wxLogMessage("Loading %zu slider set(s) from %zu OSP file(s)", projEntries.size(), entries.size());
			if (!nifPaths.empty())
				preview->SetExtraNifPaths(nifPaths);
			preview->SetProjectData(projEntries, false, cmdPreset);
			return;
		}

		if (anyOsp)
			return;
	}

	if (!cmdPreset.empty())
		wxLogWarning("Preset '%s' from the command line is ignored, presets only apply to project files.", cmdPreset);

	// Load as regular NIF files
	std::vector<std::string> paths;
	for (auto& e : entries)
		paths.push_back(e.filePath);
	preview->SetExtraNifPaths(paths);
}

void BodySlideApp::BuildPreviewMesh(ProjectData* pp, bool freshLoad) {
	bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<uint16_t> zapIdx;
	for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
		zapIdx.clear();

		auto shape = pp->baseNif->FindBlockByName<NiShape>(it->first);
		if (!pp->baseNif->GetVertsForShape(shape, verts))
			continue;

		pp->baseNif->GetUvsForShape(shape, uvs);

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, pp->dataSets, verts, zapIdx, &uvs);

		// Zap deleted verts before preview
		shape = pp->modNif.FindBlockByName<NiShape>(it->first);
		if (freshLoad && zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
			if (pp->modNif.DeleteVertsForShape(shape, zapIdx) && !keepZappedShapes)
				pp->modNif.DeleteShape(shape);
		}
		else if (zapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;
				verts.erase(verts.begin() + zapIdx[z]);
				uvs.erase(uvs.begin() + zapIdx[z]);
			}
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
		}
		else {
			// No zapping needed - just show all the verts.
			pp->modNif.SetVertsForShape(shape, verts);
			pp->modNif.SetUvsForShape(shape, uvs);
		}
	}
}

void BodySlideApp::ApplyClippingFix(NifFile& nif,
									const std::vector<Vector3>& bodyVerts,
									const std::vector<Triangle>& bodyTris,
									std::unordered_map<std::string, std::vector<Vector3>*>& shapeVerts) {
	ClippingFixOptions fixOpts;
	fixOpts.strength = clippingFixStrength / 100.0f;

	for (auto& [shapeName, vertsPtr] : shapeVerts) {
		if (!vertsPtr || vertsPtr->empty())
			continue;

		auto shape = nif.FindBlockByName<NiShape>(shapeName);
		if (!ClippingFixer::IsEligibleForFix(nif, shape))
			continue;

		std::vector<Triangle> outfitTris;
		shape->GetTriangles(outfitTris);
		if (outfitTris.empty())
			continue;

		ClippingFixer::FixClipping(bodyVerts, bodyTris, *vertsPtr, outfitTris, fixOpts);
	}
}

bool BodySlideApp::LoadExternalReference(const SliderSet& sliderSet) {
	if (!sliderSet.HasExternalReferenceInfo())
		return false;

	std::string projectFile = sliderSet.GetReferenceProjectFile();
	std::string projectName = sliderSet.GetReferenceProjectName();
	referenceShapeName = sliderSet.GetReferenceShapeName();

	// Resolve project file path relative to project directory
	wxFileName refProjectFileName(wxString::FromUTF8(projectFile));
	if (refProjectFileName.IsRelative())
		refProjectFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	std::string projectFilePath = refProjectFileName.GetFullPath().ToUTF8().data();

	// Load the slider set from the referenced project file
	SliderSetFile refSSF(projectFilePath);
	if (refSSF.fail()) {
		wxLogWarning("Could not load reference project file: %s", projectFilePath);
		return false;
	}

	referenceSliderSet.Clear();
	if (refSSF.GetSet(projectName, referenceSliderSet)) {
		wxLogWarning("Could not find reference project '%s' in file: %s", projectName, projectFilePath);
		return false;
	}

	referenceSliderSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

	// Load diff data for the reference shape
	referenceDiffData.Clear();
	referenceSliderSet.LoadSetDiffData(referenceDiffData, referenceShapeName);

	// Load the NIF file from the referenced project
	std::string refInputFile = referenceSliderSet.GetInputFileName();
	std::fstream file;
	PlatformUtil::OpenFileStream(file, refInputFile, std::ios::in | std::ios::binary);

	referenceNif = std::make_unique<NifFile>();
	if (referenceNif->Load(file)) {
		wxLogWarning("Could not load reference NIF file: %s", refInputFile);
		referenceNif.reset();
		return false;
	}

	// Verify the shape exists
	auto refShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
	if (!refShape) {
		wxLogWarning("Reference shape '%s' not found in NIF: %s", referenceShapeName, refInputFile);
		referenceNif.reset();
		return false;
	}

	return true;
}

void BodySlideApp::UpdateReferenceCheckboxState() {
	if (!preview || !referenceNif || multiProjectMode)
		return;

	bool hasClippingFix = clippingFixStrength > 0.0f;
	if (hasClippingFix) {
		preview->SetReferenceCheckboxState(true, false);
	}
	else {
		preview->SetReferenceCheckboxState(false, true);
	}
}

void BodySlideApp::UpdatePreview() {
	if (previewLoading)
		return;

	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	int weight = preview->GetWeight();
	auto shapeData = ComputeMorphedShapeData(weight);
	PostProcessPreview(shapeData, weight);
}

std::vector<ShapePreviewData> BodySlideApp::ComputeMorphedShapeData(int weight) {
	std::vector<ShapePreviewData> shapeData;
	std::vector<Vector3> verts, vertsLow, vertsHigh;
	std::vector<Vector2> uvs, uvsLow, uvsHigh;
	std::vector<uint16_t> zapIdx;

	for (int pi = 0; pi < static_cast<int>(projects.size()); pi++) {
		auto& pp = projects[pi];
		if (!pp->baseNif)
			continue;

		for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
			zapIdx.clear();

			auto shape = pp->baseNif->FindBlockByName<NiShape>(it->first);
			if (!pp->baseNif->GetVertsForShape(shape, verts))
				continue;

			pp->baseNif->GetUvsForShape(shape, uvs);
			vertsHigh = verts;
			vertsLow = verts;
			uvsHigh = uvs;
			uvsLow = uvs;

			ApplySliders(it->second.targetShape, sliderManager.slidersBig, pp->dataSets, vertsHigh, zapIdx, &uvsHigh);
			if (pp->sliderSet.GenWeights())
				ApplySliders(it->second.targetShape, sliderManager.slidersSmall, pp->dataSets, vertsLow, zapIdx, &uvsLow);

			// Calculate result of weight
			auto uvsz = uvs.size();
			for (size_t i = 0; i < verts.size(); i++) {
				verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));
				if (uvsz > i)
					uvs[i] = (uvsHigh[i] / 100.0f * weight) + (uvsLow[i] / 100.0f * (100.0f - weight));
			}

			ShapePreviewData spd;
			spd.name = it->first;
			spd.verts = std::move(verts);
			spd.uvs = std::move(uvs);
			spd.zapIdx = zapIdx;
			spd.projectIdx = pi;
			shapeData.push_back(std::move(spd));
		}
	}

	return shapeData;
}

void BodySlideApp::PostProcessPreview(std::vector<ShapePreviewData>& shapeData, int weight) {
	// Apply clipping fix and handle external reference
	bool useExternalReference = !multiProjectMode && referenceNif && preview &&
							  (clippingFixStrength > 0.0f || preview->IsShowReferenceChecked());

	std::vector<Vector3> extRefVerts;
	if (useExternalReference)
		UpdateExternalReferenceMesh(weight, &extRefVerts);

	// Hide external reference mesh when not in use
	if (!useExternalReference && referenceNif && preview) {
		preview->SetMeshVisibility(referenceShapeName, false);
	}

	if (clippingFixStrength > 0.0f) {
		for (int pi = 0; pi < static_cast<int>(projects.size()); pi++) {
			auto& pp = projects[pi];
			if (!pp->baseNif)
				continue;

			// Try built-in reference shape first
			auto refShape = ClippingFixer::FindReferenceShape(*pp->baseNif);
			const std::vector<Vector3>* bodyVerts = nullptr;
			std::vector<Triangle> bodyTris;

			if (refShape) {
				// Use built-in reference shape
				ShapePreviewData* refData = nullptr;
				for (auto& sd : shapeData) {
					if (sd.projectIdx == pi && sd.name == refShape->name.get()) {
						refData = &sd;
						break;
					}
				}
				if (refData && !refData->verts.empty()) {
					refShape->GetTriangles(bodyTris);
					bodyVerts = &refData->verts;
				}
			}

			// Fall back to external reference if no built-in reference
			if (!bodyVerts && useExternalReference && pi == 0 && !extRefVerts.empty()) {
				auto extRefShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
				if (extRefShape) {
					extRefShape->GetTriangles(bodyTris);
					bodyVerts = &extRefVerts;
					refShape = extRefShape;
				}
			}

			if (!bodyVerts || bodyTris.empty())
				continue;

			std::string refShapeName = refShape->name.get();
			std::unordered_map<std::string, std::vector<Vector3>*> shapeVerts;
			for (auto& sd : shapeData) {
				if (sd.projectIdx == pi && sd.name != refShapeName)
					shapeVerts[sd.name] = &sd.verts;
			}

			ApplyClippingFix(*pp->baseNif, *bodyVerts, bodyTris, shapeVerts);
		}
	}

	// Phase 3: Zap and update meshes
	for (auto& sd : shapeData) {
		if (sd.zapIdx.size() > 0) {
			for (int z = sd.zapIdx.size() - 1; z >= 0; z--) {
				if (sd.zapIdx[z] >= sd.verts.size())
					continue;

				sd.verts.erase(sd.verts.begin() + sd.zapIdx[z]);
				sd.uvs.erase(sd.uvs.begin() + sd.zapIdx[z]);
			}
		}

		if (previewPhysicsRunning) {
			// Remember the morphed shape so a physics tick can skin it again
			// without running all sliders, then show it simulated
			previewMorphedVerts[sd.name] = sd.verts;
			ApplyPreviewPhysicsSkinning(sd.name, sd.verts);
		}

		preview->UpdateMeshes(sd.name, &sd.verts, &sd.uvs);
	}

	preview->Render();
}

void BodySlideApp::UpdatePreviewPhysicsAvailability(bool keepRunning) {
	// Whatever led here replaced the previewed meshes, so the simulation - which
	// holds shapes of those meshes - has to go either way.
	const bool wasRunning = previewPhysicsRunning;
	EnablePreviewPhysics(false);

	previewPhysicsAvailable = false;
	for (auto& pp : projects) {
		if (Physics::HasPhysicsLinks(&pp->modNif, {})) {
			previewPhysicsAvailable = true;
			break;
		}
	}

	if (keepRunning && wasRunning && previewPhysicsAvailable)
		EnablePreviewPhysics(true);

	if (preview) {
		preview->ShowPhysicsControls(previewPhysicsAvailable);
		preview->SetPhysicsChecked(previewPhysicsRunning);
	}
}

void BodySlideApp::EnablePreviewPhysics(bool enable) {
	if (enable == previewPhysicsRunning)
		return;

	if (!enable) {
		previewPhysicsRunning = false;
		previewPhysics.clear();

		// Put the meshes back to the plain morphed shape the sliders describe
		if (IsPreviewRenderable()) {
			for (auto& morphedVerts : previewMorphedVerts)
				preview->UpdateMeshes(morphedVerts.first, &morphedVerts.second);

			preview->Render();
		}

		previewMorphedVerts.clear();
		return;
	}

	if (!IsPreviewRenderable() || projects.empty())
		return;

	// The simulation reads its kinematic input from the application skeleton,
	// which BodySlide has no other use for and therefore never loaded.
	if (AnimSkeleton::getInstance().GetActiveBoneCount() == 0 && LoadDefaultSkeletonReference() != 0) {
		wxLogError("Physics preview needs the reference skeleton, which could not be loaded.");
		return;
	}

	for (size_t i = 0; i < projects.size(); i++) {
		auto& pp = projects[i];

		auto anim = std::make_unique<AnimInfo>();
		if (!anim->LoadFromNif(&pp->modNif))
			continue;

		// The mod's own folder is the fallback for physics XMLs that aren't
		// installed under the game data path
		const std::string& nifPath = pp->inputFileName;

		std::vector<std::string> warnings;
		auto controller = std::make_unique<Physics::Controller>();
		size_t systemCount = controller->BuildFromNif(
			&pp->modNif,
			anim.get(),
			[&nifPath](const std::string& xmlPath) { return GameDataStream::OpenPhysicsXml(xmlPath, nifPath); },
			{},
			warnings);

		for (auto& warning : warnings)
			wxLogWarning("Physics: %s", warning);

		if (systemCount == 0)
			continue;

		controller->ResetDynamics();

		PreviewPhysicsProject entry;
		entry.controller = std::move(controller);
		entry.anim = std::move(anim);
		entry.projectIdx = i;
		previewPhysics.push_back(std::move(entry));
	}

	if (previewPhysics.empty()) {
		wxLogWarning("No physics XMLs could be loaded for the previewed meshes.");
		return;
	}

	previewPhysicsRunning = true;
	previewPhysicsClock.Reset();

	// Fills the morphed vertex cache and shows the first simulated frame
	UpdatePreview();
}

void BodySlideApp::ApplyPreviewPhysicsSkinning(const PreviewPhysicsProject& physics, const std::string& shapeName, std::vector<Vector3>& verts) {
	auto& modNif = projects[physics.projectIdx]->modNif;
	auto shape = modNif.FindBlockByName<NiShape>(shapeName);
	if (!shape)
		return;

	ApplySkinningToVerts(*physics.anim, shape, modNif.GetHeader().GetVersion().IsSF(), &physics.controller->PoseOverrides(), verts);
}

void BodySlideApp::ApplyPreviewPhysicsSkinning(const std::string& shapeName, std::vector<Vector3>& verts) {
	for (auto& physics : previewPhysics) {
		if (physics.controller->AffectedShapes().count(shapeName) != 0) {
			ApplyPreviewPhysicsSkinning(physics, shapeName, verts);
			return;
		}
	}
}

void BodySlideApp::PumpPreviewPhysics() {
	if (!previewPhysicsRunning)
		return;

	if (!IsPreviewRenderable()) {
		EnablePreviewPhysics(false);
		return;
	}

	float dtSeconds = 0.0f;
	if (!previewPhysicsClock.StepDue(dtSeconds))
		return;

	for (auto& physics : previewPhysics)
		physics.controller->Step(dtSeconds);

	// Only the shapes the simulation actually drives have to be skinned again
	std::vector<Vector3> verts;
	for (auto& physics : previewPhysics) {
		for (auto& shapeName : physics.controller->AffectedShapes()) {
			auto morphedVerts = previewMorphedVerts.find(shapeName);
			if (morphedVerts == previewMorphedVerts.end())
				continue;

			verts = morphedVerts->second;
			ApplyPreviewPhysicsSkinning(physics, shapeName, verts);
			preview->UpdateMeshes(shapeName, &verts);
		}
	}

	preview->Render();
}

void BodySlideApp::InjectPreviewCameraYaw(float deltaDegrees) {
	for (auto& physics : previewPhysics)
		physics.controller->InjectCameraYaw(deltaDegrees);
}

void BodySlideApp::SetPreviewWind(int directionIndex, int strengthPercent) {
	for (auto& physics : previewPhysics) {
		physics.controller->SetWindDirection(Physics::WindDirectionFromIndex(directionIndex));
		physics.controller->SetWindStrength(strengthPercent / 100.0f);
	}
}

void BodySlideApp::CleanupPreview() {
	if (!preview)
		return;

	EnablePreviewPhysics(false);
	previewPhysicsAvailable = false;

	// Cancel async load and wait for it to finish
	++previewLoadGeneration;
	if (previewLoadThread.joinable())
		previewLoadThread.join();
	previewLoading = false;

	preview->Cleanup();
	preview->ShowLoadingIndicator(false);
	referenceNif.reset();

	if (multiProjectMode) {
		projects.clear();
		multiProjectMode = false;
	}
	else if (!projects.empty()) {
		auto* pp = projects[0].get();
		delete pp->baseNif;
		pp->baseNif = nullptr;
		pp->inputFileName.clear();
	}
}

void BodySlideApp::RebuildPreviewMeshes() {
	if (!IsPreviewRenderable())
		return;

	if (projects.empty())
		return;

	int weight = preview->GetWeight();
	auto shapeData = ComputeMorphedShapeData(weight);

	// Multi-project mode
	if (multiProjectMode) {
		for (auto& pp : projects) {
			if (!pp->baseNif)
				continue;

			pp->modNif.CopyFrom(*pp->baseNif);

			bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

			for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
				// Find the matching shape data
				ShapePreviewData* spd = nullptr;
				for (auto& sd : shapeData) {
					if (sd.name == it->first) {
						spd = &sd;
						break;
					}
				}
				if (!spd)
					continue;

				// Zap deleted verts before preview
				auto shape = pp->modNif.FindBlockByName<NiShape>(it->first);
				if (spd->zapIdx.size() > 0) {
					pp->modNif.SetVertsForShape(shape, spd->verts);
					pp->modNif.SetUvsForShape(shape, spd->uvs);
					if (pp->modNif.DeleteVertsForShape(shape, spd->zapIdx) && !keepZappedShapes)
						pp->modNif.DeleteShape(shape);
				}
				else {
					pp->modNif.SetVertsForShape(shape, spd->verts);
					pp->modNif.SetUvsForShape(shape, spd->uvs);
				}
			}
		}

		// Refresh all meshes from all projects
		std::vector<NifFile*> modNifs;
		for (auto& pp : projects)
			modNifs.push_back(&pp->modNif);
		preview->RefreshMeshFromNif(modNifs);

		// Update mesh settings from all sets
		for (auto& pp : projects) {
			for (auto it = pp->sliderSet.ShapesBegin(); it != pp->sliderSet.ShapesEnd(); ++it) {
				Mesh* m = preview->GetMesh(it->first);
				if (m) {
					m->smoothSeamNormals = it->second.smoothSeamNormals;
					m->lockNormals = it->second.lockNormals;
					m->SmoothNormals();
				}
			}
		}

		PostProcessPreview(shapeData, weight);
		UpdatePreviewPhysicsAvailability(true);
		return;
	}

	// Single-project mode
	auto* pp = projects[0].get();
	if (!pp->baseNif)
		return;

	pp->modNif.CopyFrom(*pp->baseNif);

	bool keepZappedShapes = pp->sliderSet.KeepZappedShapes();

	for (auto& sd : shapeData) {
		auto shape = pp->modNif.FindBlockByName<NiShape>(sd.name);
		if (!shape)
			continue;

		if (sd.zapIdx.size() > 0) {
			pp->modNif.SetVertsForShape(shape, sd.verts);
			pp->modNif.SetUvsForShape(shape, sd.uvs);
			if (pp->modNif.DeleteVertsForShape(shape, sd.zapIdx) && !keepZappedShapes)
				pp->modNif.DeleteShape(shape);
		}
		else {
			pp->modNif.SetVertsForShape(shape, sd.verts);
			pp->modNif.SetUvsForShape(shape, sd.uvs);
		}
	}

	preview->RefreshMeshFromNif({&pp->modNif});
	UpdateMeshesFromSet(pp->sliderSet);

	// Re-add external reference mesh after refresh (which clears all meshes)
	if (referenceNif && !multiProjectMode) {
		preview->AddMeshFromNif(referenceNif.get(), const_cast<char*>(referenceShapeName.c_str()));
		preview->AddNifShapeTextures(referenceNif.get(), referenceShapeName);
		preview->SetMeshVisibility(referenceShapeName, false);
	}

	PostProcessPreview(shapeData, weight);
	UpdatePreviewPhysicsAvailability(true);
}

void BodySlideApp::UpdateExternalReferenceMesh(int weight, std::vector<Vector3>* outVerts) {
	if (!preview || !referenceNif)
		return;

	std::vector<Vector3> extRefVerts;
	std::vector<Vector2> extRefUvs;
	auto refShape = referenceNif->FindBlockByName<NiShape>(referenceShapeName);
	if (!refShape || !referenceNif->GetVertsForShape(refShape, extRefVerts))
		return;

	referenceNif->GetUvsForShape(refShape, extRefUvs);

	std::string targetName = referenceSliderSet.ShapeToTarget(referenceShapeName);
	if (!targetName.empty()) {
		// Build slider vectors from the reference project's slider set.
		// We can't use sliderManager.slidersBig/slidersSmall directly because
		// their linkedDataSets contain data names from the outfit project,
		// which don't match the data names in referenceDiffData.
		std::vector<Slider> refSlidersBig;
		std::vector<Slider> refSlidersSmall;
		for (size_t si = 0; si < referenceSliderSet.size(); si++) {
			auto& sd = referenceSliderSet[si];
			Slider s;
			s.name = sd.name;
			s.invert = sd.bInvert;
			s.zap = sd.bZap;
			s.clamp = sd.bClamp;
			s.uv = sd.bUV;
			for (auto& df : sd.dataFiles)
				if (df.targetName == targetName)
					s.linkedDataSets.push_back(df.dataName);

			s.value = sliderManager.GetSlider(sd.name, false);
			refSlidersBig.push_back(s);

			s.value = sliderManager.GetSlider(sd.name, true);
			refSlidersSmall.push_back(std::move(s));
		}

		std::vector<Vector3> refVertsHigh = extRefVerts;
		std::vector<Vector3> refVertsLow = extRefVerts;
		std::vector<Vector2> refUvsHigh = extRefUvs;
		std::vector<Vector2> refUvsLow = extRefUvs;
		std::vector<uint16_t> extZapIdx;

		ApplySliders(targetName, refSlidersBig, referenceDiffData, refVertsHigh, extZapIdx, &refUvsHigh);
		if (referenceSliderSet.GenWeights())
			ApplySliders(targetName, refSlidersSmall, referenceDiffData, refVertsLow, extZapIdx, &refUvsLow);

		auto uvsz = extRefUvs.size();
		for (size_t i = 0; i < extRefVerts.size(); i++) {
			extRefVerts[i] = (refVertsHigh[i] / 100.0f * weight) + (refVertsLow[i] / 100.0f * (100.0f - weight));
			if (uvsz > i)
				extRefUvs[i] = (refUvsHigh[i] / 100.0f * weight) + (refUvsLow[i] / 100.0f * (100.0f - weight));
		}
	}

	preview->UpdateMeshes(referenceShapeName, &extRefVerts, &extRefUvs);
	preview->SetMeshVisibility(referenceShapeName, true);

	if (outVerts)
		*outVerts = std::move(extRefVerts);
}

void BodySlideApp::UpdateMeshesFromSet(SliderSet& set) {
	for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
		Mesh* m = preview->GetMesh(it->first);
		if (m) {
			m->smoothSeamNormals = it->second.smoothSeamNormals;
			m->lockNormals = it->second.lockNormals;

			m->SmoothNormals();
		}
	}
}

void BodySlideApp::ApplyReferenceNormals(NifFile& nif) {
	static std::mutex refNormalsCacheMutex;
	std::lock_guard<std::mutex> cacheLock(refNormalsCacheMutex);

	for (auto& s : nif.GetShapes()) {
		std::string shapeName = s->name.get();

		if (refNormalsCache.find(shapeName) != refNormalsCache.end()) {
			// Apply normals from file cache
			NifFile& srcNif = refNormalsCache[shapeName];
			nif.ApplyNormalsFromFile(srcNif, shapeName);
		}
		else {
			// Check if reference normals file exists
			wxString fileName = wxString::Format("%s/RefNormals/%s.nif", wxString::FromUTF8(Config["AppDir"]), wxString::FromUTF8(shapeName));
			if (wxFile::Exists(fileName)) {
				std::fstream file;
				PlatformUtil::OpenFileStream(file, fileName.ToUTF8().data(), std::ios::in | std::ios::binary);

				NifFile srcNif;
				if (srcNif.Load(file) != 0)
					continue;

				// Apply normals from file
				nif.ApplyNormalsFromFile(srcNif, shapeName);

				// Move file to cache
				refNormalsCache[shapeName] = std::move(srcNif);
			}
		}
	}
}

bool BodySlideApp::SetDefaultConfig() {
	int xborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_X);
	if (xborder < 0)
		xborder = 0;
	int yborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_Y);
	if (yborder < 0)
		yborder = 0;

	int currentTarget = -1;
	Config.SetDefaultValue("TargetGame", currentTarget);
	currentTarget = Config.GetIntValue("TargetGame");

	Config.SetDefaultBoolValue("WarnMissingGamePath", true);
	Config.SetDefaultBoolValue("BSATextureScan", true);
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultBoolValue("UseSystemLanguage", false);
	SettingsDialogShared::SetDefaultAppearanceMode(Config);
	BodySlideConfig.SetDefaultValue("SelectedOutfit", "");
	BodySlideConfig.SetDefaultValue("SelectedPreset", "");
	BodySlideConfig.SetDefaultBoolValue("BuildMorphs", false);
	BodySlideConfig.SetDefaultBoolValue("RegexFilterOutfits", false);
	Config.SetDefaultValue("Input/SliderMinimum", 0);
	Config.SetDefaultValue("Input/SliderMaximum", 100);
	Config.SetDefaultBoolValue("Input/LeftMousePan", false);
	Config.SetDefaultBoolValue("Input/BrushSettingsNearCursor", true);
	Config.SetDefaultBoolValue("Input/MaskHistory", true);
	Config.SetDefaultBoolValue("Input/ShapeHoverHighlight", true);
	Config.SetDefaultValue("Lights/Ambient", 15);
	Config.SetDefaultValue("Lights/Frontal", 100);
	Config.SetDefaultValue("Lights/Directional0", 0);
	Config.SetDefaultValue("Lights/Directional0.x", -90);
	Config.SetDefaultValue("Lights/Directional0.y", 10);
	Config.SetDefaultValue("Lights/Directional0.z", 100);
	Config.SetDefaultValue("Lights/Directional1", 0);
	Config.SetDefaultValue("Lights/Directional1.x", 70);
	Config.SetDefaultValue("Lights/Directional1.y", 10);
	Config.SetDefaultValue("Lights/Directional1.z", 100);
	Config.SetDefaultValue("Lights/Directional2", 0);
	Config.SetDefaultValue("Lights/Directional2.x", 30);
	Config.SetDefaultValue("Lights/Directional2.y", 20);
	Config.SetDefaultValue("Lights/Directional2.z", -100);
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

	Config.SetDefaultValue("GameRegKey/Oblivion", "Software\\Bethesda Softworks\\Oblivion");
	Config.SetDefaultValue("GameRegVal/Oblivion", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout3", "Software\\Bethesda Softworks\\Fallout3");
	Config.SetDefaultValue("GameRegVal/Fallout3", "Installed Path");
	Config.SetDefaultValue("GameRegKey/FalloutNewVegas", "Software\\Bethesda Softworks\\FalloutNV");
	Config.SetDefaultValue("GameRegVal/FalloutNewVegas", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Skyrim", "Software\\Bethesda Softworks\\Skyrim");
	Config.SetDefaultValue("GameRegVal/Skyrim", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4", "Software\\Bethesda Softworks\\Fallout4");
	Config.SetDefaultValue("GameRegVal/Fallout4", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimSpecialEdition", "Software\\Bethesda Softworks\\Skyrim Special Edition");
	Config.SetDefaultValue("GameRegVal/SkyrimSpecialEdition", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4VR", "Software\\Bethesda Softworks\\Fallout 4 VR");
	Config.SetDefaultValue("GameRegVal/Fallout4VR", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimVR", "Software\\Bethesda Softworks\\Skyrim VR");
	Config.SetDefaultValue("GameRegVal/SkyrimVR", "Installed Path");

	// Target game not set, show setup dialog
	if (currentTarget == -1)
		if (!ShowSetup())
			return false;

	targetGame = (TargetGame)Config.GetIntValue("TargetGame");

#ifdef _WINDOWS
	wxString gameKey = Config["GameRegKey/" + GameUtil::TargetGames[targetGame]];
	wxString gameValueKey = Config["GameRegVal/" + GameUtil::TargetGames[targetGame]];

	if (Config["GameDataPath"].empty()) {
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (!gameKey.empty() && key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data").Append(PathSepChar);
				Config.SetDefaultValue("GameDataPath", installPath.ToUTF8().data());
				wxLogMessage("Registry game data path: %s", installPath);
			}
		}
	}
#endif

	if (Config["GameDataPath"].empty()) {
		if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
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
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Setup.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load Setup.xrc file!", _("Error"), wxICON_ERROR);
		return false;
	}

	wxDialog* setup = xrc->LoadDialog(nullptr, "dlgSetup");
	if (setup) {
		setup->SetSize(setup->FromDIP(wxSize(700, -1)));
		setup->CenterOnScreen();

		wxButton* btOblivion = XRCCTRL(*setup, "btOblivion", wxButton);
		btOblivion->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)OB); });

		wxButton* btFallout3 = XRCCTRL(*setup, "btFallout3", wxButton);
		btFallout3->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO3); });

		wxButton* btFalloutNV = XRCCTRL(*setup, "btFalloutNV", wxButton);
		btFalloutNV->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FONV); });

		wxButton* btSkyrim = XRCCTRL(*setup, "btSkyrim", wxButton);
		btSkyrim->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIM); });

		wxButton* btFallout4 = XRCCTRL(*setup, "btFallout4", wxButton);
		btFallout4->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO4); });

		wxButton* btSkyrimSE = XRCCTRL(*setup, "btSkyrimSE", wxButton);
		btSkyrimSE->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIMSE); });

		wxButton* btFallout4VR = XRCCTRL(*setup, "btFallout4VR", wxButton);
		btFallout4VR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO4VR); });

		wxButton* btSkyrimVR = XRCCTRL(*setup, "btSkyrimVR", wxButton);
		btSkyrimVR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIMVR); });

		wxButton* btStarfield = XRCCTRL(*setup, "btStarfield", wxButton);
		btStarfield->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SF); });

		wxDirPickerCtrl* dirOblivion = XRCCTRL(*setup, "dirOblivion", wxDirPickerCtrl);
		dirOblivion->Bind(wxEVT_DIRPICKER_CHANGED, [&dirOblivion, &btOblivion](wxFileDirPickerEvent&) { btOblivion->Enable(dirOblivion->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFallout3 = XRCCTRL(*setup, "dirFallout3", wxDirPickerCtrl);
		dirFallout3->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout3, &btFallout3](wxFileDirPickerEvent&) { btFallout3->Enable(dirFallout3->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFalloutNV = XRCCTRL(*setup, "dirFalloutNV", wxDirPickerCtrl);
		dirFalloutNV->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFalloutNV, &btFalloutNV](wxFileDirPickerEvent&) { btFalloutNV->Enable(dirFalloutNV->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrim = XRCCTRL(*setup, "dirSkyrim", wxDirPickerCtrl);
		dirSkyrim->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrim, &btSkyrim](wxFileDirPickerEvent&) { btSkyrim->Enable(dirSkyrim->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFallout4 = XRCCTRL(*setup, "dirFallout4", wxDirPickerCtrl);
		dirFallout4->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout4, &btFallout4](wxFileDirPickerEvent&) { btFallout4->Enable(dirFallout4->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrimSE = XRCCTRL(*setup, "dirSkyrimSE", wxDirPickerCtrl);
		dirSkyrimSE->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrimSE, &btSkyrimSE](wxFileDirPickerEvent&) { btSkyrimSE->Enable(dirSkyrimSE->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFallout4VR = XRCCTRL(*setup, "dirFallout4VR", wxDirPickerCtrl);
		dirFallout4VR->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout4VR, &btFallout4VR](wxFileDirPickerEvent&) { btFallout4VR->Enable(dirFallout4VR->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrimVR = XRCCTRL(*setup, "dirSkyrimVR", wxDirPickerCtrl);
		dirSkyrimVR->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrimVR, &btSkyrimVR](wxFileDirPickerEvent&) { btSkyrimVR->Enable(dirSkyrimVR->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirStarfield = XRCCTRL(*setup, "dirStarfield", wxDirPickerCtrl);
		dirStarfield->Bind(wxEVT_DIRPICKER_CHANGED, [&dirStarfield, &btStarfield](wxFileDirPickerEvent&) { btStarfield->Enable(dirStarfield->GetDirName().DirExists()); });

		wxFileName dir = GameUtil::GetGameDataPath(OB);
		if (dir.DirExists()) {
			dirOblivion->SetDirName(dir);
			btOblivion->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO3);
		if (dir.DirExists()) {
			dirFallout3->SetDirName(dir);
			btFallout3->Enable();
		}

		dir = GameUtil::GetGameDataPath(FONV);
		if (dir.DirExists()) {
			dirFalloutNV->SetDirName(dir);
			btFalloutNV->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIM);
		if (dir.DirExists()) {
			dirSkyrim->SetDirName(dir);
			btSkyrim->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO4);
		if (dir.DirExists()) {
			dirFallout4->SetDirName(dir);
			btFallout4->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIMSE);
		if (dir.DirExists()) {
			dirSkyrimSE->SetDirName(dir);
			btSkyrimSE->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO4VR);
		if (dir.DirExists()) {
			dirFallout4VR->SetDirName(dir);
			btFallout4VR->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIMVR);
		if (dir.DirExists()) {
			dirSkyrimVR->SetDirName(dir);
			btSkyrimVR->Enable();
		}

		dir = GameUtil::GetGameDataPath(SF);
		if (dir.DirExists()) {
			dirStarfield->SetDirName(dir);
			btStarfield->Enable();
		}

		if (setup->ShowModal() != wxID_CANCEL) {
			int targ = setup->GetReturnCode();
			Config.SetValue("TargetGame", targ);

			wxFileName dataDir;
			switch (targ) {
				case OB:
					dataDir = dirOblivion->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_ob.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case FO3:
					dataDir = dirFallout3->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo3nv.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case FONV:
					dataDir = dirFalloutNV->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo3nv.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case SKYRIM:
					dataDir = dirSkyrim->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sk.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case FO4:
					dataDir = dirFallout4->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo4.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
				case SKYRIMSE:
					dataDir = dirSkyrimSE->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sse.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case FO4VR:
					dataDir = dirFallout4VR->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo4.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
				case SKYRIMVR:
					dataDir = dirSkyrimVR->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sse.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case SF:
					dataDir = dirStarfield->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sf.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
			}

			Config.SetValue("GameDataPath", dataDir.GetFullPath().ToUTF8().data());
			Config.SetValue("GameDataPaths/" + GameUtil::TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToUTF8().data());

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
			delete setup;
		}
		else {
			delete setup;
			return false;
		}
	}

	return true;
}

void BodySlideApp::InitLanguage() {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");
	if (lang < 0)
		lang = wxLANGUAGE_ENGLISH;

	// Load language if possible, fall back to English otherwise
	if (wxLocale::IsAvailable(lang)) {
		locale = new wxLocale(lang);
		locale->AddCatalogLookupPathPrefix(wxString::FromUTF8(Config["AppDir"]) + "/lang");
		locale->AddCatalog("BodySlide");

		if (!locale->IsOk()) {
			wxLogError("System language '%d' is wrong.", lang);
			wxMessageBox(wxString::Format(_("System language '%d' is wrong."), lang));

			delete locale;
			locale = new wxLocale(wxLANGUAGE_ENGLISH);
			lang = wxLANGUAGE_ENGLISH;
		}
	}
	else {
		wxLogError("The system language '%d' is not supported by your system. Try installing support for this language.", lang);
		wxMessageBox(wxString::Format(_("The system language '%d' is not supported by your system. Try installing support for this language."), lang));

		locale = new wxLocale(wxLANGUAGE_ENGLISH);
		lang = wxLANGUAGE_ENGLISH;
	}

	wxLogMessage("Using language '%s'.", wxLocale::GetLanguageName(lang));
}

void BodySlideApp::LoadAllCategories() {
	wxLogMessage("Loading all slider categories...");
	cCollection.Clear();
	cCollection.LoadCategories(ProjectUtil::GetProjectPath() + "/SliderCategories");
}

void BodySlideApp::SetPresetGroups(const std::string& setName) {
	presetGroups.clear();
	gCollection.GetOutfitGroups(setName, presetGroups);

	if (presetGroups.empty()) {
		Config.GetValueArray("DefaultGroups", "GroupName", presetGroups);
		if (!presetGroups.empty()) {
			wxString defaultGroups;
			for (auto& group : presetGroups)
				defaultGroups.Append("'" + group + "' ");

			wxLogMessage("Using default group(s): %s", defaultGroups);
		}
		else
			wxLogMessage("No group assigned for set '%s'.", setName);
	}
	else {
		wxString groups;
		for (auto& group : presetGroups)
			groups.Append("'" + group + "' ");

		wxLogMessage("Using group(s): %s", groups);
	}
}

void BodySlideApp::LoadAllGroups() {
	wxLogMessage("Loading all slider groups...");
	gCollection.LoadGroups(ProjectUtil::GetProjectPath() + "/SliderGroups");

	ungroupedOutfits.clear();
	for (auto& o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	std::vector<std::string> aliases;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "alias", aliases);
	std::vector<std::string> groups;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "group", groups);

	if (aliases.size() == groups.size())
		for (size_t i = 0; i < aliases.size(); i++)
			groupAlias[aliases[i]] = groups[i];
}

void BodySlideApp::GetAllGroupNames(std::vector<std::string>& outGroups) {
	std::set<std::string> gNames;
	gCollection.GetAllGroups(gNames);
	outGroups.assign(gNames.begin(), gNames.end());
}

int BodySlideApp::SaveGroupList(const std::string& fileName, const std::string& groupName) {
	if (filteredOutfits.empty())
		return 1;

	std::vector<std::string> existing;
	SliderSetGroupFile outfile(fileName);
	if (outfile.GetError() != 0) {
		if (outfile.GetError() == 1)
			outfile.New(fileName);

		if (outfile.GetError() != 0) {
			wxMessageBox(_("Failed to create group file."), _("Error"), wxICON_ERROR);
			return 5;
		}
	}

	outfile.GetGroupNames(existing);
	bool found = false;
	for (auto& e : existing) {
		if (StringsEqualInsens(e.c_str(), groupName.c_str())) {
			found = true;
			break;
		}
	}
	if (found) {
		int ret = wxMessageBox(_("That group already exists in the specified file, do you wish to overwrite the group?"), _("Group already exists"), wxYES_NO | wxCANCEL);
		if (ret == wxNO)
			return 2;
		else if (ret == wxCANCEL)
			return 3;
	}

	SliderSetGroup ssg;
	ssg.SetName(groupName);
	ssg.AddMembers(filteredOutfits);

	outfile.UpdateGroup(ssg);
	outfile.Save();
	return 0;
}

void BodySlideApp::PopulateFilterData() {
	if (outfitHasZaps.empty())
		LoadSliderSets();
}

void BodySlideApp::ApplyOutfitFilter() {
	filteredOutfits.clear();

	std::unordered_set<std::string> grpFiltOutfits;
	std::vector<std::string> workFilterList;
	static wxString lastGrps = "";
	static std::set<std::string> grouplist;

	bool showUngrouped = false;
	bool regexFilterOutfits = false;
	bool filterHasZaps = false;
	bool filterBatchBuildSelected = false;

	auto menuOutfitSrchContext = sliderView->outfitsearch->GetMenu();
	if (menuOutfitSrchContext) {
		auto menuRegexOutfits = menuOutfitSrchContext->FindItem(XRCID("menuRegexOutfits"));
		if (menuRegexOutfits)
			regexFilterOutfits = menuRegexOutfits->IsChecked();

		auto menuFilterHasZaps = menuOutfitSrchContext->FindItem(XRCID("menuFilterHasZaps"));
		if (menuFilterHasZaps)
			filterHasZaps = menuFilterHasZaps->IsChecked();

		auto menuFilterOutputWinners = menuOutfitSrchContext->FindItem(XRCID("menuFilterOutputWinners"));
		if (menuFilterOutputWinners)
			filterBatchBuildSelected = menuFilterOutputWinners->IsChecked();
	}

	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	if (filterBatchBuildSelected)
		GetBuildSelection(buildSelFile, buildSelection);

	wxString grpSrch = sliderView->search->GetValue();
	std::string outfitSrch{sliderView->outfitsearch->GetValue()};

	if (lastGrps != grpSrch) {
		grouplist.clear();
		if (grpSrch.empty()) {
			gCollection.GetAllGroups(grouplist);
			showUngrouped = true;
		}
		else {
			wxStringTokenizer tokenizer(grpSrch, ",;");
			while (tokenizer.HasMoreTokens()) {
				wxString token = tokenizer.GetNextToken();
				token.Trim();
				token.Trim(false);
				std::string group = token.ToStdString();
				grouplist.insert(group);
			}
		}
		lastGrps = grpSrch;
	}
	else if (lastGrps.empty()) {
		gCollection.GetAllGroups(grouplist);
		showUngrouped = true;
	}

	for (auto& gn : grouplist) {
		if (gn == "Unassigned")
			showUngrouped = true;
		else
			gCollection.GetGroupMembers(gn, grpFiltOutfits);
	}

	if (showUngrouped)
		for (auto& ug : ungroupedOutfits)
			grpFiltOutfits.insert(ug);

	for (auto& no : outfitNameOrder) {
		bool filteredOut = false;

		if (grpFiltOutfits.find(no) == grpFiltOutfits.end())
			filteredOut = true;

		if (!filteredOut && filterHasZaps) {
			if (std::find(outfitHasZaps.cbegin(), outfitHasZaps.cend(), no) == outfitHasZaps.cend())
				filteredOut = true;
		}

		if (!filteredOut && filterBatchBuildSelected) {
			for (auto& outFile : outFileCount) {
				if (outFile.second.size() > 1) {
					bool isInConflict = std::find(outFile.second.cbegin(), outFile.second.cend(), no) != outFile.second.cend();
					if (isInConflict) {
						std::string choice = buildSelection.GetOutputChoice(outFile.first);
						if (!choice.empty() && choice != no) {
							filteredOut = true;
							break;
						}
					}
				}
			}
		}

		if (!filteredOut)
			workFilterList.push_back(no);
	}


	filteredOutfits = FilterOutfitNames(workFilterList, outfitSrch, regexFilterOutfits);

	SortOutfitNamesForDisplay(filteredOutfits);

	BodySlideConfig.SetValue("LastGroupFilter", grpSrch.ToUTF8().data());
	BodySlideConfig.SetValue("LastOutfitFilter", outfitSrch);
}

std::vector<std::string> BodySlideApp::FilterOutfitNames(const std::vector<std::string>& names, const std::string& filter, bool useRegex, std::string* regexError) const {
	if (filter.empty())
		return names;

	std::vector<std::string> matches;

	if (useRegex) {
		std::regex re;

		try {
			re.assign(filter, std::regex::icase);
		}
		catch (const std::regex_error& e) {
			if (regexError)
				*regexError = e.what();

			return matches;
		}

		for (auto& name : names)
			if (std::regex_search(name, re))
				matches.push_back(name);
	}
	else {
		wxString searchStr = wxString::FromUTF8(filter);
		searchStr.MakeLower();

		for (auto& name : names)
			if (wxString::FromUTF8(name).Lower().Contains(searchStr))
				matches.push_back(name);
	}

	return matches;
}

std::vector<std::string> BodySlideApp::ApplyPresetFilter(const std::vector<std::string>& presetNames) {
	wxString presetSearchStr = sliderView->presetFilter->GetValue();

	std::vector<std::string> filteredPresets;

	if (presetSearchStr.empty()) {
		for (auto& w : presetNames)
			filteredPresets.push_back(w);
	}
	else {
		presetSearchStr.MakeLower();

		for (auto& filterEntry : presetNames) {
			wxString entryStr = wxString::FromUTF8(filterEntry);
			if (entryStr.Lower().Contains(presetSearchStr))
				filteredPresets.push_back(entryStr.ToUTF8().data());
		}
	}

	SortPresetNamesForDisplay(filteredPresets);

	BodySlideConfig.SetValue("LastPresetFilter", presetSearchStr.ToUTF8().data());
	return filteredPresets;
}

int BodySlideApp::GetOutfits(std::vector<std::string>& outList) {
	outList.assign(outfitNameOrder.begin(), outfitNameOrder.end());
	return outList.size();
}

int BodySlideApp::GetFilteredOutfits(std::vector<std::string>& outList) {
	outList.assign(filteredOutfits.begin(), filteredOutfits.end());
	return outList.size();
}

void BodySlideApp::LoadCmdPresetFile() {
	if (cmdPresetFile.empty())
		return;

	std::vector<std::string> noGroupFilter;
	std::vector<std::string> loadedPresets;

	// Loaded before the preset folder so that the file given on the command line wins on name conflicts.
	if (!sliderManager.LoadPresetFile(cmdPresetFile, "", noGroupFilter, true, &loadedPresets)) {
		wxLogError("Failed to load preset file '%s' from the command line.", cmdPresetFile);
		cmdPresetFile.clear();
		cmdPresetPending = false;
		return;
	}

	// The presets of the file are loaded again whenever the preset collection was cleared, the name is only resolved once.
	if (cmdPresetResolved)
		return;

	cmdPresetResolved = true;

	if (!cmdPreset.empty()) {
		if (std::find(loadedPresets.begin(), loadedPresets.end(), cmdPreset) == loadedPresets.end())
			wxLogWarning("Preset '%s' was not found in preset file '%s' from the command line.", cmdPreset, cmdPresetFile);

		return;
	}

	if (loadedPresets.empty()) {
		wxLogWarning("No presets found in preset file '%s' from the command line.", cmdPresetFile);
		cmdPresetPending = false;
		return;
	}

	// Without an explicit name, the first preset of the file is used.
	cmdPreset = loadedPresets.front();
	wxLogMessage("Using preset '%s' of preset file '%s' from the command line.", cmdPreset, cmdPresetFile);
}

void BodySlideApp::LoadPresets(const std::string& sliderSet) {
	std::string outfit = sliderSet;
	if (sliderSet.empty())
		outfit = BodySlideConfig["SelectedOutfit"];

	wxLogMessage("Loading assigned presets...");

	LoadCmdPresetFile();

	std::vector<std::string> groups_and_aliases;
	for (auto& g : presetGroups) {
		groups_and_aliases.push_back(g);
		for (auto& ag : this->groupAlias)
			if (ag.second == g)
				groups_and_aliases.push_back(ag.first);
	}

	sliderManager.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", outfit, groups_and_aliases, groups_and_aliases.empty());
}

void BodySlideApp::GetPresetNames(std::vector<std::string>& outNames) {
	sliderManager.GetPresetNames(outNames);
}

std::string BodySlideApp::GetPresetFileName(const std::string& presetName) {
	return sliderManager.GetPresetFileNames(presetName);
}

void BodySlideApp::GetPresetGroups(const std::string& presetName, std::vector<std::string>& outGroups) {
	sliderManager.GetPresetGroups(presetName, outGroups);
}

void BodySlideApp::InitializeSliders(const std::string& presetName) {
	sliderManager.InitializeSliders(presetName);
}

void BodySlideApp::RefreshPresetsForCurrentOutfit() {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	std::string presetName = BodySlideConfig["SelectedPreset"];

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(presetName);
}

int BodySlideApp::BuildBodies(bool localPath, bool clean, bool tri, bool forceNormals) {
	if (projects.empty())
		return 1;

	auto& activeSet = GetActiveSet();
	std::string inputFileName = activeSet.GetInputFileName();
	NifFile nifSmall;
	NifFile nifBig;

	std::string outFileNameSmall;
	std::string outFileNameBig;

	wxLogMessage("Building set '%s' with options: Local Path = %s, Cleaning = %s, TRI = %s, GenWeights = %s",
				 activeSet.GetName(),
				 localPath ? "True" : "False",
				 clean ? "True" : "False",
				 tri ? "True" : "False",
				 activeSet.GenWeights() ? "True" : "False");

	if (localPath) {
		outFileNameSmall = outFileNameBig = activeSet.GetOutputFile();
	}
	else {
		if (GetOutputDataPath().empty()) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Would you like to show BodySlide where it is?"),
									   _("Game not found"),
									   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogMessage("Aborted build without data path.");
					return 4;
				}
			}

			wxString response = wxDirSelector(_("Please choose a directory to set as your Data path"), wxGetCwd());
			if (response.IsEmpty()) {
				wxLogMessage("Aborted build without data path.");
				return 4;
			}

			response.Append(PathSepChar);
			Config.SetValue("GameDataPath", response.ToUTF8().data());
		}

		outFileNameSmall = GetOutputDataPath() + activeSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;
		wxString path = wxString::FromUTF8(GetOutputDataPath() + activeSet.GetOutputPath());
		wxFileName::Mkdir(path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	// ALT key
	if (clean && !localPath) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folder, potentially causing crashes.\n\nDo you want to continue?"),
							   _("Clean Build"),
							   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning build.");
			return 4;
		}

		wxString removeHigh, removeLow;
		wxString msg = _("Removed the following files:\n");
		bool genWeights = activeSet.GenWeights();

		if (genWeights)
			removeHigh = outFileNameSmall + "_1.nif";
		else
			removeHigh = outFileNameSmall + ".nif";

		bool remHigh = wxRemoveFile(wxString::FromUTF8(removeHigh));
		if (remHigh)
			msg.Append(removeHigh + "\n");
		else
			msg.Append(removeHigh + _(" (no action)\n"));

		if (!genWeights) {
			wxLogMessage("%s", msg);
			wxMessageBox(msg, _("Process Successful"));
			return 0;
		}

		removeLow = outFileNameSmall + "_0.nif";
		bool remLow = wxRemoveFile(wxString::FromUTF8(removeLow));
		if (remLow)
			msg.Append(removeLow + "\n");
		else
			msg.Append(removeLow + _(" (no action)\n"));

		wxLogMessage("%s", msg);
		wxMessageBox(msg, _("Process Successful"));
		return 0;
	}

	refNormalsCache.clear();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, inputFileName, std::ios::in | std::ios::binary);

	int error = nifBig.Load(file);
	if (error) {
		wxLogError("Failed to load '%s' (%d)!", inputFileName, error);
		return 1;
	}

	if (activeSet.GenWeights())
		nifSmall.CopyFrom(nifBig);

	auto& dataSets = GetActiveDataSets();
	dataSets.Clear();
	activeSet.LoadSetDiffData(dataSets);

	bool keepZappedShapes = activeSet.KeepZappedShapes();

	std::vector<Vector3> vertsLow;
	std::vector<Vector3> vertsHigh;
	std::vector<Vector2> uvsLow;
	std::vector<Vector2> uvsHigh;
	std::vector<uint16_t> zapIdx;
	std::unordered_map<std::string, std::vector<uint16_t>> zapIdxAll;

	// Phase 1: Apply sliders and set vertices for all shapes
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		auto shape = nifBig.FindBlockByName<NiShape>(it->first);
		if (!nifBig.GetVertsForShape(shape, vertsHigh))
			continue;

		nifBig.GetUvsForShape(shape, uvsHigh);

		if (activeSet.GenWeights()) {
			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
				continue;

			nifSmall.GetUvsForShape(shapeSmall, uvsLow);
		}

		zapIdxAll.emplace(it->first, std::vector<uint16_t>());

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, dataSets, vertsHigh, zapIdx, &uvsHigh);
		nifBig.SetVertsForShape(shape, vertsHigh);
		nifBig.SetUvsForShape(shape, uvsHigh);

		if (activeSet.GenWeights()) {
			zapIdx.clear();
			ApplySliders(it->second.targetShape, sliderManager.slidersSmall, dataSets, vertsLow, zapIdx, &uvsLow);

			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			nifSmall.SetVertsForShape(shapeSmall, vertsLow);
			nifSmall.SetUvsForShape(shapeSmall, uvsLow);
		}

		zapIdxAll[it->first] = zapIdx;
		zapIdx.clear();
	}

	// Phase 2: Apply clipping fix when strength is above zero
	if (clippingFixStrength > 0.0f) {
		auto refShape = ClippingFixer::FindReferenceShape(nifBig);
		if (refShape) {
			std::vector<Vector3> bodyVerts;
			std::vector<Triangle> bodyTris;
			nifBig.GetVertsForShape(refShape, bodyVerts);
			refShape->GetTriangles(bodyTris);

			if (!bodyVerts.empty() && !bodyTris.empty()) {
				ClippingFixOptions fixOpts;
				fixOpts.strength = clippingFixStrength / 100.0f;

				for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
					auto shape = nifBig.FindBlockByName<NiShape>(it->first);
					if (shape == refShape || !ClippingFixer::IsEligibleForFix(nifBig, shape))
						continue;

					std::vector<Vector3> outfitVerts;
					std::vector<Triangle> outfitTris;
					if (!nifBig.GetVertsForShape(shape, outfitVerts))
						continue;
					shape->GetTriangles(outfitTris);

					ClippingFixer::FixClipping(bodyVerts, bodyTris, outfitVerts, outfitTris, fixOpts);
					nifBig.SetVertsForShape(shape, outfitVerts);
				}

				// Also fix the small/low weight NIF
				if (activeSet.GenWeights()) {
					auto refShapeSmall = nifSmall.FindBlockByName<NiShape>(refShape->name.get());
					if (refShapeSmall) {
						std::vector<Vector3> bodyVertsSmall;
						std::vector<Triangle> bodyTrisSmall;
						nifSmall.GetVertsForShape(refShapeSmall, bodyVertsSmall);
						refShapeSmall->GetTriangles(bodyTrisSmall);

						if (!bodyVertsSmall.empty() && !bodyTrisSmall.empty()) {
							for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
								auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
								if (shapeSmall == refShapeSmall || !ClippingFixer::IsEligibleForFix(nifSmall, shapeSmall))
									continue;

								std::vector<Vector3> outfitVerts;
								std::vector<Triangle> outfitTris;
								if (!nifSmall.GetVertsForShape(shapeSmall, outfitVerts))
									continue;
								shapeSmall->GetTriangles(outfitTris);

								ClippingFixer::FixClipping(bodyVertsSmall, bodyTrisSmall, outfitVerts, outfitTris, fixOpts);
								nifSmall.SetVertsForShape(shapeSmall, outfitVerts);
							}
						}
					}
				}
			}
		}
	}

	// Phase 3: Recalculate normals, tangents, and handle zapping
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		auto shape = nifBig.FindBlockByName<NiShape>(it->first);
		if (!shape)
			continue;

		if (!nifBig.GetVertsForShape(shape, vertsHigh))
			continue;

		if (!it->second.lockNormals) {
			nifBig.CalcNormalsForShape(shape, forceNormals, it->second.smoothSeamNormals);

			if (forceNormals)
				ApplyReferenceNormals(nifBig);
		}

		nifBig.CalcTangentsForShape(shape);

		auto zapIt = zapIdxAll.find(it->first);
		auto& shapeZapIdx = zapIt != zapIdxAll.end() ? zapIt->second : zapIdx;

		if (keepZappedShapes && shapeZapIdx.size() == vertsHigh.size()) {
			shape->flags |= 1; // Set hidden flag when shape would otherwise be fully zapped
		}
		else {
			if (nifBig.DeleteVertsForShape(shape, shapeZapIdx))
				nifBig.DeleteShape(shape); // Delete fully zapped shape
		}

		if (activeSet.GenWeights()) {
			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			if (!shapeSmall)
				continue;

			if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
				continue;

			if (!it->second.lockNormals) {
				nifSmall.CalcNormalsForShape(shapeSmall, forceNormals, it->second.smoothSeamNormals);

				if (forceNormals)
					ApplyReferenceNormals(nifSmall);
			}

			nifSmall.CalcTangentsForShape(shapeSmall);

			if (keepZappedShapes && shapeZapIdx.size() == vertsLow.size()) {
				shapeSmall->flags |= 1; // Set hidden flag when shape would otherwise be fully zapped
			}
			else {
				if (nifSmall.DeleteVertsForShape(shapeSmall, shapeZapIdx))
					nifSmall.DeleteShape(shapeSmall); // Delete fully zapped shape
			}
		}
	}

	bool triKeep = activeSet.PreventMorphFile();

	if (targetGame == SF) {
		/* Write Starfield morph.dat file */
		std::string sfMorphPath = activeSet.GetSFMorphPath();
		std::string sfMorphTargetShape = activeSet.GetSFMorphTargetShape();
		if (tri && !triKeep && !sfMorphPath.empty() && !sfMorphTargetShape.empty()) {
			std::string morphFolder = GetOutputDataPath() + sfMorphPath;
			WriteSFMorphFile(morphFolder, activeSet, nifBig, zapIdxAll);
		}
	}
	else {
		if (tri && !triKeep) {
			std::string triFilePath = outFileNameBig + ".tri";

			// TRI file already exists but isn't a body TRI file, don't overwrite!
			if (wxFileName::FileExists(wxString::FromUTF8(triFilePath)) && !IsBodyTriFile(triFilePath))
				triKeep = true;
		}

		/* Add TRI path for in-game morphs */
		if (tri && !triKeep) {
			std::string triPath = activeSet.GetOutputFilePath() + ".tri";
			std::string triPathTrimmed = triPath;
			// Replace multiple backslashes or forward slashes with one backslash
			triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex("/+|\\\\+"), "\\");

			// Remove everything before and including the meshes path
			triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex(".*meshes\\\\", std::regex_constants::icase), "");

			if (!WriteMorphTRI(outFileNameBig, activeSet, nifBig, zapIdxAll)) {
				wxLogError("Failed to write TRI file to '%s'!", triPath);
				wxMessageBox(wxString().Format(_("Failed to write TRI file to the following location\n\n%s"), triPath), _("Unable to process"), wxOK | wxICON_ERROR);
			}

			bool triToRoot = targetGame == FO4 || targetGame == FO4VR || targetGame == FO76;

			SetTriData(nifBig, triPathTrimmed, triToRoot);
			if (activeSet.GenWeights())
				SetTriData(nifSmall, triPathTrimmed, triToRoot);

			// Set all shapes to dynamic/mutable
			for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
				nifBig.SetShapeDynamic(it->first);
				if (activeSet.GenWeights())
					nifSmall.SetShapeDynamic(it->first);
			}
		}
		else if (!triKeep) {
			wxString triPath = wxString::FromUTF8(outFileNameBig + ".tri");
			if (IsBodyTriFile(triPath.ToUTF8().data()))
				wxRemoveFile(triPath);
		}
	}

	wxString savedLow;
	wxString savedHigh;
	wxString custName;
	bool useCustName = false;

	NifSaveOptions nifOptions;
	nifOptions.optimize = false;

	if (activeSet.GenWeights()) {
		outFileNameSmall += "_0.nif";
		outFileNameBig += "_1.nif";
		custName = wxString::FromUTF8(outFileNameSmall);
		savedLow = custName;

		std::fstream fileSmall;
		PlatformUtil::OpenFileStream(fileSmall, custName.ToUTF8().data(), std::ios::out | std::ios::binary);

		while (nifSmall.Save(fileSmall, nifOptions)) {
			wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
			wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

			custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
			if (custName.IsEmpty()) {
				wxLogMessage("Aborted build when choosing alternate file name.");
				return 4;
			}

			useCustName = true;
			savedLow = custName;

			PlatformUtil::OpenFileStream(fileSmall, custName.ToUTF8().data(), std::ios::out | std::ios::binary);
		}

		wxString custEnd;
		if (custName.EndsWith("_0.nif", &custEnd))
			custName = custEnd + "_1.nif";
		else
			custName.Empty();
	}
	else {
		outFileNameBig += ".nif";
		custName = wxString::FromUTF8(outFileNameBig);
	}

	if (!useCustName)
		outFileNameBig = custName.ToUTF8();

	savedHigh = custName;

	std::fstream fileBig;
	PlatformUtil::OpenFileStream(fileBig, custName.ToUTF8().data(), std::ios::out | std::ios::binary);

	while (nifBig.Save(fileBig, nifOptions)) {
		wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
		wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

		custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
		if (custName.IsEmpty()) {
			wxLogMessage("Aborted build when choosing alternate file name.");
			return 4;
		}

		useCustName = true;
		savedHigh = custName;

		PlatformUtil::OpenFileStream(fileBig, custName.ToUTF8().data(), std::ios::out | std::ios::binary);
	}

	wxString msg = _("Successfully processed the following files:\n");
	if (!savedLow.IsEmpty())
		msg.Append(savedLow += "\n");

	if (!savedHigh.IsEmpty())
		msg.Append(savedHigh);

	wxLogMessage("%s", msg);
	wxMessageBox(msg, _("Process Successful"));
	return 0;
}

int BodySlideApp::ShowBuildOverrideWithPreview(wxDialog* dlg, wxTreeListCtrl* treeListCtrl) {
	// Save the main app state so the conflicts preview doesn't corrupt it.
	// LoadProjects() -> CleanupPreview() -> AddProjectSliders() all operate on
	// shared app members (projects, sliderManager, multiProjectMode).
	PreviewPanel* savedPreview = preview;
	PreviewWindow* savedPreviewWindow = previewWindow;
	auto savedProjects = std::move(projects);
	bool savedMultiProjectMode = multiProjectMode;
	SliderManager savedSliderManager;
	std::swap(sliderManager, savedSliderManager);
	multiProjectMode = false;
	preview = nullptr;
	previewWindow = nullptr;

	PreviewWindow* conflictsPreviewWnd = nullptr;

	auto closeConflictsPreview = [&]() {
		if (!conflictsPreviewWnd)
			return;
		if (previewWindow == conflictsPreviewWnd)
			CleanupPreview();
		auto* wnd = conflictsPreviewWnd;
		conflictsPreviewWnd = nullptr;
		previewWindow = nullptr;
		preview = nullptr;
		wnd->Destroy();
	};

	wxButton* btnPreviewConflicts = XRCCTRL(*dlg, "btnPreviewConflicts", wxButton);
	if (btnPreviewConflicts) {
		btnPreviewConflicts->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxTreeListItem sel = treeListCtrl->GetSelection();
			if (!sel.IsOk())
				return;

			// Walk up to the group root (level 1) if a child is selected
			wxTreeListItem groupItem = sel;
			wxTreeListItem parent = treeListCtrl->GetItemParent(sel);
			if (parent.IsOk() && parent != treeListCtrl->GetRootItem())
				groupItem = parent;

			// Build preview entries from all children in this group
			std::vector<PreviewProjectEntry> entries;
			for (wxTreeListItem child = treeListCtrl->GetFirstChild(groupItem); child.IsOk(); child = treeListCtrl->GetNextSibling(child)) {
				std::string outfitName = treeListCtrl->GetItemText(child).ToUTF8().data();
				auto src = outfitNameSource.find(outfitName);
				if (src != outfitNameSource.end())
					entries.push_back({src->second, outfitName});
			}

			if (entries.empty())
				return;

			closeConflictsPreview();

			// Open a new standalone preview window
			wxSize previewSize = dlg->FromDIP(wxSize(800, 600));
			conflictsPreviewWnd = new PreviewWindow(wxDefaultPosition, previewSize, this);
			previewWindow = conflictsPreviewWnd;
			preview = conflictsPreviewWnd->GetPanel();

			// Handle user closing the preview window via X button.
			// Must cancel the async load thread before the panel is destroyed,
			// otherwise the CallAfter callback could target the wrong panel.
			conflictsPreviewWnd->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent&) {
				closeConflictsPreview();
			});

			wxString title = wxString::Format(_("Preview - %s"), wxString::FromUTF8(treeListCtrl->GetItemText(groupItem)));
			conflictsPreviewWnd->SetTitle(title);

			std::string baseGamePath = Config["GameDataPath"];
			preview->SetBaseDataPath(baseGamePath);
			preview->SetReadOnlyMode(true);
			preview->SetProjectData(entries, false, BodySlideConfig["SelectedPreset"]);
		});
	}

	int result = dlg->ShowModal();

	closeConflictsPreview();

	// Restore main app state
	previewWindow = savedPreviewWindow;
	preview = savedPreview;
	projects = std::move(savedProjects);
	multiProjectMode = savedMultiProjectMode;
	std::swap(sliderManager, savedSliderManager);

	return result;
}

int BodySlideApp::BuildListBodies(
	std::vector<std::string>& outfitList, std::map<std::string, std::string>& failedOutfits, bool clean, bool tri, bool forceNormals, const std::string& custPath) {
	std::string datapath = custPath;

	wxLogMessage("Started batch build with options: Custom Path = %s, Cleaning = %s, TRI = %s",
				 custPath.empty() ? "False" : custPath,
				 clean ? "True" : "False",
				 tri ? "True" : "False");

	if (clean) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?"),
							   _("Clean Batch Build"),
							   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning batch build.");
			return 1;
		}
	}

	std::string activePreset = BodySlideConfig["SelectedPreset"];

	if (datapath.empty()) {
		if (GetOutputDataPath().empty()) {
			if (clean) {
				wxLogError("Aborted batch clean with unconfigured data path. Files can't be removed that way.");
				wxMessageBox(_("WARNING: Game data path not configured. Files can't be removed that way."), _("Game not found"), wxOK | wxICON_ERROR);
				return 1;
			}
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Continue saving files to the working directory?"),
									   _("Game not found"),
									   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogError("Aborted batch build with unconfigured data path.");
					return 1;
				}
			}

			Config.SetValue("GameDataPath", Config["AppDir"] + PathSepStr);
		}

		datapath = GetOutputDataPath();
	}

	std::vector<std::string> outFileList;
	std::vector<wxArrayString> choicesList;
	for (auto& outFile : outFileCount) {
		if (outFile.second.size() > 1) {
			wxArrayString selOutfits;
			for (auto& outfit : outFile.second) {
				// Only if it's going to be batch built
				if (std::find(outfitList.begin(), outfitList.end(), outfit) != outfitList.end())
					selOutfits.Add(wxString::FromUTF8(outfit));
			}

			// Same file would not be written more than once
			if (selOutfits.size() <= 1)
				continue;

			outFileList.push_back(outFile.first);
			choicesList.push_back(selOutfits);
		}
	}

	if (!choicesList.empty()) {
		// Load BuildSelection file or create new one
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		wxXmlResource* rsrc = wxXmlResource::Get();
		wxDialog* dlgBuildOverride = rsrc->LoadDialog(sliderView, "dlgBuildOverride");
		dlgBuildOverride->SetSize(dlgBuildOverride->FromDIP(wxSize(800, 400)));
		dlgBuildOverride->SetSizeHints(dlgBuildOverride->FromDIP(wxSize(400, 400)), dlgBuildOverride->FromDIP(wxSize(-1, -1)));
		dlgBuildOverride->CenterOnParent();

		wxScrolledWindow* scrollOverrides = XRCCTRL(*dlgBuildOverride, "scrollOverrides", wxScrolledWindow);
		wxBoxSizer* choicesSizer = (wxBoxSizer*)scrollOverrides->GetSizer();

		// Create the treelist with checkbox support
		auto treeListCtrl = new wxTreeListCtrl(scrollOverrides, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE | wxTL_CHECKBOX | wxTL_3STATE);
		treeListCtrl->AppendColumn(_("Choice"), wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT);
		treeListCtrl->AppendColumn(_("Source File"), wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT);

		// Add root items directly under the (hidden) root
		for (size_t i = 0; i < choicesList.size(); i++) {
			auto& outFile = outFileList[i];
			auto& choices = choicesList[i];
			bool hasDefaultSet = false;

			wxTreeListItem rootItem = treeListCtrl->AppendItem(treeListCtrl->GetRootItem(), wxString::FromUTF8(outFile));
			treeListCtrl->CheckItem(rootItem, wxCheckBoxState::wxCHK_UNDETERMINED);

			// Add children with checkboxes
			for (size_t j = 0; j < choices.size(); j++) {
				wxString choice = choices[j];
				wxString defaultSet;

				// Check previous choices to see if radio button should be checked by default
				std::string outputChoice = buildSelection.GetOutputChoice(outFile);
				if (!outputChoice.empty()) {
					wxString c = wxString::FromUTF8(outputChoice);
					if (choices.Index(c) != wxNOT_FOUND) {
						defaultSet = c;
						hasDefaultSet = true;
					}
				}

				if (!hasDefaultSet && j == 0)
					defaultSet = choice;

				wxTreeListItem child = treeListCtrl->AppendItem(rootItem, choice);

				auto outfitSrc = outfitNameSource.find(choice.ToUTF8().data());
				if (outfitSrc != outfitNameSource.end()) {
					wxFileName outfitFileName(wxString::FromUTF8(outfitSrc->second));
					treeListCtrl->SetItemText(child, 1, outfitFileName.GetFullName());
				}
				else
					treeListCtrl->SetItemText(child, 1, _("<no source>"));

				if (!defaultSet.IsEmpty() && choice == defaultSet)
					treeListCtrl->CheckItem(child, wxCheckBoxState::wxCHK_CHECKED);
				else
					treeListCtrl->CheckItem(child, wxCheckBoxState::wxCHK_UNCHECKED);
			}

			treeListCtrl->Expand(rootItem);
		}

		bool checkBoxReverting = false;
		auto handler = [&](wxTreeListEvent& e) {
			if (checkBoxReverting) {
				e.Skip();
				return;
			}

			const wxTreeListItem item = e.GetItem();
			const wxTreeListItem parent = treeListCtrl->GetItemParent(item);

			// Level 1 items are direct children of the (hidden) root: make them non-checkable.
			if (parent == treeListCtrl->GetRootItem()) {
				checkBoxReverting = true;
				treeListCtrl->CheckItem(item, wxCheckBoxState::wxCHK_UNDETERMINED);
				checkBoxReverting = false;
				return;
			}

			// Only enforce the 'radio per level' rule for level-2 items:
			checkBoxReverting = true;

			auto checkedState = treeListCtrl->GetCheckedState(item);
			if (checkedState == wxCheckBoxState::wxCHK_CHECKED) {
				// Uncheck all siblings
				for (wxTreeListItem sib = treeListCtrl->GetFirstChild(parent); sib.IsOk(); sib = treeListCtrl->GetNextSibling(sib)) {
					if (sib != item && treeListCtrl->GetCheckedState(sib) == wxCheckBoxState::wxCHK_CHECKED)
						treeListCtrl->CheckItem(sib, wxCheckBoxState::wxCHK_UNCHECKED);
				}
			}
			else if (checkedState == wxCheckBoxState::wxCHK_UNCHECKED) {
				// Ensure at least one remains checked in this level
				bool anyChecked = false;
				for (wxTreeListItem sib = treeListCtrl->GetFirstChild(parent); sib.IsOk(); sib = treeListCtrl->GetNextSibling(sib)) {
					if (treeListCtrl->GetCheckedState(sib) == wxCheckBoxState::wxCHK_CHECKED) {
						anyChecked = true;
						break;
					}
				}

				if (!anyChecked) {
					// Re-check the one user tried to uncheck
					treeListCtrl->CheckItem(item, wxCheckBoxState::wxCHK_CHECKED);
				}
			}

			checkBoxReverting = false;
			e.Skip();
		};

		treeListCtrl->Bind(wxEVT_TREELIST_ITEM_CHECKED, handler);

		wxTextCtrl* chooseText = XRCCTRL(*dlgBuildOverride, "chooseText", wxTextCtrl);
		if (chooseText) {
			chooseText->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& WXUNUSED(event)) {
				wxString text = chooseText->GetValue().MakeLower();
				if (!text.IsEmpty()) {
					wxTreeListItem root = treeListCtrl->GetRootItem();

					for (wxTreeListItem level1 = treeListCtrl->GetFirstChild(root); level1.IsOk(); level1 = treeListCtrl->GetNextSibling(level1)) {
						wxTreeListItem firstMatch;

						// Find first matching item
						for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
							wxString label = treeListCtrl->GetItemText(level2).Lower();

							if (label.Contains(text)) {
								firstMatch = level2;
								break;
							}
						}

						if (firstMatch.IsOk()) {
							// Check the matched item
							treeListCtrl->CheckItem(firstMatch, wxCheckBoxState::wxCHK_CHECKED);

							// Uncheck all siblings except the matched one
							for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
								if (level2 != firstMatch)
									treeListCtrl->CheckItem(level2, wxCheckBoxState::wxCHK_UNCHECKED);
							}
						}
						// else: no match found, do nothing / keep existing checks
					}
				}
			});
		}

		choicesSizer->Add(treeListCtrl, 1, wxEXPAND, 0);
		scrollOverrides->FitInside();

		if (ShowBuildOverrideWithPreview(dlgBuildOverride, treeListCtrl) == wxID_CANCEL) {
			wxLogMessage("Aborted batch build by not choosing a file override.");
			delete dlgBuildOverride;
			return 1;
		}

		wxTreeListItem root = treeListCtrl->GetRootItem();

		// Iterate level 1 roots (should correspond to choicesList size)
		size_t index = 0;
		for (wxTreeListItem level1 = treeListCtrl->GetFirstChild(root); level1.IsOk() && index < choicesList.size(); level1 = treeListCtrl->GetNextSibling(level1), ++index) {
			wxString checkedItemText;

			// Find the checked child (level 2)
			for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
				if (treeListCtrl->GetCheckedState(level2) == wxCheckBoxState::wxCHK_CHECKED) {
					checkedItemText = treeListCtrl->GetItemText(level2);
					break; // assuming only one checked per level 1
				}
			}

			if (!checkedItemText.IsEmpty()) {
				wxString choiceSel = checkedItemText;

				// Add output choice to file
				buildSelection.SetOutputChoice(outFileList[index], choiceSel.ToUTF8().data());

				// Remove the selected choice from choicesList[i]
				choicesList[index].Remove(choiceSel);

				// Remove from outfitList all outfits in choicesList[index]
				for (auto& outfit : choicesList[index]) {
					auto result = std::find(outfitList.begin(), outfitList.end(), outfit.ToStdString());
					if (result != outfitList.end())
						outfitList.erase(result);
				}
			}
		}


		delete dlgBuildOverride;

		// Save output choices to file
		buildSelFile.UpdateOutputChoices(buildSelection);
		buildSelFile.Save();
	}

	refNormalsCache.clear();

	wxProgressDialog progWnd(_("Processing Outfits"), _("Starting..."), 1000, sliderView, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
	progWnd.SetSize(400, 150);
	float progstep = 1000.0f / outfitList.size();
	std::atomic<int> count = 0;
	std::unordered_map<std::string, std::string> failedOutfitsCon;
	std::mutex batchBuildMutex;
	std::mutex outputDirectoryMutex;

	auto recordFailure = [&](const std::string& outfit, const auto& message) {
		std::lock_guard<std::mutex> lock(batchBuildMutex);
		failedOutfitsCon[outfit] = message;
	};

	auto outputDirectoryExists = [](const wxString& dir) {
		wxFileName dirName;
		dirName.AssignDir(dir);
		return dirName.DirExists();
	};

	auto ensureOutputDirectory = [&](const wxString& dir) {
		if (outputDirectoryExists(dir))
			return true;

		std::lock_guard<std::mutex> lock(outputDirectoryMutex);
		if (outputDirectoryExists(dir))
			return true;

		if (wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
			return true;

		return outputDirectoryExists(dir);
	};

	auto buildOutfit = [&](const std::string& outfit) {
		int processedCount = ++count;
		wxString progMsg = wxString::Format(_("Processing '%s' (%d of %d)..."), wxString::FromUTF8(outfit), processedCount, (int)outfitList.size());
		{
			std::lock_guard<std::mutex> lock(batchBuildMutex);
			progWnd.Update((int)(processedCount * progstep) - 1, progMsg);
			progWnd.Fit();
			wxLogMessage(progMsg);
		}

		/* Load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			recordFailure(outfit, _("No recorded outfit name source"));
			return;
		}

		SliderSet currentSet;
		DiffDataSets currentDiffs;

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			if (sliderDoc.GetSet(outfit, currentSet)) {
				recordFailure(outfit, _("Unable to get slider set from file: ") + outfitNameSource[outfit]);
				return;
			}
		}
		else {
			recordFailure(outfit, _("Unable to open slider set file: ") + outfitNameSource[outfit]);
			return;
		}

		currentSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

		// ALT key
		if (clean && custPath.empty()) {
			bool genWeights = currentSet.GenWeights();

			wxString removePath = wxString::FromUTF8(datapath + currentSet.GetOutputFilePath());
			wxString removeHigh = removePath + ".nif";
			if (genWeights)
				removeHigh = removePath + "_1.nif";

			if (wxFileName::FileExists(removeHigh))
				wxRemoveFile(removeHigh);

			if (!genWeights)
				return;

			wxString removeLow = removePath + "_0.nif";
			if (wxFileName::FileExists(removeLow))
				wxRemoveFile(removeLow);

			return;
		}

		/* Load input NIFs */
		std::fstream file;
		PlatformUtil::OpenFileStream(file, currentSet.GetInputFileName(), std::ios::in | std::ios::binary);

		NifFile nifBig;
		NifFile nifSmall;
		if (nifBig.Load(file)) {
			recordFailure(outfit, _("Unable to load input nif: ") + currentSet.GetInputFileName());
			return;
		}

		if (currentSet.GenWeights())
			nifSmall.CopyFrom(nifBig);

		currentSet.LoadSetDiffData(currentDiffs);

		// Load BuildSelection file for zap choices
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		bool keepZappedShapes = currentSet.KeepZappedShapes();

		/* Shape the NIF files */
		std::vector<Vector3> vertsLow;
		std::vector<Vector3> vertsHigh;
		std::vector<Vector2> uvsLow;
		std::vector<Vector2> uvsHigh;
		std::vector<int> clamps;
		std::vector<uint16_t> zapIdx;
		std::unordered_map<std::string, std::vector<uint16_t>> zapIdxAll;

		for (size_t s = 0; s < currentSet.size(); s++) {
			std::string name = currentSet[s].name;

			if (currentSet[s].bClamp) {
				clamps.push_back(s);
				continue;
			}

			if (currentSet[s].bZap && !currentSet[s].bUV) {
				float vbig = sliderManager.GetBigPresetValue(activePreset, name, currentSet[s].defBigValue / 100.0f);
				
				/*
				* Note: skip applying sliderManager values on zaps in batch mode. 
				* If a set has a zap with the same name as the set loaded in the UI,
				* its value could be overwritten by the UI value.
				* Only BuildSelection is a reliable source for zap values in batch mode.
				*/

				if (!currentSet[s].bHidden) {
					// Apply stored zap choice for zaps visible to the user
					if (buildSelection.HasZapChoice(currentSet.GetName(), name)) {
						bool zapChoice = buildSelection.GetZapChoice(currentSet.GetName(), name);
						vbig = zapChoice ? 1.0f : 0.0f;
					}
				}

				// Apply zap toggles if zap is not in default state
				if (vbig != currentSet[s].defBigValue / 100.0f) {
					for (auto& zapToggle : currentSet[s].zapToggles) {
						// Toggled zap default values are read in later code if no preset overwrites it
						auto& slider = currentSet[zapToggle];
						slider.defBigValue = 100.0f - slider.defBigValue;
						slider.defSmallValue = 100.0f - slider.defSmallValue;
					}
				}
			}
		}

		// Phase 1: Apply sliders and set vertices for all shapes
		for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
			auto shape = nifBig.FindBlockByName<NiShape>(it->first);
			if (!nifBig.GetVertsForShape(shape, vertsHigh))
				continue;

			nifBig.GetUvsForShape(shape, uvsHigh);

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
					continue;

				nifSmall.GetUvsForShape(shapeSmall, uvsLow);
			}

			float vbig = 0.0f;
			float vsmall = 0.0f;
			zapIdxAll.emplace(it->first, std::vector<uint16_t>());

			for (size_t s = 0; s < currentSet.size(); s++) {
				std::string name = currentSet[s].name;
				std::string target = it->second.targetShape;
				std::string dn = currentSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				vbig = sliderManager.GetBigPresetValue(activePreset, name, currentSet[s].defBigValue / 100.0f);
				for (auto& sliderBig : sliderManager.slidersBig) {
					if (sliderBig.name == name && sliderBig.changed && !sliderBig.clamp && !currentSet[s].bZap) {
						vbig = sliderBig.value;
						break;
					}
				}

				if (currentSet.GenWeights()) {
					vsmall = sliderManager.GetSmallPresetValue(activePreset, name, currentSet[s].defSmallValue / 100.0f);
					for (auto& sliderSmall : sliderManager.slidersSmall) {
						if (sliderSmall.name == name && sliderSmall.changed && !sliderSmall.clamp && !currentSet[s].bZap) {
							vsmall = sliderSmall.value;
							break;
						}
					}
				}

				if (currentSet[s].bInvert) {
					vbig = 1.0f - vbig;
					if (currentSet.GenWeights())
						vsmall = 1.0f - vsmall;
				}

				if (currentSet[s].bZap && !currentSet[s].bUV) {
					if (!currentSet[s].bHidden) {
						// Apply stored zap choice for zaps visible to the user
						if (buildSelection.HasZapChoice(currentSet.GetName(), name)) {
							bool zapChoice = buildSelection.GetZapChoice(currentSet.GetName(), name);
							vbig = zapChoice ? 1.0f : 0.0f;
						}
					}

					if (vbig > 0.0f) {
						currentDiffs.GetDiffIndices(dn, target, zapIdx);
						zapIdxAll[it->first] = zapIdx;
					}
					continue;
				}

				if (currentSet[s].bUV)
					currentDiffs.ApplyUVDiff(dn, target, vbig, &uvsHigh);
				else
					currentDiffs.ApplyDiff(dn, target, vbig, &vertsHigh);

				if (currentSet.GenWeights()) {
					if (currentSet[s].bUV)
						currentDiffs.ApplyUVDiff(dn, target, vsmall, &uvsLow);
					else
						currentDiffs.ApplyDiff(dn, target, vsmall, &vertsLow);
				}
			}

			if (!clamps.empty()) {
				for (auto& c : clamps) {
					std::string dn = currentSet[c].TargetDataName(it->second.targetShape);
					std::string target = it->second.targetShape;
					if (currentSet[c].defBigValue > 0)
						currentDiffs.ApplyClamp(dn, target, &vertsHigh);

					if (currentSet.GenWeights())
						if (currentSet[c].defSmallValue > 0)
							currentDiffs.ApplyClamp(dn, target, &vertsLow);
				}
			}

			nifBig.SetVertsForShape(shape, vertsHigh);
			nifBig.SetUvsForShape(shape, uvsHigh);

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				nifSmall.SetVertsForShape(shapeSmall, vertsLow);
				nifSmall.SetUvsForShape(shapeSmall, uvsLow);
			}

			zapIdx.clear();
		}

		// Phase 2: Apply clipping fix when strength is above zero
		if (clippingFixStrength > 0.0f) {
			auto refShape = ClippingFixer::FindReferenceShape(nifBig);
			if (refShape) {
				std::vector<Vector3> bodyVerts;
				std::vector<Triangle> bodyTris;
				nifBig.GetVertsForShape(refShape, bodyVerts);
				refShape->GetTriangles(bodyTris);

				if (!bodyVerts.empty() && !bodyTris.empty()) {
					ClippingFixOptions fixOpts;
					fixOpts.strength = clippingFixStrength / 100.0f;

					for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
						auto shape = nifBig.FindBlockByName<NiShape>(it->first);
						if (shape == refShape || !ClippingFixer::IsEligibleForFix(nifBig, shape))
							continue;

						std::vector<Vector3> outfitVerts;
						std::vector<Triangle> outfitTris;
						if (!nifBig.GetVertsForShape(shape, outfitVerts))
							continue;
						shape->GetTriangles(outfitTris);

						ClippingFixer::FixClipping(bodyVerts, bodyTris, outfitVerts, outfitTris, fixOpts);
						nifBig.SetVertsForShape(shape, outfitVerts);
					}

					if (currentSet.GenWeights()) {
						auto refShapeSmall = nifSmall.FindBlockByName<NiShape>(refShape->name.get());
						if (refShapeSmall) {
							std::vector<Vector3> bodyVertsSmall;
							std::vector<Triangle> bodyTrisSmall;
							nifSmall.GetVertsForShape(refShapeSmall, bodyVertsSmall);
							refShapeSmall->GetTriangles(bodyTrisSmall);

							if (!bodyVertsSmall.empty() && !bodyTrisSmall.empty()) {
								for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
									auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
									if (shapeSmall == refShapeSmall || !ClippingFixer::IsEligibleForFix(nifSmall, shapeSmall))
										continue;

									std::vector<Vector3> outfitVerts;
									std::vector<Triangle> outfitTris;
									if (!nifSmall.GetVertsForShape(shapeSmall, outfitVerts))
										continue;
									shapeSmall->GetTriangles(outfitTris);

									ClippingFixer::FixClipping(bodyVertsSmall, bodyTrisSmall, outfitVerts, outfitTris, fixOpts);
									nifSmall.SetVertsForShape(shapeSmall, outfitVerts);
								}
							}
						}
					}
				}
			}
		}

		// Phase 3: Recalculate normals, tangents, and handle zapping
		for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
			auto shape = nifBig.FindBlockByName<NiShape>(it->first);
			if (!shape)
				continue;

			if (!nifBig.GetVertsForShape(shape, vertsHigh))
				continue;

			if (!it->second.lockNormals) {
				nifBig.CalcNormalsForShape(shape, forceNormals, it->second.smoothSeamNormals);

				if (forceNormals)
					ApplyReferenceNormals(nifBig);
			}

			nifBig.CalcTangentsForShape(shape);

			auto zapIt = zapIdxAll.find(it->first);
			auto& shapeZapIdx = zapIt != zapIdxAll.end() ? zapIt->second : zapIdx;

			if (keepZappedShapes && shapeZapIdx.size() == vertsHigh.size()) {
				shape->flags |= 1; // Set hidden flag when shape would otherwise be fully zapped
			}
			else {
				if (nifBig.DeleteVertsForShape(shape, shapeZapIdx))
					nifBig.DeleteShape(shape); // Delete fully zapped shape
			}

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				if (!shapeSmall)
					continue;

				if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
					continue;

				if (!it->second.lockNormals) {
					nifSmall.CalcNormalsForShape(shapeSmall, forceNormals, it->second.smoothSeamNormals);

					if (forceNormals)
						ApplyReferenceNormals(nifSmall);
				}

				nifSmall.CalcTangentsForShape(shapeSmall);

				if (keepZappedShapes && shapeZapIdx.size() == vertsLow.size()) {
					shapeSmall->flags |= 1; // Set hidden flag when shape would otherwise be fully zapped
				}
				else {
					if (nifSmall.DeleteVertsForShape(shapeSmall, shapeZapIdx))
						nifSmall.DeleteShape(shapeSmall); // Delete fully zapped shape
				}
			}
		}

		currentDiffs.Clear();

		/* Create directory for the outfit */
		wxString dir = wxString::FromUTF8(datapath + currentSet.GetOutputPath());
		bool success = ensureOutputDirectory(dir);

		if (!success) {
			recordFailure(outfit, _("Unable to create destination directory: ") + dir.ToUTF8().data());
			return;
		}

		std::string outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		std::string outFileNameBig = outFileNameSmall;

		bool triKeep = currentSet.PreventMorphFile();

		if (targetGame == SF) {
			/* Write Starfield morph.dat file */
			std::string sfMorphPath = currentSet.GetSFMorphPath();
			std::string sfMorphTargetShape = currentSet.GetSFMorphTargetShape();
			if (tri && !triKeep && !sfMorphPath.empty() && !sfMorphTargetShape.empty()) {
				std::string morphFolder = datapath + sfMorphPath;
				WriteSFMorphFile(morphFolder, currentSet, nifBig, zapIdxAll);
			}
		}
		else {
			if (tri && !triKeep) {
				std::string triFilePath = outFileNameBig + ".tri";

				// TRI file already exists but isn't a body TRI file, don't overwrite!
				if (wxFileName::FileExists(wxString::FromUTF8(triFilePath)) && !IsBodyTriFile(triFilePath))
					triKeep = true;
			}

			/* Add TRI path for in-game morphs */
			if (tri && !triKeep) {
				std::string triPath = currentSet.GetOutputFilePath() + ".tri";
				std::string triPathTrimmed = triPath;
				triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex("/+|\\\\+"),
													"\\"); // Replace multiple backslashes or forward slashes with one backslash
				triPathTrimmed = std::regex_replace(triPathTrimmed,
													std::regex(".*meshes\\\\", std::regex_constants::icase),
													""); // Remove everything before and including the meshes path

				if (!WriteMorphTRI(outFileNameBig, currentSet, nifBig, zapIdxAll))
					wxLogError("Failed to create TRI file to '%s'!", triPath);

				bool triToRoot = targetGame == FO4 || targetGame == FO4VR || targetGame == FO76;

				SetTriData(nifBig, triPathTrimmed, triToRoot);
				if (currentSet.GenWeights())
					SetTriData(nifSmall, triPathTrimmed, triToRoot);

				// Set all shapes to dynamic/mutable
				for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
					nifBig.SetShapeDynamic(it->first);
					if (currentSet.GenWeights())
						nifSmall.SetShapeDynamic(it->first);
				}
			}
			else if (!triKeep) {
				std::string triPath = outFileNameBig + ".tri";
				if (IsBodyTriFile(triPath))
					wxRemoveFile(triPath);
			}
		}

		NifSaveOptions nifOptions;
		nifOptions.optimize = false;

		/* Set filenames for the outfit */
		if (currentSet.GenWeights()) {
			outFileNameSmall += "_0.nif";
			outFileNameBig += "_1.nif";

			std::fstream fileBig;
			PlatformUtil::OpenFileStream(fileBig, outFileNameBig, std::ios::out | std::ios::binary);

			if (nifBig.Save(fileBig, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameBig);
				return;
			}

			std::fstream fileSmall;
			PlatformUtil::OpenFileStream(fileSmall, outFileNameSmall, std::ios::out | std::ios::binary);

			if (nifSmall.Save(fileSmall, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameSmall);
				return;
			}
		}
		else {
			outFileNameBig += ".nif";

			std::fstream fileBig;
			PlatformUtil::OpenFileStream(fileBig, outFileNameBig, std::ios::out | std::ios::binary);

			if (nifBig.Save(fileBig, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameBig);
				return;
			}
		}
	};

	// Multi-threading for 64-bit only due to memory limits of 32-bit builds
	if (sizeof(void*) >= 8 && outfitList.size() > 1) {
		std::atomic<bool> buildDone = false;
		std::thread buildTask([&] {
			ParallelForDynamic(outfitList.size(), 1, 1, [&](size_t startIndex, size_t endIndex) {
				for (size_t outfitIndex = startIndex; outfitIndex < endIndex; outfitIndex++)
					buildOutfit(outfitList[outfitIndex]);
			});
			buildDone = true;
		});

		while (!buildDone) {
			Yield();
			wxMilliSleep(100);
		}

		buildTask.join();
	}
	else {
		for (auto& outfit : outfitList)
			buildOutfit(outfit);
	}

	progWnd.Update(1000);

	failedOutfits.insert(failedOutfitsCon.begin(), failedOutfitsCon.end());

	if (failedOutfits.size() > 0)
		return 3;

	return 0;
}

std::vector<std::string> BodySlideApp::GetCmdLineBuildOutfits(std::map<std::string, std::string>& failedOutfits) {
	// Collected case insensitively, so the same outfit selected by several options is only built once.
	std::set<std::string, case_insensitive_compare> selected;

	// The groups and the filter narrow the outfit list down together, the same way the group filter
	// and the outfit filter box do in the GUI. Outfits named with the build option are added on top.
	if (!cmdGroupBuild.empty() || !cmdBuildFilter.empty()) {
		std::vector<std::string> matches = outfitNameOrder;

		if (!cmdGroupBuild.empty()) {
			matches.clear();

			for (auto& no : outfitNameOrder) {
				std::vector<std::string> groups;
				gCollection.GetOutfitGroups(no, groups);

				for (auto& g : groups) {
					if (std::find(cmdGroupBuild.begin(), cmdGroupBuild.end(), g) != cmdGroupBuild.end()) {
						matches.push_back(no);
						break;
					}
				}
			}

			wxLogMessage("Command-line build: %d outfit(s) belong to the specified group(s).", (int)matches.size());

			if (matches.empty())
				failedOutfits["--groupbuild"] = _("No outfit belongs to any of the specified groups.").ToUTF8().data();
		}

		if (!cmdBuildFilter.empty() && !matches.empty()) {
			std::string regexError;
			matches = FilterOutfitNames(matches, cmdBuildFilter, cmdBuildFilterRegex, &regexError);

			if (!regexError.empty()) {
				wxLogError("Invalid regular expression '%s' from the command line: %s", cmdBuildFilter, regexError);
				failedOutfits[cmdBuildFilter] = wxString::Format(_("Invalid regular expression: %s"), regexError).ToUTF8().data();
			}
			else {
				wxLogMessage("Command-line build: %d outfit(s) match the filter '%s'.", (int)matches.size(), cmdBuildFilter);

				if (matches.empty())
					failedOutfits[cmdBuildFilter] = _("Filter doesn't match any outfit.").ToUTF8().data();
			}
		}

		selected.insert(matches.begin(), matches.end());
	}

	for (auto& outfitName : cmdBuildOutfits) {
		auto sourceIt = outfitNameSource.find(outfitName);
		if (sourceIt == outfitNameSource.end()) {
			wxLogError("Outfit '%s' from the command line doesn't exist.", outfitName);
			failedOutfits[outfitName] = _("Outfit doesn't exist.").ToUTF8().data();
			continue;
		}

		// Insert the name as it's defined in the slider set, not as it was spelled on the command line.
		selected.insert(sourceIt->first);
	}

	// Built in the order the outfits were loaded in, no matter which option selected them.
	std::vector<std::string> outfits;
	for (auto& no : outfitNameOrder)
		if (selected.find(no) != selected.end())
			outfits.push_back(no);

	return outfits;
}

void BodySlideApp::CommandLineBuild() {
	std::map<std::string, std::string> failedOutfits;
	std::vector<std::string> outfits = GetCmdLineBuildOutfits(failedOutfits);

	// Loaded before the preset folder so that a preset file given on the command line wins on name conflicts.
	LoadCmdPresetFile();

	std::string preset;
	bool presetOverride = !cmdPreset.empty();
	if (presetOverride) {
		preset = BodySlideConfig["SelectedPreset"];
		BodySlideConfig.SetValue("SelectedPreset", cmdPreset);
	}

	std::vector<std::string> groups;
	sliderManager.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", "", groups, true);

	// Apply saved build selections for command-line builds before entering batch build conflict handling.
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	for (auto& outFile : outFileCount) {
		if (outFile.second.size() <= 1)
			continue;

		std::vector<std::string> outfitsInBuild;
		for (auto& outfit : outFile.second) {
			if (std::find(outfits.begin(), outfits.end(), outfit) != outfits.end())
				outfitsInBuild.push_back(outfit);
		}

		if (outfitsInBuild.size() <= 1)
			continue;

		std::string outputChoice = buildSelection.GetOutputChoice(outFile.first);
		if (outputChoice.empty())
			continue;

		if (std::find(outfitsInBuild.begin(), outfitsInBuild.end(), outputChoice) == outfitsInBuild.end())
			continue;

		int removedChoices = 0;

		for (auto& outfit : outfitsInBuild) {
			if (outfit == outputChoice)
				continue;

			auto result = std::find(outfits.begin(), outfits.end(), outfit);
			if (result != outfits.end()) {
				outfits.erase(result);
				removedChoices++;
			}
		}

		if (removedChoices > 0) {
			wxLogMessage("Command-line build applied saved BuildSelection for output '%s': selected '%s', skipped %d conflicting choice(s).",
						 outFile.first,
						 outputChoice,
						 removedChoices);
		}
	}

	int ret = BuildListBodies(outfits, failedOutfits, false, cmdTri, false, cmdTargetDir);

	if (presetOverride)
		BodySlideConfig.SetValue("SelectedPreset", preset);

	wxLog::FlushActive();

	if (ret == 0) {
		wxLogMessage("All command-line build sets processed successfully!");
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto& e : failedOutfits) {
			wxString ename = wxString::FromUTF8(e.first);
			wxLogError("Failed to build '%s': %s", ename, e.second);
			errlist.Add(ename + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(sliderView, _("The following sets failed"), _("Failed"), errlist, nullptr, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
		errdisplay.ShowModal();
	}

	sliderView->Close(true);
}

void BodySlideApp::SetTriData(NifFile& nif, const std::string& triPath, bool toRoot) {
	auto& hdr = nif.GetHeader();

	// Get rid of every BODYTRI block in the file, no matter where it's attached
	std::vector<uint32_t> obsoleteIds;

	for (uint32_t id = 0; id < hdr.GetNumBlocks(); id++) {
		auto stringExtraData = hdr.GetBlock<NiStringExtraData>(id);
		if (stringExtraData && stringExtraData->name.get() == "BODYTRI")
			obsoleteIds.push_back(id);
	}

	// Delete the highest block ids first to keep the remaining ones valid
	for (auto it = obsoleteIds.rbegin(); it != obsoleteIds.rend(); ++it)
		hdr.DeleteBlock(*it);

	// Fallout reads the path from the root node, the other games from a shape
	NiAVObject* target = nullptr;

	if (toRoot) {
		target = nif.GetRootNode();
	}
	else {
		for (auto& shape : nif.GetShapes()) {
			if (shape->GetNumVertices() > 0) {
				target = shape;
				break;
			}
		}
	}

	if (!target)
		return;

	auto triExtraData = std::make_unique<NiStringExtraData>();
	triExtraData->name.get() = "BODYTRI";
	triExtraData->stringData.get() = triPath;
	nif.AssignExtraData(target, std::move(triExtraData));
}

float BodySlideApp::GetSliderValue(const wxString& sliderName, bool isLo) {
	std::string sstr{sliderName.ToUTF8()};
	return sliderManager.GetSlider(sstr, isLo);
}

bool BodySlideApp::IsUVSlider(const wxString& sliderName) {
	if (projects.empty())
		return false;

	std::string sstr{sliderName.ToUTF8()};
	return GetActiveSet()[sstr].bUV;
}

std::vector<std::string> BodySlideApp::GetSliderZapToggles(const wxString& sliderName) {
	return sliderManager.GetSliderZapToggles(sliderName.ToUTF8().data());
}

void BodySlideApp::SetSliderValue(const wxString& sliderName, bool isLo, float val) {
	std::string sstr{sliderName.ToUTF8()};
	sliderManager.SetSlider(sstr, isLo, val);
}

void BodySlideApp::SetSliderChanged(const wxString& sliderName, bool isLo) {
	std::string sstr{sliderName.ToUTF8()};
	sliderManager.SetChanged(sstr, isLo);
}

int BodySlideApp::UpdateSliderPositions(const std::string& presetName) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	std::vector<std::string> groups;

	std::string outputFile = sliderManager.GetPresetFileNames(presetName);
	if (outputFile.empty())
		return -2;

	sliderManager.GetPresetGroups(presetName, groups);

	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

int BodySlideApp::SaveSliderPositions(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

int BodySlideApp::SavePresetGroups(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	return sliderManager.SavePresetGroups(outputFile, presetName, outfitName, groups);
}

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

	outfitChoice = (wxChoice*)FindWindowByName("outfitChoice", this);
	presetChoice = (wxChoice*)FindWindowByName("presetChoice", this);
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

void BodySlideFrame::OnEnterSliderWindow(wxMouseEvent& event) {
	if (this->IsActive()) {
		if (!this->FindFocus()->IsKindOf(wxClassInfo::FindClass("wxTextCtrl")) &&
			!this->FindFocus()->IsKindOf(wxClassInfo::FindClass("wxSearchCtrl"))) {
			wxScrolledWindow* sw = (wxScrolledWindow*)event.GetEventObject();
			sw->SetFocusIgnoringChildren();
		}
	}
}

void BodySlideFrame::HideSlider(SliderDisplay* slider) {
	if (!slider)
		return;

	slider->zapCheckLo->Unbind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	slider->sliderLo->Unbind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	slider->sliderReadoutLo->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	slider->zapCheckHi->Unbind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	slider->sliderHi->Unbind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	slider->sliderReadoutHi->Disconnect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	slider->Show(false);
}

void BodySlideFrame::ShowLowColumn(bool show) {
	if (show) {
		XRCCTRL(*this, "lblLowWt", wxStaticText)->Show();
		XRCCTRL(*this, "lblHighWt", wxStaticText)->Show();
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Hide();
		XRCCTRL(*this, "btnLowToHigh", wxButton)->Show();
		XRCCTRL(*this, "btnHighToLow", wxButton)->Show();
		sliderLayout->SetCols(6);
	}
	else {
		XRCCTRL(*this, "lblLowWt", wxStaticText)->Hide();
		XRCCTRL(*this, "lblHighWt", wxStaticText)->Hide();
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Show();
		XRCCTRL(*this, "btnLowToHigh", wxButton)->Hide();
		XRCCTRL(*this, "btnHighToLow", wxButton)->Hide();
		sliderLayout->SetCols(3);
	}
}

void BodySlideFrame::AddCategorySliderUI(const std::string& name, const std::vector<std::string>& sliders, bool enabled, bool oneSize) {
	SliderCategoryUI* cat = new SliderCategoryUI();

	if (!cat->Create(sliderScroll, sliderLayout, categoryTabSizer, name, sliders, enabled, oneSize))
		return;

	cat->check->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnCategoryCheckChanged, this);

	if (cat->tabButton)
		cat->tabButton->Bind(wxEVT_BUTTON, &BodySlideFrame::OnCategoryTabButton, this);

	if (!cat->isShown)
		cat->Show();

	sliderCategories[cat->categoryName] = cat;
}

void BodySlideFrame::AddSliderGUI(const std::string& name, const std::string& display, const std::string& category, bool isZap, bool oneSize) {
	SliderDisplay* sd = sliderPool.GetNext();
	if (!sd)
		return;

	int minValue = Config.GetIntValue("Input/SliderMinimum");
	int maxValue = Config.GetIntValue("Input/SliderMaximum");

	if (!sd->Create(sliderScroll, sliderLayout, name, display, category, minValue, maxValue, isZap, oneSize))
		return;

	sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
	sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
	sd->sliderReadoutHi->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);

	if (!sd->isShown)
		sd->Show();

	sliderDisplays[sd->sliderName] = sd;
}

void BodySlideFrame::ClearPresetList() {
	presetChoiceNames.clear();
	if (presetChoice)
		presetChoice->Clear();

	UpdateFavoriteButtons();
}

void BodySlideFrame::ClearOutfitList() {
	outfitChoiceNames.clear();
	if (outfitChoice)
		outfitChoice->Clear();

	UpdateFavoriteButtons();
}

void BodySlideFrame::ClearSliderGUI() {
	for (auto& sd : sliderDisplays)
		HideSlider(sd.second);

	sliderScroll->GetSizer()->Clear();

	for (auto& cat : sliderCategories) {
		cat.second->Destroy();
		delete cat.second;
	}

	sliderDisplays.clear();
	sliderCategories.clear();
}

void BodySlideFrame::SetPresetChanged(bool changed) {
	if (btnSavePreset)
		btnSavePreset->Enable(changed);
}

wxString BodySlideFrame::FavoriteChoiceLabel(const std::string& name, bool favorite) const {
	wxString label = wxString::FromUTF8(name);
	if (!favorite || label.empty())
		return label;

	return wxString::FromUTF8(FavoriteStar) + " " + label;
}

bool BodySlideFrame::SelectChoiceName(wxChoice* choice, const std::vector<std::string>& names, const std::string& selectItem) const {
	for (size_t i = 0; i < names.size(); i++) {
		if (names[i] == selectItem) {
			choice->SetSelection(i);
			return true;
		}
	}

	return false;
}

void BodySlideFrame::SetFavoriteButtonBitmap(wxButton* button, bool favorite) const {
	if (!button)
		return;

	wxString iconPath = wxString::FromUTF8(Config["AppDir"]) + wxString::FromUTF8(favorite ? FavoriteStarIcon : FavoriteStarEmptyIcon);
	wxBitmap bitmap(iconPath, wxBITMAP_TYPE_PNG);
	if (bitmap.IsOk()) {
		button->SetLabel("");
		button->SetBitmap(bitmap);
	}
}

void BodySlideFrame::PopulateOutfitList(const wxArrayString& items, const wxString& selectItem) {
	if (!outfitChoice)
		return;

	outfitChoiceNames.clear();

	for (size_t i = 0; i < items.GetCount(); i++) {
		std::string rawName = items[i].ToUTF8().data();
		outfitChoiceNames.push_back(rawName);
	}

	RebuildOutfitChoice(selectItem.ToUTF8().data());
}

void BodySlideFrame::RebuildOutfitChoice(const std::string& selectItem) {
	if (!outfitChoice)
		return;

	std::string selectedName = selectItem;
	if (selectedName.empty())
		selectedName = GetSelectedOutfitName();

	app->SortOutfitNamesForDisplay(outfitChoiceNames);

	populatingChoices = true;
	wxEventBlocker blocker(outfitChoice, wxEVT_CHOICE);
	outfitChoice->Freeze();
	outfitChoice->Clear();

	for (const auto& rawName : outfitChoiceNames)
		outfitChoice->Append(FavoriteChoiceLabel(rawName, app->IsFavoriteOutfit(rawName)));

	if (!SelectChoiceName(outfitChoice, outfitChoiceNames, selectedName)) {
		int i = wxNOT_FOUND;
		wxString missingItem = wxString::FromUTF8(selectedName);
		if (missingItem.empty()) {
			outfitChoiceNames.push_back("");
			i = outfitChoice->Append("");
		}
		else if (!missingItem.StartsWith("[")) {
			outfitChoiceNames.push_back(selectedName);
			i = outfitChoice->Append("[" + missingItem + "]");
		}
		else {
			outfitChoiceNames.push_back(selectedName);
			i = outfitChoice->Append(missingItem);
		}

		outfitChoice->SetSelection(i);
	}

	outfitChoice->Thaw();
	populatingChoices = false;
	UpdateFavoriteButtons();
}

void BodySlideFrame::PopulatePresetList(const wxArrayString& items, const wxString& selectItem) {
	if (!presetChoice)
		return;

	presetChoiceNames.clear();

	for (size_t i = 0; i < items.GetCount(); i++) {
		std::string rawName = items[i].ToUTF8().data();
		presetChoiceNames.push_back(rawName);
	}

	RebuildPresetChoice(selectItem.ToUTF8().data());
}

void BodySlideFrame::RebuildPresetChoice(const std::string& selectItem) {
	if (!presetChoice)
		return;

	std::string selectedName = selectItem;
	if (selectedName.empty())
		selectedName = GetSelectedPresetName();

	app->SortPresetNamesForDisplay(presetChoiceNames);

	populatingChoices = true;
	wxEventBlocker blocker(presetChoice, wxEVT_CHOICE);
	presetChoice->Freeze();
	presetChoice->Clear();

	for (const auto& rawName : presetChoiceNames)
		presetChoice->Append(FavoriteChoiceLabel(rawName, app->IsFavoritePreset(rawName)));

	if (!SelectChoiceName(presetChoice, presetChoiceNames, selectedName)) {
		int i = wxNOT_FOUND;
		wxString missingItem = wxString::FromUTF8(selectedName);
		if (missingItem.empty()) {
			presetChoiceNames.push_back("");
			i = presetChoice->Append("");
		}
		else if (!missingItem.StartsWith("[")) {
			presetChoiceNames.push_back(selectedName);
			i = presetChoice->Append("[" + missingItem + "]");
		}
		else {
			presetChoiceNames.push_back(selectedName);
			i = presetChoice->Append(missingItem);
		}

		presetChoice->SetSelection(i);
	}

	presetChoice->Thaw();
	populatingChoices = false;
	UpdateFavoriteButtons();
}

std::string BodySlideFrame::GetSelectedOutfitName() const {
	if (!outfitChoice)
		return "";

	int selection = outfitChoice->GetSelection();
	if (selection < 0 || selection >= static_cast<int>(outfitChoiceNames.size()))
		return "";

	return outfitChoiceNames[selection];
}

std::string BodySlideFrame::GetSelectedPresetName() const {
	if (!presetChoice)
		return "";

	int selection = presetChoice->GetSelection();
	if (selection < 0 || selection >= static_cast<int>(presetChoiceNames.size()))
		return "";

	return presetChoiceNames[selection];
}

void BodySlideFrame::UpdateFavoriteButtons() {
	if (btnFavoriteOutfit) {
		std::string outfitName = GetSelectedOutfitName();
		bool canFavorite = !outfitName.empty() && app->OutfitExists(outfitName);
		bool isFavorite = canFavorite && app->IsFavoriteOutfit(outfitName);
		btnFavoriteOutfit->Enable(canFavorite);
		SetFavoriteButtonBitmap(btnFavoriteOutfit, isFavorite);
		btnFavoriteOutfit->SetToolTip(isFavorite ? _("Remove this outfit/body from favorites") : _("Favorite this outfit/body"));
	}

	if (btnFavoritePreset) {
		std::string presetName = GetSelectedPresetName();
		bool canFavorite = !presetName.empty() && app->PresetExists(presetName);
		bool isFavorite = canFavorite && app->IsFavoritePreset(presetName);
		btnFavoritePreset->Enable(canFavorite);
		SetFavoriteButtonBitmap(btnFavoritePreset, isFavorite);
		btnFavoritePreset->SetToolTip(isFavorite ? _("Remove this preset from favorites") : _("Favorite this preset"));
	}
}

void BodySlideFrame::SetSliderPosition(const wxString& name, float newValue, short HiLo) {
	int intval = (int)(newValue * 100.0f);

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	if (HiLo == SLIDER_HI) {
		if (!sd->isZap) {
			sd->sliderReadoutHi->ChangeValue(wxString::Format("%d%%", intval));
			sd->sliderHi->SetValue(intval);
		}
		else
			sd->zapCheckHi->SetValue((intval > 0));
	}
	else if (!sd->oneSize) {
		if (!sd->isZap) {
			sd->sliderReadoutLo->ChangeValue(wxString::Format("%d%%", intval));
			sd->sliderLo->SetValue(intval);
		}
		else
			sd->zapCheckLo->SetValue((intval > 0));
	}
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

void BodySlideFrame::OnSliderChange(wxScrollEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxString fullname = w->GetName();
	wxString name;
	bool isLo = fullname.EndsWith("|LO", &name);
	if (!isLo) {
		if (!fullname.EndsWith("|HI", &name))
			return;
	}

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	app->SetSliderValue(name, isLo, event.GetPosition() / 100.0f);
	app->SetSliderChanged(name, isLo);
	if (isLo)
		sd->sliderReadoutLo->ChangeValue(wxString::Format("%d%%", event.GetPosition()));
	else
		sd->sliderReadoutHi->ChangeValue(wxString::Format("%d%%", event.GetPosition()));

	SetPresetChanged();
	app->UpdatePreview();
}

void BodySlideFrame::OnSliderReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString fullname = w->GetName();
	wxString name;
	bool isLo = fullname.EndsWith("|RLO", &name);
	if (!isLo) {
		if (!fullname.EndsWith("|RHI", &name))
			return;
	}

	SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	w->ChangeValue(wxString::Format("%0.0f%%", v));
	app->SetSliderValue(name, isLo, (float)v / 100.0f);
	app->SetSliderChanged(name, isLo);

	if (isLo)
		sd->sliderLo->SetValue(v);
	else
		sd->sliderHi->SetValue(v);

	SetPresetChanged();
	app->UpdatePreview();
}

void BodySlideFrame::OnSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnOutfitSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnSliderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	DoFilterSliders();
}

void BodySlideFrame::OnPresetFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	app->PopulatePresetList("");
}

void BodySlideFrame::DoFilterSliders() {
	sliderScroll->Freeze();

	wxString filterStr = sliderFilter->GetValue();
	filterStr.MakeLower();

	wxArrayString filterStrings;

	// Split string for "or" filtering
	wxStringTokenizer tokenizer(filterStr, ",;");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim().Trim(false);
		filterStrings.Add(token);
	}

	std::set<std::string> matchedSliders;

	for (auto& sliderDisplay : sliderDisplays) {
		if (!sliderDisplay.second)
			continue;

		// Filter slider by display name or category
		wxString sliderStr = wxString::FromUTF8(sliderDisplay.first).MakeLower();
		wxString displayStr = wxString::FromUTF8(sliderDisplay.second->displayName).MakeLower();
		wxString categoryStr = wxString::FromUTF8(sliderDisplay.second->categoryName).MakeLower();

		// Check if category is disabled
		bool disabledCat = false;

		const std::string& categoryName = sliderDisplay.second->categoryName;
		if (!categoryName.empty()) {
			SliderCategoryUI* sc = GetSliderCategory(categoryName);
			if (sc && !sc->isEnabled)
				disabledCat = true;
		}

		bool show = filterStrings.empty();
		if (!show) {
			for (auto& fstr : filterStrings) {
				// Split string by space for "and" filtering
				bool matched = false;

				wxStringTokenizer andTokenizer(fstr, " ");
				while (andTokenizer.HasMoreTokens()) {
					wxString token = andTokenizer.GetNextToken();
					token.Trim().Trim(false);

					if (displayStr.Contains(token) || sliderStr.Contains(token) || (!categoryStr.empty() && categoryStr.Contains(token)))
						matched = true;
					else {
						matched = false;
						break;
					}
				}

				if (matched) {
					show = true;
					matchedSliders.insert(sliderDisplay.first);
					break;
				}
			}
		}

		if (disabledCat)
			show = false;

		if (show) {
			if (!sliderDisplay.second->isShown)
				sliderDisplay.second->Show();
		}
		else {
			if (sliderDisplay.second->isShown)
				sliderDisplay.second->Show(false);
		}
	}

	for (auto& sliderCategory : sliderCategories) {
		if (!sliderCategory.second)
			continue;

		bool showCat = false;

		if (!filterStrings.empty()) {
			for (auto& sliderName : sliderCategory.second->sliderNames) {
				if (matchedSliders.find(sliderName) != matchedSliders.end())
					showCat = true; // Show category if any slider in it was matched
			}
		}
		else
			showCat = true; // Without a filter, show all categories

		sliderCategory.second->Show(showCat);
	}

	sliderScroll->Thaw();
	sliderScroll->Layout();
	sliderScroll->FitInside();
}

void BodySlideFrame::OnCategoryCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxCheckBox* cb = (wxCheckBox*)event.GetEventObject();
	if (!cb)
		return;

	std::string categoryName = cb->GetName().ToUTF8().data();

	SliderCategoryUI* sc = GetSliderCategory(categoryName);
	if (sc) {
		sc->isEnabled = event.IsChecked();

		DoFilterSliders();

		int scrollPos = sliderScroll->GetScrollPos(wxOrientation::wxVERTICAL);
		sliderScroll->Scroll(0, scrollPos);
	}
}

void BodySlideFrame::OnCategoryTabButton(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxStateButton* tabButton = (wxStateButton*)event.GetEventObject();
	if (!tabButton)
		return;

	std::string categoryName = tabButton->GetName().ToUTF8().data();

	SliderCategoryUI* sc = GetSliderCategory(categoryName);
	if (sc) {
		if (!sc->isEnabled) {
			sc->check->SetValue(true);
			sc->isEnabled = true;
			DoFilterSliders();
		}

		int scroll_rate_y = 0;
		sliderScroll->GetScrollPixelsPerUnit(nullptr, &scroll_rate_y);

		wxPoint window_pos = sliderScroll->CalcUnscrolledPosition(sc->check->GetPosition());
		sliderScroll->Scroll(0, window_pos.y / scroll_rate_y);
	}
}

void BodySlideFrame::OnZapCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxString sn;
	bool isLo = true;
	if (!w->GetName().EndsWith("|ZLO", &sn)) {
		isLo = false;
		if (!w->GetName().EndsWith("|ZHI", &sn)) {
			event.Skip();
			return;
		}
	}

	std::string sliderName{sn.ToUTF8()};
	wxLogMessage("Zap '%s' %s.", sn, event.IsChecked() ? "checked" : "unchecked");

	SliderDisplay* slider = GetSliderDisplay(sliderName);
	if (slider) {
		bool checked = event.IsChecked();
		if (checked) {
			if (slider->oneSize) {
				app->SetSliderValue(sn, false, 1.0f);
			}
			else {
				app->SetSliderValue(sn, true, 1.0f);
				app->SetSliderValue(sn, false, 1.0f);
				if (isLo)
					slider->zapCheckHi->SetValue(true);
				else
					slider->zapCheckLo->SetValue(true);
			}
		}
		else {
			if (slider->oneSize) {
				app->SetSliderValue(sn, false, 0.0f);
			}
			else {
				app->SetSliderValue(sn, true, 0.0f);
				app->SetSliderValue(sn, false, 0.0f);
				if (isLo)
					slider->zapCheckHi->SetValue(false);
				else
					slider->zapCheckLo->SetValue(false);
			}
		}

		app->SetZapChoice(sliderName, checked);
	}

	app->SetSliderChanged(sn, isLo);

	std::vector<std::string> zapToggles = app->GetSliderZapToggles(sn);
	for (auto& toggle : zapToggles) {
		wxLogMessage("Zap '%s' toggled.", toggle);

		app->SetSliderValue(toggle, true, 1.0f - app->GetSliderValue(toggle, true));
		app->SetSliderValue(toggle, false, 1.0f - app->GetSliderValue(toggle, false));
		app->SetSliderChanged(toggle, false);

		slider = GetSliderDisplay(toggle);
		if (slider) {
			if (slider->zapCheckHi)
				slider->zapCheckHi->SetValue(!slider->zapCheckHi->GetValue());
			if (slider->zapCheckLo)
				slider->zapCheckLo->SetValue(!slider->zapCheckLo->GetValue());
		}
	}

	SetPresetChanged();

	wxBeginBusyCursor();
	if (app->IsUVSlider(sn))
		app->UpdatePreview();
	else
		app->RebuildPreviewMeshes();
	wxEndBusyCursor();
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

void BodySlideFrame::OnChooseGroups(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> groupNames;
	std::unordered_set<std::string> curGroupNames;
	app->GetAllGroupNames(groupNames);

	wxString srch = search->GetValue();
	wxStringTokenizer tokenizer(srch, ",;");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim();
		token.Trim(false);
		curGroupNames.insert(token.ToUTF8().data());
	}

	wxArrayString grpChoices;
	wxArrayInt grpSelections;
	int idx = 0;
	for (auto& g : groupNames) {
		grpChoices.Add(wxString::FromUTF8(g));
		if (curGroupNames.find(g) != curGroupNames.end())
			grpSelections.Add(idx);

		idx++;
	}

	grpChoices.Add("Unassigned");

	// Cast and use wxAnyChoiceDialog "Create" function so we can set the wxLB_EXTENDED flag for the internal wxCheckListBox
	wxMultiChoiceDialog chooser{};
	wxAnyChoiceDialog* anyChooser = dynamic_cast<wxAnyChoiceDialog*>(&chooser);
	anyChooser->Create(this, _("Choose groups to filter outfit list"), _("Choose Groups"), grpChoices, wxCHOICEDLG_STYLE, wxDefaultPosition, wxLB_ALWAYS_SB | wxLB_EXTENDED);
	chooser.SetSelections(grpSelections);

	wxString filter;
	if (chooser.ShowModal() == wxID_OK) {
		wxArrayInt sel = chooser.GetSelections();
		for (size_t i = 0; i < sel.size(); i++) {
			if (i > 0)
				filter += ", ";
			filter += grpChoices[sel[i]];
		}
		search->ChangeValue(filter);
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnBrowseOutfitFolder(wxCommandEvent& WXUNUSED(event)) {
	if (!app->HasActiveProject())
		return;

	auto& activeSet = app->GetActiveSet();

	wxFileName folderPath(wxString::FromUTF8(activeSet.GetInputFileName()));
	if (folderPath.IsRelative())
		folderPath.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	folderPath.SetFullName("");

	if (!folderPath.FileExists() && folderPath.DirExists())
		wxLaunchDefaultApplication(folderPath.GetPath());
}

void BodySlideFrame::OnSaveGroups(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	wxFileDialog saveGroupDialog(this,
								 _("Choose or create group file"),
								 wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderGroups",
								 wxEmptyString,
								 "Group Files (*.xml)|*.xml",
								 wxFD_SAVE);
	if (saveGroupDialog.ShowModal() == wxID_CANCEL)
		return;

	wxString fName = saveGroupDialog.GetPath();
	wxString gName;
	int ret = 0;

	do {
		gName = wxGetTextFromUser(_("What would you like the new group to be called?"), _("New Group Name"));
		if (gName.IsEmpty())
			return;

		ret = app->SaveGroupList(fName.ToUTF8().data(), gName.ToUTF8().data());
	} while (ret == 2);

	if (ret == 0) {
		app->LoadAllGroups();
		app->RefreshPresetsForCurrentOutfit();
		search->ChangeValue(gName);
		outfitsearch->ChangeValue("");
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnRefreshGroups(wxCommandEvent& WXUNUSED(event)) {
	app->LoadAllGroups();
	app->RefreshPresetsForCurrentOutfit();
}

void BodySlideFrame::OnRefreshOutfits(wxCommandEvent& WXUNUSED(event)) {
	app->RefreshOutfitList();

	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	app->ActivateOutfit(outfitName);
}

void BodySlideFrame::OnRegexOutfits(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnFilterHasZaps(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateFilterData();
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnFilterOutputWinners(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateFilterData();
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnChooseOutfit(wxCommandEvent& WXUNUSED(event)) {
	if (populatingChoices)
		return;

	std::string sstr = GetSelectedOutfitName();
	if (sstr.empty())
		return;

	app->ActivateOutfit(sstr);
	UpdateFavoriteButtons();
}

void BodySlideFrame::OnChoosePreset(wxCommandEvent& WXUNUSED(event)) {
	if (populatingChoices)
		return;

	std::string sstr = GetSelectedPresetName();

	app->ActivatePreset(sstr);
	UpdateFavoriteButtons();
}

void BodySlideFrame::OnFavoriteOutfit(wxCommandEvent& WXUNUSED(event)) {
	std::string outfitName = GetSelectedOutfitName();
	if (outfitName.empty() || !app->OutfitExists(outfitName))
		return;

	app->ToggleFavoriteOutfit(outfitName);
	RebuildOutfitChoice(outfitName);
}

void BodySlideFrame::OnFavoritePreset(wxCommandEvent& WXUNUSED(event)) {
	std::string presetName = GetSelectedPresetName();
	if (presetName.empty() || !app->PresetExists(presetName))
		return;

	app->ToggleFavoritePreset(presetName);
	RebuildPresetChoice(presetName);
}

void BodySlideFrame::OnDeleteProject(wxCommandEvent& WXUNUSED(event)) {
	int res = wxMessageBox(_("Do you really wish to delete the selected project?"), _("Delete Project"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	app->DeleteOutfit(outfitName);
}

void BodySlideFrame::OnDeletePreset(wxCommandEvent& WXUNUSED(event)) {
	int res = wxMessageBox(_("Do you really wish to delete the selected preset?"), _("Delete Preset"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	app->DeletePreset(presetName);
}

void BodySlideFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	if (!btnSavePreset->IsEnabled())
		return;

	if (OutfitIsEmpty())
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	if (presetName.empty())
		return;

	int error = app->UpdateSliderPositions(presetName);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to save preset (%d)!"), error), _("Error"));
	}

	SetPresetChanged(false);
	app->LoadPresets("");
	app->PopulatePresetList(presetName);
}

void BodySlideFrame::OnEditPreset(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	if (presetName.empty())
		return;

	std::string presetFileName = app->GetPresetFileName(presetName);
	if (presetFileName.empty()) {
		wxLogError("Failed to find preset file for '%s'!", presetName);
		wxMessageBox(wxString::Format(_("Failed to find preset file for '%s'!"), presetName), _("Error"), wxICON_ERROR, this);
		return;
	}

	std::vector<std::string> groups;
	app->GetPresetGroups(presetName, groups);

	PresetSaveDialog psd(this);
	app->GetAllGroupNames(psd.allGroupNames);
	for (auto& group : groups) {
		if (std::find(psd.allGroupNames.begin(), psd.allGroupNames.end(), group) == psd.allGroupNames.end())
			psd.allGroupNames.push_back(group);
	}
	psd.SetExistingPreset(presetName, presetFileName, groups);
	psd.FilterGroups();
	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	int error = app->SavePresetGroups(psd.outFileName, presetName, groups);
	if (error) {
		wxLogError("Failed to save preset '%s' (%d)!", presetName, error);
		wxMessageBox(wxString::Format(_("Failed to save preset '%s' (%d)!"), presetName, error), _("Error"), wxICON_ERROR, this);
	}

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	BodySlideConfig.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnSavePresetAs(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	std::vector<std::string> groups;
	PresetSaveDialog psd(this);

	app->GetAllGroupNames(psd.allGroupNames);
	psd.FilterGroups();
	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	std::string fname = psd.outFileName;
	std::string presetName = psd.outPresetName;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	int error = app->SaveSliderPositions(fname, presetName, groups);
	if (error) {
		wxLogError("Failed to save preset as '%s' (%d)!", fname, error);
		wxMessageBox(wxString::Format(_("Failed to save preset as '%s' (%d)!"), fname, error), "Error");
	}

	SetPresetChanged(false);
	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	BodySlideConfig.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnGroupManager(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> outfits;
	app->GetOutfits(outfits);

	GroupManager gm(this, outfits);
	gm.ShowModal();
	app->LoadAllGroups();
	app->RefreshPresetsForCurrentOutfit();
}

void BodySlideFrame::OnConflictPopup(wxMouseEvent& WXUNUSED(event)) {
	std::vector<std::string> conflictingOutfits = app->GetConflictingOutfits();
	std::string currentOutfitName = BodySlideConfig["SelectedOutfit"];

	int id = fileCollisionMenu->GetMenuItemCount();
	while (id--)
		fileCollisionMenu->Delete(id);

	for (id = 0; id < static_cast<int>(conflictingOutfits.size()); id++) {
		std::string& outfitName = conflictingOutfits[id];
		wxMenuItem* outfitItem = fileCollisionMenu->AppendRadioItem(id, wxString::FromUTF8(outfitName));

		if (outfitName == currentOutfitName)
			outfitItem->Check();
	}

	id = GetPopupMenuSelectionFromUser(*fileCollisionMenu, wxDefaultPosition);
	if (id < 0)
		return;

	std::string& selectedOutfitName = conflictingOutfits[id];
	if (selectedOutfitName != currentOutfitName)
		app->ActivateOutfit(selectedOutfitName);
}

void BodySlideFrame::OnOutfitChoiceSelect(wxCommandEvent& WXUNUSED(event)) {
	app->SetDefaultBuildSelection();
}

void BodySlideFrame::OnHighToLow(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(false);
}

void BodySlideFrame::OnLowToHigh(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(true);
}

void BodySlideFrame::OnPreview(wxCommandEvent& WXUNUSED(event)) {
	if (app->IsPreviewPoppedOut()) {
		app->ClosePreview();
		previewVisible = false;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", false);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
		UpdatePreviewButtonLabel();
		return;
	}

	if (previewVisible) {
		// Hide preview
		UnsplitPreview();
		previewVisible = false;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", false);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", false);
		UpdatePreviewButtonLabel();
	}
	else {
		bool restorePreviewPoppedOut = BodySlideConfig.GetBoolValue("BodySlideFrame.previewPoppedOut", false)
			|| BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false);
		if (restorePreviewPoppedOut) {
			app->PopOutPreview();
			previewVisible = true;
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
			UpdatePreviewButtonLabel();

			// The preview may have been hidden while outfits changed; reload current data
			// now that the detached preview window is visible again.
			if (!OutfitIsEmpty()) {
				app->InitPreviewPanel();
				if (app->HasActiveProject())
					app->InitPreview();
			}
			return;
		}

		// Show preview
		SplitPreview();
		previewVisible = true;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", false);
		UpdatePreviewButtonLabel();

		// Trigger preview load if outfit is selected
		if (!OutfitIsEmpty()) {
			app->InitPreviewPanel();
			if (app->HasActiveProject()) {
				app->InitPreview();
			}
		}
	}
}

void BodySlideFrame::OnSashPosChanged(wxSplitterEvent& event) {
	if (!IsVisible())
		return;

	int pos = event.GetSashPosition();
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", pos);
	savedSashPosition = pos;
	savedPreviewWidth = previewOnLeft ? pos : splitter->GetSize().GetWidth() - pos;
	if (savedPreviewWidth > 0)
		BodySlideConfig.SetValue("BodySlideFrame.previewWidth", savedPreviewWidth);
}

void BodySlideFrame::OnSashPosChanging(wxSplitterEvent& event) {
	if (!splitter || !splitter->IsSplit())
		return;

	const int minMainWidth = FromDIP(MinBodySlideLeftPaneWidthDip);
	if (previewOnLeft) {
		// The main pane is on the right, so the sash may not move too far right
		int maxSashPos = splitter->GetSize().GetWidth() - minMainWidth;
		if (maxSashPos < splitter->GetMinimumPaneSize())
			maxSashPos = splitter->GetMinimumPaneSize();

		if (event.GetSashPosition() > maxSashPos)
			event.SetSashPosition(maxSashPos);
	}
	else if (event.GetSashPosition() < minMainWidth) {
		event.SetSashPosition(minMainWidth);
	}
}

void BodySlideFrame::OnPreviewPopout(wxCommandEvent& WXUNUSED(event)) {
	if (app->IsPreviewPoppedOut()) {
		if (!BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false))
			app->DockPreview(true);
		return;
	}

	app->PopOutPreview();
}

void BodySlideFrame::OnPreviewWindowClosed() {
	app->DockPreview(false, true);
}

void BodySlideFrame::UnsplitPreview() {
	if (!splitter || !splitter->IsSplit())
		return;

	savedSashPosition = splitter->GetSashPosition();
	savedPreviewWidth = previewOnLeft ? savedSashPosition : splitter->GetSize().GetWidth() - savedSashPosition;
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", savedSashPosition);
	BodySlideConfig.SetValue("BodySlideFrame.previewWidth", savedPreviewWidth);
	splitter->Unsplit(previewPanel);

	if (savedPreviewWidth > 0) {
		wxSize sz = GetSize();
		sz.SetWidth(sz.GetWidth() - savedPreviewWidth);
		SetSize(sz);
	}
}

void BodySlideFrame::SplitPreview(wxPanel* panel) {
	if (!splitter || splitter->IsSplit())
		return;

	wxPanel* panelToSplit = panel ? panel : previewPanel;
	if (!panelToSplit)
		return;

	int previewWidth = savedPreviewWidth;
	if (previewWidth <= 0)
		previewWidth = BodySlideConfig.GetIntValue("BodySlideFrame.previewWidth");
	if (previewWidth <= 0)
		previewWidth = 400;

	// With the preview on the left, the sash position is the preview width itself
	int sashPos = previewWidth;
	if (!previewOnLeft) {
		sashPos = savedSashPosition;
		if (sashPos <= 0)
			sashPos = BodySlideConfig.GetIntValue("BodySlideFrame.sashpos");
		if (sashPos <= 0)
			sashPos = GetClientSize().GetWidth();
	}

	wxSize sz = GetSize();
	sz.SetWidth(sz.GetWidth() + previewWidth);
	SetSize(sz);

	panelToSplit->Show();

	if (previewOnLeft)
		splitter->SplitVertically(panelToSplit, leftPanel, sashPos);
	else
		splitter->SplitVertically(leftPanel, panelToSplit, sashPos);

	savedSashPosition = sashPos;
	savedPreviewWidth = previewWidth;
}

void BodySlideFrame::SetPreviewOnLeft(bool onLeft) {
	if (previewOnLeft == onLeft)
		return;

	if (!splitter || !splitter->IsSplit()) {
		previewOnLeft = onLeft;
		return;
	}

	// Keep the current preview width and swap the panes in place, without resizing the frame
	const int splitterWidth = splitter->GetSize().GetWidth();
	const int sashPos = splitter->GetSashPosition();

	int previewWidth = previewOnLeft ? sashPos : splitterWidth - sashPos;
	if (previewWidth <= 0)
		previewWidth = savedPreviewWidth;
	if (previewWidth <= 0)
		previewWidth = FromDIP(400);

	wxWindow* previewPane = splitter->GetWindow1() == leftPanel ? splitter->GetWindow2() : splitter->GetWindow1();
	if (!previewPane) {
		previewOnLeft = onLeft;
		return;
	}

	previewOnLeft = onLeft;

	splitter->Unsplit(previewPane);
	previewPane->Show();

	int newSashPos = onLeft ? previewWidth : splitterWidth - previewWidth;
	if (newSashPos < splitter->GetMinimumPaneSize())
		newSashPos = splitter->GetMinimumPaneSize();

	if (onLeft)
		splitter->SplitVertically(previewPane, leftPanel, newSashPos);
	else
		splitter->SplitVertically(leftPanel, previewPane, newSashPos);

	savedSashPosition = newSashPos;
	savedPreviewWidth = previewWidth;
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", newSashPos);
	BodySlideConfig.SetValue("BodySlideFrame.previewWidth", previewWidth);
}

void BodySlideFrame::UpdatePreviewButtonLabel() {
	auto btn = (wxButton*)FindWindowByName("btnPreview", this);
	if (btn)
		btn->SetLabel(previewVisible ? _("Hide Preview") : _("Show Preview"));
}

void BodySlideFrame::OnBuildBodies(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	bool tri = false;
	bool forceNormals = false;

	auto cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	auto cbForceBodyNormals = (wxCheckBox*)FindWindowByName("cbForceBodyNormals");
	if (cbForceBodyNormals)
		forceNormals = cbForceBodyNormals->IsChecked();

	if (wxGetKeyState(WXK_CONTROL))
		app->BuildBodies(true, false, tri, forceNormals);
	else if (wxGetKeyState(WXK_ALT))
		app->BuildBodies(false, true, tri, forceNormals);
	else
		app->BuildBodies(false, false, tri, forceNormals);
}

void BodySlideFrame::OnBatchBuild(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	bool custpath = false;
	bool clean = false;
	if (wxGetKeyState(WXK_CONTROL))
		custpath = true;
	else if (wxGetKeyState(WXK_ALT))
		clean = true;

	if (app->clippingFixStrength > 0.0f) {
		int answer = wxMessageBox(
			_("Fix Clipping is enabled for this batch build.\n\n"
			  "Use this carefully: applying clipping fixes to many outfits at once can create unwelcome side effects on some meshes.\n\n"
			  "Consider building outfits one-by-one and checking each result in Preview.\n\n"
			  "Do you want to continue with batch build?"),
			_("Warning"),
			wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
			this);

		if (answer != wxYES)
			return;
	}

	wxArrayString oChoices;
	std::vector<std::string> outfitChoices;
	std::vector<std::string> toBuild;

	bool tri = false;
	bool forceNormals = false;

	auto cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	auto cbForceBodyNormals = (wxCheckBox*)FindWindowByName("cbForceBodyNormals");
	if (cbForceBodyNormals)
		forceNormals = cbForceBodyNormals->IsChecked();

	app->GetFilteredOutfits(outfitChoices);

	std::map<std::string, uint32_t> outfitIndices;

	uint32_t idx = 0;
	for (auto& o : outfitChoices) {
		oChoices.Add(wxString::FromUTF8(o));
		outfitIndices[o] = idx;
		idx++;
	}

	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* batchBuildChooser = rsrc->LoadDialog(this, "dlgBatchBuild");
	if (!batchBuildChooser)
		return;

	batchBuildChooser->SetSize(batchBuildChooser->FromDIP(wxSize(650, 300)));
	batchBuildChooser->SetSizeHints(batchBuildChooser->FromDIP(wxSize(650, 300)), batchBuildChooser->FromDIP(wxSize(650, -1)));
	batchBuildChooser->CenterOnParent();

	// Load BuildSelection file
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	app->GetBuildSelection(buildSelFile, buildSelection);

	batchBuildList = XRCCTRL((*batchBuildChooser), "batchBuildList", wxCheckListBox);
	batchBuildList->Bind(wxEVT_RIGHT_UP, &BodySlideFrame::OnBatchBuildContext, this);

	batchBuildList->Append(oChoices);

	for (uint32_t i = 0; i < oChoices.size(); i++)
		batchBuildList->Check(i);

	for (auto& outFile : app->outFileCount) {
		if (outFile.second.size() > 1) {
			std::vector<std::string> outfitsInBuild;
			for (auto& outfit : outFile.second) {
				// Only if it's going to be batch built
				if (std::find(outfitChoices.begin(), outfitChoices.end(), outfit) != outfitChoices.end())
					outfitsInBuild.push_back(outfit);
			}

			// Same file would not be written more than once
			if (outfitsInBuild.size() <= 1)
				continue;

			std::string outputChoice = buildSelection.GetOutputChoice(outFile.first);
			if (!outputChoice.empty()) {
				for (auto& outfit : outfitsInBuild) {
					if (outfit != outputChoice) {
						// Uncheck choice by default
						batchBuildList->Check(outfitIndices[outfit], false);
					}
				}
			}
		}
	}

	if (batchBuildChooser->ShowModal() == wxID_OK) {
		wxArrayInt sel;
		batchBuildList->GetCheckedItems(sel);
		toBuild.clear();
		for (size_t i = 0; i < sel.size(); i++)
			toBuild.push_back(outfitChoices[sel[i]]);

		delete batchBuildChooser;
	}
	else {
		delete batchBuildChooser;
		return;
	}

	std::map<std::string, std::string> failedOutfits;
	int ret;
	if (custpath) {
		std::string path = wxDirSelector(_("Choose a folder to contain the saved files")).ToStdString();
		if (path.empty())
			return;

		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, forceNormals, path + PathSepStr);
	}
	else if (clean)
		ret = app->BuildListBodies(toBuild, failedOutfits, true, tri, forceNormals);
	else
		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, forceNormals);

	wxLog::FlushActive();

	if (ret == 0) {
		wxLogMessage("All sets processed successfully!");
		wxMessageBox(_("All sets processed successfully!"), _("Complete"), wxICON_INFORMATION);
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto& e : failedOutfits) {
			wxString ename = wxString::FromUTF8(e.first);
			wxLogError("Failed to build '%s': %s", ename, e.second);
			errlist.Add(ename + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(this, _("The following sets failed"), _("Failed"), errlist, (void**)0, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
		errdisplay.ShowModal();
	}
}

void BodySlideFrame::OnBatchBuildContext(wxMouseEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("batchBuildContext");
	if (menu) {
		menu->Bind(wxEVT_MENU, &BodySlideFrame::OnBatchBuildSelect, this);
		PopupMenu(menu);
		delete menu;
	}
}

void BodySlideFrame::OnBatchBuildSelect(wxCommandEvent& event) {
	if (event.GetId() == XRCID("batchBuildNone")) {
		for (uint32_t i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i, false);
	}
	else if (event.GetId() == XRCID("batchBuildAll")) {
		for (uint32_t i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i);
	}
	else if (event.GetId() == XRCID("batchBuildInvert")) {
		for (uint32_t i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i, !batchBuildList->IsChecked(i));
	}
}

void BodySlideFrame::OnOutfitStudio(wxCommandEvent& WXUNUSED(event)) {
	app->LaunchOutfitStudio();
}

void BodySlideFrame::OnChooseTargetGame(wxCommandEvent& event) {
	wxChoice* choiceTargetGame = (wxChoice*)event.GetEventObject();
	wxWindow* parent = choiceTargetGame->GetGrandParent();
	wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*parent, "fpSkeletonFile", wxFilePickerCtrl);
	wxChoice* choiceSkeletonRoot = XRCCTRL(*parent, "choiceSkeletonRoot", wxChoice);

	TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
	switch (targ) {
		case OB:
			fpSkeletonFile->SetPath("res/skeleton_ob.nif");
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case FO3:
		case FONV:
			fpSkeletonFile->SetPath("res/skeleton_fo3nv.nif");
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case SKYRIM:
			fpSkeletonFile->SetPath("res/skeleton_female_sk.nif");
			choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
			break;
		case SKYRIMSE:
		case SKYRIMVR:
			fpSkeletonFile->SetPath("res/skeleton_female_sse.nif");
			choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
			break;
		case SF:
			fpSkeletonFile->SetPath("res/skeleton_female_sf.nif");
			choiceSkeletonRoot->SetStringSelection("Root");
			break;
		case FO4:
		case FO4VR:
		case FO76:
		default:
			fpSkeletonFile->SetPath("res/skeleton_fo4.nif");
			choiceSkeletonRoot->SetStringSelection("Root");
			break;
	}

	wxCheckListBox* dataFileList = XRCCTRL(*parent, "DataFileList", wxCheckListBox);
	wxString dataDir = GameUtil::GetGameDataPath(targ);

	wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*parent, "dpGameDataPath", wxDirPickerCtrl);
	dpGameDataPath->SetPath(dataDir);

	SettingsFillDataFiles(dataFileList, dataDir, targ);
}

void BodySlideFrame::SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame) {
	dataFileList->Clear();

	wxString cp = "GameDataFiles/" + GameUtil::TargetGames[targetGame];
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false).Trim();
		val.MakeLower();
		fsearch[val] = true;
	}

	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& file : files) {
		file = file.AfterLast('/').AfterLast('\\');
		dataFileList->Insert(file, dataFileList->GetCount());

		if (fsearch.find(file.Lower()) == fsearch.end())
			dataFileList->Check(dataFileList->GetCount() - 1);
	}
}

void BodySlideFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(settings->FromDIP(wxSize(525, -1)));
		settings->SetMinSize(settings->FromDIP(wxSize(525, -1)));
		settings->CenterOnParent();

		wxCollapsiblePane* advancedPane = XRCCTRL(*settings, "advancedPane", wxCollapsiblePane);
		advancedPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&settings](wxCommandEvent&) { settings->Fit(); });

		SettingsDialogShared::CommonSettingsDialogControls commonControls{};
		SettingsDialogShared::InitCommonSettingsDialog(*settings,
			Config,
			BodySlideConfig,
			SupportedLangs.data(),
			SupportedLangs.size(),
			commonControls);

		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);

		wxCheckBox* cbPreviewAlwaysDetached = XRCCTRL(*settings, "cbPreviewAlwaysDetached", wxCheckBox);
		cbPreviewAlwaysDetached->SetValue(BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false));

		wxCheckBox* cbPreviewOnLeft = XRCCTRL(*settings, "cbPreviewOnLeft", wxCheckBox);
		cbPreviewOnLeft->SetValue(BodySlideConfig.GetBoolValue("BodySlideFrame.previewOnLeft", false));

		// Hide the single instance setting (only relevant for Outfit Studio)
		XRCCTRL(*settings, "lbSingleInstanceBehavior", wxStaticText)->Hide();
		XRCCTRL(*settings, "choiceSingleInstanceBehavior", wxChoice)->Hide();

		SettingsFillDataFiles(commonControls.dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		commonControls.choiceTargetGame->Bind(wxEVT_CHOICE, &BodySlideFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			int targetGameSelection = 0;
			bool needsRestart = false;
			SettingsDialogShared::SaveCommonSettingsDialog(
				Config,
				BodySlideConfig,
				GameUtil::TargetGames.data(),
				GameUtil::TargetGames.size(),
				SupportedLangs.data(),
				SupportedLangs.size(),
				commonControls,
				[this]() { app->InitLanguage(); },
				targetGameSelection,
				needsRestart);

			TargetGame targ = (TargetGame)targetGameSelection;

			BodySlideConfig.SetBoolValue("BodySlideFrame.previewAlwaysDetached", cbPreviewAlwaysDetached->IsChecked());
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewOnLeft", cbPreviewOnLeft->IsChecked());
			SetPreviewOnLeft(cbPreviewOnLeft->IsChecked());

			// Only feeds a uniform, so the meshes and their textures stay as they are
			app->ApplyComplexMaterialSetting();

			// This one picks the shader files, so it rebuilds the preview meshes when it changed
			app->ApplyPBRSetting();

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
			app->SaveFavorites();
			app->targetGame = targ;
			app->LoadFavorites();
			GameUtil::InitArchives();
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

void BodySlideFrame::OnClippingStrengthChanged(wxCommandEvent& WXUNUSED(event)) {
	auto sliderClippingStrength = XRCCTRL(*this, "sliderClippingStrength", wxSlider);
	if (!sliderClippingStrength)
		return;

	app->clippingFixStrength = static_cast<float>(sliderClippingStrength->GetValue());
	app->UpdateReferenceCheckboxState();
	app->UpdatePreview();
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

SliderCategoryUI::SliderCategoryUI() {}

bool SliderCategoryUI::Create(wxScrolledWindow* scrollWindow, wxSizer* sliderLayout, wxSizer* categoryTabSizer, const std::string& name, const std::vector<std::string>& sliders, bool pEnabled, bool pOneSize) {
	categoryName = name;
	sliderNames = sliders;

	isEnabled = pEnabled;
	oneSize = pOneSize;

	if (isCreated) {
		check->SetValue(isEnabled);
		label->SetLabel(name);

		Show();
		return true;
	}

	if (!oneSize) {
		sliderLayout->AddSpacer(0);

		dummyPanel1 = new wxPanel(scrollWindow);
		dummyPanel1->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(dummyPanel1, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	check = new wxCheckBox(scrollWindow, wxID_ANY, "");
	check->SetName(name);
	sliderLayout->Add(check, 0, wxRIGHT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 5);
	check->SetValue(isEnabled);

	label = new wxStaticText(scrollWindow, wxID_ANY, name);
	label->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Andalus"));
	label->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(label, 0, wxLEFT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

	if (!oneSize) {
		dummyPanel2 = new wxPanel(scrollWindow);
		dummyPanel2->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(dummyPanel2, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	sliderLayout->AddSpacer(0);

	if (categoryTabSizer) {
		tabButton = new wxStateButton(categoryTabSizer->GetContainingWindow(), wxID_ANY, name, wxDefaultPosition, wxDefaultSize, 0L, wxDefaultValidator, name, true);
		categoryTabSizer->Add(tabButton, 0, 0, 0);
	}

	Show(false);
	isCreated = true;
	return true;
}

void SliderCategoryUI::Show(bool show) {
	if (dummyPanel1)
		dummyPanel1->Show(show && !oneSize);

	check->Show(show);
	label->Show(show);

	if (dummyPanel2)
		dummyPanel2->Show(show && !oneSize);

	if (tabButton) {
		auto wrapSizer = (wxWrapSizer*)tabButton->GetContainingSizer();
		wrapSizer->Show(tabButton, show);

		wxSize minSize = wrapSizer->CalcMin();
		wrapSizer->RepositionChildren(minSize);
	}

	isShown = show;
}

void SliderCategoryUI::Destroy() {
	if (dummyPanel1) {
		dummyPanel1->Destroy();
		dummyPanel1 = nullptr;
	}

	check->Destroy();
	check = nullptr;

	label->Destroy();
	label = nullptr;

	if (dummyPanel2) {
		dummyPanel2->Destroy();
		dummyPanel2 = nullptr;
	}

	if (tabButton) {
		tabButton->Destroy();
		tabButton = nullptr;
	}

	isShown = false;
}


SliderDisplay::SliderDisplay() {}

bool SliderDisplay::Create(wxScrolledWindow* scrollWindow,
						   wxSizer* sliderLayout,
						   const std::string& name,
						   const std::string& display,
						   const std::string& category,
						   int minValue,
						   int maxValue,
						   bool pIsZap,
						   bool pOneSize) {
	isZap = pIsZap;
	oneSize = pOneSize;

	sliderName = name;
	displayName = display;
	categoryName = category;

	wxString nameStr = wxString::FromUTF8(name);
	wxString displayNameStr = wxString::FromUTF8(display);

	if (isCreated) {
		if (lblSliderLo->GetLabel() != displayNameStr)
			lblSliderLo->SetLabel(displayNameStr);

		lblSliderLo->Show(!oneSize);

		if (!oneSize)
			sliderLayout->Add(lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

		zapCheckLo->SetName(nameStr + "|ZLO");
		zapCheckLo->Show(!oneSize && isZap);

		if (!oneSize && isZap) {
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(zapCheckLo, 0, wxALIGN_LEFT, 0);
		}

		sliderLo->SetName(nameStr + "|LO");
		sliderLo->SetMin(minValue);
		sliderLo->SetMax(maxValue);
		sliderLo->Show(!oneSize && !isZap);

		if (!oneSize && !isZap)
			sliderLayout->Add(sliderLo, 1, wxEXPAND, 0);

		sliderReadoutLo->SetName(nameStr + "|RLO");
		sliderReadoutLo->Show(!oneSize && !isZap);

		if (!oneSize && !isZap)
			sliderLayout->Add(sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);

		if (lblSliderHi->GetLabel() != displayNameStr)
			lblSliderHi->SetLabel(displayNameStr);

		sliderLayout->Add(lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

		zapCheckHi->SetName(nameStr + "|ZHI");
		zapCheckHi->Show(isZap);

		if (isZap) {
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(zapCheckHi, 0, wxALIGN_LEFT, 0);
		}

		sliderHi->SetName(nameStr + "|HI");
		sliderHi->SetMin(minValue);
		sliderHi->SetMax(maxValue);
		sliderHi->Show(!isZap);

		if (!isZap)
			sliderLayout->Add(sliderHi, 1, wxEXPAND, 0);

		sliderReadoutHi->SetName(nameStr + "|RHI");
		sliderReadoutHi->Show(!isZap);

		if (!isZap)
			sliderLayout->Add(sliderReadoutHi, 0, wxRIGHT | wxALIGN_CENTER, 10);

		Show();
		return true;
	}

	lblSliderLo = new wxStaticText(scrollWindow, wxID_ANY, display, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	lblSliderLo->SetForegroundColour(wxColour(200, 200, 200));
	lblSliderLo->Show(!oneSize);

	if (!oneSize)
		sliderLayout->Add(lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

	zapCheckLo = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nameStr + "|ZLO");
	zapCheckLo->Show(!oneSize && isZap);

	if (!oneSize && isZap) {
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(zapCheckLo, 0, wxALIGN_LEFT, 0);
	}

	sliderLo = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxDefaultSize, wxSL_BOTTOM | wxSL_HORIZONTAL);
	sliderLo->SetTickFreq(5);
	sliderLo->SetName(nameStr + "|LO");
	sliderLo->Show(!oneSize && !isZap);

	if (!oneSize && !isZap)
		sliderLayout->Add(sliderLo, 1, wxEXPAND, 0);

	sliderReadoutLo = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(scrollWindow->FromDIP(50), -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
	sliderReadoutLo->Show(!oneSize && !isZap);
	sliderReadoutLo->SetName(nameStr + "|RLO");

	if (!oneSize && !isZap)
		sliderLayout->Add(sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);

	lblSliderHi = new wxStaticText(scrollWindow, wxID_ANY, display, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	lblSliderHi->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

	zapCheckHi = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nameStr + "|ZHI");
	zapCheckHi->Show(isZap);

	if (isZap) {
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(zapCheckHi, 0, wxALIGN_LEFT, 0);
	}

	sliderHi = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	sliderHi->SetTickFreq(5);
	sliderHi->SetName(nameStr + "|HI");
	sliderHi->Show(!isZap);

	if (!isZap)
		sliderLayout->Add(sliderHi, 1, wxEXPAND, 0);

	sliderReadoutHi = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(scrollWindow->FromDIP(50), -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
	sliderReadoutHi->Show(!isZap);
	sliderReadoutHi->SetName(nameStr + "|RHI");

	if (!isZap)
		sliderLayout->Add(sliderReadoutHi, 0, wxRIGHT | wxALIGN_CENTER, 10);

	Show(false);
	isCreated = true;
	return true;
}

void SliderDisplay::Show(bool show) {
	lblSliderLo->Show(show && !oneSize);
	zapCheckLo->Show(show && !oneSize && isZap);
	sliderLo->Show(show && !oneSize && !isZap);
	sliderReadoutLo->Show(show && !oneSize && !isZap);
	lblSliderHi->Show(show);
	zapCheckHi->Show(show && isZap);
	sliderHi->Show(show && !isZap);
	sliderReadoutHi->Show(show && !isZap);
	isShown = show;
}


SliderDisplay* SliderDisplayPool::Push() {
	if (pool.size() < MaxPoolSize) {
		auto entry = new SliderDisplay();
		pool.push_back(entry);
		return entry;
	}

	return nullptr;
}

void SliderDisplayPool::CreatePool(size_t poolSize, wxScrolledWindow* scrollWindow, wxSizer* sliderLayout) {
	if (poolSize > MaxPoolSize)
		poolSize = MaxPoolSize;

	pool.resize(poolSize, nullptr);

	for (auto& p : pool) {
		if (!p)
			p = new SliderDisplay();

		if (!p->IsCreated())
			p->Create(scrollWindow, sliderLayout, "sliderPoolDummy", "", "", 0, 100, false);
	}
}

SliderDisplay* SliderDisplayPool::Get(size_t index) {
	if (pool.size() > index)
		return pool[index];

	return nullptr;
}

SliderDisplay* SliderDisplayPool::GetNext() {
	for (size_t i = 0; i < pool.size(); ++i) {
		SliderDisplay* entry = pool[i];

		// Index of a slider that is invisible can be reused
		if (entry && !entry->isShown)
			return entry;
	}

	return Push();
}

void SliderDisplayPool::Clear() {
	for (auto& p : pool)
		delete p;

	pool.clear();
}
