#include "BodySlideApp.h"

ConfigurationManager Config;

BEGIN_EVENT_TABLE(BodySlideFrame, wxFrame)
	EVT_MENU(wxID_EXIT, BodySlideFrame::OnExit)
	EVT_ACTIVATE(BodySlideFrame::OnActivateFrame)
	EVT_ICONIZE(BodySlideFrame::OnIconizeFrame)
	EVT_COMMAND_SCROLL(wxID_ANY, BodySlideFrame::OnSliderChange)
	EVT_TEXT_ENTER(wxID_ANY, BodySlideFrame::OnSliderReadoutChange)
	EVT_TEXT(XRCID("searchHolder"), BodySlideFrame::OnSearchChange)
	EVT_TEXT(XRCID("outfitsearchHolder"), BodySlideFrame::OnOutfitSearchChange)
    EVT_TIMER(DELAYLOAD_TIMER, BodySlideFrame::OnDelayLoad)
	EVT_CHOICE(XRCID("outfitChoice"), BodySlideFrame::OnChooseOutfit)
	EVT_CHOICE(XRCID("presetChoice"), BodySlideFrame::OnChoosePreset)

	EVT_BUTTON(XRCID("btnPreviewHi"), BodySlideFrame::OnPreviewHi)
	EVT_BUTTON(XRCID("btnPreviewLo"), BodySlideFrame::OnPreviewLo)
	EVT_BUTTON(XRCID("btnBuildBatch"), BodySlideFrame::OnBatchBuild)
	EVT_BUTTON(XRCID("btnBuild"), BodySlideFrame::OnBuildBodies)
	EVT_BUTTON(XRCID("btnOutfitStudio"), BodySlideFrame::OnOutfitStudio)
	EVT_BUTTON(XRCID("btnPresets"), BodySlideFrame::OnSavePreset)
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
	if (previewBaseNif != NULL) {
		delete previewBaseNif;
		previewBaseNif = NULL;
	}
	if (preview0) {
		ClosePreview(SMALL_PREVIEW);
	}
	if (preview1) {
		ClosePreview(BIG_PREVIEW);
	}

	Config.SaveConfig("Config.xml");
}

bool BodySlideApp::OnInit() {
	/*
	1 2 3 0
	0 4 5 0
	1 0 6 0
	0 0 0 1

	Mat4 m;
	m[0] = 1.0f; 
	m[1] = 2.0f;
	m[2] = 3.0f;
	m[3] = 0.0f;
	
	m[4] = 0.0f;
	m[5] = 4.0f;
	m[6] = 5.0f;
	m[7] = 0.0f;
	
	m[8] = 1.0f;
	m[9] = 0.0f;
	m[10] = 6.0f;
	m[11] = 0.0f;
	
	m[12] = 0.0f;
	m[13] = 0.0;
	m[14] = 0.0;
	m[15] = 1.0f;

	Mat4 inv = m.Inverse();
	*/

	wxInitAllImageHandlers();

	preview0 = NULL;
	preview1 = NULL;
	sliderView = NULL;
	previewBaseNif = NULL;
	outfitStudio = NULL;

	Bind(wxEVT_CHAR_HOOK, &BodySlideApp::CharHook, this);

	/* Init CommonControls? */

	Config.LoadConfig();
	SetDefaultConfig();	
	LoadAllCategories();

	int w = Config.GetIntValue("BodySlideFrame.width", 800);	
	int h = Config.GetIntValue("BodySlideFrame.height", 600);

	sliderView = new BodySlideFrame(this, "BodySlide", wxPoint(1, 1), wxSize(w, h));
	sliderView->SetPosition(wxPoint(Config.GetIntValue("BodySlideFrame.x", 100), Config.GetIntValue("BodySlideFrame.y", 100)));
	sliderView->Show(true);
	SetTopWindow(sliderView);
	
	/*
	outfitStudio = new OutfitStudio(NULL, wxDefaultPosition, wxDefaultSize);
	outfitStudio->Show(true);
	SetTopWindow(outfitStudio);
	*/

	string activeOutfit = Config.GetCString("SelectedOutfit", "CBBE Body");
	curOutfit = activeOutfit;

	LoadAllGroups();

	// If WIP preview version
	//wxMessageBox("WIP PREVIEW, DON'T SHARE THIS VERSION ANYWHERE! -ousnius", "Alert");

	return true;
}

void BodySlideApp::LoadData() {
	sliderView->Freeze();
	setupOutfit(curOutfit);
	string activeOutfit = Config.GetCString("SelectedOutfit", "CBBE Body");
	string activePreset = Config.GetCString("SelectedPreset", "CBBE");
	ActivatePreset(activePreset);
	sliderView->Thaw();
}

void BodySlideApp::setupOutfit(const string& outfitName) {
	if (!sliderView)
		return;

	sliderView->ClearOutfitList();
	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	if (preview0)
		preview0->Close();

	if (preview1)
		preview1->Close();
	
	string activeOutfit = Config.GetCString("SelectedOutfit", "CBBE Body");
	string activePreset = Config.GetCString("SelectedPreset", "CBBE");
	sliderManager.ClearSliders();
//	outfitNameSource.clear();
	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(activeOutfit);
	PopulatePresetList(activePreset);
	createSliders(outfitName);
}
int BodySlideApp::createSetSliders(const string& outfit, bool hideAll) {
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
		} else
			return 2;
	}	
	return 0;
}

int BodySlideApp::createSliders(const string& outfit, bool hideAll) {
	int ret = LoadSliderSets();
	if (ret)
		return ret;

	createSetSliders(outfit, hideAll);
	PopulateOutfitList(outfit);
	string activePreset = Config.GetCString("SelectedPreset", "CBBE");
	sliderManager.InitializeSliders(activePreset);

	return 0;
}

void BodySlideApp::RefreshOutfitList() {
	LoadSliderSets();
	string activeOutfit = Config.GetCString("SelectedOutfit", "CBBE Body");
	PopulateOutfitList(activeOutfit);
}

