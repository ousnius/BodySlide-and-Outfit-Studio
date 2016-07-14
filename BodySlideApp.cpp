/*
BodySlide and Outfit Studio
Copyright(C) 2016  Caliente & ousnius

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
#include <ppl.h>

ConfigurationManager Config;

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

	Config.LoadConfig();

	logger.Initialize();
	wxLogMessage("Initializing BodySlide...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

	wxInitAllImageHandlers();

	preview = nullptr;
	sliderView = nullptr;
	previewBaseNif = nullptr;
	outfitStudio = nullptr;

	Bind(wxEVT_CHAR_HOOK, &BodySlideApp::CharHook, this);

	wxLogMessage("Working directory: %s", wxGetCwd());
	SetDefaultConfig();

	InitLanguage();

	wxString gameName = "Target game: ";
	switch (targetGame) {
		case FO3: gameName.Append("Fallout 3"); break;
		case FONV: gameName.Append("Fallout New Vegas"); break;
		case SKYRIM: gameName.Append("Skyrim"); break;
		case FO4: gameName.Append("Fallout 4"); break;
		default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	string activeOutfit = Config.GetCString("SelectedOutfit");
	curOutfit = activeOutfit;

	LoadAllCategories();
	LoadAllGroups();

	int x = Config.GetIntValue("BodySlideFrame.x");
	int y = Config.GetIntValue("BodySlideFrame.y");
	int w = Config.GetIntValue("BodySlideFrame.width");	
	int h = Config.GetIntValue("BodySlideFrame.height");
	string maximized = Config["BodySlideFrame.maximized"];

	wxLogMessage("Loading BodySlide frame at X:%d Y:%d with W:%d H:%d...", x, y, w, h);
	sliderView = new BodySlideFrame(this, wxSize(w, h));
	sliderView->SetPosition(wxPoint(x, y));
	if (maximized == "true")
		sliderView->Maximize();

	sliderView->Show();
	SetTopWindow(sliderView);

	InitArchives();

	if (!Config["GameDataPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["GameDataPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["GameDataPath"]);
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
	cmdOutfitStudio = parser.Found("os");
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
	catch (const exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}
	
	if (sliderView)
		sliderView->delayLoad.Stop();

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
	catch (const exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}
	
	if (sliderView)
		sliderView->delayLoad.Stop();

	logger.SetFormatter(false);
	wxLogError("Unhandled exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unhandled exception has occurred: %s, the program will terminate."), error), _("Unhandled exception"), wxICON_ERROR);
}

void BodySlideApp::OnFatalException() {
	if (sliderView)
		sliderView->delayLoad.Stop();

	logger.SetFormatter(false);
	wxLogError("Fatal exception has occurred, the program will terminate.");
	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);
}


void BodySlideApp::InitArchives() {
	// Auto-detect archives
	FSManager::del();

	vector<string> fileList;
	GetArchiveFiles(fileList);

	FSManager::addArchives(fileList);
}

void BodySlideApp::GetArchiveFiles(vector<string>& outList) {
	string cp = "GameDataFiles";
	int targ = Config.GetIntValue("TargetGame");

	switch (targ) {
		case 0:
			cp += "/Fallout3";
			break;
		case 1:
			cp += "/FalloutNewVegas";
			break;
		case 2:
			cp += "/Skyrim";
			break;
		case 3:
			cp += "/Fallout4";
			break;
	}

	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	map<wxString, bool> fsearch;
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
		f = f.AfterLast('\\').MakeLower();
		if (fsearch.find(f) == fsearch.end()) {
			outList.push_back(dataDir.ToStdString() + f.ToStdString());
		}
	}
}

void BodySlideApp::LoadData() {
	wxLogMessage("Loading initial data...");
	sliderView->Freeze();
	setupOutfit(curOutfit);
	string activeOutfit = Config.GetCString("SelectedOutfit");
	string activePreset = Config.GetCString("SelectedPreset");
	ActivatePreset(activePreset);
	sliderView->Layout();
	sliderView->Refresh();
	sliderView->Thaw();

	if (!cmdGroupBuild.IsEmpty())
		GroupBuild(cmdGroupBuild.ToStdString());

	if (cmdOutfitStudio)
		LaunchOutfitStudio();
}

void BodySlideApp::setupOutfit(const string& outfitName) {
	if (!sliderView)
		return;

	wxLogMessage("Setting up set '%s'...", outfitName);
	sliderView->ClearOutfitList();
	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	if (preview)
		preview->Close();

	sliderManager.ClearSliders();
	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);

	string activeOutfit = Config.GetCString("SelectedOutfit");
	string activePreset = Config.GetCString("SelectedPreset");
	LoadPresets(activeOutfit);
	PopulatePresetList(activePreset);
	createSliders(outfitName);
	wxLogMessage("Finished setting up '%s'.", outfitName);
}

int BodySlideApp::createSetSliders(const string& outfit, bool hideAll) {
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
			curOutfit = outfit;
			activeSet.SetBaseDataPath(Config["ShapeDataPath"]);
			activeSet.LoadSetDiffData(dataSets);

			sliderManager.AddSlidersInSet(activeSet, hideAll);
			DisplayActiveSet();
		}
		else
			return 3;
	}
	else
		return 2;

	return 0;
}

int BodySlideApp::createSliders(const string& outfit, bool hideAll) {
	LoadSliderSets();

	int error = createSetSliders(outfit, hideAll);
	if (error)
		wxLogError("Failed to load set '%s' from slider set list (%d).", outfit, error);

	PopulateOutfitList(outfit);
	return 0;
}

void BodySlideApp::RefreshOutfitList() {
	LoadSliderSets();
	string activeOutfit = Config.GetCString("SelectedOutfit");
	PopulateOutfitList(activeOutfit);
}

int BodySlideApp::LoadSliderSets() {
	wxLogMessage("Loading all slider sets...");
	dataSets.Clear();
	outfitNameSource.clear();
	outfitNameOrder.clear();
	outFileCount.clear();

	wxArrayString files;
	wxDir::GetAllFiles("SliderSets", &files, "*.osp");
	wxDir::GetAllFiles("SliderSets", &files, "*.xml");

	for (auto &file : files) {
		SliderSetFile sliderDoc;
		sliderDoc.Open(file.ToStdString());
		if (sliderDoc.fail())
			continue;

		string outFilePath;
		vector<string> outfitNames;
		sliderDoc.GetSetNamesUnsorted(outfitNames, false);
		for (int i = 0; i < outfitNames.size(); i++) {
			outfitNameSource[outfitNames[i]] = file.ToStdString();
			outfitNameOrder.push_back(outfitNames[i]);

			sliderDoc.GetSetOutputFilePath(outfitNames[i], outFilePath);
			if (!outFilePath.empty()) {
				std::transform(outFilePath.begin(), outFilePath.end(), outFilePath.begin(), ::tolower);
				outFileCount[outFilePath].push_back(outfitNames[i]);
			}
		}
	}

	ungroupedOutfits.clear();
	for (auto &o : outfitNameSource) {
		vector<string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	return 0;
}

void BodySlideApp::ActivateOutfit(const string& outfitName) {
	wxLogMessage("Activating set '%s'...", outfitName);

	Config.SetValue("SelectedOutfit", outfitName);
	sliderView->Freeze();

	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();
	
	CleanupPreview();

	string activePreset = Config.GetCString("SelectedPreset");

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(activePreset);
	
	int error = createSetSliders(outfitName);
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

void BodySlideApp::ActivatePreset(const string &presetName, const bool& updatePreview) {
	wxLogMessage("Applying preset '%s' to sliders.", presetName);

	Config.SetValue("SelectedPreset", presetName);
	sliderManager.InitializeSliders(presetName);

	bool zapChanged = false;
	Slider* sliderSmall = nullptr;
	Slider* sliderBig = nullptr;
	for (int i = 0; i < sliderManager.slidersBig.size(); i++) {
		sliderSmall = &sliderManager.slidersSmall[i];
		sliderBig = &sliderManager.slidersBig[i];

		if (sliderBig->zap && !zapChanged) {
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

void BodySlideApp::PopulatePresetList(const string& select) {
	vector<string> presets;
	wxArrayString items;
	sliderManager.GetPresetNames(presets);
	items.reserve(presets.size());
	for (int i = 0; i < presets.size(); i++)
		items.Add(presets[i].c_str());

	sliderView->PopulatePresetList(items, select);
}

void BodySlideApp::PopulateOutfitList(const string& select) {
	string myselect = curOutfit;
	if (!select.empty())
		myselect = select;

	wxArrayString items;

	size_t n = outfitNameSource.size();
	if (n == 0)
		return;

	ApplyOutfitFilter();

	for (auto &fo : filteredOutfits)
		items.Add(fo);

	sliderView->PopulateOutfitList(items, wxString(myselect.c_str()));
}

void BodySlideApp::DisplayActiveSet() {
	if (activeSet.GenWeights())
		sliderView->ShowLowColumn(true);
	else
		sliderView->ShowLowColumn(false);

	// Category name, slider names, hidden flag
	vector<tuple<string, vector<string>, bool>> sliderCategories;
	vector<string> cats;
	cCollection.GetAllCategories(cats);

	// Populate category data
	for (auto &cat : cats) {
		vector<string> catSliders;
		cCollection.GetCategorySliders(cat, catSliders);

		if (catSliders.size() > 0) {
			bool catHidden = cCollection.GetCategoryHidden(cat);
			sliderCategories.push_back(make_tuple(cat, catSliders, !catHidden));
		}
		else
			continue;
	}

	// Loop slider set
	vector<vector<int>> catSliders;
	for (int i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].bHidden)
			continue;

		// Find the category in the list
		bool regularSlider = true;
		int iter = 0;
		for (auto &cat : sliderCategories) {
			catSliders.push_back(vector<int>());
			if (find(get<1>(cat).begin(), get<1>(cat).end(), activeSet[i].name) != get<1>(cat).end()) {
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
			string name = get<0>(cat);
			bool show = get<2>(cat);
			string displayName;

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

void BodySlideApp::LaunchOutfitStudio() {
	if (outfitStudio) {
		if (outfitStudio->IsIconized()) {
			if (outfitStudio->IsMaximized()) {
				outfitStudio->Restore();
				outfitStudio->Maximize();
			}
			else
				outfitStudio->Restore();
		}
		else
			outfitStudio->SetFocus();
	}
	else {
		int x = Config.GetIntValue("OutfitStudioFrame.x");
		int y = Config.GetIntValue("OutfitStudioFrame.y");
		int w = Config.GetIntValue("OutfitStudioFrame.width");
		int h = Config.GetIntValue("OutfitStudioFrame.height");
		string maximized = Config["OutfitStudioFrame.maximized"];

		outfitStudio = new OutfitStudio(wxPoint(x, y), wxSize(w, h), Config);
		if (maximized == "true")
			outfitStudio->Maximize();

		outfitStudio->Show();
	}
}

void BodySlideApp::ApplySliders(const string& targetShape, vector<Slider>& sliderSet, vector<Vector3>& verts, vector<ushort>& ZapIdx, vector<Vector2>* uvs) {
	for (auto &slider : sliderSet) {
		float val = slider.value;
		if (slider.zap) {
			if (val > 0)
				for (int j = 0; j < slider.linkedDataSets.size(); j++)
					dataSets.GetDiffIndices(slider.linkedDataSets[j], targetShape, ZapIdx);
		}
		else {
			if (slider.invert)
				val = 1.0f - val;

			for (int j = 0; j < slider.linkedDataSets.size(); j++) {
				if (slider.uv)
					dataSets.ApplyUVDiff(slider.linkedDataSets[j], targetShape, val, uvs);
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

int BodySlideApp::WriteMorphTRI(const string& triPath, SliderSet& sliderSet, NifFile& nif, unordered_map<string, vector<ushort>>& zapIndices) {
	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	string triFilePath = triPath + ".tri";

	for (auto shape = sliderSet.TargetShapesBegin(); shape != sliderSet.TargetShapesEnd(); ++shape) {
		for (int s = 0; s < sliderSet.size(); s++) {
			string dn = sliderSet[s].TargetDataName(shape->first);
			string target = shape->first;
			if (dn.empty())
				continue;

			if (!sliderSet[s].bUV && !sliderSet[s].bClamp && !sliderSet[s].bZap) {
				MorphDataPtr morph = make_shared<MorphData>();
				morph->name = sliderSet[s].name;

				const vector<ushort>& shapeZapIndices = zapIndices[shape->second];

				vector<Vector3> verts;
				int shapeVertCount = nif.GetVertCountForShape(shape->second);
				shapeVertCount += shapeZapIndices.size();
				if (shapeVertCount > 0)
					verts.resize(shapeVertCount);
				else
					continue;

				currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);

				if (shapeZapIndices.size() > 0 && shapeZapIndices.back() >= verts.size())
					continue;

				for (int i = shapeZapIndices.size() - 1; i >= 0; i--)
					verts.erase(verts.begin() + shapeZapIndices[i]);
				
				int i = 0;
				for (auto &v : verts) {
					if (!v.IsZero(true))
						morph->offsets.emplace(i, v);
					i++;
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(shape->second, morph);
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

	preview = new PreviewWindow(this);
}

void BodySlideApp::InitPreview() {
	if (!preview)
		return;

	wxLogMessage("Loading preview meshes...");
	string inputFileName = activeSet.GetInputFileName();
	string inputSetName = activeSet.GetName();
	bool freshLoad = false;

	if (!previewBaseNif) {
		previewBaseNif = new NifFile();
		previewBaseNif->Load(inputFileName);
		if (PreviewMod.Load(inputFileName))
			return;

		freshLoad = true;
		sliderManager.FlagReload(false);
	}
	else if (previewBaseName != inputFileName || previewSetName != inputSetName || sliderManager.NeedReload()) {
		delete previewBaseNif;
		previewBaseNif = new NifFile();
		previewBaseNif->Load(inputFileName);
		if (PreviewMod.Load(inputFileName) != 0)
			return;

		freshLoad = true;
		sliderManager.FlagReload(false);
	}

	previewBaseName = inputFileName;
	previewSetName = inputSetName;
	
	preview->ShowWeight(activeSet.GenWeights());

	vector<Vector3> verts;
	vector<Vector2> uvs;
	vector<ushort> zapIdx;
	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		zapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts))
			continue;

		previewBaseNif->GetUvsForShape(it->second, uvs);

		ApplySliders(it->first, sliderManager.slidersBig, verts, zapIdx, &uvs);

		// Zap deleted verts before preview
		if (freshLoad && zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(it->second, verts);
			PreviewMod.SetUvsForShape(it->second, uvs);
			PreviewMod.DeleteVertsForShape(it->second, zapIdx);
		}
		else if (zapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;
				verts.erase(verts.begin() + zapIdx[z]);
				uvs.erase(uvs.begin() + zapIdx[z]);
			}
			PreviewMod.SetVertsForShape(it->second, verts);
			PreviewMod.SetUvsForShape(it->second, uvs);
		}
		else {
			// No zapping needed - just show all the verts.
			PreviewMod.SetVertsForShape(it->second, verts);
			PreviewMod.SetUvsForShape(it->second, uvs);
		}
	}

	string baseGamePath = Config["GameDataPath"];
	preview->AddMeshFromNif(&PreviewMod);
	preview->SetBaseDataPath(baseGamePath);

	vector<string> shapeNames;
	PreviewMod.GetShapeList(shapeNames);
	for (auto &s : shapeNames)
		preview->AddNifShapeTexture(&PreviewMod, s);

	preview->Refresh();
}

void BodySlideApp::UpdatePreview() {
	if (!preview)
		return;

	if (!previewBaseNif)
		return;
	
	int weight = preview->GetWeight();
	vector<Vector3> verts, vertsLow, vertsHigh;
	vector<Vector2> uv;
	vector<ushort> zapIdx;
	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		zapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts))
			continue;

		previewBaseNif->GetUvsForShape(it->second, uv);
		vertsHigh = verts;
		vertsLow = verts;

		ApplySliders(it->first, sliderManager.slidersBig, vertsHigh, zapIdx, &uv);
		if (activeSet.GenWeights())
			ApplySliders(it->first, sliderManager.slidersSmall, vertsLow, zapIdx, &uv);

		// Calculate result of weight
		for (int i = 0; i < verts.size(); i++)
			verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));

		// Zap deleted verts before applying to the shape
		if (zapIdx.size() > 0) {
			for (int z = zapIdx.size() - 1; z >= 0; z--) {
				if (zapIdx[z] >= verts.size())
					continue;

				verts.erase(verts.begin() + zapIdx[z]);
				uv.erase(uv.begin() + zapIdx[z]);
			}
		}
		preview->Update(it->second, &verts, &uv);
	}

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
	
	vector<Vector3> verts, vertsLow, vertsHigh;
	vector<ushort> zapIdx;
	Vector3 v;
	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		zapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts))
			continue;
		
		vertsHigh = verts;
		vertsLow = verts;

		ApplySliders(it->first, sliderManager.slidersBig, vertsHigh, zapIdx);
		if (activeSet.GenWeights())
			ApplySliders(it->first, sliderManager.slidersSmall, vertsLow, zapIdx);
		
		// Calculate result of weight
		for (int i = 0; i < verts.size(); i++)
			verts[i] = (vertsHigh[i] / 100.0f * weight) + (vertsLow[i] / 100.0f * (100.0f - weight));

		// Zap deleted verts before preview
		if (zapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(it->second, verts);
			PreviewMod.DeleteVertsForShape(it->second, zapIdx);
		}
		else {
			// No zapping needed - just show all the verts.
			PreviewMod.SetVertsForShape(it->second, verts);
		}
	}

	preview->RefreshMeshFromNif(&PreviewMod);
}

void BodySlideApp::SetDefaultConfig() {
	Config.SetDefaultValue("TargetGame", 3);
	targetGame = Config.GetIntValue("TargetGame");

	Config.SetDefaultValue("ShapeDataPath", wxGetCwd().ToStdString() + "\\ShapeData");
	Config.SetDefaultValue("WarnMissingGamePath", "true");
	Config.SetDefaultValue("WarnBatchBuildOverride", "true");
	Config.SetDefaultValue("BSATextureScan", "true");
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultValue("UseSystemLanguage", "false");
	Config.SetDefaultValue("SelectedPreset", "");
	Config.SetDefaultValue("SelectedOutfit", "");

	Config.SetDefaultValue("GameRegKey/Fallout3", "Software\\Bethesda Softworks\\Fallout3");
	Config.SetDefaultValue("GameRegVal/Fallout3", "Installed Path");
	Config.SetDefaultValue("GameRegKey/FalloutNewVegas", "Software\\Bethesda Softworks\\FalloutNV");
	Config.SetDefaultValue("GameRegVal/FalloutNewVegas", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Skyrim", "Software\\Bethesda Softworks\\Skyrim");
	Config.SetDefaultValue("GameRegVal/Skyrim", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4", "Software\\Bethesda Softworks\\Fallout4");
	Config.SetDefaultValue("GameRegVal/Fallout4", "Installed Path");

	wxString gameKey;
	wxString gameValueKey;
	switch (targetGame) {
	case FO3:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_fo3nv.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Bip");
		gameKey = Config["GameRegKey/Fallout3"];
		gameValueKey = Config["GameRegVal/Fallout3"];
		break;
	case FONV:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_fo3nv.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Bip");
		gameKey = Config["GameRegKey/FalloutNewVegas"];
		gameValueKey = Config["GameRegVal/FalloutNewVegas"];
		break;
	case SKYRIM:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_xpmse.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "NPC");
		gameKey = Config["GameRegKey/Skyrim"];
		gameValueKey = Config["GameRegVal/Skyrim"];
		break;
	case FO4:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_fo4.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Root");
		gameKey = Config["GameRegKey/Fallout4"];
		gameValueKey = Config["GameRegVal/Fallout4"];
		break;
	default:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Root");
	}

	Config.SetDefaultValue("ReferenceTemplates", "");
	Config.SetDefaultValue("Input/SliderMinimum", 0);
	Config.SetDefaultValue("Input/SliderMaximum", 100);
	Config.SetDefaultValue("Input/LeftMousePan", "false");
	Config.SetDefaultValue("Editing/CenterMode", "Object");
	Config.SetDefaultValue("Lights/Ambient", 80);
	Config.SetDefaultValue("Lights/Brightness1", 55);
	Config.SetDefaultValue("Lights/Brightness2", 45);
	Config.SetDefaultValue("Lights/Brightness3", 45);
	Config.SetDefaultValue("BodySlideFrame.width", 800);
	Config.SetDefaultValue("BodySlideFrame.height", 600);
	Config.SetDefaultValue("BodySlideFrame.x", 100);
	Config.SetDefaultValue("BodySlideFrame.y", 100);
	Config.SetDefaultValue("OutfitStudioFrame.width", 990);
	Config.SetDefaultValue("OutfitStudioFrame.height", 757);
	Config.SetDefaultValue("OutfitStudioFrame.x", 100);
	Config.SetDefaultValue("OutfitStudioFrame.y", 100);
	Config.SetDefaultValue("OutfitStudioFrame.sashpos", 850);

	if (Config["GameDataPath"].empty()) {
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data\\");
				Config.SetDefaultValue("GameDataPath", installPath.ToStdString());
				wxLogMessage("Registry game data path: %s", installPath);
			}
			else if (Config["WarnMissingGamePath"] == "true") {
				wxLogWarning("Failed to find game install path registry value or GameDataPath in the config.");
				wxMessageBox(_("Failed to find game install path registry value or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
			}
		}
		else if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
			wxMessageBox(_("Failed to find game install path registry key or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
		}
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);
}

wxString BodySlideApp::GetGameDataPath(TargetGame gameID) {
	wxString dataPath;
	wxString gamestr;
	switch (gameID) {
		case FO3: 
			gamestr = "Fallout3";
			break;
		case FONV: 
			gamestr = "FalloutNewVegas";
			break;
		case SKYRIM:
			gamestr = "Skyrim";
			break;
		case FO4:
			gamestr = "Fallout4";
			break;
		default: break;
	}
	wxString gkey = "GameRegKey/" + gamestr;
	wxString gval = "GameRegVal/" + gamestr;
	wxString cust = "GameDataPaths/" + gamestr;

	if (!Config[cust].IsEmpty()) {
		dataPath = Config[cust];
	}
	else {
		wxRegKey key(wxRegKey::HKLM, Config[gkey], wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			if (key.HasValues() && key.QueryValue(Config[gval], dataPath)) {
				dataPath.Append("Data\\");
			}
		}
	}
	return dataPath;
}

void BodySlideApp::InitLanguage() {
	if (Config["UseSystemLanguage"] != "true")
		return;

	if (locale)
		delete locale;

	language = wxLocale::GetSystemLanguage();

	// Load language if possible, fall back to English otherwise
	if (wxLocale::IsAvailable(language)) {
		locale = new wxLocale(language);
		locale->AddCatalogLookupPathPrefix("lang");
		locale->AddCatalog("BodySlide");

		if (!locale->IsOk()) {
			wxLogError("System language '%d' is wrong.", language);
			wxMessageBox(wxString::Format(_("System language '%d' is wrong."), language));

			delete locale;
			locale = new wxLocale(wxLANGUAGE_ENGLISH);
			language = wxLANGUAGE_ENGLISH;
		}
	}
	else {
		wxLogError("The system language '%d' is not supported by your system. Try installing support for this language.", language);
		wxMessageBox(wxString::Format(_("The system language '%d' is not supported by your system. Try installing support for this language."), language));

		locale = new wxLocale(wxLANGUAGE_ENGLISH);
		language = wxLANGUAGE_ENGLISH;
	}

	wxLogMessage("Using language '%s'.", wxLocale::GetLanguageName(language));
}

void BodySlideApp::LoadAllCategories() {
	wxLogMessage("Loading all slider categories...");
	cCollection.LoadCategories("SliderCategories");
}

void BodySlideApp::SetPresetGroups(const string& setName) {
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
	gCollection.LoadGroups("SliderGroups");

	ungroupedOutfits.clear();
	for (auto &o : outfitNameSource) {
		vector<string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	vector<string> aliases;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "alias", aliases);
	vector<string> groups;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "group", groups);

	if (aliases.size() == groups.size())
		for (int i = 0; i < aliases.size(); i++)
			groupAlias[aliases[i]] = groups[i];
}

void BodySlideApp::GetAllGroupNames(vector<string>& outGroups) {
	set<string> gNames;
	gCollection.GetAllGroups(gNames);
	outGroups.assign(gNames.begin(), gNames.end());
}

int BodySlideApp::SaveGroupList(const string& fileName, const string& groupName) {
	if (filteredOutfits.empty())
		return 1;

	vector <string> existing;
	SliderSetGroupFile outfile(fileName);
	if (outfile.GetError() != 0) {
		if (outfile.GetError() == 1)
			outfile.New(fileName);

		if (outfile.GetError() != 0) {
			wxMessageBox(_("Failed to create group file."), "Error", wxICON_ERROR);
			return 5;
		}
	}

	outfile.GetGroupNames(existing);
	bool found = false;
	for (auto &e : existing) {
		if (_stricmp(e.c_str(), groupName.c_str()) == 0) {
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
	unordered_set<string> grpFiltOutfits;
	vector<string> workfiltList;
	static wxString lastGrps = "";
	static set<string> grouplist;


	wxString grpSrch = sliderView->search->GetValue();
	wxString outfitSrch = sliderView->outfitsearch->GetValue();

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
				string group = token;
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
		regex re;
		try {
			re.assign(outfitSrch, regex::icase);
			for (auto &w : workfiltList)
				if (regex_search(w, re))
					filteredOutfits.push_back(w);
		}
		catch (regex_error) {
			for (auto &w : workfiltList)
				filteredOutfits.push_back(w);
		}
	}

	Config.SetValue("LastGroupFilter", string(grpSrch));
	Config.SetValue("LastOutfitFilter", string(outfitSrch));
}

int BodySlideApp::GetOutfits(vector<string>& outList) {
	outList.assign(outfitNameOrder.begin(), outfitNameOrder.end());
	return outList.size();
}

int BodySlideApp::GetFilteredOutfits(vector<string>& outList) {
	outList.assign(filteredOutfits.begin(), filteredOutfits.end());
	return outList.size();
}

void BodySlideApp::LoadPresets(const string& sliderSet) {
	string outfit = sliderSet;
	if (sliderSet.empty())
		outfit = Config.GetCString("SelectedOutfit");

	wxLogMessage("Loading assigned presets...");

	vector<string> groups_and_aliases;
	for (auto &g : presetGroups) {
		groups_and_aliases.push_back(g);
		for (auto &ag : this->groupAlias)
			if (ag.second == g)
				groups_and_aliases.push_back(ag.first);
	}

	sliderManager.LoadPresets("SliderPresets", outfit, groups_and_aliases);
}

int BodySlideApp::BuildBodies(bool localPath, bool clean, bool tri) {
	string inputFileName = activeSet.GetInputFileName();
	NifFile nifSmall;
	NifFile nifBig;

	string outFileNameSmall;
	string outFileNameBig;

	wxLogMessage("Building set '%s' with options: Local Path = %s, Cleaning = %s, TRI = %s, GenWeights = %s",
		activeSet.GetName(), localPath ? "True" : "False", clean ? "True" : "False", tri ? "True" : "False", activeSet.GenWeights() ? "True" : "False");

	if (localPath) {
		outFileNameSmall = outFileNameBig = activeSet.GetOutputFile();
	}
	else {
		if (Config["GameDataPath"].empty()) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Would you like to show BodySlide where it is?"), _("Game not found"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogMessage("Aborted build without data path.");
					return 4;
				}
			}

			wxString cwd = wxGetCwd();
			wxString response = wxDirSelector(_("Please choose a directory to set as your Data path"), cwd);
			if (response.IsEmpty()) {
				wxLogMessage("Aborted build without data path.");
				return 4;
			}

			response.Append('\\');
			Config.SetValue("GameDataPath", response.ToStdString());
		}

		outFileNameSmall = Config["GameDataPath"] + activeSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;
		wxString path = Config["GameDataPath"] + activeSet.GetOutputPath();
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
		
		bool remHigh = wxRemoveFile(removeHigh);
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
		bool remLow = wxRemoveFile(removeLow);
		if (remLow)
			msg.Append(removeLow + "\n");
		else
			msg.Append(removeLow + _(" (no action)\n"));

		wxLogMessage("%s", msg);
		wxMessageBox(msg, _("Process Successful"));
		return 0;
	}

	int error = nifBig.Load(inputFileName);
	if (error) {
		wxLogError("Failed to load '%s' (%d)!", inputFileName, error);
		return 1;
	}

	if (activeSet.GenWeights())
		if (nifSmall.Load(inputFileName))
			return 1;

	vector<Vector3> vertsLow;
	vector<Vector3> vertsHigh;
	vector<ushort> zapIdx;
	unordered_map<string, vector<ushort>> zapIdxAll;

	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		if (!nifBig.GetVertsForShape(it->second, vertsHigh))
			continue;

		if (activeSet.GenWeights())
			if (!nifSmall.GetVertsForShape(it->second, vertsLow))
				continue;

		zapIdxAll.emplace(it->second, vector<ushort>());

		ApplySliders(it->first, sliderManager.slidersBig, vertsHigh, zapIdx);
		nifBig.SetVertsForShape(it->second, vertsHigh);
		if (targetGame == FO4) {
			nifBig.SmoothNormalsForShape(it->second);
			nifBig.CalcTangentsForShape(it->second);
		}
		nifBig.DeleteVertsForShape(it->second, zapIdx);

		if (activeSet.GenWeights()) {
			zapIdx.clear();
			ApplySliders(it->first, sliderManager.slidersSmall, vertsLow, zapIdx);
			nifSmall.SetVertsForShape(it->second, vertsLow);
			nifSmall.DeleteVertsForShape(it->second, zapIdx);
		}

		zapIdxAll[it->second] = zapIdx;
		zapIdx.clear();
	}

	/* Add TRI path for in-game morphs */
	if (tri) {
		string triPath = activeSet.GetOutputFilePath() + ".tri";
		string triPathTrimmed = triPath;
		triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
		triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

		if (!WriteMorphTRI(outFileNameBig, activeSet, nifBig, zapIdxAll)) {
			wxLogError("Failed to write TRI file to '%s'!", triPath);
			wxMessageBox(wxString().Format(_("Failed to write TRI file to the following location\n\n%s"), triPath), _("Unable to process"), wxOK | wxICON_ERROR);
		}

		if (targetGame < FO4) {
			string triShapeLink;
			for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
				triShapeLink = it->second;
				if (tri && nifBig.GetVertCountForShape(triShapeLink) > 0) {
					nifBig.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
					if (activeSet.GenWeights())
						nifSmall.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
					tri = false;
				}
			}
		}
		else {
			nifBig.AddStringExtraData(nifBig.NodeName(nifBig.GetRootNodeID()), "BODYTRI", triPathTrimmed, true);
			if (activeSet.GenWeights())
				nifSmall.AddStringExtraData(nifBig.NodeName(nifBig.GetRootNodeID()), "BODYTRI", triPathTrimmed, true);
		}
	}

	wxString savedLow;
	wxString savedHigh;
	wxString custName;
	bool useCustName = false;

	if (activeSet.GenWeights()) {
		outFileNameSmall += "_0.nif";
		outFileNameBig += "_1.nif";
		custName = outFileNameSmall;
		savedLow = custName;
		while (nifSmall.Save(custName.ToStdString())) {
			wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
			wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

			custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
			if (custName.IsEmpty()) {
				wxLogMessage("Aborted build when choosing alternate file name.");
				return 4;
			}

			useCustName = true;
			savedLow = custName;
		}

		wxString custEnd;
		if (custName.EndsWith("_0.nif", &custEnd))
			custName = custEnd + "_1.nif";
		else
			custName.Empty();
	}
	else {
		outFileNameBig += ".nif";
		custName = outFileNameBig;
	}

	if (!useCustName)
		outFileNameBig = custName;

	savedHigh = custName;
	while (nifBig.Save(custName.ToStdString())) {
		wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
		wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

		custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
		if (custName.IsEmpty()) {
			wxLogMessage("Aborted build when choosing alternate file name.");
			return 4;
		}

		useCustName = true;
		savedHigh = custName;
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

int BodySlideApp::BuildListBodies(vector<string>& outfitList, map<string, string>& failedOutfits, bool clean, bool tri, const string& custPath) {
	string datapath = custPath;
	wxProgressDialog* progWnd;

	wxLogMessage("Started batch build with options: Custom Path = %s, Cleaning = %s, TRI = %s",
		custPath.empty() ? "False" : custPath, clean ? "True" : "False", tri ? "True" : "False");

	if (clean) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?"), _("Clean Batch Build"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning batch build.");
			return 1;
		}
	}

	string activePreset = Config["SelectedPreset"];

	if (datapath.empty()) {
		if (Config["GameDataPath"].empty()) {
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

			wxString cwd = wxGetCwd();
			Config.SetValue("GameDataPath", cwd.ToStdString() + "\\");
		}
		datapath = Config["GameDataPath"];
	}

	if (Config.MatchValue("WarnBatchBuildOverride", "true")) {
		vector<wxArrayString> choicesList;
		for (auto &filePath : outFileCount) {
			if (filePath.second.size() > 1) {
				wxArrayString selFilePaths;
				for (auto &file : filePath.second) {
					// Only if it's going to be batch built
					if (find(outfitList.begin(), outfitList.end(), file) != outfitList.end())
						selFilePaths.Add(file);
				}

				// Same file would not be written more than once
				if (selFilePaths.size() <= 1)
					continue;

				choicesList.push_back(selFilePaths);
			}
		}

		if (!choicesList.empty()) {
			wxXmlResource* rsrc = wxXmlResource::Get();
			wxDialog* dlgBuildOverride = rsrc->LoadDialog(sliderView, "dlgBuildOverride");
			dlgBuildOverride->SetSize(wxSize(650, 400));
			dlgBuildOverride->SetSizeHints(wxSize(650, 400), wxSize(650, -1));
			dlgBuildOverride->CenterOnParent();

			wxScrolledWindow* scrollOverrides = XRCCTRL(*dlgBuildOverride, "scrollOverrides", wxScrolledWindow);
			wxBoxSizer* choicesSizer = (wxBoxSizer*)scrollOverrides->GetSizer();

			int nChoice = 1;
			vector<wxRadioBox*> choiceBoxes;
			for (auto &choices : choicesList) {
				wxRadioBox* choiceBox = new wxRadioBox(scrollOverrides, wxID_ANY, _("Choose output set") + wxString::Format(" #%d", nChoice), wxDefaultPosition, wxDefaultSize, choices, 1, wxRA_SPECIFY_COLS);
				choicesSizer->Add(choiceBox, 0, wxALL | wxEXPAND, 5);

				choiceBoxes.push_back(choiceBox);
				nChoice++;
			}

			scrollOverrides->FitInside();

			if (dlgBuildOverride->ShowModal() == wxID_CANCEL) {
				wxLogMessage("Aborted batch build by not choosing a file override.");
				delete dlgBuildOverride;
				return 1;
			}

			for (int i = 0; i < choicesList.size(); i++) {
				// Remove others from the list of outfits to build
				choicesList[i].Remove(choiceBoxes[i]->GetStringSelection());

				for (auto &file : choicesList[i]) {
					auto result = find(outfitList.begin(), outfitList.end(), file);
					if (result != outfitList.end())
						outfitList.erase(result);
				}
			}

			delete dlgBuildOverride;
		}
	}

	progWnd = new wxProgressDialog(_("Processing Outfits"), _("Starting..."), 1000, nullptr, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_ELAPSED_TIME);
	progWnd->SetSize(400, 150);
	float progstep = 1000.0f / outfitList.size();
	int count = 1;

	wxMutex mtx;
	concurrency::parallel_for_each(outfitList.begin(), outfitList.end(), [&](const string& outfit)
	{
		mtx.Lock();
		wxString progMsg = wxString::Format(_("Processing '%s' (%d of %d)..."), outfit, count, outfitList.size());
		wxLogMessage(progMsg);
		progWnd->Update((int)(count * progstep) - 1, progMsg);
		count++;
		mtx.Unlock();

		/* Load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			mtx.Lock();
			failedOutfits[outfit] = _("No recorded outfit name source");
			mtx.Unlock();
			return;
		}

		SliderSet currentSet;
		DiffDataSets currentDiffs;

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			if (!sliderDoc.GetSet(outfit, currentSet)) {
				currentSet.SetBaseDataPath(Config["ShapeDataPath"]);
				currentSet.LoadSetDiffData(currentDiffs);
			}
			else {
				mtx.Lock();
				failedOutfits[outfit] = _("Unable to get slider set from file: ") + outfitNameSource[outfit];
				mtx.Unlock();
			}
		}
		else {
			mtx.Lock();
			failedOutfits[outfit] = _("Unable to open slider set file: ") + outfitNameSource[outfit];
			mtx.Unlock();
			return;
		}

		// ALT key
		if (clean && custPath.empty()) {
			bool genWeights = currentSet.GenWeights();

			string removePath = datapath + currentSet.GetOutputFilePath();
			string removeHigh = removePath + ".nif";
			if (genWeights)
				removeHigh = removePath + "_1.nif";

			if (wxFileName::FileExists(removeHigh))
				wxRemoveFile(removeHigh);

			if (!genWeights)
				return;

			string removeLow = removePath + "_0.nif";
			if (wxFileName::FileExists(removeLow))
				wxRemoveFile(removeLow);

			return;
		}

		/* Load input NIFs */
		NifFile nifBig;
		NifFile nifSmall;
		if (nifBig.Load(currentSet.GetInputFileName())) {
			mtx.Lock();
			failedOutfits[outfit] = _("Unable to load input nif: ") + currentSet.GetInputFileName();
			mtx.Unlock();
			return;
		}

		if (currentSet.GenWeights())
			if (nifSmall.Load(currentSet.GetInputFileName()))
				return;

		/* Shape the NIF files */
		vector<Vector3> vertsLow;
		vector<Vector3> vertsHigh;
		vector<ushort> zapIdx;
		unordered_map<string, vector<ushort>> zapIdxAll;

		for (auto it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
			if (!nifBig.GetVertsForShape(it->second, vertsHigh))
				continue;

			if (currentSet.GenWeights())
				if (!nifSmall.GetVertsForShape(it->second, vertsLow))
					continue;

			float vbig = 0.0f;
			float vsmall = 0.0f;
			vector<int> clamps;
			zapIdxAll.emplace(it->second, vector<ushort>());

			for (int s = 0; s < currentSet.size(); s++) {
				string dn = currentSet[s].TargetDataName(it->first);
				string target = it->first;
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

				if (currentSet[s].bZap) {
					if (vbig > 0.0f) {
						currentDiffs.GetDiffIndices(dn, target, zapIdx);
						zapIdxAll[it->second] = zapIdx;
					}
					continue;
				}

				currentDiffs.ApplyDiff(dn, target, vbig, &vertsHigh);
				if (currentSet.GenWeights())
					currentDiffs.ApplyDiff(dn, target, vsmall, &vertsLow);
			}

			if (!clamps.empty()) {
				for (auto &c : clamps) {
					string dn = currentSet[c].TargetDataName(it->first);
					string target = it->first;
					if (currentSet[c].defBigValue > 0)
						currentDiffs.ApplyClamp(dn, target, &vertsHigh);

					if (currentSet.GenWeights())
						if (currentSet[c].defSmallValue > 0)
							currentDiffs.ApplyClamp(dn, target, &vertsLow);
				}
			}

			nifBig.SetVertsForShape(it->second, vertsHigh);
			if (targetGame == FO4) {
				nifBig.SmoothNormalsForShape(it->second);
				nifBig.CalcTangentsForShape(it->second);
			}
			nifBig.DeleteVertsForShape(it->second, zapIdx);

			if (currentSet.GenWeights()) {
				nifSmall.SetVertsForShape(it->second, vertsLow);
				nifSmall.DeleteVertsForShape(it->second, zapIdx);
			}

			zapIdx.clear();
		}

		/* Create directory for the outfit */
		wxString dir = datapath + currentSet.GetOutputPath();
		bool success = wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

		if (!success) {
			mtx.Lock();
			failedOutfits[outfit] = _("Unable to create destination directory: ") + dir.ToStdString();
			mtx.Unlock();
			return;
		}

		string outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		string outFileNameBig = outFileNameSmall;

		/* Add TRI path for in-game morphs */
		bool triEnd = tri;
		if (triEnd) {
			string triPath = currentSet.GetOutputFilePath() + ".tri";
			string triPathTrimmed = triPath;
			triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
			triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

			if (!WriteMorphTRI(outFileNameBig, currentSet, nifBig, zapIdxAll)) {
				mtx.Lock();
				wxLogError("Failed to create TRI file to '%s'!", triPath);
				mtx.Unlock();
			}

			if (targetGame < FO4) {
				for (auto it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
					string triShapeLink = it->second;
					if (triEnd && nifBig.GetVertCountForShape(triShapeLink) > 0) {
						nifBig.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
						if (currentSet.GenWeights())
							nifSmall.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
						triEnd = false;
					}
				}
			}
			else {
				nifBig.AddStringExtraData(nifBig.NodeName(nifBig.GetRootNodeID()), "BODYTRI", triPathTrimmed, true);
				if (activeSet.GenWeights())
					nifSmall.AddStringExtraData(nifBig.NodeName(nifBig.GetRootNodeID()), "BODYTRI", triPathTrimmed, true);
			}
		}

		/* Set filenames for the outfit */
		if (currentSet.GenWeights()) {
			outFileNameSmall += "_0.nif";
			outFileNameBig += "_1.nif";
			if (nifBig.Save(outFileNameBig)) {
				mtx.Lock();
				failedOutfits[outfit] = _("Unable to save nif file: ") + outFileNameBig;
				mtx.Unlock();
				return;
			}
			if (nifSmall.Save(outFileNameSmall)) {
				mtx.Lock();
				failedOutfits[outfit] = _("Unable to save nif file: ") + outFileNameSmall;
				mtx.Unlock();
				return;
			}
		}
		else {
			outFileNameBig += ".nif";
			if (nifBig.Save(outFileNameBig)) {
				mtx.Lock();
				failedOutfits[outfit] = _("Unable to save nif file: ") + outFileNameBig;
				mtx.Unlock();
				return;
			}
		}
	});

	progWnd->Update(1000);
	delete progWnd;

	if (failedOutfits.size() > 0)
		return 3;

	return 0;
}

