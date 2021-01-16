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
#include "../files/wxDDSImage.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include <wx/debugrpt.h>
#include <regex>
#include <atomic>

#ifdef WIN64
	#include <ppl.h>
	#include <ppltasks.h>
	#include <concurrent_unordered_map.h>
#else
	#undef _PPL_H
#endif

ConfigurationManager Config;
ConfigurationManager BodySlideConfig;

const wxString TargetGames[] = { "Fallout3", "FalloutNewVegas", "Skyrim", "Fallout4", "SkyrimSpecialEdition", "Fallout4VR", "SkyrimVR" };
const wxLanguage SupportedLangs[] = {
	wxLANGUAGE_ENGLISH, wxLANGUAGE_AFRIKAANS, wxLANGUAGE_ARABIC, wxLANGUAGE_CATALAN, wxLANGUAGE_CZECH, wxLANGUAGE_DANISH, wxLANGUAGE_GERMAN,
	wxLANGUAGE_GREEK, wxLANGUAGE_SPANISH, wxLANGUAGE_BASQUE, wxLANGUAGE_FINNISH, wxLANGUAGE_FRENCH, wxLANGUAGE_HINDI,
	wxLANGUAGE_HUNGARIAN, wxLANGUAGE_INDONESIAN, wxLANGUAGE_ITALIAN, wxLANGUAGE_JAPANESE, wxLANGUAGE_KOREAN, wxLANGUAGE_LITHUANIAN,
	wxLANGUAGE_LATVIAN, wxLANGUAGE_MALAY, wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI, wxLANGUAGE_DUTCH, wxLANGUAGE_POLISH,
	wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN, wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK, wxLANGUAGE_SLOVENIAN, wxLANGUAGE_ALBANIAN,
	wxLANGUAGE_SWEDISH, wxLANGUAGE_TAMIL, wxLANGUAGE_TURKISH, wxLANGUAGE_UKRAINIAN, wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE
};

wxBEGIN_EVENT_TABLE(BodySlideFrame, wxFrame)
	EVT_MENU(wxID_EXIT, BodySlideFrame::OnExit)
	EVT_CLOSE(BodySlideFrame::OnClose)
	EVT_ACTIVATE(BodySlideFrame::OnActivateFrame)
	EVT_ICONIZE(BodySlideFrame::OnIconizeFrame)
	EVT_COMMAND_SCROLL(wxID_ANY, BodySlideFrame::OnSliderChange)
	EVT_TEXT_ENTER(wxID_ANY, BodySlideFrame::OnSliderReadoutChange)
	EVT_TEXT(XRCID("searchHolder"), BodySlideFrame::OnSearchChange)
	EVT_TEXT(XRCID("outfitsearchHolder"), BodySlideFrame::OnOutfitSearchChange)
    EVT_TIMER(DELAYLOAD_TIMER, BodySlideFrame::OnDelayLoad)
	EVT_CHOICE(XRCID("outfitChoice"), BodySlideFrame::OnChooseOutfit)
	EVT_CHOICE(XRCID("presetChoice"), BodySlideFrame::OnChoosePreset)

	EVT_BUTTON(XRCID("btnDeleteProject"), BodySlideFrame::OnDeleteProject)
	EVT_BUTTON(XRCID("btnDeletePreset"), BodySlideFrame::OnDeletePreset)

	EVT_BUTTON(XRCID("btnPreview"), BodySlideFrame::OnPreview)
	EVT_BUTTON(XRCID("btnHighToLow"), BodySlideFrame::OnHighToLow)
	EVT_BUTTON(XRCID("btnLowToHigh"), BodySlideFrame::OnLowToHigh)
	EVT_BUTTON(XRCID("btnBuildBatch"), BodySlideFrame::OnBatchBuild)
	EVT_BUTTON(XRCID("btnBuild"), BodySlideFrame::OnBuildBodies)
	EVT_BUTTON(XRCID("btnOutfitStudio"), BodySlideFrame::OnOutfitStudio)
	EVT_BUTTON(XRCID("btnSettings"), BodySlideFrame::OnSettings)
	EVT_BUTTON(XRCID("btnAbout"), BodySlideFrame::OnAbout)
	EVT_BUTTON(XRCID("btnSavePreset"), BodySlideFrame::OnSavePreset)
	EVT_BUTTON(XRCID("btnSavePresetAs"), BodySlideFrame::OnSavePresetAs)
	EVT_BUTTON(XRCID("btnGroupManager"), BodySlideFrame::OnGroupManager)
	EVT_BUTTON(XRCID("btnChooseGroups"), BodySlideFrame::OnChooseGroups)
	EVT_BUTTON(XRCID("btnRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
	EVT_BUTTON(XRCID("btnEditProject"), BodySlideFrame::OnEditProject)
		
	EVT_MENU(XRCID("menuChooseGroups"), BodySlideFrame::OnChooseGroups)	
	EVT_MENU(XRCID("menuRefreshGroups"), BodySlideFrame::OnRefreshGroups)
	EVT_MENU(XRCID("menuRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
	EVT_MENU(XRCID("menuSaveGroups"), BodySlideFrame::OnSaveGroups)
	
	EVT_MOVE_END(BodySlideFrame::OnMoveWindow)
	EVT_SIZE(BodySlideFrame::OnSetSize)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(BodySlideApp);

BodySlideApp::~BodySlideApp() {
	delete previewBaseNif;
	previewBaseNif = nullptr;

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
	std::string dataDir{wxStandardPaths::Get().GetDataDir().ToUTF8()};
#endif

	Config.LoadConfig(dataDir + "/Config.xml");
	BodySlideConfig.LoadConfig(dataDir + "/BodySlide.xml", "BodySlideConfig");

	Config.SetDefaultValue("AppDir", dataDir);

	logger.Initialize(Config.GetIntValue("LogLevel", -1), dataDir + "/Log_BS.txt");
	wxLogMessage("Initializing BodySlide...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

	wxSetEnv("AppDir", wxString::FromUTF8(dataDir));

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);
	xrc->InitAllHandlers();
	wxInitAllImageHandlers();
	wxImage::AddHandler(new wxDDSHandler);

	preview = nullptr;
	sliderView = nullptr;
	previewBaseNif = nullptr;

	Bind(wxEVT_CHAR_HOOK, &BodySlideApp::CharHook, this);

	wxLogMessage("Working directory: %s", wxGetCwd());
	wxLogMessage("Executable directory: %s", wxString::FromUTF8(dataDir));
	if (!SetDefaultConfig())
		return false;

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
		default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	int x = BodySlideConfig.GetIntValue("BodySlideFrame.x");
	int y = BodySlideConfig.GetIntValue("BodySlideFrame.y");
	int w = BodySlideConfig.GetIntValue("BodySlideFrame.width");
	int h = BodySlideConfig.GetIntValue("BodySlideFrame.height");
	std::string maximized = BodySlideConfig["BodySlideFrame.maximized"];

	wxLogMessage("Loading BodySlide frame at X:%d Y:%d with W:%d H:%d...", x, y, w, h);
	sliderView = new BodySlideFrame(this, wxSize(w, h));
	sliderView->SetPosition(wxPoint(x, y));
	if (maximized == "true")
		sliderView->Maximize();

	sliderView->Show();
	SetTopWindow(sliderView);

	InitArchives();

	if (!GetOutputDataPath().empty()) {
		bool dirWritable = wxFileName::IsDirWritable(GetOutputDataPath());
		bool dirReadable = wxFileName::IsDirReadable(GetOutputDataPath());
		if (!dirWritable || !dirReadable)
			wxMessageBox(_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."), _("Warning"), wxICON_WARNING);
	}

	wxLogMessage("BodySlide initialized.");
	return true;
}

void BodySlideApp::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
}

bool BodySlideApp::OnCmdLineParsed(wxCmdLineParser& parser) {
	parser.Found("gbuild", &cmdGroupBuild);
	parser.Found("t", &cmdTargetDir);
	parser.Found("p", &cmdPreset);
	cmdTri = parser.Found("tri");
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
	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);

	wxDebugReport report;
	report.AddExceptionContext();
	report.Process();
}


void BodySlideApp::InitArchives() {
	// Auto-detect archives
	FSManager::del();

	std::vector<std::string> fileList;
	GetArchiveFiles(fileList);

	FSManager::addArchives(fileList);
}

void BodySlideApp::GetArchiveFiles(std::vector<std::string>& outList) {
	TargetGame targ = (TargetGame)Config.GetIntValue("TargetGame");
	std::string cp = "GameDataFiles/" + TargetGames[targ];
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false);
		val = val.Trim().MakeLower();
		fsearch[val] = true;
	}

	wxString dataDir = Config["GameDataPath"];
	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& f : files) {
		f = f.AfterLast('/').AfterLast('\\');
		if (fsearch.find(f.Lower()) == fsearch.end())
			outList.push_back((dataDir + f).ToUTF8().data());
	}
}

void BodySlideApp::LoadData() {
	if (!sliderView)
		return;

	wxLogMessage("Loading initial data...");
	LoadAllCategories();
	LoadAllGroups();
	LoadSliderSets();

	std::string activeOutfit = BodySlideConfig["SelectedOutfit"];
	if (activeOutfit.empty() && !outfitNameOrder.empty()) {
		activeOutfit = outfitNameOrder.front();
		BodySlideConfig.SetValue("SelectedOutfit", activeOutfit);
	}

	sliderView->Freeze();
	sliderView->ClearOutfitList();
	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	if (preview)
		preview->Close();

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
		PopulatePresetList(activePreset);
		ActivatePreset(activePreset);

		wxLogMessage("Finished setting up '%s'.", activeOutfit);
	}

	PopulateOutfitList(activeOutfit);

	sliderView->Thaw();
	sliderView->Layout();

	if (!cmdGroupBuild.IsEmpty())
		GroupBuild(cmdGroupBuild.ToUTF8().data());
}

