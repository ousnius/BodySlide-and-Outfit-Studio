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
	
	EVT_TEXT_ENTER(wxID_ANY, OutfitStudio::OnReadoutChange)
	EVT_COMMAND_SCROLL(wxID_ANY, OutfitStudio::OnSlider)
	EVT_BUTTON(wxID_ANY, OutfitStudio::OnClickSliderButton)
	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudio::OnSelectSliders)
	EVT_CHECKBOX(wxID_ANY, OutfitStudio::OnCheckBox)

	EVT_MENU(XRCID("saveBaseShape"), OutfitStudio::OnSetBaseShape)
	EVT_MENU(XRCID("exportOutfitNif"), OutfitStudio::OnExportOutfitNif)
	EVT_MENU(XRCID("exportOutfitNifWithRef"), OutfitStudio::OnExportOutfitNifWithRef)
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

	EVT_MENU(XRCID("btnRecalcNormals"), OutfitStudio::OnRecalcNormals)
	EVT_MENU(XRCID("btnAutoNormals"), OutfitStudio::OnAutoNormals)
	EVT_MENU(XRCID("btnSmoothSeams"), OutfitStudio::OnSmoothNormalSeams)

	EVT_MENU(XRCID("btnGhostMode"), OutfitStudio::OnGhostMesh)
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
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudio::OnTransferSelectedWeight)
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
	
	EVT_BUTTON(XRCID("meshTabButton"), OutfitStudio::OnMeshBoneButtonClick)
	EVT_BUTTON(XRCID("boneTabButton"), OutfitStudio::OnMeshBoneButtonClick)
	
	EVT_MOVE_END(OutfitStudio::OnMoveWindow)
	EVT_SIZE(OutfitStudio::OnSetSize)

END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// OutfitStudio frame
// ----------------------------------------------------------------------------

OutfitStudio::OutfitStudio(wxWindow* parent, const wxPoint& pos, const wxSize& size, ConfigurationManager& inConfig) : appConfig(inConfig) {
	wxXmlResource* resource = wxXmlResource::Get();
	SetParent(parent);
	progressVal = 0;

	resource->InitAllHandlers();
	bool loaded = resource->Load("res\\outfitStudio.xrc");
	if (!loaded)
		return;

	loaded = resource->LoadFrame(this, GetParent(), "outfitStudio");
	if (!loaded)
		return;

	SetIcon(wxIcon("res\\outfitstudio.png", wxBITMAP_TYPE_PNG));

	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	toolBar = (wxToolBar*)FindWindowByName("toolbar");
	wxMenuBar* menu = GetMenuBar();

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

	if (menu)
		menu->Enable(XRCID("btnWeightBrush"), false);


	visStateImages = new wxImageList(16, 16, false, 2);
	wxBitmap visImg("res\\icoVisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap invImg("res\\icoInvisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap wfImg("res\\icoWireframe.png", wxBITMAP_TYPE_PNG);
	visStateImages->Add(visImg);
	visStateImages->Add(invImg);
	visStateImages->Add(wfImg);

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	wxWindow* rightPanel = FindWindowByName("rightSplitPanel");
	rightPanel->SetDoubleBuffered(true);
	rightPanel->SetBackgroundColour(wxColor(112, 112, 112));

	wxStateButton* meshTab = (wxStateButton*)FindWindowByName("meshTabButton");
	meshTab->SetCheck();

	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	outfitShapes->AssignStateImageList(visStateImages);
	shapesRoot = outfitShapes->AddRoot("Shapes");

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	bonesRoot = outfitBones->AddRoot("Bones");

	glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs(this));
	glView->SetNotifyWindow(this);

	resource->AttachUnknownControl("mGLView", glView, this);

	project = new OutfitProject(appConfig, this);	// Create empty project
	CreateSetSliders();

	bEditSlider = false;
	activeItem = nullptr;
	selectedItems.clear();

	previousMirror = true;
	previewScale = 1.0f;

	SetSize(size);
	SetPosition(pos);

	leftPanel->Layout();
}

OutfitStudio::~OutfitStudio() {
	delete project;
	project = nullptr;

	for (auto sd : sliderDisplays)
		delete sd.second;

	delete glView;
	static_cast<BodySlideApp*>(wxApp::GetInstance())->CloseOutfitStudio();
}

void OutfitStudio::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = GetPosition();
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
	float inc = 1.0f;
	if (project->SliderCount()) {
		inc = 90.0f / project->SliderCount();
		StartProgress("Creating sliders...");
	}

	UpdateProgress(0.0f, "Clearing old sliders...");

	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	sliderScroll->Freeze();
	sliderScroll->DestroyChildren();
	for (auto sd : sliderDisplays)
		delete sd.second;

	sliderDisplays.clear();

	wxSizer* rootSz = sliderScroll->GetSizer();

	for (int i = 0; i < project->SliderCount(); i++)  {
		UpdateProgress(inc, "Loading slider: " + project->SliderName(i));
		if (project->SliderClamp(i))    // clamp sliders are a special case, usually an incorrect scale
			continue;

		createSliderGUI(project->SliderName(i), i, sliderScroll, rootSz);
	}

	sliderScroll->Thaw();
	sliderScroll->FitInside();

	EndProgress();
}

