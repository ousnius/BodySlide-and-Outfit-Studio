#include "OutfitStudio.h"
#include "BodySlideApp.h"
#include <functional>

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(OutfitStudio, wxFrame)
	EVT_MENU(XRCID("fileExit"), OutfitStudio::OnExit)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudio::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudio::OnLoadProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudio::OnLoadReference)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudio::OnLoadOutfit)
	EVT_MENU(XRCID("fileSave"), OutfitStudio::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudio::OnSaveSliderSetAs)

	//EVT_ERASE_BACKGROUND(OutfitStudio::OnEraseBackground)
	
	EVT_TEXT_ENTER(wxID_ANY, OutfitStudio::OnReadoutChange)
	EVT_COMMAND_SCROLL(wxID_ANY, OutfitStudio::OnSlider)
	EVT_BUTTON(wxID_ANY, OutfitStudio::OnClickSliderButton)
	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudio::OnSelectSliders)
	EVT_CHECKBOX(wxID_ANY, OutfitStudio::OnCheckBox)	

	EVT_MENU(XRCID("saveBaseShape"), OutfitStudio::OnSaveBaseOutfit)
	EVT_MENU(XRCID("exportOutfitNif"), OutfitStudio::OnExportCurrentShapeNif)
	EVT_MENU(XRCID("makeConvRef"), OutfitStudio::OnMakeConvRef)
	
	EVT_MENU(XRCID("sliderLoadPreset"), OutfitStudio::OnLoadPreset)
	EVT_MENU(XRCID("sliderConform"), OutfitStudio::OnSliderConform)
	EVT_MENU(XRCID("sliderConformAll"), OutfitStudio::OnSliderConformAll)
	EVT_MENU(XRCID("importSliderBSD"), OutfitStudio::OnSliderImportBSD)
	EVT_MENU(XRCID("importSliderOBJ"), OutfitStudio::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudio::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderNew"), OutfitStudio::OnNewSlider)
	EVT_MENU(XRCID("sliderNewZap"), OutfitStudio::OnNewZapSlider)
	EVT_MENU(XRCID("sliderNewCombined"), OutfitStudio::OnNewCombinedSlider)
	EVT_MENU(XRCID("sliderNegate"), OutfitStudio::OnSliderNegate)
	EVT_MENU(XRCID("sliderClear"), OutfitStudio::OnClearSlider)
	EVT_MENU(XRCID("sliderDelete"), OutfitStudio::OnDeleteSlider)
	EVT_MENU(XRCID("sliderProperties"), OutfitStudio::OnSliderProperties)
	
	EVT_MENU(XRCID("btnEditMode"), OutfitStudio::OnEditMode)
	EVT_MENU(XRCID("btnXMirror"), OutfitStudio::OnXMirror)
	EVT_MENU(XRCID("btnConnected"), OutfitStudio::OnConnectedOnly)
	
	EVT_MENU(XRCID("btnSelect"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnMaskBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnInflateBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnDeflateBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnMoveBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnSmoothBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudio::OnSelectBrush)
	
	EVT_MENU(XRCID("brushSettings"), OutfitStudio::OnBrushSettings)
	EVT_MENU(XRCID("btnIncreaseSize"), OutfitStudio::OnIncBrush)
	EVT_MENU(XRCID("btnDecreaseSize"), OutfitStudio::OnDecBrush)
	EVT_MENU(XRCID("btnIncreaseStr"), OutfitStudio::OnIncStr)
	EVT_MENU(XRCID("btnDecreaseStr"), OutfitStudio::OnDecStr)
	EVT_MENU(XRCID("btnClearMask"), OutfitStudio::OnClearMask)
	EVT_MENU(XRCID("btnInvertMask"), OutfitStudio::OnInvertMask)
	EVT_MENU(XRCID("btnShowMask"), OutfitStudio::OnShowMask)

	EVT_MENU(XRCID("btnGhostMode"), OutfitStudio::OnGhostMesh)
	EVT_MENU(XRCID("btnRecalcNormals"), OutfitStudio::OnRecalcNormals)
	EVT_MENU(XRCID("btnAutoNormals"), OutfitStudio::OnAutoNormals)
	EVT_MENU(XRCID("btnSmoothSeams"), OutfitStudio::OnSmoothNormalSeams)
	EVT_MENU(XRCID("btnSmoothThresh"), OutfitStudio::OnSetSmoothThreshold)

	EVT_MENU(XRCID("btnShowWireframe"), OutfitStudio::OnShowWireframe)
	EVT_MENU(XRCID("btnEnableLighting"), OutfitStudio::OnEnableLighting)
	EVT_MENU(XRCID("btnEnableTextures"), OutfitStudio::OnEnableTextures)

	EVT_MENU(XRCID("importShape"), OutfitStudio::OnImportShape)
	EVT_MENU(XRCID("exportShape"), OutfitStudio::OnExportShape)
	EVT_MENU(XRCID("moveShape"), OutfitStudio::OnMoveShape)
	EVT_MENU(XRCID("offsetShape"), OutfitStudio::OnOffsetShape)
	EVT_MENU(XRCID("scaleShape"), OutfitStudio::OnScaleShape)
	EVT_MENU(XRCID("virtscaleShape"), OutfitStudio::OnVirtScaleShape)
	EVT_MENU(XRCID("rotateShape"), OutfitStudio::OnRotateShape)
	EVT_MENU(XRCID("renameShape"), OutfitStudio::OnRenameShape)
	EVT_MENU(XRCID("setShapeTex"), OutfitStudio::OnSetShapeTexture)
	EVT_MENU(XRCID("copyShape"), OutfitStudio::OnDupeShape)
	EVT_MENU(XRCID("deleteShape"), OutfitStudio::OnDeleteShape)
	EVT_MENU(XRCID("addBone"), OutfitStudio::OnAddBone)
	EVT_MENU(XRCID("deleteBone"), OutfitStudio::OnDeleteBone)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudio::OnCopyBoneWeight)	
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudio::OnCopySelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudio::OnMaskWeighted)
	EVT_MENU(XRCID("buildSkinPartitions"), OutfitStudio::OnBuildSkinPartitions)

	EVT_MENU(XRCID("editUndo"), OutfitStudio::OnUndo)
	EVT_MENU(XRCID("editRedo"), OutfitStudio::OnRedo)

	EVT_TREE_STATE_IMAGE_CLICK(wxID_ANY, OutfitStudio::OnOutfitVisToggle)
	EVT_TREE_SEL_CHANGING(XRCID("outfitShapes"), OutfitStudio::OnCheckTreeSel)
	EVT_TREE_SEL_CHANGED(XRCID("outfitShapes"), OutfitStudio::OnOutfitShapeSelect)
	EVT_TREE_SEL_CHANGED(XRCID("outfitBones"), OutfitStudio::OnOutfitBoneSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitShapes"), OutfitStudio::OnOutfitShapeContext)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudio::OnBoneContext)
	EVT_COMMAND_KILL_FOCUS(XRCID("outfitShapes"), OutfitStudio::OnKillFocusOutfitShapes)
	
	EVT_BUTTON(XRCID("meshTabButton"), OutfitStudio::OnMeshBoneButtonClick)
	EVT_BUTTON(XRCID("boneTabButton"), OutfitStudio::OnMeshBoneButtonClick)
	
	EVT_MOVE_END(OutfitStudio::OnMoveWindow)
	EVT_SIZE(OutfitStudio::OnSetSize)

END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// OutfitStudio frame
// ----------------------------------------------------------------------------

// frame constructor
OutfitStudio::OutfitStudio(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size, ConfigurationManager& inConfig) : appConfig(inConfig) {
	progressVal = 0;
	wxXmlResource* rsrc = wxXmlResource::Get();

	SetParent(parent);
	bool loaded;
	rsrc->InitAllHandlers();
	loaded = rsrc->Load("res\\outfitStudio.xrc");
	if (!loaded) 
		return;
	rsrc->LoadFrame(this, GetParent(), "outfitStudio");
	
	this->SetIcon(wxIcon("res\\outfitstudio.png", wxBITMAP_TYPE_PNG));

	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	toolBar = (wxToolBar*)FindWindowByName("toolbar");
	wxMenuBar* menu = this->GetMenuBar();

	//menu->GetMenu(
	int statusWidths[] = { -1, 275, 100 };
	if (statusBar) {
		statusBar->SetFieldsCount(3);
		statusBar->SetStatusWidths(3, statusWidths);
		statusBar->SetStatusText("Ready!");
	}

	if (toolBar) {
		toolBar->ToggleTool(XRCID("btnSelect"), true);
		toolBar->SetToolDisabledBitmap(XRCID("btnSelect"), wxBitmap("res\\SelectBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnMaskBrush"), wxBitmap("res\\MaskBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnInflateBrush"), wxBitmap("res\\InflateBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnDeflateBrush"), wxBitmap("res\\DeflateBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnMoveBrush"), wxBitmap("res\\MoveBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnSmoothBrush"), wxBitmap("res\\SmoothBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnWeightBrush"), wxBitmap("res\\WeightBrush_d.png", wxBITMAP_TYPE_PNG));
	}
	if (menu) {
		menu->Enable(XRCID("btnWeightBrush"), false);
		//menu->Check(XRCID("btnInflateBrush"), true);
	}


	visStateImages = new wxImageList(16, 16, false, 2);
	wxBitmap visImg("res\\icoVisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap invImg("res\\icoInvisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap wfImg("res\\icoWireframe.png", wxBITMAP_TYPE_PNG);
	//visStateImages->Add(visImg);
	visStateImages->Add(visImg);
	visStateImages->Add(invImg);
	visStateImages->Add(wfImg);

	wxWindow* p = FindWindowByName("leftSplitPanel");
	wxWindow* r = FindWindowByName("rightSplitPanel");
	r->SetDoubleBuffered(true);
	r->SetBackgroundColour(wxColor(112, 112, 112));

	wxStateButton* meshCheck = (wxStateButton*)FindWindowByName("meshTabButton");
	meshCheck->SetCheck();
	//butt->Create(this, 8080, "butt");
	//(wxStateButton*) FindWindowByName("meshTabButton");


	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	outfitShapes->AssignStateImageList(visStateImages);
	shapesRoot = outfitShapes->AddRoot("Meshes");

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	bonesRoot = outfitBones->AddRoot("stuff");
/*
	wxAcceleratorEntry accelEntries[6];
	accelEntries[0].Set(wxACCEL_NORMAL,'0',XRCID("btnSelect"));
	accelEntries[1].Set(wxACCEL_NORMAL,'1',XRCID("btnMaskBrush"));
	accelEntries[2].Set(wxACCEL_NORMAL,'2',XRCID("btnInflateBrush"));	
	accelEntries[3].Set(wxACCEL_NORMAL,'3',XRCID("btnDeflateBrush"));
	accelEntries[4].Set(wxACCEL_NORMAL,'4',XRCID("btnMoveBrush"));
	accelEntries[5].Set(wxACCEL_NORMAL,'5',XRCID("btnSmoothBrush"));
	wxAcceleratorTable accel (6, accelEntries);
	SetAcceleratorTable(accel);

	*/
	glView = new wxGLPanel(p, wxDefaultSize);
	glView->SetNotifyWindow(this);
	
	rsrc->AttachUnknownControl("mGLView", glView, this);

	Proj = new OutfitProject(appConfig, this); // create empty project
	CreateSetSliders();
	
	bEditSlider = false;
	activeItem = NULL;
	previewMove = vec3(0.0f, 0.0f, 0.0f);
	previewScale = 1.0f;

	this->SetSize(size);
	this->SetPosition(pos);

	p->Layout();
}

OutfitStudio::~OutfitStudio() {
	if (Proj)
		delete Proj;
	Proj = NULL;
	for (auto d: sliderDisplays) {
		delete(d.second);
	}
	sliderDisplays.clear();
	((BodySlideApp*)wxApp::GetInstance())->CloseOutfitStudio();
}

void OutfitStudio::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = this->GetPosition();
	Config.SetValue("OutfitStudioFrame.x", p.x);
	Config.SetValue("OutfitStudioFrame.y", p.y);
	event.Skip();
}

void OutfitStudio::OnSetSize(wxSizeEvent& event) {
	wxSize p = event.GetSize();
	Config.SetValue("OutfitStudioFrame.width", p.x);
	Config.SetValue("OutfitStudioFrame.height", p.y);
	event.Skip();
}

void OutfitStudio::CreateSetSliders() {	
	int inc = 1;
	if (Proj->SliderCount() != 0) {
		inc = 90 / (Proj->SliderCount());
		StartProgress("Creating Set Sliders");
	}

	UpdateProgress(0, "Clearing old Sliders");

	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	sliderScroll->Freeze();
	sliderScroll->DestroyChildren();
	for (auto sd: sliderDisplays) {
		delete sd.second;
	}
	sliderDisplays.clear();

	wxSizer* rootSz = sliderScroll->GetSizer();

	for (int i = 0; i < Proj->SliderCount(); i++)  {
		UpdateProgress(inc, "Slider " + Proj->SliderName(i));
		if (Proj->SliderClamp(i))    // clamp sliders are a special case, usually an incorrect scale
			continue;
		createSliderGUI(Proj->SliderName(i), i, sliderScroll, rootSz);
	}

	sliderScroll->Thaw();
	sliderScroll->FitInside();

	EndProgress();
}

void OutfitStudio::createSliderGUI(const string& name, int id, wxScrolledWindow* wnd, wxSizer* rootSz) {
	SliderDisplay* d = new SliderDisplay();
	d->sliderPane = new wxPanel(wnd, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxTAB_TRAVERSAL);
	//sliderPane->SetBackgroundColour(wxColour(125, 77, 138));
	d->sliderPane->SetBackgroundColour(wxNullColour);
	d->sliderPane->SetMinSize(wxSize(-1, 25));
	d->sliderPane->SetMaxSize(wxSize(-1, 25));
	
	//wxBoxSizer* paneSz;
	d->paneSz = new wxBoxSizer(wxHORIZONTAL);
	
	//wxBitmapButton* btnSliderEdit;
	d->btnSliderEditID = 900 + id;
	d->btnSliderEdit = new wxBitmapButton(d->sliderPane, 900 + id, wxBitmap(wxT("res\\EditSmall.png"), wxBITMAP_TYPE_ANY), wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, name + "|btn");
	
	d->btnSliderEdit->SetBitmapDisabled(wxBitmap(wxT("res\\EditSmall_d.png"), wxBITMAP_TYPE_ANY));
	d->btnSliderEdit->SetToolTip(wxT("Turn on edit mode for this slider."));
	
	d->paneSz->Add(d->btnSliderEdit, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
	
	d->sliderNameCheckID = 1000 + id;
	d->sliderNameCheck = new wxCheckBox(d->sliderPane, 1000 + id, "", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|check");
	d->sliderNameCheck->SetForegroundColour(wxColour(255, 255, 255));
	d->paneSz->Add(d->sliderNameCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//d->sliderNameID = 1050+id;
	d->sliderName = new wxStaticText(d->sliderPane, wxID_ANY, name.c_str(), wxDefaultPosition, wxDefaultSize, 0, name + "|lbl");
	d->sliderName->SetForegroundColour(wxColour(255, 255, 255));
	
	d->paneSz->Add(d->sliderName, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
	
	d->sliderID = 2000 + id;
	d->slider = new wxSlider(d->sliderPane, 2000 + id, 0, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL, wxDefaultValidator, name + "|slider");
	d->slider->SetMinSize(wxSize(-1, 20));
	d->slider->SetMaxSize(wxSize(-1, 20));

	d->paneSz->Add(d->slider, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxEXPAND, 5);
	
	d->sliderReadoutID = 1200;
	d->sliderReadout = new wxTextCtrl(d->sliderPane, 1200 + id, wxT("0%"), wxDefaultPosition, wxSize(40, -1), wxWANTS_CHARS | wxTE_RIGHT | wxSIMPLE_BORDER, wxDefaultValidator, name + "|readout");
	d->sliderReadout->SetMaxLength(0); 
	d->sliderReadout->SetForegroundColour(wxColour(255, 255, 255));
	d->sliderReadout->SetBackgroundColour(wxColour(48, 48, 48));
	d->sliderReadout->SetMinSize(wxSize(40, 20));
	d->sliderReadout->SetMaxSize(wxSize(40, 20));
	d->sliderReadout->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(OutfitStudio::OnReadoutChange), NULL, this);
	
	d->paneSz->Add(d->sliderReadout, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);	
	
	d->sliderPane->SetSizer(d->paneSz);
	d->sliderPane->Layout();
	d->paneSz->Fit(d->sliderPane);
	rootSz->Add(d->sliderPane, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	d->hilite = false;

	ShowSliderEffect(id);
	sliderDisplays[name] = d;
}

void OutfitStudio::SetSliderValue(int index, int val) {
	string name = Proj->SliderName(index);
	Proj->SliderValue(index) = val / 100.0f;	
	sliderDisplays[name]->sliderReadout->SetValue(wxString::Format("%d%%", val));	
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::SetSliderValue(const string& name, int val) {
	Proj->SliderValue(name) = val / 100.0f;	
	sliderDisplays[name]->sliderReadout->SetValue(wxString::Format("%d%%", val));	
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::ApplySliders(bool recalcBVH) {
	vector<vector3> refVerts;
	vector<vector3> outfitVerts;
		
	vector<string> outfitshapes;
	vector<string> refshapes;

	Proj->RefShapes(refshapes);
	Proj->OutfitShapes(outfitshapes);

	for (auto shape: refshapes) {
		Proj->GetLiveRefVerts(shape, refVerts);
		glView->UpdateMeshVertices(shape, &refVerts, recalcBVH);
	}

	for (auto shape: outfitshapes) {
		Proj->GetLiveOutfitVerts(shape, outfitVerts);
		glView->UpdateMeshVertices(shape, &outfitVerts, recalcBVH);
	}
}

void OutfitStudio::ShowSliderEffect(int sliderID, bool show) {
	if (Proj->ValidSlider(sliderID)) {
		Proj->SliderShow(sliderID) = show;
		if (show)
			((wxCheckBox*)FindWindowById(1000 + sliderID))->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else 			
			((wxCheckBox*)FindWindowById(1000 + sliderID))->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudio::ShowSliderEffect(const string& sliderName, bool show) {
	if (Proj->ValidSlider(sliderName)) {
		Proj->SliderShow(sliderName) = show;
		SliderDisplay* d = sliderDisplays[sliderName];
		if (show)
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else 			
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudio::UpdateActiveShapeUI() {
	mesh* m = glView->GetMesh(activeShape);
	if (m) {
		if (m->smoothSeamNormals) {
			GetMenuBar()->Check(XRCID("btnSmoothSeams"), true);
		} else {
			GetMenuBar()->Check(XRCID("btnSmoothSeams"), false);
		}
	}
}

void OutfitStudio::SelectShape(const string& shapeName) {
	wxTreeItemId item;
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;
	wxTreeItemIdValue subcookie;
	item = outfitShapes->GetFirstChild(shapesRoot, cookie);
	while (item.IsOk()) {
		subitem = outfitShapes->GetFirstChild(item, subcookie);
		while (subitem.IsOk()) {
			if (outfitShapes->GetItemText(subitem) == shapeName) {
				outfitShapes->SelectItem(subitem);
				outfitShapes->EnsureVisible(subitem);
				return;
			}
			subitem = outfitShapes->GetNextChild(item, subcookie);
		}
		item = outfitShapes->GetNextSibling(item);
	}
	UpdateActiveShapeUI();
}

void OutfitStudio::OnShapeLoadDone(AsyncMonitor* monitor) {
	OutfitStudioThreadMonitor* ostm = (OutfitStudioThreadMonitor*)monitor;
	vector<vector3> v;
	vector<tri> t;
	vector<vector2> uv;
	Proj->testFile.CopyDataForIndex(0, &v, &t, &uv);

	vector<string> shapenames;
	Proj->testFile.GetGroupList(shapenames);

	if (uv.size() > 0)
		glView->AddExplicitMesh(&v, &t, &uv, shapenames[0]);
	else
		glView->AddExplicitMesh(&v, &t, NULL, shapenames[0]);
			
	activeShape = shapenames[0];
	glView->SetActiveShape(shapenames[0]);

	wxTreeItemId item = outfitShapes->AppendItem(shapesRoot, Proj->testFile._asyncFN);
	wxTreeItemId subitem;

	subitem=outfitShapes->AppendItem(item, shapenames[0]);
	outfitShapes->SetItemState(subitem, 0);
	outfitShapes->SelectItem(subitem);
	
	UpdateActiveShapeUI();
	glView->Refresh();
	ostm->Delete();
}

void OutfitStudio::UpdateShapeSource(const string& shapeName, bool bIsOutfit) {
	Proj->UpdateShapeFromMesh(shapeName, glView->GetMesh(shapeName), bIsOutfit);
}

void OutfitStudio::ActiveShapeUpdated(TweakStroke* refStroke, bool bIsUndo) {
	if (bEditSlider) {
		unordered_map<int,vector3> strokediff;
		for (auto p: refStroke->pointStartState) {
			if (bIsUndo) {
				strokediff[p.first] = p.second - refStroke->pointEndState[p.first];		
			} else {
				strokediff[p.first] = refStroke->pointEndState[p.first] - p.second;		
			}
		}
		Proj->UpdateMorphResult(activeShape, activeSlider, strokediff, activeItem->bIsOutfitShape);

	} else { 
		if (refStroke->BrushType() == TBT_WEIGHT) {
			unordered_map<int, float> newWeights;
			TweakBrush* br = refStroke->GetRefBrush();
			string refBone;
		
			if (br->Name() == "WeightPaint") {
				refBone = ((TB_Weight*)br)->refBone;
			} else {
				
				refBone = ((TB_Unweight*)br)->refBone;
			}
			if (activeItem->bIsOutfitShape) {
				Proj->workAnim.GetWeights(activeShape, refBone, newWeights);
			} else {				
				Proj->baseAnim.GetWeights(activeShape, refBone, newWeights);
			}

			if(bIsUndo) {
				for (auto p: refStroke->pointStartState) {
					if (p.second.y == 0.0f) {
						newWeights.erase(p.first);
					} else {
						newWeights[p.first] = p.second.y;				
					}
				}
			} else {
				for (auto p: refStroke->pointEndState) {
					if (p.second.y == 0.0f) {
						newWeights.erase(p.first);
					} else {
						newWeights[p.first] = p.second.y;				
					}
				}
			} 
			if (activeItem->bIsOutfitShape) {
				Proj->workAnim.SetWeights(activeShape, refBone, newWeights);
			} else {				
				Proj->baseAnim.SetWeights(activeShape, refBone, newWeights);
			}
		}
		Proj->SetDirty(activeShape);
	}
}

string OutfitStudio::GetActiveShape() {
	return activeShape;
}

string OutfitStudio::GetActiveBone() {
	return activeBone;
}

bool OutfitStudio::IsDirty() {
	return Proj->IsDirty();
}	
bool OutfitStudio::IsDirty(const string& shapeName) {
	return Proj->IsDirty(shapeName);
}
void OutfitStudio::SetClean(const string& shapeName) {
	Proj->Clean(shapeName);
}

// slider edit states -- enable/disable menu items
void OutfitStudio::EnterSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), true);
	menu->Enable(XRCID("sliderExportBSD"), true);
	menu->Enable(XRCID("sliderNegate"), true);
	menu->Enable(XRCID("sliderClear"), true);
	menu->Enable(XRCID("sliderDelete"), true);
	menu->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudio::ExitSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), false);
	menu->Enable(XRCID("sliderExportBSD"), false);
	menu->Enable(XRCID("sliderNegate"), false);
	menu->Enable(XRCID("sliderClear"), false);
	menu->Enable(XRCID("sliderDelete"), false);
	menu->Enable(XRCID("sliderProperties"), false);
}

bool OutfitStudio::NotifyStrokeStarting() {
	if (!activeItem) 
		return false;
	if (bEditSlider) 
		return true;
	if (Proj->AllSlidersZero()) 
		return true;

	int	response = wxMessageBox("You can only edit the base shape when all sliders are zero. Do you wish to set all sliders to zero now?  Note, use the pencil button next to a slider to enable editing of that slider's morph."
		, wxMessageBoxCaptionStr, wxYES_NO, this);
	if (response == wxYES) {
		ZeroSliders();
	} 
	return false;
}

void OutfitStudio::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close(true);
}

void OutfitStudio::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this, "Select a slider set to load", "SliderSets", wxEmptyString, "Slider Set Files (*.xml)|*.xml", wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	string file = loadProjectDialog.GetPath();
	vector<string> setnames;
	SliderSetFile InFile(file);
	if (InFile.fail()) {
		wxMessageBox("Failed to open " + file + " as a slider set file!", "Slider set import failure", 5L | wxICON_ERROR);
		return;
	}

	InFile.GetSetNames(setnames);
	wxArrayString choices;
	for (auto s : setnames)
		choices.Add(s);

	string outfit;
	if (choices.GetCount() > 1) {
		outfit = wxGetSingleChoice("Please choose an outfit to load", "Import a slider set", choices, 0, this);
		if (outfit.empty())
			return;
	}
	else if (choices.GetCount() == 1)
		outfit = choices.front();
	else
		return;

	StartProgress("Loading files for project");

	ClearProject();
	Proj->ClearReference();
	Proj->ClearOutfit();

	delete Proj;
	Proj = new OutfitProject(appConfig, this);

	UpdateProgress(10, "Loading Outfit data");
	StartSubProgress(10, 40);

	int ret = Proj->OutfitFromSliderSet(file, outfit);
	if (ret) {
		EndProgress();
		wxMessageBox("Failed to create project from slider set file!", "Slider set import failure", 5L | wxICON_ERROR);
		RefreshGUIFromProj();
		return;
	}

	UpdateProgress(50, "Creating reference shape");
	string shape = Proj->baseShapeName;
	if (!shape.empty()) {
		ret = Proj->LoadReferenceNif(Proj->activeSet.GetInputFileName(), shape, false);
		if (ret) {
			EndProgress();
			wxMessageBox("Failed to migrate reference mesh!", "Slider set import failure", 5L | wxICON_ERROR);
			RefreshGUIFromProj();
			return;
		}
	}

	UpdateProgress(60, "Loading Textures");
	vector<string> shapes;
	Proj->RefShapes(shapes);
	for (auto s : shapes)
		Proj->SetRefTexture(s, "_AUTO_");

	Proj->SetOutfitTextures("_AUTO_");

	UpdateProgress(80, "Creating Reference UI");
	ReferenceGUIFromProj();

	UpdateProgress(85, "Creating Working UI");
	WorkingGUIFromProj();
	AnimationGUIFromProj();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);
	else if (refRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(refRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		activeShape = activeItem->shapeName;
		glView->SetActiveShape(activeShape);
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = NULL;

	outfitShapes->ExpandAll();

	UpdateProgress(90, "Creating Sliders");
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	UpdateProgress(99, "Applying Slider effect");
	ApplySliders();

	UpdateProgress(100, "Complete");
	this->GetMenuBar()->Enable(XRCID("fileSave"), true);
	EndProgress();
}

void OutfitStudio::OnBrushSettingsSlider(wxScrollEvent& event) {
	int id = event.GetId();
	wxString valstr;
	if (id == XRCID("slideSize")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valSize", wxStaticText);
		valstr.sprintf("%.3f",(float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valstr);

	} else if (id == XRCID("slideStr")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valStr", wxStaticText);
		valstr.sprintf("%.3f",(float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valstr);

	} else if (id == XRCID("slideFocus")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valFocus", wxStaticText);
		valstr.sprintf("%.3f",(float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valstr);

	} else if (id == XRCID("slideSpace")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valSpace", wxStaticText);
		valstr.sprintf("%.3f",(float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valstr);
	}
}

void OutfitStudio::OnBrushSettings(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	TweakBrush* tb = glView->GetActiveBrush();
	wxString valstr;
	wxStaticText* lbl;
	if (!tb)
		return;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgBrushSettings", "wxDialog")) {
		XRCCTRL(dlg, "slideSize", wxSlider)->SetValue(glView->GetBrushSize() * 1000);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valSize", wxStaticText);
		valstr.sprintf("%.3f", glView->GetBrushSize());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideStr", wxSlider)->SetValue(tb->getStrength() * 1000);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valStr", wxStaticText);
		valstr.sprintf("%.3f", tb->getStrength());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideFocus", wxSlider)->SetValue(tb->getFocus() * 1000);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valFocus", wxStaticText);
		valstr.sprintf("%.3f", tb->getFocus());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideSpace", wxSlider)->SetValue(tb->getSpacing() * 1000);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valSpace", wxStaticText);
		valstr.sprintf("%.3f", tb->getSpacing());
		lbl->SetLabel(valstr);

		dlg.Connect(wxEVT_SLIDER, wxScrollEventHandler(OutfitStudio::OnBrushSettingsSlider), NULL, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		int result = dlg.ShowModal();
		if (result == wxID_OK) {	
			float slideSize = XRCCTRL(dlg, "slideSize", wxSlider)->GetValue() / 1000.0f;
			float slideStr = XRCCTRL(dlg, "slideStr", wxSlider)->GetValue() / 1000.0f;
			float slideFocus = XRCCTRL(dlg, "slideFocus", wxSlider)->GetValue() / 1000.0f;
			float slideSpace = XRCCTRL(dlg, "slideSpace", wxSlider)->GetValue() / 1000.0f;

			glView->SetBrushSize(slideSize);
			tb->setStrength(slideStr);
			tb->setFocus(slideFocus);
			tb->setSpacing(slideSpace);

			CheckBrushBounds();
		}
	}
}

void OutfitStudio::OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event) {
	string fn = event.GetPath();
	vector<string> Shapes;
	wxWindow* npWiz = ((wxFilePickerCtrl*)event.GetEventObject())->GetParent();
	wxChoice* setnamechoice = (wxChoice*)XRCCTRL((*npWiz), "npSliderSetName", wxChoice);
	wxChoice* refshapechoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);	
	XRCCTRL((*npWiz), "npRefIsSliderset", wxRadioButton)->SetValue(true);
	setnamechoice->Clear();
	refshapechoice->Clear();
	if (fn.rfind(".xml") != string::npos) {
		SliderSetFile ssf(fn);
		if (ssf.fail()) 
			return;

		vector<string> SetNames;

		ssf.GetSetNames(SetNames);
		for (auto sn: SetNames) {
			setnamechoice->AppendString(sn);
		}
	
		int c = 0;
		int i = 0;
		if (SetNames.size() > 0) {
			setnamechoice->SetSelection(0);
			ssf.SetShapes(SetNames[0],Shapes);
			for (auto rsn: Shapes) {
				if (rsn == "BaseShape") 
					c = i;
				refshapechoice->AppendString(rsn);
				i++;
			}
			refshapechoice->SetSelection(c);
		}  
	} else if (fn.rfind(".nif") != string::npos) {
		NifFile checkfile;
		if (checkfile.Load(fn)) {
			return;
		}
		int c = 0;
		int i = 0;
		checkfile.GetShapeList(Shapes);
		for (auto rsn: Shapes) {
			if (rsn == "BaseShape") 
				c = i;
			refshapechoice->AppendString(rsn);
			i ++;
		}
		refshapechoice->SetSelection(c);
	}	
}

void OutfitStudio::OnNPWizChangeSetNameChoice(wxCommandEvent& event) {
	wxWindow* npWiz = ((wxChoice*)event.GetEventObject())->GetParent();
	wxFilePickerCtrl* file = (wxFilePickerCtrl*)XRCCTRL((*npWiz),"npSliderSetFile",wxFilePickerCtrl);
	if(!file) return;

	string fn = file->GetPath();
	SliderSetFile ssf(fn);
	if(ssf.fail()) 
		return;

	vector<string> shapes;
	wxChoice* chooser = (wxChoice*)event.GetEventObject();
	ssf.SetShapes(chooser->GetStringSelection().ToAscii().data(), shapes);
	wxChoice* refshapechoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refshapechoice->Clear();

	int c = 0;
	int i = 0;
	for(auto rsn: shapes) {
		if(rsn == "BaseShape") 
			c = i;
		refshapechoice->AppendString(rsn);
		i++;
	}
	refshapechoice->SetSelection(c);
}

void OutfitStudio::OnNewProject(wxCommandEvent& WXUNUSED(event)) {
	wxWizard wiz;
	wxWizardPage* pg1;
	wxWizardPage* pg2;
	bool result = false;
	vector<string> templateNames;
	wxArrayString tmplNameArray;
	Config.GetValueArray("ReferenceTemplates", "Template", templateNames);
	tmplNameArray.assign(templateNames.begin(), templateNames.end());

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudio::OnNPWizChangeSetNameChoice, this);
		wiz.FitToPage(pg1);

		pg2 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj2", wxWizardPageSimple);
		XRCCTRL(wiz, "npNifFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNewProject2FP_NIF, this);
		XRCCTRL(wiz, "npObjFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNewProject2FP_OBJ, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNewProject2FP_Texture, this);

		wxChoice* tmplChoice = XRCCTRL(wiz, "npTemplateChoice", wxChoice);
		tmplChoice->Clear();
		tmplChoice->Append(tmplNameArray);
		tmplChoice->Select(0);

		result = wiz.RunWizard(pg1);
	}
	if (!result)
		return;

	this->GetMenuBar()->Enable(XRCID("fileSave"), false);

	string outfitName = XRCCTRL(wiz, "npOutfitName", wxTextCtrl)->GetValue();

	StartProgress("Loading files for new project");

	ClearProject();
	Proj->ClearReference();
	Proj->ClearOutfit();

	delete Proj;
	Proj = new OutfitProject(appConfig, this);

	UpdateProgress(10, "Loading Reference");

	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		Proj->LoadReferenceTemplate(string(XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection()));
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		string fname = string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath());

		if (fname.rfind(".xml") != string::npos) {
			Proj->LoadReference(
				string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection()),
				true,
				string(XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection()));

		}
		else if (fname.rfind(".nif") != string::npos) {
			Proj->LoadReferenceNif(
				string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection()));
		}
	}

	vector<string> shapes;
	Proj->RefShapes(shapes);
	for (auto s : shapes)
		Proj->SetRefTexture(s, "_AUTO_");

	UpdateProgress(40, "Loading outfit");
	int loadReturn = 0;
	int objReturn = 0;

	if (XRCCTRL(wiz, "npWorkNif", wxRadioButton)->GetValue() == true)
		loadReturn = Proj->LoadOutfit(string(XRCCTRL(wiz, "npNifFilename", wxFilePickerCtrl)->GetPath()), outfitName);
	else if (XRCCTRL(wiz, "npWorkObj", wxRadioButton)->GetValue() == true)
		objReturn = Proj->AddShapeFromObjFile(string(XRCCTRL(wiz, "npObjFilename", wxFilePickerCtrl)->GetPath()), outfitName);

	if (loadReturn == 1)
		wxMessageBox("Failed to load outfit .nif file!", "Load failure", wxOK | wxICON_ERROR, this);

	if (objReturn == 1)
		wxMessageBox("Unable to load specified obj file!");
	else if (objReturn == 2)
		wxMessageBox("Failed to load skeleton template (res\\SkeletonBlank.nif)");

	UpdateProgress(80, "Creating Reference UI");

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == true)
		Proj->SetOutfitTextures("_AUTO_");
	else if (XRCCTRL(wiz, "npTexDefault", wxRadioButton)->GetValue() == true)
		Proj->SetOutfitTexturesDefault(string(XRCCTRL(wiz, "npDefaultTexChoice", wxChoice)->GetStringSelection()));
	else
		Proj->SetOutfitTextures(string(XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath()));

	ReferenceGUIFromProj();
	UpdateProgress(85, "Creating Working UI");
	WorkingGUIFromProj();
	AnimationGUIFromProj();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);
	else if (refRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(refRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		activeShape = activeItem->shapeName;
		glView->SetActiveShape(activeShape);
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = NULL;

	outfitShapes->ExpandAll();

	UpdateProgress(90, "Creating Sliders");
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	UpdateProgress(99, "Applying Slider Values");
	ApplySliders();

	UpdateProgress(100, "Complete");

	EndProgress();
}

void OutfitStudio::OnLoadReference(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again.");
		return;
	}

	wxDialog dlg;
	vector<string> templateNames;
	wxArrayString tmplNameArray;
	Config.GetValueArray("ReferenceTemplates", "Template", templateNames);
	tmplNameArray.assign(templateNames.begin(), templateNames.end());

	int result = wxID_CANCEL;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadRef", "wxDialog")) {
		XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(dlg, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudio::OnNPWizChangeSetNameChoice, this);

		wxChoice* tmplChoice = XRCCTRL(dlg, "npTemplateChoice", wxChoice);
		tmplChoice->Clear();
		tmplChoice->Append(tmplNameArray);
		tmplChoice->Select(0);
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress("Loading files for reference");

	vector<string> oldShapes;
	Proj->RefShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	UpdateProgress(10, "Loading Reference Slider Set");

	bool ClearRef = !(XRCCTRL(dlg, "chkClearSliders", wxCheckBox)->IsChecked());

	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		Proj->LoadReferenceTemplate(string(XRCCTRL(dlg, "npTemplateChoice", wxChoice)->GetStringSelection()), ClearRef);
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		string fname = string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath());

		if (fname.rfind(".xml") != string::npos) {
			Proj->LoadReference(
				string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection()),
				ClearRef,
				string(XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection()));

		}
		else if (fname.rfind(".nif") != string::npos) {
			Proj->LoadReferenceNif(
				string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection()),
				ClearRef);
		}
	}
	else
		Proj->ClearReference();

	Proj->RefShapes(oldShapes);
	for (auto s : oldShapes)
		Proj->SetRefTexture(s, "_AUTO_");

	UpdateProgress(60, "Creating Reference UI");
	ReferenceGUIFromProj();
	AnimationGUIFromProj();

	outfitShapes->ExpandAll();

	UpdateProgress(70, "Creating Sliders");
	StartSubProgress(70, 99);
	CreateSetSliders();

	ShowSliderEffect(0);
	UpdateProgress(99, "Applying Slider Values");
	ApplySliders();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);
	else if (refRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(refRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		activeShape = activeItem->shapeName;
		glView->SetActiveShape(activeShape);
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = NULL;

	UpdateProgress(100, "Complete");
	EndProgress();
}

void OutfitStudio::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_NIF, this);
		XRCCTRL(dlg, "npObjFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_OBJ, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_Texture, this);
		if (Proj->workNif.IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue();

	this->GetMenuBar()->Enable(XRCID("fileSave"), false);

	StartProgress("Loading files for outfit");

	vector<string> oldShapes;
	Proj->OutfitShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	UpdateProgress(0, "Loading outfit");

	int loadReturn = 0;
	int objReturn = 0;
	if (XRCCTRL(dlg, "npWorkNif", wxRadioButton)->GetValue() == true) {
		if (!XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->IsChecked())
			loadReturn = Proj->LoadOutfit(string(XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->GetPath()), outfitName);
		else
			loadReturn = Proj->AddNif(string(XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->GetPath()));
	}
	else if (XRCCTRL(dlg, "npWorkObj", wxRadioButton)->GetValue() == true)
		objReturn = Proj->AddShapeFromObjFile(string(XRCCTRL(dlg, "npObjFilename", wxFilePickerCtrl)->GetPath()), outfitName);
	else
		Proj->ClearOutfit();

	if (loadReturn == 1)
		wxMessageBox("Failed to load outfit .nif file!", "Load Failure", wxOK | wxICON_ERROR, this);

	if (objReturn == 1)
		wxMessageBox("Unable to load specified .obj file!");
	else if (objReturn == 2)
		wxMessageBox("Failed to load skeleton template (res\\SkeletonBlank.nif).");

	if (XRCCTRL(dlg, "npTexAuto", wxRadioButton)->GetValue() == true)
		Proj->SetOutfitTextures("_AUTO_");
	else if (XRCCTRL(dlg, "npTexDefault", wxRadioButton)->GetValue() == true)
		Proj->SetOutfitTexturesDefault(string(XRCCTRL(dlg, "npDefaultTexChoice", wxChoice)->GetStringSelection()));
	else
		Proj->SetOutfitTextures(string(XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath()));

	UpdateProgress(50, "Creating Working UI");
	WorkingGUIFromProj();
	AnimationGUIFromProj();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk()) {
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);
	}
	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		activeShape = activeItem->shapeName;
		glView->SetActiveShape(activeShape);
		outfitShapes->SelectItem(itemToSelect);
	}
	else {
		activeItem = NULL;
	}

	outfitShapes->ExpandAll();
	UpdateProgress(100, "Complete");

	EndProgress();
}