int BodySlideApp::LoadSliderSets() {
	WIN32_FIND_DATAA wfd;
	HANDLE hfind;
	string filename;
	DWORD searchStatus = 0;
	vector<string> outfitNames;

	dataSets.Clear();
	outfitNameSource.clear();
	outfitNameOrder.clear();

	//activeSet.Clear();

	hfind = FindFirstFileA("SliderSets\\*.xml", &wfd);

	if (hfind == INVALID_HANDLE_VALUE) {
		return 3;
	}

	while (searchStatus != ERROR_NO_MORE_FILES) {
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{
			filename = "SliderSets\\";
			filename += wfd.cFileName;

			SliderSetFile sliderDoc;
			sliderDoc.Open(filename);

			if (!sliderDoc.fail()) {
				sliderDoc.GetSetNamesUnsorted(outfitNames,false);
				for (int i = 0; i < outfitNames.size(); i++) {
					outfitNameSource[outfitNames[i]] = filename;
					outfitNameOrder.push_back(outfitNames[i]);
				}/*
				if(!sliderDoc.GetSet(outfit,activeSet)) {
					curOutfit = outfit;
					activeSet.SetBaseDataPath(config["ShapeDataPath"]);

					activeSet.LoadSetDiffData(dataSets);

					sliderManager.AddSlidersInSet(activeSet, hideAll);
					sliderManager.InitializeSliders("CBBE");

				}*/
				
			}
			if (!FindNextFileA(hfind, &wfd)) {
				searchStatus = GetLastError();
			} else 
				searchStatus = 0;
		}
	}	
	FindClose(hfind);
		
	ungroupedOutfits.clear();
	for (auto o: outfitNameSource) {
		vector<string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	return 0;
}

void BodySlideApp::ActivateOutfit(const string& outfitName) {
	Config.SetValue("SelectedOutfit", outfitName);
	sliderView->Freeze();

	sliderView->ClearPresetList();
	sliderView->ClearSliderGUI();

	if (preview0)
		preview0->Close();
	if (preview1)
		preview1->Close();

	string activePreset = Config.GetCString("SelectedPreset", "CBBE");

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(activePreset);
	createSetSliders(outfitName);
	ActivatePreset(activePreset);
	PopulateOutfitList(outfitName);

	sliderView->Thaw();
	sliderView->Refresh();
}

void BodySlideApp::ActivatePreset(const string &presetName) {
	Config.SetValue("SelectedPreset", presetName);
	sliderManager.InitializeSliders(presetName);
	Slider* slider;
	for (int i = 0; i < sliderManager.SlidersBig.size(); i++) {		
		slider = &sliderManager.SlidersBig[i];
		sliderView->SetSliderPosition(slider->Name.c_str(), slider->Value, SLIDER_HI);

		slider = &sliderManager.SlidersSmall[i];
		sliderView->SetSliderPosition(slider->Name.c_str(), slider->Value, SLIDER_LO);
	}	
	if (preview0) {
		UpdatePreview(SMALL_PREVIEW);
	}
	if (preview1) {
		UpdatePreview(BIG_PREVIEW);
	}
}

void BodySlideApp::RefreshSliders() {
	Slider* slider;
	for (int i = 0; i < sliderManager.SlidersBig.size(); i++) {		
		slider = &sliderManager.SlidersBig[i];
		sliderView->SetSliderPosition(slider->Name.c_str(), slider->Value, SLIDER_HI);

		slider = &sliderManager.SlidersSmall[i];
		sliderView->SetSliderPosition(slider->Name.c_str(), slider->Value, SLIDER_LO);
	}	
	if (preview0) {
		UpdatePreview(SMALL_PREVIEW);
	}
	if (preview1) {
		UpdatePreview(BIG_PREVIEW);
	}
}

void BodySlideApp::PopulatePresetList(const string& select) {
	vector<string> presets;
	wxArrayString items;
	sliderManager.GetPresetNames(presets);
	items.reserve(presets.size());
	for(int i=0;i<presets.size();i++){ 
		items.Add(presets[i].c_str());
	}
	sliderView->PopulatePresetList(items,select);
}

void BodySlideApp::PopulateOutfitList(const string& select) {
	string myselect = curOutfit;
	if (!select.empty()) {
		myselect = select;
	}

	wxArrayString items;

	size_t n = outfitNameSource.size();
	if (n == 0) 
		return;

	ApplyOutfitFilter();

	for (auto fo: filteredOutfits)
		items.Add(fo);

	//map<string,string>::iterator outfitIter;
	//for (outfitIter = outfitNameSource.begin(); outfitIter != outfitNameSource.end(); ++outfitIter) {
	//	items.Add(outfitIter->first.c_str());
	//}

	sliderView->PopulateOutfitList(items, wxString(myselect.c_str()));
}

void BodySlideApp::DisplayActiveSet() {
	if (activeSet.GenWeights()) {
		sliderView->ShowLowColumn(true);
	} else {
		sliderView->ShowLowColumn(false);
	}
	
	// Category name, slider names, hidden flag
	vector<tuple<string,vector<string>,bool>> sliderCategories;
	vector<string> cats;
	cCollection.GetAllCategories(cats);

	// Populate category data
	for (auto cat: cats) {
		vector<string> catSliders;
		cCollection.GetCategorySliders(cat, catSliders);

		if (catSliders.size() > 0) {
			bool catHidden = cCollection.GetCategoryHidden(cat);
			sliderCategories.push_back(make_tuple(cat, catSliders, !catHidden));
		} else
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
		for (auto cat: sliderCategories) {
			catSliders.push_back(vector<int>());
			if (find(get<1>(cat).begin(), get<1>(cat).end(), activeSet[i].Name) != get<1>(cat).end()) {
				catSliders[iter].push_back(i);
				regularSlider = false;
				break;
			}
			iter++;
		}
		if (regularSlider)
			sliderView->AddSliderGUI(activeSet[i].Name.c_str(), activeSet[i].bZap, !activeSet.GenWeights());
	}
		
	// Create category UI
	int iter = 0;
	if (catSliders.size() > 0) {
		for (auto cat: sliderCategories) {
			string name = get<0>(cat);
			bool show = get<2>(cat);

			if (catSliders.size() > iter && catSliders[iter].size() > 0) {
				sliderView->AddCategorySliderUI(name, show, !activeSet.GenWeights());
				if (show) {
					for (auto s: catSliders[iter])
						sliderView->AddSliderGUI(activeSet[s].Name.c_str(), activeSet[s].bZap, !activeSet.GenWeights());
				}
			}
			iter++;
		}
	}
}

void BodySlideApp::LaunchOutfitStudio() {
	if (outfitStudio != NULL) {
		outfitStudio->SetFocus();
	} else {
		int w = Config.GetIntValue("OutfitStudioFrame.width", 990);	
		int h = Config.GetIntValue("OutfitStudioFrame.height", 757);
		wxPoint loc (Config.GetIntValue("OutfitStudioFrame.x", 100), Config.GetIntValue("OutfitStudioFrame.y", 100));
		outfitStudio = new OutfitStudio(sliderView, loc, wxSize(w, h), Config);		
		outfitStudio->Show();
	}
}

void BodySlideApp::ApplySliders(const string& targetShape, vector<Slider>& sliderSet, vector<vec3>& verts, vector<ushort>& ZapIdx, vector<vec2>* uvs) {	
	int j;
	for (auto slider: sliderSet) {
		float val = slider.Value;
		if (slider.zap) {
			if (val > 0) {
				for (j = 0; j < slider.linkedDataSets.size(); j++) {
					dataSets.GetDiffIndices(slider.linkedDataSets[j], targetShape, ZapIdx);
				}
			}
		} else {					
			if (slider.invert) 
				val = 1.0f - val;
			for (j = 0; j < slider.linkedDataSets.size(); j++) {
				if (slider.uv) {
					dataSets.ApplyUVDiff(slider.linkedDataSets[j], targetShape, val, uvs);
				} else {
					dataSets.ApplyDiff(slider.linkedDataSets[j], targetShape, val, &verts);			
				}
			}		
		}
	}	
	for (auto slider: sliderSet) {
		if (slider.clamp && slider.Value > 0) {
			for (j = 0; j < slider.linkedDataSets.size(); j++) {
				dataSets.ApplyClamp(slider.linkedDataSets[j], targetShape, &verts);
			}
		}
	}
}

int BodySlideApp::WriteMorphTRI(const string& triPath, SliderSet& sliderSet, NifFile& nif, unordered_map<string, vector<ushort>> zapIndices) {
	DiffDataSets currentDiffs;
	sliderSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	string triFilePath = triPath + ".tri";
	map<string, string>::iterator shape;

	for (shape = sliderSet.TargetShapesBegin(); shape != sliderSet.TargetShapesEnd(); ++shape) {
		for (int s = 0; s < sliderSet.size(); s++) {
			string dn = sliderSet[s].TargetDataName(shape->first);
			string target = shape->first;
			if (dn.empty())
				continue;

			if (!sliderSet[s].bUV && !sliderSet[s].bClamp && !sliderSet[s].bZap) {
				MorphDataPtr morph = make_shared<MorphData>();
				morph->name = sliderSet[s].Name;

				vector<vec3> verts;
				ushort shapeVertCount = nif.GetVertCountForShape(shape->second);
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

				for (ushort i = 0; i < verts.size(); i++)
					if (!verts[i].IsZero(true))
						morph->offsets.emplace(i, verts[i]);

				if (morph->offsets.size() > 0)
					tri.AddMorph(shape->second, morph);
			}
		}
	}

	if (!tri.Write(triFilePath, true))
		return false;

	return true;
}

void BodySlideApp::ShowPreview(char PreviewType) {
	if (PreviewType == SMALL_PREVIEW && preview0 != NULL) 
		return;
	if (PreviewType == BIG_PREVIEW && preview1 != NULL) 
		return;
	
	string inputFileName;
	bool freshload = false;
	inputFileName = activeSet.GetInputFileName();

	if (previewBaseNif == NULL) {
		previewBaseNif = new NifFile;
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if (PreviewMod.Load(inputFileName) != 0) {
			return;
		}
		freshload = true;
		sliderManager.FlagReload(false);
	} else if ((previewBaseName != inputFileName) || sliderManager.NeedReload()) {
		delete previewBaseNif;
		previewBaseNif = new NifFile;
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if (PreviewMod.Load(inputFileName) != 0) {
			return;
		}
		freshload = true;
		sliderManager.FlagReload(false);
	} 

	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<vector2> uvs0;
	vector<ushort> ZapIdx;
	vector3 v;
	for (it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		v = activeSet.GetTargetVirtualOffset(it->first);
		if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f) {
			previewBaseNif->VirtualOffsetShape(it->second, v, false);
		}

		ZapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts0))
			continue;		
		previewBaseNif->GetUvsForShape(it->second, uvs0);

		if (PreviewType == SMALL_PREVIEW) {
			ApplySliders(it->first, sliderManager.SlidersSmall, verts0, ZapIdx, &uvs0);
		} else {
			ApplySliders(it->first, sliderManager.SlidersBig, verts0, ZapIdx, &uvs0);
		}
		
		// Zap deleted verts before preview
		if (freshload && ZapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(it->second, verts0);
			PreviewMod.SetUvsForShape(it->second, uvs0);
			//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0], it->first,ZapIdx);
			PreviewMod.DeleteVertsForShape(it->second, ZapIdx);
		} else if (ZapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0], it->first,ZapIdx);
			for (int z = ZapIdx.size() - 1; z >= 0; z--) {
				verts0.erase(verts0.begin() + ZapIdx[z]);
				uvs0.erase(uvs0.begin() + ZapIdx[z]);
			}
			PreviewMod.SetVertsForShape(it->second, verts0);
			PreviewMod.SetUvsForShape(it->second, uvs0);
		} else {
			// No zapping needed -- just show all the verts.  
			PreviewMod.SetVertsForShape(it->second, verts0);
			PreviewMod.SetUvsForShape(it->second, uvs0);
		}
	}

	vector<string> shapeNames;
	string baseGamePath = Config["GameDataPath"];
	if (PreviewType == SMALL_PREVIEW) {
		preview0 = new PreviewWindow((HWND)sliderView->GetHWND(), &PreviewMod, SMALL_PREVIEW);
		preview0->SetBaseDataPath(baseGamePath);
		PreviewMod.GetShapeList(shapeNames);
		for (auto s: shapeNames) {
			preview0->AddNifShapeTexture(&PreviewMod, s);
			preview0->Refresh();
		}

	} else  {
		preview1 = new PreviewWindow((HWND)sliderView->GetHWND(), &PreviewMod, BIG_PREVIEW);
		preview1->SetBaseDataPath(baseGamePath);
		PreviewMod.GetShapeList(shapeNames);
		for (auto s: shapeNames) {
			preview1->AddNifShapeTexture(&PreviewMod, s);
			preview1->Refresh();
		}
	}
}

