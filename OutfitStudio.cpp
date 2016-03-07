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

#include "OutfitStudio.h"
#include "BodySlideApp.h"
#include "ShapeProperties.h"
#include <functional>

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(OutfitStudio, wxFrame)
	EVT_MENU(XRCID("fileExit"), OutfitStudio::OnExit)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudio::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudio::OnLoadProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudio::OnLoadReference)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudio::OnLoadOutfit)
	EVT_MENU(XRCID("fileSave"), OutfitStudio::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudio::OnSaveSliderSetAs)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("brushPane"), OutfitStudio::OnBrushPane)
	EVT_COMMAND_SCROLL(XRCID("brushSize"), OutfitStudio::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushStr"), OutfitStudio::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushFocus"), OutfitStudio::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushSpace"), OutfitStudio::OnBrushSettingsSlider)
	
	EVT_TEXT_ENTER(wxID_ANY, OutfitStudio::OnReadoutChange)
	EVT_COMMAND_SCROLL(wxID_ANY, OutfitStudio::OnSlider)
	EVT_BUTTON(wxID_ANY, OutfitStudio::OnClickSliderButton)
	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudio::OnSelectSliders)
	EVT_CHECKBOX(XRCID("cbFixedWeight"), OutfitStudio::OnFixedWeight)
	EVT_CHECKBOX(wxID_ANY, OutfitStudio::OnCheckBox)

	EVT_MENU(XRCID("saveBaseShape"), OutfitStudio::OnSetBaseShape)
	EVT_MENU(XRCID("exportOutfitNif"), OutfitStudio::OnExportOutfitNif)
	EVT_MENU(XRCID("exportOutfitNifWithRef"), OutfitStudio::OnExportOutfitNifWithRef)
	EVT_MENU(XRCID("makeConvRef"), OutfitStudio::OnMakeConvRef)
	
	EVT_MENU(XRCID("sliderLoadPreset"), OutfitStudio::OnLoadPreset)
	EVT_MENU(XRCID("sliderSavePreset"), OutfitStudio::OnSavePreset)
	EVT_MENU(XRCID("sliderConform"), OutfitStudio::OnSliderConform)
	EVT_MENU(XRCID("sliderConformAll"), OutfitStudio::OnSliderConformAll)
	EVT_MENU(XRCID("sliderImportBSD"), OutfitStudio::OnSliderImportBSD)
	EVT_MENU(XRCID("sliderImportOBJ"), OutfitStudio::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderImportFBX"), OutfitStudio::OnSliderImportFBX)
	EVT_MENU(XRCID("sliderImportTRI"), OutfitStudio::OnSliderImportTRI)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudio::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderExportOBJ"), OutfitStudio::OnSliderExportOBJ)
	EVT_MENU(XRCID("sliderExportTRI"), OutfitStudio::OnSliderExportTRI)
	EVT_MENU(XRCID("sliderNew"), OutfitStudio::OnNewSlider)
	EVT_MENU(XRCID("sliderNewZap"), OutfitStudio::OnNewZapSlider)
	EVT_MENU(XRCID("sliderNewCombined"), OutfitStudio::OnNewCombinedSlider)
	EVT_MENU(XRCID("sliderNegate"), OutfitStudio::OnSliderNegate)
	EVT_MENU(XRCID("sliderClear"), OutfitStudio::OnClearSlider)
	EVT_MENU(XRCID("sliderDelete"), OutfitStudio::OnDeleteSlider)
	EVT_MENU(XRCID("sliderProperties"), OutfitStudio::OnSliderProperties)
	
	EVT_MENU(XRCID("btnXMirror"), OutfitStudio::OnXMirror)
	EVT_MENU(XRCID("btnConnected"), OutfitStudio::OnConnectedOnly)
	EVT_MENU(XRCID("btnBrushCollision"), OutfitStudio::OnGlobalBrushCollision)
	
	EVT_MENU(XRCID("btnSelect"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnMaskBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnInflateBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnDeflateBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnMoveBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnSmoothBrush"), OutfitStudio::OnSelectBrush)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudio::OnSelectBrush)

	EVT_MENU(XRCID("btnViewFront"), OutfitStudio::OnSetView)
	EVT_MENU(XRCID("btnViewBack"), OutfitStudio::OnSetView)
	EVT_MENU(XRCID("btnViewLeft"), OutfitStudio::OnSetView)
	EVT_MENU(XRCID("btnViewRight"), OutfitStudio::OnSetView)
	EVT_MENU(XRCID("btnViewPerspective"), OutfitStudio::OnTogglePerspective)
	
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
	EVT_MENU(XRCID("importFBX"), OutfitStudio::OnImportFBX)
	EVT_MENU(XRCID("exportFBX"), OutfitStudio::OnExportFBX)
	EVT_MENU(XRCID("uvInvertX"), OutfitStudio::OnInvertUV)
	EVT_MENU(XRCID("uvInvertY"), OutfitStudio::OnInvertUV)

	EVT_MENU(XRCID("moveShape"), OutfitStudio::OnMoveShape)
	EVT_MENU(XRCID("scaleShape"), OutfitStudio::OnScaleShape)
	EVT_MENU(XRCID("rotateShape"), OutfitStudio::OnRotateShape)
	EVT_MENU(XRCID("renameShape"), OutfitStudio::OnRenameShape)
	EVT_MENU(XRCID("setReference"), OutfitStudio::OnSetReference)
	EVT_MENU(XRCID("copyShape"), OutfitStudio::OnDupeShape)
	EVT_MENU(XRCID("deleteShape"), OutfitStudio::OnDeleteShape)
	EVT_MENU(XRCID("addBone"), OutfitStudio::OnAddBone)
	EVT_MENU(XRCID("addCustomBone"), OutfitStudio::OnAddCustomBone)
	EVT_MENU(XRCID("deleteBone"), OutfitStudio::OnDeleteBone)
	EVT_MENU(XRCID("deleteBoneSelected"), OutfitStudio::OnDeleteBoneFromSelected)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudio::OnCopyBoneWeight)	
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudio::OnCopySelectedWeight)
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudio::OnTransferSelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudio::OnMaskWeighted)
	EVT_MENU(XRCID("buildSkinPartitions"), OutfitStudio::OnBuildSkinPartitions)
	EVT_MENU(XRCID("shapeProperties"), OutfitStudio::OnShapeProperties)

	EVT_MENU(XRCID("editUndo"), OutfitStudio::OnUndo)
	EVT_MENU(XRCID("editRedo"), OutfitStudio::OnRedo)

	EVT_TREE_STATE_IMAGE_CLICK(wxID_ANY, OutfitStudio::OnShapeVisToggle)
	EVT_TREE_SEL_CHANGING(XRCID("outfitShapes"), OutfitStudio::OnCheckTreeSel)
	EVT_TREE_SEL_CHANGED(XRCID("outfitShapes"), OutfitStudio::OnShapeSelect)
	EVT_TREE_ITEM_ACTIVATED(XRCID("outfitShapes"), OutfitStudio::OnShapeActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitShapes"), OutfitStudio::OnShapeContext)
	EVT_TREE_BEGIN_DRAG(XRCID("outfitShapes"), OutfitStudio::OnShapeDrag)
	EVT_TREE_END_DRAG(XRCID("outfitShapes"), OutfitStudio::OnShapeDrop)
	EVT_TREE_SEL_CHANGED(XRCID("outfitBones"), OutfitStudio::OnBoneSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudio::OnBoneContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudio::OnBoneTreeContext)
	
	EVT_BUTTON(XRCID("meshTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("boneTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("lightsTabButton"), OutfitStudio::OnTabButtonClick)

	EVT_SLIDER(XRCID("lightAmbientSlider"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightBrightnessSlider1"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightBrightnessSlider2"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightBrightnessSlider3"), OutfitStudio::OnUpdateLights)
	EVT_BUTTON(XRCID("lightReset"), OutfitStudio::OnResetLights)
	
	EVT_MOVE_END(OutfitStudio::OnMoveWindow)
	EVT_SIZE(OutfitStudio::OnSetSize)
wxEND_EVENT_TABLE()

// ----------------------------------------------------------------------------
// OutfitStudio frame
// ----------------------------------------------------------------------------

OutfitStudio::OutfitStudio(wxWindow* parent, const wxPoint& pos, const wxSize& size, ConfigurationManager& inConfig) : appConfig(inConfig) {
	wxLogMessage("Loading Outfit Studio at X:%d Y:%d with W:%d H:%d...", pos.x, pos.y, size.GetWidth(), size.GetHeight());

	wxXmlResource* resource = wxXmlResource::Get();
	SetParent(parent);
	progressVal = 0;

	resource->InitAllHandlers();
	bool loaded = resource->Load("res\\outfitStudio.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load outfitStudio.xrc file!", "Error", wxICON_ERROR);
		Close(true);
		return;
	}

	loaded = resource->LoadFrame(this, GetParent(), "outfitStudio");
	if (!loaded) {
		wxMessageBox("Failed to load Outfit Studio frame!", "Error", wxICON_ERROR);
		Close(true);
		return;
	}

	SetIcon(wxIcon("res\\outfitstudio.png", wxBITMAP_TYPE_PNG));

	int statusWidths[] = { -1, 275, 100 };
	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	if (statusBar) {
		statusBar->SetFieldsCount(3);
		statusBar->SetStatusWidths(3, statusWidths);
		statusBar->SetStatusText("Ready!");
	}

/*
	HWND h = this->GetHandle();
	bool ret = ChangeWindowsMessageFilterEX(h, WM_DROPFILES, MSGFLT_ADD);
	if (!ret) {
		wxMessageBox("blargh");
	}
	ret = ChangeWindowMessageFilterEX(h,WM_COPYDATA, MSGFLT_ADD);
	ret = ChangeWindowMessageFilterEX(h,WM_COPY, MSGFLT_ADD);
	ret = ChangeWindowMessageFilterEX(h,WM_MOVE, MSGFLT_ADD);
	ret = ChangeWindowMessageFilterEX(h,0x0049, MSGFLT_ADD);
*/
	this->DragAcceptFiles(true);
	
	toolBar = (wxToolBar*)FindWindowByName("toolbar");
	if (toolBar) {
		toolBar->ToggleTool(XRCID("btnSelect"), true);
		toolBar->SetToolDisabledBitmap(XRCID("btnSelect"), wxBitmap("res\\SelectBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnMaskBrush"), wxBitmap("res\\MaskBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnInflateBrush"), wxBitmap("res\\InflateBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnDeflateBrush"), wxBitmap("res\\DeflateBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnMoveBrush"), wxBitmap("res\\MoveBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnSmoothBrush"), wxBitmap("res\\SmoothBrush_d.png", wxBITMAP_TYPE_PNG));
		toolBar->SetToolDisabledBitmap(XRCID("btnWeightBrush"), wxBitmap("res\\WeightBrush_d.png", wxBITMAP_TYPE_PNG));
		wxSlider* fovSlider = (wxSlider*)toolBar->FindWindowByName("fovSlider");
		if (fovSlider)
			fovSlider->Bind(wxEVT_SLIDER, &OutfitStudio::OnFieldOfViewSlider, this);
	}
	
	wxMenuBar* menu = GetMenuBar();
	if (menu)
		menu->Enable(XRCID("btnWeightBrush"), false);


	visStateImages = new wxImageList(16, 16, false, 2);
	wxBitmap visImg("res\\icoVisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap invImg("res\\icoInvisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap wfImg("res\\icoWireframe.png", wxBITMAP_TYPE_PNG);

	if (visImg.IsOk())
		visStateImages->Add(visImg);
	if (invImg.IsOk())
		visStateImages->Add(invImg);
	if (wfImg.IsOk())
		visStateImages->Add(wfImg);

	wxStateButton* meshTab = (wxStateButton*)FindWindowByName("meshTabButton");
	if (meshTab)
		meshTab->SetCheck();

	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	if (outfitShapes) {
		outfitShapes->AssignStateImageList(visStateImages);
		shapesRoot = outfitShapes->AddRoot("Shapes");
	}

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	if (outfitBones)
		bonesRoot = outfitBones->AddRoot("Bones");

	int ambient = appConfig.GetIntValue("Lights/Ambient");
	int brightness1 = appConfig.GetIntValue("Lights/Brightness1");
	int brightness2 = appConfig.GetIntValue("Lights/Brightness2");
	int brightness3 = appConfig.GetIntValue("Lights/Brightness3");

	lightSettings = (wxPanel*)FindWindowByName("lightSettings");
	if (lightSettings) {
		wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
		if (lightAmbientSlider)
			lightAmbientSlider->SetValue(ambient);

		wxSlider* lightBrightnessSlider1 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider1");
		if (lightBrightnessSlider1)
			lightBrightnessSlider1->SetValue(brightness1);

		wxSlider* lightBrightnessSlider2 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider2");
		if (lightBrightnessSlider2)
			lightBrightnessSlider2->SetValue(brightness2);

		wxSlider* lightBrightnessSlider3 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider3");
		if (lightBrightnessSlider3)
			lightBrightnessSlider3->SetValue(brightness3);
	}

	boneScale = (wxSlider*)FindWindowByName("boneScale");
	
	targetGame = appConfig.GetIntValue("TargetGame");
	
	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	if (leftPanel) {
		glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
		glView->SetNotifyWindow(this);
	}

	wxWindow* rightPanel = FindWindowByName("rightSplitPanel");
	if (rightPanel)
		rightPanel->SetDoubleBuffered(true);

	if (glView)
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

	if (leftPanel)
		leftPanel->Layout();

	wxSplitterWindow* splitter = (wxSplitterWindow*)FindWindowByName("splitter");
	if (splitter)
		splitter->SetSashPosition(850);

	wxSplitterWindow* splitterRight = (wxSplitterWindow*)FindWindowByName("splitterRight");
	if (splitterRight)
		splitterRight->SetSashPosition(200);

	SetDropTarget(new DnDFile(this));

	wxLogMessage("Outfit Studio loaded.");
}

OutfitStudio::~OutfitStudio() {
	if (project) {
		delete project;
		project = nullptr;
	}

	for (auto &sd : sliderDisplays)
		delete sd.second;

	if (glView)
		delete glView;

	static_cast<BodySlideApp*>(wxApp::GetInstance())->CloseOutfitStudio();
	wxLogMessage("Outfit Studio closed.");
}

void OutfitStudio::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = GetPosition();
	Config.SetValue("OutfitStudioFrame.x", p.x);
	Config.SetValue("OutfitStudioFrame.y", p.y);
	event.Skip();
}

void OutfitStudio::OnSetSize(wxSizeEvent& event) {
	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		Config.SetValue("OutfitStudioFrame.width", p.x);
		Config.SetValue("OutfitStudioFrame.height", p.y);
	}

	Config.SetValue("OutfitStudioFrame.maximized", maximized ? "true" : "false");
	event.Skip();
}

void OutfitStudio::CreateSetSliders() {
	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	if (!sliderScroll)
		return;

	int inc = 1;
	if (project->SliderCount()) {
		inc = 90 / project->SliderCount();
		StartProgress("Creating sliders...");
	}

	UpdateProgress(0, "Clearing old sliders...");

	sliderScroll->Freeze();
	sliderScroll->DestroyChildren();
	for (auto &sd : sliderDisplays)
		delete sd.second;

	sliderDisplays.clear();

	wxSizer* rootSz = sliderScroll->GetSizer();

	for (int i = 0; i < project->SliderCount(); i++)  {
		UpdateProgress(inc, "Loading slider: " + project->GetSliderName(i));
		if (project->SliderClamp(i))    // clamp sliders are a special case, usually an incorrect scale
			continue;

		createSliderGUI(project->GetSliderName(i), i, sliderScroll, rootSz);
	}

	if (!sliderScroll->GetDropTarget())
		sliderScroll->SetDropTarget(new DnDSliderFile(this));

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

	d->btnSliderEdit = new wxBitmapButton(d->sliderPane, wxID_ANY, wxBitmap("res\\EditSmall.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, name + "|btn");
	d->btnSliderEdit->SetBitmapDisabled(wxBitmap("res\\EditSmall_d.png", wxBITMAP_TYPE_ANY));
	d->btnSliderEdit->SetToolTip("Turn on edit mode for this slider.");
	d->paneSz->Add(d->btnSliderEdit, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnMinus = new wxButton(d->sliderPane, wxID_ANY, "-", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnMinus");
	d->btnMinus->SetToolTip("Weaken slider data by 1%.");
	d->btnMinus->SetForegroundColour(wxTransparentColour);
	d->btnMinus->Hide();
	d->paneSz->Add(d->btnMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnPlus = new wxButton(d->sliderPane, wxID_ANY, "+", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnPlus");
	d->btnPlus->SetToolTip("Strengthen slider data by 1%.");
	d->btnPlus->SetForegroundColour(wxTransparentColour);
	d->btnPlus->Hide();
	d->paneSz->Add(d->btnPlus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->sliderNameCheckID = 1000 + id;
	d->sliderNameCheck = new wxCheckBox(d->sliderPane, d->sliderNameCheckID, "", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, name + "|check");
	d->sliderNameCheck->SetForegroundColour(wxColour(255, 255, 255));
	d->paneSz->Add(d->sliderNameCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	d->sliderName = new wxStaticText(d->sliderPane, wxID_ANY, name.c_str(), wxDefaultPosition, wxDefaultSize, 0, name + "|lbl");
	d->sliderName->SetForegroundColour(wxColour(255, 255, 255));
	d->paneSz->Add(d->sliderName, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->sliderID = 2000 + id;
	d->slider = new wxSlider(d->sliderPane, d->sliderID, 0, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL, wxDefaultValidator, name + "|slider");
	d->slider->SetMinSize(wxSize(-1, 20));
	d->slider->SetMaxSize(wxSize(-1, 20));

	d->paneSz->Add(d->slider, 1, wxLEFT | wxRIGHT | wxEXPAND, 5);

	d->sliderReadout = new wxTextCtrl(d->sliderPane, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, -1), wxWANTS_CHARS | wxTE_RIGHT | wxTE_PROCESS_ENTER | wxSIMPLE_BORDER, wxDefaultValidator, name + "|readout");
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

string OutfitStudio::NewSlider(const string& suggestedName, bool skipPrompt) {
	string namebase = "NewSlider";
	if (suggestedName != "")
	{
		namebase = suggestedName;
	}
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;

	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);
	string finalName;
	if (!skipPrompt) {
		finalName = wxGetTextFromUser("Enter a name for the new slider:", "Create New Slider", thename, this);
		if (finalName.empty())
			return finalName;
	}
	else {
		finalName = thename;
	}

	finalName = project->NameAbbreviate(finalName);
	wxLogMessage("Creating new slider '%s'.", finalName);

	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddEmptySlider(finalName);
	ShowSliderEffect(finalName);
	sliderScroll->FitInside();

	return finalName;
}

void OutfitStudio::SetSliderValue(int index, int val) {
	string name = project->GetSliderName(index);
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
	vector<Vector3> verts;
	vector<string> shapes;
	project->GetShapes(shapes);

	for (auto &shape : shapes) {
		project->GetLiveVerts(shape, verts);
		glView->UpdateMeshVertices(shape, &verts, recalcBVH);
	}

	glView->Render();
}

void OutfitStudio::ShowSliderEffect(int sliderID, bool show) {
	if (project->ValidSlider(sliderID)) {
		project->SliderShow(sliderID) = show;
		wxCheckBox* sliderState = (wxCheckBox*)FindWindowById(1000 + sliderID);
		if (!sliderState)
			return;

		if (show)
			sliderState->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			sliderState->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
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
	if (!outfitShapes)
		return;

	wxTreeItemId item;
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;
	wxTreeItemIdValue subcookie;
	item = outfitShapes->GetFirstChild(shapesRoot, cookie);
	while (item.IsOk()) {
		subitem = outfitShapes->GetFirstChild(item, subcookie);
		while (subitem.IsOk()) {
			if (outfitShapes->GetItemText(subitem) == shapeName) {
				outfitShapes->UnselectAll();
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

vector<string> OutfitStudio::GetShapeList() {
	vector<string> shapes;
	wxTreeItemIdValue cookie;

	wxTreeItemId curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		wxString shapeName = outfitShapes->GetItemText(curItem);
		shapes.push_back(shapeName.ToStdString());
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
	}

	return shapes;
}

void OutfitStudio::UpdateShapeSource(const string& shapeName) {
	project->UpdateShapeFromMesh(shapeName, glView->GetMesh(shapeName));
}

void OutfitStudio::ActiveShapesUpdated(TweakStroke* refStroke, bool bIsUndo, bool setWeights) {
	if (bEditSlider) {
		vector<mesh*> refMeshes = refStroke->GetRefMeshes();
		for (auto &m : refMeshes) {
			unordered_map<ushort, Vector3> strokeDiff;
			if (refStroke->pointStartState.find(m) != refStroke->pointStartState.end()) {
				for (auto &p : refStroke->pointStartState[m]) {
					if (bIsUndo)
						strokeDiff[p.first] = p.second - refStroke->pointEndState[m][p.first];
					else
						strokeDiff[p.first] = refStroke->pointEndState[m][p.first] - p.second;
				}
				project->UpdateMorphResult(m->shapeName, activeSlider, strokeDiff);
			}
		}
	}
	else {
		if (refStroke->BrushType() == TBT_WEIGHT) {
			vector<mesh*> refMeshes = refStroke->GetRefMeshes();

			for (auto &m : refMeshes) {
				if (refStroke->pointStartState.find(m) != refStroke->pointStartState.end()) {
					unordered_map<ushort, float>* weights = &project->workWeights[m->shapeName];
					if (bIsUndo) {
						for (auto &p : refStroke->pointStartState[m]) {
							if (p.second.y == 0.0f)
								weights->erase(p.first);
							else
								(*weights)[p.first] = p.second.y;
						}
					}
					else {
						for (auto &p : refStroke->pointEndState[m]) {
							if (p.second.y == 0.0f)
								weights->erase(p.first);
							else
								(*weights)[p.first] = p.second.y;
						}
					}

					if (setWeights) {
						TweakBrush* br = refStroke->GetRefBrush();
						string refBone;

						if (br->Name() == "Weight Paint")
							refBone = ((TB_Weight*)br)->refBone;
						else if (br->Name() == "Weight Erase")
							refBone = ((TB_Unweight*)br)->refBone;
						else
							refBone = ((TB_SmoothWeight*)br)->refBone;

						project->GetWorkAnim()->SetWeights(m->shapeName, refBone, *weights);
					}
				}
			}
			project->workWeights.clear();
		}
	}
}

vector<ShapeItemData*>& OutfitStudio::GetSelectedItems() {
	return selectedItems;
}

string OutfitStudio::GetActiveBone() {
	return activeBone;
}

void OutfitStudio::EnterSliderEdit(const string& sliderName) {
	bEditSlider = true;
	activeSlider = sliderName;
	SliderDisplay* d = sliderDisplays[activeSlider];
	d->slider->SetValue(100);
	SetSliderValue(activeSlider, 100);

	if (d->sliderNameCheck->Get3StateValue() == wxCheckBoxState::wxCHK_UNCHECKED) {
		d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		ShowSliderEffect(d->sliderID - 2000, true);
	}

	d->sliderNameCheck->Enable(false);
	d->slider->SetFocus();
	d->btnMinus->Show();
	d->btnPlus->Show();
	d->sliderPane->Layout();
	glView->GetStrokeManager()->PushBVH();
	glView->SetStrokeManager(&d->sliderStrokes);
	MenuEnterSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudio::ExitSliderEdit() {
	SliderDisplay* d = sliderDisplays[activeSlider];
	d->sliderNameCheck->Enable(true);
	d->slider->SetValue(0);
	SetSliderValue(activeSlider, 0);
	ShowSliderEffect(activeSlider, true);
	d->slider->SetFocus();
	d->btnMinus->Hide();
	d->btnPlus->Hide();
	d->sliderPane->Layout();
	activeSlider.clear();
	bEditSlider = false;
	glView->GetStrokeManager()->PushBVH();
	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudio::MenuEnterSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), true);
	menu->Enable(XRCID("menuExportSlider"), true);
	menu->Enable(XRCID("sliderNegate"), true);
	menu->Enable(XRCID("sliderClear"), true);
	menu->Enable(XRCID("sliderDelete"), true);
	menu->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudio::MenuExitSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), false);
	menu->Enable(XRCID("menuExportSlider"), false);
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

		pg2 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj2", wxWizardPageSimple);
		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_Texture, this);

		wxChoice* tmplChoice = XRCCTRL(wiz, "npTemplateChoice", wxChoice);
		tmplChoice->Append(tmplNameArray);
		tmplChoice->Select(0);

		wiz.FitToPage(pg1);
		wiz.CenterOnParent();

		result = wiz.RunWizard(pg1);
	}
	if (!result)
		return;

	GetMenuBar()->Enable(XRCID("fileSave"), false);

	string outfitName = XRCCTRL(wiz, "npOutfitName", wxTextCtrl)->GetValue();

	wxLogMessage("Creating project '%s'...", outfitName);
	StartProgress(wxString::Format("Creating project '%s'...", outfitName));

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();
	
	glView->Cleanup();
	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(appConfig, this);

	UpdateProgress(10, "Loading reference...");

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);
		error = project->LoadReferenceTemplate(refTemplate.ToStdString());
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToStdString(),
				sliderSetName.ToStdString(), true, refShape.ToStdString());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToStdString(), refShape.ToStdString());
		}
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	UpdateProgress(40, "Loading outfit...");

	error = 0;
	if (XRCCTRL(wiz, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		wxLogMessage("Loading outfit '%s' from '%s'...", outfitName, fileName);
		if (fileName.Lower().EndsWith(".nif"))
			error = project->AddNif(fileName.ToStdString(), true, outfitName);
		else if (fileName.Lower().EndsWith(".obj"))
			error = project->AddShapeFromObjFile(fileName.ToStdString(), outfitName);
		else if (fileName.Lower().EndsWith(".fbx"))
			error = project->ImportShapeFBX(fileName.ToStdString(), outfitName);
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, "Creating outfit...");

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetTextures("_AUTO_");
	else
		project->SetTextures(XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath().ToStdString());

	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format("Creating %d slider(s)...", project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, "Applying slider effects...");
	ApplySliders();

	wxLogMessage("Project created.");
	UpdateProgress(100, "Finished");

	EndProgress();
}

void OutfitStudio::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this, "Select a slider set to load", "SliderSets", wxEmptyString, "Slider Set Files (*.osp;*.xml)|*.osp;*.xml", wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	string file = loadProjectDialog.GetPath();
	vector<string> setnames;
	SliderSetFile InFile(file);
	if (InFile.fail()) {
		wxLogError("Failed to open '%s' as a slider set file!", file);
		wxMessageBox(wxString::Format("Failed to open '%s' as a slider set file!", file), "Slider Set Error", wxICON_ERROR);
		return;
	}

	InFile.GetSetNames(setnames);
	wxArrayString choices;
	for (auto &s : setnames)
		choices.Add(s);

	string outfit;
	if (choices.GetCount() > 1) {
		outfit = wxGetSingleChoice("Please choose an outfit to load", "Load a slider set", choices, 0, this);
		if (outfit.empty())
			return;
	}
	else if (choices.GetCount() == 1)
		outfit = choices.front();
	else
		return;

	wxLogMessage("Loading project '%s' from file '%s'...", outfit, file);
	StartProgress("Loading project...");

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(appConfig, this);

	wxLogMessage("Loading outfit data...");
	UpdateProgress(10, "Loading outfit data...");
	StartSubProgress(10, 40);

	int error = project->OutfitFromSliderSet(file, outfit);
	if (error) {
		EndProgress();
		wxLogError("Failed to create project (%d)!", error);
		wxMessageBox(wxString::Format("Failed to create project '%s' from file '%s' (%d)!", outfit, file, error), "Slider Set Error", wxICON_ERROR);
		RefreshGUIFromProj();
		return;
	}

	string shape = project->GetBaseShape();
	wxLogMessage("Loading reference shape '%s'...", shape);
	UpdateProgress(50, wxString::Format("Loading reference shape '%s'...", shape));

	if (!shape.empty()) {
		error = project->LoadReferenceNif(project->activeSet.GetInputFileName(), shape, false);
		if (error) {
			EndProgress();
			RefreshGUIFromProj();
			return;
		}
	}

	wxLogMessage("Loading textures...");
	UpdateProgress(60, "Loading textures...");

	project->SetTextures("_AUTO_");

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, "Creating outfit...");
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format("Creating %d slider(s)...", project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, "Applying slider effects...");
	ApplySliders();

	wxLogMessage("Project loaded.");
	UpdateProgress(100, "Finished");
	GetMenuBar()->Enable(XRCID("fileSave"), true);
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
		tmplChoice->Append(tmplNameArray);
		tmplChoice->Select(0);

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress("Loading reference...");
	glView->DeleteMesh(project->GetBaseShape());

	UpdateProgress(10, "Loading reference set...");
	bool ClearRef = !(XRCCTRL(dlg, "chkClearSliders", wxCheckBox)->IsChecked());

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(dlg, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);
		error = project->LoadReferenceTemplate(refTemplate.ToStdString(), ClearRef);
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToStdString(),
				sliderSetName.ToStdString(), ClearRef, refShape.ToStdString());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToStdString(), refShape.ToStdString(), ClearRef);
		}
	}
	else
		project->ClearReference();

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	project->SetTexture(project->GetBaseShape(), "_AUTO_");

	wxLogMessage("Creating reference...");
	UpdateProgress(60, "Creating reference...");
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(70, wxString::Format("Creating %d slider(s)...", project->SliderCount()));
	StartSubProgress(70, 99);
	CreateSetSliders();

	ShowSliderEffect(0);
	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, "Applying slider effects...");
	ApplySliders();

	wxLogMessage("Reference loaded.");
	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_File, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_Texture, this);
		if (project->GetWorkNif()->IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue();

	GetMenuBar()->Enable(XRCID("fileSave"), false);

	wxLogMessage("Loading outfit...");
	StartProgress("Loading outfit...");

	vector<string> oldShapes;
	project->GetShapes(oldShapes);
	for (auto &s : oldShapes) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s);
		}
	}

	UpdateProgress(1, "Loading outfit...");

	int ret = 0;
	if (XRCCTRL(dlg, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		if (fileName.Lower().EndsWith(".nif")) {
			if (!XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->IsChecked())
				ret = project->AddNif(fileName.ToStdString(), true, outfitName);
			else
				ret = project->AddNif(fileName.ToStdString(), false);
		}
		else if (fileName.Lower().EndsWith(".obj"))
			ret = project->AddShapeFromObjFile(fileName.ToStdString(), outfitName);
		else if (fileName.Lower().EndsWith(".fbx"))
			ret = project->ImportShapeFBX(fileName.ToStdString(), outfitName);
	}
	else
		project->ClearOutfit();

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	if (XRCCTRL(dlg, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetTextures("_AUTO_");
	else
		project->SetTextures(XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToStdString());

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, "Creating outfit...");
	RefreshGUIFromProj();

	wxLogMessage("Outfit loaded.");
	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::ClearProject() {
	vector<string> oldShapes;
	project->GetShapes(oldShapes);
	for (auto &s : oldShapes)
		glView->DeleteMesh(s);

	project->mFileName.clear();
	project->mOutfitName.clear();
	project->mDataDir.clear();
	project->mBaseFile.clear();
	project->mGamePath.clear();
	project->mGameFile.clear();

	if (targetGame == SKYRIM)
		project->mGenWeights = true;
	else
		project->mGenWeights = false;

	project->mCopyRef = true;

	glView->DestroyOverlays();
}

void OutfitStudio::RenameProject(const string& projectName) {
	project->outfitName = projectName;
	if (outfitRoot.IsOk() && outfitShapes)
		outfitShapes->SetItemText(outfitRoot, projectName);
}

void OutfitStudio::RefreshGUIFromProj() {
	WorkingGUIFromProj();
	AnimationGUIFromProj();

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		outfitShapes->UnselectAll();
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	vector<string> shapeNames;
	for (auto &s : selectedItems)
		shapeNames.push_back(s->shapeName);

	glView->SetActiveShapes(shapeNames);

	if (activeItem)
		glView->SetSelectedShape(activeItem->shapeName);
	else
		glView->SetSelectedShape("");
}

void OutfitStudio::AnimationGUIFromProj() {
	if (!outfitBones)
		return;

	vector<string> activeBones;
	if (outfitBones->GetChildrenCount(bonesRoot) > 0)
		outfitBones->DeleteChildren(bonesRoot);

	activeBone.clear();
	project->GetActiveBones(activeBones);
	for (auto &bone : activeBones)
		outfitBones->AppendItem(bonesRoot, bone);
}

void OutfitStudio::WorkingGUIFromProj() {
	if (outfitRoot.IsOk()) {
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	vector<string> shapes;
	project->GetShapes(shapes);

	wxTreeItemId subItem;
	activeItem = nullptr;
	selectedItems.clear();

	if (outfitShapes && shapes.size() > 0) {
		if (shapes.size() == 1 && project->IsBaseShape(shapes.front()))
			outfitRoot = outfitShapes->AppendItem(shapesRoot, "Reference Only");
		else
			outfitRoot = outfitShapes->AppendItem(shapesRoot, project->OutfitName());
	}

	for (auto &shape : shapes) {
		glView->DeleteMesh(shape);

		glView->AddMeshFromNif(project->GetWorkNif(), shape, true);
		glView->SetMeshTexture(shape, project->GetShapeTexture(shape), project->GetWorkNif()->IsShaderSkin(shape));
		if (outfitShapes) {
			subItem = outfitShapes->AppendItem(outfitRoot, shape);
			outfitShapes->SetItemState(subItem, 0);
			outfitShapes->SetItemData(subItem, new ShapeItemData(project->GetWorkNif(), shape));

			if (project->IsBaseShape(shape)) {
				outfitShapes->SetItemBold(subItem);
				outfitShapes->SetItemTextColour(subItem, wxColour(0, 255, 0));
			}
		}
		glView->Render();
	}

	if (outfitShapes)
		outfitShapes->ExpandAll();
}

void OutfitStudio::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();

	string copyStr = XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue();
	copyStr = project->NameAbbreviate(copyStr);
	string defSliderSetFile = copyStr + ".osp";
	string defShapeDataDir = copyStr;
	string defOutputFile = copyStr + ".nif";

	wxFilePickerCtrl* fp = (wxFilePickerCtrl*)win->FindWindowByName("sssSliderSetFile");
	if (fp)
		fp->SetPath(defSliderSetFile);

	wxDirPickerCtrl* dp = (wxDirPickerCtrl*)win->FindWindowByName("sssShapeDataFolder");
	if (dp)
		dp->SetPath(defShapeDataDir);

	fp = (wxFilePickerCtrl*)win->FindWindowByName("sssShapeDataFile");
	if (fp)
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

void OutfitStudio::OnSaveSliderSet(wxCommandEvent& event) {
	if (project->mFileName.empty()) {
		OnSaveSliderSetAs(event);
	}
	else {
		if (!project->GetWorkNif()->IsValid()) {
			wxMessageBox("There are no valid shapes loaded!", "Error");
			return;
		}

		if (project->HasUnweighted()) {
			wxLogWarning("Unweighted vertices found.");
			int error = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
			if (error != wxYES)
				return;
		}

		wxLogMessage("Saving project '%s'...", project->OutfitName());
		StartProgress(wxString::Format("Saving project '%s'...", project->OutfitName()));
		project->ClearBoneScale();

		vector<mesh*> shapeMeshes;
		vector<string> shapes;
		project->GetShapes(shapes);
		for (auto &s : shapes)
			if (!project->IsBaseShape(s))
				shapeMeshes.push_back(glView->GetMesh(s));

		bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

		if (updateNormals)
			project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

		string error = project->Save(project->mFileName, project->mOutfitName, project->mDataDir, project->mBaseFile,
			project->mGamePath, project->mGameFile, project->mGenWeights, project->mCopyRef);

		if (!error.empty()) {
			wxLogError(error.c_str());
			wxMessageBox(error, "Error", wxOK | wxICON_ERROR);
		}

		EndProgress();
	}
}

void OutfitStudio::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox("There are no valid shapes loaded!", "Error");
		return;
	}

	if (project->HasUnweighted()) {
		wxLogWarning("Unweighted vertices found.");
		int error = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return;
	}

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnSSSNameCopy, this);
		XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudio::OnSSSGenWeightsTrue, this);
		XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudio::OnSSSGenWeightsFalse, this);
		wxString sssName;
		if (!project->mOutfitName.empty())
			sssName = project->mOutfitName;
		else if (!project->OutfitName().empty())
			sssName = project->OutfitName();
		else
			sssName = "New Outfit";

		sssName = project->NameAbbreviate(sssName.ToStdString());
		XRCCTRL(dlg, "sssName", wxTextCtrl)->SetValue(sssName);
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory("SliderSets");

		if (!project->mFileName.empty())
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(project->mFileName);
		else
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(sssName + ".osp");

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
		if (project->GetBaseShape().empty()) {
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->Disable();
		}
		else
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(project->mCopyRef);

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	wxString strFileName;
	wxString strOutfitName;
	wxString strDataDir;
	wxString strBaseFile;
	wxString strGamePath;
	wxString strGameFile;
	bool copyRef;
	bool genWeights;

	strFileName = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strFileName.length() <= 4) {
		wxMessageBox("Invalid or no slider set file specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	if (!strFileName.EndsWith(".osp"))
		strFileName = strFileName.Append(".osp");

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

	wxFileName relativeFolder(strDataDir);
	if (!relativeFolder.IsRelative()) {
		wxString curDir(wxGetCwd());
		wxString dataFolder(wxString::Format("%s/%s", curDir, "ShapeData"));
		relativeFolder.MakeRelativeTo(dataFolder);
		strDataDir = relativeFolder.GetFullPath();
	}

	strBaseFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strBaseFile.length() <= 4) {
		wxMessageBox("An invalid or no base outfit .nif file name specified! Please try again.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	if (!strBaseFile.EndsWith(".nif"))
		strBaseFile = strBaseFile.Append(".nif");

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

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format("Saving project '%s'...", strOutfitName));
	project->ClearBoneScale();

	vector<mesh*> shapeMeshes;
	vector<string> shapes;
	project->GetShapes(shapes);
	for (auto &s : shapes)
		if (!project->IsBaseShape(s))
			shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));

	if (updateNormals)
		project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	string error = project->Save(strFileName, strOutfitName, strDataDir, strBaseFile,
		strGamePath, strGameFile, genWeights, copyRef);

	if (error.empty()) {
		GetMenuBar()->Enable(XRCID("fileSave"), true);
		RenameProject(strOutfitName.ToStdString());

		static_cast<BodySlideApp*>(wxApp::GetInstance())->RefreshOutfitList();
	}
	else {
		wxLogError(error.c_str());
		wxMessageBox(error, "Error", wxOK | wxICON_ERROR);
	}

	EndProgress();
}