int BodySlideApp::CreateSetSliders(const std::string& outfit) {
	wxLogMessage("Creating sliders...");
	dataSets.Clear();
	if (outfitNameSource.find(outfit) == outfitNameSource.end())
		return 1;

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfitNameSource[outfit]);
	if (!sliderDoc.fail()) {
		activeSet.Clear();
		sliderManager.ClearSliders();
		if (!sliderDoc.GetSet(outfit, activeSet)) {
			activeSet.SetBaseDataPath(Config["AppDir"] + PathSepStr + "ShapeData");
			activeSet.LoadSetDiffData(dataSets);

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

std::string BodySlideApp::GetOutputDataPath() const {
	std::string res = Config["OutputDataPath"];
	return res.empty() ? Config["GameDataPath"] : res;
}

void BodySlideApp::RefreshOutfitList() {
	LoadSliderSets();
	PopulateOutfitList("");
}

int BodySlideApp::LoadSliderSets() {
	wxLogMessage("Loading all slider sets...");
	dataSets.Clear();
	outfitNameSource.clear();
	outfitNameOrder.clear();
	outFileCount.clear();

	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(Config["AppDir"]) + "/SliderSets", &files, "*.osp");
	wxDir::GetAllFiles(wxString::FromUTF8(Config["AppDir"]) + "/SliderSets", &files, "*.xml");

	for (auto &file : files) {
		std::string fileName{file.ToUTF8()};

		SliderSetFile sliderDoc;
		sliderDoc.Open(fileName);
		if (sliderDoc.fail())
			continue;

		std::string outFilePath;
		std::vector<std::string> outfitNames;
		sliderDoc.GetSetNamesUnsorted(outfitNames, false);
		for (auto &o : outfitNames) {
			if (outfitNameSource.find(o) != outfitNameSource.end())
				continue;

			outfitNameSource[o] = fileName;
			outfitNameOrder.push_back(o);

			sliderDoc.GetSetOutputFilePath(o, outFilePath);
			if (!outFilePath.empty())
				outFileCount[outFilePath].push_back(o);
		}
	}

	ungroupedOutfits.clear();
	for (auto &o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

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

void BodySlideApp::ActivatePreset(const std::string &presetName, const bool updatePreview) {
	wxLogMessage("Applying preset '%s' to sliders.", presetName);

	BodySlideConfig.SetValue("SelectedPreset", presetName);
	sliderManager.InitializeSliders(presetName);

	bool zapChanged = false;
	Slider* sliderSmall = nullptr;
	Slider* sliderBig = nullptr;
	for (int i = 0; i < sliderManager.slidersBig.size(); i++) {
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

	if (preview && updatePreview)
		zapChanged ? RebuildPreviewMeshes() : UpdatePreview();
}

void BodySlideApp::DeleteOutfit(const std::string& outfitName) {
	auto outfit = outfitNameSource.find(outfitName);
	if (outfit == outfitNameSource.end())
		return;

	int select = wxNOT_FOUND;
	auto outfitChoice = (wxChoice*)sliderView->FindWindowByName("outfitChoice", sliderView);
	if (outfitChoice)
		select = outfitChoice->GetSelection();

	wxLogMessage("Loading project file '%s'...", outfit->second);

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfit->second);
	if (!sliderDoc.fail()) {
		wxLogMessage("Deleting project '%s'...", outfitName);

		if (!sliderDoc.DeleteSet(outfit->first)) {
			if (sliderDoc.Save()) {
				RefreshOutfitList();

				if (outfitChoice) {
					int count = outfitChoice->GetCount();
					if (count > select)
						outfitChoice->Select(select);
					else if (count > select - 1)
						outfitChoice->Select(select - 1);

					ActivateOutfit(outfitChoice->GetStringSelection().ToUTF8().data());
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
	auto presetChoice = (wxChoice*)sliderView->FindWindowByName("presetChoice", sliderView);
	if (presetChoice)
		select = presetChoice->GetSelection();

	wxLogMessage("Deleting preset '%s'...", presetName);
	if (!sliderManager.DeletePreset(outputFile, presetName)) {
		LoadPresets("");
		PopulatePresetList(presetName);

		if (presetChoice) {
			int count = presetChoice->GetCount();
			if (count > select)
				presetChoice->Select(select);
			else if (count > select - 1)
				presetChoice->Select(select - 1);

			ActivatePreset(presetChoice->GetStringSelection().ToUTF8().data());
		}
	}
	else
		wxLogMessage("Failed to delete preset!");
}

void BodySlideApp::RefreshSliders() {
	Slider* slider = nullptr;
	for (int i = 0; i < sliderManager.slidersBig.size(); i++) {
		slider = &sliderManager.slidersBig[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_HI);

		slider = &sliderManager.slidersSmall[i];
		sliderView->SetSliderPosition(slider->name.c_str(), slider->value, SLIDER_LO);
	}

	if (preview)
		UpdatePreview();
}

void BodySlideApp::PopulatePresetList(const std::string& select) {
	std::vector<std::string> presets;
	wxArrayString items;
	sliderManager.GetPresetNames(presets);
	items.reserve(presets.size());
	for (int i = 0; i < presets.size(); i++)
		items.Add(wxString::FromUTF8(presets[i]));

	sliderView->PopulatePresetList(items, wxString::FromUTF8(select));
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

	for (auto &fo : filteredOutfits)
		items.Add(wxString::FromUTF8(fo));

	sliderView->PopulateOutfitList(items, wxString::FromUTF8(myselect));
}

void BodySlideApp::DisplayActiveSet() {
	if (activeSet.GenWeights())
		sliderView->ShowLowColumn(true);
	else
		sliderView->ShowLowColumn(false);

	// Category name, slider names, hidden flag
	std::vector<std::tuple<std::string, std::vector<std::string>, bool>> sliderCategories;
	std::vector<std::string> cats;
	cCollection.GetAllCategories(cats);

	// Populate category data
	for (auto &cat : cats) {
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
	for (int i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].bHidden)
			continue;

		// Find the category in the list
		bool regularSlider = true;
		int iter = 0;
		for (auto &cat : sliderCategories) {
			catSliders.push_back(std::vector<int>());
			if (find(std::get<1>(cat).begin(), std::get<1>(cat).end(), activeSet[i].name) != std::get<1>(cat).end()) {
				catSliders[iter].push_back(i);
				regularSlider = false;
				break;
			}
			iter++;
		}

		// Not in a category
		if (regularSlider)
			sliderView->AddSliderGUI(activeSet[i].name, activeSet[i].name, activeSet[i].bZap, !activeSet.GenWeights());
	}

	// Create category UI
	int iter = 0;
	if (catSliders.size() > 0) {
		for (auto &cat : sliderCategories) {
			std::string name = std::get<0>(cat);
			bool show = std::get<2>(cat);
			std::string displayName;

			if (catSliders.size() > iter && catSliders[iter].size() > 0) {
				sliderView->AddCategorySliderUI(name, show, !activeSet.GenWeights());
				if (show) {
					for (auto &s : catSliders[iter]) {
						displayName = cCollection.GetSliderDisplayName(name, activeSet[s].name);
						if (displayName.empty())
							displayName = activeSet[s].name;

						sliderView->AddSliderGUI(activeSet[s].name, displayName, activeSet[s].bZap, !activeSet.GenWeights());
					}
				}
			}
			iter++;
		}
	}
}

void BodySlideApp::EditProject(const std::string& projectName) {
	auto project = outfitNameSource.find(projectName);
	if (project == outfitNameSource.end())
		return;

	int select = wxNOT_FOUND;
	auto outfitChoice = (wxChoice*)sliderView->FindWindowByName("outfitChoice", sliderView);
	if (outfitChoice)
		select = outfitChoice->GetSelection();

	wxLogMessage("Launching Outfit Studio with project file '%s' and project '%s'...", project->second, project->first);
	LaunchOutfitStudio(wxString::Format("-proj \"%s\" \"%s\"", wxString::FromUTF8(project->first), wxString::FromUTF8(project->second)));
}

void BodySlideApp::LaunchOutfitStudio(const wxString& args) {
#ifdef WIN64
	const wxString osExec = "OutfitStudio x64.exe";
#else
	const wxString osExec = "OutfitStudio.exe";
#endif

	wxString osExecCmd = wxString::Format("\"%s\\%s\" %s", wxString::FromUTF8(Config["AppDir"]), osExec, args);

	if (!wxExecute(osExecCmd, wxEXEC_ASYNC)) {
		wxLogError("Failed to execute '%s' process.", osExecCmd);
		wxMessageBox(_("Failed to launch Outfit Studio executable!"), _("Error"), wxICON_ERROR);
	}
}

void BodySlideApp::ApplySliders(const std::string& targetShape, std::vector<Slider>& sliderSet, std::vector<Vector3>& verts, std::vector<ushort>& ZapIdx, std::vector<Vector2>* uvs) {
	for (auto &slider : sliderSet) {
		float val = slider.value;
		if (slider.zap && !slider.uv) {
			if (val > 0)
				for (int j = 0; j < slider.linkedDataSets.size(); j++)
					dataSets.GetDiffIndices(slider.linkedDataSets[j], targetShape, ZapIdx);
		}
		else {
			if (slider.invert)
				val = 1.0f - val;

			for (int j = 0; j < slider.linkedDataSets.size(); j++) {
				if (slider.uv) {
					if (uvs)
						dataSets.ApplyUVDiff(slider.linkedDataSets[j], targetShape, val, uvs);
				}
				else
					dataSets.ApplyDiff(slider.linkedDataSets[j], targetShape, val, &verts);
			}
		}
	}

	for (auto &slider : sliderSet)
		if (slider.clamp && slider.value > 0)
			for (int j = 0; j < slider.linkedDataSets.size(); j++)
				dataSets.ApplyClamp(slider.linkedDataSets[j], targetShape, &verts);
}

bool BodySlideApp::WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, NifFile& nif, std::unordered_map<std::string, std::vector<ushort>>& zapIndices) {
	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	std::string triFilePath = triPath + ".tri";

	for (auto targetShape = sliderSet.ShapesBegin(); targetShape != sliderSet.ShapesEnd(); ++targetShape) {
		auto shape = nif.FindBlockByName<NiShape>(targetShape->first);
		if (!shape)
			continue;

		const std::vector<ushort>& shapeZapIndices = zapIndices[targetShape->first];

		int shapeVertCount = shape->GetNumVertices();
		shapeVertCount += shapeZapIndices.size();

		if (shapeVertCount <= 0)
			continue;

		if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= shapeVertCount)
			continue;

		for (int s = 0; s < sliderSet.size(); s++) {
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

					for (int i = shapeZapIndices.size() - 1; i >= 0; i--)
						uvs.erase(uvs.begin() + shapeZapIndices[i]);

					int i = 0;
					for (auto &uv : uvs) {
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

					for (int i = shapeZapIndices.size() - 1; i >= 0; i--)
						verts.erase(verts.begin() + shapeZapIndices[i]);

					int i = 0;
					for (auto &v : verts) {
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

void BodySlideApp::CopySliderValues(bool toHigh) {
	wxLogMessage("Copying slider values to %s weight.", toHigh ? "high" : "low");

	if (toHigh) {
		for (int i = 0; i < sliderManager.slidersSmall.size(); i++) {
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
		for (int i = 0; i < sliderManager.slidersBig.size(); i++) {
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

	if (preview)
		UpdatePreview();
}

void BodySlideApp::ShowPreview() {
	if (preview)
		return;

	int x = BodySlideConfig.GetIntValue("PreviewFrame.x");
	int y = BodySlideConfig.GetIntValue("PreviewFrame.y");
	int w = BodySlideConfig.GetIntValue("PreviewFrame.width");
	int h = BodySlideConfig.GetIntValue("PreviewFrame.height");
	std::string maximized = BodySlideConfig["PreviewFrame.maximized"];

	preview = new PreviewWindow(wxPoint(x, y), wxSize(w, h), this);
	if (maximized == "true")
		preview->Maximize();
}

void BodySlideApp::InitPreview() {
	if (!preview)
		return;

	wxLogMessage("Loading preview meshes...");
	std::string inputFileName = activeSet.GetInputFileName();
	std::string inputSetName = activeSet.GetName();
	bool freshLoad = false;

	if (previewBaseNif && (previewBaseName != inputFileName || previewSetName != inputSetName || sliderManager.NeedReload())) {
		delete previewBaseNif;
		previewBaseNif = nullptr;
	}

	if (!previewBaseNif) {
		previewBaseNif = new NifFile();
		PreviewMod.Clear();

		std::fstream file;
		PlatformUtil::OpenFileStream(file, inputFileName, std::ios::in | std::ios::binary);
		if (previewBaseNif->Load(file))
			return;

		PreviewMod.CopyFrom(*previewBaseNif);

		freshLoad = true;
		sliderManager.FlagReload(false);
	}

	previewBaseName = std::move(inputFileName);
	previewSetName = std::move(inputSetName);
	
	preview->ShowWeight(activeSet.GenWeights());

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<ushort> zapIdx;
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		zapIdx.clear();

		auto shape = previewBaseNif->FindBlockByName<NiShape>(it->first);
		if (!previewBaseNif->GetVertsForShape(shape, verts))
			continue;

		previewBaseNif->GetUvsForShape(shape, uvs);

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, verts, zapIdx, &uvs);

		// Zap deleted verts before preview
		shape = PreviewMod.FindBlockByName<NiShape>(it->first);
		if (freshLoad && zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(shape, verts);
			PreviewMod.SetUvsForShape(shape, uvs);
			if (PreviewMod.DeleteVertsForShape(shape, zapIdx))
				PreviewMod.DeleteShape(shape);
		}
		else if (zapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;
				verts.erase(verts.begin() + zapIdx[z]);
				uvs.erase(uvs.begin() + zapIdx[z]);
			}
			PreviewMod.SetVertsForShape(shape, verts);
			PreviewMod.SetUvsForShape(shape, uvs);
		}
		else {
			// No zapping needed - just show all the verts.
			PreviewMod.SetVertsForShape(shape, verts);
			PreviewMod.SetUvsForShape(shape, uvs);
		}
	}

	std::string baseGamePath = Config["GameDataPath"];
	preview->AddMeshFromNif(&PreviewMod);
	preview->SetBaseDataPath(baseGamePath);

	UpdateMeshesFromSet();

	for (auto &s : PreviewMod.GetShapeNames())
		preview->AddNifShapeTextures(&PreviewMod, s);

	preview->SetNormalsGenerationLayers(activeSet.GetNormalsGenLayers());

	preview->Refresh();
}

void BodySlideApp::UpdatePreview() {
	if (!preview)
		return;

	if (!previewBaseNif)
		return;
	
	int weight = preview->GetWeight();
	std::vector<Vector3> verts, vertsLow, vertsHigh;
	std::vector<Vector2> uvs, uvsLow, uvsHigh;
	std::vector<ushort> zapIdx;
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		zapIdx.clear();

		auto shape = previewBaseNif->FindBlockByName<NiShape>(it->first);
		if (!previewBaseNif->GetVertsForShape(shape, verts))
			continue;

		previewBaseNif->GetUvsForShape(shape, uvs);
		vertsHigh = verts;
		vertsLow = verts;
		uvsHigh = uvs;
		uvsLow = uvs;

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, vertsHigh, zapIdx, &uvsHigh);
		if (activeSet.GenWeights())
			ApplySliders(it->second.targetShape, sliderManager.slidersSmall, vertsLow, zapIdx, &uvsLow);

		// Calculate result of weight
		auto uvsz = uvs.size();
		for (int i = 0; i < verts.size(); i++) {
			verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));
			if (uvsz > i)
				uvs[i] = (uvsHigh[i] / 100.0f * weight) + (uvsLow[i] / 100.0f * (100.0f - weight));
		}

		// Zap deleted verts before applying to the shape
		if (zapIdx.size() > 0) {
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;

				verts.erase(verts.begin() + zapIdx[z]);
				uvs.erase(uvs.begin() + zapIdx[z]);
			}
		}
		preview->UpdateMeshes(it->first, &verts, &uvs);
	}

	preview->SetNormalsGenerationLayers(activeSet.GetNormalsGenLayers());

	preview->Render();
}

void BodySlideApp::CleanupPreview() {
	if (!preview)
		return;

	preview->Cleanup();
}

void BodySlideApp::RebuildPreviewMeshes() {
	if (!preview)
		return;

	if (!previewBaseNif)
		return;

	int weight = preview->GetWeight();
	PreviewMod.CopyFrom((*previewBaseNif));
	
	std::vector<Vector3> verts, vertsLow, vertsHigh;
	std::vector<Vector2> uvs, uvsLow, uvsHigh;
	std::vector<ushort> zapIdx;
	Vector3 v;
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		zapIdx.clear();

		auto shape = previewBaseNif->FindBlockByName<NiShape>(it->first);
		if (!previewBaseNif->GetVertsForShape(shape, verts))
			continue;

		previewBaseNif->GetUvsForShape(shape, uvs);
		vertsHigh = verts;
		vertsLow = verts;
		uvsHigh = uvs;
		uvsLow = uvs;

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, vertsHigh, zapIdx, &uvsHigh);
		if (activeSet.GenWeights())
			ApplySliders(it->second.targetShape, sliderManager.slidersSmall, vertsLow, zapIdx, &uvsLow);
		
		// Calculate result of weight
		for (int i = 0; i < verts.size(); i++) {
			verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));
			uvs[i] = (uvsHigh[i] / 100.0f * weight) + (uvsLow[i] / 100.0f * (100.0f - weight));
		}

		// Zap deleted verts before preview
		shape = PreviewMod.FindBlockByName<NiShape>(it->first);
		if (zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(shape, verts);
			PreviewMod.SetUvsForShape(shape, uvs);
			if (PreviewMod.DeleteVertsForShape(shape, zapIdx))
				PreviewMod.DeleteShape(shape);
		}
		else {
			// No zapping needed - just show all the verts.
			PreviewMod.SetVertsForShape(shape, verts);
			PreviewMod.SetUvsForShape(shape, uvs);
		}
	}

	preview->RefreshMeshFromNif(&PreviewMod);
	UpdateMeshesFromSet();
}