void OutfitStudio::createSliderGUI(const string& name, int id, wxScrolledWindow* wnd, wxSizer* rootSz) {
	SliderDisplay* d = new SliderDisplay();
	d->sliderPane = new wxPanel(wnd, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxTAB_TRAVERSAL);
	d->sliderPane->SetBackgroundColour(wxNullColour);
	d->sliderPane->SetMinSize(wxSize(-1, 25));
	d->sliderPane->SetMaxSize(wxSize(-1, 25));

	d->paneSz = new wxBoxSizer(wxHORIZONTAL);

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
	d->sliderReadout = new wxTextCtrl(d->sliderPane, 1200 + id, wxT("0%"), wxDefaultPosition, wxSize(40, -1), wxWANTS_CHARS | wxTE_RIGHT | wxTE_PROCESS_ENTER | wxSIMPLE_BORDER, wxDefaultValidator, name + "|readout");
	d->sliderReadout->SetMaxLength(0);
	d->sliderReadout->SetForegroundColour(wxColour(255, 255, 255));
	d->sliderReadout->SetBackgroundColour(wxColour(48, 48, 48));
	d->sliderReadout->SetMinSize(wxSize(40, 20));
	d->sliderReadout->SetMaxSize(wxSize(40, 20));
	d->sliderReadout->Connect(wxEVT_KILL_FOCUS, wxCommandEventHandler(OutfitStudio::OnReadoutChange), 0, this);

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
	string name = project->SliderName(index);
	project->SliderValue(index) = val / 100.0f;
	sliderDisplays[name]->sliderReadout->SetValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::SetSliderValue(const string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	sliderDisplays[name]->sliderReadout->SetValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::ApplySliders(bool recalcBVH) {
	vector<Vector3> refVerts;
	vector<Vector3> outfitVerts;

	vector<string> outfitshapes;
	vector<string> refshapes;

	project->RefShapes(refshapes);
	project->OutfitShapes(outfitshapes);

	for (auto shape : refshapes) {
		project->GetLiveRefVerts(shape, refVerts);
		glView->UpdateMeshVertices(shape, &refVerts, recalcBVH);
	}

	for (auto shape : outfitshapes) {
		project->GetLiveOutfitVerts(shape, outfitVerts);
		glView->UpdateMeshVertices(shape, &outfitVerts, recalcBVH);
	}
}

void OutfitStudio::ShowSliderEffect(int sliderID, bool show) {
	if (project->ValidSlider(sliderID)) {
		project->SliderShow(sliderID) = show;
		if (show)
			((wxCheckBox*)FindWindowById(1000 + sliderID))->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			((wxCheckBox*)FindWindowById(1000 + sliderID))->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudio::ShowSliderEffect(const string& sliderName, bool show) {
	if (project->ValidSlider(sliderName)) {
		project->SliderShow(sliderName) = show;
		SliderDisplay* d = sliderDisplays[sliderName];
		if (show)
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudio::UpdateActiveShapeUI() {
	if (!activeItem)
		return;

	mesh* m = glView->GetMesh(activeItem->shapeName);
	if (m) {
		if (m->smoothSeamNormals)
			GetMenuBar()->Check(XRCID("btnSmoothSeams"), true);
		else
			GetMenuBar()->Check(XRCID("btnSmoothSeams"), false);
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

void OutfitStudio::UpdateShapeSource(const string& shapeName, bool bIsOutfit) {
	project->UpdateShapeFromMesh(shapeName, glView->GetMesh(shapeName), bIsOutfit);
}

void OutfitStudio::ActiveShapeUpdated(TweakStroke* refStroke, bool bIsUndo) {
	if (bEditSlider) {
		unordered_map<ushort, Vector3> strokeDiff;
		for (auto p : refStroke->pointStartState) {
			if (bIsUndo)
				strokeDiff[p.first] = p.second - refStroke->pointEndState[p.first];
			else
				strokeDiff[p.first] = refStroke->pointEndState[p.first] - p.second;
		}
		project->UpdateMorphResult(activeItem->shapeName, activeSlider, strokeDiff, activeItem->bIsOutfitShape);
	}
	else {
		if (refStroke->BrushType() == TBT_WEIGHT) {
			unordered_map<ushort, float> newWeights;
			TweakBrush* br = refStroke->GetRefBrush();
			string refBone;

			if (br->Name() == "Weight Paint")
				refBone = ((TB_Weight*)br)->refBone;
			else if (br->Name() == "Weight Erase")
				refBone = ((TB_Unweight*)br)->refBone;
			else
				refBone = ((TB_SmoothWeight*)br)->refBone;

			if (activeItem->bIsOutfitShape)
				project->workAnim.GetWeights(activeItem->shapeName, refBone, newWeights);
			else
				project->baseAnim.GetWeights(activeItem->shapeName, refBone, newWeights);

			if (bIsUndo) {
				for (auto p : refStroke->pointStartState) {
					if (p.second.y == 0.0f)
						newWeights.erase(p.first);
					else
						newWeights[p.first] = p.second.y;
				}
			}
			else {
				for (auto p : refStroke->pointEndState) {
					if (p.second.y == 0.0f)
						newWeights.erase(p.first);
					else
						newWeights[p.first] = p.second.y;
				}
			}
			if (activeItem->bIsOutfitShape)
				project->workAnim.SetWeights(activeItem->shapeName, refBone, newWeights);
			else
				project->baseAnim.SetWeights(activeItem->shapeName, refBone, newWeights);
		}
		else
			project->SetDirty(activeItem->shapeName);
	}
}

string OutfitStudio::GetActiveBone() {
	return activeBone;
}

bool OutfitStudio::IsDirty() {
	return project->IsDirty();
}

bool OutfitStudio::IsDirty(const string& shapeName) {
	return project->IsDirty(shapeName);
}

void OutfitStudio::SetClean(const string& shapeName) {
	project->Clean(shapeName);
}

// Slider edit states - enable/disable menu items.
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

	if (bEditSlider || project->AllSlidersZero())
		return true;

	int	response = wxMessageBox("You can only edit the base shape when all sliders are zero. Do you wish to set all sliders to zero now?  Note, use the pencil button next to a slider to enable editing of that slider's morph.",
		wxMessageBoxCaptionStr, wxYES_NO, this);

	if (response == wxYES)
		ZeroSliders();

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
		wxMessageBox("Failed to open " + file + " as a slider set file!", "Slider set import failure", wxICON_ERROR);
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

	StartProgress("Creating project...");

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	delete project;
	project = new OutfitProject(appConfig, this);

	UpdateProgress(10.0f, "Loading outfit data...");
	StartSubProgress(10.0f, 40.0f);

	int ret = project->OutfitFromSliderSet(file, outfit);
	if (ret) {
		EndProgress();
		wxMessageBox("Failed to create project from slider set file!", "Slider set import failure", wxICON_ERROR);
		RefreshGUIFromProj();
		return;
	}

	UpdateProgress(50.0f, "Loading reference shape...");
	string shape = project->baseShapeName;
	if (!shape.empty()) {
		ret = project->LoadReferenceNif(project->activeSet.GetInputFileName(), shape, false);
		if (ret) {
			EndProgress();
			RefreshGUIFromProj();
			return;
		}
	}

	UpdateProgress(60.0f, "Loading textures...");
	vector<string> shapes;
	project->RefShapes(shapes);
	for (auto s : shapes)
		project->SetRefTexture(s, "_AUTO_");

	project->SetOutfitTextures("_AUTO_");

	UpdateProgress(80, "Creating reference...");
	ReferenceGUIFromProj();

	UpdateProgress(85, "Creating outfit...");
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
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	outfitShapes->ExpandAll();

	UpdateProgress(90.0f, "Creating sliders...");
	StartSubProgress(90.0f, 99.0f);
	CreateSetSliders();

	ShowSliderEffect(0);

	UpdateProgress(99.0f, "Applying slider effects...");
	ApplySliders();

	UpdateProgress(100.0f, "Finished");
	this->GetMenuBar()->Enable(XRCID("fileSave"), true);
	EndProgress();
}

void OutfitStudio::OnBrushSettingsSlider(wxScrollEvent& event) {
	int id = event.GetId();
	wxString valStr;
	if (id == XRCID("slideSize")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valSize", wxStaticText);
		valStr.sprintf("%.3f", (float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valStr);
	}
	else if (id == XRCID("slideStr")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valStr", wxStaticText);
		valStr.sprintf("%.3f", (float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valStr);
	}
	else if (id == XRCID("slideFocus")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valFocus", wxStaticText);
		valStr.sprintf("%.3f", (float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valStr);
	}
	else if (id == XRCID("slideSpace")) {
		wxWindow* w = ((wxSlider*)event.GetEventObject())->GetParent();
		wxStaticText* lbl = (wxStaticText*)XRCCTRL((*w), "valSpace", wxStaticText);
		valStr.sprintf("%.3f", (float)event.GetInt() / 1000.0f);
		lbl->SetLabel(valStr);
	}
}

void OutfitStudio::OnBrushSettings(wxCommandEvent& WXUNUSED(event)) {
	TweakBrush* tb = glView->GetActiveBrush();
	if (!tb)
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgBrushSettings", "wxDialog")) {
		wxString valstr;
		XRCCTRL(dlg, "slideSize", wxSlider)->SetValue(glView->GetBrushSize() * 1000.0f);
		wxStaticText* lbl = (wxStaticText*)XRCCTRL(dlg, "valSize", wxStaticText);
		valstr.sprintf("%.3f", glView->GetBrushSize());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideStr", wxSlider)->SetValue(tb->getStrength() * 1000.0f);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valStr", wxStaticText);
		valstr.sprintf("%.3f", tb->getStrength());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideFocus", wxSlider)->SetValue(tb->getFocus() * 1000.0f);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valFocus", wxStaticText);
		valstr.sprintf("%.3f", tb->getFocus());
		lbl->SetLabel(valstr);

		XRCCTRL(dlg, "slideSpace", wxSlider)->SetValue(tb->getSpacing() * 1000.0f);
		lbl = (wxStaticText*)XRCCTRL(dlg, "valSpace", wxStaticText);
		valstr.sprintf("%.3f", tb->getSpacing());
		lbl->SetLabel(valstr);

		dlg.Connect(wxEVT_SLIDER, wxScrollEventHandler(OutfitStudio::OnBrushSettingsSlider), 0, this);
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
		for (auto sn : SetNames) {
			setnamechoice->AppendString(sn);
		}

		int c = 0;
		int i = 0;
		if (SetNames.size() > 0) {
			setnamechoice->SetSelection(0);
			ssf.SetShapes(SetNames[0], Shapes);
			for (auto rsn : Shapes) {
				if (rsn == "BaseShape")
					c = i;
				refshapechoice->AppendString(rsn);
				i++;
			}
			refshapechoice->SetSelection(c);
		}
	}
	else if (fn.rfind(".nif") != string::npos) {
		NifFile checkfile;
		if (checkfile.Load(fn)) {
			return;
		}
		int c = 0;
		int i = 0;
		checkfile.GetShapeList(Shapes);
		for (auto rsn : Shapes) {
			if (rsn == "BaseShape")
				c = i;
			refshapechoice->AppendString(rsn);
			i++;
		}
		refshapechoice->SetSelection(c);
	}
}

void OutfitStudio::OnNPWizChangeSetNameChoice(wxCommandEvent& event) {
	wxWindow* npWiz = ((wxChoice*)event.GetEventObject())->GetParent();
	wxFilePickerCtrl* file = (wxFilePickerCtrl*)XRCCTRL((*npWiz), "npSliderSetFile", wxFilePickerCtrl);
	if (!file)
		return;

	string fn = file->GetPath();
	SliderSetFile ssf(fn);
	if (ssf.fail())
		return;

	vector<string> shapes;
	wxChoice* chooser = (wxChoice*)event.GetEventObject();
	ssf.SetShapes(chooser->GetStringSelection().ToAscii().data(), shapes);
	wxChoice* refshapechoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refshapechoice->Clear();

	int c = 0;
	int i = 0;
	for (auto rsn : shapes) {
		if (rsn == "BaseShape")
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

	StartProgress("Creating project...");

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	delete project;
	project = new OutfitProject(appConfig, this);

	UpdateProgress(10.0f, "Loading reference...");

	int ret = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		ret = project->LoadReferenceTemplate(string(XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection()));
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		string fname = string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath());

		if (fname.rfind(".xml") != string::npos) {
			ret = project->LoadReference(string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection()), true,
				string(XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection()));
		}
		else if (fname.rfind(".nif") != string::npos) {
			ret = project->LoadReferenceNif(string(XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection()));
		}
	}

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	vector<string> shapes;
	project->RefShapes(shapes);
	for (auto s : shapes)
		project->SetRefTexture(s, "_AUTO_");

	UpdateProgress(40.0f, "Loading outfit...");

	ret = 0;
	if (XRCCTRL(wiz, "npWorkNif", wxRadioButton)->GetValue() == true)
		ret = project->LoadOutfit(string(XRCCTRL(wiz, "npNifFilename", wxFilePickerCtrl)->GetPath()), outfitName);
	else if (XRCCTRL(wiz, "npWorkObj", wxRadioButton)->GetValue() == true)
		ret = project->AddShapeFromObjFile(string(XRCCTRL(wiz, "npObjFilename", wxFilePickerCtrl)->GetPath()), outfitName);

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	UpdateProgress(80.0f, "Creating reference...");

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetOutfitTextures("_AUTO_");
	else if (XRCCTRL(wiz, "npTexDefault", wxRadioButton)->GetValue() == true)
		project->SetOutfitTexturesDefault(string(XRCCTRL(wiz, "npDefaultTexChoice", wxChoice)->GetStringSelection()));
	else
		project->SetOutfitTextures(string(XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath()));

	ReferenceGUIFromProj();
	UpdateProgress(85.0f, "Creating outfit...");
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
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	outfitShapes->ExpandAll();

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	UpdateProgress(90.0f, "Creating sliders...");
	StartSubProgress(90.0f, 99.0f);
	CreateSetSliders();

	ShowSliderEffect(0);

	UpdateProgress(99.0f, "Applying slider effects...");
	ApplySliders();

	UpdateProgress(100.0f, "Finished");

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

	StartProgress("Loading reference...");

	vector<string> oldShapes;
	project->RefShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	UpdateProgress(10.0f, "Loading reference set...");

	bool ClearRef = !(XRCCTRL(dlg, "chkClearSliders", wxCheckBox)->IsChecked());

	int ret = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		ret = project->LoadReferenceTemplate(string(XRCCTRL(dlg, "npTemplateChoice", wxChoice)->GetStringSelection()), ClearRef);
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		string fname = string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath());

		if (fname.rfind(".xml") != string::npos) {
			ret = project->LoadReference(string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection()), ClearRef,
				string(XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection()));
		}
		else if (fname.rfind(".nif") != string::npos) {
			ret = project->LoadReferenceNif(string(XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath()),
				string(XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection()), ClearRef);
		}
	}
	else
		project->ClearReference();

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	project->RefShapes(oldShapes);
	for (auto s : oldShapes)
		project->SetRefTexture(s, "_AUTO_");

	UpdateProgress(60.0f, "Creating reference...");
	ReferenceGUIFromProj();
	AnimationGUIFromProj();

	outfitShapes->ExpandAll();

	UpdateProgress(70.0f, "Creating sliders...");
	StartSubProgress(70.0f, 99.0f);
	CreateSetSliders();

	ShowSliderEffect(0);
	UpdateProgress(99.0f, "Applying slider effects...");
	ApplySliders();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);
	else if (refRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(refRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	UpdateProgress(100.0f, "Finished");
	EndProgress();
}

void OutfitStudio::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_NIF, this);
		XRCCTRL(dlg, "npObjFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_OBJ, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_Texture, this);
		if (project->workNif.IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue();

	this->GetMenuBar()->Enable(XRCID("fileSave"), false);

	StartProgress("Loading outfit...");

	vector<string> oldShapes;
	project->OutfitShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	UpdateProgress(1.0f, "Loading outfit...");

	int ret = 0;
	if (XRCCTRL(dlg, "npWorkNif", wxRadioButton)->GetValue() == true) {
		if (!XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->IsChecked())
			ret = project->LoadOutfit(string(XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->GetPath()), outfitName);
		else
			ret = project->AddNif(string(XRCCTRL(dlg, "npNifFilename", wxFilePickerCtrl)->GetPath()));
	}
	else if (XRCCTRL(dlg, "npWorkObj", wxRadioButton)->GetValue() == true)
		ret = project->AddShapeFromObjFile(string(XRCCTRL(dlg, "npObjFilename", wxFilePickerCtrl)->GetPath()), outfitName);
	else
		project->ClearOutfit();

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	if (XRCCTRL(dlg, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetOutfitTextures("_AUTO_");
	else if (XRCCTRL(dlg, "npTexDefault", wxRadioButton)->GetValue() == true)
		project->SetOutfitTexturesDefault(string(XRCCTRL(dlg, "npDefaultTexChoice", wxChoice)->GetStringSelection()));
	else
		project->SetOutfitTextures(string(XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath()));

	UpdateProgress(50.0f, "Creating outfit...");
	WorkingGUIFromProj();
	AnimationGUIFromProj();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	outfitShapes->ExpandAll();
	UpdateProgress(100.0f, "Finished");

	EndProgress();
}

void OutfitStudio::ClearProject() {
	vector<string> oldShapes;
	project->RefShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	project->OutfitShapes(oldShapes);
	for (auto s : oldShapes)
		glView->DeleteMesh(s);

	project->mFileName = "";
	project->mOutfitName = "";
	project->mDataDir = "";
	project->mBaseFile = "";
	project->mGamePath = "";
	project->mGameFile = "";
	project->mGenWeights = true;
	project->mCopyRef = true;

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

	project->RefBones(refBones);
	for (auto bone : refBones)
		outfitBones->AppendItem(bonesRoot, bone);
}

void OutfitStudio::ReferenceGUIFromProj() {
	if (refRoot.IsOk()) {
		outfitShapes->DeleteChildren(refRoot);
		outfitShapes->Delete(refRoot);
		refRoot.Unset();
	}
	if (!project->baseNif.IsValid())
		return;

	string ssname = project->SliderSetName();
	string ssfname = project->SliderSetFileName();
	activeItem = nullptr;
	selectedItems.clear();

	vector<string> refShapes;
	project->RefShapes(refShapes);
	if (refShapes.size() > 0)
		refRoot = outfitShapes->InsertItem(shapesRoot, 0, ssfname);

	wxTreeItemId baseSubItem;
	string shape = project->baseShapeName;

	baseSubItem = outfitShapes->AppendItem(refRoot, shape);
	outfitShapes->SetItemState(baseSubItem, 0);
	outfitShapes->SetItemData(baseSubItem, new ShapeItemData(false, &project->baseNif, project->baseShapeName));
	glView->AddMeshFromNif(&project->baseNif, shape);
	glView->SetMeshTexture(shape, project->RefTexture(shape), project->RefShapeShaderType(shape));
}

void OutfitStudio::WorkingGUIFromProj() {
	if (outfitRoot.IsOk()) {
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	vector<string> workshapes;
	wxTreeItemId subitem;
	project->OutfitShapes(workshapes);
	activeItem = nullptr;
	selectedItems.clear();

	if (workshapes.size() > 0)
		outfitRoot = outfitShapes->AppendItem(shapesRoot, project->OutfitName());

	for (auto shape : workshapes) {
		glView->DeleteMesh(shape);

		glView->AddMeshFromNif(&project->workNif, shape);
		glView->SetMeshTexture(shape, project->OutfitTexture(shape), project->OutfitShapeShaderType(shape));
		subitem = outfitShapes->AppendItem(outfitRoot, shape);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(true, &project->workNif, shape));
	}
}

void OutfitStudio::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();

	string copyStr = XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue();
	copyStr = project->NameAbbreviate(copyStr);
	string defSliderSetFile = copyStr + ".xml";
	string defShapeDataDir = copyStr;
	string defOutputFile = copyStr + ".nif";

	wxFilePickerCtrl* fp = (wxFilePickerCtrl*)win->FindWindowByName("sssSliderSetFile");
	fp->SetPath(defSliderSetFile);

	wxDirPickerCtrl* dp = (wxDirPickerCtrl*)win->FindWindowByName("sssShapeDataFolder");
	dp->SetPath(defShapeDataDir);

	fp = (wxFilePickerCtrl*)win->FindWindowByName("sssShapeDataFile");
	fp->SetPath(defOutputFile);
}

void OutfitStudio::OnSSSGenWeightsTrue(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel("_0/_1.nif");
}

void OutfitStudio::OnSSSGenWeightsFalse(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel(".nif");
}

void OutfitStudio::OnSaveSliderSet(wxCommandEvent& WXUNUSED(event)) {
	if (project->mFileName.empty()) {
		OnSaveSliderSetAs(wxCommandEvent());
	}
	else {
		if (project->OutfitHasUnweighted()) {
			int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
			if (ret != wxYES)
				return;
		}

		StartProgress("Saving project...");
		project->ClearBoneScale();

		vector<string> shapes;
		vector<mesh*> shapeMeshes;
		project->OutfitShapes(shapes);
		for (auto s : shapes){
			if (IsDirty(s)) {
				UpdateShapeSource(s, true);
				project->RefreshMorphOutfitShape(s);
			}
			shapeMeshes.push_back(glView->GetMesh(s));
		}
		project->RefShapes(shapes);
		for (auto s : shapes) {
			if (IsDirty(s))
				UpdateShapeSource(s, false);
		}
		bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

		if (updateNormals)
			project->UpdateNifNormals(&project->workNif, shapeMeshes);

		string res = project->Save(project->mFileName, project->mOutfitName, project->mDataDir, project->mBaseFile,
			project->mGamePath, project->mGameFile, project->mGenWeights, project->mCopyRef);

		if (!res.empty())
			wxMessageBox(res, "Error", wxOK | wxICON_ERROR);

		EndProgress();
	}
}

void OutfitStudio::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!project->baseNif.IsValid()) {
		wxMessageBox("You can not save a project without a valid reference shape loaded!", "Error", wxOK | wxICON_ERROR, this);
		return;
	}

	if (project->OutfitHasUnweighted()) {
		int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (ret != wxYES)
			return;
	}

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnSSSNameCopy, this);
		XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudio::OnSSSGenWeightsTrue, this);
		XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudio::OnSSSGenWeightsFalse, this);
		string sssName;
		if (!project->mOutfitName.empty())
			sssName = project->mOutfitName;
		else if (!project->OutfitName().empty())
			sssName = project->OutfitName();
		else
			sssName = "New Outfit";

		sssName = project->NameAbbreviate(sssName);
		XRCCTRL(dlg, "sssName", wxTextCtrl)->SetValue(sssName);
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory("SliderSets");

		if (!project->mFileName.empty())
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(project->mFileName);
		else
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(sssName + ".xml");

		if (!project->mDataDir.empty())
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(project->mDataDir);
		else
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(sssName);

		if (!project->mBaseFile.empty())
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(project->mBaseFile);
		else
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(sssName + ".nif");

		if (!project->mGamePath.empty())
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(project->mGamePath);
		else
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue("meshes\\armor\\" + sssName);

		if (!project->mGameFile.empty())
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(project->mGameFile);
		else
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(sssName);

		if (project->mGenWeights) {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(true);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(false);
		}
		else {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(false);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(true);
		}
		XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(project->mCopyRef);
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

	StartProgress("Saving project...");
	project->ClearBoneScale();

	vector<string> shapes;
	vector<mesh*> shapeMeshes;
	project->OutfitShapes(shapes);
	for (auto s : shapes) {
		if (IsDirty(s)) {
			UpdateShapeSource(s, true);
			project->RefreshMorphOutfitShape(s);
		}
		shapeMeshes.push_back(glView->GetMesh(s));
	}
	project->RefShapes(shapes);
	for (auto s : shapes) {
		if (IsDirty(s))
			UpdateShapeSource(s, false);
	}

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

	if (updateNormals)
		project->UpdateNifNormals(&project->workNif, shapeMeshes);

	string res = project->Save(strFileName, strOutfitName, strDataDir, strBaseFile,
		strGamePath, strGameFile, genWeights, copyRef);

	if (res.empty())
		this->GetMenuBar()->Enable(XRCID("fileSave"), true);
	else
		wxMessageBox(res, "Error", wxOK | wxICON_ERROR);

	EndProgress();
}