void OutfitStudio::OnBrushPane(wxCollapsiblePaneEvent& event) {
	wxCollapsiblePane* brushPane = (wxCollapsiblePane*)event.GetEventObject();
	if (!brushPane)
		return;

	if (!glView->GetActiveBrush())
		brushPane->Collapse();

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	if (leftPanel)
		leftPanel->Layout();
}

void OutfitStudio::OnSetBaseShape(wxCommandEvent& WXUNUSED(event)) {
	wxLogMessage("Setting new base shape.");
	project->ClearBoneScale();

	vector<string> shapes;
	project->GetShapes(shapes);
	for (auto &s : shapes)
		if (!project->IsBaseShape(s))
			UpdateShapeSource(s);

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
	if (!project->GetWorkNif()->IsValid())
		return;

	if (project->HasUnweighted()) {
		wxLogWarning("Unweighted vertices found.");
		int error = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return;
	}

	string fileName = wxFileSelector("Export outfit NIF", wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.empty())
		return;

	wxLogMessage("Exporting outfit to NIF file '%s'...", fileName);
	project->ClearBoneScale();

	vector<mesh*> shapeMeshes;
	vector<string> shapes;
	project->GetShapes(shapes);
	for (auto &s : shapes)
		if (!project->IsBaseShape(s))
			shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));
	int error = project->SaveOutfitNif(fileName, shapeMeshes, updateNormals);
	if (error) {
		wxLogError("Failed to save NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format("Failed to save NIF file '%s'!", fileName), "Export Error", wxICON_ERROR);
	}
}

