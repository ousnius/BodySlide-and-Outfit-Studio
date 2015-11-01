/*
BodySlide and Outfit Studio
Copyright(C) 2015  Caliente & ousnius

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
#include "XmlFinder.h"
#include "FSManager.h"

ConfigurationManager Config;

BEGIN_EVENT_TABLE(BodySlideFrame, wxFrame)
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
	EVT_BUTTON(XRCID("btnPresets"), BodySlideFrame::OnSavePreset)
	EVT_BUTTON(XRCID("btnGroupManager"), BodySlideFrame::OnGroupManager)
	EVT_BUTTON(XRCID("btnChooseGroups"), BodySlideFrame::OnChooseGroups)
	EVT_BUTTON(XRCID("btnRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
		
	EVT_MENU(XRCID("menuChooseGroups"), BodySlideFrame::OnChooseGroups)	
	EVT_MENU(XRCID("menuRefreshGroups"), BodySlideFrame::OnRefreshGroups)
	EVT_MENU(XRCID("menuRefreshOutfits"), BodySlideFrame::OnRefreshOutfits)
	EVT_MENU(XRCID("menuSaveGroups"), BodySlideFrame::OnSaveGroups)
	
	EVT_MOVE_END(BodySlideFrame::OnMoveWindow)
	EVT_SIZE(BodySlideFrame::OnSetSize)

END_EVENT_TABLE();

IMPLEMENT_APP(BodySlideApp)

BodySlideApp::~BodySlideApp() {
	delete previewBaseNif;
	previewBaseNif = nullptr;

	FSManager::del();
}

bool BodySlideApp::OnInit() {
	if (!wxApp::OnInit())
		return false;

	Config.LoadConfig();

	logger.Initialize();
	wxHandleFatalExceptions();
	wxLogMessage("Initializing BodySlide...");
	wxLogMessage("Working directory: %s", wxGetCwd());

	wxInitAllImageHandlers();

	preview = nullptr;
	sliderView = nullptr;
	previewBaseNif = nullptr;
	outfitStudio = nullptr;

	Bind(wxEVT_CHAR_HOOK, &BodySlideApp::CharHook, this);

	SetDefaultConfig();

	string activeOutfit = Config.GetCString("SelectedOutfit");
	curOutfit = activeOutfit;

	LoadAllCategories();
	LoadAllGroups();

	int x = Config.GetIntValue("BodySlideFrame.x");
	int y = Config.GetIntValue("BodySlideFrame.y");
	int w = Config.GetIntValue("BodySlideFrame.width");	
	int h = Config.GetIntValue("BodySlideFrame.height");

	wxLogMessage("Loading BodySlide frame at X:%d Y:%d with W:%d H:%d...", x, y, w, h);
	sliderView = new BodySlideFrame(this, wxSize(w, h));
	sliderView->SetPosition(wxPoint(x, y));
	sliderView->Show();
	SetTopWindow(sliderView);

	if (straightOutfitStudio)
		LaunchOutfitStudio();

	wxLogMessage("BodySlide initialized.");
	return true;
}

void BodySlideApp::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
}

bool BodySlideApp::OnCmdLineParsed(wxCmdLineParser& parser) {
	straightOutfitStudio = parser.Found("os");
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

	logger.SetFormatter(false);
	wxLogFatalError("Unexpected exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format("Unexpected exception has occurred: %s, the program will terminate.", error), "Unexpected exception", wxICON_ERROR);
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

	logger.SetFormatter(false);
	wxLogFatalError("Unhandled exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format("Unhandled exception has occurred: %s, the program will terminate.", error), "Unhandled exception", wxICON_ERROR);
}

void BodySlideApp::OnFatalException() {
	logger.SetFormatter(false);
	wxLogFatalError("Fatal exception has occurred, the program will terminate.");
	wxMessageBox("Fatal exception has occurred, the program will terminate.", "Fatal exception", wxICON_ERROR);
}


void BodySlideApp::LoadData() {
	wxLogMessage("Loading initial data...");
	sliderView->Freeze();
	setupOutfit(curOutfit);
	string activeOutfit = Config.GetCString("SelectedOutfit");
	string activePreset = Config.GetCString("SelectedPreset");
	ActivatePreset(activePreset);
	sliderView->Thaw();
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
	wxLogMessage("Finished setting up set '%s'...", outfitName);
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
	wxLogMessage("Loading slider sets...");
	dataSets.Clear();
	outfitNameSource.clear();
	outfitNameOrder.clear();

	XmlFinder finder("SliderSets");
	while (!finder.atEnd()) {
		string filename = finder.next();
		SliderSetFile sliderDoc;
		sliderDoc.Open(filename);

		if (sliderDoc.fail())
			continue;
		vector<string> outfitNames;
		sliderDoc.GetSetNamesUnsorted(outfitNames, false);
		for (int i = 0; i < outfitNames.size(); i++) {
			outfitNameSource[outfitNames[i]] = filename;
			outfitNameOrder.push_back(outfitNames[i]);
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

	InitPreview();
	ActivatePreset(activePreset);

	sliderView->Thaw();
	sliderView->Refresh();
	wxLogMessage("Finished activating set '%s'...", outfitName);
}

void BodySlideApp::ActivatePreset(const string &presetName) {
	Config.SetValue("SelectedPreset", presetName);
	sliderManager.InitializeSliders(presetName);
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
		if (regularSlider)
			sliderView->AddSliderGUI(activeSet[i].name.c_str(), activeSet[i].bZap, !activeSet.GenWeights());
	}

	// Create category UI
	int iter = 0;
	if (catSliders.size() > 0) {
		for (auto &cat : sliderCategories) {
			string name = get<0>(cat);
			bool show = get<2>(cat);

			if (catSliders.size() > iter && catSliders[iter].size() > 0) {
				sliderView->AddCategorySliderUI(name, show, !activeSet.GenWeights());
				if (show)
					for (auto &s : catSliders[iter])
						sliderView->AddSliderGUI(activeSet[s].name.c_str(), activeSet[s].bZap, !activeSet.GenWeights());
			}
			iter++;
		}
	}
}

void BodySlideApp::LaunchOutfitStudio() {
	if (outfitStudio) {
		outfitStudio->SetFocus();
	}
	else {
		int x = Config.GetIntValue("OutfitStudioFrame.x");
		int y = Config.GetIntValue("OutfitStudioFrame.y");
		int w = Config.GetIntValue("OutfitStudioFrame.width");
		int h = Config.GetIntValue("OutfitStudioFrame.height");

		outfitStudio = new OutfitStudio(sliderView, wxPoint(x, y), wxSize(w, h), Config);
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

int BodySlideApp::WriteMorphTRI(const string& triPath, SliderSet& sliderSet, NifFile& nif, unordered_map<string, vector<ushort>> zapIndices) {
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

				vector<Vector3> verts;
				int shapeVertCount = nif.GetVertCountForShape(shape->second);
				shapeVertCount += zapIndices[shape->second].size();
				if (shapeVertCount > 0)
					verts.resize(shapeVertCount);
				else
					continue;

				currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);

				if (zapIndices[shape->second].size() > 0)
					for (int i = verts.size() - 1; i >= 0; i--)
						if (find(zapIndices[shape->second].begin(), zapIndices[shape->second].end(), i) != zapIndices[shape->second].end())
							verts.erase(verts.begin() + i);
				
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
	string inputFileName;
	bool freshLoad = false;
	inputFileName = activeSet.GetInputFileName();

	if (!previewBaseNif) {
		previewBaseNif = new NifFile();
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if (PreviewMod.Load(inputFileName))
			return;

		freshLoad = true;
		sliderManager.FlagReload(false);
	}
	else if ((previewBaseName != inputFileName) || sliderManager.NeedReload()) {
		delete previewBaseNif;
		previewBaseNif = new NifFile();
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if (PreviewMod.Load(inputFileName) != 0)
			return;

		freshLoad = true;
		sliderManager.FlagReload(false);
	}

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
	Config.SetDefaultValue("TargetGame", 2);
	targetGame = Config.GetIntValue("TargetGame");

	Config.SetDefaultValue("ShapeDataPath", wxGetCwd().ToStdString() + "\\ShapeData");
	Config.SetDefaultValue("WarnMissingGamePath", "true");
	Config.SetDefaultValue("BSATextureScan", "true");
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultValue("SelectedPreset", "");
	Config.SetDefaultValue("SelectedOutfit", "");

	wxString gameKey;
	wxString gameValueKey;
	switch (targetGame) {
	case FO3:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_fo3nv.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Bip");
		gameKey = "SOFTWARE\\Bethesda Softworks\\Fallout3";
		gameValueKey = "Installed Path";
		break;
	case FONV:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_fo3nv.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "Bip");
		gameKey = "SOFTWARE\\Bethesda Softworks\\FalloutNV";
		gameValueKey = "Installed Path";
		break;
	case SKYRIM:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_xpmse.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "NPC");
		gameKey = "SOFTWARE\\Bethesda Softworks\\Skyrim";
		gameValueKey = "Installed Path";
		break;
	default:
		Config.SetDefaultValue("Anim/DefaultSkeletonReference", "res\\skeleton_female.nif");
		Config.SetDefaultValue("Anim/SkeletonRootName", "NPC");
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

	if (!Config.Exists("GameDataPath")) {
		wxRegKey key(wxRegKey::HKLM, gameKey);
		if (key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data\\");
				Config.SetDefaultValue("GameDataPath", installPath.ToStdString());
				wxLogMessage("Registry game data path: %s", installPath);
			}
			else if (Config["WarnMissingGamePath"] == "true")
				wxMessageBox("Could not find both your game install path (registry key value) and GameDataPath in the configuration.", "Warning");
		}
		else if (Config["WarnMissingGamePath"] == "true")
			wxMessageBox("Could not find both your game installation (registry key) and GameDataPath in the configuration.", "Warning");
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);
}

void BodySlideApp::LoadAllCategories() {
	wxLogMessage("Loading slider categories...");
	cCollection.LoadCategories("SliderCategories");
}

void BodySlideApp::SetPresetGroups(const string& setName) {
	presetGroups.clear();
	gCollection.GetOutfitGroups(setName, presetGroups);

	if (presetGroups.empty()) {
		Config.GetValueArray("DefaultGroups", "GroupName", presetGroups);
		wxLogMessage("Using default groups for set '%s'.", setName);
	}
}

void BodySlideApp::LoadAllGroups() {
	wxLogMessage("Loading slider groups...");
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
			wxMessageBox("Failed to create group file", "Error", wxICON_ERROR);
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
		int ret = wxMessageBox("That group already exists in the specified file, do you wish to overwrite the group?", "Group exists", wxYES_NO | wxCANCEL);
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

	wxLogMessage("Loading presets for '%s'...", outfit);

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
		if (!Config.Exists("GameDataPath")) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox("WARNING: Game data path not configured. Would you like to show BodySlide where it is?", "Game not found", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogMessage("Aborted build without data path.");
					return 4;
				}
			}

			wxString cwd = wxGetCwd();
			wxString response = wxDirSelector("Please choose a directory to set as your Data path", cwd);
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
		int ret = wxMessageBox("WARNING: This will delete the output files from the output folder, potentially causing crashes.\n\nDo you want to continue?", "Clean Build", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning build.");
			return 4;
		}

		wxString removeHigh, removeLow;
		wxString msg = "Removed the following files:\n";
		bool genWeights = activeSet.GenWeights();

		if (genWeights)
			removeHigh = outFileNameSmall + "_1.nif";
		else
			removeHigh = outFileNameSmall + ".nif";
		
		bool remHigh = wxRemoveFile(removeHigh);
		if (remHigh)
			msg.Append(removeHigh + "\n");
		else
			msg.Append(removeHigh + " (no action)\n");

		if (!genWeights) {
			wxLogMessage("%s", msg);
			wxMessageBox(msg, "Process Successful");
			return 0;
		}

		removeLow = outFileNameSmall + "_0.nif";
		bool remLow = wxRemoveFile(removeLow);
		if (remLow)
			msg.Append(removeLow + "\n");
		else
			msg.Append(removeLow + " (no action)\n");

		wxLogMessage("%s", msg);
		wxMessageBox(msg, "Process Successful");
		return 0;
	}

	int error = nifSmall.Load(inputFileName);
	if (error) {
		wxLogError("Failed to load '%s' (%d)!", inputFileName, error);
		return 1;
	}

	if (nifBig.Load(inputFileName))
		return 1;

	vector<Vector3> vertsLow;
	vector<Vector3> vertsHigh;
	vector<ushort> zapIdx;
	unordered_map<string, vector<ushort>> zapIdxAll;

	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		if (!nifSmall.GetVertsForShape(it->second, vertsLow))
			continue;

		if (!nifBig.GetVertsForShape(it->second, vertsHigh))
			continue;

		zapIdxAll.emplace(it->second, vector<ushort>());

		ApplySliders(it->first, sliderManager.slidersSmall, vertsLow, zapIdx);
		zapIdx.clear();
		ApplySliders(it->first, sliderManager.slidersBig, vertsHigh, zapIdx);

		nifSmall.SetVertsForShape(it->second, vertsLow);
		nifSmall.DeleteVertsForShape(it->second, zapIdx);

		nifBig.SetVertsForShape(it->second, vertsHigh);
		nifBig.DeleteVertsForShape(it->second, zapIdx);

		zapIdxAll[it->second] = zapIdx;
	}

	/* Add RaceMenu TRI path for in-game morphs */
	if (tri) {
		string triPath = activeSet.GetOutputFilePath() + ".tri";
		string triPathTrimmed = triPath;
		triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
		triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

		if (!WriteMorphTRI(outFileNameBig, activeSet, nifBig, zapIdxAll)) {
			wxLogError("Failed to write TRI file to '%s'!", triPath);
			wxMessageBox(wxString().Format("Failed to write TRI file to the following location\n\n%s", triPath), "Unable to process", wxOK | wxICON_ERROR);
		}

		string triShapeLink;
		for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
			triShapeLink = it->second;
			if (tri && nifBig.GetVertCountForShape(triShapeLink) > 0) {
				nifSmall.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
				nifBig.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
				tri = false;
			}
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
			wxMessageBox(wxString().Format("Failed to build set to the following location\n\n%s", custName), "Unable to process", wxOK | wxICON_ERROR);

			custName = wxSaveFileSelector("Choose alternate file name", "*.nif", custName);
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
		wxMessageBox(wxString().Format("Failed to build set to the following location\n\n%s", custName), "Unable to process", wxOK | wxICON_ERROR);

		custName = wxSaveFileSelector("Choose alternate file name", "*.nif", custName);
		if (custName.IsEmpty()) {
			wxLogMessage("Aborted build when choosing alternate file name.");
			return 4;
		}

		useCustName = true;
		savedHigh = custName;
	}

	wxString msg = "Successfully processed the following files:\n";
	if (!savedLow.IsEmpty())
		msg.Append(savedLow += "\n");

	if (!savedHigh.IsEmpty())
		msg.Append(savedHigh);

	wxLogMessage("%s", msg);
	wxMessageBox(msg, "Process Successful");
	return 0;
}