void BodySlideApp::UpdateMeshesFromSet() {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		mesh* m = preview->GetMesh(it->first);
		if (m) {
			m->smoothSeamNormals = it->second.smoothSeamNormals;
			m->lockNormals = it->second.lockNormals;

			m->SmoothNormals();
		}
	}
}

void BodySlideApp::ApplyReferenceNormals(NifFile& nif) {
	for (auto &s : nif.GetShapes()) {
		std::string shapeName = s->GetName();

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
	Config.SetDefaultBoolValue("WarnBatchBuildOverride", true);
	Config.SetDefaultBoolValue("BSATextureScan", true);
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultBoolValue("UseSystemLanguage", false);
	BodySlideConfig.SetDefaultValue("SelectedOutfit", "");
	BodySlideConfig.SetDefaultValue("SelectedPreset", "");
	BodySlideConfig.SetDefaultBoolValue("BuildMorphs", false);
	Config.SetDefaultValue("Input/SliderMinimum", 0);
	Config.SetDefaultValue("Input/SliderMaximum", 100);
	Config.SetDefaultBoolValue("Input/LeftMousePan", false);
	Config.SetDefaultValue("Lights/Ambient", 20);
	Config.SetDefaultValue("Lights/Frontal", 20);
	Config.SetDefaultValue("Lights/Directional0", 60);
	Config.SetDefaultValue("Lights/Directional0.x", -90);
	Config.SetDefaultValue("Lights/Directional0.y", 10);
	Config.SetDefaultValue("Lights/Directional0.z", 100);
	Config.SetDefaultValue("Lights/Directional1", 60);
	Config.SetDefaultValue("Lights/Directional1.x", 70);
	Config.SetDefaultValue("Lights/Directional1.y", 10);
	Config.SetDefaultValue("Lights/Directional1.z", 100);
	Config.SetDefaultValue("Lights/Directional2", 85);
	Config.SetDefaultValue("Lights/Directional2.x", 30);
	Config.SetDefaultValue("Lights/Directional2.y", 20);
	Config.SetDefaultValue("Lights/Directional2.z", -100);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.width", 800);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.height", 600);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.x", 100);
	BodySlideConfig.SetDefaultValue("BodySlideFrame.y", 100);

	wxSize previewSize(720 + xborder * 2, 720 + yborder * 2);
	BodySlideConfig.SetDefaultValue("PreviewFrame.width", previewSize.GetWidth());
	BodySlideConfig.SetDefaultValue("PreviewFrame.height", previewSize.GetHeight());
	BodySlideConfig.SetDefaultValue("PreviewFrame.x", 100);
	BodySlideConfig.SetDefaultValue("PreviewFrame.y", 100);

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

	wxString gameKey = Config["GameRegKey/" + TargetGames[targetGame]];
	wxString gameValueKey = Config["GameRegVal/" + TargetGames[targetGame]];

	if (Config["GameDataPath"].empty()) {
#ifdef _WINDOWS
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data").Append(PathSepChar);
				Config.SetDefaultValue("GameDataPath", installPath.ToUTF8().data());
				wxLogMessage("Registry game data path: %s", installPath);
			}
			else if (Config["WarnMissingGamePath"] == "true") {
				wxLogWarning("Failed to find game install path registry value or GameDataPath in the config.");
				wxMessageBox(_("Failed to find game install path registry value or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
			}
		}
		else
#endif
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

	return true;
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
		setup->SetSize(wxSize(700, -1));
		setup->CenterOnScreen();

		wxButton* btFallout3 = XRCCTRL(*setup, "btFallout3", wxButton);
		btFallout3->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(0); });

		wxButton* btFalloutNV = XRCCTRL(*setup, "btFalloutNV", wxButton);
		btFalloutNV->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(1); });

		wxButton* btSkyrim = XRCCTRL(*setup, "btSkyrim", wxButton);
		btSkyrim->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(2); });

		wxButton* btFallout4 = XRCCTRL(*setup, "btFallout4", wxButton);
		btFallout4->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(3); });

		wxButton* btSkyrimSE = XRCCTRL(*setup, "btSkyrimSE", wxButton);
		btSkyrimSE->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(4); });

		wxButton* btFallout4VR = XRCCTRL(*setup, "btFallout4VR", wxButton);
		btFallout4VR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(5); });

		wxButton* btSkyrimVR = XRCCTRL(*setup, "btSkyrimVR", wxButton);
		btSkyrimVR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(6); });

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

		wxFileName dir = GetGameDataPath(FO3);
		if (dir.DirExists()) {
			dirFallout3->SetDirName(dir);
			btFallout3->Enable();
		}

		dir = GetGameDataPath(FONV);
		if (dir.DirExists()) {
			dirFalloutNV->SetDirName(dir);
			btFalloutNV->Enable();
		}

		dir = GetGameDataPath(SKYRIM);
		if (dir.DirExists()) {
			dirSkyrim->SetDirName(dir);
			btSkyrim->Enable();
		}

		dir = GetGameDataPath(FO4);
		if (dir.DirExists()) {
			dirFallout4->SetDirName(dir);
			btFallout4->Enable();
		}

		dir = GetGameDataPath(SKYRIMSE);
		if (dir.DirExists()) {
			dirSkyrimSE->SetDirName(dir);
			btSkyrimSE->Enable();
		}

		dir = GetGameDataPath(FO4VR);
		if (dir.DirExists()) {
			dirFallout4VR->SetDirName(dir);
			btFallout4VR->Enable();
		}

		dir = GetGameDataPath(SKYRIMVR);
		if (dir.DirExists()) {
			dirSkyrimVR->SetDirName(dir);
			btSkyrimVR->Enable();
		}

		if (setup->ShowModal() != wxID_CANCEL) {
			int targ = setup->GetReturnCode();
			Config.SetValue("TargetGame", targ);

			wxFileName dataDir;
			switch (targ) {
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
			}

			Config.SetValue("GameDataPath", dataDir.GetFullPath().ToUTF8().data());
			Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToUTF8().data());

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