void OutfitStudio::OnSetBaseShape(wxCommandEvent& WXUNUSED(event)) {
	project->ClearBoneScale();
	vector<string> shapes;
	project->OutfitShapes(shapes);
	for (auto s : shapes)
		UpdateShapeSource(s, true);

	ZeroSliders();
	project->ClearWorkSliders();
	if (!activeSlider.empty()) {
		bEditSlider = false;
		SliderDisplay* d = sliderDisplays[activeSlider];
		d->sliderStrokes.Clear(); //InvalidateHistoricalBVH();
		d->slider->SetFocus();
		HighlightSlider("");
		activeSlider = "";
		glView->SetStrokeManager(nullptr);
	}
}

void OutfitStudio::OnExportOutfitNif(wxCommandEvent& WXUNUSED(event)) {
	if (!project->workNif.IsValid())
		return;

	if (project->OutfitHasUnweighted()) {
		int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (ret != wxYES)
			return;
	}

	string fileName = wxFileSelector("Export outfit NIF", wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.empty())
		return;

	project->ClearBoneScale();

	vector<string> shapes;
	vector<mesh*> shapeMeshes;
	project->OutfitShapes(shapes);
	for (auto s : shapes)
		shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));
	project->SaveOutfitNif(fileName, shapeMeshes, updateNormals);
}