void OutfitStudio::ClearProject() {
	vector<string> oldShapes;
	Proj->RefShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	Proj->OutfitShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	Proj->mFileName = "";
	Proj->mOutfitName = "";
	Proj->mDataDir = "";
	Proj->mBaseFile = "";
	Proj->mGamePath = "";
	Proj->mGameFile = "";
	Proj->mGenWeights = true;
	Proj->mCopyRef = true;

	glView->DestroyOverlays();
}

void OutfitStudio::RefreshGUIFromProj() {
	ReferenceGUIFromProj();
	WorkingGUIFromProj();
	AnimationGUIFromProj();
}

void OutfitStudio::AnimationGUIFromProj() {
	vector<string> refBones;
	if (outfitBones->GetChildrenCount(bonesRoot) > 0)
		outfitBones->DeleteChildren(bonesRoot);

	Proj->RefBones(refBones);
	for (auto bone : refBones)
		outfitBones->AppendItem(bonesRoot, bone);
}

void OutfitStudio::ReferenceGUIFromProj() {
	if (refRoot.IsOk()) {
		outfitShapes->DeleteChildren(refRoot);
		outfitShapes->Delete(refRoot);
		refRoot.Unset();
	}
	if (!Proj->baseNif.IsValid())
		return;

	string ssname = Proj->SliderSetName();
	string ssfname = Proj->SliderSetFileName();
	activeItem = NULL;

	vector<string> refShapes;
	Proj->RefShapes(refShapes);
	if (refShapes.size() > 0)
		refRoot = outfitShapes->InsertItem(shapesRoot, 0, ssfname);

	wxTreeItemId baseSubItem;
	string shape = Proj->baseShapeName;

	baseSubItem = outfitShapes->AppendItem(refRoot, shape);
	outfitShapes->SetItemState(baseSubItem, 0);
	outfitShapes->SetItemData(baseSubItem, new ShapeItemData(false, &Proj->baseNif, Proj->baseShapeName));
	glView->AddMeshFromNif(&Proj->baseNif, (char*)shape.c_str());
	glView->SetMeshTexture(shape, Proj->RefTexture(shape), Proj->RefShapeShaderType(shape));
}