wxString BodySlideApp::GetGameDataPath(TargetGame targ) {
	wxString dataPath;
	wxString gamestr = TargetGames[targ];
	wxString gkey = "GameRegKey/" + gamestr;
	wxString gval = "GameRegVal/" + gamestr;
	wxString cust = "GameDataPaths/" + gamestr;

	if (!Config[cust].IsEmpty()) {
		dataPath = Config[cust];
	}
#ifdef _WINDOWS
	else {
		wxRegKey key(wxRegKey::HKLM, Config[gkey], wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			if (key.HasValues() && key.QueryValue(Config[gval], dataPath)) {
				dataPath.Append("Data").Append(PathSepChar);
			}
		}
	}
#endif
	return dataPath;
}

void BodySlideApp::InitLanguage() {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");

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
	cCollection.LoadCategories(Config["AppDir"] + "/SliderCategories");
}

void BodySlideApp::SetPresetGroups(const std::string& setName) {
	presetGroups.clear();
	gCollection.GetOutfitGroups(setName, presetGroups);

	if (presetGroups.empty()) {
		Config.GetValueArray("DefaultGroups", "GroupName", presetGroups);
		if (!presetGroups.empty()) {
			wxString defaultGroups;
			for (auto &group : presetGroups)
				defaultGroups.Append("'" + group + "' ");

			wxLogMessage("Using default group(s): %s", defaultGroups);
		}
		else
			wxLogMessage("No group assigned for set '%s'.", setName);
	}
	else {
		wxString groups;
		for (auto &group : presetGroups)
			groups.Append("'" + group + "' ");

		wxLogMessage("Using group(s): %s", groups);
	}
}

void BodySlideApp::LoadAllGroups() {
	wxLogMessage("Loading all slider groups...");
	gCollection.LoadGroups(Config["AppDir"] + "/SliderGroups");

	ungroupedOutfits.clear();
	for (auto &o : outfitNameSource) {
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
		for (int i = 0; i < aliases.size(); i++)
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
	for (auto &e : existing) {
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

void BodySlideApp::ApplyOutfitFilter() {
	bool showUngrouped = false;
	filteredOutfits.clear();
	std::unordered_set<std::string> grpFiltOutfits;
	std::vector<std::string> workfiltList;
	static wxString lastGrps = "";
	static std::set<std::string> grouplist;


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
				std::string group = token;
				grouplist.insert(group);
			}
		}
		lastGrps = grpSrch;
	}
	else if (lastGrps.empty()) {
		gCollection.GetAllGroups(grouplist);
		showUngrouped = true;
	}

	for (auto &gn : grouplist) {
		if (gn == "Unassigned")
			showUngrouped = true;
		else
			gCollection.GetGroupMembers(gn, grpFiltOutfits);
	}

	if (showUngrouped)
		for (auto &ug : ungroupedOutfits)
			grpFiltOutfits.insert(ug);

	for (auto &no : outfitNameOrder)
		if (grpFiltOutfits.find(no) != grpFiltOutfits.end())
			workfiltList.push_back(no);


	if (outfitSrch.empty()) {
		for (auto &w : workfiltList)
			filteredOutfits.push_back(w);
	}
	else {
		std::regex re;
		try {
			re.assign(outfitSrch, std::regex::icase);
			for (auto &w : workfiltList)
				if (std::regex_search(w, re))
					filteredOutfits.push_back(w);
		}
		catch (std::regex_error&) {
			for (auto &w : workfiltList)
				filteredOutfits.push_back(w);
		}
	}

	BodySlideConfig.SetValue("LastGroupFilter", std::string(grpSrch));
	BodySlideConfig.SetValue("LastOutfitFilter", outfitSrch);
}

int BodySlideApp::GetOutfits(std::vector<std::string>& outList) {
	outList.assign(outfitNameOrder.begin(), outfitNameOrder.end());
	return outList.size();
}

int BodySlideApp::GetFilteredOutfits(std::vector<std::string>& outList) {
	outList.assign(filteredOutfits.begin(), filteredOutfits.end());
	return outList.size();
}

void BodySlideApp::LoadPresets(const std::string& sliderSet) {
	std::string outfit = sliderSet;
	if (sliderSet.empty())
		outfit = BodySlideConfig["SelectedOutfit"];

	wxLogMessage("Loading assigned presets...");

	std::vector<std::string> groups_and_aliases;
	for (auto &g : presetGroups) {
		groups_and_aliases.push_back(g);
		for (auto &ag : this->groupAlias)
			if (ag.second == g)
				groups_and_aliases.push_back(ag.first);
	}

	sliderManager.LoadPresets(Config["AppDir"] + "/SliderPresets", outfit, groups_and_aliases, groups_and_aliases.empty());
}