void OutfitStudio::OnExportOutfitNifWithRef(wxCommandEvent& event) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (project->GetBaseShape().empty()) {
		OnExportOutfitNif(event);
		return;
	}

	if (project->HasUnweighted()) {
		wxLogWarning("Unweighted vertices found.");
		int error = wxMessageBox("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?", "Unweighted Vertices", wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return;
	}

	string fileName = wxFileSelector("Export project NIF", wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.empty())
		return;

	wxLogMessage("Exporting outfit with reference to NIF file '%s'...", fileName);
	project->ClearBoneScale();

	vector<mesh*> shapeMeshes;
	vector<string> shapes;
	project->GetShapes(shapes);
	for (auto &s : shapes)
		shapeMeshes.push_back(glView->GetMesh(s));

	bool updateNormals = GetMenuBar()->IsChecked(XRCID("btnAutoNormals"));
	int error = project->SaveOutfitNif(fileName, shapeMeshes, updateNormals, true);
	if (error) {
		wxLogError("Failed to save NIF file '%s' with reference!", fileName);
		wxMessageBox(wxString::Format("Failed to save NIF file '%s' with reference!", fileName), "Export Error", wxICON_ERROR);
	}
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
	wxLogMessage("Creating new conversion slider '%s'...", finalName);

	project->ClearBoneScale();
	project->AddCombinedSlider(finalName);

	string shape = project->GetBaseShape();
	if (!shape.empty()) {
		project->UpdateShapeFromMesh(shape, glView->GetMesh(shape));
		project->NegateSlider(finalName, shape);
	}

	vector<string> sliderList;
	project->GetSliderList(sliderList);
	for (auto &s : sliderList) {
		if (!s.compare(finalName))
			continue;
		project->DeleteSlider(s);
	}
	CreateSetSliders();
}