//const string& filename, const string& outfitName
void OutfitStudio::WorkingGUIFromProj() {
	if (outfitRoot.IsOk()) {
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	vector<string> workshapes;
	wxTreeItemId subitem;
	Proj->OutfitShapes(workshapes);
	activeItem = NULL;

	if (workshapes.size() > 0)
		outfitRoot = outfitShapes->AppendItem(shapesRoot, Proj->OutfitName());

	for (auto shape : workshapes) {
		glView->DeleteMesh(shape);

		//if(shape == "BaseShape") 
		//	continue;

		glView->AddMeshFromNif(&Proj->workNif, (char*)shape.c_str());
		glView->SetMeshTexture(shape, Proj->OutfitTexture(shape), Proj->OutfitShapeShaderType(shape));
		subitem = outfitShapes->AppendItem(outfitRoot, shape);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(true, &Proj->workNif, shape));
	}
}

void OutfitStudio::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();
	wxTextCtrl* sssName = (wxTextCtrl*)win->FindWindowByName("sssName");
	string copyStr = sssName->GetValue();
	copyStr = Proj->NameAbbreviate(copyStr);
	string defSliderSetFile = copyStr + ".xml";
	string defShapeDataDir = copyStr;
	string defOutputFile = copyStr + ".nif";
	string defOutputPath = "meshes\\armor\\" + copyStr;
	XRCCTRL((*win), "sssOutputFileName", wxTextCtrl)->SetLabel(copyStr);
	XRCCTRL((*win), "sssOutputDataPath", wxTextCtrl)->SetLabel(defOutputPath);
	wxFilePickerCtrl* fp = (wxFilePickerCtrl*)win->FindWindowByName("sssSliderSetFile");
	fp->SetPath(defSliderSetFile);
	wxDirPickerCtrl* dp = (wxDirPickerCtrl*)win->FindWindowByName("sssShapeDataFolder");
	dp->SetPath(defShapeDataDir);
	fp = (wxFilePickerCtrl*)win->FindWindowByName("sssShapeDataFile");
	fp->SetPath(defOutputFile);
}

void OutfitStudio::OnSaveSliderSet(wxCommandEvent &event) {
	if (Proj->mFileName.empty()) {
		OnSaveSliderSetAs(event);
	}
	else {
		if (Proj->OutfitHasUnweighted()) {
			int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
			if (ret != wxYES)
				return;
		}
		StartProgress("Saving...");
		vector<string> shapes;
		vector<mesh*> shapeMeshes;
		Proj->OutfitShapes(shapes);
		for (auto s : shapes){
			if (IsDirty(s)) {
				UpdateShapeSource(s, true);
				Proj->RefreshMorphOutfitShape(s);
			}
			shapeMeshes.push_back(glView->GetMesh(s));
		}
		Proj->RefShapes(shapes);
		for (auto s : shapes) {
			if (IsDirty(s))
				UpdateShapeSource(s, false);
		}
		bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

		if (updateNormals)
			Proj->UpdateNifNormals(&Proj->workNif, shapeMeshes);

		Proj->Save(Proj->mFileName,
			Proj->mOutfitName,
			Proj->mDataDir,
			Proj->mBaseFile,
			Proj->mGamePath,
			Proj->mGameFile,
			Proj->mGenWeights,
			Proj->mCopyRef);

		EndProgress();
	}
}