void OutfitStudio::OnExportOutfitNifWithRef(wxCommandEvent& WXUNUSED(event)) {
	if (!project->workNif.IsValid())
		return;

	if (!project->baseNif.IsValid()) {
		OnExportOutfitNif(wxCommandEvent());
		return;
	}

	if (project->OutfitHasUnweighted()) {
		int ret = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (ret != wxYES)
			return;
	}

	string fileName = wxFileSelector("Export project NIF", wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.empty())
		return;

	project->ClearBoneScale();

	vector<string> shapes;
	vector<mesh*> shapeMeshes;
	project->OutfitShapes(shapes);
	project->RefShapes(shapes);
	for (auto s : shapes)
		shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));
	project->SaveOutfitNif(fileName, shapeMeshes, updateNormals, true);
}

void OutfitStudio::OnMakeConvRef(wxCommandEvent& WXUNUSED(event)) {
	if (project->AllSlidersZero()) {
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

	finalName = project->NameAbbreviate(finalName);
	project->ClearBoneScale();
	project->AddCombinedSlider(finalName);

	vector<string> refShapes;
	project->RefShapes(refShapes);
	for (auto rs : refShapes) {
		project->UpdateShapeFromMesh(rs, glView->GetMesh(rs), false);
		project->NegateSlider(finalName, rs, false);
	}

	vector<string> sliderList;
	project->GetSliderList(sliderList);
	for (auto s : sliderList) {
		if (!s.compare(finalName))
			continue;
		project->DeleteSlider(s);
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

	if (wxGetKeyState(WXK_ALT)) {
		notSelf = true;
		state = groupstate;
	}

	if (wxGetKeyState(WXK_CONTROL)) {
		if (state == 2)
			state = 0;
		if (state == 1)
			state = 2;
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
		project->OutfitShapes(shapes);
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
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(item);
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
	}
	else {
		subitem = outfitShapes->GetFirstChild(item, cookie);
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(subitem);
		if (activeItem)
			glView->SetActiveShape(activeItem->shapeName);
		else
			glView->SetActiveShape("");
	}

	selectedItems.clear();
	wxArrayTreeItemIds selected;
	outfitShapes->GetSelections(selected);
	for (auto i : selected) {
		if (outfitShapes->GetItemParent(i).IsOk()) {
			ShapeItemData* data = (ShapeItemData*)outfitShapes->GetItemData(i);
			if (data)
				selectedItems.push_back(data);
		}
		else {
			subitem = outfitShapes->GetFirstChild(i, cookie);
			ShapeItemData* data = (ShapeItemData*)outfitShapes->GetItemData(subitem);
			if (data)
				selectedItems.push_back(data);
		}
	}

	UpdateActiveShapeUI();
}

void OutfitStudio::OnOutfitBoneSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;

	project->ClearBoneScale();
	((wxSlider*)FindWindowByName("boneScale"))->SetValue(0);

	if (!activeItem)
		return;

	if (outfitBones->GetItemParent(item).IsOk()) {
		// Clear vcolors of all shapes
		vector<string> workShapes;
		project->workNif.GetShapeList(workShapes);
		for (auto s : workShapes) {
			mesh* m = glView->GetMesh(s);
			m->ColorChannelFill(1, 0.0f);
		}

		// Selected Shape
		activeBone = outfitBones->GetItemText(item);
		unordered_map<ushort, float> boneWeights;
		if (activeItem->bIsOutfitShape) {
			project->workAnim.GetWeights(activeItem->shapeName, activeBone, boneWeights);
			mesh* workMesh = glView->GetMesh(activeItem->shapeName);
			workMesh->ColorChannelFill(1, 0.0f);

			for (auto bw : boneWeights) {
				workMesh->vcolors[bw.first].y = bw.second;
			}
		}
		boneWeights.clear();

		// Reference Shape
		project->baseAnim.GetWeights(project->baseShapeName, activeBone, boneWeights);
		mesh* baseMesh = glView->GetMesh(project->baseShapeName);
		if (baseMesh) {
			baseMesh->ColorChannelFill(1, 0.0f);

			for (auto bw : boneWeights) {
				baseMesh->vcolors[bw.first].y = bw.second;
			}
		}
		glView->Refresh();
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

void OutfitStudio::OnSelectBrush(wxCommandEvent& event) {
	int id = event.GetId();
	wxWindow* w = FindFocus();
	wxString s = w->GetName();
	if (s.EndsWith("|readout")){
		GetMenuBar()->Disable();
		return;
	}

	if (glView->GetActiveBrush() && glView->GetActiveBrush()->Type() == TBT_WEIGHT) {
		glView->SetXMirror(previousMirror);
		GetMenuBar()->Check(XRCID("btnXMirror"), previousMirror);
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
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
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
		if (IsDirty(activeItem->shapeName)) {
			int response = wxMessageBox("You have unsaved changes to the base mesh shape, do you wish to apply the changes? If you select NO, the changes will be lost.", wxMessageBoxCaptionStr, wxYES_NO | wxCANCEL, this);
			if (response == wxCANCEL)
				return;

			if (response == wxYES) {
				vector<string> outfitshapes;
				vector<string> refshapes;
				project->RefShapes(refshapes);
				project->OutfitShapes(outfitshapes);

				for (auto s : refshapes) {
					if (s.empty())
						continue;
					UpdateShapeSource(s, false);
					project->RefreshMorphOutfitShape(s, false);
				}
				for (auto s : outfitshapes) {
					UpdateShapeSource(s, true);
					project->RefreshMorphOutfitShape(s, true);
				}
			}
			if (response == wxNO)
				project->Clean();
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
		glView->SetStrokeManager(nullptr);
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

	int index = project->SliderIndexFromName(sliderName);
	project->SliderValue(index) = v / 100.0f;

	ApplySliders();
}

void OutfitStudio::OnMeshBoneButtonClick(wxCommandEvent& event) {
	int id = event.GetId();
	wxMenuBar* menuBar = GetMenuBar();
	if (id == XRCID("meshTabButton")) {
		outfitBones->Hide();
		outfitShapes->Show();
		((wxStateButton*)FindWindowByName("boneTabButton"))->SetCheck(false);
		((wxSlider*)FindWindowByName("boneScale"))->Show(false);
		((wxStaticText*)FindWindowByName("boneScaleLabel"))->Show(false);
		outfitShapes->GetParent()->Layout();
		project->ClearBoneScale();

		if (glView->GetActiveBrush()->Type() == TBT_WEIGHT) {
			glView->SetXMirror(previousMirror);
			GetMenuBar()->Check(XRCID("btnXMirror"), previousMirror);
		}

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
		wxSlider* boneScale = (wxSlider*)FindWindowByName("boneScale");
		boneScale->SetValue(0);
		boneScale->Show();
		((wxStaticText*)FindWindowByName("boneScaleLabel"))->Show();
		outfitShapes->GetParent()->Layout();

		glView->SetActiveBrush(10);
		toolBar->ToggleTool(XRCID("btnWeightBrush"), true);
		menuBar->Check(XRCID("btnWeightBrush"), true);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		glView->SetEditMode();
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
	if (!project->AllSlidersZero()) {
		for (int s = 0; s < project->SliderCount(); s++) {
			if (project->SliderClamp(s))
				continue;

			SetSliderValue(s, 0);
			sliderDisplays[project->SliderName(s)]->slider->SetValue(0);
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
		project->RefShapes(refshapes);
		project->OutfitShapes(outfitshapes);

		for (auto s : refshapes) {
			if (s.empty())
				continue;
			UpdateShapeSource(s, false);
			project->RefreshMorphOutfitShape(s, false);
		}
		for (auto s : outfitshapes) {
			UpdateShapeSource(s, true);
			project->RefreshMorphOutfitShape(s, true);
		}

		//OnSliderConform(wxCommandEvent());
		//glView->GetStrokeManager()->InvalidateHistoricalBVH();
		glView->GetStrokeManager()->Clear();
	}

	if (response == wxNO) {
		project->Clean();
		//glView->GetStrokeManager()->InvalidateHistoricalBVH();
		glView->GetStrokeManager()->Clear();
	}
	return response;
}

void OutfitStudio::OnSlider(wxScrollEvent& event) {
	int type = event.GetEventType();
	static bool sentinel = false;

	if (type == wxEVT_SCROLL_CHANGED)
		if (sentinel)
			return;

	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	if (IsDirty() && !bEditSlider) {
		sentinel = true;
		if (PromptUpdateBase() == wxCANCEL)
			return;
		sentinel = false;
	}

	string sliderName = s->GetName();
	if (sliderName == "boneScale") {
		wxArrayTreeItemIds selItems;
		outfitBones->GetSelections(selItems);
		if (selItems.size() > 0) {
			string selectedBone = outfitBones->GetItemText(selItems.front());
			project->ApplyBoneScale(selectedBone, event.GetPosition());
		}
		return;
	}
	sliderName = s->GetName().BeforeLast('|');
	if (sliderName.empty())
		return;

	SetSliderValue(sliderName, event.GetPosition());

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

	if (IsDirty())
		if (PromptUpdateBase() == wxCANCEL)
			return;

	presets.LoadPresets("SliderPresets", project->SliderSetName(), names);
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
		for (int i = 0; i < project->SliderCount(); i++) {
			if (project->SliderClamp(i))
				continue;

			if (hi)
				r = presets.GetBigPreset(choice, project->SliderName(i), v);
			else
				r = presets.GetSmallPreset(choice, project->SliderName(i), v);

			if (!r)
				v = project->SliderDefault(i, hi) / 100.0f;
			if (project->SliderInvert(i))
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

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector("Export.bsd slider data to directory", wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		for (auto i : selectedItems)
			project->SaveSliderBSDToDir(activeSlider, i->shapeName, dir, i->bIsOutfitShape);
	}
	else {
		string fn = wxFileSelector("Export .bsd slider data", wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.empty())
			return;

		project->SaveSliderBSD(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape);
	}

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

	project->SetSliderFromBSD(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape);
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

	if (!project->SetSliderFromOBJ(activeSlider, activeItem->shapeName, fn, activeItem->bIsOutfitShape)) {
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

	int result;
	if (selectedItems.size() > 1) {
		string prompt = "Are you sure you wish to clear the unmasked slider data \"";
		prompt += activeSlider + "\" for the selected shapes? This cannot be undone.";
		result = wxMessageBox(prompt, "Confirm data erase", wxYES_NO | wxICON_WARNING, this);
	}
	else {
		string prompt = "Are you sure you wish to clear the unmasked slider data \"";
		prompt += activeSlider + "\" for the shape \"" + activeItem->shapeName + "\" ? This cannot be undone.";
		result = wxMessageBox(prompt, "Confirm data erase", wxYES_NO | wxICON_WARNING, this);
	}
	if (result != wxYES)
		return;

	unordered_map<ushort, float> mask;
	for (auto i : selectedItems) {
		mask.clear();
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			project->ClearUnmaskedDiff(i->shapeName, activeSlider, &mask, i->bIsOutfitShape);
		else
			project->ClearSlider(i->shapeName, activeSlider, i->bIsOutfitShape);
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

	finalName = project->NameAbbreviate(finalName);
	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddEmptySlider(finalName);
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

	finalName = project->NameAbbreviate(finalName);
	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	unordered_map<ushort, float> unmasked;
	for (auto i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->shapeName);
		project->AddZapSlider(finalName, unmasked, i->shapeName, i->bIsOutfitShape);
	}

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

	finalName = project->NameAbbreviate(finalName);
	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddCombinedSlider(finalName);
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

	for (auto i : selectedItems)
		project->NegateSlider(activeSlider, i->shapeName, i->bIsOutfitShape);
}

void OutfitStudio::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
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
		glView->SetStrokeManager(nullptr);
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
		project->DeleteSlider(activeSlider);
		activeSlider = "";

		ApplySliders();
	}
}

void OutfitStudio::OnSliderProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to show properties for!", "Error");
		return;
	}

	wxDialog dlg;
	wxString tmpstr;

	string sstr;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		int curSlider = project->SliderIndexFromName(activeSlider);
		long loVal = (int)(project->SliderDefault(curSlider, false));
		long hiVal = (int)(project->SliderDefault(curSlider, true));
		XRCCTRL(dlg, "edSliderName", wxTextCtrl)->SetLabel(activeSlider);
		tmpstr.sprintf("%d", loVal);
		XRCCTRL(dlg, "edValLo", wxTextCtrl)->SetValue(tmpstr);
		tmpstr.sprintf("%d", hiVal);
		XRCCTRL(dlg, "edValHi", wxTextCtrl)->SetValue(tmpstr);
		if (project->SliderHidden(curSlider))
			XRCCTRL(dlg, "chkHidden", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		if (project->SliderInvert(curSlider))
			XRCCTRL(dlg, "chkInvert", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		if (project->SliderZap(curSlider))
			XRCCTRL(dlg, "chkZap", wxCheckBox)->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		XRCCTRL(dlg, "wxID_CANCEL", wxButton)->SetFocus();

		if (dlg.ShowModal() == wxID_OK) {
			project->SetSliderZap(curSlider, XRCCTRL(dlg, "chkZap", wxCheckBox)->GetValue());
			project->SetSliderInvert(curSlider, XRCCTRL(dlg, "chkInvert", wxCheckBox)->GetValue());
			project->SetSliderHidden(curSlider, XRCCTRL(dlg, "chkHidden", wxCheckBox)->GetValue());
			XRCCTRL(dlg, "edValLo", wxTextCtrl)->GetValue().ToLong(&loVal);
			project->SetSliderDefault(curSlider, loVal, false);
			XRCCTRL(dlg, "edValHi", wxTextCtrl)->GetValue().ToLong(&hiVal);
			project->SetSliderDefault(curSlider, hiVal, true);
			tmpstr = XRCCTRL(dlg, "edSliderName", wxTextCtrl)->GetValue();
			sstr = tmpstr.ToAscii().data();

			if (activeSlider != sstr) {
				project->SetSliderName(curSlider, sstr);
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
	project->OutfitShapes(shapes);
	if (shapes.size() == 0)
		return;
	if (!project->baseNif.IsValid())
		return;

	StartProgress("Conforming all shapes...");
	float inc = 100.0f / shapes.size();
	float pos = 0.0f;

	auto selectedItemsSave = selectedItems;

	curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		selectedItems.clear();
		selectedItems.push_back((ShapeItemData*)outfitShapes->GetItemData(curItem));
		UpdateProgress(pos * inc, "Conforming: " + activeItem->shapeName);
		StartSubProgress(pos * inc, pos * inc + inc);
		OnSliderConform(event);
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
		pos++;
	}

	selectedItems = selectedItemsSave;

	statusBar->SetStatusText("All shapes conformed.");
	UpdateProgress(100.0f, "Finished");
	EndProgress();
}

void OutfitStudio::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (!project->baseNif.IsValid())
		return;

	StartProgress("Conforming...");
	ZeroSliders();

	for (auto i : selectedItems) {
		if (!i->bIsOutfitShape) {
			wxMessageBox("You can't conform the reference shape to itself! Skipping this shape.", "Warning", wxICON_WARNING);
			continue;
		}

		UpdateProgress(1.0f, "Initializing data...");
		project->InitConform();

		if (IsDirty(i->shapeName)) {
			UpdateShapeSource(i->shapeName, true);
			project->RefreshMorphOutfitShape(i->shapeName);
		}
		UpdateProgress(50.0f, "Conforming: " + i->shapeName);

		project->morpher.CopyMeshMask(glView->GetMesh(i->shapeName), i->shapeName);
		project->ConformShape(i->shapeName);

		statusBar->SetStatusText("Shape(s) conformed.");
		UpdateProgress(99.0f);
	}

	UpdateProgress(100.0f, "Finished");
	EndProgress();
}

void OutfitStudio::OnImportShape(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector("Import .obj file for new shape", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	int ret = project->AddShapeFromObjFile(fn, "New Shape", activeItem->shapeName);
	if (ret == 101) {	// User chose to merge shapes
		vector<Vector3> v;
		project->GetLiveOutfitVerts(activeItem->shapeName, v);
		glView->UpdateMeshVertices(activeItem->shapeName, &v);
		return;
	}
	else if (ret)
		return;

	WorkingGUIFromProj();
	outfitShapes->ExpandAll();
	glView->Refresh();
}

void OutfitStudio::OnExportShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector("Export selected shapes as .obj files", wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		project->ClearBoneScale();
		for (auto i : selectedItems)
			project->ExportShape(i->shapeName, dir + "\\" + i->shapeName + ".obj", i->bIsOutfitShape);
	}
	else {
		string fname = wxFileSelector("Export shape as an .obj file", wxEmptyString, wxString(activeItem->shapeName + ".obj"), "", "Obj Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (!fname.empty()) {
			project->ClearBoneScale();
			project->ExportShape(activeItem->shapeName, fname, activeItem->bIsOutfitShape);
		}
	}
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
	} while ((project->IsValidShape(newShapeName)));

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	for (uint i = 0; i < sizeof(chars); ++i)
		replace(newShapeName.begin(), newShapeName.end(), chars[i], '_');

	project->RenameShape(activeItem->shapeName, newShapeName, activeItem->bIsOutfitShape);
	glView->RenameShape(activeItem->shapeName, newShapeName);

	activeItem->shapeName = newShapeName;
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
	Vector3 offs;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMoveShape")) {
		XRCCTRL(dlg, "msOldOffset", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnMoveShapeOldOffset, this);
		XRCCTRL(dlg, "msX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnMoveShapeText, this);
		XRCCTRL(dlg, "msTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnMoveShapeText, this);
		XRCCTRL(dlg, "msTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnMoveShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			offs.x = atof(XRCCTRL(dlg, "msTextX", wxTextCtrl)->GetValue().ToAscii().data());
			offs.y = atof(XRCCTRL(dlg, "msTextY", wxTextCtrl)->GetValue().ToAscii().data());
			offs.z = atof(XRCCTRL(dlg, "msTextZ", wxTextCtrl)->GetValue().ToAscii().data());
		}

		unordered_map<ushort, float> mask;
		unordered_map<ushort, float>* mptr = nullptr;
		unordered_map<ushort, Vector3> diff;
		for (auto i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			Vector3 d;
			vector<Vector3> verts;
			float diffY, diffZ;
			project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);

			if (bEditSlider) {
				diff.clear();
				for (int i = 0; i < verts.size(); i++) {
					d = (offs - previewMove) * (1.0f - mask[i]);
					diff[i] = d;
					diffY = diff[i].y / 10.0f;
					diffZ = diff[i].z / 10.0f;
					diff[i].z = diffY;
					diff[i].y = diffZ;
					verts[i] += d;
				}
				project->UpdateMorphResult(i->shapeName, activeSlider, diff, i->bIsOutfitShape);
			}
			else {
				d = offs - previewMove;
				project->OffsetShape(i->shapeName, d, i->bIsOutfitShape, mptr);
			}

			project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}
		previewMove.Zero();
	}
}

void OutfitStudio::OnMoveShapeOldOffset(wxCommandEvent& event) {
	wxWindow* parent = ((wxButton*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	XRCCTRL(*parent, "msX", wxSlider)->SetValue(0);
	XRCCTRL(*parent, "msY", wxSlider)->SetValue(-2544);
	XRCCTRL(*parent, "msZ", wxSlider)->SetValue(3287);
	XRCCTRL(*parent, "msTextX", wxTextCtrl)->SetValue("0.00000");
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->SetValue("-2.54431");
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->SetValue("3.28790");

	OnPreviewMove(event);
}

void OutfitStudio::OnMoveShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "msX", wxSlider)->GetValue() / 1000.0f;
	slider.y = XRCCTRL(*parent, "msY", wxSlider)->GetValue() / 1000.0f;
	slider.z = XRCCTRL(*parent, "msZ", wxSlider)->GetValue() / 1000.0f;

	XRCCTRL(*parent, "msTextX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.x));
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.y));
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.z));

	OnPreviewMove(event);
}

void OutfitStudio::OnMoveShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;
	
	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "msTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "msTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "msTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	XRCCTRL(*parent, "msX", wxSlider)->SetValue(changed.x * 1000);
	XRCCTRL(*parent, "msY", wxSlider)->SetValue(changed.y * 1000);
	XRCCTRL(*parent, "msZ", wxSlider)->SetValue(changed.z * 1000);

	OnPreviewMove(event);
}

void OutfitStudio::OnPreviewMove(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "msTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "msTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "msTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	unordered_map<ushort, Vector3> diff;
	for (auto i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		Vector3 d;
		vector<Vector3> verts;
		float diffY, diffZ;
		project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
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
			project->UpdateMorphResult(i->shapeName, activeSlider, diff, i->bIsOutfitShape);
		}
		else {
			d = changed - previewMove;
			project->OffsetShape(i->shapeName, d, i->bIsOutfitShape, mptr);
		}

		project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}
	previewMove = changed;
}

void OutfitStudio::OnOffsetShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<Vector3> verts;
	Vector3 offs;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (activeItem->bIsOutfitShape)
		offs = project->workNif.GetShapeVirtualOffset(activeItem->shapeName);
	else
		offs = project->baseNif.GetShapeVirtualOffset(activeItem->shapeName);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgOffsetShape")) {
		XRCCTRL(dlg, "osX", wxSlider)->SetValue(offs.x * 1000);
		XRCCTRL(dlg, "osY", wxSlider)->SetValue(offs.y * 1000);
		XRCCTRL(dlg, "osZ", wxSlider)->SetValue(offs.z * 1000);
		XRCCTRL(dlg, "osTextX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", offs.x));
		XRCCTRL(dlg, "osTextY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", offs.y));
		XRCCTRL(dlg, "osTextZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", offs.z));
		
		XRCCTRL(dlg, "osX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnOffsetShapeSlider, this);
		XRCCTRL(dlg, "osY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnOffsetShapeSlider, this);
		XRCCTRL(dlg, "osZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnOffsetShapeSlider, this);
		XRCCTRL(dlg, "osTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnOffsetShapeText, this);
		XRCCTRL(dlg, "osTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnOffsetShapeText, this);
		XRCCTRL(dlg, "osTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnOffsetShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			offs.x = atof(XRCCTRL(dlg, "osTextX", wxTextCtrl)->GetValue().ToAscii().data());
			offs.y = atof(XRCCTRL(dlg, "osTextY", wxTextCtrl)->GetValue().ToAscii().data());
			offs.z = atof(XRCCTRL(dlg, "osTextZ", wxTextCtrl)->GetValue().ToAscii().data());
		}

		if (activeItem->bIsOutfitShape)
			project->workNif.VirtualOffsetShape(activeItem->shapeName, offs, false);
		else
			project->baseNif.VirtualOffsetShape(activeItem->shapeName, offs, false);

		project->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	}
}

void OutfitStudio::OnOffsetShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "osX", wxSlider)->GetValue() / 1000.0f;
	slider.y = XRCCTRL(*parent, "osY", wxSlider)->GetValue() / 1000.0f;
	slider.z = XRCCTRL(*parent, "osZ", wxSlider)->GetValue() / 1000.0f;

	XRCCTRL(*parent, "osTextX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.x));
	XRCCTRL(*parent, "osTextY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.y));
	XRCCTRL(*parent, "osTextZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.z));

	OnPreviewOffset(event);
}

void OutfitStudio::OnOffsetShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;
	
	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "osTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "osTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "osTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	XRCCTRL(*parent, "osX", wxSlider)->SetValue(changed.x * 1000);
	XRCCTRL(*parent, "osY", wxSlider)->SetValue(changed.y * 1000);
	XRCCTRL(*parent, "osZ", wxSlider)->SetValue(changed.z * 1000);

	OnPreviewOffset(event);
}

void OutfitStudio::OnPreviewOffset(wxCommandEvent& event) {
	vector<Vector3> verts;
	Vector3 current, changed;
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	if (activeItem->bIsOutfitShape)
		current = project->workNif.GetShapeVirtualOffset(activeItem->shapeName);
	else
		current = project->baseNif.GetShapeVirtualOffset(activeItem->shapeName);

	changed.x = atof(XRCCTRL(*parent, "osTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "osTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "osTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	if (activeItem->bIsOutfitShape)
		project->workNif.VirtualOffsetShape(activeItem->shapeName, changed, false);
	else
		project->baseNif.VirtualOffsetShape(activeItem->shapeName, changed, false);

	project->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
}

void OutfitStudio::OnScaleShape(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		XRCCTRL(dlg, "scaleValue", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnPreviewScale, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		float scale;
		if (dlg.ShowModal() == wxID_OK)
			scale = atof(XRCCTRL(dlg, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
		else
			scale = 1.0f;

		scale *= 1.0f / previewScale;

		unordered_map<ushort, float> mask;
		unordered_map<ushort, float>* mptr = nullptr;
		for (auto i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			vector<Vector3> verts;
			project->ScaleShape(i->shapeName, scale, i->bIsOutfitShape, mptr);
			project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewScale = 1.0f;
	}
}

void OutfitStudio::OnPreviewScale(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	float scale = atof(XRCCTRL(*parent, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
	if (scale == 0.0f)
		return;

	float scaleNew = scale * (1.0f / previewScale);

	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	for (auto i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		vector<Vector3> verts;
		project->ScaleShape(i->shapeName, scaleNew, i->bIsOutfitShape, mptr);
		project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewScale = scale;
}

void OutfitStudio::OnVirtScaleShape(wxCommandEvent& event) {
	wxDialog dlg;
	vector<Vector3> verts;
	float scale;
	bool fromCenter;
	//Vector3 centerpoint;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (activeItem->bIsOutfitShape)
		project->workNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);
	else
		project->baseNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);

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
			project->workNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);
		else
			project->baseNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);

		project->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
		glView->UpdateMeshVertices(activeItem->shapeName, &verts);
	}
}

void OutfitStudio::OnPreviewVirtScale(wxCommandEvent& event) {
	vector<Vector3> verts;
	float scale;
	bool fromCenter;

	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	if (activeItem->bIsOutfitShape)
		project->workNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);
	else
		project->baseNif.GetShapeVirtualScale(activeItem->shapeName, scale, fromCenter);

	scale = atof(XRCCTRL(*parent, "scaleValue", wxTextCtrl)->GetValue().ToAscii().data());
	if (scale == 0.0f)
		return;

	fromCenter = true;
	if (activeItem->bIsOutfitShape)
		project->workNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);
	else
		project->baseNif.VirtualScaleShape(activeItem->shapeName, scale, fromCenter);

	project->GetLiveVerts(activeItem->shapeName, verts, activeItem->bIsOutfitShape);
	glView->UpdateMeshVertices(activeItem->shapeName, &verts);
}

void OutfitStudio::OnRotateShape(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotateShape")) {
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			float angleX = atof(XRCCTRL(dlg, "angleX", wxTextCtrl)->GetValue().ToAscii().data());
			float angleY = atof(XRCCTRL(dlg, "angleY", wxTextCtrl)->GetValue().ToAscii().data());
			float angleZ = atof(XRCCTRL(dlg, "angleZ", wxTextCtrl)->GetValue().ToAscii().data());
			Vector3 angle(angleX, angleY, angleZ);

			unordered_map<ushort, float> mask;
			unordered_map<ushort, float>* mptr = nullptr;
			for (auto i : selectedItems) {
				mask.clear();
				mptr = nullptr;
				glView->GetShapeMask(mask, i->shapeName);
				if (mask.size() > 0)
					mptr = &mask;

				vector<Vector3> verts;
				project->RotateShape(i->shapeName, angle, i->bIsOutfitShape, mptr);
				project->GetLiveVerts(i->shapeName, verts, i->bIsOutfitShape);
				glView->UpdateMeshVertices(i->shapeName, &verts);
			}
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
				if (!project->workNif.GetTextureForShape(activeItem->shapeName, texpath, i))
					continue;
			}
			else {
				if (!project->baseNif.GetTextureForShape(activeItem->shapeName, texpath, i))
					continue;
			}
			stTexGrid->SetCellValue(texpath, i, 0);
		}

		oDispPath = project->OutfitTexture(activeItem->shapeName);
		XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->SetPath(oDispPath);
		XRCCTRL(dlg, "btApplyDiffuse", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnApplyDiffuse, this);

		if (dlg.ShowModal() == wxID_OK) {
			nDispPath = XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->GetPath();
			if (nDispPath != oDispPath) {
				if (activeItem->bIsOutfitShape) {
					project->SetOutfitTexture(activeItem->shapeName, nDispPath);
					glView->SetMeshTexture(activeItem->shapeName, nDispPath, project->OutfitShapeShaderType(activeItem->shapeName));
				}
				else {
					project->SetRefTexture(activeItem->shapeName, nDispPath);
					glView->SetMeshTexture(activeItem->shapeName, nDispPath, project->RefShapeShaderType(activeItem->shapeName));
				}
			}

			for (int i = 0; i < 9; i++) {
				texpath = stTexGrid->GetCellValue(i, 0);
				if (activeItem->bIsOutfitShape) {
					project->workNif.SetTextureForShape(activeItem->shapeName, texpath, i);
					project->workNif.TrimTexturePaths();
				}
				else {
					project->baseNif.SetTextureForShape(activeItem->shapeName, texpath, i);
					project->baseNif.TrimTexturePaths();
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

void OutfitStudio::OnDupeShape(wxCommandEvent& WXUNUSED(event)) {
	string newname;
	wxTreeItemId subitem;
	if (activeItem) {
		if (!outfitRoot.IsOk()) {
			wxMessageBox("You can only copy shapes into an outfit, and there is no outfit in the current project. Load one first!");
			return;
		}

		do {
			wxString result = wxGetTextFromUser("Please enter a unique name for the duplicated shape.", "Duplicate Shape");
			if (result.empty())
				return;
			newname = result;
		} while (project->IsValidShape(newname));

		project->ClearBoneScale();
		mesh* curshapemesh = glView->GetMesh(activeItem->shapeName);

		if (activeItem->bIsOutfitShape)
			project->DuplicateOutfitShape(activeItem->shapeName, newname, curshapemesh);
		else
			project->DuplicateRefShape(activeItem->shapeName, newname, curshapemesh);

		glView->AddMeshFromNif(&project->workNif, newname);
		project->SetOutfitTexture(newname, "_AUTO_");

		glView->SetMeshTexture(newname, project->OutfitTexture(newname), project->OutfitShapeShaderType(newname));

		subitem = outfitShapes->AppendItem(outfitRoot, newname);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(true, &project->workNif, newname));
	}
}

void OutfitStudio::OnDeleteShape(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxMessageBox("Are you sure you wish to delete the selected shapes?  This action cannot be undone.", "Confirm Delete", wxYES_NO) == wxNO)
		return;

	vector<ShapeItemData> selected;
	for (auto i : selectedItems)
		selected.push_back(*i);

	for (auto i : selected) {
		if (i.bIsOutfitShape) {
			project->DeleteOutfitShape(i.shapeName);
			glView->DeleteMesh(i.shapeName);
			wxTreeItemId item = i.GetId();
			outfitShapes->Delete(item);
		}
		else {
			wxMessageBox("You can't delete the reference shape! Skipping this shape.", "Warning", wxICON_WARNING);
		}
	}

	AnimationGUIFromProj();
}

void OutfitStudio::OnAddBone(wxCommandEvent& event) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones"))
		return;

	dlg.SetSize(450, 470);

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	function<void(wxTreeItemId, AnimBone*)> fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
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

			project->AddBoneRef(t);
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

		project->DeleteBone(bname);
		activeBone = "";

		outfitBones->Delete(selItems[i]);
	}

	outfitBones->GetSelections(selItems);
	if (selItems.size() > 0) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems[0]);
		OnOutfitBoneSelect(treeEvent);
	}
}