void BodySlideApp::GroupBuild(const string& group) {
	vector<string> outfits;
	for (auto &o : outfitNameSource) {
		vector<string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (find(groups.begin(), groups.end(), group) != groups.end())
			outfits.push_back(o.first);
	}

	string preset;
	if (!cmdPreset.IsEmpty()) {
		preset = Config["SelectedPreset"];
		Config.SetValue("SelectedPreset", cmdPreset.ToStdString());
	}

	map<string, string> failedOutfits;
	int ret = BuildListBodies(outfits, failedOutfits, false, cmdTri, cmdTargetDir.ToStdString());

	if (!cmdPreset.IsEmpty())
		Config.SetValue("SelectedPreset", preset);

	if (ret == 0) {
		wxLogMessage("All group build sets processed successfully!");
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto &e : failedOutfits) {
			wxLogError("Failed to build '%s': %s", e.first, e.second);
			errlist.Add(e.first + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(sliderView, _("The following sets failed"), _("Failed"), errlist, nullptr, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
		errdisplay.ShowModal();
	}

	sliderView->Close(true);
}

float BodySlideApp::GetSliderValue(const wxString& sliderName, bool isLo) {
	string sstr = sliderName.ToStdString();
	return sliderManager.GetSlider(sstr, isLo);
}

void BodySlideApp::SetSliderValue(const wxString& sliderName, bool isLo, float val) {
	string sstr = sliderName.ToStdString();
	sliderManager.SetSlider(sstr, isLo, val);
}

void BodySlideApp::SetSliderChanged(const wxString& sliderName, bool isLo) {
	string sstr = sliderName.ToStdString();
	sliderManager.SetChanged(sstr, isLo);
}

int BodySlideApp::UpdateSliderPositions(const string& presetName) {
	string outfitName = Config["SelectedOutfit"];
	vector<string> groups;

	string outputFile = sliderManager.GetPresetFileNames(presetName);
	if (outputFile.empty())
		return -2;

	sliderManager.GetPresetGroups(presetName, groups);

	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

int BodySlideApp::SaveSliderPositions(const string& outputFile, const string& presetName, vector<string>& groups) {
	string outfitName = Config["SelectedOutfit"];
	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

BodySlideFrame::BodySlideFrame(BodySlideApp* app, const wxSize &size) : delayLoad(this, DELAYLOAD_TIMER) {
	this->app = app;
	rowCount = 0;

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->InitAllHandlers();
	bool loaded = xrc->Load("res\\xrc\\BodySlide.xrc");
	if (!loaded) {
		wxMessageBox(_("Failed to load BodySlide.xrc file!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	xrc->LoadFrame(this, GetParent(), "bodySlideFrame");
	if (!loaded) {
		wxMessageBox(_("Failed to load BodySlide frame!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	xrc->Load("res\\xrc\\BatchBuild.xrc");
	xrc->Load("res\\xrc\\Settings.xrc");
	xrc->Load("res\\xrc\\About.xrc");

	delayLoad.Start(100, true);

	SetDoubleBuffered(true);
	SetIcon(wxIcon("res\\images\\OutfitStudio.png", wxBITMAP_TYPE_PNG));
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

	wxString val = Config.GetCString("LastGroupFilter");
	search->ChangeValue(val);
	val = Config.GetCString("LastOutfitFilter");
	outfitsearch->ChangeValue(val);

	wxButton* btnAbout = (wxButton*)FindWindowByName("btnAbout");
	if (btnAbout)
		btnAbout->SetBitmap(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(15, 15)));

	if (app->targetGame != SKYRIM && app->targetGame != FO4)
		XRCCTRL(*this, "cbMorphs", wxCheckBox)->Show(false);
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

void BodySlideFrame::AddSliderGUI(const wxString& name, const wxString& displayName, bool isZap, bool oneSize) {
	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (!scrollWindow)
		return;

	wxSizer* sliderLayout = scrollWindow->GetSizer();
	if (!sliderLayout)
		return;

	BodySlideFrame::SliderDisplay* sd = new BodySlideFrame::SliderDisplay;
	sd->isZap = isZap;
	sd->oneSize = oneSize;

	int minValue = Config.GetIntValue("Input/SliderMinimum");
	int maxValue = Config.GetIntValue("Input/SliderMaximum");

	if (!oneSize) {
		sd->lblSliderLo = new wxStaticText(scrollWindow, wxID_ANY, displayName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
		sd->lblSliderLo->SetForegroundColour(wxColour(200, 200, 200));
		sliderLayout->Add(sd->lblSliderLo, 0, wxLEFT | wxALIGN_CENTER, 5);

		if (isZap) {
			sd->zapCheckLo = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZLO");
			sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(sd->zapCheckLo, 0, wxALIGN_LEFT, 0);
		}
		else {
			sd->sliderLo = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 24), wxSL_AUTOTICKS | wxSL_BOTTOM | wxSL_HORIZONTAL);
			sd->sliderLo->SetTickFreq(5);
			sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
			sd->sliderLo->SetName(name + "|LO");
			sliderLayout->Add(sd->sliderLo, 1, wxEXPAND, 0);

			sd->sliderReadoutLo = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(50, -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
			sd->sliderReadoutLo->SetName(name + "|RLO");
			sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);
			sliderLayout->Add(sd->sliderReadoutLo, 0, wxALL | wxALIGN_CENTER, 0);
		}
	}

	sd->lblSliderHi = new wxStaticText(scrollWindow, wxID_ANY, displayName, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	sd->lblSliderHi->SetForegroundColour(wxColour(200, 200, 200));
	sliderLayout->Add(sd->lblSliderHi, 0, wxLEFT | wxALIGN_CENTER, 5);

	if (isZap) {
		sd->zapCheckHi = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZHI");
		sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(sd->zapCheckHi, 0, wxALIGN_LEFT, 0);
	}
	else {
		sd->sliderHi = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 24), wxSL_AUTOTICKS | wxSL_HORIZONTAL);
		sd->sliderHi->SetTickFreq(5);
		sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
		sd->sliderHi->SetName(name + "|HI");
		sliderLayout->Add(sd->sliderHi, 1, wxEXPAND, 0);

		sd->sliderReadoutHi = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(50, -1), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
		sd->sliderReadoutHi->SetName(name + "|RHI");
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

		int i;
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
	wxString fullname = name;
	int intval = (int)(newValue * 100.0f);

	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToStdString());
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
	app->CloseOutfitStudio(true);

	int ret = Config.SaveConfig("Config.xml");
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
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToStdString());
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
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(name.ToStdString());
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
		app->cCollection.SetCategoryHidden(cb->GetName().ToStdString(), false);
	else
		app->cCollection.SetCategoryHidden(cb->GetName().ToStdString(), true);

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

	string sliderName = sn.ToStdString();
	wxLogMessage("Zap '%s' %s.", sliderName, event.IsChecked() ? "checked" : "unchecked");

	if (event.IsChecked()) {
		if (sliderDisplays[sliderName]->oneSize) {
			app->SetSliderValue(sliderName, false, 1.0f);
		}
		else {
			app->SetSliderValue(sliderName, true, 1.0f);
			app->SetSliderValue(sliderName, false, 1.0f);
			if (isLo)
				sliderDisplays[sliderName]->zapCheckHi->SetValue(true);
			else
				sliderDisplays[sliderName]->zapCheckLo->SetValue(true);
		}
	}
	else {
		if (sliderDisplays[sliderName]->oneSize) {
			app->SetSliderValue(sliderName, false, 0.0f);
		}
		else {
			app->SetSliderValue(sliderName, true, 0.0f);
			app->SetSliderValue(sliderName, false, 0.0f);
			if (isLo)
				sliderDisplays[sliderName]->zapCheckHi->SetValue(false);
			else
				sliderDisplays[sliderName]->zapCheckLo->SetValue(false);
		}
	}
	app->SetSliderChanged(sliderName, isLo);

	wxBeginBusyCursor();
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
	vector<string> groupNames;
	unordered_set<string> curGroupNames;
	app->GetAllGroupNames(groupNames);

	wxString srch = search->GetValue();
	wxStringTokenizer tokenizer(srch, ",;");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim();
		token.Trim(false);
		curGroupNames.insert(token.ToStdString());
	}

	wxArrayString grpChoices;
	wxArrayInt grpSelections;
	int idx = 0;
	for (auto &g : groupNames) {
		grpChoices.Add(g);
		if (curGroupNames.find(g) != curGroupNames.end()) {
			grpSelections.Add(idx);
		}
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

	wxFileDialog saveGroupDialog(this, _("Choose or create group file"), "SliderGroups", wxEmptyString, "Group Files (*.xml)|*.xml", wxFD_SAVE);
	if (saveGroupDialog.ShowModal() == wxID_CANCEL)
		return;

	string fName = saveGroupDialog.GetPath();
	string gName;
	int ret;

	do {
		gName = wxGetTextFromUser(_("What would you like the new group to be called?"), _("New Group Name"));
		if (gName.empty())
			return;

		ret = app->SaveGroupList(fName, gName);
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
}

void BodySlideFrame::OnChooseOutfit(wxCommandEvent& event) {
	string sstr = event.GetString().ToStdString();
	app->ActivateOutfit(sstr);
}

void BodySlideFrame::OnChoosePreset(wxCommandEvent& event) {
	string sstr = event.GetString().ToStdString();
	app->ActivatePreset(sstr);
}

void BodySlideFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	string presetName = Config["SelectedPreset"];
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

	vector<string> groups;
	PresetSaveDialog psd(this);

	app->GetAllGroupNames(psd.allGroupNames);
	psd.FilterGroups();
	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	string fname = psd.outFileName;
	string presetName = psd.outPresetName;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	int error = app->SaveSliderPositions(fname, presetName, groups);
	if (error) {
		wxLogError("Failed to save preset as '%s' (%d)!", fname, error);
		wxMessageBox(wxString::Format(_("Failed to save preset as '%s' (%d)!"), fname, error), "Error");
	}

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	Config.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnGroupManager(wxCommandEvent& WXUNUSED(event)) {
	vector<string> outfits;
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

	wxCheckBox* cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	bool tri = false;
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	if (wxGetKeyState(WXK_CONTROL))
		app->BuildBodies(true, false, tri);
	else if (wxGetKeyState(WXK_ALT))
		app->BuildBodies(false, true, tri);
	else
		app->BuildBodies(false, false, tri);
}

void BodySlideFrame::OnBatchBuild(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	wxArrayString oChoices;
	wxArrayInt oSelections;
	vector<string> outfitChoices;
	vector<string> toBuild;

	wxCheckBox* cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	bool custpath = false;
	bool clean = false;
	bool tri = false;
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	if (wxGetKeyState(WXK_CONTROL))
		custpath = true;
	else if (wxGetKeyState(WXK_ALT))
		clean = true;

	app->GetFilteredOutfits(outfitChoices);

	int idx = 0;
	for (auto &o : outfitChoices) {
		oChoices.Add(o);
		oSelections.Add(idx);
		idx++;
	}

	wxString filter;
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

	map<string, string> failedOutfits;
	int ret;
	if (custpath) {
		string path = wxDirSelector(_("Choose a folder to contain the saved files"));
		if (path.empty())
			return;

		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, path + "\\");
	}
	else if (clean)
		ret = app->BuildListBodies(toBuild, failedOutfits, true, tri);
	else
		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri);

	if (ret == 0) {
		wxLogMessage("All sets processed successfully!");
		wxMessageBox(_("All sets processed successfully!"), _("Complete"), wxICON_INFORMATION);
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto &e : failedOutfits) {
			wxLogError("Failed to build '%s': %s", e.first, e.second);
			errlist.Add(e.first + ":" + e.second);
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
	else {
		for (int i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i);
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
			fpSkeletonFile->SetPath("res\\skeleton_fo3nv.nif");
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case SKYRIM:
			fpSkeletonFile->SetPath("res\\skeleton_female_xpmse.nif");
			choiceSkeletonRoot->SetStringSelection("NPC");
			break;
		case FO4:
		default:
			fpSkeletonFile->SetPath("res\\skeleton_fo4.nif");
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

	wxString cp = "GameDataFiles";
	switch (targetGame) {
		case FO3:
			cp += "/Fallout3";
			break;
		case FONV:
			cp += "/FalloutNewVegas";
			break;
		case SKYRIM:
			cp += "/Skyrim";
			break;
		case FO4:
			cp += "/Fallout4";
			break;
	}

	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false);
		val = val.Trim();
		std::transform(val.begin(), val.end(), val.begin(), ::tolower);
		fsearch[val] = true;
	}

	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& f : files) {
		f = f.AfterLast('\\');
		dataFileList->Insert(f, dataFileList->GetCount());
		std::transform(f.begin(), f.end(), f.begin(), ::tolower);
		if (fsearch.find(f) == fsearch.end()) {
			dataFileList->Check(dataFileList->GetCount() - 1);
		}
	}
}

void BodySlideFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	if (app->IsOutfitStudioOpened()) {
		wxMessageBox(_("You can't change the settings while Outfit Studio is opened up."), _("Warning"), wxICON_WARNING, this);
		return;
	}

	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(wxSize(525, 533));
		settings->CenterOnParent();

		wxChoice* choiceTargetGame = XRCCTRL(*settings, "choiceTargetGame", wxChoice);
		choiceTargetGame->Select(Config.GetIntValue("TargetGame"));

		wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*settings, "dpGameDataPath", wxDirPickerCtrl);
		wxString gameDataPath = Config["GameDataPath"];
		dpGameDataPath->SetPath(gameDataPath);

		wxCheckBox* cbBBOverrideWarn = XRCCTRL(*settings, "cbBBOverrideWarn", wxCheckBox);
		cbBBOverrideWarn->SetValue(Config["WarnBatchBuildOverride"] != "false");

		wxCheckBox* cbBSATextures = XRCCTRL(*settings, "cbBSATextures", wxCheckBox);
		cbBSATextures->SetValue(Config["BSATextureScan"] != "false");

		wxCheckBox* cbLeftMousePan = XRCCTRL(*settings, "cbLeftMousePan", wxCheckBox);
		cbLeftMousePan->SetValue(Config["Input/LeftMousePan"] != "false");

		wxCheckBox* cbLanguage = XRCCTRL(*settings, "cbLanguage", wxCheckBox);
		cbLanguage->SetValue(Config["UseSystemLanguage"] != "false");

		wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*settings, "fpSkeletonFile", wxFilePickerCtrl);
		fpSkeletonFile->SetPath(Config["Anim/DefaultSkeletonReference"]);

		wxChoice* choiceSkeletonRoot = XRCCTRL(*settings, "choiceSkeletonRoot", wxChoice);
		choiceSkeletonRoot->SetStringSelection(Config["Anim/SkeletonRootName"]);

		wxCheckListBox* dataFileList = XRCCTRL(*settings, "DataFileList", wxCheckListBox);
		SettingsFillDataFiles(dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		choiceTargetGame->Bind(wxEVT_CHOICE, &BodySlideFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
			Config.SetValue("TargetGame", targ);

			wxString TargetGames[4] = { "Fallout3", "FalloutNewVegas", "Skyrim", "Fallout4" };
			if (!dpGameDataPath->GetPath().IsEmpty()) {
				wxFileName gameDataDir = dpGameDataPath->GetDirName();
				Config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToStdString());
				Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), gameDataDir.GetFullPath().ToStdString());
			}

			wxArrayInt items;
			wxString selectedfiles;
			for (int i = 0; i < dataFileList->GetCount(); i++)
				if (!dataFileList->IsChecked(i))
					selectedfiles += dataFileList->GetString(i) + "; ";

			selectedfiles = selectedfiles.BeforeLast(';');
			Config.SetValue("GameDataFiles/" + TargetGames[targ].ToStdString(), selectedfiles.ToStdString());

			Config.SetValue("WarnBatchBuildOverride", cbBBOverrideWarn->IsChecked() ? "true" : "false");
			Config.SetValue("BSATextureScan", cbBSATextures->IsChecked() ? "true" : "false");
			Config.SetValue("Input/LeftMousePan", cbLeftMousePan->IsChecked() ? "true" : "false");
			Config.SetValue("UseSystemLanguage", cbLanguage->IsChecked() ? "true" : "false");

			wxFileName skeletonFile = fpSkeletonFile->GetFileName();
			Config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToStdString());
			Config.SetValue("Anim/SkeletonRootName", choiceSkeletonRoot->GetStringSelection().ToStdString());

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
	Config.SetValue("BodySlideFrame.x", p.x);
	Config.SetValue("BodySlideFrame.y", p.y);
	event.Skip();
}

void BodySlideFrame::OnSetSize(wxSizeEvent& event) {
	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		Config.SetValue("BodySlideFrame.width", p.x);
		Config.SetValue("BodySlideFrame.height", p.y);
	}

	Config.SetValue("BodySlideFrame.maximized", maximized ? "true" : "false");
	event.Skip();
}