void OutfitStudio::OnSelectSliders(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	for (auto &sd : sliderDisplays)
		ShowSliderEffect(sd.first, checked);

	ApplySliders();
}

void OutfitStudio::OnFixedWeight(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bFixedWeight = checked;
}

void OutfitStudio::OnShapeVisToggle(wxTreeEvent& event) {
	static int groupstate = 0;
	bool notSelf = false;
	bool bVis;
	bool bGhost;

	int state = outfitShapes->GetItemState(event.GetItem());
	string s = outfitShapes->GetItemText(event.GetItem()).ToAscii().data();

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
		project->GetShapes(shapes);
		for (auto &shape : shapes) {
			if (shape == s)
				continue;

			glView->SetShapeGhostMode(shape, bGhost);
			glView->ShowShape(shape, bVis);
		}
		groupstate = state;
	}
	else {
		outfitShapes->SetItemState(event.GetItem(), state);
		glView->SetShapeGhostMode(s, bGhost);
		glView->ShowShape(s, bVis);
		groupstate = 0;
	}

	event.Skip();
}

void OutfitStudio::OnShapeSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;

	if (!item.IsOk())
		return;

	if (outfitShapes->GetItemParent(item).IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(item);
	}
	else {
		subitem = outfitShapes->GetFirstChild(item, cookie);
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(subitem);
	}

	selectedItems.clear();

	vector<string> shapeNames;
	wxArrayTreeItemIds selected;
	outfitShapes->GetSelections(selected);

	for (auto &i : selected) {
		if (outfitShapes->GetItemParent(i).IsOk()) {
			ShapeItemData* data = (ShapeItemData*)outfitShapes->GetItemData(i);
			if (data) {
				shapeNames.push_back(data->shapeName);
				selectedItems.push_back(data);
			}
		}
		else {
			subitem = outfitShapes->GetFirstChild(i, cookie);
			ShapeItemData* data = (ShapeItemData*)outfitShapes->GetItemData(subitem);
			if (data) {
				shapeNames.push_back(data->shapeName);
				selectedItems.push_back(data);
			}
		}
	}

	glView->SetActiveShapes(shapeNames);

	if (activeItem)
		glView->SetSelectedShape(activeItem->shapeName);
	else
		glView->SetSelectedShape("");

	UpdateActiveShapeUI();
}

void OutfitStudio::OnShapeActivated(wxTreeEvent& WXUNUSED(event)) {
	wxCommandEvent evt;
	OnShapeProperties(evt);
}