void OutfitStudio::OnCopyBoneWeight(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!project->baseNif.IsValid())
		return;

	StartProgress("Copying bone weights...");

	unordered_map<ushort, float> mask;
	for (int i = 0; i < selectedItems.size(); i++) {
		if (selectedItems[i]->bIsOutfitShape) {
			mask.clear();
			glView->GetShapeMask(mask, selectedItems[i]->shapeName);
			project->CopyBoneWeights(selectedItems[i]->shapeName, &mask);
		}
		else
			wxMessageBox("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape.", "Can't copy weights", wxICON_WARNING);
	}

	UpdateProgress(100.0f, "Finished");
	EndProgress();
}

void OutfitStudio::OnCopySelectedWeight(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!project->baseNif.IsValid())
		return;

	vector<string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	for (int i = 0; i < selItems.size(); i++)
		selectedBones.push_back(string(outfitBones->GetItemText(selItems[i])));

	StartProgress("Copying selected bone weights...");

	unordered_map<ushort, float> mask;
	for (int i = 0; i < selectedItems.size(); i++) {
		if (selectedItems[i]->bIsOutfitShape) {
			mask.clear();
			glView->GetShapeMask(mask, selectedItems[i]->shapeName);
			project->CopyBoneWeights(selectedItems[i]->shapeName, &mask, &selectedBones);
		}
		else
			wxMessageBox("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape.", "Can't copy weights", wxICON_WARNING);
	}

	UpdateProgress(100.0f, "Finished");
	EndProgress();
}