void OutfitStudio::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!Proj->baseNif.IsValid()) {
		wxMessageBox("You can not save a project without a valid reference shape loaded!", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	if (Proj->OutfitHasUnweighted()) {
		int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (ret != wxYES)
			return;
	}

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &OutfitStudio::OnSSSNameCopy, this);
		string sssName;
		if (!Proj->mOutfitName.empty())
			sssName = Proj->mOutfitName;
		else if (!Proj->OutfitName().empty())
			sssName = Proj->OutfitName();
		else
			sssName = "New Outfit";

		sssName = Proj->NameAbbreviate(sssName);
		XRCCTRL(dlg, "sssName", wxTextCtrl)->SetValue(sssName);
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory("SliderSets");

		if (!Proj->mFileName.empty())
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(Proj->mFileName);
		else
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(sssName + ".xml");

		if (!Proj->mDataDir.empty())
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(Proj->mDataDir);
		else
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(sssName);

		if (!Proj->mBaseFile.empty())
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(Proj->mBaseFile);
		else
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(sssName + ".nif");

		if (!Proj->mGamePath.empty())
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(Proj->mGamePath);
		else
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue("meshes\\armor\\" + sssName);

		if (!Proj->mGameFile.empty())
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(Proj->mGameFile);
		else
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(sssName);

		if (Proj->mGenWeights) {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(true);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(false);
		}
		else {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(false);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(true);
		}
		XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(Proj->mCopyRef);
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	string strFileName, strOutfitName, strDataDir, strBaseFile, strGamePath, strGameFile;
	bool copyRef;
	bool genWeights;

	strFileName = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strFileName.empty() || strFileName.length() <= 4) {
		wxMessageBox("Invalid or no slider set file specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}
	if (strFileName.substr(strFileName.length() - 4, strFileName.length()) != ".xml")
		strFileName.append(".xml");

	strOutfitName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue();
	if (strOutfitName.empty()) {
		wxMessageBox("No outfit name specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetDirName().GetFullName();
	if (strDataDir.empty()) {
		strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetPath();
		if (strDataDir.empty()) {
			wxMessageBox("No data folder specified! Please try again.", "Error", wxOK | wxICON_ERROR);
			return;
		}
	}

	strBaseFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strBaseFile.empty() || strBaseFile.length() <= 4) {
		wxMessageBox("An invalid or no base outfit .nif file name specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}
	if (strBaseFile.substr(strBaseFile.length() - 4, strBaseFile.length()) != ".nif")
		strBaseFile.append(".nif");

	strGamePath = XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->GetValue();
	if (strGamePath.empty()) {
		wxMessageBox("No game file path specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	strGameFile = XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->GetValue();
	if (strGameFile.empty()) {
		wxMessageBox("No game file name specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();

	StartProgress("Saving...");

	vector<string> shapes;
	vector<mesh*> shapeMeshes;
	Proj->OutfitShapes(shapes);
	for (auto s : shapes) {
		if (IsDirty(s)) {
			UpdateShapeSource(s, true);
			Proj->RefreshMorphOutfitShape(s);
		}
		shapeMeshes.push_back(glView->GetMesh(s));
	}
	Proj->RefShapes(shapes);
	for (auto s : shapes) {
		if (IsDirty(s))
			UpdateShapeSource(s, false);
	}

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

	if (updateNormals)
		Proj->UpdateNifNormals(&Proj->workNif, shapeMeshes);

	if (strFileName.find("SliderSets\\") == string::npos)
		strFileName = "SliderSets\\" + strFileName;

	string res = Proj->Save(strFileName, strOutfitName, strDataDir, strBaseFile,
		strGamePath, strGameFile, genWeights, copyRef);

	if (res.empty())
		this->GetMenuBar()->Enable(XRCID("fileSave"), true);

	EndProgress();
}

void OutfitStudio::OnSaveBaseOutfit(wxCommandEvent& WXUNUSED(event)) {
	if (activeItem == NULL)
		return;

	vector<string> shapes;
	Proj->OutfitShapes(shapes);
	for (auto s : shapes) {
		UpdateShapeSource(s, true);
	}
	ZeroSliders();
	Proj->ClearWorkSliders();
	if (!activeSlider.empty()) {
		bEditSlider = false;
		SliderDisplay* d = sliderDisplays[activeSlider];
		d->sliderStrokes.Clear(); //InvalidateHistoricalBVH();
		d->slider->SetFocus();
		HighlightSlider("");
		activeSlider = "";
		glView->SetStrokeManager(NULL);
	}
}

void OutfitStudio::OnExportCurrentShapeNif(wxCommandEvent& WXUNUSED(event)) {
	if (activeShape.empty())
		return;
	if (Proj->OutfitHasUnweighted()) {
		int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (ret != wxYES)
			return;
	}

	vector<string> shapes;
	vector<mesh*> shapeMeshes;
	string fn = wxFileSelector("Save current shape", wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;
	Proj->OutfitShapes(shapes);
	for (auto s : shapes)
		shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));
	Proj->SaveModifiedOutfitNif(fn, shapeMeshes, updateNormals);
}

void OutfitStudio::OnMakeConvRef(wxCommandEvent& WXUNUSED(event)) {
	if (Proj->AllSlidersZero()) {
		wxMessageBox("This function requires at least one slider position to be non-zero.");
		return;
	}

	string namebase = "ConvertToBase";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;
	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser("Create a conversion slider for the current slider settings with the following name: ", "Create New Conversion Slider", thename, this);
	if (finalName == "")
		return;
	finalName = Proj->NameAbbreviate(finalName);

	Proj->AddCombinedSlider(finalName);

	vector<string> refShapes;
	Proj->RefShapes(refShapes);
	for (auto rs : refShapes) {
		Proj->UpdateShapeFromMesh(rs, glView->GetMesh(rs), false);
		Proj->NegateSlider(finalName, rs, false);
	}

	vector<string> sliderList;
	Proj->GetSliderList(sliderList);
	for (auto s : sliderList) {
		if (!s.compare(finalName))
			continue;
		Proj->DeleteSlider(s);
	}
	CreateSetSliders();
}

void OutfitStudio::OnSelectSliders(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	for (auto sd : sliderDisplays)
		ShowSliderEffect(sd.first, checked);

	ApplySliders();
}

void OutfitStudio::OnOutfitVisToggle(wxTreeEvent& event) {
	string s;
	static int groupstate = 0;
	bool notSelf = false;
	bool bVis;
	bool bGhost;
	int state = outfitShapes->GetItemState(event.GetItem());
	s = outfitShapes->GetItemText(event.GetItem()).ToAscii().data();

	if ((GetKeyState(VK_MENU) & 0x8000) > 0) {
		notSelf = true;
		state = groupstate;
	}

	if ((GetKeyState(VK_CONTROL) & 0x8000) > 0) {
		if (state == 2) state = 0;
		if (state == 1) state = 2;
	}


	if (state == 0) {
		bVis = false;
		bGhost = false;
		state = 1;
	}
	else if (state == 1) {
		bVis = true;
		bGhost = true;
		state = 2;
	}
	else {
		bVis = true;
		bGhost = false;
		state = 0;
	}
	if (notSelf) {
		vector<string> shapes;
		Proj->OutfitShapes(shapes);
		for (auto shape : shapes) {
			if (shape == s) continue;
			glView->ShowShape(shape.c_str(), bVis);
			glView->SetShapeGhostMode(shape.c_str(), bGhost);
		}
		groupstate = state;
	}
	else {
		outfitShapes->SetItemState(event.GetItem(), state);
		glView->ShowShape(s.c_str(), bVis);
		glView->SetShapeGhostMode(s.c_str(), bGhost);
		groupstate = 0;
	}

	event.Skip();
}

void OutfitStudio::OnOutfitShapeSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;


	if (outfitShapes->GetItemParent(item).IsOk()) {
		activeShape = outfitShapes->GetItemText(item);
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(item);
		glView->SetActiveShape(activeShape);

	}
	else {
		subitem = outfitShapes->GetFirstChild(item, cookie);
		activeShape = outfitShapes->GetItemText(subitem);
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(subitem);
		glView->SetActiveShape(activeShape);
	}

	UpdateActiveShapeUI();
}

void OutfitStudio::OnOutfitBoneSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;

	if (!activeItem)
		return;

	if (outfitBones->GetItemParent(item).IsOk()) {
		//activeShape = outfitShapes->GetItemText(item);
		//activeItem = (ShapeItemData*) outfitShapes->GetItemData(item);
		//glView->SetActiveShape(activeShape);
		//glView->ToggleMaskVisible();

		// Clear vcolors of all shapes
		vector<string> workShapes;
		Proj->workNif.GetShapeList(workShapes);
		for (auto s : workShapes) {
			mesh* m = glView->GetMesh(s);
			m->ColorChannelFill(1, 0.0f);
		}

		// Selected Shape
		activeBone = outfitBones->GetItemText(item);
		unordered_map<int, float> boneWeights;
		if (activeItem->bIsOutfitShape) {
			Proj->workAnim.GetWeights(activeShape, activeBone, boneWeights);
			mesh* workMesh = glView->GetMesh(activeShape);
			workMesh->ColorChannelFill(1, 0.0f);

			for (auto bw : boneWeights) {
				workMesh->vcolors[bw.first].y = bw.second;
			}
		}
		boneWeights.clear();

		// Reference Shape
		Proj->baseAnim.GetWeights(Proj->baseShapeName, activeBone, boneWeights);
		mesh* baseMesh = glView->GetMesh(Proj->baseShapeName);
		if (baseMesh) {
			baseMesh->ColorChannelFill(1, 0.0f);

			for (auto bw : boneWeights) {
				baseMesh->vcolors[bw.first].y = bw.second;
			}
		}
		glView->Refresh();
	}
	else {
		//subitem = outfitShapes->GetFirstChild(item,cookie);
		//activeShape = outfitShapes->GetItemText(subitem);
		//activeItem = (ShapeItemData*) outfitShapes->GetItemData(subitem);
		//glView->SetActiveShape(activeShape);
	}
}

void OutfitStudio::OnCheckTreeSel(wxTreeEvent& event) {
	int outflags;
	wxPoint p;
	wxGetMousePosition(&p.x, &p.y);
	p = outfitShapes->ScreenToClient(p);

	int mask = wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMSTATEICON;
	outfitShapes->HitTest(p, outflags);
	if ((outflags & mask) != 0)
		event.Veto();
}

void OutfitStudio::OnOutfitShapeContext(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (outfitShapes->GetItemParent(item).IsOk()) {
		outfitShapes->SelectItem(item);
		//OnOutfitShapeSelect(event);
		wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuMeshContext");
		if (menu) {
			PopupMenu(menu);
			delete menu;
		}
	}
}

void OutfitStudio::OnBoneContext(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	outfitBones->SelectItem(item);
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnCheckBox(wxCommandEvent& event) {
	wxCheckBox* box = (wxCheckBox*)event.GetEventObject();
	if (!box)
		return;
	string name = box->GetName().BeforeLast('|');
	ShowSliderEffect(name, event.IsChecked());
	ApplySliders();
}

void OutfitStudio::OnKillFocusOutfitShapes(wxCommandEvent& event) {
	event.Skip();
}

void OutfitStudio::OnSelectBrush(wxCommandEvent& event) {
	int id = event.GetId();
	wxWindow* w = FindFocus();
	wxString s = w->GetName();
	if (s.EndsWith("|readout")){
		GetMenuBar()->Disable();
		return;
	}

	if (id == XRCID("btnSelect")) {
		glView->SetEditMode(false);
		GetMenuBar()->Check(XRCID("btnSelect"), true);
		GetToolBar()->ToggleTool(XRCID("btnSelect"), true);
		return;
	}
	if (id == XRCID("btnMaskBrush")) {
		glView->SetActiveBrush(0);
		GetMenuBar()->Check(XRCID("btnMaskBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnMaskBrush"), true);
	}
	else if (id == XRCID("btnInflateBrush")) {
		glView->SetActiveBrush(1);
		GetMenuBar()->Check(XRCID("btnInflateBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnInflateBrush"), true);
	}
	else if (id == XRCID("btnDeflateBrush")) {
		glView->SetActiveBrush(2);
		GetMenuBar()->Check(XRCID("btnDeflateBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnDeflateBrush"), true);
	}
	else if (id == XRCID("btnMoveBrush")) {
		glView->SetActiveBrush(3);
		GetMenuBar()->Check(XRCID("btnMoveBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnMoveBrush"), true);
	}
	else if (id == XRCID("btnSmoothBrush")) {
		glView->SetActiveBrush(4);
		GetMenuBar()->Check(XRCID("btnSmoothBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnSmoothBrush"), true);
	}
	else if (id == XRCID("btnWeightBrush")) {
		glView->SetActiveBrush(10);
		GetMenuBar()->Check(XRCID("btnWeightBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnWeightBrush"), true);
	}
	else {
		glView->SetEditMode(false);
		return;
	}
	glView->SetEditMode();
}

void OutfitStudio::OnClickSliderButton(wxCommandEvent& event) {
	wxBitmapButton* btn = (wxBitmapButton*)event.GetEventObject();
	if (!btn)
		return;

	string clickedName = btn->GetName().BeforeLast('|');
	if (clickedName.empty()) {
		event.Skip();
		return;
	}

	bEditSlider = false;

	if (activeSlider != clickedName) {
		if (IsDirty(activeShape)) {
			int response = wxMessageBox("You have unsaved changes to the base mesh shape, do you wish to apply the changes? If you select NO, the changes will be lost.", wxMessageBoxCaptionStr, wxYES_NO | wxCANCEL, this);
			if (response == wxCANCEL)
				return;

			if (response == wxYES) {
				vector<string> outfitshapes;
				vector<string> refshapes;
				Proj->RefShapes(refshapes);
				Proj->OutfitShapes(outfitshapes);

				for (auto s : refshapes) {
					UpdateShapeSource(s, false);
					Proj->RefreshMorphOutfitShape(s, false);
				}
				for (auto s : outfitshapes) {
					UpdateShapeSource(s, true);
					Proj->RefreshMorphOutfitShape(s, true);
				}
			}
			if (response == wxNO)
				Proj->Clean();
		}

		bEditSlider = true;
		activeSlider = clickedName;
		SliderDisplay* d = sliderDisplays[activeSlider];
		d->slider->SetValue(100);
		SetSliderValue(activeSlider, 100);

		if (d->sliderNameCheck->Get3StateValue() == wxCheckBoxState::wxCHK_UNCHECKED) {
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
			ShowSliderEffect(d->sliderID - 2000, true);
		}
		d->sliderNameCheck->Enable(false);
		d->slider->SetFocus();
		//glView->GetStrokeManager()->InvalidateHistoricalBVH();
		glView->GetStrokeManager()->PushBVH();
		glView->SetStrokeManager(&d->sliderStrokes);
		EnterSliderEdit();
	}
	else {
		SliderDisplay* d = sliderDisplays[activeSlider];
		d->sliderNameCheck->Enable(true);
		d->slider->SetValue(0);
		SetSliderValue(activeSlider, 0);
		ShowSliderEffect(activeSlider, true);
		activeSlider = "";
		//d->sliderStrokes.InvalidateHistoricalBVH();
		glView->GetStrokeManager()->PushBVH();
		d->slider->SetFocus();
		glView->SetStrokeManager(NULL);
		ExitSliderEdit();
	}
	//d->slider->SetValue(100);
	//d->sliderReadout->SetLabel("100");
	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudio::OnReadoutChange(wxCommandEvent& event){
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString sn = w->GetName();
	if (!sn.EndsWith("|readout", &sn))
		return;

	string sliderName = sn.ToAscii().data();

	double v;
	w->GetValue().ToDouble(&v);
	w->ChangeValue(wxString::Format("%0.0f%%", v));

	SliderDisplay* d = sliderDisplays[sliderName];
	d->slider->SetValue(v);

	int index = Proj->SliderIndexFromName(sliderName);
	Proj->SliderValue(index) = v / 100.0f;

	ApplySliders();
}

void OutfitStudio::OnMeshBoneButtonClick(wxCommandEvent& event) {
	int id = event.GetId();
	wxMenuBar* menuBar = GetMenuBar();
	if (id == XRCID("meshTabButton")) {
		outfitBones->Hide();
		outfitShapes->Show();
		((wxStateButton*)FindWindowByName("boneTabButton"))->SetCheck(false);
		outfitShapes->GetParent()->Layout();
		glView->SetActiveBrush(1);
		toolBar->ToggleTool(XRCID("btnInflateBrush"), true);
		menuBar->Check(XRCID("btnInflateBrush"), true);
		glView->SetWeightVisible(false);
		Refresh();

		toolBar->EnableTool(XRCID("btnWeightBrush"), false);

		toolBar->EnableTool(XRCID("btnInflateBrush"), true);
		toolBar->EnableTool(XRCID("btnDeflateBrush"), true);
		toolBar->EnableTool(XRCID("btnMoveBrush"), true);
		toolBar->EnableTool(XRCID("btnSmoothBrush"), true);

		menuBar->Enable(XRCID("btnWeightBrush"), false);

		menuBar->Enable(XRCID("btnInflateBrush"), true);
		menuBar->Enable(XRCID("btnDeflateBrush"), true);
		menuBar->Enable(XRCID("btnMoveBrush"), true);
		menuBar->Enable(XRCID("btnSmoothBrush"), true);

	}
	else if (id == XRCID("boneTabButton")) {
		outfitShapes->Hide();
		outfitBones->Show();
		((wxStateButton*)FindWindowByName("meshTabButton"))->SetCheck(false);
		outfitShapes->GetParent()->Layout();
		glView->SetActiveBrush(10);
		toolBar->ToggleTool(XRCID("btnWeightBrush"), true);
		menuBar->Check(XRCID("btnWeightBrush"), true);
		glView->SetWeightVisible(true);

		toolBar->EnableTool(XRCID("btnWeightBrush"), true);

		toolBar->EnableTool(XRCID("btnInflateBrush"), false);
		toolBar->EnableTool(XRCID("btnDeflateBrush"), false);
		toolBar->EnableTool(XRCID("btnMoveBrush"), false);
		toolBar->EnableTool(XRCID("btnSmoothBrush"), false);

		menuBar->Enable(XRCID("btnWeightBrush"), true);

		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);

		Refresh();
	}
}

void OutfitStudio::HighlightSlider(const string& name) {
	for (auto d : sliderDisplays) {
		if (d.first == name) {
			d.second->hilite = true;
			d.second->sliderPane->SetBackgroundColour(wxColour(125, 77, 138));
		}
		else {
			d.second->hilite = false;
			d.second->sliderPane->SetBackgroundColour(wxNullColour);
			d.second->slider->Disable();
			d.second->slider->Enable();
		}
	}
	sliderScroll->Refresh();
}

void OutfitStudio::ZeroSliders() {
	if (!Proj->AllSlidersZero()) {
		for (int s = 0; s < Proj->SliderCount(); s++) {
			if (Proj->SliderClamp(s))
				continue;

			SetSliderValue(s, 0);
			sliderDisplays[Proj->SliderName(s)]->slider->SetValue(0);
		}
		ApplySliders();
	}
}

int OutfitStudio::PromptUpdateBase() {
	int response;
	response = wxMessageBox("You have unsaved changes to the base mesh shape, do you wish to apply the changes?  If you select NO, the changes will be lost.", wxMessageBoxCaptionStr, wxYES_NO | wxCANCEL, this);

	if (response == wxCANCEL)
		return response;

	if (response == wxYES) {
		vector<string> outfitshapes;
		vector<string> refshapes;
		Proj->RefShapes(refshapes);
		Proj->OutfitShapes(outfitshapes);

		for (auto s : refshapes) {
			UpdateShapeSource(s, false);
			Proj->RefreshMorphOutfitShape(s, false);
		}
		for (auto s : outfitshapes) {
			UpdateShapeSource(s, true);
			Proj->RefreshMorphOutfitShape(s, true);
		}

		//OnSliderConform(wxCommandEvent());
		//glView->GetStrokeManager()->InvalidateHistoricalBVH();
		glView->GetStrokeManager()->Clear();
	}

	if (response == wxNO) {
		Proj->Clean();
		//	glView->GetStrokeManager()->InvalidateHistoricalBVH();
		glView->GetStrokeManager()->Clear();
	}
	return response;
}

void OutfitStudio::OnSlider(wxScrollEvent& event) {
	vector<vector3> v;
	int response;
	int type = event.GetEventType();
	static bool sentinel = false;

	if (type == wxEVT_SCROLL_CHANGED) {
		if (sentinel) return;
	}
	else if (type == wxEVT_SCROLL_THUMBTRACK)
		response = 1;
	else
		return;

	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	string sliderName = s->GetName().BeforeLast('|');

	if (IsDirty() && !bEditSlider) {
		sentinel = true;
		PromptUpdateBase();
		sentinel = false;
	}

	SetSliderValue(sliderName, event.GetPosition());
	//SetSliderValue(id, event.GetPosition());
	//	string name = activeSet[id].Name;
	//	activeSet[id].curValue = val;	
	//	((wxTextCtrl*)FindWindowById(1200 + id))->SetValue(wxString::Format("%d%%", event.GetPosition()));

	if (!bEditSlider && event.GetEventType() == wxEVT_SCROLL_CHANGED)
		ApplySliders(true);
	else
		ApplySliders(false);
}

void OutfitStudio::OnLoadPreset(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	PresetCollection presets;
	vector<string> names;
	wxChoice* presetChoice;

	string choice;
	bool hi = true;

	presets.LoadPresets("SliderPresets", Proj->SliderSetName(), names);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		if (dlg.ShowModal() != wxID_OK)
			return;

		choice = presetChoice->GetStringSelection();
		if (XRCCTRL(dlg, "weightLo", wxRadioButton)->GetValue())
			hi = false;

		float v;
		bool r;
		if (choice == "Zero All") {
			ZeroSliders();
			return;
		}
		for (int i = 0; i < Proj->SliderCount(); i++) {
			if (Proj->SliderClamp(i))
				continue;

			if (hi)
				r = presets.GetBigPreset(choice, Proj->SliderName(i), v);
			else
				r = presets.GetSmallPreset(choice, Proj->SliderName(i), v);

			if (!r)
				v = Proj->SliderDefault(i, hi) / 100.0f;
			if (Proj->SliderInvert(i))
				v = 1.0f - v;

			v *= 100.0f;
			SetSliderValue(i, v);
		}
		ApplySliders();
	}
}

void OutfitStudio::OnSliderExportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to export data from!", "Error");
		return;
	}

	string fn = wxFileSelector("Export .bsd slider data", wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	Proj->SaveSliderBSD(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape);
	ApplySliders();
}

void OutfitStudio::OnSliderImportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to import data to!", "Error");
		return;
	}

	string fn = wxFileSelector("Import .bsd slider data", wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	Proj->SetSliderFromBSD(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape);
	ApplySliders();
}

void OutfitStudio::OnSliderImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to import data to!", "Error");
		return;
	}

	string fn = wxFileSelector("Import .obj file for slider calculation", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	if (!Proj->SetSliderFromOBJ(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape)) {
		wxMessageBox("Vertex count of .obj file mesh does not match currently selected shape!");
		return;
	}

	ApplySliders();
}

void OutfitStudio::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to clear data from!", "Error");
		return;
	}

	string prompt = "Are you sure you wish to clear the unmasked slider data \"";
	prompt += activeSlider + "\" for the shape \"" + activeItem->shapeName + "\" ?  This cannot be undone.";
	int result = wxMessageBox(prompt, "Confirm data erase", wxYES_NO | wxICON_WARNING, this);
	unordered_map<int, float> mask;

	if (result == wxYES) {
		glView->GetActiveMask(mask);
		if (mask.size() > 0)
			Proj->ClearUnmaskedDiff(activeItem->shapeName, activeSlider, &mask, activeItem->bIsOutfitShape);
		else
			Proj->ClearSlider(activeItem->shapeName, activeSlider, activeItem->bIsOutfitShape);
	}

	ApplySliders();
}