int BodySlideApp::BuildBodies(bool localPath, bool clean, bool tri, bool forceNormals) {
	std::string inputFileName = activeSet.GetInputFileName();
	NifFile nifSmall;
	NifFile nifBig;

	std::string outFileNameSmall;
	std::string outFileNameBig;

	wxLogMessage("Building set '%s' with options: Local Path = %s, Cleaning = %s, TRI = %s, GenWeights = %s",
		activeSet.GetName(), localPath ? "True" : "False", clean ? "True" : "False", tri ? "True" : "False", activeSet.GenWeights() ? "True" : "False");

	if (localPath) {
		outFileNameSmall = outFileNameBig = activeSet.GetOutputFile();
	}
	else {
		if (GetOutputDataPath().empty()) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Would you like to show BodySlide where it is?"), _("Game not found"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
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
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folder, potentially causing crashes.\n\nDo you want to continue?"), _("Clean Build"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
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

	std::vector<Vector3> vertsLow;
	std::vector<Vector3> vertsHigh;
	std::vector<Vector2> uvsLow;
	std::vector<Vector2> uvsHigh;
	std::vector<ushort> zapIdx;
	std::unordered_map<std::string, std::vector<ushort>> zapIdxAll;

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

		zapIdxAll.emplace(it->first, std::vector<ushort>());

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, vertsHigh, zapIdx, &uvsHigh);
		nifBig.SetVertsForShape(shape, vertsHigh);
		nifBig.SetUvsForShape(shape, uvsHigh);

		if (!it->second.lockNormals) {
			nifBig.CalcNormalsForShape(shape, forceNormals, it->second.smoothSeamNormals);

			if (forceNormals)
				ApplyReferenceNormals(nifBig);
		}

		nifBig.CalcTangentsForShape(shape);
		if (nifBig.DeleteVertsForShape(shape, zapIdx))
			nifBig.DeleteShape(shape);

		if (activeSet.GenWeights()) {
			zapIdx.clear();
			ApplySliders(it->second.targetShape, sliderManager.slidersSmall, vertsLow, zapIdx, &uvsLow);

			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			nifSmall.SetVertsForShape(shapeSmall, vertsLow);
			nifSmall.SetUvsForShape(shapeSmall, uvsLow);

			if (!it->second.lockNormals) {
				nifSmall.CalcNormalsForShape(shapeSmall, forceNormals, it->second.smoothSeamNormals);

				if (forceNormals)
					ApplyReferenceNormals(nifSmall);
			}

			nifSmall.CalcTangentsForShape(shapeSmall);
			if (nifSmall.DeleteVertsForShape(shapeSmall, zapIdx))
				nifSmall.DeleteShape(shapeSmall);
		}

		zapIdxAll[it->first] = zapIdx;
		zapIdx.clear();
	}

	/* Add TRI path for in-game morphs */
	if (tri) {
		std::string triPath = activeSet.GetOutputFilePath() + ".tri";
		std::string triPathTrimmed = triPath;
		triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex("/+|\\\\+"), "\\");									// Replace multiple backslashes or forward slashes with one backslash
		triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex(".*meshes\\\\", std::regex_constants::icase), "");	// Remove everything before and including the meshes path

		if (!WriteMorphTRI(outFileNameBig, activeSet, nifBig, zapIdxAll)) {
			wxLogError("Failed to write TRI file to '%s'!", triPath);
			wxMessageBox(wxString().Format(_("Failed to write TRI file to the following location\n\n%s"), triPath), _("Unable to process"), wxOK | wxICON_ERROR);
		}

		if (targetGame != FO4 && targetGame != FO4VR && targetGame != FO76) {
			for (auto targetShape = activeSet.ShapesBegin(); targetShape != activeSet.ShapesEnd(); ++targetShape) {
				auto shape = nifBig.FindBlockByName<NiShape>(targetShape->first);
				if (!shape)
					continue;

				if (tri && shape->GetNumVertices() > 0) {
					AddTriData(nifBig, targetShape->first, triPathTrimmed);
					if (activeSet.GenWeights())
						AddTriData(nifSmall, targetShape->first, triPathTrimmed);

					tri = false;
				}
			}
		}
		else {
			AddTriData(nifBig, "", triPathTrimmed, true);
			if (activeSet.GenWeights())
				AddTriData(nifSmall, "", triPathTrimmed, true);
		}

		// Set all shapes to dynamic/mutable
		for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
			nifBig.SetShapeDynamic(it->first);
			if (activeSet.GenWeights())
				nifSmall.SetShapeDynamic(it->first);
		}
	}
	else {
		wxString triPath = wxString::FromUTF8(outFileNameBig + ".tri");
		if (wxFileName::FileExists(triPath))
			wxRemoveFile(triPath);
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

int BodySlideApp::BuildListBodies(std::vector<std::string>& outfitList, std::map<std::string, std::string>& failedOutfits, bool clean, bool tri, bool forceNormals, const std::string& custPath) {
	std::string datapath = custPath;

	wxLogMessage("Started batch build with options: Custom Path = %s, Cleaning = %s, TRI = %s",
		custPath.empty() ? "False" : custPath, clean ? "True" : "False", tri ? "True" : "False");

	if (clean) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?"), _("Clean Batch Build"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
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
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Continue saving files to the working directory?"), _("Game not found"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogError("Aborted batch build with unconfigured data path.");
					return 1;
				}
			}

			Config.SetValue("GameDataPath", Config["AppDir"] + PathSepStr);
		}

		datapath = GetOutputDataPath();
	}

	if (Config.MatchValue("WarnBatchBuildOverride", "true")) {
		std::vector<std::string> outFileList;
		std::vector<wxArrayString> choicesList;
		for (auto &outFile : outFileCount) {
			if (outFile.second.size() > 1) {
				wxArrayString selOutfits;
				for (auto &outfit : outFile.second) {
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
			std::string buildSelFileName = Config["AppDir"] + PathSepStr + "BuildSelection.xml";

			BuildSelectionFile buildSelFile(buildSelFileName);

			if (buildSelFile.GetError())
				buildSelFile.New(buildSelFileName);

			BuildSelection buildSelection;
			buildSelFile.Get(buildSelection);

			wxXmlResource* rsrc = wxXmlResource::Get();
			wxDialog* dlgBuildOverride = rsrc->LoadDialog(sliderView, "dlgBuildOverride");
			dlgBuildOverride->SetSize(wxSize(650, 400));
			dlgBuildOverride->SetSizeHints(wxSize(650, 400), wxSize(650, -1));
			dlgBuildOverride->CenterOnParent();

			wxScrolledWindow* scrollOverrides = XRCCTRL(*dlgBuildOverride, "scrollOverrides", wxScrolledWindow);
			wxBoxSizer* choicesSizer = (wxBoxSizer*)scrollOverrides->GetSizer();

			int nChoice = 1;
			std::vector<wxRadioBox*> choiceBoxes;
			for (int i = 0; i < choicesList.size(); i++) {
				auto& outFile = outFileList[i];
				auto& choices = choicesList[i];
				wxRadioBox* choiceBox = new wxRadioBox(scrollOverrides, wxID_ANY, _("Choose output set") + wxString::Format(" #%d", nChoice), wxDefaultPosition, wxDefaultSize, choices, 1, wxRA_SPECIFY_COLS);
				choicesSizer->Add(choiceBox, 0, wxALL | wxEXPAND, 5);

				choiceBoxes.push_back(choiceBox);
				nChoice++;

				// Check previous choices to see if radio button should be checked by default
				std::string outputChoice = buildSelection.GetOutputChoice(outFile);
				if (!outputChoice.empty()) {
					wxString c = wxString::FromUTF8(outputChoice);
					if (choices.Index(c) != wxNOT_FOUND)
						choiceBox->SetStringSelection(c);
				}
			}

			scrollOverrides->FitInside();

			if (dlgBuildOverride->ShowModal() == wxID_CANCEL) {
				wxLogMessage("Aborted batch build by not choosing a file override.");
				delete dlgBuildOverride;
				return 1;
			}

			for (int i = 0; i < choicesList.size(); i++) {
				wxString choiceSel = choiceBoxes[i]->GetStringSelection();

				// Add output choice to file
				buildSelection.SetOutputChoice(outFileList[i], choiceSel.ToUTF8().data());

				// Remove others from the list of outfits to build
				choicesList[i].Remove(choiceSel);

				for (auto &outfit : choicesList[i]) {
					auto result = std::find(outfitList.begin(), outfitList.end(), outfit.ToUTF8());
					if (result != outfitList.end())
						outfitList.erase(result);
				}
			}

			delete dlgBuildOverride;

			// Save output choices to file
			buildSelFile.Update(buildSelection);
			buildSelFile.Save();
		}
	}

	refNormalsCache.clear();

	wxProgressDialog progWnd(_("Processing Outfits"), _("Starting..."), 1000, sliderView, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
	progWnd.SetSize(400, 150);
	float progstep = 1000.0f / outfitList.size();
	std::atomic<int> count = 0;

	// Multi-threading for 64-bit only due to memory limits of 32-bit builds
#ifdef _PPL_H
	// Parallel loop is run inside a task
	concurrency::concurrent_unordered_map<std::string, std::string> failedOutfitsCon;
	auto buildTask = concurrency::create_task([&] {
	concurrency::parallel_for_each(outfitList.begin(), outfitList.end(), [&](const std::string& outfit)
#else
	#define return continue
	std::unordered_map<std::string, std::string> failedOutfitsCon;
	for (auto &outfit : outfitList)
#endif
	{
		wxString progMsg = wxString::Format(_("Processing '%s' (%d of %d)..."), outfit, ++count, (int)outfitList.size());
		progWnd.Update((int)(count * progstep) - 1, progMsg);
		progWnd.Fit();

		wxLogMessage(progMsg);

		/* Load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			failedOutfitsCon[outfit] = _("No recorded outfit name source");
			return;
		}

		SliderSet currentSet;
		DiffDataSets currentDiffs;

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			if (sliderDoc.GetSet(outfit, currentSet)) {
				failedOutfitsCon[outfit] = _("Unable to get slider set from file: ") + outfitNameSource[outfit];
				return;
			}
		}
		else {
			failedOutfitsCon[outfit] = _("Unable to open slider set file: ") + outfitNameSource[outfit];
			return;
		}

		currentSet.SetBaseDataPath(Config["AppDir"] + PathSepStr + "ShapeData");

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
			failedOutfitsCon[outfit] = _("Unable to load input nif: ") + currentSet.GetInputFileName();
			return;
		}

		if (currentSet.GenWeights())
			nifSmall.CopyFrom(nifBig);

		currentSet.LoadSetDiffData(currentDiffs);

		/* Shape the NIF files */
		std::vector<Vector3> vertsLow;
		std::vector<Vector3> vertsHigh;
		std::vector<Vector2> uvsLow;
		std::vector<Vector2> uvsHigh;
		std::vector<ushort> zapIdx;
		std::unordered_map<std::string, std::vector<ushort>> zapIdxAll;

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
			std::vector<int> clamps;
			zapIdxAll.emplace(it->first, std::vector<ushort>());

			for (int s = 0; s < currentSet.size(); s++) {
				std::string target = it->second.targetShape;
				std::string dn = currentSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				if (currentSet[s].bClamp)  {
					clamps.push_back(s);
					continue;
				}

				vbig = sliderManager.GetBigPresetValue(activePreset, currentSet[s].name, currentSet[s].defBigValue / 100.0f);
				for (auto &sliderBig : sliderManager.slidersBig) {
					if (sliderBig.name == currentSet[s].name && sliderBig.changed && !sliderBig.clamp) {
						vbig = sliderBig.value;
						break;
					}
				}

				if (currentSet.GenWeights()) {
					vsmall = sliderManager.GetSmallPresetValue(activePreset, currentSet[s].name, currentSet[s].defSmallValue / 100.0f);
					for (auto &sliderSmall : sliderManager.slidersSmall) {
						if (sliderSmall.name == currentSet[s].name && sliderSmall.changed && !sliderSmall.clamp) {
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
				for (auto &c : clamps) {
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

			if (!it->second.lockNormals) {
				nifBig.CalcNormalsForShape(shape, forceNormals, it->second.smoothSeamNormals);

				if (forceNormals)
					ApplyReferenceNormals(nifBig);
			}

			nifBig.CalcTangentsForShape(shape);
			if (nifBig.DeleteVertsForShape(shape, zapIdx))
				nifBig.DeleteShape(shape);

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				nifSmall.SetVertsForShape(shapeSmall, vertsLow);
				nifSmall.SetUvsForShape(shapeSmall, uvsLow);

				if (!it->second.lockNormals) {
					nifSmall.CalcNormalsForShape(shapeSmall, forceNormals, it->second.smoothSeamNormals);

					if (forceNormals)
						ApplyReferenceNormals(nifSmall);
				}

				nifSmall.CalcTangentsForShape(shapeSmall);
				if (nifSmall.DeleteVertsForShape(shapeSmall, zapIdx))
					nifSmall.DeleteShape(shapeSmall);
			}

			zapIdx.clear();
		}

		currentDiffs.Clear();

		/* Create directory for the outfit */
		wxString dir = wxString::FromUTF8(datapath + currentSet.GetOutputPath());
		bool success = wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

		if (!success) {
			failedOutfitsCon[outfit] = _("Unable to create destination directory: ") + dir.ToUTF8().data();
			return;
		}

		std::string outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		std::string outFileNameBig = outFileNameSmall;

		/* Add TRI path for in-game morphs */
		bool triEnd = tri;
		if (triEnd) {
			std::string triPath = currentSet.GetOutputFilePath() + ".tri";
			std::string triPathTrimmed = triPath;
			triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex("/+|\\\\+"), "\\");									// Replace multiple backslashes or forward slashes with one backslash
			triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex(".*meshes\\\\", std::regex_constants::icase), "");	// Remove everything before and including the meshes path

			if (!WriteMorphTRI(outFileNameBig, currentSet, nifBig, zapIdxAll))
				wxLogError("Failed to create TRI file to '%s'!", triPath);

			if (targetGame != FO4 && targetGame != FO4VR && targetGame != FO76) {
				for (auto targetShape = currentSet.ShapesBegin(); targetShape != currentSet.ShapesEnd(); ++targetShape) {
					auto shape = nifBig.FindBlockByName<NiShape>(targetShape->first);
					if (!shape)
						continue;

					if (triEnd && shape->GetNumVertices() > 0) {
						AddTriData(nifBig, targetShape->first, triPathTrimmed);
						if (currentSet.GenWeights())
							AddTriData(nifSmall, targetShape->first, triPathTrimmed);

						triEnd = false;
					}
				}
			}
			else {
				AddTriData(nifBig, "", triPathTrimmed, true);
				if (currentSet.GenWeights())
					AddTriData(nifSmall, "", triPathTrimmed, true);
			}

			// Set all shapes to dynamic/mutable
			for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
				nifBig.SetShapeDynamic(it->first);
				if (currentSet.GenWeights())
					nifSmall.SetShapeDynamic(it->first);
			}
		}
		else {
			std::string triPath = outFileNameBig + ".tri";
			if (wxFileName::FileExists(triPath))
				wxRemoveFile(triPath);
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
				failedOutfitsCon[outfit] = _("Unable to save nif file: ") + outFileNameBig;
				return;
			}

			std::fstream fileSmall;
			PlatformUtil::OpenFileStream(fileSmall, outFileNameSmall, std::ios::out | std::ios::binary);

			if (nifSmall.Save(fileSmall, nifOptions)) {
				failedOutfitsCon[outfit] = _("Unable to save nif file: ") + outFileNameSmall;
				return;
			}
		}
		else {
			outFileNameBig += ".nif";

			std::fstream fileBig;
			PlatformUtil::OpenFileStream(fileBig, outFileNameBig, std::ios::out | std::ios::binary);

			if (nifBig.Save(fileBig, nifOptions)) {
				failedOutfitsCon[outfit] = _("Unable to save nif file: ") + outFileNameBig;
				return;
			}
		}
#ifdef _PPL_H
	});
	});

	// Yield outside of task
	while (!buildTask.is_done()) {
		Yield();
		wxMilliSleep(100);
	}
#else
	}
#undef return
#endif

	progWnd.Update(1000);

	failedOutfits.insert(failedOutfitsCon.begin(), failedOutfitsCon.end());

	if (failedOutfits.size() > 0)
		return 3;

	return 0;
}