int BodySlideApp::BuildListBodies(const vector<string>& outfitList, map<string, string>& failedOutfits, bool clean, bool tri, const string& custPath) {
	string datapath = custPath;
	string outFileNameSmall;
	string outFileNameBig;
	NifFile nifSmall;
	NifFile nifBig;
	SliderSet currentSet;
	DiffDataSets currentDiffs;
	wxProgressDialog* progWnd;

	wxLogMessage("Started batch build with options: Custom Path = %s, Cleaning = %s, TRI = %s",
		custPath.empty() ? "False" : custPath, clean ? "True" : "False", tri ? "True" : "False");

	if (clean) {
		int ret = wxMessageBox("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?", "Clean Batch Build", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning batch build.");
			return 1;
		}
	}

	string activePreset = Config["SelectedPreset"];

	if (datapath.empty()) {
		if (!Config.Exists("GameDataPath")) {
			if (clean) {
				wxLogError("Aborted batch clean with unconfigured data path. Files can't be removed that way.");
				wxMessageBox("WARNING: Game data path not configured. Files can't be removed that way.", "Game not found", wxOK | wxICON_ERROR);
				return 1;
			}
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox("WARNING: Game data path not configured. Continue saving files to the working directory?", "Game not found", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
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

	progWnd = new wxProgressDialog("Processing Outfits", "Starting...", 1000, nullptr, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_ELAPSED_TIME);
	progWnd->SetSize(400, 150);
	float progstep = 1000.0f / outfitList.size();
	int count = 1;

	for (auto &outfit : outfitList) {
		wxString progMsg = wxString::Format("Processing '%s' (%d of %d)...", outfit, count, outfitList.size());
		wxLogMessage(progMsg);
		progWnd->Update((int)(count*progstep), progMsg);
		count++;

		/* Load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			failedOutfits[outfit] = "No recorded outfit name source";
			continue;
		}

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			currentSet.Clear();
			currentDiffs.Clear();
			if (!sliderDoc.GetSet(outfit, currentSet)) {
				currentSet.SetBaseDataPath(Config["ShapeDataPath"]);
				currentSet.LoadSetDiffData(currentDiffs);
			}
			else
				failedOutfits[outfit] = "Unable to get slider set from file: " + outfitNameSource[outfit];
		}
		else {
			failedOutfits[outfit] = "Unable to open slider set file: " + outfitNameSource[outfit];
			continue;
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
				continue;

			string removeLow = removePath + "_0.nif";
			if (wxFileName::FileExists(removeLow))
				wxRemoveFile(removeLow);

			continue;
		}

		/* load iput nifs */
		if (nifSmall.Load(currentSet.GetInputFileName())) {
			failedOutfits[outfit] = "Unable to load input nif: " + currentSet.GetInputFileName();
			continue;
		}
		if (nifBig.Load(currentSet.GetInputFileName()))
			continue;

		/* Shape the NIF files */
		vector<Vector3> vertsLow;
		vector<Vector3> vertsHigh;
		vector<ushort> zapIdx;
		unordered_map<string, vector<ushort>> zapIdxAll;

		for (auto it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
			if (!nifSmall.GetVertsForShape(it->second, vertsLow))
				continue;
			if (!nifBig.GetVertsForShape(it->second, vertsHigh))
				continue;

			float vsmall;
			float vbig;
			vector<int> clamps;
			zapIdx.clear();
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

				vsmall = sliderManager.GetSmallPresetValue(activePreset, currentSet[s].name, currentSet[s].defSmallValue / 100.0f);
				vbig = sliderManager.GetBigPresetValue(activePreset, currentSet[s].name, currentSet[s].defBigValue / 100.0f);

				for (auto &sliderSmall : sliderManager.slidersSmall) {
					if (sliderSmall.name == currentSet[s].name && sliderSmall.changed && !sliderSmall.clamp) {
						vsmall = sliderSmall.value;
						break;
					}
				}

				for (auto &sliderBig : sliderManager.slidersBig) {
					if (sliderBig.name == currentSet[s].name && sliderBig.changed && !sliderBig.clamp) {
						vbig = sliderBig.value;
						break;
					}
				}

				if (currentSet[s].bInvert) {
					vsmall = 1.0f - vsmall;
					vbig = 1.0f - vbig;
				}

				if (currentSet[s].bZap) {
					if (vbig > 0.0f) {
						currentDiffs.GetDiffIndices(dn, target, zapIdx);
						zapIdxAll[it->second] = zapIdx;
					}
					continue;
				}

				currentDiffs.ApplyDiff(dn, target, vsmall, &vertsLow);
				currentDiffs.ApplyDiff(dn, target, vbig, &vertsHigh);
			}

			if (!clamps.empty()) {
				for (auto &c : clamps) {
					string dn = currentSet[c].TargetDataName(it->first);
					string target = it->first;
					if (currentSet[c].defSmallValue > 0)
						currentDiffs.ApplyClamp(dn, target, &vertsLow);
					if (currentSet[c].defBigValue > 0)
						currentDiffs.ApplyClamp(dn, target, &vertsHigh);
				}
			}

			nifSmall.SetVertsForShape(it->second, vertsLow);
			nifSmall.DeleteVertsForShape(it->second, zapIdx);

			nifBig.SetVertsForShape(it->second, vertsHigh);
			nifBig.DeleteVertsForShape(it->second, zapIdx);
		}

		/* Create directory for the outfit */
		wxString dir = datapath + currentSet.GetOutputPath();
		bool success = wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

		if (!success) {
			failedOutfits[outfit] = "Unable to create destination directory: " + dir.ToStdString();
			continue;
		}

		outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;

		/* Add RaceMenu TRI path for in-game morphs */
		bool triEnd = tri;
		if (triEnd) {
			string triPath = currentSet.GetOutputFilePath() + ".tri";
			string triPathTrimmed = triPath;
			triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
			triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

			if (!WriteMorphTRI(outFileNameBig, currentSet, nifBig, zapIdxAll)) {
				wxLogError("Failed to create TRI file to '%s'!", triPath);
				wxMessageBox(wxString().Format("Failed to create TRI file in the following location\n\n%s", triPath), "TRI File", wxOK | wxICON_ERROR);
			}

			for (auto it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
				string triShapeLink = it->second;
				if (triEnd && nifBig.GetVertCountForShape(triShapeLink) > 0) {
					nifSmall.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
					nifBig.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
					triEnd = false;
				}
			}
		}

		/* Set filenames for the outfit */
		if (currentSet.GenWeights()) {
			outFileNameSmall += "_0.nif";
			outFileNameBig += "_1.nif";
			if (nifBig.Save(outFileNameBig)) {
				failedOutfits[outfit] = "Unable to save nif file: " + outFileNameBig;
				continue;
			}
			if (nifSmall.Save(outFileNameSmall)) {
				failedOutfits[outfit] = "Unable to save nif file: " + outFileNameSmall;
				continue;
			}
		}
		else {
			outFileNameBig += ".nif";
			if (nifBig.Save(outFileNameBig)) {
				failedOutfits[outfit] = "Unable to save nif file: " + outFileNameBig;
				continue;
			}
		}
	}

	progWnd->Update(1000);
	delete progWnd;

	if (failedOutfits.size() > 0)
		return 3;

	return 0;
}

float BodySlideApp::GetSliderValue(const wxString& sliderName, bool isLo) {
	string sstr = sliderName.ToAscii().data();
	return sliderManager.GetSlider(sstr, isLo);
}

void BodySlideApp::SetSliderValue(const wxString& sliderName, bool isLo, float val) {
	string sstr = sliderName.ToAscii().data();
	sliderManager.SetSlider(sstr, isLo, val);
}

void BodySlideApp::SetSliderChanged(const wxString& sliderName, bool isLo) {
	string sstr = sliderName.ToAscii().data();
	sliderManager.SetChanged(sstr, isLo);
}

int BodySlideApp::SaveSliderPositions(const string& outputFile, const string& presetName, vector<string>& groups) {
	string outfitName = Config.GetCString("SelectedOutfit");
	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

BodySlideFrame::BodySlideFrame(BodySlideApp* app, const wxSize &size) : delayLoad(this, DELAYLOAD_TIMER) {
	this->app = app;
	rowCount = 0;

	wxXmlResource* rsrc = wxXmlResource::Get();
	rsrc->InitAllHandlers();
	bool loaded = rsrc->Load("res\\BodyslideFrame.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load BodyslideFrame.xrc file!", "Error", wxICON_ERROR);
		Close(true);
		return;
	}

	rsrc->LoadFrame(this, GetParent(), "BodyslideFrame");
	if (!loaded) {
		wxMessageBox("Failed to load BodySlide frame!", "Error", wxICON_ERROR);
		Close(true);
		return;
	}

	delayLoad.Start(100, true);

	SetDoubleBuffered(true);
	SetIcon(wxIcon("res\\outfitstudio.png", wxBITMAP_TYPE_PNG));
	SetSize(size);

	batchBuildList = nullptr;
	wxMenu* srchMenu = rsrc->LoadMenu("menuGroupContext");
	wxMenu* outfitsrchMenu = rsrc->LoadMenu("menuOutfitSrchContext");

	search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 23), wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->ShowCancelButton(true);
	search->SetDescriptiveText("Group Filter");
	search->SetToolTip("Filter by group");
	search->SetMenu(srchMenu);

	outfitsearch = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 23), wxTE_PROCESS_ENTER);
	outfitsearch->ShowSearchButton(true);
	outfitsearch->ShowCancelButton(true);
	outfitsearch->SetDescriptiveText("Outfit Filter");
	outfitsearch->SetToolTip("Filter by outfit");
	outfitsearch->SetMenu(outfitsrchMenu);

	rsrc->AttachUnknownControl("searchHolder", search, this);
	rsrc->AttachUnknownControl("outfitsearchHolder", outfitsearch, this);

	wxScrolledWindow* scrollWindow = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (scrollWindow) {
		scrollWindow->SetScrollRate(5, 26);
		scrollWindow->SetFocusIgnoringChildren();
		scrollWindow->SetBackgroundColour(wxColor(0x40, 0x40, 0x40));
		scrollWindow->Bind(wxEVT_ENTER_WINDOW, &BodySlideFrame::OnEnterSliderWindow, this);
	}

	wxString val = Config.GetCString("LastGroupFilter");
	search->ChangeValue(val);
	val = Config.GetCString("LastOutfitFilter");
	outfitsearch->ChangeValue(val);

	wxButton* btnAbout = (wxButton*)FindWindowByName("btnAbout");
	if (btnAbout)
		btnAbout->SetBitmap(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(15, 15)));
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
		XRCCTRL(*this, "lblLowWt", wxStaticText)->GetContainingSizer()->ShowItems(true);
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Show(false);
		sliderLayout->SetCols(6);
	}
	else {
		XRCCTRL(*this, "lblLowWt", wxStaticText)->GetContainingSizer()->ShowItems(false);
		XRCCTRL(*this, "lblSingleWt", wxStaticText)->Show(true);
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
		child->SetBackgroundColour(wxColor(90, 90, 90));
		sliderLayout->Add(child, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	wxCheckBox* check = new wxCheckBox(scrollWindow, wxID_ANY, "");
	check->SetName(name);
	sliderLayout->Add(check, 0, wxRIGHT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 5);
	check->SetValue(show);
	check->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnCategoryCheckChanged, this);

	child = new wxStaticText(scrollWindow, wxID_ANY, name);
	child->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Andalus"));
	child->SetForegroundColour(wxColor(200, 200, 200));
	sliderLayout->Add(child, 0, wxLEFT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

	if (!oneSize) {
		child = new wxPanel(scrollWindow);
		child->SetBackgroundColour(wxColor(90, 90, 90));
		sliderLayout->Add(child, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	sliderLayout->AddSpacer(0);
	scrollWindow->FitInside();
}

void BodySlideFrame::AddSliderGUI(const wxString& name, bool isZap, bool oneSize) {
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
		sd->lblSliderLo = new wxStaticText(scrollWindow, wxID_ANY, name, wxDefaultPosition, wxSize(-1, 22), wxALIGN_CENTER_HORIZONTAL);
		sd->lblSliderLo->SetBackgroundColour(wxColor(0x40, 0x40, 0x40));
		sd->lblSliderLo->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
		sliderLayout->Add(sd->lblSliderLo, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT, 5);

		if (isZap) {
			sd->zapCheckLo = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZLO");
			sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(sd->zapCheckLo, 0, wxALIGN_LEFT, 0);
		}
		else {
			sd->sliderLo = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 22), wxSL_AUTOTICKS | wxSL_BOTTOM | wxSL_HORIZONTAL);
			sd->sliderLo->SetMaxSize(wxSize(-1, 22));
			sd->sliderLo->SetTickFreq(5);
			sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
			sd->sliderLo->SetName(name + "|LO");
			sliderLayout->Add(sd->sliderLo, 1, wxALIGN_LEFT | wxEXPAND, 0);

			sd->sliderReadoutLo = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, 18), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
			sd->sliderReadoutLo->SetName(name + "|RLO");
			sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);
			sliderLayout->Add(sd->sliderReadoutLo, 0, wxALL, 0);
		}
	}

	sd->lblSliderHi = new wxStaticText(scrollWindow, wxID_ANY, name, wxDefaultPosition, wxSize(-1, 22), wxALIGN_CENTER_HORIZONTAL);
	sd->lblSliderHi->SetBackgroundColour(wxColor(0x40, 0x40, 0x40));
	sd->lblSliderHi->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
	sliderLayout->Add(sd->lblSliderHi, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT, 5);

	if (isZap) {
		sd->zapCheckHi = new wxCheckBox(scrollWindow, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZHI");
		sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(sd->zapCheckHi, 0, wxALIGN_LEFT, 0);
	}
	else {
		sd->sliderHi = new wxSlider(scrollWindow, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 22), wxSL_AUTOTICKS | wxSL_HORIZONTAL);
		sd->sliderHi->SetMaxSize(wxSize(-1, 22));
		sd->sliderHi->SetTickFreq(5);
		sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
		sd->sliderHi->SetName(name + "|HI");
		sliderLayout->Add(sd->sliderHi, 1, wxALIGN_LEFT | wxEXPAND, 0);

		sd->sliderReadoutHi = new wxTextCtrl(scrollWindow, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, 18), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
		sd->sliderReadoutHi->SetName(name + "|RHI");
		sd->sliderReadoutHi->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), nullptr, this);
		sliderLayout->Add(sd->sliderReadoutHi, 0, wxRIGHT, 10);
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
			sd->sliderReadoutHi->SetValue(wxString::Format("%d%%", intval));
			sd->sliderHi->SetValue(intval);
		}
		else
			sd->zapCheckHi->SetValue((intval > 0));
	}
	else if (!sd->oneSize){
		if (!sd->isZap) {
			sd->sliderReadoutLo->SetValue(wxString::Format("%d%%", intval));
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
		sd->sliderReadoutLo->SetValue(wxString::Format("%d%%", event.GetPosition()));
	else
		sd->sliderReadoutHi->SetValue(wxString::Format("%d%%", event.GetPosition()));

	app->UpdatePreview();
}

void BodySlideFrame::OnSliderReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
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
	w->GetValue().ToDouble(&v);
	w->ChangeValue(wxString::Format("%0.0f%%", v));

	app->SetSliderValue(name, isLo, (float)v / 100.0f);

	if (isLo)
		sd->sliderLo->SetValue(v);
	else
		sd->sliderHi->SetValue(v);

	app->UpdatePreview();

	event.Skip();
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

	string sliderName = sn.ToAscii().data();
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
	wxMultiChoiceDialog chooser(this, "Choose groups to filter outfit list", "Choose Groups", grpChoices);
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

	wxFileDialog saveGroupDialog(this, "Choose or create group file", "SliderGroups", wxEmptyString, "Group Files (*.xml)|*.xml", wxFD_SAVE);
	if (saveGroupDialog.ShowModal() == wxID_CANCEL)
		return;

	string fName = saveGroupDialog.GetPath();
	string gName;
	int ret;

	do {
		gName = wxGetTextFromUser("What would you like the new group to be called?", "New Group Name");
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
	string sstr = event.GetString().ToAscii().data();
	app->ActivateOutfit(sstr);
}

void BodySlideFrame::OnChoosePreset(wxCommandEvent& event) {
	string sstr = event.GetString().ToAscii().data();
	app->ActivatePreset(sstr);
}

void BodySlideFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
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
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format("Failed to save preset (%d)!", error), "Error");
	}

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
}