void OutfitStudio::OnBoneSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;

	project->ClearBoneScale();
	if (boneScale)
		boneScale->SetValue(0);

	if (!activeItem)
		return;

	if (outfitBones->GetItemParent(item).IsOk()) {
		// Clear vcolors of all shapes
		vector<string> shapes;
		project->GetShapes(shapes);
		for (auto &s : shapes) {
			mesh* m = glView->GetMesh(s);
			m->ColorChannelFill(1, 0.0f);
		}

		activeBone = outfitBones->GetItemText(item);

		// Show weights of selected shapes without reference
		for (auto &s : selectedItems) {
			unordered_map<ushort, float> boneWeights;
			if (!project->IsBaseShape(s->shapeName)) {
				project->GetWorkAnim()->GetWeights(s->shapeName, activeBone, boneWeights);
				mesh* m = glView->GetMesh(s->shapeName);
				if (m) {
					m->ColorChannelFill(1, 0.0f);
					for (auto &bw : boneWeights)
						m->vcolors[bw.first].y = bw.second;
				}
			}
		}

		// Always show weights of reference shape
		unordered_map<ushort, float> boneWeights;
		project->GetWorkAnim()->GetWeights(project->GetBaseShape(), activeBone, boneWeights);
		mesh* m = glView->GetMesh(project->GetBaseShape());
		if (m) {
			m->ColorChannelFill(1, 0.0f);
			for (auto &bw : boneWeights)
				m->vcolors[bw.first].y = bw.second;
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

void OutfitStudio::OnShapeContext(wxTreeEvent& event) {
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

void OutfitStudio::OnShapeDrag(wxTreeEvent& event) {
	if (activeItem) {
		outfitShapes->SetCursor(wxCURSOR_HAND);
		event.Allow();
	}
}

void OutfitStudio::OnShapeDrop(wxTreeEvent& event) {
	outfitShapes->SetCursor(wxNullCursor);
	wxTreeItemId dropItem = event.GetItem();

	if (!dropItem.IsOk())
		return;

	// Make first child
	if (dropItem == outfitRoot)
		dropItem = 0;
	
	// Duplicate item
	wxTreeItemId movedItem = outfitShapes->InsertItem(outfitRoot, dropItem, activeItem->shapeName);
	if (!movedItem.IsOk())
		return;

	// Set data
	ShapeItemData* dropData = new ShapeItemData(activeItem->refFile, activeItem->shapeName);
	outfitShapes->SetItemState(movedItem, 0);
	outfitShapes->SetItemData(movedItem, dropData);
	if (project->IsBaseShape(dropData->shapeName)) {
		outfitShapes->SetItemBold(movedItem);
		outfitShapes->SetItemTextColour(movedItem, wxColour(0, 255, 0));
	}
	
	// Delete old item
	outfitShapes->Delete(activeItem->GetId());

	// Select new item
	outfitShapes->UnselectAll();
	outfitShapes->SelectItem(movedItem);
}

void OutfitStudio::OnBoneContext(wxTreeEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnBoneTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneTreeContext");
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
	wxMenuBar* menuBar = GetMenuBar();
	wxToolBar* toolBar = GetToolBar();

	wxString s = w->GetName();
	if (s.EndsWith("|readout")){
		menuBar->Disable();
		return;
	}

	if (glView->GetActiveBrush() && glView->GetActiveBrush()->Type() == TBT_WEIGHT) {
		glView->SetXMirror(previousMirror);
		menuBar->Check(XRCID("btnXMirror"), previousMirror);
	}

	if (id == XRCID("btnSelect")) {
		glView->SetEditMode(false);
		glView->SetActiveBrush(-1);
		menuBar->Check(XRCID("btnSelect"), true);
		toolBar->ToggleTool(XRCID("btnSelect"), true);

		wxCollapsiblePane* brushPane = (wxCollapsiblePane*)FindWindowByName("brushPane");
		if (brushPane) {
			brushPane->Collapse();
			wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
			if (leftPanel)
				leftPanel->Layout();
		}
		return;
	}
	if (id == XRCID("btnMaskBrush")) {
		glView->SetActiveBrush(0);
		menuBar->Check(XRCID("btnMaskBrush"), true);
		toolBar->ToggleTool(XRCID("btnMaskBrush"), true);
	}
	else if (id == XRCID("btnInflateBrush")) {
		glView->SetActiveBrush(1);
		menuBar->Check(XRCID("btnInflateBrush"), true);
		toolBar->ToggleTool(XRCID("btnInflateBrush"), true);
	}
	else if (id == XRCID("btnDeflateBrush")) {
		glView->SetActiveBrush(2);
		menuBar->Check(XRCID("btnDeflateBrush"), true);
		toolBar->ToggleTool(XRCID("btnDeflateBrush"), true);
	}
	else if (id == XRCID("btnMoveBrush")) {
		glView->SetActiveBrush(3);
		menuBar->Check(XRCID("btnMoveBrush"), true);
		toolBar->ToggleTool(XRCID("btnMoveBrush"), true);
	}
	else if (id == XRCID("btnSmoothBrush")) {
		glView->SetActiveBrush(4);
		menuBar->Check(XRCID("btnSmoothBrush"), true);
		toolBar->ToggleTool(XRCID("btnSmoothBrush"), true);
	}
	else if (id == XRCID("btnWeightBrush")) {
		glView->SetActiveBrush(10);
		menuBar->Check(XRCID("btnWeightBrush"), true);
		toolBar->ToggleTool(XRCID("btnWeightBrush"), true);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		menuBar->Check(XRCID("btnXMirror"), false);
	}
	else {
		glView->SetEditMode(false);
		return;
	}
	glView->SetEditMode();

	CheckBrushBounds();
	UpdateBrushPane();
}

void OutfitStudio::OnSetView(wxCommandEvent& event) {
	int id = event.GetId();

	if (id == XRCID("btnViewFront"))
		glView->SetView('F');
	else if (id == XRCID("btnViewBack"))
		glView->SetView('B');
	else if (id == XRCID("btnViewLeft"))
		glView->SetView('L');
	else if (id == XRCID("btnViewRight"))
		glView->SetView('R');
}

void OutfitStudio::OnTogglePerspective(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	GetMenuBar()->Check(event.GetId(), enabled);
	GetToolBar()->ToggleTool(event.GetId(), enabled);
	glView->SetPerspective(enabled);
}

void OutfitStudio::OnFieldOfViewSlider(wxCommandEvent& event) {
	wxSlider* fovSlider = (wxSlider*)event.GetEventObject();
	if (fovSlider) {
		int fieldOfView = fovSlider->GetValue();
		wxStaticText* fovLabel = (wxStaticText*)GetToolBar()->FindWindowByName("fovLabel");
		if (fovLabel)
			fovLabel->SetLabel(wxString::Format("Field of View: %d", fieldOfView));

		glView->SetFieldOfView(fieldOfView);
	}
}

void OutfitStudio::OnUpdateLights(wxCommandEvent& WXUNUSED(event)) {
	wxSlider* ambientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	wxSlider* brightnessSlider1 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider1");
	wxSlider* brightnessSlider2 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider2");
	wxSlider* brightnessSlider3 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider3");

	if (!ambientSlider || !brightnessSlider1 || !brightnessSlider2 || !brightnessSlider3)
		return;

	int ambient = ambientSlider->GetValue();
	int brightness1 = brightnessSlider1->GetValue();
	int brightness2 = brightnessSlider2->GetValue();
	int brightness3 = brightnessSlider3->GetValue();
	glView->UpdateLights(ambient, brightness1, brightness2, brightness3);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Brightness1", brightness1);
	Config.SetValue("Lights/Brightness2", brightness2);
	Config.SetValue("Lights/Brightness3", brightness3);
}

void OutfitStudio::OnResetLights(wxCommandEvent& WXUNUSED(event)) {
	int ambient = 80;
	int brightness1 = 55;
	int brightness2 = 45;
	int brightness3 = 45;

	glView->UpdateLights(ambient, brightness1, brightness2, brightness3);

	wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	if (lightAmbientSlider)
		lightAmbientSlider->SetValue(ambient);

	wxSlider* lightBrightnessSlider1 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider1");
	if (lightBrightnessSlider1)
		lightBrightnessSlider1->SetValue(brightness1);

	wxSlider* lightBrightnessSlider2 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider2");
	if (lightBrightnessSlider2)
		lightBrightnessSlider2->SetValue(brightness2);

	wxSlider* lightBrightnessSlider3 = (wxSlider*)lightSettings->FindWindowByName("lightBrightnessSlider3");
	if (lightBrightnessSlider3)
		lightBrightnessSlider3->SetValue(brightness3);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Brightness1", brightness1);
	Config.SetValue("Lights/Brightness2", brightness2);
	Config.SetValue("Lights/Brightness3", brightness3);
}

void OutfitStudio::OnClickSliderButton(wxCommandEvent& event) {
	wxWindow* btn = (wxWindow*)event.GetEventObject();
	if (!btn)
		return;

	wxString buttonName = btn->GetName();
	string clickedName = buttonName.BeforeLast('|').ToStdString();
	if (clickedName.empty()) {
		event.Skip();
		return;
	}

	float scale = 0.0f;
	if (buttonName.AfterLast('|') == "btnMinus")
		scale = 0.99f;
	else if (buttonName.AfterLast('|') == "btnPlus")
		scale = 1.01f;

	if (scale != 0.0f) {
		for (auto &i : selectedItems) {
			vector<Vector3> verts;
			project->ScaleMorphResult(i->shapeName, activeSlider, scale);
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}
		return;
	}

	if (activeSlider != clickedName)
		EnterSliderEdit(clickedName);
	else
		ExitSliderEdit();
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

void OutfitStudio::OnTabButtonClick(wxCommandEvent& event) {
	int id = event.GetId();
	wxMenuBar* menuBar = GetMenuBar();

	if (id != XRCID("boneTabButton")) {
		if (boneScale)
			boneScale->Show(false);

		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		if (boneScaleLabel)
			boneScaleLabel->Show(false);

		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		if (cbFixedWeight)
			cbFixedWeight->Show(false);

		project->ClearBoneScale();

		glView->SetXMirror(previousMirror);
		glView->SetActiveBrush(1);
		glView->SetWeightVisible(false);

		if (menuBar) {
			menuBar->Check(XRCID("btnXMirror"), previousMirror);
			menuBar->Check(XRCID("btnInflateBrush"), true);
			menuBar->Enable(XRCID("btnWeightBrush"), false);
			menuBar->Enable(XRCID("btnInflateBrush"), true);
			menuBar->Enable(XRCID("btnDeflateBrush"), true);
			menuBar->Enable(XRCID("btnMoveBrush"), true);
			menuBar->Enable(XRCID("btnSmoothBrush"), true);
		}

		if (toolBar) {
			toolBar->ToggleTool(XRCID("btnInflateBrush"), true);
			toolBar->EnableTool(XRCID("btnWeightBrush"), false);
			toolBar->EnableTool(XRCID("btnInflateBrush"), true);
			toolBar->EnableTool(XRCID("btnDeflateBrush"), true);
			toolBar->EnableTool(XRCID("btnMoveBrush"), true);
			toolBar->EnableTool(XRCID("btnSmoothBrush"), true);
		}
	}

	if (id == XRCID("meshTabButton")) {
		if (outfitBones)
			outfitBones->Hide();
		if (lightSettings)
			lightSettings->Hide();
		if (outfitShapes)
			outfitShapes->Show();

		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		if (boneTabButton)
			boneTabButton->SetCheck(false);

		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");
		if (lightsTabButton)
			lightsTabButton->SetCheck(false);
	}
	else if (id == XRCID("boneTabButton")) {
		if (outfitShapes)
			outfitShapes->Hide();
		if (lightSettings)
			lightSettings->Hide();
		if (outfitBones)
			outfitBones->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		if (meshTabButton)
			meshTabButton->SetCheck(false);

		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");
		if (lightsTabButton)
			lightsTabButton->SetCheck(false);

		if (boneScale) {
			boneScale->SetValue(0);
			boneScale->Show();
		}
		
		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		if (boneScaleLabel)
			boneScaleLabel->Show();

		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		if (cbFixedWeight)
			cbFixedWeight->Show();

		glView->SetActiveBrush(10);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetEditMode();
		glView->SetWeightVisible(true);

		if (menuBar) {
			menuBar->Check(XRCID("btnWeightBrush"), true);
			menuBar->Check(XRCID("btnXMirror"), false);
			menuBar->Enable(XRCID("btnWeightBrush"), true);
			menuBar->Enable(XRCID("btnInflateBrush"), false);
			menuBar->Enable(XRCID("btnDeflateBrush"), false);
			menuBar->Enable(XRCID("btnMoveBrush"), false);
			menuBar->Enable(XRCID("btnSmoothBrush"), false);
		}

		if (toolBar) {
			toolBar->ToggleTool(XRCID("btnWeightBrush"), true);
			toolBar->EnableTool(XRCID("btnWeightBrush"), true);
			toolBar->EnableTool(XRCID("btnInflateBrush"), false);
			toolBar->EnableTool(XRCID("btnDeflateBrush"), false);
			toolBar->EnableTool(XRCID("btnMoveBrush"), false);
			toolBar->EnableTool(XRCID("btnSmoothBrush"), false);
		}
	}
	else if (id == XRCID("lightsTabButton")) {
		if (outfitShapes)
			outfitShapes->Hide();
		if (outfitBones)
			outfitBones->Hide();
		if (lightSettings)
			lightSettings->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		if (meshTabButton)
			meshTabButton->SetCheck(false);

		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		if (boneTabButton)
			boneTabButton->SetCheck(false);
	}

	CheckBrushBounds();
	UpdateBrushPane();

	wxPanel* topSplitPanel = (wxPanel*)FindWindowByName("topSplitPanel");
	if (topSplitPanel)
		topSplitPanel->Layout();

	wxPanel* bottomSplitPanel = (wxPanel*)FindWindowByName("bottomSplitPanel");
	if (bottomSplitPanel)
		bottomSplitPanel->Layout();

	Refresh();
}

void OutfitStudio::HighlightSlider(const string& name) {
	for (auto &d : sliderDisplays) {
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
			sliderDisplays[project->GetSliderName(s)]->slider->SetValue(0);
		}
		ApplySliders();
	}
}

void OutfitStudio::OnSlider(wxScrollEvent& event) {
	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	string sliderName = s->GetName();
	if (sliderName == "brushSize" || sliderName == "brushStrength" || sliderName == "brushFocus" || sliderName == "brushSpacing")
		return;

	if (sliderName != "boneScale") {
		project->ClearBoneScale(false);
		if (boneScale)
			boneScale->SetValue(0);
	}

	if (outfitBones && sliderName == "boneScale") {
		wxArrayTreeItemIds selItems;
		outfitBones->GetSelections(selItems);
		if (selItems.size() > 0) {
			string selectedBone = outfitBones->GetItemText(selItems.front());
			project->ApplyBoneScale(selectedBone, event.GetPosition(), true);
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

	presets.LoadPresets("SliderPresets", choice, names, true);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto &n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		if (dlg.ShowModal() != wxID_OK)
			return;

		choice = presetChoice->GetStringSelection();
		if (XRCCTRL(dlg, "weightLo", wxRadioButton)->GetValue())
			hi = false;


		if (choice == "Zero All") {
			ZeroSliders();
			wxLogMessage("Resetting sliders to zero.");
			return;
		}

		wxLogMessage("Applying preset '%s' with option '%s'.", choice, hi ? "High" : "Low");

		float v;
		bool r;
		for (int i = 0; i < project->SliderCount(); i++) {
			if (!presets.GetSliderExists(choice, project->GetSliderName(i)))
				continue;

			if (project->SliderClamp(i))
				continue;

			if (hi)
				r = presets.GetBigPreset(choice, project->GetSliderName(i), v);
			else
				r = presets.GetSmallPreset(choice, project->GetSliderName(i), v);

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

void OutfitStudio::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	vector<string> sliders;
	project->GetSliderList(sliders);
	if (sliders.empty()) {
		wxMessageBox("There are no sliders loaded!", "Error", wxICON_ERROR, this);
		return;
	}

	SliderSetGroupCollection groupCollection;
	groupCollection.LoadGroups("SliderGroups");

	set<string> allGroups;
	groupCollection.GetAllGroups(allGroups);

	PresetSaveDialog psd(this);
	psd.allGroupNames.assign(allGroups.begin(), allGroups.end());
	psd.FilterGroups();

	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	string fileName = psd.outFileName;
	string presetName = psd.outPresetName;

	vector<string> groups;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	bool addedSlider = false;
	PresetCollection presets;
	for (auto &s : sliders) {
		int index = project->SliderIndexFromName(s);
		if (project->SliderZap(index) || project->SliderHidden(index))
			continue;

		float value = project->SliderValue(index);
		if (project->SliderInvert(index))
			value = 1.0f - value;

		if (project->SliderDefault(index, true) == value)
			continue;

		presets.SetSliderPreset(presetName, s, value, -10000.0f);

		if (!addedSlider)
			addedSlider = true;
	}

	if (!addedSlider) {
		wxLogMessage("No changes were made to the sliders, so no preset was saved!");
		wxMessageBox("No changes were made to the sliders, so no preset was saved!", "Info", wxICON_INFORMATION, this);
		return;
	}

	int error = presets.SavePreset(fileName, presetName, "OutfitStudio", groups);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format("Failed to save preset (%d)!", error), "Error", wxICON_ERROR, this);
	}
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

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", activeSlider, activeItem->shapeName, fn);
	project->SetSliderFromBSD(activeSlider, activeItem->shapeName, fn);
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

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromOBJ(activeSlider, activeItem->shapeName, fn)) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox("Vertex count of .obj file mesh does not match currently selected shape!", "Error", wxICON_ERROR);
		return;
	}

	ApplySliders();
}

void OutfitStudio::OnSliderImportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox("There are no valid shapes loaded!", "Error");
		return;
	}

	string fn = wxFileSelector("Import .tri morphs", wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	int result = wxMessageBox("This will delete all loaded sliders. Are you sure?", "TRI Import", wxYES_NO | wxCANCEL | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Importing morphs from TRI file '%s'...", fn);

	TriFile tri;
	if (!tri.Read(fn)) {
		wxLogError("Failed to load TRI file '%s'!", fn);
		wxMessageBox("Failed to load TRI file!", "Error", wxICON_ERROR);
		return;
	}

	// Deleting sliders
	sliderScroll->Freeze();
	vector<string> erase;
	for (auto &sd : sliderDisplays) {
		sd.second->slider->SetValue(0);
		SetSliderValue(sd.first, 0);
		ShowSliderEffect(sd.first, true);
		sd.second->sliderStrokes.Clear();
		sd.second->slider->SetFocus();

		sd.second->btnSliderEdit->Destroy();
		sd.second->slider->Destroy();
		sd.second->sliderName->Destroy();
		sd.second->sliderNameCheck->Destroy();
		sd.second->sliderReadout->Destroy();
		sd.second->sliderPane->Destroy();
		delete sd.second;

		erase.push_back(sd.first);
		project->DeleteSlider(sd.first);
	}

	for (auto &e : erase)
		sliderDisplays.erase(e);

	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();
	sliderScroll->FitInside();
	activeSlider = "";

	vector<string> shapes;
	project->GetShapes(shapes);

	wxString addedMorphs;
	auto morphs = tri.GetMorphs();
	for (auto &morph : morphs) {
		if (find(shapes.begin(), shapes.end(), morph.first) == shapes.end())
			continue;

		addedMorphs += morph.first + "\n";
		for (auto &morphData : morph.second) {
			string finalName = project->NameAbbreviate(morphData->name);
			if (!project->ValidSlider(finalName)) {
				createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(finalName);
				ShowSliderEffect(finalName);
			}

			unordered_map<ushort, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			project->SetSliderFromTRI(morphData->name, morph.first, diff);
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();
	ApplySliders();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedMorphs);
	wxMessageBox(wxString::Format("Added morphs for the following shapes:\n\n%s", addedMorphs), "TRI Import");
}

void OutfitStudio::OnSliderImportFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to import data to!", "Error");
		return;
	}

	string fn = wxFileSelector("Import .fbx file for slider calculation", wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromFBX(activeSlider, activeItem->shapeName, fn)) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox("Vertex count of .obj file mesh does not match currently selected shape!", "Error", wxICON_ERROR);
		return;
	}

	ApplySliders();
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
		string dir = wxDirSelector("Export .bsd slider data to directory", wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		for (auto &i : selectedItems) {
			string targetFile = dir + "\\" + i->shapeName + "_" + activeSlider + ".bsd";
			wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderBSD(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		string fn = wxFileSelector("Export .bsd slider data", wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.empty())
			return;

		wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		project->SaveSliderBSD(activeSlider, activeItem->shapeName, fn);
	}

	ApplySliders();
}

void OutfitStudio::OnSliderExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to export data from!", "Error");
		return;
	}

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector("Export .obj slider data to directory", wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		for (auto &i : selectedItems) {
			string targetFile = dir + "\\" + i->shapeName + "_" + activeSlider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderOBJ(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		string fn = wxFileSelector("Export .obj slider data", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.empty())
			return;

		wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		if (project->SaveSliderOBJ(activeSlider, activeItem->shapeName, fn)) {
			wxLogError("Failed to export OBJ file '%s'!", fn);
			wxMessageBox("Failed to export OBJ file!", "Error", wxICON_ERROR);
		}
		statusBar->SetStatusText("Ready!");
	}

	ApplySliders();
}

void OutfitStudio::OnSliderExportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox("There are no valid shapes loaded!", "Error");
		return;
	}

	string fn = wxFileSelector("Export .tri morphs", wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	wxLogMessage("Exporting TRI morphs to '%s'...", fn);
	if (!project->WriteMorphTRI(fn)) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox("Failed to export TRI file!", "Error", wxICON_ERROR);
		return;
	}
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

	if (selectedItems.size() > 1)
		wxLogMessage("Clearing slider data of '%s' for the selected shapes.", activeSlider);
	else
		wxLogMessage("Clearing slider data of '%s' for the shape '%s'.", activeSlider, activeItem->shapeName);

	unordered_map<ushort, float> mask;
	for (auto &i : selectedItems) {
		mask.clear();
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			project->ClearUnmaskedDiff(i->shapeName, activeSlider, &mask);
		else
			project->ClearSlider(i->shapeName, activeSlider);
	}

	ApplySliders();
}

void OutfitStudio::OnNewSlider(wxCommandEvent& WXUNUSED(event)) {
	NewSlider();
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
	wxLogMessage("Creating new zap '%s'.", finalName);

	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	unordered_map<ushort, float> unmasked;
	for (auto &i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->shapeName);
		project->AddZapSlider(finalName, unmasked, i->shapeName);
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
	wxLogMessage("Creating new combined slider '%s'.", finalName);

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

	wxLogMessage("Negating slider for the selected shapes.");
	for (auto &i : selectedItems)
		project->NegateSlider(activeSlider, i->shapeName);
}