void BodySlideApp::UpdatePreview(char PreviewType) {
	if (PreviewType == SMALL_PREVIEW) {
		if (preview0 == NULL) return;
	}
	else {
		if (preview1 == NULL) return;
	}

	if (previewBaseNif == NULL)
		return;

	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<vector2> uv0;
	vector<ushort> ZapIdx;
	for (it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		ZapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts0))
			continue;
		previewBaseNif->GetUvsForShape(it->second, uv0);
		if (PreviewType == SMALL_PREVIEW) {
			ApplySliders(it->first, sliderManager.SlidersSmall, verts0, ZapIdx, &uv0);
			// zap deleted verts before applying to the shape
			if (ZapIdx.size() > 0) {
				for (int z = ZapIdx.size() - 1; z >= 0; z--) {
					verts0.erase(verts0.begin() + ZapIdx[z]);
					uv0.erase(uv0.begin() + ZapIdx[z]);
				}
			}
			preview0->Update(it->second, &verts0, &uv0);
		} else {
			ApplySliders(it->first, sliderManager.SlidersBig, verts0, ZapIdx, &uv0);
			// zap deleted verts before applying to the shape
			if (ZapIdx.size() > 0) {
				//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0], it->first, ZapIdx);
				for (int z = ZapIdx.size() - 1; z >= 0; z--) {
					verts0.erase(verts0.begin() + ZapIdx[z]);
					uv0.erase(uv0.begin() + ZapIdx[z]);
				}
			}
			preview1->Update(it->second, &verts0, &uv0);
		}
	}
}

void  BodySlideApp::RebuildPreviewMeshes(char PreviewType) {	
	if (PreviewType == SMALL_PREVIEW) {
		if (preview0 == NULL) return;
	} else {
		if (preview1 == NULL) return;
	}

	if (previewBaseNif == NULL) 
		return;
	
	PreviewMod.CopyFrom((*previewBaseNif));
	
	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<ushort> ZapIdx;
	vector3 v;
	for (it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		v = activeSet.GetTargetVirtualOffset(it->first);
		if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f) {
			previewBaseNif->VirtualOffsetShape(it->second, v, false);
		}

		ZapIdx.clear();
		if (!previewBaseNif->GetVertsForShape(it->second, verts0))
			continue;		

		if (PreviewType == SMALL_PREVIEW) {
			ApplySliders(it->first, sliderManager.SlidersSmall, verts0, ZapIdx);
		} else {
			ApplySliders(it->first, sliderManager.SlidersBig, verts0, ZapIdx);
		}
		
		// Zap deleted verts before preview
		if (ZapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(it->second, verts0);
			//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0], it->first, ZapIdx);
			PreviewMod.DeleteVertsForShape(it->second, ZapIdx);
		} else {
			// No zapping needed -- just show all the verts.  
			PreviewMod.SetVertsForShape(it->second, verts0);
		}
	}

	if (PreviewType == SMALL_PREVIEW ) {
		preview0->RefreshMeshFromNif(&PreviewMod);
	} else {
		preview1->RefreshMeshFromNif(&PreviewMod);
	}
}