void BodySlideApp::GroupBuild(const std::string& group) {
	std::vector<std::string> outfits;
	for (auto &o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (find(groups.begin(), groups.end(), group) != groups.end())
			outfits.push_back(o.first);
	}

	std::string preset;
	if (!cmdPreset.IsEmpty()) {
		preset = BodySlideConfig["SelectedPreset"];
		BodySlideConfig.SetValue("SelectedPreset", cmdPreset.ToUTF8().data());
	}

	std::vector<std::string> groups;
	sliderManager.LoadPresets(Config["AppDir"] + "/SliderPresets", "", groups, true);

	std::map<std::string, std::string> failedOutfits;
	int ret = BuildListBodies(outfits, failedOutfits, false, cmdTri, false, cmdTargetDir.ToUTF8().data());

	if (!cmdPreset.IsEmpty())
		BodySlideConfig.SetValue("SelectedPreset", preset);

	wxLog::FlushActive();

	if (ret == 0) {
		wxLogMessage("All group build sets processed successfully!");
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto &e : failedOutfits) {
			wxString ename = wxString::FromUTF8(e.first);
			wxLogError("Failed to build '%s': %s", ename, e.second);
			errlist.Add(ename + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(sliderView, _("The following sets failed"), _("Failed"), errlist, nullptr, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
		errdisplay.ShowModal();
	}

	sliderView->Close(true);
}

void BodySlideApp::AddTriData(NifFile& nif, const std::string& shapeName, const std::string& triPath, bool toRoot) {
	NiAVObject* target = nullptr;

	if (toRoot)
		target = nif.GetRootNode();
	else
		target = nif.FindBlockByName<NiShape>(shapeName);

	if (target) {
		auto triExtraData = new NiStringExtraData();
		triExtraData->SetName("BODYTRI");
		triExtraData->SetStringData(triPath);
		nif.AssignExtraData(target, triExtraData);
	}
}

float BodySlideApp::GetSliderValue(const wxString& sliderName, bool isLo) {
	std::string sstr{sliderName.ToUTF8()};
	return sliderManager.GetSlider(sstr, isLo);
}

bool BodySlideApp::IsUVSlider(const wxString& sliderName) {
	std::string sstr{sliderName.ToUTF8()};
	return activeSet[sstr].bUV;
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

BodySlideFrame::BodySlideFrame(BodySlideApp* a, const wxSize &size) : delayLoad(this, DELAYLOAD_TIMER) {
	app = a;
	rowCount = 0;

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

	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/BatchBuild.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Settings.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/About.xrc");

	delayLoad.Start(100, true);

	SetDoubleBuffered(true);
	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/BodySlide.png", wxBITMAP_TYPE_PNG));
	SetSize(size);

	batchBuildList = nullptr;
	wxMenu* srchMenu = xrc->LoadMenu("menuGroupContext");
	wxMenu* outfitsrchMenu = xrc->LoadMenu("menuOutfitSrchContext");

	search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->ShowCancelButton(true);
	search->SetDescriptiveText(_("Group Filter"));
	search->SetToolTip(_("Filter by group"));
	search->SetMenu(srchMenu);

	outfitsearch = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	outfitsearch->ShowSearchButton(true);
	outfitsearch->ShowCancelButton(true);
	outfitsearch->SetDescriptiveText(_("Outfit Filter"));
	outfitsearch->SetToolTip(_("Filter by outfit"));
	outfitsearch->SetMenu(outfitsrchMenu);

	xrc->AttachUnknownControl("searchHolder", search, this);
	xrc->AttachUnknownControl("outfitsearchHolder", outfitsearch, this);

	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (scrollWindow) {
		scrollWindow->SetScrollRate(5, 26);
		scrollWindow->SetFocusIgnoringChildren();
		scrollWindow->Bind(wxEVT_ENTER_WINDOW, &BodySlideFrame::OnEnterSliderWindow, this);
	}

	wxString val = BodySlideConfig["LastGroupFilter"];
	search->ChangeValue(val);
	val = BodySlideConfig["LastOutfitFilter"];
	outfitsearch->ChangeValue(val);

	auto cbMorphs = XRCCTRL(*this, "cbMorphs", wxCheckBox);
	if (cbMorphs) {
		bool buildMorphsDef = BodySlideConfig.GetBoolValue("BuildMorphs");
		cbMorphs->SetValue(buildMorphsDef);

		switch (app->targetGame) {
		case SKYRIM:
		case FO4:
		case FO4VR:
		case SKYRIMSE:
		case SKYRIMVR:
			cbMorphs->Show();
			break;
		}
	}

	auto cbForceBodyNormals = XRCCTRL(*this, "cbForceBodyNormals", wxCheckBox);
	if (cbForceBodyNormals) {
		bool forceBodyNormalsDef = BodySlideConfig.GetBoolValue("ForceBodyNormals");
		cbForceBodyNormals->SetValue(forceBodyNormalsDef);

		switch (app->targetGame) {
		case SKYRIMSE:
		case SKYRIMVR:
			//cbForceBodyNormals->Show();
			break;
		}
	}
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
		if (!this->FindFocus()->IsKindOf(wxClassInfo::FindClass("wxTextCtrl"))) {
			wxScrolledWindow* sw = (wxScrolledWindow*)event.GetEventObject();
			sw->SetFocusIgnoringChildren();
		}
	}
}

void BodySlideFrame::ShowLowColumn(bool show) {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	wxFlexGridSizer* sliderLayout = (wxFlexGridSizer*)scrollWindow->GetSizer();
	if (!sliderLayout)
		return;

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

void BodySlideFrame::AddCategorySliderUI(const wxString& name, bool show, bool oneSize) {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	wxSizer* sliderLayout = scrollWindow->GetSizer();
	if (!sliderLayout)
		return;

	wxWindow* child;
	if (!oneSize) {
		sliderLayout->AddSpacer(0);

		child = new wxPanel(scrollWindow);
		child->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(child, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	wxCheckBox* check = new wxCheckBox(scrollWindow, wxID_ANY, "");
	check->SetName(name);
	sliderLayout->Add(check, 0, wxRIGHT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 5);
	check->SetValue(show);
	check->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnCategoryCheckChanged, this);

	child = new wxStaticText(scrollWindow, wxID_ANY, name);
	child->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Andalus"));
	child->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(child, 0, wxLEFT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

	if (!oneSize) {
		child = new wxPanel(scrollWindow);
		child->SetBackgroundColour(wxColour(90, 90, 90));
		sliderLayout->Add(child, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	sliderLayout->AddSpacer(0);
	scrollWindow->FitInside();
}

void BodySlideFrame::AddSliderGUI(const std::string& name, const std::string& display, bool isZap, bool oneSize) {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	wxSizer* sliderLayout = scrollWindow->GetSizer();
	if (!sliderLayout)
		return;

	wxString sliderName = wxString::FromUTF8(name);
	wxString displayName = wxString::FromUTF8(display);

	auto sd = new SliderDisplay();
	sd->isZap = isZap;
	sd->oneSize = oneSize;

	int minValue = Config.GetIntValue("Input/SliderMinimum");
	int maxValue = Config.GetIntValue("Input/SliderMaximum");

	if (!oneSize) {
		sd->lblSliderLo = new wxStaticText(scrollWindow, wxID_ANY, displayName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
		sd->lblSliderLo->SetForegroundColour(wxColour(200, 200, 200));
		sliderLayout->Add(sd->lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

		if (isZap) {
			sd->zapCheckLo = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, sliderName + "|ZLO");
			sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(sd->zapCheckLo, 0, wxALIGN_LEFT, 0);
		}
		else {
			sd->sliderLo = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 24), wxSL_AUTOTICKS | wxSL_BOTTOM | wxSL_HORIZONTAL);
			sd->sliderLo->SetTickFreq(5);
			sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
			sd->sliderLo->SetName(sliderName + "|LO");
			sliderLayout->Add(sd->sliderLo, 1, wxEXPAND, 0);

			sd->sliderReadoutLo = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(50, -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
			sd->sliderReadoutLo->SetName(sliderName + "|RLO");
			sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);
			sliderLayout->Add(sd->sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);
		}
	}

	sd->lblSliderHi = new wxStaticText(scrollWindow, wxID_ANY, displayName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	sd->lblSliderHi->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(sd->lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

	if (isZap) {
		sd->zapCheckHi = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, sliderName + "|ZHI");
		sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(sd->zapCheckHi, 0, wxALIGN_LEFT, 0);
	}
	else {
		sd->sliderHi = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 24), wxSL_AUTOTICKS | wxSL_HORIZONTAL);
		sd->sliderHi->SetTickFreq(5);
		sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
		sd->sliderHi->SetName(sliderName + "|HI");
		sliderLayout->Add(sd->sliderHi, 1, wxEXPAND, 0);

		sd->sliderReadoutHi = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(50, -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
		sd->sliderReadoutHi->SetName(sliderName + "|RHI");
		sd->sliderReadoutHi->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);
		sliderLayout->Add(sd->sliderReadoutHi, 0, wxRIGHT | wxALIGN_CENTER, 10);
	}

	sd->sliderName = name;
	sliderDisplays[sd->sliderName] = sd;

	scrollWindow->FitInside();
	rowCount++;
}

void BodySlideFrame::ClearPresetList() {
	wxChoice* presetChoice = (wxChoice*)FindWindowByName("presetChoice", this);
	if (presetChoice)
		presetChoice->Clear();
}

void  BodySlideFrame::ClearOutfitList() {
	wxChoice* outfitChoice = (wxChoice*)FindWindowByName("outfitChoice", this);
	if (outfitChoice)
		outfitChoice->Clear();
}

void BodySlideFrame::ClearSliderGUI() {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (scrollWindow) {
		scrollWindow->GetSizer()->Clear();
		scrollWindow->DestroyChildren();
	}
	for (auto &sd : sliderDisplays)
		delete sd.second;

	sliderDisplays.clear();

	rowCount = 0;
}

void BodySlideFrame::PopulateOutfitList(const wxArrayString& items, const wxString& selectItem) {
	wxChoice* outfitChoice = (wxChoice*)FindWindowByName("outfitChoice", this);
	if (!outfitChoice)
		return;

	outfitChoice->Clear();
	outfitChoice->Append(items);
	if (!outfitChoice->SetStringSelection(selectItem)) {
		int i = wxNOT_FOUND;
		if (selectItem.empty())
			i = outfitChoice->Append("");
		else if (selectItem.First('['))
			i = outfitChoice->Append("[" + selectItem + "]");
		else
			i = outfitChoice->Append(selectItem);

		outfitChoice->SetSelection(i);
	}
}

void BodySlideFrame::PopulatePresetList(const wxArrayString& items, const wxString& selectItem) {
	wxChoice* presetChoice = (wxChoice*)FindWindowByName("presetChoice", this);
	if (!presetChoice)
		return;

	presetChoice->Clear();
	presetChoice->Append(items);
	presetChoice->Select(presetChoice->FindString(selectItem));
}

void BodySlideFrame::SetSliderPosition(const wxString &name, float newValue, short HiLo) {
	int intval = (int)(newValue * 100.0f);

	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
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
	else if (!sd->oneSize){
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
	app->ClosePreview();

	auto cbMorphs = XRCCTRL(*this, "cbMorphs", wxCheckBox);
	if (cbMorphs)
		BodySlideConfig.SetBoolValue("BuildMorphs", cbMorphs->GetValue());

	auto cbForceBodyNormals = XRCCTRL(*this, "cbForceBodyNormals", wxCheckBox);
	if (cbForceBodyNormals)
		BodySlideConfig.SetBoolValue("ForceBodyNormals", cbForceBodyNormals->GetValue());

	int ret = BodySlideConfig.SaveConfig(Config["AppDir"] + "/BodySlide.xml", "BodySlideConfig");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);

	wxLogMessage("BodySlide closed.");
	Destroy();
}

void BodySlideFrame::OnActivateFrame(wxActivateEvent& event) {
	event.Skip();
	if (event.GetActive()) {
		wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
		if (!scrollWindow)
			return;

		scrollWindow->SetFocusIgnoringChildren();
	}
}

void BodySlideFrame::OnIconizeFrame(wxIconizeEvent& event) {
	event.Skip();
	if (!event.IsIconized()) {
		wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
		if (!scrollWindow)
			return;

		lastScroll = scrollWindow->GetScrollPos(wxVERTICAL);
		CallAfter(&BodySlideFrame::PostIconizeFrame);
	}
}

void BodySlideFrame::PostIconizeFrame() {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	scrollWindow->SetFocusIgnoringChildren();
	scrollWindow->Scroll(0, lastScroll);
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
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	app->SetSliderValue(name, isLo, event.GetPosition() / 100.0f);
	app->SetSliderChanged(name, isLo);
	if (isLo)
		sd->sliderReadoutLo->ChangeValue(wxString::Format("%d%%", event.GetPosition()));
	else
		sd->sliderReadoutHi->ChangeValue(wxString::Format("%d%%", event.GetPosition()));

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
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToUTF8().data());
	if (!sd)
		return;

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	w->ChangeValue(wxString::Format("%0.0f%%", v));
	app->SetSliderValue(name, isLo, (float)v / 100.0f);

	if (isLo)
		sd->sliderLo->SetValue(v);
	else
		sd->sliderHi->SetValue(v);

	app->UpdatePreview();
}

void BodySlideFrame::OnSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnOutfitSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnCategoryCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w)
		return;

	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	wxCheckBox* cb = (wxCheckBox*)event.GetEventObject();
	if (!cb)
		return;

	if (event.IsChecked())
		app->cCollection.SetCategoryHidden(cb->GetName().ToUTF8().data(), false);
	else
		app->cCollection.SetCategoryHidden(cb->GetName().ToUTF8().data(), true);

	Freeze();

	ClearSliderGUI();
	app->DisplayActiveSet();
	app->RefreshSliders();

	int scrollPos = scrollWindow->GetScrollPos(wxOrientation::wxVERTICAL);
	scrollWindow->Scroll(0, scrollPos);

	Thaw();
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
		if (event.IsChecked()) {
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
	}

	app->SetSliderChanged(sn, isLo);

	std::vector<std::string> zapToggles = app->GetSliderZapToggles(sn);
	for (auto &toggle : zapToggles) {
		wxLogMessage("Zap '%s' toggled.", sn);

		app->SetSliderValue(toggle, true, 1.0f - app->GetSliderValue(toggle, true));
		app->SetSliderValue(toggle, false, 1.0f - app->GetSliderValue(toggle, false));
		app->SetSliderChanged(toggle, true);
		app->SetSliderChanged(toggle, false);

		slider = GetSliderDisplay(toggle);
		if (slider) {
			if (slider->zapCheckHi)
				slider->zapCheckHi->SetValue(!slider->zapCheckHi->GetValue());
			if (slider->zapCheckLo)
				slider->zapCheckLo->SetValue(!slider->zapCheckLo->GetValue());
		}
	}

	wxBeginBusyCursor();
	if (app->IsUVSlider(sn))
		app->UpdatePreview();
	else
		app->RebuildPreviewMeshes();
	wxEndBusyCursor();
}

void BodySlideFrame::OnEraseBackground(wxEraseEvent &WXUNUSED(event)) {
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
	for (auto &g : groupNames) {
		grpChoices.Add(wxString::FromUTF8(g));
		if (curGroupNames.find(g) != curGroupNames.end())
			grpSelections.Add(idx);

		idx++;
	}

	grpChoices.Add("Unassigned");

	wxString filter;
	wxMultiChoiceDialog chooser(this, _("Choose groups to filter outfit list"), _("Choose Groups"), grpChoices);
	chooser.SetSelections(grpSelections);
	if (chooser.ShowModal() == wxID_OK) {
		wxArrayInt sel = chooser.GetSelections();
		for (int i = 0; i < sel.size(); i++) {
			if (i > 0) filter += ", ";
			filter += grpChoices[sel[i]];
		}
		search->ChangeValue(filter);
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnSaveGroups(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	wxFileDialog saveGroupDialog(this, _("Choose or create group file"), wxString::FromUTF8(Config["AppDir"]) + "/SliderGroups", wxEmptyString, "Group Files (*.xml)|*.xml", wxFD_SAVE);
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
		search->ChangeValue(gName);
		outfitsearch->ChangeValue("");
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnRefreshGroups(wxCommandEvent& WXUNUSED(event)) {
	app->LoadAllGroups();
}

void BodySlideFrame::OnRefreshOutfits(wxCommandEvent& WXUNUSED(event)) {
	app->RefreshOutfitList();

	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	app->ActivateOutfit(outfitName);
}

void BodySlideFrame::OnChooseOutfit(wxCommandEvent& event) {
	std::string sstr{event.GetString().ToUTF8()};
	app->ActivateOutfit(sstr);
}

void BodySlideFrame::OnChoosePreset(wxCommandEvent& event) {
	std::string sstr{event.GetString().ToUTF8()};
	app->ActivatePreset(sstr);
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

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
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

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	BodySlideConfig.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnGroupManager(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> outfits;
	app->GetOutfits(outfits);

	GroupManager gm(this, outfits);
	gm.ShowModal();
	app->LoadPresets("");
}

void BodySlideFrame::OnHighToLow(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(false);
}

void BodySlideFrame::OnLowToHigh(wxCommandEvent& WXUNUSED(event)) {
	app->CopySliderValues(true);
}

void BodySlideFrame::OnPreview(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	app->ShowPreview();
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

	wxArrayString oChoices;
	wxArrayInt oSelections;
	std::vector<std::string> outfitChoices;
	std::vector<std::string> toBuild;

	bool custpath = false;
	bool clean = false;
	bool tri = false;
	bool forceNormals = false;

	auto cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	auto cbForceBodyNormals = (wxCheckBox*)FindWindowByName("cbForceBodyNormals");
	if (cbForceBodyNormals)
		forceNormals = cbForceBodyNormals->IsChecked();

	if (wxGetKeyState(WXK_CONTROL))
		custpath = true;
	else if (wxGetKeyState(WXK_ALT))
		clean = true;

	app->GetFilteredOutfits(outfitChoices);

	int idx = 0;
	for (auto &o : outfitChoices) {
		oChoices.Add(wxString::FromUTF8(o));
		oSelections.Add(idx);
		idx++;
	}

	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* batchBuildChooser = rsrc->LoadDialog(this, "dlgBatchBuild");
	if (!batchBuildChooser)
		return;

	batchBuildChooser->SetSize(wxSize(650, 300));
	batchBuildChooser->SetSizeHints(wxSize(650, 300), wxSize(650, -1));
	batchBuildChooser->CenterOnParent();

	batchBuildList = XRCCTRL((*batchBuildChooser), "batchBuildList", wxCheckListBox);
	batchBuildList->Bind(wxEVT_RIGHT_UP, &BodySlideFrame::OnBatchBuildContext, this);
	batchBuildList->Append(oChoices);
	for (int i = 0; i < oChoices.size(); i++)
		batchBuildList->Check(i);

	if (batchBuildChooser->ShowModal() == wxID_OK) {
		wxArrayInt sel;
		batchBuildList->GetCheckedItems(sel);
		toBuild.clear();
		for (int i = 0; i < sel.size(); i++)
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
		std::string path = wxDirSelector(_("Choose a folder to contain the saved files"));
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
		for (auto &e : failedOutfits) {
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
		for (int i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i, false);
	}
	else if (event.GetId() == XRCID("batchBuildAll")) {
		for (int i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i);
	}
	else if (event.GetId() == XRCID("batchBuildInvert")) {
		for (int i = 0; i < batchBuildList->GetCount(); i++)
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
		case FO4:
		case FO4VR:
		case FO76:
		default:
			fpSkeletonFile->SetPath("res/skeleton_fo4.nif");
			choiceSkeletonRoot->SetStringSelection("Root");
			break;
	}

	wxCheckListBox* dataFileList = XRCCTRL(*parent, "DataFileList", wxCheckListBox);
	wxString dataDir = app->GetGameDataPath(targ);

	wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*parent, "dpGameDataPath", wxDirPickerCtrl);
	dpGameDataPath->SetPath(dataDir);
	
	SettingsFillDataFiles(dataFileList, dataDir, targ);
}

void BodySlideFrame::SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame) {
	dataFileList->Clear();

	wxString cp = "GameDataFiles/" + TargetGames[targetGame];
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
		settings->SetSize(wxSize(525, -1));
		settings->SetMinSize(wxSize(525, -1));
		settings->CenterOnParent();

		wxCollapsiblePane* advancedPane = XRCCTRL(*settings, "advancedPane", wxCollapsiblePane);
		advancedPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&settings](wxCommandEvent&) {
			settings->Fit();
		});

		wxChoice* choiceTargetGame = XRCCTRL(*settings, "choiceTargetGame", wxChoice);
		choiceTargetGame->Select(Config.GetIntValue("TargetGame"));

		wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*settings, "dpGameDataPath", wxDirPickerCtrl);
		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);
		dpGameDataPath->SetPath(gameDataPath);

		wxDirPickerCtrl* dpOutputPath = XRCCTRL(*settings, "dpOutputPath", wxDirPickerCtrl);
		wxString outputPath = wxString::FromUTF8(Config["OutputDataPath"]);
		dpOutputPath->SetPath(outputPath);

		wxCheckBox* cbBBOverrideWarn = XRCCTRL(*settings, "cbBBOverrideWarn", wxCheckBox);
		cbBBOverrideWarn->SetValue(Config["WarnBatchBuildOverride"] != "false");

		wxCheckBox* cbBSATextures = XRCCTRL(*settings, "cbBSATextures", wxCheckBox);
		cbBSATextures->SetValue(Config["BSATextureScan"] != "false");

		wxCheckBox* cbLeftMousePan = XRCCTRL(*settings, "cbLeftMousePan", wxCheckBox);
		cbLeftMousePan->SetValue(Config["Input/LeftMousePan"] != "false");

		wxChoice* choiceLanguage = XRCCTRL(*settings, "choiceLanguage", wxChoice);
		for (int i = 0; i < std::extent<decltype(SupportedLangs)>::value; i++)
			choiceLanguage->AppendString(wxLocale::GetLanguageName(SupportedLangs[i]));

		if (!choiceLanguage->SetStringSelection(wxLocale::GetLanguageName(Config.GetIntValue("Language"))))
			choiceLanguage->SetStringSelection("English");

		wxColourPickerCtrl* cpColorBackground = XRCCTRL(*settings, "cpColorBackground", wxColourPickerCtrl);
		if (Config.Exists("Rendering/ColorBackground")) {
			int colorBackgroundR = Config.GetIntValue("Rendering/ColorBackground.r");
			int colorBackgroundG = Config.GetIntValue("Rendering/ColorBackground.g");
			int colorBackgroundB = Config.GetIntValue("Rendering/ColorBackground.b");
			cpColorBackground->SetColour(wxColour(colorBackgroundR, colorBackgroundG, colorBackgroundB));
		}

		wxColourPickerCtrl* cpColorWire = XRCCTRL(*settings, "cpColorWire", wxColourPickerCtrl);
		if (Config.Exists("Rendering/ColorWire")) {
			int colorWireR = Config.GetIntValue("Rendering/ColorWire.r");
			int colorWireG = Config.GetIntValue("Rendering/ColorWire.g");
			int colorWireB = Config.GetIntValue("Rendering/ColorWire.b");
			cpColorWire->SetColour(wxColour(colorWireR, colorWireG, colorWireB));
		}

		wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*settings, "fpSkeletonFile", wxFilePickerCtrl);
		fpSkeletonFile->SetPath(wxString::FromUTF8(Config["Anim/DefaultSkeletonReference"]));

		wxChoice* choiceSkeletonRoot = XRCCTRL(*settings, "choiceSkeletonRoot", wxChoice);
		choiceSkeletonRoot->SetStringSelection(Config["Anim/SkeletonRootName"]);

		wxCheckListBox* dataFileList = XRCCTRL(*settings, "DataFileList", wxCheckListBox);
		SettingsFillDataFiles(dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		choiceTargetGame->Bind(wxEVT_CHOICE, &BodySlideFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
			Config.SetValue("TargetGame", targ);

			if (!dpGameDataPath->GetPath().IsEmpty()) {
				wxFileName gameDataDir = dpGameDataPath->GetDirName();
				Config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToUTF8().data());
				Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), gameDataDir.GetFullPath().ToUTF8().data());
			}

			// set OutputDataPath even if it is empty
			wxFileName outputDataDir = dpOutputPath->GetDirName();
			Config.SetValue("OutputDataPath", outputDataDir.GetFullPath().ToUTF8().data());

			wxArrayInt items;
			wxString selectedfiles;
			for (int i = 0; i < dataFileList->GetCount(); i++)
				if (!dataFileList->IsChecked(i))
					selectedfiles += dataFileList->GetString(i) + "; ";

			selectedfiles = selectedfiles.BeforeLast(';');
			Config.SetValue("GameDataFiles/" + TargetGames[targ].ToStdString(), selectedfiles.ToUTF8().data());

			Config.SetBoolValue("WarnBatchBuildOverride", cbBBOverrideWarn->IsChecked());
			Config.SetBoolValue("BSATextureScan", cbBSATextures->IsChecked());
			Config.SetBoolValue("Input/LeftMousePan", cbLeftMousePan->IsChecked());
			
			int oldLang = Config.GetIntValue("Language");
			int newLang = SupportedLangs[choiceLanguage->GetSelection()];
			if (oldLang != newLang) {
				Config.SetValue("Language", newLang);
				app->InitLanguage();
			}

			wxColour colorBackground = cpColorBackground->GetColour();
			Config.SetValue("Rendering/ColorBackground.r", colorBackground.Red());
			Config.SetValue("Rendering/ColorBackground.g", colorBackground.Green());
			Config.SetValue("Rendering/ColorBackground.b", colorBackground.Blue());

			wxColour colorWire = cpColorWire->GetColour();
			Config.SetValue("Rendering/ColorWire.r", colorWire.Red());
			Config.SetValue("Rendering/ColorWire.g", colorWire.Green());
			Config.SetValue("Rendering/ColorWire.b", colorWire.Blue());

			wxFileName skeletonFile = fpSkeletonFile->GetFileName();
			Config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToUTF8().data());
			Config.SetValue("Anim/SkeletonRootName", choiceSkeletonRoot->GetStringSelection().ToUTF8().data());

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
			app->InitArchives();
		}

		delete settings;
	}
}

void BodySlideFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* about = wxXmlResource::Get()->LoadDialog(this, "dlgAbout");
	if (about) {
		about->SetSize(wxSize(625, 375));
		about->SetMinSize(wxSize(625, 375));
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