void OutfitStudio::OnNewSlider(wxCommandEvent& WXUNUSED(event)) {
	string namebase = "NewSlider";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;

	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser("Enter a name for the new slider:", "Create New Slider", thename, this);
	if (finalName.empty())
		return;

	finalName = Proj->NameAbbreviate(finalName);
	createSliderGUI(finalName, Proj->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	Proj->AddEmptySlider(finalName);
	ShowSliderEffect(finalName);
	sliderScroll->FitInside();
}

void OutfitStudio::OnNewZapSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	string namebase = "NewZap";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;

	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser("Enter a name for the new zap:", "Create New Zap", thename, this);
	if (finalName.empty())
		return;

	finalName = Proj->NameAbbreviate(finalName);
	createSliderGUI(finalName, Proj->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	unordered_map<int, float> unmasked;
	glView->GetActiveUnmasked(unmasked);

	Proj->AddZapSlider(finalName, unmasked, activeItem->shapeName, activeItem->bIsOutfitShape);
	ShowSliderEffect(finalName);
	sliderScroll->FitInside();
}

void OutfitStudio::OnNewCombinedSlider(wxCommandEvent& WXUNUSED(event)) {
	string namebase = "NewSlider";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;

	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser("Enter a name for the new slider:", "Create New Slider", thename, this);
	if (finalName.empty())
		return;

	finalName = Proj->NameAbbreviate(finalName);
	createSliderGUI(finalName, Proj->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	Proj->AddCombinedSlider(finalName);
	sliderScroll->FitInside();
}

void OutfitStudio::OnSliderNegate(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to negate!", "Error");
		return;
	}

	Proj->NegateSlider(activeSlider, activeItem->shapeName, activeItem->bIsOutfitShape);
}

void OutfitStudio::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to delete!", "Error");
		return;
	}

	string prompt = "Are you sure you wish to delete the slider \"";
	prompt += activeSlider + "\"? This cannot be undone.";
	int result = wxMessageBox(prompt, "Confirm slider delete", wxYES_NO | wxICON_WARNING, this);
	if (result == wxYES) {
		SliderDisplay* sd = sliderDisplays[activeSlider];
		sd->slider->SetValue(0);
		SetSliderValue(activeSlider, 0);
		ShowSliderEffect(activeSlider, true);
		//sd->sliderStrokes.InvalidateHistoricalBVH();
		sd->sliderStrokes.Clear();
		sd->slider->SetFocus();
		glView->SetStrokeManager(NULL);
		ExitSliderEdit();

		sd->btnSliderEdit->Destroy();
		sd->slider->Destroy();
		sd->sliderName->Destroy();
		sd->sliderNameCheck->Destroy();
		sd->sliderReadout->Destroy();
		sd->sliderPane->Destroy();

		sliderScroll->FitInside();
		delete sd;
		sliderDisplays.erase(activeSlider);
		Proj->DeleteSlider(activeSlider);
		activeSlider = "";

		ApplySliders();
	}
}

void OutfitStudio::OnSliderProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to show properties for!", "Error");
		return;
	}

	wxDialog dlg;
	wxString tmpstr;

	string sstr;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		int curSlider = Proj->SliderIndexFromName(activeSlider);
		long loVal = (int)(Proj->SliderDefault(curSlider, false));
		long hiVal = (int)(Proj->SliderDefault(curSlider, true));
		XRCCTRL(dlg, "edSliderName", wxTextCtrl)->SetLabel(activeSlider);
		tmpstr.sprintf("%d", loVal);
		XRCCTRL(dlg, "edValLo", wxTextCtrl)->SetValue(tmpstr);
		tmpstr.sprintf("%d", hiVal);
		XRCCTRL(dlg, "edValHi", wxTextCtrl)->SetValue(tmpstr);
		if (Proj->SliderHidden(curSlider))
			XRCCTRL(dlg, "chkHidden", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		if (Proj->SliderInvert(curSlider))
			XRCCTRL(dlg, "chkInvert", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		if (Proj->SliderZap(curSlider))
			XRCCTRL(dlg, "chkZap", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		XRCCTRL(dlg, "wxID_CANCEL", wxButton)->SetFocus();

		if (dlg.ShowModal() == wxID_OK) {
			Proj->SetSliderZap(curSlider, XRCCTRL(dlg, "chkZap", wxCheckBox)->GetValue());
			Proj->SetSliderInvert(curSlider, XRCCTRL(dlg, "chkInvert", wxCheckBox)->GetValue());
			Proj->SetSliderHidden(curSlider, XRCCTRL(dlg, "chkHidden", wxCheckBox)->GetValue());
			XRCCTRL(dlg, "edValLo", wxTextCtrl)->GetValue().ToLong(&loVal);
			Proj->SetSliderDefault(curSlider, loVal, false);
			XRCCTRL(dlg, "edValHi", wxTextCtrl)->GetValue().ToLong(&hiVal);
			Proj->SetSliderDefault(curSlider, hiVal, true);
			tmpstr = XRCCTRL(dlg, "edSliderName", wxTextCtrl)->GetValue();
			sstr = tmpstr.ToAscii().data();

			if (activeSlider != sstr) {
				Proj->SetSliderName(curSlider, sstr);
				SliderDisplay* d = sliderDisplays[activeSlider];
				sliderDisplays[sstr] = d;
				sliderDisplays.erase(activeSlider);
				d->slider->SetName(sstr + "|slider");
				d->sliderName->SetName(sstr + "|lbl");
				d->btnSliderEdit->SetName(sstr + "|btn");
				d->sliderNameCheck->SetName(sstr + "|check");
				d->sliderReadout->SetName(sstr + "|readout");
				d->sliderName->SetLabel(tmpstr);
				activeSlider = sstr;
			}
		}
	}
}

void OutfitStudio::OnSliderConformAll(wxCommandEvent& event) {
	vector<string> shapes;

	wxTreeItemId curItem;
	wxTreeItemIdValue cookie;
	Proj->OutfitShapes(shapes);
	if (shapes.size() == 0)
		return;
	if (!Proj->baseNif.IsValid())
		return;

	StartProgress("Conforming All Shapes");
	float inc = 100.0f / shapes.size();
	float pos = 0;

	ShapeItemData* activeItemSave = activeItem;
	string activeShapeSave = activeShape;


	curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(curItem);
		activeShape = activeItem->shapeName;
		UpdateProgress(pos * inc, "Conforming " + activeShape);
		StartSubProgress(pos * inc, pos * inc + inc);
		OnSliderConform(event);
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
		pos++;
	}

	activeItem = activeItemSave;
	activeShape = activeShapeSave;

	statusBar->SetStatusText("All shapes conformed.");
	UpdateProgress(100, "All shapes conformed.");
	EndProgress();
}