void OutfitStudio::OnTransferSelectedWeight(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!project->baseNif.IsValid())
		return;

	if (!activeItem->bIsOutfitShape) {
		wxMessageBox("Sorry, you can't copy weights from the reference shape to itself.", "Error");
		return;
	}

	int baseVertCount = project->baseNif.GetVertCountForShape(project->baseShapeName);
	int workVertCount = project->workNif.GetVertCountForShape(activeItem->shapeName);
	if (baseVertCount != workVertCount) {
		wxMessageBox("The vertex count of the reference and chosen shape is not the same!", "Error");
		return;
	}

	vector<string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	for (int i = 0; i < selItems.size(); i++)
		selectedBones.push_back(string(outfitBones->GetItemText(selItems[i])));

	StartProgress("Transferring bone weights...");

	unordered_map<ushort, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->shapeName, &mask, &selectedBones);

	EndProgress();
}

void OutfitStudio::OnMaskWeighted(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	vector<string> bones;
	project->RefBones(bones);
	unordered_map<ushort, float> boneWeights;

	for (auto i : selectedItems) {
		mesh* m = glView->GetMesh(i->shapeName);
		if (!m)
			continue;

		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));
		for (auto b : bones) {
			boneWeights.clear();
			if (i->bIsOutfitShape)
				project->workAnim.GetWeights(i->shapeName, b, boneWeights);
			else
				project->baseAnim.GetWeights(i->shapeName, b, boneWeights);
			for (auto bw : boneWeights)
				m->vcolors[bw.first].x = 1.0f;
		}
	}

	glView->Refresh();
}