void OutfitStudio::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox("There is no slider in edit mode to delete!", "Error");
		return;
	}

	string prompt = "Are you sure you wish to delete the slider \"";
	prompt += activeSlider + "\"? This cannot be undone.";
	int result = wxMessageBox(prompt, "Confirm slider delete", wxYES_NO | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Deleting slider '%s'.", activeSlider);

	SliderDisplay* sd = sliderDisplays[activeSlider];
	sd->slider->SetValue(0);
	SetSliderValue(activeSlider, 0);
	ShowSliderEffect(activeSlider, true);
	//sd->sliderStrokes.InvalidateHistoricalBVH();
	sd->sliderStrokes.Clear();
	sd->slider->SetFocus();
	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();

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

		if (!project->mGenWeights) {
			XRCCTRL(dlg, "lbValLo", wxStaticText)->Hide();
			XRCCTRL(dlg, "edValLo", wxTextCtrl)->Hide();
			XRCCTRL(dlg, "lbValHi", wxStaticText)->SetLabel("Default");
		}

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
	project->GetShapes(shapes);
	if (shapes.size() - 1 == 0 || project->GetBaseShape().empty())
		return;

	if (!outfitShapes)
		return;

	wxLogMessage("Conforming all shapes...");
	StartProgress("Conforming all shapes...");
	int inc = 100 / shapes.size() - 1;
	int pos = 0;

	auto selectedItemsSave = selectedItems;

	curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		selectedItems.clear();
		selectedItems.push_back((ShapeItemData*)outfitShapes->GetItemData(curItem));
		UpdateProgress(pos * inc, "Conforming: " + selectedItems.front()->shapeName);
		StartSubProgress(pos * inc, pos * inc + inc);
		OnSliderConform(event);
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
		pos++;
	}

	selectedItems = selectedItemsSave;

	if (statusBar)
		statusBar->SetStatusText("All shapes conformed.");

	wxLogMessage("All shapes conformed.");
	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	StartProgress("Conforming...");
	if (project->GetBaseShape().empty()) {
		EndProgress();
		return;
	}

	wxLogMessage("Conforming...");
	ZeroSliders();

	for (auto &i : selectedItems) {
		if (project->IsBaseShape(i->shapeName))
			continue;

		wxLogMessage("Conforming '%s'...", i->shapeName);
		UpdateProgress(1, "Initializing data...");
		project->InitConform();

		UpdateProgress(50, "Conforming: " + i->shapeName);

		project->morpher.CopyMeshMask(glView->GetMesh(i->shapeName), i->shapeName);
		project->ConformShape(i->shapeName);

		UpdateProgress(99);
	}

	if (statusBar)
		statusBar->SetStatusText("Shape(s) conformed.");

	wxLogMessage("%d shape(s) conformed.", selectedItems.size());
	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::OnImportShape(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector("Import .obj file for new shape", wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing shape from OBJ file '%s'...", fn);

	int ret;
	if (activeItem)
		ret = project->AddShapeFromObjFile(fn, project->OutfitName(), activeItem->shapeName);
	else
		ret = project->AddShapeFromObjFile(fn, project->OutfitName());

	if (ret == 101) {	// User chose to merge shapes
		vector<Vector3> v;
		project->GetLiveVerts(activeItem->shapeName, v);
		glView->UpdateMeshVertices(activeItem->shapeName, &v);
		return;
	}
	else if (ret)
		return;

	wxLogMessage("Imported shape.");

	RefreshGUIFromProj();
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
		for (auto &i : selectedItems) {
			string targetFile = dir + "\\" + i->shapeName + ".obj";
			wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", i->shapeName, targetFile);
			project->ExportShapeObj(targetFile, i->shapeName, Vector3(0.1f, 0.1f, 0.1f));
		}
	}
	else {
		string fileName = wxFileSelector("Export shape as an .obj file", wxEmptyString, wxString(activeItem->shapeName + ".obj"), "", "Obj Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (!fileName.empty()) {
			wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", activeItem->shapeName, fileName);
			project->ClearBoneScale();
			if (project->ExportShapeObj(fileName, activeItem->shapeName, Vector3(0.1f, 0.1f, 0.1f))) {
				wxLogError("Failed to export OBJ file '%s'!", fileName);
				wxMessageBox("Failed to export OBJ file!", "Error", wxICON_ERROR);
			}
		}
	}
}

void OutfitStudio::OnImportFBX(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector("Import .fbx file for new shape", wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing shape from FBX file '%s'...", fn);

	int ret;
	if (activeItem)
		ret = project->ImportShapeFBX(fn, project->OutfitName(), activeItem->shapeName);
	else
		ret = project->ImportShapeFBX(fn, project->OutfitName());

	if (ret == 101) {
		vector<Vector3> v;
		project->GetLiveVerts(activeItem->shapeName, v);
		glView->UpdateMeshVertices(activeItem->shapeName, &v);
		return;
	}
	if (ret)
		return;

	wxLogMessage("Imported shape.");

	RefreshGUIFromProj();
	glView->Refresh();
}

void OutfitStudio::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
	string defFile = "OutfitStudioShapes.fbx";
	if (activeItem)
		defFile = activeItem->shapeName + ".fbx";

	string fn = wxFileSelector("Export .fbx file for new shape", wxEmptyString, defFile, "", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	if (activeItem)
		project->ExportShapeFBX(fn, activeItem->shapeName);
	else
		project->ExportShapeFBX(fn, "");
}

void OutfitStudio::OnInvertUV(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	bool invertX = (event.GetId() == XRCID("uvInvertX"));
	bool invertY = (event.GetId() == XRCID("uvInvertY"));

	for (auto &i : selectedItems)
		project->GetWorkNif()->InvertUVsForShape(i->shapeName, invertX, invertY);

	RefreshGUIFromProj();
}

void OutfitStudio::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	string newShapeName;
	do {
		wxString result = wxGetTextFromUser("Please enter a new unique name for the shape.", "Rename Shape");
		if (result.empty())
			return;

		newShapeName = result;
	} while (project->IsValidShape(newShapeName));

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	for (uint i = 0; i < sizeof(chars); ++i)
		replace(newShapeName.begin(), newShapeName.end(), chars[i], '_');

	wxLogMessage("Renaming shape '%s' to '%s'.", activeItem->shapeName, newShapeName);
	project->RenameShape(activeItem->shapeName, newShapeName);
	glView->RenameShape(activeItem->shapeName, newShapeName);

	activeItem->shapeName = newShapeName;
	outfitShapes->SetItemText(activeItem->GetId(), newShapeName);
}

void OutfitStudio::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (project->IsBaseShape(activeItem->shapeName)) {
		wxMessageBox("The selected shape is the reference shape already.", "Error");
		return;
	}

	project->SetBaseShape(activeItem->shapeName);
	RefreshGUIFromProj();
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

void OutfitStudio::OnMoveShape(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	Vector3 offs;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMoveShape")) {
		XRCCTRL(dlg, "msOldOffset", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudio::OnMoveShapeOldOffset, this);
		XRCCTRL(dlg, "msSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnMoveShapeSlider, this);
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
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			Vector3 d;
			float diffX, diffY, diffZ;
			if (bEditSlider) {
				diff.clear();
				int vertexCount = project->GetVertexCount(i->shapeName);
				for (int j = 0; j < vertexCount; j++) {
					d = (offs - previewMove) * (1.0f - mask[j]);
					diff[j] = d;
					diffX = diff[j].x / -10.0f;
					diffY = diff[j].y / 10.0f;
					diffZ = diff[j].z / 10.0f;
					diff[j].z = diffY;
					diff[j].y = diffZ;
					diff[j].x = diffX;
				}
				project->UpdateMorphResult(i->shapeName, activeSlider, diff);
			}
			else {
				d = offs - previewMove;
				project->OffsetShape(i->shapeName, d, mptr);
			}

			vector<Vector3> verts;
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}
		previewMove.Zero();
	}
}

void OutfitStudio::OnMoveShapeOldOffset(wxCommandEvent& event) {
	wxWindow* parent = ((wxButton*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	XRCCTRL(*parent, "msSliderX", wxSlider)->SetValue(0);
	XRCCTRL(*parent, "msSliderY", wxSlider)->SetValue(-2544);
	XRCCTRL(*parent, "msSliderZ", wxSlider)->SetValue(3287);
	XRCCTRL(*parent, "msTextX", wxTextCtrl)->SetValue("0.00000");
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->SetValue("-2.54431");
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->SetValue("3.28790");

	PreviewMove(Vector3(0.00000f, -2.54431f, 3.28790f));
}

void OutfitStudio::OnMoveShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "msSliderX", wxSlider)->GetValue() / 1000.0f;
	slider.y = XRCCTRL(*parent, "msSliderY", wxSlider)->GetValue() / 1000.0f;
	slider.z = XRCCTRL(*parent, "msSliderZ", wxSlider)->GetValue() / 1000.0f;

	XRCCTRL(*parent, "msTextX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.x));
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.y));
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", slider.z));

	PreviewMove(slider);
}

void OutfitStudio::OnMoveShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;
	
	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "msTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "msTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "msTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	XRCCTRL(*parent, "msSliderX", wxSlider)->SetValue(changed.x * 1000);
	XRCCTRL(*parent, "msSliderY", wxSlider)->SetValue(changed.y * 1000);
	XRCCTRL(*parent, "msSliderZ", wxSlider)->SetValue(changed.z * 1000);

	PreviewMove(changed);
}

void OutfitStudio::PreviewMove(const Vector3& changed) {
	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	unordered_map<ushort, Vector3> diff;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		Vector3 d;
		float diffX, diffY, diffZ;
		if (bEditSlider) {
			int vertexCount = project->GetVertexCount(i->shapeName);
			for (int j = 0; j < vertexCount; j++) {
				d = (changed - previewMove) * (1.0f - mask[j]);
				diff[j] = d;
				diffX = diff[j].x / -10.0f;
				diffY = diff[j].y / 10.0f;
				diffZ = diff[j].z / 10.0f;
				diff[j].z = diffY;
				diff[j].y = diffZ;
				diff[j].x = diffX;
			}
			project->UpdateMorphResult(i->shapeName, activeSlider, diff);
		}
		else {
			d = changed - previewMove;
			project->OffsetShape(i->shapeName, d, mptr);
		}

		vector<Vector3> verts;
		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}
	previewMove = changed;
}

void OutfitStudio::OnScaleShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		XRCCTRL(dlg, "ssSlider", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssText", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnScaleShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		float scale;
		if (dlg.ShowModal() == wxID_OK)
			scale = atof(XRCCTRL(dlg, "ssText", wxTextCtrl)->GetValue().ToAscii().data());
		else
			scale = 1.0f;

		scale *= 1.0f / previewScale;

		unordered_map<ushort, float> mask;
		unordered_map<ushort, float>* mptr = nullptr;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			vector<Vector3> verts;
			project->ScaleShape(i->shapeName, scale, mptr);
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewScale = 1.0f;
	}
}

void OutfitStudio::OnScaleShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	float scale = 1.0f;
	scale = XRCCTRL(*parent, "ssSlider", wxSlider)->GetValue() / 1000.0f;
	XRCCTRL(*parent, "ssText", wxTextCtrl)->SetValue(wxString::Format("%0.5f", scale));

	PreviewScale(scale);
}

void OutfitStudio::OnScaleShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	float scale;
	scale = atof(XRCCTRL(*parent, "ssText", wxTextCtrl)->GetValue().ToAscii().data());

	if (scale < 0.01f) {
		scale = 0.01f;
		XRCCTRL(*parent, "ssText", wxTextCtrl)->SetValue(wxString::Format("%0.5f", scale));
	}
	XRCCTRL(*parent, "ssSlider", wxSlider)->SetValue(scale * 1000);

	PreviewScale(scale);
}

void OutfitStudio::PreviewScale(const float& scale) {
	float scaleNew = scale * 1.0f / previewScale;

	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		vector<Vector3> verts;
		project->ScaleShape(i->shapeName, scaleNew, mptr);
		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewScale = scale;
}

void OutfitStudio::OnRotateShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotateShape")) {
		XRCCTRL(dlg, "rsSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnRotateShapeText, this);
		XRCCTRL(dlg, "rsTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnRotateShapeText, this);
		XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnRotateShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		Vector3 angle;
		if (dlg.ShowModal() == wxID_OK) {
			angle.x = atof(XRCCTRL(dlg, "rsTextX", wxTextCtrl)->GetValue().ToAscii().data());
			angle.y = atof(XRCCTRL(dlg, "rsTextY", wxTextCtrl)->GetValue().ToAscii().data());
			angle.z = atof(XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->GetValue().ToAscii().data());
		}

		angle -= previewRotation;

		unordered_map<ushort, float> mask;
		unordered_map<ushort, float>* mptr = nullptr;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			vector<Vector3> verts;
			project->RotateShape(i->shapeName, angle, mptr);
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewRotation.Zero();
	}
}

void OutfitStudio::OnRotateShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "rsSliderX", wxSlider)->GetValue() / 100.0f;
	slider.y = XRCCTRL(*parent, "rsSliderY", wxSlider)->GetValue() / 100.0f;
	slider.z = XRCCTRL(*parent, "rsSliderZ", wxSlider)->GetValue() / 100.0f;

	XRCCTRL(*parent, "rsTextX", wxTextCtrl)->SetValue(wxString::Format("%0.4f", slider.x));
	XRCCTRL(*parent, "rsTextY", wxTextCtrl)->SetValue(wxString::Format("%0.4f", slider.y));
	XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->SetValue(wxString::Format("%0.4f", slider.z));

	PreviewRotation(slider);
}