void OutfitStudio::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (!Proj->baseNif.IsValid())
		return;

	ZeroSliders();

	if (!activeItem->bIsOutfitShape) {
		wxMessageBox("You can't conform the reference shape to itself!", "Error", MB_ICONERROR);
		return;
	}

	string msg;
	string msgLbl = "Conforming [" + activeShape + "]: ";

	StartProgress("Conforming " + activeShape);

	msg = msgLbl + "Initializing data";
	statusBar->SetStatusText(msg);
	UpdateProgress(0, msgLbl);

	Proj->InitConform();

	if (IsDirty(activeShape)) {
		msg = msgLbl + "Recording Modified Shape Data";
		statusBar->SetStatusText(msg);
		UpdateShapeSource(activeShape, true);
		Proj->RefreshMorphOutfitShape(activeShape);
	}
	UpdateProgress(50, "Conforming " + activeShape);

	Proj->morpher.CopyMeshMask(glView->GetMesh(activeShape), activeShape);
	Proj->ConformShape(activeShape);

	msg = msgLbl + "Finished.";
	statusBar->SetStatusText(msg);
	UpdateProgress(100, msg);

	EndProgress();
}

void OutfitStudio::OnImportShape(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector("Import .obj file for new shape", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;


	int ret = Proj->AddShapeFromObjFile(fn, "New Shape", activeShape);
	if (ret == 1) {
		wxMessageBox("Unable to load file: " + fn);
		return;
	}
	else if (ret == 2) {
		wxMessageBox("Failed to load skeleton template (res\\SkeletonBlank.nif)");
		return;
	}
	else if (ret == 101) {  // user chose to merge shapes
		vector<vec3> v;
		Proj->GetLiveOutfitVerts(activeShape, v);
		glView->UpdateMeshVertices(activeShape, &v);
		return;
	}
	else if (ret == 100)  // user canceled at the shape name prompt
		return;

	WorkingGUIFromProj();
	outfitShapes->ExpandAll();
	//glView->AddExplicitMesh(&v, &t, &uv, "ObjTest");
	glView->Refresh();
}

void OutfitStudio::OnExportShape(wxCommandEvent& WXUNUSED(event)) {
	if (activeShape.empty())
		return;

	string fname = wxFileSelector("Export shape as an .obj file", wxEmptyString, string(activeItem->shapeName + ".obj").c_str(), "", "Obj Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	Proj->ExportShape(activeItem->shapeName, fname, activeItem->bIsOutfitShape);
}

void OutfitStudio::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	string newShapeName;
	do {
		wxString result = wxGetTextFromUser("Please enter a new unique name for the shape.", "Rename Shape");
		if (result.empty()) return;
		newShapeName = result;
	} while ((Proj->IsValidShape(newShapeName)));

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	for (unsigned int i = 0; i < sizeof(chars); ++i)
		replace(newShapeName.begin(), newShapeName.end(), chars[i], '_');

	Proj->RenameShape(activeItem->shapeName, newShapeName, activeItem->bIsOutfitShape);
	glView->RenameShape(activeItem->shapeName, newShapeName);

	activeItem->shapeName = newShapeName;
	activeShape = newShapeName;
	outfitShapes->SetItemText(activeItem->GetId(), newShapeName);
}

void OutfitStudio::OnEnterClose(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_RETURN) {
		wxDialog* parent = (wxDialog*)((wxWindow*)event.GetEventObject())->GetParent();
		if (!parent)
			return;

		parent->Close();
		parent->SetReturnCode(wxID_OK);
	}
	event.Skip();
}

void OutfitStudio::OnMoveShape(wxCommandEvent& event) {
	wxDialog dlg;
	unordered_map<int, float> mask;
	unordered_map<int, vec3> diff;
	unordered_map<int, float>* mptr = NULL;
	vector<vec3> verts;
	vec3 d, offs(0.0, -2.54431, 3.2879);

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMoveShape")) {
		XRCCTRL(dlg, "msCustX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewMove, this);
		XRCCTRL(dlg, "msCustY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewMove, this);
		XRCCTRL(dlg, "msCustZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewMove, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			wxRadioBox* b = XRCCTRL(dlg, "msOffsetChoice", wxRadioBox);
			if (b->GetSelection() != 0) {
				offs.x = atof(XRCCTRL(dlg, "msCustX", wxTextCtrl)->GetValue().ToAscii().data());
				offs.y = atof(XRCCTRL(dlg, "msCustY", wxTextCtrl)->GetValue().ToAscii().data());
				offs.z = atof(XRCCTRL(dlg, "msCustZ", wxTextCtrl)->GetValue().ToAscii().data());
			}
		}
		else
			offs.Zero();

		glView->GetActiveMask(mask);
		if (mask.size() > 0)
			mptr = &mask;

		float diffY, diffZ;
		Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		if (bEditSlider) {
			for (int i = 0; i < verts.size(); i++) {
				d = (offs - previewMove) * (1.0f - mask[i]);
				diff[i] = d;
				diffY = diff[i].y / 10.0f;
				diffZ = diff[i].z / 10.0f;
				diff[i].z = diffY;
				diff[i].y = diffZ;
				verts[i] += d;
			}
			Proj->UpdateMorphResult(activeItem->shapeName, activeSlider, diff, activeItem->bIsOutfitShape);
		}
		else {
			d = offs - previewMove;
			Proj->OffsetShape(activeItem->shapeName, d, activeItem->bIsOutfitShape, mptr);
		}

		Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
		previewMove.Zero();
	}
}

void OutfitStudio::OnPreviewMove(wxCommandEvent& event) {
	vector<vec3> verts;
	vec3 d, changed;
	unordered_map<int, float> mask;
	unordered_map<int, vec3> diff;
	unordered_map<int, float>* mptr = NULL;

	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	changed.x = atof(XRCCTRL(*parent, "msCustX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "msCustY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "msCustZ", wxTextCtrl)->GetValue().ToAscii().data());

	glView->GetActiveMask(mask);
	if (mask.size() > 0)
		mptr = &mask;

	float diffY, diffZ;
	Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	if (bEditSlider) {
		for (int i = 0; i < verts.size(); i++) {
			d = (changed - previewMove) * (1.0f - mask[i]);
			diff[i] = d;
			diffY = diff[i].y / 10.0f;
			diffZ = diff[i].z / 10.0f;
			diff[i].z = diffY;
			diff[i].y = diffZ;
			verts[i] += d;
		}
		Proj->UpdateMorphResult(activeItem->shapeName, activeSlider, diff, activeItem->bIsOutfitShape);
	}
	else {
		d = changed - previewMove;
		Proj->OffsetShape(activeItem->shapeName, d, activeItem->bIsOutfitShape, mptr);
	}

	Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	previewMove = changed;
}

void OutfitStudio::OnOffsetShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<vec3> verts;
	vec3 offs;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (activeItem->bIsOutfitShape)
		offs = Proj->workNif.GetShapeVirtualOffset(activeItem->shapeName);
	else
		offs = Proj->baseNif.GetShapeVirtualOffset(activeItem->shapeName);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgOffsetShape")) {
		XRCCTRL(dlg, "osCustX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", offs.x));
		XRCCTRL(dlg, "osCustY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", offs.y));
		XRCCTRL(dlg, "osCustZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", offs.z));

		XRCCTRL(dlg, "osCustX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewOffset, this);
		XRCCTRL(dlg, "osCustY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewOffset, this);
		XRCCTRL(dlg, "osCustZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewOffset, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			offs.x = atof(XRCCTRL(dlg, "osCustX", wxTextCtrl)->GetValue().ToAscii().data());
			offs.y = atof(XRCCTRL(dlg, "osCustY", wxTextCtrl)->GetValue().ToAscii().data());
			offs.z = atof(XRCCTRL(dlg, "osCustZ", wxTextCtrl)->GetValue().ToAscii().data());
		}

		if (activeItem->bIsOutfitShape)
			Proj->workNif.VirtualOffsetShape(activeItem->shapeName, offs, false);
		else
			Proj->baseNif.VirtualOffsetShape(activeItem->shapeName, offs, false);

		Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	}
}

void OutfitStudio::OnPreviewOffset(wxCommandEvent& event) {
	vector<vec3> verts;
	vec3 current, changed;
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	if (activeItem->bIsOutfitShape)
		current = Proj->workNif.GetShapeVirtualOffset(activeItem->shapeName);
	else
		current = Proj->baseNif.GetShapeVirtualOffset(activeItem->shapeName);

	changed.x = atof(XRCCTRL(*parent, "osCustX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "osCustY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "osCustZ", wxTextCtrl)->GetValue().ToAscii().data());

	if (activeItem->bIsOutfitShape)
		Proj->workNif.VirtualOffsetShape(activeItem->shapeName, changed, false);
	else
		Proj->baseNif.VirtualOffsetShape(activeItem->shapeName, changed, false);

	Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
}

void OutfitStudio::OnScaleShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<vec3> verts;
	unordered_map<int, float> mask;
	unordered_map<int, float>* mptr = NULL;
	float scale;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		XRCCTRL(dlg, "scaleValue", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewScale, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK)
			scale = atof(XRCCTRL(dlg, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
		else
			scale = 1.0f;

		glView->GetActiveMask(mask);
		if (mask.size() > 0)
			mptr = &mask;

		scale *= 1.0f / previewScale;

		Proj->ScaleShape(activeItem->shapeName, scale, activeItem->bIsOutfitShape, mptr);
		Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
		previewScale = 1.0f;
	}
}

void OutfitStudio::OnPreviewScale(wxCommandEvent& event) {
	vector<vec3> verts;
	unordered_map<int, float> mask;
	unordered_map<int, float>* mptr = NULL;
	float scale;

	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	scale = atof(XRCCTRL(*parent, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
	if (scale == 0.0f)
		return;

	glView->GetActiveMask(mask);
	if (mask.size() > 0)
		mptr = &mask;

	float scaleNew = scale * (1.0f / previewScale);

	Proj->ScaleShape(activeItem->shapeName, scaleNew, activeItem->bIsOutfitShape, mptr);
	Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	previewScale = scale;
}

void OutfitStudio::OnVirtScaleShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<vec3> verts;
	float scale;
	bool fromCenter;
	//vec3 centerpoint;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (activeItem->bIsOutfitShape)
		Proj->workNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);
	else
		Proj->baseNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgVirtScaleShape")) {
		XRCCTRL(dlg, "scaleValue", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewVirtScale, this);
		XRCCTRL(dlg, "scaleValue", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", scale));
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK)
			scale = atof(XRCCTRL(dlg, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
		if (scale == 0.0f)
			return;

		fromCenter = true;
		if (activeItem->bIsOutfitShape)
			Proj->workNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);
		else
			Proj->baseNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);

		Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	}
}

void OutfitStudio::OnPreviewVirtScale(wxCommandEvent& event) {
	vector<vec3> verts;
	float scale;
	bool fromCenter;

	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	if (activeItem->bIsOutfitShape)
		Proj->workNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);
	else
		Proj->baseNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);

	scale = atof(XRCCTRL(*parent, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
	if (scale == 0.0f)
		return;

	fromCenter = true;
	if (activeItem->bIsOutfitShape)
		Proj->workNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);
	else
		Proj->baseNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);

	Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
}

void OutfitStudio::OnRotateShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<vec3> verts;
	unordered_map<int, float> mask;
	unordered_map<int, vec3> diff;
	unordered_map<int, float>* mptr = NULL;
	float angleX, angleY, angleZ;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotateShape")) {
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			angleX = atof(XRCCTRL(dlg, "angleX", wxTextCtrl)->GetValue().ToAscii().data());
			angleY = atof(XRCCTRL(dlg, "angleY", wxTextCtrl)->GetValue().ToAscii().data());
			angleZ = atof(XRCCTRL(dlg, "angleZ", wxTextCtrl)->GetValue().ToAscii().data());

			glView->GetActiveMask(mask);
			if (mask.size() > 0)
				mptr = &mask;

			vec3 angle(angleX, angleY, angleZ);

			Proj->RotateShape(activeItem->shapeName, angle, activeItem->bIsOutfitShape, mptr);
			Proj->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
			glView->UpdateMeshVertices(activeItem->shapeName, &verts);
		}
	}
}

void OutfitStudio::OnSetShapeTexture(wxCommandEvent& event) {
	wxDialog dlg;
	string texpath;
	string oDispPath;
	string nDispPath;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgShapeTextures")) {
		wxGrid* stTexGrid = XRCCTRL(dlg, "stTexGrid", wxGrid);
		stTexGrid->CreateGrid(9, 1);
		stTexGrid->EnableEditing(true);
		stTexGrid->EnableGridLines(true);
		stTexGrid->EnableDragGridSize(false);
		stTexGrid->SetMargins(0, 0);

		// Columns
		stTexGrid->SetColSize(0, 350);
		stTexGrid->EnableDragColMove(false);
		stTexGrid->EnableDragColSize(false);
		stTexGrid->SetColLabelSize(30);
		stTexGrid->SetColLabelValue(0, wxT("Game Texture Paths (not including game data\\textures path)"));
		stTexGrid->SetColLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

		// Rows
		stTexGrid->AutoSizeRows();
		stTexGrid->EnableDragRowSize(false);
		stTexGrid->SetRowLabelSize(80);
		stTexGrid->SetRowLabelValue(0, wxT("Diffuse"));
		stTexGrid->SetRowLabelValue(1, wxT("Normal"));
		stTexGrid->SetRowLabelValue(2, wxT("Glow/Skin"));
		stTexGrid->SetRowLabelValue(3, wxT("Parallax"));
		stTexGrid->SetRowLabelValue(4, wxT("Environment"));
		stTexGrid->SetRowLabelValue(5, wxT("Env Mask"));
		stTexGrid->SetRowLabelValue(6, wxT("6"));
		stTexGrid->SetRowLabelValue(7, wxT("Specular"));
		stTexGrid->SetRowLabelValue(8, wxT("8"));
		stTexGrid->SetRowLabelValue(9, wxT("9"));
		stTexGrid->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

		// Cell Defaults
		stTexGrid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_TOP);

		for (int i = 0; i < 9; i++) {
			if (activeItem->bIsOutfitShape) {
				Proj->workNif.GetTextureForShape(activeShape, texpath, i);
			}
			else {
				Proj->baseNif.GetTextureForShape(activeShape, texpath, i);
			}
			stTexGrid->SetCellValue(texpath, i, 0);
		}
		oDispPath = Proj->OutfitTexture(activeShape);
		XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->SetPath(oDispPath);
		XRCCTRL(dlg, "btApplyDiffuse", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnApplyDiffuse, this);

		if (dlg.ShowModal() == wxID_OK) {
			nDispPath = XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->GetPath();
			if (nDispPath != oDispPath) {
				if (activeItem->bIsOutfitShape) {
					Proj->SetOutfitTexture(activeShape, nDispPath);
					glView->SetMeshTexture(activeShape, nDispPath, Proj->OutfitShapeShaderType(activeShape));
				}
				else {
					Proj->SetRefTexture(activeShape, nDispPath);
					glView->SetMeshTexture(activeShape, nDispPath, Proj->RefShapeShaderType(activeShape));
				}
			}

			for (int i = 0; i < 9; i++) {
				texpath = stTexGrid->GetCellValue(i, 0);
				if (activeItem->bIsOutfitShape) {
					Proj->workNif.SetTextureForShape(activeShape, texpath, i);
					Proj->workNif.TrimTexturePaths();
				}
				else {
					Proj->baseNif.SetTextureForShape(activeShape, texpath, i);
					Proj->baseNif.TrimTexturePaths();
				}
			}
		}
	}
}