void BodySlideFrame::OnGroupManager(wxCommandEvent& WXUNUSED(event)) {
	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* groupManager = rsrc->LoadDialog(this, "dlgGroupManager");
	if (!groupManager)
		return;

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

	wxCheckBox* cbRaceMenu = (wxCheckBox*)FindWindowByName("cbRaceMenu");
	bool tri = false;
	if (cbRaceMenu)
		tri = cbRaceMenu->IsChecked();

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

	wxCheckBox* cbRaceMenu = (wxCheckBox*)FindWindowByName("cbRaceMenu");
	bool custpath = false;
	bool clean = false;
	bool tri = false;
	if (cbRaceMenu)
		tri = cbRaceMenu->IsChecked();

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
		string path = wxDirSelector("Choose a folder to contain the saved files");
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
		wxMessageBox("All sets processed successfully!", "Complete", wxICON_INFORMATION);
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto &e : failedOutfits) {
			wxLogError("Failed to build '%s': %s", e.first, e.second);
			errlist.Add(e.first + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(this, "The following sets failed", "Failed", errlist, (void**)0, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
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
	wxChoice* choiceSkeletonRoot = XRCCTRL(*parent, "choiceSkeletonRoot", wxChoice);
	switch (choiceTargetGame->GetSelection()) {
		case FO3:
		case FONV:
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case SKYRIM:
		default:
			choiceSkeletonRoot->SetStringSelection("NPC");
			break;
	}
}

void BodySlideFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(wxSize(475, 400));
		settings->CenterOnParent();

		wxChoice* choiceTargetGame = XRCCTRL(*settings, "choiceTargetGame", wxChoice);
		choiceTargetGame->Select(Config.GetIntValue("TargetGame"));
		
		wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*settings, "dpGameDataPath", wxDirPickerCtrl);
		wxString gameDataPath = Config["GameDataPath"];
		dpGameDataPath->SetPath(gameDataPath);

		wxCheckBox* cbBSATextures = XRCCTRL(*settings, "cbBSATextures", wxCheckBox);
		cbBSATextures->SetValue(Config["BSATextureScan"] != "false");

		wxCheckBox* cbLeftMousePan = XRCCTRL(*settings, "cbLeftMousePan", wxCheckBox);
		cbLeftMousePan->SetValue(Config["Input/LeftMousePan"] != "false");

		wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*settings, "fpSkeletonFile", wxFilePickerCtrl);
		fpSkeletonFile->SetPath(Config["Anim/DefaultSkeletonReference"]);

		wxChoice* choiceSkeletonRoot = XRCCTRL(*settings, "choiceSkeletonRoot", wxChoice);
		choiceSkeletonRoot->SetStringSelection(Config["Anim/SkeletonRootName"]);
		
		settings->Bind(wxEVT_CHOICE, &BodySlideFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			Config.SetValue("TargetGame", choiceTargetGame->GetSelection());

			if (!dpGameDataPath->GetPath().IsEmpty()) {
				wxFileName gameDataDir = dpGameDataPath->GetDirName();
				gameDataDir.MakeRelativeTo();
				Config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToStdString());
				FSManager::del();
			}

			Config.SetValue("BSATextureScan", cbBSATextures->IsChecked() ? "true" : "false");
			Config.SetValue("Input/LeftMousePan", cbLeftMousePan->IsChecked() ? "true" : "false");

			wxFileName skeletonFile = fpSkeletonFile->GetFileName();
			skeletonFile.MakeRelativeTo();
			Config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToStdString());

			Config.SetValue("Anim/SkeletonRootName", choiceSkeletonRoot->GetStringSelection().ToStdString());
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
	wxSize p = event.GetSize();
	Config.SetValue("BodySlideFrame.width", p.x);
	Config.SetValue("BodySlideFrame.height", p.y);
	event.Skip();
}