void BodySlideApp::SetDefaultConfig() {
	HKEY skyrimRegKey;
	char installPath[256];
	DWORD pathSize = 256;
	string buildpath;

	long result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Bethesda Softworks\\Skyrim", 0, KEY_READ, &skyrimRegKey);
	if (result == ERROR_SUCCESS) {
		result = RegQueryValueExA(skyrimRegKey, "Installed Path", 0, 0, (BYTE*)installPath, &pathSize);
		if (result == ERROR_SUCCESS) {
			buildpath = installPath;
			buildpath += "Data\\";
			Config.SetDefaultValue("GameDataPath", buildpath);
		} else
			wxMessageBox("Could not find your Skyrim install path (registry key value). Did you launch it through the official launcher once?", "Warning");
	} else
		wxMessageBox("Could not find your Skyrim installation (registry key). Did you launch it through the official launcher once?", "Warning");
	
	Config.SetDefaultValue("ShapeDataPath", ".\\ShapeData");
	Config.SetDefaultValue("WarnMissingGamePath", "true");
	Config.SetDefaultValue("SelectedPreset", "CBBE");	
	
	if (!Config.Exists("SelectedOutfit"))
		Config.SetValue("SelectedOutfit", "CBBE Body");
	if (!Config.Exists("Anim/DefaultSkeletonReference"))
		Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_female.nif");
	if (!Config.Exists("ReferenceTemplates"))
		Config.SetValue("ReferenceTemplates", "");
	if (!Config.Exists("Input/SliderMinimum"))
		Config.SetValue("Input/SliderMinimum", 0);
	if (!Config.Exists("Input/SliderMaximum"))
		Config.SetValue("Input/SliderMaximum", 100);
}

void BodySlideApp::LoadAllCategories() {
	cCollection.LoadCategories("SliderCategories");
}

void BodySlideApp::SetPresetGroups(const string& setName) {
	presetGroups.clear();
	gCollection.GetOutfitGroups(setName, presetGroups);

	if (presetGroups.empty()) {
		Config.GetValueArray("DefaultGroups", "GroupName", presetGroups);
	}
}

void BodySlideApp::LoadAllGroups() {
	gCollection.LoadGroups("SliderGroups");
	
	ungroupedOutfits.clear();
	for (auto o: outfitNameSource) {
		vector<string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty()) {
			ungroupedOutfits.push_back(o.first);
		}
	}

	vector<string> aliases;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "alias", aliases);
	vector<string> groups;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "group", groups);

	if (aliases.size() == groups.size()) {
		for (int i = 0; i < aliases.size(); i++) {
			groupAlias[aliases[i]] = groups[i];
		}
	}
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
		if (outfile.GetError() == 1) {
			outfile.New(fileName);
		}
		if (outfile.GetError() != 0) {
			wxMessageBox("Failed to create group file", "Error", wxICON_ERROR);
			return 5;
		}
	}

	outfile.GetGroupNames(existing);
	bool found = false;
	for (auto e: existing) {
		if (_stricmp(e.c_str(), groupName.c_str()) == 0) {
			found = true;
			break;
		}
	}
	if (found) {
		int ret = wxMessageBox("That group already exists in the specified file, do you wish to overwrite the group?", "Group exists", wxYES_NO | wxCANCEL);
		if (ret == wxNO) {
			return 2;
		} else if (ret == wxCANCEL) {
			return 3;
		}
	}
	SliderSetGroup ssg;
	ssg.SetName(groupName);
	ssg.AddMembers(filteredOutfits);

	outfile.UpdateGroup(ssg);
	outfile.Save();
	return 0;
}

void BodySlideApp::ApplyOutfitFilter() {
	bool showungrouped = false;
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
			showungrouped = true;
		} else {
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
	} else if (lastGrps.empty()) {
		gCollection.GetAllGroups(grouplist);
		showungrouped = true;
	}

	for (auto gn: grouplist) {
		if (gn == "Unassigned") {
			showungrouped = true;
		} else {
			gCollection.GetGroupMembers(gn, grpFiltOutfits);
		}
	}	

	if (showungrouped) {
		for (auto ug: ungroupedOutfits) {
			grpFiltOutfits.insert(ug);
		}
	}

	for (auto no: outfitNameOrder) {
		if (grpFiltOutfits.find(no) != grpFiltOutfits.end()) {
			workfiltList.push_back(no);
		}
	}


	if (outfitSrch.empty()) {
		for (auto w: workfiltList)
			filteredOutfits.push_back(w);
	} else {
		regex re;
		try {
			re.assign(outfitSrch, regex::icase);
			for (auto w: workfiltList)
				if (regex_search(w, re))
					filteredOutfits.push_back(w);
		} catch (regex_error) {		
			for (auto w: workfiltList) {
				filteredOutfits.push_back(w);
			}
		}
	}

	Config.SetValue("LastGroupFilter", string(grpSrch));
	Config.SetValue("LastOutfitFilter", string(outfitSrch));
}

int BodySlideApp::GetFilteredOutfits(vector<string>& outList) {
	outList.assign(filteredOutfits.begin(), filteredOutfits.end());
	return outList.size();
}

void BodySlideApp::LoadPresets(const string& sliderSet) {
	string outfit = sliderSet;
	if (sliderSet.empty())
		outfit = Config.GetCString("SelectedOutfit", "");

	vector<string> groups_and_aliases;

	for (auto g: presetGroups) {
		groups_and_aliases.push_back(g);
		for (auto ag : this->groupAlias) {
			if (ag.second == g) {
				groups_and_aliases.push_back(ag.first);
			}
		}
	}

	sliderManager.LoadPresets("SliderPresets", outfit, groups_and_aliases);
}