void OutfitStudio::OnApplyDiffuse(wxCommandEvent& event) {
	wxDialog* dlg = (wxDialog*)((wxWindow*)event.GetEventObject())->GetParent();
	if (!dlg)
		return;

	wxFilePickerCtrl* dispPath = (wxFilePickerCtrl*)dlg->FindWindow("stDisplayTexture");
	wxGrid* texGrid = (wxGrid*)dlg->FindWindow("stTexGrid");
	if (!dispPath || !texGrid)
		return;

	string tex = texGrid->GetCellValue(0, 0);
	if (!tex.empty()) {
		string newTex = appConfig["GameDataPath"] + "textures\\" + tex;
		dispPath->SetPath(newTex);
	}
}

void OutfitStudio::OnDupeShape(wxCommandEvent& event) {
	string newname;
	wxTreeItemId subitem;
	if (activeItem) {
		if (!outfitRoot.IsOk()) {
			wxMessageBox("You can only copy shapes into an outfit, and there is no outfit in the current project. Load one first!");
			return;
		}

		do {
			wxString result = wxGetTextFromUser("Please enter a unique name for the duplicated shape.", "Duplicate Shape");
			if (result.empty()) return;
			newname = result;
		} while (Proj->IsValidShape(newname));

		mesh* curshapemesh = glView->GetMesh(activeShape);

		if (activeItem->bIsOutfitShape)
			Proj->DuplicateOutfitShape(activeShape, newname, curshapemesh);
		else
			Proj->DuplicateRefShape(activeShape, newname, curshapemesh);

		glView->AddMeshFromNif(&Proj->workNif, (char*)newname.c_str());
		Proj->SetOutfitTexture(newname, "_AUTO_");

		glView->SetMeshTexture(newname, Proj->OutfitTexture(newname), Proj->OutfitShapeShaderType(newname));

		subitem = outfitShapes->AppendItem(outfitRoot, newname);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(true, &Proj->workNif, newname));
	}
}

void OutfitStudio::OnDeleteShape(wxCommandEvent& event) {
	if (activeItem && activeItem->bIsOutfitShape) {
		if (wxMessageBox("Are you sure you wish to delete " + activeShape + "?  This action cannot be undone.", "Confirm Delete", wxYES_NO) == wxNO)
			return;

		Proj->DeleteOutfitShape(activeShape);
		glView->DeleteMesh(activeShape);
		wxTreeItemId item = activeItem->GetId();
		outfitShapes->Delete(item);
		AnimationGUIFromProj();
		OnOutfitShapeSelect(wxTreeEvent(wxEVT_TREE_SEL_CHANGED, outfitShapes, outfitShapes->GetSelection()));
	}
	else {
		//Proj->DeleteRefShape(activeShape);
	}
}

void OutfitStudio::OnAddBone(wxCommandEvent& event) {
	wxDialog dlg;

	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones")) {
		return;
	}
	dlg.SetSize(450, 470);

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	function< void(wxTreeItemId, AnimBone*) > fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
		for (auto cb : boneParent->children) {
			if (!cb->boneName.empty()) {
				auto newItem = boneTree->AppendItem(treeParent, cb->boneName);
				fAddBoneChildren(newItem, cb);
			}
			else
				fAddBoneChildren(treeParent, cb);
		}
	};
	AnimBone* rb = AnimSkeleton::getInstance().GetBonePtr();
	wxTreeItemId rt = boneTree->AddRoot(rb->boneName);
	fAddBoneChildren(rt, rb);

	if (dlg.ShowModal() == wxID_OK) {
		wxArrayTreeItemIds sel;
		boneTree->GetSelections(sel);
		for (int i = 0; i < sel.size(); i++) {
			string t = boneTree->GetItemText(sel[i]);

			Proj->AddBoneRef(t);
			outfitBones->AppendItem(bonesRoot, t);
			/* TODO: Insert bone into outfit */
		}
	}
}

void OutfitStudio::OnDeleteBone(wxCommandEvent& event) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (int i = 0; i < selItems.size(); i++) {
		string bname = outfitBones->GetItemText(selItems[i]);

		Proj->DeleteBone(bname);
		activeBone = "";

		outfitBones->Delete(selItems[i]);
	}

	outfitBones->GetSelections(selItems);
	if (selItems.size() > 0) {
		OnOutfitBoneSelect(wxTreeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems[0]));
	}
}

void OutfitStudio::OnCopyBoneWeight(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!Proj->baseNif.IsValid())
		return;

	if (activeItem->bIsOutfitShape) {
		StartProgress("Copy bone weights");
		unordered_map<int, float> mask;
		glView->GetActiveMask(mask);
		Proj->CopyBoneWeights(activeShape, &mask);
		EndProgress();
	}
	else
		wxMessageBox("Sorry, you cannot copy weights from the reference shape to itself.", "Cannot copy weights", wxOK | wxICON_INFORMATION, this);
}

void OutfitStudio::OnCopySelectedWeight(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!Proj->baseNif.IsValid())
		return;

	vector<string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	for (int i = 0; i < selItems.size(); i++) {
		selectedBones.push_back(string(outfitBones->GetItemText(selItems[i])));
	}

	if (activeItem->bIsOutfitShape) {
		StartProgress("Copy bone weights");
		unordered_map<int, float> mask;
		glView->GetActiveMask(mask);
		Proj->CopyBoneWeights(activeShape, &mask, &selectedBones);
		EndProgress();
	}
	else
		wxMessageBox("Sorry, you cannot copy weights from the reference shape to itself.", "Cannot copy weights", wxOK | wxICON_INFORMATION, this);
}

void OutfitStudio::OnMaskWeighted(wxCommandEvent& event) {
	if (activeShape.empty())
		return;

	vector<string> bones;
	Proj->RefBones(bones);
	unordered_map<int, float> boneWeights;
	mesh* m = glView->GetMesh(activeShape);
	m->ColorFill(vec3(0.0f, 0.0f, 0.0f));
	for (auto b : bones) {
		if (activeItem->bIsOutfitShape)
			Proj->workAnim.GetWeights(activeShape, b, boneWeights);
		else
			Proj->baseAnim.GetWeights(activeShape, b, boneWeights);
		for (auto bw : boneWeights)
			m->vcolors[bw.first].x = 1.0f;
	}
	glView->Refresh();
}

void OutfitStudio::OnBuildSkinPartitions(wxCommandEvent& event) {
	if (!activeItem)
		return;

	Proj->BuildShapeSkinPartions(activeShape, activeItem->bIsOutfitShape);
}

// ---------------------------------------------------------------------------
// wxGLPanel
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxGLPanel, wxPanel)
	EVT_PAINT(wxGLPanel::OnPaint)
	EVT_ERASE_BACKGROUND(wxGLPanel::OnEraseBackground)
	EVT_SIZE(wxGLPanel::OnSize)
	EVT_MOUSEWHEEL(wxGLPanel::OnMouseWheel)
	EVT_MOTION(wxGLPanel::OnMouseMove)
	EVT_LEFT_DOWN(wxGLPanel::OnLeftDown)
	EVT_LEFT_DCLICK(wxGLPanel::OnLeftDown)
	EVT_LEFT_UP(wxGLPanel::OnLeftUp)
	EVT_MIDDLE_DOWN(wxGLPanel::OnMiddleDown)
	EVT_MIDDLE_UP(wxGLPanel::OnMiddleUp)
	EVT_RIGHT_DOWN(wxGLPanel::OnRightDown)
	EVT_RIGHT_UP(wxGLPanel::OnRightUp)
	EVT_CHAR_HOOK(wxGLPanel::OnKeys)
	EVT_IDLE(wxGLPanel::OnIdle)
	EVT_MOUSE_CAPTURE_LOST(wxGLPanel::OnCaptureLost)
END_EVENT_TABLE()

wxGLPanel::wxGLPanel(wxWindow* parent, const wxSize& size) : wxPanel(parent, -1, wxDefaultPosition, size) {
	HWND pHwnd;
	pHwnd = (HWND)parent->GetHWND();
	if (!gls.MultiSampleQueried()) {
		HWND queryMSWndow = CreateWindowA("STATIC", "Multisampletester", WS_CHILD | SS_OWNERDRAW | SS_NOTIFY, 0, 0, 768, 768, pHwnd, 0, GetModuleHandle(NULL), NULL);
		gls.QueryMultisample(queryMSWndow);
		DestroyWindow(queryMSWndow);
	}

	gls.Initialize((HWND)GetHWND(), false);
	//NoImg.png
	//"whitegrid.png"
	//gls.AddMaterial("robes.dds","maskvshader.vs","defshader.fs");
	//gls.AddMaterial("femalebody_1.dds","maskvshader.vs","skinshader.fs");
	gls.SetStartingView(vec3(0.0f, -5.0f, -15.0f), 768, 768, 65.0f);
	gls.ToggleMask();

	rbuttonDown = false;
	lbuttonDown = false;
	mbuttonDown = false;
	isLDragging = false;
	isRDragging = false;
	isMDragging = false;

	lastX = 0;
	lastY = 0;

	brushSize = 0.45f;
	activeBrush = NULL;
	editMode = false;
	transformMode = 0;
	bMaskPaint = false;
	bWeightPaint = false;
	isPainting = false;
	isTransforming = false;
	bXMirror = true;
	bAutoNormals = true;
	bConnectedEdit = false;

	strokeManager = &baseStrokes;
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	notifyWindow = win;
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, char* shapeName) {
	vector<string> shapeList;
	nif->GetShapeList(shapeList);

	for (int i = 0; i < shapeList.size(); i++) {
		if (shapeName && (shapeList[i] == shapeName))
			gls.AddMeshFromNif(nif, shapeList[i]);
		else if (shapeName)
			continue;
		else
			gls.AddMeshFromNif(nif, shapeList[i]);

		gls.GetMesh(shapeList[i])->BuildTriAdjacency();
		gls.GetMesh(shapeList[i])->BuildEdgeList();
		gls.GetMesh(shapeList[i])->ColorFill(vec3(0, 0, 0));
	}
}

void wxGLPanel::AddExplicitMesh(vector<vector3>* v, vector<tri>* t, vector<vector2>* uv, const string& shapename) {
	gls.AddMeshExplicit(v, t, uv, shapename);
	gls.GetMesh(shapename)->BuildTriAdjacency();
	gls.GetMesh(shapename)->BuildEdgeList();
	gls.GetMesh(shapename)->ColorFill(vec3(0, 0, 0));
}

void wxGLPanel::SetMeshTexture(const string& shapeName, const string& texturefile, int shaderType) {
	mesh* m = gls.GetMesh(shapeName);
	if (!m) return;
	int mat;
	if (shaderType == 0)
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\defshader.fs");
	else
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\skinshader.fs");

	m->MatRef = mat;
}

void wxGLPanel::UpdateMeshVertices(const string& shapeName, vector<vector3>* verts, bool updateBVH) {
	int id = gls.GetMeshID(shapeName);
	gls.Update(id, verts);
	if (updateBVH)
		BVHUpdateQueue.insert(id);

	RecalcNormals(shapeName);
	Refresh();
}

void wxGLPanel::RecalculateMeshBVH(const string& shapeName) {
	gls.RecalculateMeshBVH(gls.GetMeshID(shapeName));
}

void wxGLPanel::ShowShape(const string& shapeName, bool show) {
	int id = gls.GetMeshID(shapeName);
	gls.SetMeshVisibility(id, show);
	Refresh();
}

void wxGLPanel::SetActiveShape(const string& shapeName) {
	gls.SetActiveMesh(shapeName);
}

void wxGLPanel::SetActiveBrush(int brushID) {
	bMaskPaint = false;
	bWeightPaint = false;
	switch (brushID) {
	case 0:
		activeBrush = &maskBrush;
		bMaskPaint = true;
		break;
	case 1:
		activeBrush = &standardBrush;
		break;
	case 2:
		activeBrush = &deflateBrush;
		break;
	case 3:
		activeBrush = &moveBrush;
		break;
	case 4:
		activeBrush = &smoothBrush;
		break;
	case 10:
		editMode = true;
		activeBrush = &weightBrush;
		bWeightPaint = true;
		break;
	}
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	if (event.GetUnicodeKey() == 'V') {
		wxDialog dlg;
		OutfitStudio* main = (OutfitStudio*)notifyWindow;
		wxPoint cursorPos(event.GetPosition());

		unordered_map<int, vec3> diff;
		vector<vec3> verts;
		vec3 newPos;
		vtx oldVertex;

		if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &oldVertex))
			return;

		if (wxXmlResource::Get()->LoadDialog(&dlg, main, "dlgMoveVertex")) {
			main->Proj->GetLiveVerts(main->activeItem->shapeName, verts, main->activeItem->bIsOutfitShape);
			XRCCTRL(dlg, "posX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].x));
			XRCCTRL(dlg, "posY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].y));
			XRCCTRL(dlg, "posZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].z));

			if (dlg.ShowModal() == wxID_OK) {
				newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().ToAscii().data());
				newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().ToAscii().data());
				newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().ToAscii().data());

				if (main->bEditSlider) {
					diff[oldVertex.indexRef] = newPos - verts[oldVertex.indexRef];
					float diffY = diff[oldVertex.indexRef].y / 10.0f;
					float diffZ = diff[oldVertex.indexRef].z / 10.0f;
					diff[oldVertex.indexRef].z = diffY;
					diff[oldVertex.indexRef].y = diffZ;
					verts[oldVertex.indexRef] = newPos;
					main->Proj->UpdateMorphResult(main->activeItem->shapeName, main->activeSlider, diff, main->activeItem->bIsOutfitShape);
				}
				else {
					main->Proj->MoveVertex(main->activeItem->shapeName, newPos, oldVertex.indexRef, main->activeItem->bIsOutfitShape);
				}
				main->Proj->GetLiveVerts(main->activeItem->shapeName, verts, main->activeItem->bIsOutfitShape);
				UpdateMeshVertices(main->activeItem->shapeName, &verts);
			}
		}
	}
	event.Skip();
}