void OutfitStudio::OnRotateShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "rsTextX", wxTextCtrl)->GetValue().ToAscii().data());
	changed.y = atof(XRCCTRL(*parent, "rsTextY", wxTextCtrl)->GetValue().ToAscii().data());
	changed.z = atof(XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->GetValue().ToAscii().data());

	XRCCTRL(*parent, "rsSliderX", wxSlider)->SetValue(changed.x * 100);
	XRCCTRL(*parent, "rsSliderY", wxSlider)->SetValue(changed.y * 100);
	XRCCTRL(*parent, "rsSliderZ", wxSlider)->SetValue(changed.z * 100);

	PreviewRotation(changed);
}


void OutfitStudio::PreviewRotation(const Vector3& changed) {
	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		vector<Vector3> verts;
		project->RotateShape(i->shapeName, changed - previewRotation, mptr);
		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}
	previewRotation = changed;
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

		char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
		for (uint i = 0; i < sizeof(chars); ++i)
			replace(newname.begin(), newname.end(), chars[i], '_');

		wxLogMessage("Duplicating shape '%s' as '%s'.", activeItem->shapeName, newname);
		project->ClearBoneScale();

		mesh* curshapemesh = glView->GetMesh(activeItem->shapeName);
		project->DuplicateShape(activeItem->shapeName, newname, curshapemesh);

		glView->AddMeshFromNif(project->GetWorkNif(), newname, false);
		project->SetTexture(newname, "_AUTO_");

		glView->SetMeshTexture(newname, project->GetShapeTexture(newname), project->GetWorkNif()->IsShaderSkin(newname));

		subitem = outfitShapes->AppendItem(outfitRoot, newname);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(project->GetWorkNif(), newname));
	}
}

void OutfitStudio::OnDeleteShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (wxMessageBox("Are you sure you wish to delete the selected shapes?  This action cannot be undone.", "Confirm Delete", wxYES_NO) == wxNO)
		return;

	vector<ShapeItemData> selected;
	for (auto &i : selectedItems)
		selected.push_back(*i);

	for (auto &i : selected) {
		if (!project->IsBaseShape(i.shapeName)) {
			wxLogMessage("Deleting shape '%s'.", i.shapeName);
			project->DeleteShape(i.shapeName);
			glView->DeleteMesh(i.shapeName);
			wxTreeItemId item = i.GetId();
			outfitShapes->Delete(item);
		}
		else
			wxMessageBox("You can't delete the reference shape!");
	}

	AnimationGUIFromProj();
}

void OutfitStudio::OnAddBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones"))
		return;

	dlg.SetSize(450, 470);
	dlg.CenterOnParent();

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	function<void(wxTreeItemId, AnimBone*)> fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
		for (auto &cb : boneParent->children) {
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
			string bone = boneTree->GetItemText(sel[i]);
			wxLogMessage("Adding bone '%s' to project.", bone);

			project->AddBoneRef(bone);
			outfitBones->AppendItem(bonesRoot, bone);
		}
	}
}

void OutfitStudio::OnAddCustomBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone")) {
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			wxString bone = XRCCTRL(dlg, "boneName", wxTextCtrl)->GetValue();
			if (bone.empty()) {
				wxMessageBox("No bone name was entered!", "Error", wxICON_INFORMATION, this);
				return;
			}

			wxTreeItemIdValue cookie;
			wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
			while (item.IsOk()) {
				if (outfitBones->GetItemText(item) == bone) {
					wxMessageBox(wxString::Format("Bone '%s' already exists in the project!", bone), "Error", wxICON_INFORMATION, this);
					return;
				}
				item = outfitBones->GetNextChild(bonesRoot, cookie);
			}

			Vector3 translation;
			translation.x = atof(XRCCTRL(dlg, "textX", wxTextCtrl)->GetValue().ToAscii().data());
			translation.y = atof(XRCCTRL(dlg, "textY", wxTextCtrl)->GetValue().ToAscii().data());
			translation.z = atof(XRCCTRL(dlg, "textZ", wxTextCtrl)->GetValue().ToAscii().data());

			wxLogMessage("Adding custom bone '%s' to project.", bone);
			project->AddCustomBoneRef(bone.ToStdString(), translation);
			outfitBones->AppendItem(bonesRoot, bone);
		}
	}
}

void OutfitStudio::OnDeleteBone(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (int i = 0; i < selItems.size(); i++) {
		string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting bone '%s' from project.", bone);

		project->DeleteBone(bone);
		activeBone = "";

		outfitBones->Delete(selItems[i]);
	}

	outfitBones->GetSelections(selItems);
	if (!selItems.empty()) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems[0]);
		OnBoneSelect(treeEvent);
	}
}

void OutfitStudio::OnDeleteBoneFromSelected(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (int i = 0; i < selItems.size(); i++) {
		string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting weights of bone '%s' from selected shapes.", bone);

		for (auto &s : selectedItems)
			project->GetWorkAnim()->RemoveShapeBone(s->shapeName, bone);
	}

	if (!selItems.empty()) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems[0]);
		OnBoneSelect(treeEvent);
	}
}

void OutfitStudio::OnCopyBoneWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (project->GetBaseShape().empty())
		return;

	StartProgress("Copying bone weights...");

	unordered_map<ushort, float> mask;
	for (int i = 0; i < selectedItems.size(); i++) {
		if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
			wxLogMessage("Copying bone weights to '%s'...", selectedItems[i]->shapeName);
			mask.clear();
			glView->GetShapeMask(mask, selectedItems[i]->shapeName);
			project->CopyBoneWeights(selectedItems[i]->shapeName, &mask);
		}
		else
			wxMessageBox("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape.", "Can't copy weights", wxICON_WARNING);
	}

	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!outfitBones)
		return;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (project->GetBaseShape().empty())
		return;

	vector<string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	string bonesString;
	for (int i = 0; i < selItems.size(); i++) {
		string boneName = outfitBones->GetItemText(selItems[i]).ToStdString();
		bonesString += "'" + boneName + "' ";
		selectedBones.push_back(boneName);
	}

	StartProgress("Copying selected bone weights...");

	unordered_map<ushort, float> mask;
	for (int i = 0; i < selectedItems.size(); i++) {
		if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
			wxLogMessage("Copying selected bone weights to '%s' for %s...", selectedItems[i]->shapeName, bonesString);
			mask.clear();
			glView->GetShapeMask(mask, selectedItems[i]->shapeName);
			project->CopyBoneWeights(selectedItems[i]->shapeName, &mask, &selectedBones);
		}
		else
			wxMessageBox("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape.", "Can't copy weights", wxICON_WARNING);
	}

	UpdateProgress(100, "Finished");
	EndProgress();
}

void OutfitStudio::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!outfitBones)
		return;

	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	if (project->GetBaseShape().empty())
		return;

	if (project->IsBaseShape(activeItem->shapeName)) {
		wxMessageBox("Sorry, you can't copy weights from the reference shape to itself.", "Error");
		return;
	}

	int baseVertCount = project->GetVertexCount(project->GetBaseShape());
	int workVertCount = project->GetVertexCount(activeItem->shapeName);
	if (baseVertCount != workVertCount) {
		wxMessageBox("The vertex count of the reference and chosen shape is not the same!", "Error");
		return;
	}

	vector<string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	string bonesString;
	for (int i = 0; i < selItems.size(); i++) {
		string boneName = outfitBones->GetItemText(selItems[i]).ToStdString();
		bonesString += "'" + boneName + "' ";
		selectedBones.push_back(boneName);
	}

	wxLogMessage("Transferring selected bone weights to '%s' for %s...", activeItem->shapeName, bonesString);
	StartProgress("Transferring bone weights...");

	unordered_map<ushort, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->shapeName, &mask, &selectedBones);

	EndProgress();
}

void OutfitStudio::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	unordered_map<ushort, float> boneWeights;
	for (auto &i : selectedItems) {
		mesh* m = glView->GetMesh(i->shapeName);
		if (!m)
			continue;

		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));
		auto& bones = project->GetWorkAnim()->shapeBones;
		if (bones.find(i->shapeName) != bones.end()) {
			for (auto &b : bones[i->shapeName]) {
				boneWeights.clear();
				project->GetWorkAnim()->GetWeights(i->shapeName, b, boneWeights);
				for (auto &bw : boneWeights)
					m->vcolors[bw.first].x = 1.0f;
			}
		}
	}

	glView->Refresh();
}

void OutfitStudio::OnBuildSkinPartitions(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	for (auto &i : selectedItems) {
		wxLogMessage("Building skin partitions for '%s'.", i->shapeName);
		project->BuildShapeSkinPartions(i->shapeName);
	}
}

void OutfitStudio::OnShapeProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox("There is no shape selected!", "Error");
		return;
	}

	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* shapeProperties = rsrc->LoadDialog(this, "dlgShapeProp");
	if (!shapeProperties)
		return;

	ShapeProperties prop(this, project->GetWorkNif(), activeItem->shapeName);
	prop.ShowModal();
}