void OutfitStudio::OnBuildSkinPartitions(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	for (auto i : selectedItems)
		project->BuildShapeSkinPartions(i->shapeName, i->bIsOutfitShape);
}

// ---------------------------------------------------------------------------
// wxGLPanel
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxGLPanel, wxGLCanvas)
	EVT_PAINT(wxGLPanel::OnPaint)
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

wxGLPanel::wxGLPanel(wxWindow* parent, const wxSize& size, const int* attribs) : wxGLCanvas(parent, wxID_ANY, attribs, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE) {
	context = new wxGLContext(this);
	rbuttonDown = false;
	lbuttonDown = false;
	mbuttonDown = false;
	isLDragging = false;
	isRDragging = false;
	isMDragging = false;

	lastX = 0;
	lastY = 0;

	brushSize = 0.45f;
	activeBrush = nullptr;
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

wxGLPanel::~wxGLPanel() {
	delete context;
}

void wxGLPanel::OnShown() {
	gls.Initialize(this, context, false);
	auto size = GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), size.GetWidth(), size.GetHeight(), 65.0f);
	gls.ToggleMask();
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	notifyWindow = win;
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, const string& shapeName) {
	vector<string> shapeList;
	nif->GetShapeList(shapeList);

	for (int i = 0; i < shapeList.size(); i++) {
		if (!shapeName.empty() && (shapeList[i] == shapeName))
			gls.AddMeshFromNif(nif, shapeList[i]);
		else if (!shapeName.empty())
			continue;
		else
			gls.AddMeshFromNif(nif, shapeList[i]);

		gls.GetMesh(shapeList[i])->BuildTriAdjacency();
		gls.GetMesh(shapeList[i])->BuildEdgeList();
		gls.GetMesh(shapeList[i])->ColorFill(Vector3());
		RecalcNormals(shapeList[i]);
	}
}

void wxGLPanel::AddExplicitMesh(vector<Vector3>* v, vector<Triangle>* t, vector<Vector2>* uv, const string& shapeName) {
	gls.AddMeshExplicit(v, t, uv, shapeName);
	gls.GetMesh(shapeName)->BuildTriAdjacency();
	gls.GetMesh(shapeName)->BuildEdgeList();
	gls.GetMesh(shapeName)->ColorFill(Vector3());
	RecalcNormals(shapeName);
}

void wxGLPanel::SetMeshTexture(const string& shapeName, const string& texturefile, int shaderType) {
	mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	GLMaterial* mat;
	if (shaderType == 0)
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\defshader.fs");
	else
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\skinshader.fs");

	m->material = mat;
}