bool wxGLPanel::StartBrushStroke(wxPoint& screenPos) {
	vec3 o;
	vec3 n;
	vec3 v;
	vec3 vo;
	vec3 d;
	vec3 s;
	bool meshHit;

	TweakPickInfo tpi;
	meshHit = gls.CollideMesh(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &tpi.facet);
	if (!meshHit)
		return false;

	if (!((OutfitStudio*)notifyWindow)->NotifyStrokeStarting())
		return false;

	if (bXMirror) {
		gls.GetPickRay(screenPos.x, screenPos.y, d, s);
		d.x *= -1;
		s.x *= -1;
		if (!gls.CollideMesh(screenPos.x, screenPos.y, o, n, &tpi.facetM, &d, &s))
			tpi.facetM = -1;
	}

	n.Normalize();

	wxRect r = this->GetClientRect();
	int cx = r.GetWidth() / 2;
	int cy = r.GetHeight() / 2;
	//gls.GetPickRay(cx, cy, v, vo);
	gls.GetPickRay(screenPos.x, screenPos.y, v, vo);
	v = v * -1;
	tpi.view = v;

	//activeStroke=strokeManager.CreateStroke(gls.GetActiveMesh(), &tweakBrush);
	//tweakBrush.setMirror(true);
	savedBrush = activeBrush;

	if ((GetKeyState(VK_CONTROL) & 0x8000) > 0) {
		if ((GetKeyState(VK_MENU) & 0x8000) > 0) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		if ((GetKeyState(VK_MENU) & 0x8000) > 0) {
			unweightBrush.refBone = ((OutfitStudio*)notifyWindow)->GetActiveBone();
			unweightBrush.setStrength(-weightBrush.getStrength());
			activeBrush = &unweightBrush;
		}
		else {
			weightBrush.refBone = ((OutfitStudio*)notifyWindow)->GetActiveBone();
		}
	}
	else if ((GetKeyState(VK_MENU) & 0x8000) > 0) {
		if (activeBrush == &standardBrush) {
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			activeBrush = &standardBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			activeBrush = &UnMaskBrush;
		}
	}
	else if (activeBrush != &weightBrush && ((GetKeyState(VK_SHIFT) & 0x8000) > 0)) {
		activeBrush = &smoothBrush;
	}
	activeStroke = strokeManager->CreateStroke(gls.GetActiveMesh(), activeBrush);
	activeBrush->setConnected(bConnectedEdit);
	activeBrush->setMirror(bXMirror);
	activeBrush->setRadius(brushSize);
	activeBrush->setLiveNormals(bAutoNormals);
	activeStroke->beginStroke(tpi);
	activeStroke->updateStroke(tpi);
	return true;
}

void wxGLPanel::UpdateBrushStroke(wxPoint& screenPos) {
	vec3 o;
	vec3 n;
	vec3 v;
	vec3 vo;

	vec3 d;	// mirror pick ray direction
	vec3 s;	// mirror pick ray origin

	TweakPickInfo tpi;

	if (activeStroke) {
		wxRect r = this->GetClientRect();
		int cx = r.GetWidth() / 2;
		int cy = r.GetHeight() / 2;
		gls.GetPickRay(cx, cy, v, vo);

		gls.UpdateCursor(screenPos.x, screenPos.y);

		if (activeBrush->Type() == TBT_MOVE)
		{
			vec3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			gls.CollideMesh(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &tpi.facet);
			if (bXMirror) {
				gls.GetPickRay(screenPos.x, screenPos.y, d, s);
				d.x *= -1;
				s.x *= -1;
				if (!gls.CollideMesh(screenPos.x, screenPos.y, o, n, &tpi.facetM, &d, &s))
					tpi.facetM = -1;
			}
			tpi.normal.Normalize();
		}

		v = v*-1;
		tpi.view = v;
		activeStroke->updateStroke(tpi);
	}
}

void wxGLPanel::EndBrushStroke(wxPoint& screenPos) {
	if (activeStroke) {
		activeStroke->endStroke();
		if (activeStroke->BrushType() == TBT_XFORM) {
			//strokeManager->InvalidateHistoricalBVH();
		}

		activeStroke = NULL;
		if (activeBrush != &maskBrush)
			((OutfitStudio*)notifyWindow)->ActiveShapeUpdated(strokeManager->GetCurStateStroke());
		activeBrush = savedBrush;
	}
}

bool wxGLPanel::StartTransform(wxPoint& screenPos) {
	TweakPickInfo tpi;
	int meshHit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &tpi.facet);
	if (meshHit == -1) {
		return false;
	}

	tpi.center = xformCenter;

	string mname = gls.GetOverlay(meshHit)->shapeName;
	if (mname.find("Move") != string::npos) {
		translateBrush.SetXFormType(0);
		activeBrush = &translateBrush;
		switch (mname[0]) {
		case 'X':
			tpi.view = vec3(1.0f, 0.0f, 0.0f);
			tpi.normal = vec3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = vec3(0.0f, 1.0f, 0.0f);
			tpi.normal = vec3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = vec3(0.0f, 0.0f, 1.0f);
			tpi.normal = vec3(1.0f, 0.0f, 0.0f);
			break;
		}
	}
	else if (mname.find("Rotate") != string::npos) {
		translateBrush.SetXFormType(1);
		activeBrush = &translateBrush;
		switch (mname[0]) {
		case 'X':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, vec3(1.0f, 0.0f, 0.0f), -tpi.center.x);
			//tpi.view = vec3(0.0f, 1.0f, 0.0f);
			tpi.normal = vec3(1.0f, 0.0f, 0.0f);
			break;
		case 'Y':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, vec3(0.0f, 1.0f, 0.0f), -tpi.center.y);
			//tpi.view = vec3(-1.0f, 0.0f, 0.0f);
			tpi.normal = vec3(0.0f, 1.0f, 0.0f);
			break;
		case 'Z':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, vec3(0.0f, 0.0f, 1.0f), -tpi.center.z);
			//tpi.view = vec3(-1.0f, 0.0f, 0.0f);
			tpi.normal = vec3(0.0f, 0.0f, 1.0f);
			break;
		}
	}
	else if (mname.find("Scale") != string::npos) {
		translateBrush.SetXFormType(2);
		activeBrush = &translateBrush;
		switch (mname[0]) {
		case 'X':
			tpi.view = vec3(1.0f, 0.0f, 0.0f);
			tpi.normal = vec3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = vec3(0.0f, 1.0f, 0.0f);
			tpi.normal = vec3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = vec3(0.0f, 0.0f, 1.0f);
			tpi.normal = vec3(1.0f, 0.0f, 0.0f);
			break;
		}
	}

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMesh(), activeBrush);

	//	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin,tpi.normal,0.0f);
	activeStroke->beginStroke(tpi);
	activeStroke->updateStroke(tpi);

	ShowTransformTool(false, false);

	return true;
}

void wxGLPanel::UpdateTransform(wxPoint& screenPos) {

	TweakPickInfo tpi;

	vec3 pn;
	float pd;
	((TB_XForm*)(activeBrush))->GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, -pd);
	if (tpi.origin.x < 0)
		tpi.origin.x = tpi.origin.x;
	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndTransform(wxPoint& screenPos) {
	activeStroke->endStroke();
	activeStroke = NULL;
	ShowTransformTool(true, false);
	((OutfitStudio*)notifyWindow)->ActiveShapeUpdated(strokeManager->GetCurStateStroke());
}

bool wxGLPanel::UndoStroke() {
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	mesh* m = strokeManager->GetCurStateMesh();
	bool skip = (!gls.IsValidMesh(m));
	bool ret = strokeManager->backStroke(skip);
	if (!skip) {
		if (ret && curStroke && curStroke->BrushType() != TBT_MASK) {
			((OutfitStudio*)notifyWindow)->ActiveShapeUpdated(curStroke, true);
			if (curStroke->BrushType() == TBT_XFORM) {
				ShowTransformTool(true, false);
			}

		}
	}
	Refresh();
	return ret;
}

bool wxGLPanel::RedoStroke() {
	mesh* m = strokeManager->GetNextStateMesh();
	bool skip = (!gls.IsValidMesh(m));
	bool ret = strokeManager->forwardStroke(skip);
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	if (!skip) {
		if (ret && curStroke->BrushType() != TBT_MASK) {
			((OutfitStudio*)notifyWindow)->ActiveShapeUpdated(curStroke);
			if (curStroke->BrushType() == TBT_XFORM) {
				ShowTransformTool(true, false);
			}
		}
	}
	Refresh();
	return ret;
}

void wxGLPanel::ShowTransformTool(bool show, bool updateBrush) {
	int xform_move = -1;
	int yform_move = -1;
	int zform_move = -1;

	static TweakBrush* saveBrush = NULL;
	if (updateBrush)
		saveBrush = activeBrush;

	vec3 o;
	mesh* m = gls.GetActiveMesh();

	if (m) {
		string mode = Config.GetString("Editing/CenterMode");
		if (mode == "Object") {
			o = gls.GetActiveMesh()->bvh->Center();
		}
		else if (mode == "Selected") {
			o = gls.GetActiveCenter();
		}
	}

	xformCenter = o;
	if (show) {
		if (updateBrush)
			saveBrush = activeBrush;

		xform_move = gls.AddVis3dArrow(o, vec3(1.0f, 0.0f, 0.0f), 0.02f, 0.1f, 1.5f, vec3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		gls.GetOverlay("XMoveMesh")->CreateBVH();
		yform_move = gls.AddVis3dArrow(o, vec3(0.0f, 1.0f, 0.0f), 0.02f, 0.1f, 1.5f, vec3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		gls.GetOverlay("YMoveMesh")->CreateBVH();
		zform_move = gls.AddVis3dArrow(o, vec3(0.0f, 0.0f, 1.0f), 0.02f, 0.1f, 1.5f, vec3(0.0f, 0.0f, 1.0f), "ZMoveMesh");
		gls.GetOverlay("ZMoveMesh")->CreateBVH();
		gls.AddVis3dRing(o, vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.03f, vec3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		gls.GetOverlay("XRotateMesh")->CreateBVH();
		gls.AddVis3dRing(o, vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.03f, vec3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		gls.GetOverlay("YRotateMesh")->CreateBVH();
		gls.AddVis3dRing(o, vec3(0.0f, 0.0f, 1.0f), 1.0f, 0.03f, vec3(0.0f, 0.0f, 1.0f), "ZRotateMesh");
		gls.GetOverlay("ZRotateMesh")->CreateBVH();
	}

	else {
		if (updateBrush) {
			activeBrush = saveBrush;
			if (activeBrush == NULL)
				editMode = false;
		}
		gls.SetOverlayVisibility("XMoveMesh", false);
		gls.SetOverlayVisibility("YMoveMesh", false);
		gls.SetOverlayVisibility("ZMoveMesh", false);
		gls.SetOverlayVisibility("XRotateMesh", false);
		gls.SetOverlayVisibility("YRotateMesh", false);
		gls.SetOverlayVisibility("ZRotateMesh", false);
	}

	Refresh();
}

void wxGLPanel::OnIdle(wxIdleEvent& event) {
	for (auto it : BVHUpdateQueue) {
		gls.RecalculateMeshBVH(it);
	}
	BVHUpdateQueue.clear();
}

void wxGLPanel::OnPaint(wxPaintEvent& event) {
	gls.RenderOneFrame();
	event.Skip();
}

void wxGLPanel::OnEraseBackground(wxEraseEvent& event) {
}

void wxGLPanel::OnSize(wxSizeEvent& event) {
	wxSize sz = event.GetSize();
	gls.SetSize(sz.GetX(), sz.GetY());
	Refresh();
}

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	if ((GetKeyState('S') & 0x8000) > 0)  {
		wxPoint p = event.GetPosition();
		int delt = event.GetWheelRotation();
		if (delt < 0)
			DecBrush();
		else
			IncBrush();
		gls.UpdateCursor(p.x, p.y);
	}
	else {
		int delt = event.GetWheelRotation();
		gls.DollyCamera(delt);
	}
	Refresh();
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	wxFrame* parent = (wxFrame*)this->GetGrandParent()->GetGrandParent();
	if (parent->IsActive())
		this->SetFocus();

	bool cursorExists = false;
	int x;
	int y;
	int t = 0;
	float w = 0.0f;
	float m = 0.0f;
	event.GetPosition(&x, &y);

	if (mbuttonDown) {
		isMDragging = true;
		gls.PanCamera(x - lastX, y - lastY);
		Refresh();
	}

	if (rbuttonDown) {
		isRDragging = true;
		if ((GetKeyState(VK_SHIFT) & 0x8000) > 0) {
			gls.PanCamera(x - lastX, y - lastY);
		}
		else {
			gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
		}
		Refresh();
	}
	if (lbuttonDown) {
		isLDragging = true;
		if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else  {
			if (Config.MatchValue("Input/LeftMousePan", "true")) {
				//	if((GetKeyState(VK_SHIFT) & 0x8000) > 0) {
				gls.PanCamera(x - lastX, y - lastY);
				//	}
			}
		}
		Refresh();
	}
	if (!rbuttonDown && !lbuttonDown) {
		if (editMode && transformMode == 0) {
			cursorExists = gls.UpdateCursor(x, y, &t, &w, &m);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if (cursorExists) {
			if (bWeightPaint)
				((OutfitStudio*)notifyWindow)->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g", t, w), 1);
			else if (bMaskPaint)
				((OutfitStudio*)notifyWindow)->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", t, m), 1);
			else {
				OutfitStudio* main = (OutfitStudio*)notifyWindow;
				vector<vec3> verts;
				main->Proj->GetLiveVerts(main->activeItem->shapeName, verts, main->activeItem->bIsOutfitShape);
				main->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", t, verts[t].x, verts[t].y, verts[t].z), 1);
			}
		}
		else {
			((OutfitStudio*)notifyWindow)->statusBar->SetStatusText("", 1);
		}
		Refresh();
	}
	lastX = x;
	lastY = y;
}

void wxGLPanel::OnLeftDown(wxMouseEvent& event) {
	CaptureMouse();
	lbuttonDown = true;
	bool meshHit;

	if (editMode) {
		meshHit = StartBrushStroke(event.GetPosition());
		if (meshHit) {
			isPainting = true;
		}
	}
	else if (transformMode != 0) {
		meshHit = StartTransform(event.GetPosition());
		if (meshHit) {
			isTransforming = true;
		}
	}
}

void wxGLPanel::OnMiddleDown(wxMouseEvent& event) {
	CaptureMouse();
	mbuttonDown = true;
}

void wxGLPanel::OnMiddleUp(wxMouseEvent& event) {
	if (GetCapture() == this) {
		ReleaseMouse();
	}
	isMDragging = false;
	mbuttonDown = false;
}

void wxGLPanel::OnLeftUp(wxMouseEvent& event) {
	if (GetCapture() == this) {
		ReleaseMouse();
	}
	if (!isLDragging && (!isPainting)) {
		int x, y;
		event.GetPosition(&x, &y);
		wxPoint p = event.GetPosition();
		int meshID = gls.PickMesh(x, y);
		if (meshID == -1) {

		}
		else {
			// if(mBrushMode == BRUSH_SELECT) {
			//((OutfitStudio*)GetParent())->SelectShape(gls.GetMeshName(meshID));
			((OutfitStudio*)notifyWindow)->SelectShape(gls.GetMeshName(meshID));
			// }
		}
	}
	if (isPainting){
		EndBrushStroke(event.GetPosition());
		isPainting = false;
	}
	if (isTransforming) {
		EndTransform(event.GetPosition());
		isTransforming = false;
	}

	isLDragging = false;
	lbuttonDown = false;
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPainting) {
		EndBrushStroke(wxPoint(0, 0));
		isPainting = false;
	}
	isLDragging = false;
	lbuttonDown = false;
}

void wxGLPanel::OnRightDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();
	rbuttonDown = true;
}

void wxGLPanel::OnRightUp(wxMouseEvent& WXUNUSED(event)) {
	if (HasCapture())
		ReleaseMouse();
	rbuttonDown = false;
}

void OutfitStudio::OnNewProject2FP_NIF(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkNif", wxRadioButton)->SetValue(true);
}
void OutfitStudio::OnNewProject2FP_OBJ(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkObj", wxRadioButton)->SetValue(true);
}
void OutfitStudio::OnNewProject2FP_Texture(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npTexFile", wxRadioButton)->SetValue(true);
}
void OutfitStudio::OnLoadOutfitFP_NIF(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkNif", wxRadioButton)->SetValue(true);
}
void OutfitStudio::OnLoadOutfitFP_OBJ(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkObj", wxRadioButton)->SetValue(true);
}
void OutfitStudio::OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npTexFile", wxRadioButton)->SetValue(true);
}