void OutfitStudio::OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event) {
	string fn = event.GetPath();
	vector<string> shapes;
	wxWindow* npWiz = ((wxFilePickerCtrl*)event.GetEventObject())->GetParent();
	wxChoice* setNameChoice = (wxChoice*)XRCCTRL((*npWiz), "npSliderSetName", wxChoice);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	XRCCTRL((*npWiz), "npRefIsSliderset", wxRadioButton)->SetValue(true);
	setNameChoice->Clear();
	refShapeChoice->Clear();

	if (fn.rfind(".osp") != string::npos || fn.rfind(".xml") != string::npos) {
		SliderSetFile ssf(fn);
		if (ssf.fail())
			return;

		vector<string> setNames;
		ssf.GetSetNames(setNames);

		for (auto &sn : setNames)
			setNameChoice->AppendString(sn);

		if (setNames.size() > 0) {
			setNameChoice->SetSelection(0);
			ssf.SetShapes(setNames[0], shapes);
			for (auto &rsn : shapes)
				refShapeChoice->AppendString(rsn);

			refShapeChoice->SetSelection(0);
		}
	}
	else if (fn.rfind(".nif") != string::npos) {
		NifFile checkFile;
		if (checkFile.Load(fn))
			return;

		checkFile.GetShapeList(shapes);
		for (auto &rsn : shapes)
			refShapeChoice->AppendString(rsn);

		refShapeChoice->SetSelection(0);
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
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refShapeChoice->Clear();

	for (auto &rsn : shapes)
		refShapeChoice->AppendString(rsn);

	refShapeChoice->SetSelection(0);
}

void OutfitStudio::OnBrushSettingsSlider(wxScrollEvent& WXUNUSED(event)) {
	TweakBrush* brush = glView->GetActiveBrush();
	wxCollapsiblePane* parent = (wxCollapsiblePane*)FindWindowByName("brushPane");
	if (!parent)
		return;

	wxStaticText* valSize = (wxStaticText*)XRCCTRL(*parent, "valSize", wxStaticText);
	wxStaticText* valStrength = (wxStaticText*)XRCCTRL(*parent, "valStr", wxStaticText);
	wxStaticText* valFocus = (wxStaticText*)XRCCTRL(*parent, "valFocus", wxStaticText);
	wxStaticText* valSpacing = (wxStaticText*)XRCCTRL(*parent, "valSpace", wxStaticText);

	float slideSize = XRCCTRL(*parent, "brushSize", wxSlider)->GetValue() / 1000.0f;
	float slideStr = XRCCTRL(*parent, "brushStr", wxSlider)->GetValue() / 1000.0f;
	float slideFocus = XRCCTRL(*parent, "brushFocus", wxSlider)->GetValue() / 1000.0f;
	float slideSpace = XRCCTRL(*parent, "brushSpace", wxSlider)->GetValue() / 1000.0f;

	wxString valSizeStr = wxString::Format("%0.3f", slideSize);
	wxString valStrengthStr = wxString::Format("%0.3f", slideStr);
	wxString valFocusStr = wxString::Format("%0.3f", slideFocus);
	wxString valSpacingStr = wxString::Format("%0.3f", slideSpace);

	valSize->SetLabel(valSizeStr);
	valStrength->SetLabel(valStrengthStr);
	valFocus->SetLabel(valFocusStr);
	valSpacing->SetLabel(valSpacingStr);

	if (brush) {
		glView->SetBrushSize(slideSize);
		brush->setStrength(slideStr);
		brush->setFocus(slideFocus);
		brush->setSpacing(slideSpace);
		CheckBrushBounds();
	}
}

// ---------------------------------------------------------------------------
// wxGLPanel
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(wxGLPanel, wxGLCanvas)
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
wxEND_EVENT_TABLE()

wxGLPanel::wxGLPanel(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs)
	: wxGLCanvas(parent, attribs, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE) {

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
	bAutoNormals = true;
	bXMirror = true;
	bConnectedEdit = false;
	bGlobalBrushCollision = true;

	strokeManager = &baseStrokes;
}

wxGLPanel::~wxGLPanel() {
	delete context;
}

void wxGLPanel::OnShown() {
	gls.Initialize(this, context);
	auto size = GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight());
	gls.ToggleMask();

	int ambient = Config.GetIntValue("Lights/Ambient");
	int brightness1 = Config.GetIntValue("Lights/Brightness1");
	int brightness2 = Config.GetIntValue("Lights/Brightness2");
	int brightness3 = Config.GetIntValue("Lights/Brightness3");
	UpdateLights(ambient, brightness1, brightness2, brightness3);
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	os = dynamic_cast<OutfitStudio*>(win);
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, const string& shapeName, bool buildNormals) {
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

		if (buildNormals)
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

void wxGLPanel::SetMeshTexture(const string& shapeName, const string& texturefile, bool isSkin) {
	mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	GLMaterial* mat;
	if (!isSkin)
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\defshader.fs");
	else
		mat = gls.AddMaterial(texturefile, "res\\maskvshader.vs", "res\\skinshader.fs");

	m->material = mat;
}

void wxGLPanel::UpdateMeshVertices(const string& shapeName, vector<Vector3>* verts, bool updateBVH, bool recalcNormals) {
	int id = gls.GetMeshID(shapeName);
	gls.Update(id, verts);
	if (updateBVH)
		BVHUpdateQueue.insert(id);

	if (recalcNormals)
		RecalcNormals(shapeName);

	gls.RenderOneFrame();
}

void wxGLPanel::RecalculateMeshBVH(const string& shapeName) {
	gls.RecalculateMeshBVH(gls.GetMeshID(shapeName));
}

void wxGLPanel::ShowShape(const string& shapeName, bool show) {
	int id = gls.GetMeshID(shapeName);
	gls.SetMeshVisibility(id, show);
	gls.RenderOneFrame();
}

void wxGLPanel::SetActiveShapes(const vector<string>& shapeNames) {
	gls.SetActiveMeshesID(shapeNames);
}

void wxGLPanel::SetSelectedShape(const string& shapeName) {
	gls.SetSelectedMesh(shapeName);
}

void wxGLPanel::SetActiveBrush(int brushID) {
	bMaskPaint = false;
	bWeightPaint = false;

	switch (brushID) {
	case -1:
		activeBrush = nullptr;
		break;
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
		wxPoint cursorPos(event.GetPosition());

		unordered_map<ushort, Vector3> diff;
		vector<Vector3> verts;
		Vector3 newPos;
		Vertex oldVertex;

		if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &oldVertex))
			return;

		if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
			os->project->GetLiveVerts(os->activeItem->shapeName, verts);
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
					os->project->UpdateMorphResult(os->activeItem->shapeName, os->activeSlider, diff);
				}
				else {
					os->project->MoveVertex(os->activeItem->shapeName, newPos, oldVertex.indexRef);
				}
				os->project->GetLiveVerts(os->activeItem->shapeName, verts);
				UpdateMeshVertices(os->activeItem->shapeName, &verts);
			}
		}
	}
	else if (event.GetKeyCode() == WXK_SPACE)
		os->ToggleBrushPane();

	event.Skip();
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	Vector3 o;
	Vector3 n;
	Vector3 v;
	Vector3 vo;
	Vector3 d;
	Vector3 s;
	bool meshHit;

	TweakPickInfo tpi;
	meshHit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, nullptr, bGlobalBrushCollision, &tpi.facet);
	if (!meshHit)
		return false;

	if (!os->NotifyStrokeStarting())
		return false;

	if (bXMirror) {
		gls.GetPickRay(screenPos.x, screenPos.y, d, s);
		d.x *= -1;
		s.x *= -1;
		if (!gls.CollideMeshes(screenPos.x, screenPos.y, o, n, nullptr, bGlobalBrushCollision, &tpi.facetM, &d, &s))
			tpi.facetM = -1;
	}

	n.Normalize();

	wxRect r = GetClientRect();
	gls.GetPickRay(screenPos.x, screenPos.y, v, vo);
	v = v * -1;
	tpi.view = v;

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

	if (activeBrush->Type() == TBT_WEIGHT) {
		for (auto &s : os->GetSelectedItems()) {
			int boneIndex = os->project->GetWorkAnim()->GetShapeBoneIndex(s->shapeName, os->GetActiveBone());
			if (boneIndex < 0)
				os->project->AddBoneRef(os->GetActiveBone());
		}
	}

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMeshes(), activeBrush);

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
		wxRect r = GetClientRect();
		int cx = r.GetWidth() / 2;
		int cy = r.GetHeight() / 2;
		gls.GetPickRay(cx, cy, v, vo);

		gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision);
		gls.RenderOneFrame();

		if (activeBrush->Type() == TBT_MOVE) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, nullptr, bGlobalBrushCollision, &tpi.facet);
			if (bXMirror) {
				gls.GetPickRay(screenPos.x, screenPos.y, d, s);
				d.x *= -1;
				s.x *= -1;
				if (!gls.CollideMeshes(screenPos.x, screenPos.y, o, n, nullptr, bGlobalBrushCollision, &tpi.facetM, &d, &s))
					tpi.facetM = -1;
			}
			tpi.normal.Normalize();
		}

		v = v * -1;
		tpi.view = v;
		activeStroke->updateStroke(tpi);

		if (activeBrush->Type() == TBT_WEIGHT) {
			if (os->boneScale) {
				string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos);
					os->ActiveShapesUpdated(strokeManager->GetCurStateStroke(), false, false);
				}
			}
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (activeStroke) {
		activeStroke->endStroke();

		if (activeStroke->BrushType() == TBT_WEIGHT) {
			if (os->boneScale) {
				string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
				}
			}
		}

		if (activeStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());

			if (!os->bEditSlider && activeStroke->BrushType() != TBT_WEIGHT) {
				vector<string> shapes;
				os->project->GetShapes(shapes);
				for (auto &s : shapes) {
					os->UpdateShapeSource(s);
					os->project->RefreshMorphShape(s);
				}
			}
		}

		activeStroke = nullptr;
		activeBrush = savedBrush;
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	int meshHit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &tpi.facet);
	if (meshHit == -1)
		return false;

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

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMeshes(), activeBrush);

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
	os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());
}

bool wxGLPanel::UndoStroke() {
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	bool ret = strokeManager->backStroke(gls.GetActiveMeshes());
	if (ret) {
		if (curStroke && curStroke->BrushType() == TBT_WEIGHT && os->outfitBones) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				if (os->boneScale) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
				}
			}
		}

		if (curStroke && curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke, true);

			if (!os->bEditSlider) {
				vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}

			if (curStroke->BrushType() == TBT_XFORM)
				ShowTransformTool(true, false);
		}
	}

	gls.RenderOneFrame();
	return ret;
}

bool wxGLPanel::RedoStroke() {
	bool ret = strokeManager->forwardStroke(gls.GetActiveMeshes());
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	if (ret) {
		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				if (os->boneScale) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
				}
			}
		}

		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke);

			if (!os->bEditSlider) {
				vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}

			if (curStroke->BrushType() == TBT_XFORM)
				ShowTransformTool(true, false);
		}
	}

	gls.RenderOneFrame();
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
	string mode = Config.GetString("Editing/CenterMode");
	if (mode == "Object") {
		if (!gls.GetActiveMeshes().empty())
			o = gls.GetActiveMeshes().back()->bvh->Center();
	}
	else if (mode == "Selected")
		o = gls.GetActiveCenter();

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

	gls.RenderOneFrame();
}

void wxGLPanel::OnIdle(wxIdleEvent& WXUNUSED(event)) {
	for (auto &it : BVHUpdateQueue) {
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
	gls.RenderOneFrame();
}

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	if (wxGetKeyState(wxKeyCode('S')))  {
		wxPoint p = event.GetPosition();
		int delt = event.GetWheelRotation();
		if (delt < 0)
			DecBrush();
		else
			IncBrush();

		gls.UpdateCursor(p.x, p.y, bGlobalBrushCollision);
	}
	else {
		int delt = event.GetWheelRotation();
		gls.DollyCamera(delt);
		gls.UpdateProjection();
	}
	
	os->CheckBrushBounds();
	os->UpdateBrushPane();
	gls.RenderOneFrame();
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	wxFrame* parent = (wxFrame*)GetGrandParent()->GetGrandParent();
	if (parent->IsActive())
		SetFocus();

	bool cursorExists = false;
	int x;
	int y;
	int t = 0;
	float w = 0.0f;
	float m = 0.0f;
	event.GetPosition(&x, &y);

	if (mbuttonDown) {
		isMDragging = true;
		if (wxGetKeyState(wxKeyCode::WXK_SHIFT))
			gls.DollyCamera(y - lastY);
		else
			gls.PanCamera(x - lastX, y - lastY);

		gls.RenderOneFrame();
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

		gls.RenderOneFrame();
	}

	if (lbuttonDown) {
		isLDragging = true;
		if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else {
			if (Config.MatchValue("Input/LeftMousePan", "true"))
				gls.PanCamera(x - lastX, y - lastY);
		}

		gls.RenderOneFrame();
	}

	if (!rbuttonDown && !lbuttonDown) {
		string hitMeshName;
		if (editMode && transformMode == 0) {
			cursorExists = gls.UpdateCursor(x, y, bGlobalBrushCollision, &hitMeshName, &t, &w, &m);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		gls.RenderOneFrame();

		if (os->statusBar) {
			if (cursorExists) {
				if (bWeightPaint)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g", t, w), 1);
				else if (bMaskPaint)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", t, m), 1);
				else {
					vector<Vector3> verts;
					os->project->GetLiveVerts(hitMeshName, verts);
					if (verts.size() > t)
						os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", t, verts[t].x, verts[t].y, verts[t].z), 1);
				}
			}
			else {
				os->statusBar->SetStatusText("", 1);
			}
		}
	}

	lastX = x;
	lastY = y;
}

void wxGLPanel::OnLeftDown(wxMouseEvent& event) {
	if (!HasCapture())
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

void wxGLPanel::OnMiddleDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	mbuttonDown = true;
}

void wxGLPanel::OnMiddleUp(wxMouseEvent& WXUNUSED(event)) {
	if (GetCapture() == this)
		ReleaseMouse();

	isMDragging = false;
	mbuttonDown = false;
}

void wxGLPanel::OnLeftUp(wxMouseEvent& event) {
	if (GetCapture() == this)
		ReleaseMouse();

	if (!isLDragging && (!isPainting)) {
		int x, y;
		event.GetPosition(&x, &y);
		wxPoint p = event.GetPosition();
		int meshID = gls.PickMesh(x, y);
		if (meshID == -1) {

		}
		else {
			os->SelectShape(gls.GetMeshName(meshID));
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


bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner) {
		wxString inputFile;
		if (fileNames.GetCount() > 0)
			inputFile = fileNames.Item(0);

		string mergeShapeName = "";
		if (owner->activeItem)
			mergeShapeName = owner->activeItem->shapeName;

		wxString dataName = inputFile.AfterLast('\\');
		dataName = dataName.BeforeLast('.');

		if (inputFile.MakeLower().EndsWith(".nif")) {
			owner->StartProgress("Adding NIF file...");
			owner->UpdateProgress(1, "Adding NIF file...");
			owner->project->AddNif(inputFile.ToStdString(), false);
			owner->project->SetTextures("_AUTO_");

			owner->UpdateProgress(60, "Refreshing GUI...");
			owner->RefreshGUIFromProj();

			owner->UpdateProgress(100, "Finished.");
			owner->EndProgress();
		}
		else if (inputFile.MakeLower().EndsWith(".obj")) {
			owner->StartProgress("Adding OBJ file...");
			owner->UpdateProgress(1, "Adding OBJ file...");
			owner->project->AddShapeFromObjFile(inputFile.ToStdString(), dataName.ToStdString(), mergeShapeName);
			owner->project->SetTextures("_AUTO_");

			owner->UpdateProgress(60, "Refreshing GUI...");
			owner->RefreshGUIFromProj();

			owner->UpdateProgress(100, "Finished.");
			owner->EndProgress();
		}
		else if (inputFile.MakeLower().EndsWith(".fbx")) {
			owner->StartProgress("Adding FBX file...");
			owner->UpdateProgress(1, "Adding FBX file...");
			owner->project->ImportShapeFBX(inputFile.ToStdString(), dataName.ToStdString(), mergeShapeName);
			owner->project->SetTextures("_AUTO_");

			owner->UpdateProgress(60, "Refreshing GUI...");
			owner->RefreshGUIFromProj();

			owner->UpdateProgress(100, "Finished.");
			owner->EndProgress();
		}
	}
	else
		return false;

	return true;
}

bool DnDSliderFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner) {
		bool isMultiple = (fileNames.GetCount() > 1);
		for (int i = 0; i < fileNames.GetCount(); i++)	{
			wxString inputFile;
			inputFile = fileNames.Item(i);

			wxString dataName = inputFile.AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			bool isBSD = inputFile.MakeLower().EndsWith(".bsd");
			bool isOBJ = inputFile.MakeLower().EndsWith(".obj");
			bool isFBX = inputFile.MakeLower().EndsWith(".fbx");
			if (isBSD || isOBJ) {
				if (!owner->activeItem) {
					wxMessageBox("There is no shape selected!", "Error");
					return false;
				}

				if (lastResult == wxDragCopy) {
					targetSlider = owner->NewSlider(dataName.ToStdString(), isMultiple);
				}

				if (targetSlider.empty())
					return false;

				owner->StartProgress("Loading slider file...");
				owner->UpdateProgress(1, "Loading slider file...");

				if (isBSD)
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else
					return false;


				owner->UpdateProgress(100, "Finished.");
				owner->EndProgress();
			}
		}
		owner->EnterSliderEdit(targetSlider);
		targetSlider.clear();
	}
	else
		return false;

	return true;
}

wxDragResult DnDSliderFile::OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult) {
	targetSlider.clear();
	lastResult = defResult;

	if (defResult == wxDragCopy)
		return lastResult;

	if (owner) {
		for (auto &child : owner->sliderDisplays) {
			if (child.second->sliderPane->GetRect().Contains(x, y)) {
				targetSlider = child.first;
				lastResult = wxDragMove;
				break;
			}
		}

		if (targetSlider.empty())
			if (owner->sliderScroll->HitTest(x, y) == wxHT_WINDOW_INSIDE)
				lastResult = wxDragCopy;
	}
	else
		lastResult = wxDragCancel;

	return lastResult;
}


void OutfitStudio::OnLoadOutfitFP_File(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkFile", wxRadioButton)->SetValue(true);
}

void OutfitStudio::OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npTexFile", wxRadioButton)->SetValue(true);
}