int BodySlideApp::BuildBodies(bool localPath, bool clean, bool tri) {
	string inputFileName = activeSet.GetInputFileName();
	NifFile nifSmall;
	NifFile nifBig;

	string outFileNameSmall;
	string outFileNameBig;

	if (localPath) {
		outFileNameSmall = outFileNameBig = activeSet.GetOutputFile();
	}
	else {
		if (!Config.Exists("GameDataPath")) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox("WARNING: Skyrim data path not configured. Would you like to show BodySlide where it is?", "Skyrim not found", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					return 4;
				}
			}
			char buffer[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, buffer);
			string response = wxDirSelector("Please choose a directory to set as your Skyrim Data directory.", buffer);
			if (response.empty())
				return 4;
			response += "\\";
			Config.SetValue("GameDataPath", response);
		}
		outFileNameSmall = Config["GameDataPath"] + activeSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;
		string tmp = Config["GameDataPath"] + activeSet.GetOutputPath();
		SHCreateDirectoryExA(NULL, tmp.c_str(), NULL);
	}

	// ALT key
	if (clean && !localPath) {
		int ret = wxMessageBox("WARNING: This will delete the output files from the output folder, potentially causing crashes.\n\nDo you want to continue?", "Clean Build", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES)
			return 4;
		FILE * file;
		string removeHigh, removeLow;
		string msg = "Successfully removed the following files:\n";
		bool genWeights = activeSet.GenWeights();

		if (genWeights)
			removeHigh = outFileNameSmall + "_1.nif";
		else
			removeHigh = outFileNameSmall + ".nif";
		msg += removeHigh + "\n";

		file = fopen(removeHigh.c_str(), "r+");
		if (file != NULL) {
			fclose(file);
			remove(removeHigh.c_str());
		}

		if (!genWeights) {
			wxMessageBox(msg, "Process Successful");
			return 0;
		}

		removeLow = outFileNameSmall + "_0.nif";
		msg += removeHigh;

		file = fopen(removeLow.c_str(), "r+");
		if (file != NULL) {
			fclose(file);
			remove(removeLow.c_str());
		}
		wxMessageBox(msg, "Process Successful");
		return 0;
	}

	if (nifSmall.Load(inputFileName) != 0)
		return 1;
	if (nifBig.Load(inputFileName) != 0)
		return 1;

	map<string, string>::iterator it;
	vector<vec3> verts0;
	vector<vec3> verts1;
	vector<ushort> ZapIdx;
	unordered_map<string, vector<ushort>> ZapIdxAll;

	for (it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
		if (!nifSmall.GetVertsForShape(it->second, verts0))
			continue;
		if (!nifBig.GetVertsForShape(it->second, verts1))
			continue;

		ZapIdxAll.emplace(it->second, vector<ushort>());

		ApplySliders(it->first, sliderManager.SlidersSmall, verts0, ZapIdx);
		ZapIdx.clear();
		ApplySliders(it->first, sliderManager.SlidersBig, verts1, ZapIdx);

		nifSmall.SetVertsForShape(it->second, verts0);
		nifSmall.DeleteVertsForShape(it->second, ZapIdx);

		nifBig.SetVertsForShape(it->second, verts1);
		nifBig.DeleteVertsForShape(it->second, ZapIdx);

		ZapIdxAll[it->second] = ZapIdx;
	}

	/* Add RaceMenu TRI path for in-game morphs */
	if (tri) {
		string triPath = activeSet.GetOutputFilePath() + ".tri";
		string triPathTrimmed = triPath;
		triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
		triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

		if (!WriteMorphTRI(outFileNameBig, activeSet, nifBig, ZapIdxAll)) {
			wxMessageBox(wxString().Format("Error creating TRI file in the following location\n\n%s", triPath),
				wxString().Format("Unable to process (error%d)", GetLastError()), wxOK | wxICON_ERROR);
		}

		string triShapeLink;
		for (it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it) {
			triShapeLink = it->second;
			if (tri && !triShapeLink.empty()) {
				nifSmall.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
				nifBig.AddStringExtraData(triShapeLink, "BODYTRI", triPathTrimmed);
				tri = false;
			}
		}
	}
	
	string saved1;
	string saved2;
	bool useCustName = false;
	string custName;
	if (activeSet.GenWeights()) {
		outFileNameSmall += "_0.nif";
		outFileNameBig += "_1.nif";
		custName =  outFileNameSmall;
		saved1 = custName;
		while (nifSmall.Save(custName)) {
			wxMessageBox(wxString().Format("Unable to process bodies/outfits in the following location\n\n%s", custName),
						 wxString().Format("Unable to process (error%d)", GetLastError()), wxOK | wxICON_ERROR);
			custName = wxSaveFileSelector("Choose alternate file name", "*.nif", custName.c_str());
			if(custName.empty()) 
				return 4;
			useCustName =true;
			saved1 = custName;
		}

		custName = custName.substr(0,custName.rfind("_0.nif")) + "_1.nif";
	} else {
		outFileNameBig += ".nif";
		custName = outFileNameBig;
	}

	if (!useCustName) {
		outFileNameBig = custName;
	}

	saved2 = custName;
	while (nifBig.Save(custName)) {
		wxMessageBox(wxString().Format("Unable to process bodies/outfits in the following location\n\n%s", custName),
					 wxString().Format("Unable to process (error%d)", GetLastError()), wxOK | wxICON_ERROR);
		custName = wxSaveFileSelector("Choose alternate file name", "*.nif", custName.c_str());
		if (custName.empty()) 
			return 4;
		useCustName = true;
		saved2 = custName;
	}

	string msg = "Successfully processed the following files:\n";
	if (!saved1.empty())
		msg += saved1 += "\n";	
	if (!saved2.empty())
		msg += saved2;

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

	if (clean) {
		int ret = wxMessageBox("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?", "Clean Batch Build", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES)
			return 1;
	}

	string activePreset = Config["SelectedPreset"];

	if (datapath.empty()) {
		if (!Config.Exists("GameDataPath")) {
			if (clean) {
				wxMessageBox("WARNING: Skyrim data path not configured. Files can't be removed that way.", "Skyrim not found", wxOK | wxICON_ERROR);
				return 1;
			}
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox("WARNING: Skyrim data path not configured. Continue saving files? \r\n(Files will be saved in the BodySlide\\meshes directory)", "Skyrim not found", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES)
					return 1;
			}
			char buffer[MAX_PATH];
			GetCurrentDirectoryA(MAX_PATH, buffer);
			Config.SetValue("GameDataPath", string(buffer) + "\\");
		}
		datapath = Config["GameDataPath"];
	}

	progWnd = new wxProgressDialog("Processing Outfits", "Starting...", 1000, NULL, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_ELAPSED_TIME);
	progWnd->SetSize(400, 150);
	stringstream progmsg;
	float progstep = 1000.0f / outfitList.size();
	int count = 1;

	/* loop */
	bool triEnd;
	for (auto outfit : outfitList) {
		triEnd = tri;
		progmsg.str("");
		progmsg << "Processing '" << outfit << "' ( " << count << " of " << outfitList.size() << " )";
		progWnd->Update((int)(count*progstep), progmsg.str());
		count++;
		/* load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			failedOutfits[outfit] = "No recorded outfit name source";
			continue;
		}

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			currentSet.Clear();
			if (!sliderDoc.GetSet(outfit, currentSet)) {
				currentSet.SetBaseDataPath(Config["ShapeDataPath"]);
				currentSet.LoadSetDiffData(currentDiffs);
			}
			else {
				failedOutfits[outfit] = "Unable to get slider set from file: " + outfitNameSource[outfit];
			}
		}
		else {
			failedOutfits[outfit] = "Unable to open slider set file: " + outfitNameSource[outfit];
			continue;
		}

		// ALT key
		if (clean && custPath == "") {
			FILE * file;
			string removeHigh, removeLow;
			string removePath = datapath + currentSet.GetOutputFilePath();
			bool genWeights = currentSet.GenWeights();

			if (genWeights)
				removeHigh = removePath + "_1.nif";
			else
				removeHigh = removePath + ".nif";

			file = fopen(removeHigh.c_str(), "r+");
			if (file != NULL) {
				fclose(file);
				remove(removeHigh.c_str());
			}

			if (!genWeights)
				continue;

			removeLow = removePath + "_0.nif";
			file = fopen(removeLow.c_str(), "r+");
			if (file != NULL) {
				fclose(file);
				remove(removeLow.c_str());
			}
			continue;
		}

		/* load iput nifs */
		if (nifSmall.Load(currentSet.GetInputFileName())) {
			failedOutfits[outfit] = "Unable to load small input nif: " + currentSet.GetInputFileName();
			continue;
		}
		if (nifBig.Load(currentSet.GetInputFileName())) {
			failedOutfits[outfit] = "Unable to load big input nif: " + currentSet.GetInputFileName();
			continue;
		}

		/* Shape the nif files */
		map<string, string>::iterator it;
		vector<vec3> verts0;
		vector<vec3> verts1;
		vector<ushort> ZapIdx;
		unordered_map<string, vector<ushort>> ZapIdxAll;

		for (it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
			if (!nifSmall.GetVertsForShape(it->second, verts0))
				continue;
			if (!nifBig.GetVertsForShape(it->second, verts1))
				continue;

			float vsmall;
			float vbig;
			vector<int> clamps;
			ZapIdx.clear();
			ZapIdxAll.emplace(it->second, vector<ushort>());

			for (int s = 0; s < currentSet.size(); s++) {
				string dn = currentSet[s].TargetDataName(it->first);
				string target = it->first;
				if (dn.empty())
					continue;

				if (currentSet[s].bClamp)  {
					clamps.push_back(s);
					continue;
				}

				vsmall = sliderManager.GetSmallPresetValue(activePreset, currentSet[s].Name, currentSet[s].defSmallValue / 100.0f);
				vbig = sliderManager.GetBigPresetValue(activePreset, currentSet[s].Name, currentSet[s].defBigValue / 100.0f);
				if (currentSet[s].bInvert) {
					vsmall = 1.0f - vsmall;
					vbig = 1.0f - vbig;
				}

				if (currentSet[s].bZap) {
					if (vbig > 0) {
						currentDiffs.GetDiffIndices(dn, target, ZapIdx);
						ZapIdxAll[it->second] = ZapIdx;
					}
					continue;
				}
				currentDiffs.ApplyDiff(dn, target, vsmall, &verts0);
				currentDiffs.ApplyDiff(dn, target, vbig, &verts1);
			}
			if (!clamps.empty()) {
				for (auto c : clamps) {
					string dn = currentSet[c].TargetDataName(it->first);
					string target = it->first;
					if (currentSet[c].defSmallValue > 0) {
						currentDiffs.ApplyClamp(dn, target, &verts0);
					}
					if (currentSet[c].defBigValue > 0) {
						currentDiffs.ApplyClamp(dn, target, &verts1);
					}
				}
			}

			nifSmall.SetVertsForShape(it->second, verts0);
			nifSmall.DeleteVertsForShape(it->second, ZapIdx);

			nifBig.SetVertsForShape(it->second, verts1);
			nifBig.DeleteVertsForShape(it->second, ZapIdx);
		}

		/* Create directory for the outfit */
		string dir = datapath + currentSet.GetOutputPath();
		stringstream errss;
		int error = SHCreateDirectoryExA(NULL, dir.c_str(), NULL);
		if ((error != ERROR_SUCCESS) && (error != ERROR_ALREADY_EXISTS) && (error != ERROR_FILE_EXISTS)) {
			errss << "Unable to create destination directory: " << dir << " [" << hex << (unsigned int)error << "]";
			failedOutfits[outfit] = errss.str();
			continue;
		}

		outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;

		/* Add RaceMenu TRI path for in-game morphs */
		if (triEnd) {
			string triPath = currentSet.GetOutputFilePath() + ".tri";
			string triPathTrimmed = triPath;
			triPathTrimmed = regex_replace(triPathTrimmed, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
			triPathTrimmed = regex_replace(triPathTrimmed, regex(".*meshes\\\\", regex_constants::icase), ""); // Remove everything before and including the meshes path

			if (!WriteMorphTRI(outFileNameBig, currentSet, nifBig, ZapIdxAll)) {
				wxMessageBox(wxString().Format("Error creating TRI file in the following location\n\n%s", triPath),
					wxString().Format("Unable to process (error%d)", GetLastError()), wxOK | wxICON_ERROR);
			}

			string triShapeLink;
			for (it = currentSet.TargetShapesBegin(); it != currentSet.TargetShapesEnd(); ++it) {
				triShapeLink = it->second;
				if (triEnd && !triShapeLink.empty()) {
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
			if ((error = nifBig.Save(outFileNameBig)) != 0) {
				errss << "Unable to save nif file: " << outFileNameBig << " [" << hex << (unsigned int)error << "]";
				failedOutfits[outfit] = errss.str();
				continue;
			}
			if (nifSmall.Save(outFileNameSmall)) {
				errss << "Unable to save nif file: " << outFileNameSmall << " [" << hex << (unsigned int)error << "]";
				failedOutfits[outfit] = errss.str();
				continue;
			}
		}
		else {
			outFileNameBig += ".nif";
			if (nifBig.Save(outFileNameBig)) {
				errss << "Unable to save nif file: " << outFileNameBig << " [" << hex << (unsigned int)error << "]";
				failedOutfits[outfit] = errss.str();
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
	string outfitName = Config.GetCString("SelectedOutfit", "CBBE Body");
	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

BodySlideFrame::BodySlideFrame(BodySlideApp* app, const wxString &title, const wxPoint &pos, const wxSize &size) 
: delayLoad(this, DELAYLOAD_TIMER)
//: wxFrame(NULL, wxID_ANY, title, pos, size)
{
	this->app = app;
	delayLoad.Start(100, true);
	rowCount = 0;
	wxXmlResource* rsrc = wxXmlResource::Get();
	bool loaded;
	rsrc->InitAllHandlers();
	loaded = rsrc->Load("res\\BodyslideFrame.xrc");
	if (!loaded) 
		return;
	rsrc->LoadFrame(this, GetParent(), "BodyslideFrame");

	this->SetIcon(wxIcon("res\\outfitstudio.png", wxBITMAP_TYPE_PNG));
	this->SetDoubleBuffered(true);

	batchBuildList = NULL;
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

	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	
	SetSize(size);
	sw->SetScrollRate(5, 26);
	sw->SetFocusIgnoringChildren();
	sw->Bind(wxEVT_ENTER_WINDOW, &BodySlideFrame::OnEnterSliderWindow, this);

	wxString val = Config.GetCString("LastGroupFilter", "");
	search->ChangeValue(val);
	val = Config.GetCString("LastOutfitFilter", "");
	outfitsearch->ChangeValue(val);
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
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	wxFlexGridSizer* sliderLayout = (wxFlexGridSizer*)sw->GetSizer();
	if (show) {
		XRCCTRL(*this,"lblLowWt", wxStaticText)->GetContainingSizer()->ShowItems(true);
		XRCCTRL(*this,"lblSingleWt", wxStaticText)->Show(false);
		XRCCTRL(*this,"btnPreviewLo", wxButton)->Show(true);
		sliderLayout->SetCols(6);
	} else {
		XRCCTRL(*this,"lblLowWt", wxStaticText)->GetContainingSizer()->ShowItems(false);
		XRCCTRL(*this,"lblSingleWt", wxStaticText)->Show(true);
		XRCCTRL(*this,"btnPreviewLo", wxButton)->Hide();
		sliderLayout->SetCols(3);
	}
}

void BodySlideFrame::AddCategorySliderUI(const wxString& name, bool show, bool oneSize) {
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	wxSizer* sliderLayout = sw->GetSizer();
	wxWindow* w;
	
	if (!oneSize) {
		sliderLayout->AddSpacer(0);

		w = new wxPanel(sw, -1, -1, -1, -1);
		w->SetBackgroundColour(wxColor(90, 90, 90));
		sliderLayout->Add(w, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}
	
	wxCheckBox* check = new wxCheckBox(sw, wxID_ANY, "");
	check->SetName(name);
	sliderLayout->Add(check, 0, wxRIGHT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 5);
	check->SetValue(show);
	check->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnCategoryCheckChanged, this);

	w = new wxStaticText(sw, wxID_ANY, name);
	w->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Andalus"));
	w->SetForegroundColour(wxColor(200, 200, 200));
	sliderLayout->Add(w, 0, wxLEFT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
	
	if (!oneSize) {
		w = new wxPanel(sw, -1, -1, -1, -1);
		w->SetBackgroundColour(wxColor(90, 90, 90));
		sliderLayout->Add(w, 0, wxTOP | wxBOTTOM | wxEXPAND, 10);
	}

	sliderLayout->AddSpacer(0);

	sw->FitInside();
}

void BodySlideFrame::AddSliderGUI(const wxString& name, bool isZap, bool oneSize) {
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	wxSizer* sliderLayout = sw->GetSizer();

	BodySlideFrame::SliderDisplay* sd = new BodySlideFrame::SliderDisplay;
	sd->isZap = isZap;
	sd->oneSize = oneSize;

	int minValue = Config.GetIntValue("Input/SliderMinimum", 0);
	int maxValue = Config.GetIntValue("Input/SliderMaximum", 100);
	
	if (!oneSize) {
		sd->lblSliderLo = new wxStaticText(sw, wxID_ANY, name, wxDefaultPosition, wxSize(-1, 22), wxALIGN_CENTER_HORIZONTAL);
		sd->lblSliderLo->SetBackgroundColour(wxColor(0x40, 0x40, 0x40));
		sd->lblSliderLo->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
		sliderLayout->Add(sd->lblSliderLo, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT, 5);

		if (isZap) {
			sd->zapCheckLo = new wxCheckBox(sw, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZLO");
			sd->zapCheckLo->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
			sliderLayout->AddStretchSpacer();
			sliderLayout->Add(sd->zapCheckLo, 0, wxALIGN_LEFT, 0);
		} else {
			sd->sliderLo = new wxSlider(sw, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 22), wxSL_AUTOTICKS | wxSL_BOTTOM | wxSL_HORIZONTAL);
			sd->sliderLo->SetMaxSize(wxSize(-1, 22));
			sd->sliderLo->SetTickFreq(5, 0);
			sd->sliderLo->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
			sd->sliderLo->SetName(name + "|LO");
			sliderLayout->Add(sd->sliderLo, 1, wxALIGN_LEFT | wxEXPAND, 0);
			
			sd->sliderReadoutLo = new wxTextCtrl(sw, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, 18), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
			sd->sliderReadoutLo->SetName(name + "|RLO");
			sd->sliderReadoutLo->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), NULL, this);
			sliderLayout->Add(sd->sliderReadoutLo, 0, wxALL, 0);
		}
	}

	sd->lblSliderHi = new wxStaticText(sw, wxID_ANY, name, wxDefaultPosition, wxSize(-1, 22), wxALIGN_CENTER_HORIZONTAL);
	sd->lblSliderHi->SetBackgroundColour(wxColor(0x40, 0x40, 0x40));
	sd->lblSliderHi->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
	sliderLayout->Add(sd->lblSliderHi, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT, 5);

	if (isZap) {
		sd->zapCheckHi = new wxCheckBox(sw, wxID_ANY, " ", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|ZHI");
		sd->zapCheckHi->Bind(wxEVT_CHECKBOX, &BodySlideFrame::OnZapCheckChanged, this);
		sliderLayout->AddStretchSpacer();
		sliderLayout->Add(sd->zapCheckHi, 0, wxALIGN_LEFT, 0);	
	} else {
		sd->sliderHi = new wxSlider(sw, wxID_ANY, 0, minValue, maxValue, wxDefaultPosition, wxSize(-1, 22), wxSL_AUTOTICKS | wxSL_HORIZONTAL);
		sd->sliderHi->SetMaxSize(wxSize(-1, 22));
		sd->sliderHi->SetTickFreq(5, 0);
		sd->sliderHi->Bind(wxEVT_ERASE_BACKGROUND, &BodySlideFrame::OnEraseBackground, this);
		sd->sliderHi->SetName(name + "|HI");	
		sliderLayout->Add(sd->sliderHi, 1, wxALIGN_LEFT | wxEXPAND, 0);

		sd->sliderReadoutHi = new wxTextCtrl(sw, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, 18), wxTE_CENTRE | wxNO_BORDER | wxTE_PROCESS_ENTER);
		sd->sliderReadoutHi->SetName(name + "|RHI");
		sd->sliderReadoutHi->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(BodySlideFrame::OnSliderReadoutChange), NULL, this);	
		sliderLayout->Add(sd->sliderReadoutHi, 0, wxRIGHT, 10);
	}

	sd->sliderName = name;
	sliderDisplays[sd->sliderName] = sd;

	sw->FitInside();
	rowCount++;
}

void BodySlideFrame::ClearPresetList() {
	wxChoice* c = (wxChoice*)FindWindowByName("presetChoice", this);
	if (c) c->Clear();
}

void  BodySlideFrame::ClearOutfitList() {
	wxChoice* c = (wxChoice*)FindWindowByName("outfitChoice", this);
	if (c) c->Clear();
}

void BodySlideFrame::ClearSliderGUI() {
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	if (sw) {
		sw->GetSizer()->Clear();
		sw->DestroyChildren(); 
	}
	for (auto sd: sliderDisplays) {
		delete sd.second;
	}
	sliderDisplays.clear();

	rowCount = 0;
}

void BodySlideFrame::PopulateOutfitList(const wxArrayString& items, const wxString& selectItem) {
	wxChoice* c = (wxChoice*)FindWindowByName("outfitChoice", this);
	if (!c) return;

	c->Clear();
	c->Append(items);
	if (!c->SetStringSelection(selectItem)) {
		//c->SetValue("BWWW");
		
		int i;
		if (selectItem.First('['))
			i = c->Append("[" + selectItem + "]");
		else
			i = c->Append(selectItem);

		c->SetSelection(i);
	}
	//c->Select(c->FindString(selectItem));
}

void BodySlideFrame::PopulatePresetList(const wxArrayString& items, const wxString& selectItem) {
	wxChoice* c = (wxChoice*)FindWindowByName("presetChoice", this);
	if (!c) return;
	c->Clear();
	c->Append(items);
	c->Select(c->FindString(selectItem));
}

void BodySlideFrame::SetSliderPosition(const wxString &name, float newValue, short HiLo) {
	wxString fullname = name;
	int intval = (int)(newValue * 100.0f);

	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(string(name));
	if (!sd) 
		return;

	if (HiLo == SLIDER_HI) {
		if(!sd->isZap) {
			sd->sliderReadoutHi->SetValue(wxString::Format("%d%%", intval));
			sd->sliderHi->SetValue(intval);
		} else {
			sd->zapCheckHi->SetValue((intval>0));
		}

	} else if (!sd->oneSize){
		if (!sd->isZap) {
			sd->sliderReadoutLo->SetValue(wxString::Format("%d%%", intval));
			sd->sliderLo->SetValue(intval);
		} else {
			sd->zapCheckLo->SetValue((intval>0));
		}
	}

	/*
	if(HiLo == SLIDER_HI) {
		fullname += "|HI";
	} else 
		fullname += "|LO";

	wxSlider* theSlider = (wxSlider*)FindWindowByName(fullname,this);
	if(theSlider == NULL) 
		return;

	theSlider->SetValue(intval);
	wxTextCtrl* readout = (wxTextCtrl*)FindWindowById(theSlider->GetId()+1);
	if(readout) {
		readout->SetValue(wxString::Format("%d%%",intval));
	}
	*/
}

void BodySlideFrame::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close(true);
}

void BodySlideFrame::OnActivateFrame(wxActivateEvent& event) {
	event.Skip();
	if (event.GetActive()) {
		wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
		sw->SetFocusIgnoringChildren();
	}
}

void BodySlideFrame::OnIconizeFrame(wxIconizeEvent& event) {
	event.Skip();
	if (!event.IsIconized()) {
		wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
		lastScroll = sw->GetScrollPos(wxVERTICAL);
		CallAfter(&BodySlideFrame::PostIconizeFrame);
	}
}

void BodySlideFrame::PostIconizeFrame() {
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	sw->SetFocusIgnoringChildren();
	sw->Scroll(0, lastScroll);
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
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(string(name));
	if (!sd) 
		return;	

	app->SetSliderValue(name, isLo, ((float)event.GetPosition() / 100.0f));	
	app->SetSliderChanged(name, isLo);
	if (isLo) {
		sd->sliderReadoutLo->SetValue(wxString::Format("%d%%", event.GetPosition()));
		app->UpdatePreview(SMALL_PREVIEW);
	} else {
		sd->sliderReadoutHi->SetValue(wxString::Format("%d%%", event.GetPosition()));
		app->UpdatePreview(BIG_PREVIEW);
	}

	/*
	if (w) {
		wxWindowID id = w->GetId();
		((wxTextCtrl*)FindWindowById(id + 1, this))->SetValue(wxString::Format("%d%%", event.GetPosition()));
		//fullname = slider->GetName();
		bool isLo = fullname.EndsWith("|LO", &name);
		if(!isLo) 
			name = fullname.BeforeLast('|');
		app->SetSliderValue(name, isLo, ((float)event.GetPosition() / 100.0f));
	}
	*/
}

void BodySlideFrame::OnSliderReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject() ;
	if (!w)
		return;
		
	wxString fullname = w->GetName();
	wxString name;
	bool isLo = fullname.EndsWith("|RLO", &name);
	if (!isLo) {
		if(!fullname.EndsWith("|RHI", &name)) 
			return;
	}
	BodySlideFrame::SliderDisplay* sd = GetSliderDisplay(string(name));
	if (!sd) 
		return;	

	double v;
 	w->GetValue().ToDouble(&v);
	w->ChangeValue(wxString::Format("%0.0f%%", v));

	app->SetSliderValue(name, isLo, (float)v / 100.0f);
	
	if (isLo) {
		sd->sliderLo->SetValue(v);
		app->UpdatePreview(SMALL_PREVIEW);

	} else {
		sd->sliderHi->SetValue(v);
		app->UpdatePreview(BIG_PREVIEW);
	}

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
	if (!w) return;
	
	wxScrolledWindow* sw = (wxScrolledWindow*)FindWindowByName("SliderScrollWindow", this);
	wxCheckBox* cb = (wxCheckBox*)event.GetEventObject();
	if (event.IsChecked()) {
		app->cCollection.SetCategoryHidden(string(cb->GetName().mb_str()), false);
	} else {
		app->cCollection.SetCategoryHidden(string(cb->GetName().mb_str()), true);
	}

	Freeze();

	int scrollPos = sw->GetScrollPos(wxOrientation::wxVERTICAL);
	ClearSliderGUI();
	app->DisplayActiveSet();
	app->RefreshSliders();
	sw->Scroll(0, scrollPos);

	Thaw();
}

void BodySlideFrame::OnZapCheckChanged(wxCommandEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w) return;

	wxString sn;
	bool isLo = true;
	if (!w->GetName().EndsWith("|ZLO", &sn)) {
		isLo=false;
		if (!w->GetName().EndsWith("|ZHI", &sn)) {
			event.Skip();
			return;
		}
	}

	string sliderName = sn.ToAscii().data();
	if (event.IsChecked()) {
		if (sliderDisplays[sliderName]->oneSize) {
			app->SetSliderValue(sliderName, false, 1.0f);
		} else {
			app->SetSliderValue(sliderName, true, 1.0f);
			app->SetSliderValue(sliderName, false, 1.0f);
			if (isLo) {
				sliderDisplays[sliderName]->zapCheckHi->SetValue(true);
			} else {
				sliderDisplays[sliderName]->zapCheckLo->SetValue(true);
			}
		}

	} else {
		if (sliderDisplays[sliderName]->oneSize) {
			app->SetSliderValue(sliderName, false, 0.0f);
		} else {
			app->SetSliderValue(sliderName, true, 0.0f);
			app->SetSliderValue(sliderName, false, 0.0f);
			if (isLo) {
				sliderDisplays[sliderName]->zapCheckHi->SetValue(false);
			} else {
				sliderDisplays[sliderName]->zapCheckLo->SetValue(false);
			}
		}
	}

	wxBeginBusyCursor();
	app->RebuildPreviewMeshes(SMALL_PREVIEW);
	app->RebuildPreviewMeshes(BIG_PREVIEW);
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
		curGroupNames.insert(string(token));
	}

	wxArrayString grpChoices;
	wxArrayInt grpSelections;
	int idx = 0;
	for (auto g: groupNames) {
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

	if (app->SaveSliderPositions(fname, presetName, groups) ) {
		wxMessageBox("Failed to save preset!", "Error");
	}
	
	app->LoadPresets("");
	app->PopulatePresetList(presetName);
}

void BodySlideFrame::OnPreviewHi(wxCommandEvent& WXUNUSED(event)) {
	app->ShowPreview(BIG_PREVIEW);
}

void BodySlideFrame::OnPreviewLo(wxCommandEvent& WXUNUSED(event)) {
	app->ShowPreview(SMALL_PREVIEW);
}

void BodySlideFrame::OnBuildBodies(wxCommandEvent& WXUNUSED(event)) {
	wxCheckBox* cbRaceMenu = (wxCheckBox*)FindWindowByName("cbRaceMenu");
	bool tri = false;
	if (cbRaceMenu)
		tri = cbRaceMenu->IsChecked();

	if (GetKeyState(VK_CONTROL) & 0x8000)
		app->BuildBodies(true, false, tri);
	else if (GetKeyState(VK_MENU) & 0x8000)
		app->BuildBodies(false, true, tri);
	else
		app->BuildBodies(false, false, tri);
}

void BodySlideFrame::OnBatchBuild(wxCommandEvent& WXUNUSED(event)) {
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

	if (GetKeyState(VK_CONTROL) & 0x8000)
		custpath = true;
	else if (GetKeyState(VK_MENU) & 0x8000) // Alt
		clean = true;

	app->GetFilteredOutfits(outfitChoices);

	int idx = 0;
	for (auto o: outfitChoices) {
		oChoices.Add(o);
		oSelections.Add(idx);
		idx++;
	}

	wxString filter;
	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* batchBuildChooser = rsrc->LoadDialog(this, "dlgBatchBuild");
	batchBuildChooser->SetSize(wxSize(650, 300));
	batchBuildChooser->SetSizeHints(wxSize(650, 300), wxSize(650, -1));
	batchBuildChooser->CenterOnParent();

	batchBuildList = XRCCTRL((*batchBuildChooser), "batchBuildList", wxCheckListBox);
	batchBuildList->Bind(wxEVT_RIGHT_UP, &BodySlideFrame::OnBatchBuildContext, this);
	batchBuildList->Append(oChoices);
	for (int i = 0; i < oChoices.size(); i++) {
		batchBuildList->Check(i);
	}

	if (batchBuildChooser->ShowModal() == wxID_OK) {
		wxArrayInt sel;
		batchBuildList->GetCheckedItems(sel);
		toBuild.clear();
		for (int i = 0; i < sel.size(); i++) {
			toBuild.push_back(outfitChoices[sel[i]]);
		}
	} else {
		return;
	}
	
	map<string, string> failedOutfits;
	int ret;
	if (custpath) {
		string path = wxDirSelector("Choose a folder to contain the saved files");
		if (path.empty()) 
			return;
		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, path + "\\");
	} else if (clean) {
		ret = app->BuildListBodies(toBuild, failedOutfits, true, tri);
	} else 
		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri);

	if (ret == 0) {
		wxMessageBox("All outfits processed successfully!", "Complete", wxICON_INFORMATION);
	} else if (ret == 3) {
		wxArrayString errlist;
		for (auto e: failedOutfits)
			errlist.Add(e.first + ":" + e.second);

		wxSingleChoiceDialog errdisplay(this, "The following outfits failed", "Failed", errlist, (void**)NULL, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
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
	} else {
		for (int i = 0; i < batchBuildList->GetCount(); i++)
			batchBuildList->Check(i);
	}
}

void BodySlideFrame::OnOutfitStudio(wxCommandEvent& WXUNUSED(event)) {
	app->LaunchOutfitStudio();
}

void BodySlideFrame::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = this->GetPosition();
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

long BodySlideFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) {
	switch (nMsg) {
	case MSG_PREVIEWCLOSING:
		app->ClosePreview(SMALL_PREVIEW);
		return 0;
		break;
	case MSG_BIGPREVIEWCLOSING:
		app->ClosePreview(BIG_PREVIEW);
		return 0;
		break;
	default:
		break;
	}
	return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}