void wxGLPanel::UpdateMeshVertices(const string& shapeName, vector<Vector3>* verts, bool updateBVH) {
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
		activeBrush = &weightBrush;
		bWeightPaint = true;
		break;
	}
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	if (event.GetUnicodeKey() == 'V') {
		wxDialog dlg;
		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		wxPoint cursorPos(event.GetPosition());

		unordered_map<ushort, Vector3> diff;
		vector<Vector3> verts;
		Vector3 newPos;
		Vertex oldVertex;

		if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &oldVertex))
			return;

		if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
			os->project->GetLiveVerts(os->activeItem->shapeName, verts, os->activeItem->bIsOutfitShape);
			XRCCTRL(dlg, "posX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].x));
			XRCCTRL(dlg, "posY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].y));
			XRCCTRL(dlg, "posZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[oldVertex.indexRef].z));

			if (dlg.ShowModal() == wxID_OK) {
				newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().ToAscii().data());
				newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().ToAscii().data());
				newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().ToAscii().data());

				if (os->bEditSlider) {
					diff[oldVertex.indexRef] = newPos - verts[oldVertex.indexRef];
					float diffY = diff[oldVertex.indexRef].y / 10.0f;
					float diffZ = diff[oldVertex.indexRef].z / 10.0f;
					diff[oldVertex.indexRef].z = diffY;
					diff[oldVertex.indexRef].y = diffZ;
					verts[oldVertex.indexRef] = newPos;
					os->project->UpdateMorphResult(os->activeItem->shapeName, os->activeSlider, diff, os->activeItem->bIsOutfitShape);
				}
				else {
					os->project->MoveVertex(os->activeItem->shapeName, newPos, oldVertex.indexRef, os->activeItem->bIsOutfitShape);
				}
				os->project->GetLiveVerts(os->activeItem->shapeName, verts, os->activeItem->bIsOutfitShape);
				UpdateMeshVertices(os->activeItem->shapeName, &verts);
			}
		}
	}
	event.Skip();
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	OutfitStudio* os = (OutfitStudio*)notifyWindow;
	Vector3 o;
	Vector3 n;
	Vector3 v;
	Vector3 vo;
	Vector3 d;
	Vector3 s;
	bool meshHit;

	TweakPickInfo tpi;
	meshHit = gls.CollideMesh(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &tpi.facet);
	if (!meshHit)
		return false;

	if (!os->NotifyStrokeStarting())
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
	gls.GetPickRay(screenPos.x, screenPos.y, v, vo);
	v = v * -1;
	tpi.view = v;

	//activeStroke=strokeManager.CreateStroke(gls.GetActiveMesh(), &tweakBrush);
	//tweakBrush.setMirror(true);
	savedBrush = activeBrush;

	if (wxGetKeyState(WXK_CONTROL)) {
		if (wxGetKeyState(WXK_ALT)) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		if (wxGetKeyState(WXK_ALT)) {
			unweightBrush.refBone = os->GetActiveBone();
			unweightBrush.setStrength(-weightBrush.getStrength());
			activeBrush = &unweightBrush;
		}
		else if (wxGetKeyState(WXK_SHIFT)) {
			smoothWeightBrush.refBone = os->GetActiveBone();
			smoothWeightBrush.setStrength(weightBrush.getStrength() * 15.0f);
			activeBrush = &smoothWeightBrush;
		}
		else {
			weightBrush.refBone = os->GetActiveBone();
		}
	}
	else if (wxGetKeyState(WXK_ALT)) {
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
	else if (activeBrush != &weightBrush && wxGetKeyState(WXK_SHIFT)) {
		activeBrush = &smoothBrush;
	}

	if (activeBrush->Type() == TBT_WEIGHT && os->IsDirty())
		if (os->PromptUpdateBase() == wxCANCEL)
			return false;

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMesh(), activeBrush);
	activeBrush->setConnected(bConnectedEdit);
	activeBrush->setMirror(bXMirror);
	activeBrush->setRadius(brushSize);
	activeBrush->setLiveNormals(bAutoNormals);
	activeStroke->beginStroke(tpi);
	activeStroke->updateStroke(tpi);
	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	Vector3 o;
	Vector3 n;
	Vector3 v;
	Vector3 vo;

	Vector3 d;	// Mirror pick ray direction.
	Vector3 s;	// Mirror pick ray origin.

	TweakPickInfo tpi;

	if (activeStroke) {
		wxRect r = this->GetClientRect();
		int cx = r.GetWidth() / 2;
		int cy = r.GetHeight() / 2;
		gls.GetPickRay(cx, cy, v, vo);

		gls.UpdateCursor(screenPos.x, screenPos.y);

		if (activeBrush->Type() == TBT_MOVE)
		{
			Vector3 pn;
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

		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		if (activeBrush->Type() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = ((wxSlider*)FindWindowByName("boneScale"))->GetValue();
				os->ActiveShapeUpdated(strokeManager->GetCurStateStroke());
				os->project->ApplyBoneScale(selectedBone, boneScalePos);
			}
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (activeStroke) {
		activeStroke->endStroke();

		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		if (activeStroke->BrushType() != TBT_MASK)
			os->ActiveShapeUpdated(strokeManager->GetCurStateStroke());

		if (activeStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = ((wxSlider*)FindWindowByName("boneScale"))->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos);
			}
		}

		activeStroke = nullptr;
		activeBrush = savedBrush;
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
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
			tpi.view = Vector3(1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = Vector3(0.0f, 0.0f, 1.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		}
	}
	else if (mname.find("Rotate") != string::npos) {
		translateBrush.SetXFormType(1);
		activeBrush = &translateBrush;
		switch (mname[0]) {
		case 'X':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), -tpi.center.x);
			//tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		case 'Y':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), -tpi.center.y);
			//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 1.0f, 0.0f);
			break;
		case 'Z':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), -tpi.center.z);
			//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		}
	}
	else if (mname.find("Scale") != string::npos) {
		translateBrush.SetXFormType(2);
		activeBrush = &translateBrush;
		switch (mname[0]) {
		case 'X':
			tpi.view = Vector3(1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = Vector3(0.0f, 0.0f, 1.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		}
	}

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMesh(), activeBrush);

	//gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin,tpi.normal,0.0f);
	activeStroke->beginStroke(tpi);
	activeStroke->updateStroke(tpi);

	ShowTransformTool(false, false);

	return true;
}

void wxGLPanel::UpdateTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	((TB_XForm*)(activeBrush))->GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, -pd);
	if (tpi.origin.x < 0)
		tpi.origin.x = tpi.origin.x;

	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndTransform() {
	activeStroke->endStroke();
	activeStroke = nullptr;
	ShowTransformTool(true, false);
	((OutfitStudio*)notifyWindow)->ActiveShapeUpdated(strokeManager->GetCurStateStroke());
}

bool wxGLPanel::UndoStroke() {
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	mesh* m = strokeManager->GetCurStateMesh();
	bool skip = (!gls.IsValidMesh(m));
	bool ret = strokeManager->backStroke(skip);
	if (!skip && ret) {
		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		if (curStroke && curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapeUpdated(curStroke, true);
			if (curStroke->BrushType() == TBT_XFORM)
				ShowTransformTool(true, false);
		}
		if (curStroke && curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = ((wxSlider*)FindWindowByName("boneScale"))->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos);
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
	if (!skip && ret) {
		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapeUpdated(curStroke);
			if (curStroke->BrushType() == TBT_XFORM)
				ShowTransformTool(true, false);
		}
		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = ((wxSlider*)FindWindowByName("boneScale"))->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos);
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

	static TweakBrush* saveBrush = nullptr;
	if (updateBrush)
		saveBrush = activeBrush;

	Vector3 o;
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

		xform_move = gls.AddVis3dArrow(o, Vector3(1.0f, 0.0f, 0.0f), 0.02f, 0.1f, 1.5f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		gls.GetOverlay("XMoveMesh")->CreateBVH();
		yform_move = gls.AddVis3dArrow(o, Vector3(0.0f, 1.0f, 0.0f), 0.02f, 0.1f, 1.5f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		gls.GetOverlay("YMoveMesh")->CreateBVH();
		zform_move = gls.AddVis3dArrow(o, Vector3(0.0f, 0.0f, 1.0f), 0.02f, 0.1f, 1.5f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");
		gls.GetOverlay("ZMoveMesh")->CreateBVH();
		gls.AddVis3dRing(o, Vector3(1.0f, 0.0f, 0.0f), 1.0f, 0.03f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		gls.GetOverlay("XRotateMesh")->CreateBVH();
		gls.AddVis3dRing(o, Vector3(0.0f, 1.0f, 0.0f), 1.0f, 0.03f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		gls.GetOverlay("YRotateMesh")->CreateBVH();
		gls.AddVis3dRing(o, Vector3(0.0f, 0.0f, 1.0f), 1.0f, 0.03f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");
		gls.GetOverlay("ZRotateMesh")->CreateBVH();
	}

	else {
		if (updateBrush) {
			activeBrush = saveBrush;
			if (!activeBrush)
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
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		OnShown();
	}

	gls.RenderOneFrame();
	event.Skip();
}

void wxGLPanel::OnSize(wxSizeEvent& event) {
	wxSize sz = event.GetSize();
	gls.SetSize(sz.GetX(), sz.GetY());
	Refresh();
}

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	if (wxGetKeyState(wxKeyCode('S')))  {
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
		if (wxGetKeyState(WXK_SHIFT)) {
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
			if (Config.MatchValue("Input/LeftMousePan", "true"))
				gls.PanCamera(x - lastX, y - lastY);
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

		OutfitStudio* os = (OutfitStudio*)notifyWindow;
		if (cursorExists) {
			if (bWeightPaint)
				os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g", t, w), 1);
			else if (bMaskPaint)
				os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", t, m), 1);
			else {
				vector<Vector3> verts;
				os->project->GetLiveVerts(os->activeItem->shapeName, verts, os->activeItem->bIsOutfitShape);
				os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", t, verts[t].x, verts[t].y, verts[t].z), 1);
			}
		}
		else {
			os->statusBar->SetStatusText("", 1);
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
			((OutfitStudio*)notifyWindow)->SelectShape(gls.GetMeshName(meshID));
		}
	}
	if (isPainting){
		EndBrushStroke();
		isPainting = false;
	}
	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	isLDragging = false;
	lbuttonDown = false;
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPainting) {
		EndBrushStroke();
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
