/*
BodySlide and Outfit Studio
Copyright(C) 2017  Caliente & ousnius

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
	EVT_MENU(XRCID("fileUnload"), OutfitStudio::OnUnloadProject)

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
	EVT_MENU(XRCID("importOutfitNif"), OutfitStudio::OnImportOutfitNif)
	EVT_MENU(XRCID("exportOutfitNif"), OutfitStudio::OnExportOutfitNif)
	EVT_MENU(XRCID("exportOutfitNifWithRef"), OutfitStudio::OnExportOutfitNifWithRef)
	EVT_MENU(XRCID("importPhysicsData"), OutfitStudio::OnImportPhysicsData)
	EVT_MENU(XRCID("exportPhysicsData"), OutfitStudio::OnExportPhysicsData)
	EVT_MENU(XRCID("makeConvRef"), OutfitStudio::OnMakeConvRef)
	
	EVT_MENU(XRCID("sliderLoadPreset"), OutfitStudio::OnLoadPreset)
	EVT_MENU(XRCID("sliderSavePreset"), OutfitStudio::OnSavePreset)
	EVT_MENU(XRCID("sliderConform"), OutfitStudio::OnSliderConform)
	EVT_MENU(XRCID("sliderConformAll"), OutfitStudio::OnSliderConformAll)
	EVT_MENU(XRCID("sliderImportBSD"), OutfitStudio::OnSliderImportBSD)
	EVT_MENU(XRCID("sliderImportOBJ"), OutfitStudio::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderImportFBX"), OutfitStudio::OnSliderImportFBX)
	EVT_MENU(XRCID("sliderImportOSD"), OutfitStudio::OnSliderImportOSD)
	EVT_MENU(XRCID("sliderImportTRI"), OutfitStudio::OnSliderImportTRI)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudio::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderExportOBJ"), OutfitStudio::OnSliderExportOBJ)
	EVT_MENU(XRCID("sliderExportOSD"), OutfitStudio::OnSliderExportOSD)
	EVT_MENU(XRCID("sliderExportTRI"), OutfitStudio::OnSliderExportTRI)
	EVT_MENU(XRCID("sliderNew"), OutfitStudio::OnNewSlider)
	EVT_MENU(XRCID("sliderNewZap"), OutfitStudio::OnNewZapSlider)
	EVT_MENU(XRCID("sliderNewCombined"), OutfitStudio::OnNewCombinedSlider)
	EVT_MENU(XRCID("sliderNegate"), OutfitStudio::OnSliderNegate)
	EVT_MENU(XRCID("sliderMask"), OutfitStudio::OnMaskAffected)
	EVT_MENU(XRCID("sliderClear"), OutfitStudio::OnClearSlider)
	EVT_MENU(XRCID("sliderDelete"), OutfitStudio::OnDeleteSlider)
	EVT_MENU(XRCID("sliderProperties"), OutfitStudio::OnSliderProperties)
	
	EVT_MENU(XRCID("btnXMirror"), OutfitStudio::OnXMirror)
	EVT_MENU(XRCID("btnConnected"), OutfitStudio::OnConnectedOnly)
	EVT_MENU(XRCID("btnBrushCollision"), OutfitStudio::OnGlobalBrushCollision)
	
	EVT_MENU(XRCID("btnSelect"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnTransform"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnVertexEdit"), OutfitStudio::OnSelectTool)

	EVT_MENU(XRCID("btnMaskBrush"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnInflateBrush"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnDeflateBrush"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnMoveBrush"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnSmoothBrush"), OutfitStudio::OnSelectTool)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudio::OnSelectTool)

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
	EVT_MENU(XRCID("deleteVerts"), OutfitStudio::OnDeleteVerts)
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

	EVT_TREE_SEL_CHANGED(XRCID("segmentTree"), OutfitStudio::OnSegmentSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudio::OnSegmentContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudio::OnSegmentTreeContext)
	EVT_MENU(XRCID("addSegment"), OutfitStudio::OnAddSegment)
	EVT_MENU(XRCID("addSubSegment"), OutfitStudio::OnAddSubSegment)
	EVT_MENU(XRCID("deleteSegment"), OutfitStudio::OnDeleteSegment)
	EVT_MENU(XRCID("deleteSubSegment"), OutfitStudio::OnDeleteSubSegment)
	EVT_CHOICE(XRCID("segmentType"), OutfitStudio::OnSegmentTypeChanged)
	EVT_BUTTON(XRCID("segmentApply"), OutfitStudio::OnSegmentApply)
	EVT_BUTTON(XRCID("segmentReset"), OutfitStudio::OnSegmentReset)

	EVT_TREE_SEL_CHANGED(XRCID("partitionTree"), OutfitStudio::OnPartitionSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudio::OnPartitionContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudio::OnPartitionTreeContext)
	EVT_MENU(XRCID("addPartition"), OutfitStudio::OnAddPartition)
	EVT_MENU(XRCID("deletePartition"), OutfitStudio::OnDeletePartition)
	EVT_CHOICE(XRCID("partitionType"), OutfitStudio::OnPartitionTypeChanged)
	EVT_BUTTON(XRCID("partitionApply"), OutfitStudio::OnPartitionApply)
	EVT_BUTTON(XRCID("partitionReset"), OutfitStudio::OnPartitionReset)
	
	EVT_BUTTON(XRCID("meshTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("boneTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("segmentTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("partitionTabButton"), OutfitStudio::OnTabButtonClick)
	EVT_BUTTON(XRCID("lightsTabButton"), OutfitStudio::OnTabButtonClick)

	EVT_SLIDER(XRCID("lightAmbientSlider"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightFrontalSlider"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional0Slider"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional1Slider"), OutfitStudio::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional2Slider"), OutfitStudio::OnUpdateLights)
	EVT_BUTTON(XRCID("lightReset"), OutfitStudio::OnResetLights)
	
	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("splitter"), OutfitStudio::OnSashPosChanged)
	EVT_MOVE_END(OutfitStudio::OnMoveWindow)
	EVT_SIZE(OutfitStudio::OnSetSize)
wxEND_EVENT_TABLE()

// ----------------------------------------------------------------------------
// OutfitStudio frame
// ----------------------------------------------------------------------------

OutfitStudio::OutfitStudio(const wxPoint& pos, const wxSize& size, ConfigurationManager& inConfig) : appConfig(inConfig) {
	wxLogMessage("Loading Outfit Studio at X:%d Y:%d with W:%d H:%d...", pos.x, pos.y, size.GetWidth(), size.GetHeight());

	wxXmlResource *xrc = wxXmlResource::Get();
	if (!xrc->Load("res\\xrc\\OutfitStudio.xrc")) {
		wxMessageBox(_("Failed to load OutfitStudio.xrc file!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	if (!xrc->LoadFrame(this, nullptr, "outfitStudio")) {
		wxMessageBox(_("Failed to load Outfit Studio frame!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	SetIcon(wxIcon("res\\images\\OutfitStudio.png", wxBITMAP_TYPE_PNG));

	xrc->Load("res\\xrc\\Project.xrc");
	xrc->Load("res\\xrc\\Actions.xrc");
	xrc->Load("res\\xrc\\Slider.xrc");
	xrc->Load("res\\xrc\\Skeleton.xrc");

	int statusWidths[] = { -1, 275, 100 };
	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	statusBar->SetFieldsCount(3);
	statusBar->SetStatusWidths(3, statusWidths);
	statusBar->SetStatusText(_("Ready!"));

	this->DragAcceptFiles(true);

	xrc->LoadMenuBar(this, "menuBar");
	xrc->LoadToolBar(this, "toolBar");

	wxSlider* fovSlider = (wxSlider*)GetToolBar()->FindWindowByName("fovSlider");
	fovSlider->Bind(wxEVT_SLIDER, &OutfitStudio::OnFieldOfViewSlider, this);

	visStateImages = new wxImageList(16, 16, false, 2);
	wxBitmap visImg("res\\images\\icoVisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap invImg("res\\images\\icoInvisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap wfImg("res\\images\\icoWireframe.png", wxBITMAP_TYPE_PNG);

	if (visImg.IsOk())
		visStateImages->Add(visImg);
	if (invImg.IsOk())
		visStateImages->Add(invImg);
	if (wfImg.IsOk())
		visStateImages->Add(wfImg);

	targetGame = (TargetGame)appConfig.GetIntValue("TargetGame");

	wxStateButton* meshTab = (wxStateButton*)FindWindowByName("meshTabButton");
	meshTab->SetCheck();

	if (targetGame != FO4) {
		wxStateButton* segmentTab = (wxStateButton*)FindWindowByName("segmentTabButton");
		segmentTab->Show(false);
	}
	else {
		wxStateButton* partitionTab = (wxStateButton*)FindWindowByName("partitionTabButton");
		partitionTab->Show(false);
	}

	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	outfitShapes->AssignStateImageList(visStateImages);
	shapesRoot = outfitShapes->AddRoot("Shapes");

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	bonesRoot = outfitBones->AddRoot("Bones");

	segmentTree = (wxTreeCtrl*)FindWindowByName("segmentTree");
	segmentRoot = segmentTree->AddRoot("Segments");

	partitionTree = (wxTreeCtrl*)FindWindowByName("partitionTree");
	partitionRoot = partitionTree->AddRoot("Partitions");

	wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
	segmentType->Show(false);

	wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
	partitionType->Show(false);

	int ambient = appConfig.GetIntValue("Lights/Ambient");
	int frontal = appConfig.GetIntValue("Lights/Frontal");
	int directional0 = appConfig.GetIntValue("Lights/Directional0");
	int directional1 = appConfig.GetIntValue("Lights/Directional1");
	int directional2 = appConfig.GetIntValue("Lights/Directional2");

	lightSettings = (wxPanel*)FindWindowByName("lightSettings");
	wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	lightAmbientSlider->SetValue(ambient);

	wxSlider* lightFrontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	lightFrontalSlider->SetValue(frontal);

	wxSlider* lightDirectional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	lightDirectional0Slider->SetValue(directional0);

	wxSlider* lightDirectional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	lightDirectional1Slider->SetValue(directional1);

	wxSlider* lightDirectional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");
	lightDirectional2Slider->SetValue(directional2);

	boneScale = (wxSlider*)FindWindowByName("boneScale");

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
	glView->SetNotifyWindow(this);

	wxWindow* rightPanel = FindWindowByName("rightSplitPanel");
	rightPanel->SetDoubleBuffered(true);

	xrc->AttachUnknownControl("mGLView", glView, this);

	project = new OutfitProject(appConfig, this);	// Create empty project
	CreateSetSliders();

	bEditSlider = false;
	activeItem = nullptr;
	selectedItems.clear();

	previousMirror = true;
	previewScale = Vector3(1.0f, 1.0f, 1.0f);

	SetSize(size);
	SetPosition(pos);

	wxSplitterWindow* splitter = (wxSplitterWindow*)FindWindowByName("splitter");
	int sashPos = appConfig.GetIntValue("OutfitStudioFrame.sashpos");
	splitter->SetSashPosition(sashPos);

	wxSplitterWindow* splitterRight = (wxSplitterWindow*)FindWindowByName("splitterRight");
	splitterRight->SetSashPosition(200);

	leftPanel->Layout();

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
	wxXmlResource::Get()->Unload("res\\xrc\\OutfitStudio.xrc");
	wxLogMessage("Outfit Studio closed.");
}

void OutfitStudio::OnSashPosChanged(wxSplitterEvent& event) {
	if (!IsVisible())
		return;

	int pos = event.GetSashPosition();
	Config.SetValue("OutfitStudioFrame.sashpos", pos);
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
		StartProgress(_("Creating sliders..."));
	}

	UpdateProgress(0, _("Clearing old sliders..."));

	sliderScroll->Freeze();
	sliderScroll->DestroyChildren();
	for (auto &sd : sliderDisplays)
		delete sd.second;

	sliderDisplays.clear();

	wxSizer* rootSz = sliderScroll->GetSizer();

	for (int i = 0; i < project->SliderCount(); i++)  {
		UpdateProgress(inc, _("Loading slider: ") + project->GetSliderName(i));
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

	d->btnSliderEdit = new wxBitmapButton(d->sliderPane, wxID_ANY, wxBitmap("res\\images\\EditSmall.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, name + "|btn");
	d->btnSliderEdit->SetBitmapDisabled(wxBitmap("res\\images\\EditSmall_d.png", wxBITMAP_TYPE_ANY));
	d->btnSliderEdit->SetToolTip(_("Turn on edit mode for this slider."));
	d->paneSz->Add(d->btnSliderEdit, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnMinus = new wxButton(d->sliderPane, wxID_ANY, "-", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnMinus");
	d->btnMinus->SetToolTip(_("Weaken slider data by 1%."));
	d->btnMinus->SetForegroundColour(wxTransparentColour);
	d->btnMinus->Hide();
	d->paneSz->Add(d->btnMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnPlus = new wxButton(d->sliderPane, wxID_ANY, "+", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, name + "|btnPlus");
	d->btnPlus->SetToolTip(_("Strengthen slider data by 1%."));
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
		finalName = wxGetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), thename, this);
		if (finalName.empty())
			return finalName;
	}
	else {
		finalName = thename;
	}

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
	sliderDisplays[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::SetSliderValue(const string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	sliderDisplays[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudio::ApplySliders(bool recalcBVH) {
	vector<Vector3> verts;
	vector<Vector2> uvs;
	vector<string> shapes;
	project->GetShapes(shapes);

	for (auto &shape : shapes) {
		project->GetLiveVerts(shape, verts, &uvs);
		glView->UpdateMeshVertices(shape, &verts, recalcBVH, true, false, &uvs);
	}

	bool tMode = glView->GetTransformMode();
	bool vMode = glView->GetVertexEdit();

	if (tMode)
		glView->ShowTransformTool();
	if (vMode)
		glView->ShowVertexEdit();

	if (!tMode && !vMode)
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
	if (!activeItem) {
		if (glView->GetTransformMode())
			glView->ShowTransformTool(false);
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit(false);

		CreateSegmentTree();
		CreatePartitionTree();
		outfitBones->UnselectAll();
	}
	else {
		mesh* m = glView->GetMesh(activeItem->shapeName);
		if (m) {
			if (m->smoothSeamNormals)
				GetMenuBar()->Check(XRCID("btnSmoothSeams"), true);
			else
				GetMenuBar()->Check(XRCID("btnSmoothSeams"), false);

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
			if (glView->GetVertexEdit())
				glView->ShowVertexEdit();
		}

		CreateSegmentTree(activeItem->shapeName);
		CreatePartitionTree(activeItem->shapeName);
		ReselectBone();
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
				outfitShapes->UnselectAll();
				outfitShapes->SelectItem(subitem);
				outfitShapes->EnsureVisible(subitem);
				return;
			}
			subitem = outfitShapes->GetNextChild(item, subcookie);
		}
		item = outfitShapes->GetNextSibling(item);
	}
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
						project->workWeights[m->shapeName].clear();
					}
				}
			}
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
	menu->Enable(XRCID("sliderMask"), true);
	menu->Enable(XRCID("sliderClear"), true);
	menu->Enable(XRCID("sliderDelete"), true);
	menu->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudio::MenuExitSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), false);
	menu->Enable(XRCID("menuExportSlider"), false);
	menu->Enable(XRCID("sliderNegate"), false);
	menu->Enable(XRCID("sliderMask"), false);
	menu->Enable(XRCID("sliderClear"), false);
	menu->Enable(XRCID("sliderDelete"), false);
	menu->Enable(XRCID("sliderProperties"), false);
}

bool OutfitStudio::NotifyStrokeStarting() {
	if (!activeItem)
		return false;

	if (bEditSlider || project->AllSlidersZero())
		return true;

	int	response = wxMessageBox(_("You can only edit the base shape when all sliders are zero. Do you wish to set all sliders to zero now?  Note, use the pencil button next to a slider to enable editing of that slider's morph."),
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

	UpdateReferenceTemplates();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudio::OnNPWizChangeSetNameChoice, this);

		pg2 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj2", wxWizardPageSimple);
		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnLoadOutfitFP_Texture, this);

		wxChoice* tmplChoice = XRCCTRL(wiz, "npTemplateChoice", wxChoice);
		for (auto &tmpl : refTemplates)
			tmplChoice->Append(tmpl.name);

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
	StartProgress(wxString::Format(_("Creating project '%s'..."), outfitName));

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

	UpdateProgress(10, _("Loading reference..."));

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		string tmplName = refTemplate.ToStdString();
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const ReferenceTemplate& rt) { return rt.name == tmplName; });
		if (tmpl != refTemplates.end())
			error = project->LoadReferenceTemplate((*tmpl).sourceFile, (*tmpl).set, (*tmpl).shape);
		else
			error = 1;
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToStdString(),
				sliderSetName.ToStdString(), false, refShape.ToStdString());
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

	UpdateProgress(40, _("Loading outfit..."));

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
	UpdateProgress(80, _("Creating outfit..."));

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetTextures();
	else
		project->SetTextures(XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath().ToStdString());

	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, _("Applying slider effects..."));
	ApplySliders();

	wxLogMessage("Project created.");
	UpdateProgress(100, _("Finished"));

	EndProgress();
}

void OutfitStudio::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this, _("Select a slider set to load"), "SliderSets", wxEmptyString, "Slider Set Files (*.osp;*.xml)|*.osp;*.xml", wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	string file = loadProjectDialog.GetPath();
	vector<string> setnames;
	SliderSetFile InFile(file);
	if (InFile.fail()) {
		wxLogError("Failed to open '%s' as a slider set file!", file);
		wxMessageBox(wxString::Format(_("Failed to open '%s' as a slider set file!"), file), _("Slider Set Error"), wxICON_ERROR);
		return;
	}

	InFile.GetSetNames(setnames);
	wxArrayString choices;
	for (auto &s : setnames)
		choices.Add(s);

	string outfit;
	if (choices.GetCount() > 1) {
		outfit = wxGetSingleChoice(_("Please choose an outfit to load"), _("Load a slider set"), choices, 0, this);
		if (outfit.empty())
			return;
	}
	else if (choices.GetCount() == 1)
		outfit = choices.front();
	else
		return;

	wxLogMessage("Loading project '%s' from file '%s'...", outfit, file);
	StartProgress(_("Loading project..."));

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
	UpdateProgress(10, _("Loading outfit data..."));
	StartSubProgress(10, 40);

	int error = project->OutfitFromSliderSet(file, outfit);
	if (error) {
		EndProgress();
		wxLogError("Failed to create project (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to create project '%s' from file '%s' (%d)!"), outfit, file, error), _("Slider Set Error"), wxICON_ERROR);
		RefreshGUIFromProj();
		return;
	}

	string shape = project->GetBaseShape();
	wxLogMessage("Loading reference shape '%s'...", shape);
	UpdateProgress(50, wxString::Format(_("Loading reference shape '%s'..."), shape));

	if (!shape.empty()) {
		error = project->LoadReferenceNif(project->activeSet.GetInputFileName(), shape, true);
		if (error) {
			EndProgress();
			RefreshGUIFromProj();
			return;
		}
	}

	wxLogMessage("Loading textures...");
	UpdateProgress(60, _("Loading textures..."));

	project->SetTextures();

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, _("Applying slider effects..."));
	ApplySliders();

	wxLogMessage("Project loaded.");
	UpdateProgress(100, _("Finished"));
	GetMenuBar()->Enable(XRCID("fileSave"), true);
	EndProgress();
}

void OutfitStudio::OnLoadReference(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	UpdateReferenceTemplates();

	wxDialog dlg;
	int result = wxID_CANCEL;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadRef", "wxDialog")) {
		XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudio::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(dlg, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudio::OnNPWizChangeSetNameChoice, this);

		wxChoice* tmplChoice = XRCCTRL(dlg, "npTemplateChoice", wxChoice);
		for (auto &tmpl : refTemplates)
			tmplChoice->Append(tmpl.name);

		tmplChoice->Select(0);
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress(_("Loading reference..."));
	glView->DeleteMesh(project->GetBaseShape());

	UpdateProgress(10, _("Loading reference set..."));
	bool mergeSliders = (XRCCTRL(dlg, "chkClearSliders", wxCheckBox)->IsChecked());

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(dlg, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		string tmplName = refTemplate.ToStdString();
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const ReferenceTemplate& rt) { return rt.name == tmplName; });
		if (tmpl != refTemplates.end())
			error = project->LoadReferenceTemplate((*tmpl).sourceFile, (*tmpl).set, (*tmpl).shape, mergeSliders);
		else
			error = 1;
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToStdString(),
				sliderSetName.ToStdString(), mergeSliders, refShape.ToStdString());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToStdString(), refShape.ToStdString(), mergeSliders);
		}
	}
	else
		project->ClearReference();

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	project->SetTextures(project->GetBaseShape());

	wxLogMessage("Creating reference...");
	UpdateProgress(60, _("Creating reference..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(70, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(70, 99);
	CreateSetSliders();

	ShowSliderEffect(0);
	wxLogMessage("Applying slider effects...");
	UpdateProgress(99, _("Applying slider effects..."));
	ApplySliders();

	wxLogMessage("Reference loaded.");
	UpdateProgress(100, _("Finished"));
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
	StartProgress(_("Loading outfit..."));

	vector<string> oldShapes;
	project->GetShapes(oldShapes);
	for (auto &s : oldShapes) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s);
		}
	}

	UpdateProgress(1, _("Loading outfit..."));

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
		project->SetTextures();
	else
		project->SetTextures(XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToStdString());

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	wxLogMessage("Outfit loaded.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudio::OnUnloadProject(wxCommandEvent& WXUNUSED(event)) {
	int res = wxMessageBox(_("Do you really want to unload the project? All unsaved changes will be lost."), _("Unload Project"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	wxLogMessage("Unloading project...");
	GetMenuBar()->Enable(XRCID("fileSave"), false);

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

	CreateSetSliders();
	RefreshGUIFromProj();
	glView->Render();
}

void OutfitStudio::UpdateReferenceTemplates() {
	refTemplates.clear();

	string fileName = "RefTemplates.xml";
	if (!wxFileName::IsFileReadable(fileName))
		return;

	XMLDocument doc;
	XMLElement* root;
	if (doc.LoadFile(fileName.c_str()) == XML_SUCCESS) {
		root = doc.FirstChildElement("RefTemplates");
		if (!root)
			return;

		XMLElement* element = root->FirstChildElement("Template");
		while (element) {
			ReferenceTemplate refTemplate;
			refTemplate.name = element->GetText();
			refTemplate.sourceFile = element->Attribute("sourcefile");
			refTemplate.set = element->Attribute("set");
			refTemplate.shape = element->Attribute("shape");
			refTemplates.push_back(refTemplate);
			element = element->NextSiblingElement("Template");
		}
	}
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

	if (targetGame == SKYRIM || targetGame == SKYRIMSE)
		project->mGenWeights = true;
	else
		project->mGenWeights = false;

	project->mCopyRef = true;

	glView->DestroyOverlays();
}

void OutfitStudio::RenameProject(const string& projectName) {
	project->outfitName = projectName;
	if (outfitRoot.IsOk())
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

	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudio::AnimationGUIFromProj() {
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

	if (shapes.size() > 0) {
		if (shapes.size() == 1 && project->IsBaseShape(shapes.front()))
			outfitRoot = outfitShapes->AppendItem(shapesRoot, "Reference Only");
		else
			outfitRoot = outfitShapes->AppendItem(shapesRoot, project->OutfitName());
	}

	for (auto &shape : shapes) {
		glView->DeleteMesh(shape);

		glView->AddMeshFromNif(project->GetWorkNif(), shape, true);

		MaterialFile matFile;
		bool hasMatFile = project->GetShapeMaterialFile(shape, matFile);
		glView->SetMeshTextures(shape, project->GetShapeTextures(shape), hasMatFile, matFile);

		subItem = outfitShapes->AppendItem(outfitRoot, shape);
		outfitShapes->SetItemState(subItem, 0);
		outfitShapes->SetItemData(subItem, new ShapeItemData(shape));

		if (project->IsBaseShape(shape)) {
			outfitShapes->SetItemBold(subItem);
			outfitShapes->SetItemTextColour(subItem, wxColour(0, 255, 0));
		}
		glView->Render();
	}

	outfitShapes->ExpandAll();
}

void OutfitStudio::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();
	string copyStr = XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue();

	project->ReplaceForbidden(copyStr);

	string defSliderSetFile = copyStr + ".osp";
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

void OutfitStudio::OnSaveSliderSet(wxCommandEvent& event) {
	if (project->mFileName.empty()) {
		OnSaveSliderSetAs(event);
	}
	else {
		if (!project->GetWorkNif()->IsValid()) {
			wxMessageBox(_("There are no valid shapes loaded!"), "Error");
			return;
		}

		if (project->HasUnweighted()) {
			wxLogWarning("Unweighted vertices found.");
			int error = wxMessageBox(_("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?"), _("Unweighted Vertices"), wxYES_NO | wxICON_WARNING, this);
			if (error != wxYES)
				return;
		}

		wxLogMessage("Saving project '%s'...", project->OutfitName());
		StartProgress(wxString::Format(_("Saving project '%s'..."), project->OutfitName()));
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
			wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		}

		EndProgress();
	}
}

void OutfitStudio::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (project->HasUnweighted()) {
		wxLogWarning("Unweighted vertices found.");
		int error = wxMessageBox(_("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?"), _("Unweighted Vertices"), wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
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

		project->ReplaceForbidden(sssName);

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
		wxMessageBox(_("Invalid or no slider set file specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	if (!strFileName.EndsWith(".osp"))
		strFileName = strFileName.Append(".osp");

	strOutfitName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue();
	if (strOutfitName.empty()) {
		wxMessageBox(_("No outfit name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetDirName().GetFullName();
	if (strDataDir.empty()) {
		strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetPath();
		if (strDataDir.empty()) {
			wxMessageBox(_("No data folder specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
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
		wxMessageBox(_("An invalid or no base outfit .nif file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	if (!strBaseFile.EndsWith(".nif"))
		strBaseFile = strBaseFile.Append(".nif");

	strGamePath = XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->GetValue();
	if (strGamePath.empty()) {
		wxMessageBox(_("No game file path specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	strGameFile = XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->GetValue();
	if (strGameFile.empty()) {
		wxMessageBox(_("No game file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format(_("Saving project '%s'..."), strOutfitName));
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
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
	}

	EndProgress();
}

void OutfitStudio::OnBrushPane(wxCollapsiblePaneEvent& event) {
	wxCollapsiblePane* brushPane = (wxCollapsiblePane*)event.GetEventObject();
	if (!brushPane)
		return;

	if (!brushPane->IsCollapsed())
		if (!glView->GetEditMode())
			brushPane->Collapse();

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
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

void OutfitStudio::OnImportOutfitNif(wxCommandEvent& WXUNUSED(event)) {
	string fileName = wxFileSelector(_("Import NIF file"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_FILE_MUST_EXIST, this);
	if (fileName.empty())
		return;

	StartProgress(_("Importing NIF file..."));
	UpdateProgress(1, _("Importing NIF file..."));
	project->AddNif(fileName, false);
	project->SetTextures();

	UpdateProgress(60, _("Refreshing GUI..."));
	RefreshGUIFromProj();

	UpdateProgress(100, _("Finished."));
	EndProgress();
}

void OutfitStudio::OnExportOutfitNif(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (project->HasUnweighted()) {
		wxLogWarning("Unweighted vertices found.");
		int error = wxMessageBox(_("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?"), _("Unweighted Vertices"), wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return;
	}

	string fileName = wxFileSelector(_("Export outfit NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
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
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
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
		int error = wxMessageBox(_("At least one vertex does not have any weighting assigned to it. This will cause issues and you should fix it using the weight brush. The affected vertices have been put under a mask. Do you want to save anyway?"), _("Unweighted Vertices"), wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return;
	}

	string fileName = wxFileSelector(_("Export project NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
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
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s' with reference!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudio::OnImportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	string fileName = wxFileSelector(_("Import physics data to project"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_FILE_MUST_EXIST, this);
	if (fileName.empty())
		return;

	BSClothExtraData physicsBlock;
	if (!physicsBlock.FromHKX(fileName)) {
		wxLogError("Failed to import physics data file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to import physics data file '%s'!"), fileName), _("Import Error"), wxICON_ERROR);
	}

	auto& physicsData = project->GetClothData();
	physicsData[fileName] = physicsBlock;
}

void OutfitStudio::OnExportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	auto& physicsData = project->GetClothData();
	if (physicsData.empty()) {
		wxMessageBox(_("There is no physics data loaded!"), _("Info"), wxICON_INFORMATION);
		return;
	}

	wxArrayString fileNames;
	for (auto &data : physicsData)
		fileNames.Add(data.first);

	wxSingleChoiceDialog physicsDataChoice(this, _("Please choose the physics data source you want to export."), _("Choose physics data"), fileNames);
	if (physicsDataChoice.ShowModal() == wxID_CANCEL)
		return;

	int sel = physicsDataChoice.GetSelection();
	string selString = fileNames[sel].ToStdString();

	if (!selString.empty()) {
		string fileName = wxFileSelector(_("Export physics data"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.empty())
			return;

		if (!physicsData[selString].ToHKX(fileName)) {
			wxLogError("Failed to save physics data file '%s'!", fileName);
			wxMessageBox(wxString::Format(_("Failed to save physics data file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudio::OnMakeConvRef(wxCommandEvent& WXUNUSED(event)) {
	if (project->AllSlidersZero()) {
		wxMessageBox(_("This function requires at least one slider position to be non-zero."));
		return;
	}

	string namebase = "ConvertToBase";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;
	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser(_("Create a conversion slider for the current slider settings with the following name: "), _("Create New Conversion Slider"), thename, this);
	if (finalName == "")
		return;

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
	string s = outfitShapes->GetItemText(event.GetItem()).ToStdString();

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

void OutfitStudio::OnShapeActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitShapes->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnShapeProperties(evt);
}

void OutfitStudio::OnBoneSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	wxTreeItemId subitem;

	project->ClearBoneScale();
	boneScale->SetValue(0);

	if (!activeItem)
		return;

	if (outfitBones->GetItemParent(item).IsOk()) {
		// Clear history
		glView->GetStrokeManager()->Clear();

		// Clear vcolors of all shapes
		vector<string> shapes;
		project->GetShapes(shapes);
		for (auto &s : shapes) {
			mesh* m = glView->GetMesh(s);
			if (m)
				m->ColorChannelFill(1, 0.0f);
		}

		activeBone = outfitBones->GetItemText(item);

		// Show weights of selected shapes without reference
		for (auto &s : selectedItems) {
			unordered_map<ushort, float> boneWeights;
			if (!project->IsBaseShape(s->shapeName)) {
				project->GetWorkAnim()->GetWeights(s->shapeName, activeBone, boneWeights);
				project->workWeights[s->shapeName] = boneWeights;

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
		project->workWeights[project->GetBaseShape()] = boneWeights;

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
	ShapeItemData* dropData = new ShapeItemData(activeItem->shapeName);
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

void OutfitStudio::OnSegmentSelect(wxTreeEvent& event) {
	ShowSegment(event.GetItem());

	wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
	segmentApply->Enable();

	wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");
	segmentReset->Enable();
}

void OutfitStudio::OnSegmentContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	segmentTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(event.GetItem()));
	if (subSegmentData)
		menu = wxXmlResource::Get()->LoadMenu("menuSubSegmentContext");
	else
		menu = wxXmlResource::Get()->LoadMenu("menuSegmentContext");

	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnSegmentTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuSegmentTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnAddSegment(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId newItem;
	if (!activeSegment.IsOk() || segmentTree->GetChildrenCount(segmentRoot) <= 0) {
		vector<Triangle> shapeTris;
		project->GetWorkNif()->GetTrisForShape(activeItem->shapeName, &shapeTris);

		set<uint> tris;
		for (int id = 0; id < tris.size(); id++)
			tris.insert(id);

		newItem = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(tris));
	}
	else
		newItem = segmentTree->InsertItem(segmentRoot, activeSegment, "Segment", -1, -1, new SegmentItemData(set<uint>()));

	if (newItem.IsOk()) {
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
}

void OutfitStudio::OnAddSubSegment(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId newItem;
	wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
	if (parent == segmentRoot) {
		set<uint> tris;
		if (segmentTree->GetChildrenCount(activeSegment) <= 0) {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData)
				tris = segmentData->tris;
		}

		newItem = segmentTree->PrependItem(activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(tris, 0xFFFFFFFF));
	}
	else
		newItem = segmentTree->InsertItem(parent, activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(set<uint>(), 0xFFFFFFFF));

	if (newItem.IsOk()) {
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
}

void OutfitStudio::OnDeleteSegment(wxCommandEvent& WXUNUSED(event)) {
	SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (segmentData) {
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SegmentItemData* siblingData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData) {
				for (auto &id : segmentData->tris)
					siblingData->tris.insert(id);

				wxTreeItemIdValue cookie;
				wxTreeItemId child = segmentTree->GetFirstChild(sibling, cookie);
				if (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						for (auto &id : segmentData->tris)
							childData->tris.insert(id);
					}
				}
			}

			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(sibling);
		}
		else {
			segmentTree->Delete(activeSegment);
			activeSegment.Unset();
		}
	}

	UpdateSegmentNames();
}

void OutfitStudio::OnDeleteSubSegment(wxCommandEvent& WXUNUSED(event)) {
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		set<uint> tris = subSegmentData->tris;
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SubSegmentItemData* siblingData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				for (auto &id : tris)
					siblingData->tris.insert(id);

			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(sibling);
		}
		else {
			wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(parent);
		}
	}

	UpdateSegmentNames();
}

void OutfitStudio::OnSegmentTypeChanged(wxCommandEvent& event) {
	wxChoice* segmentType = (wxChoice*)event.GetEventObject();

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			unsigned long type = 0xFFFFFFFF;
			segmentType->GetStringSelection().Prepend("0x").ToULong(&type, 16);
			subSegmentData->type = type;
		}
	}
}

void OutfitStudio::OnSegmentApply(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);

	BSSubIndexTriShape::BSSITSSegmentation segmentation;

	uint parentArrayIndex = 0;
	uint segmentIndex = 0;
	uint triangleOffset = 0;

	vector<uint> triangles;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData) {
			// Create new segment
			BSSubIndexTriShape::BSSITSSegment segment;
			segment.numPrimitives = segmentData->tris.size();
			segment.startIndex = triangleOffset;

			size_t childCount = segmentTree->GetChildrenCount(child);
			segment.numSubSegments = childCount;

			// Create new segment data record
			BSSubIndexTriShape::BSSITSSubSegmentDataRecord segmentDataRecord;
			segmentDataRecord.segmentUser = segmentIndex;

			segmentation.subSegmentData.arrayIndices.push_back(parentArrayIndex);
			segmentation.subSegmentData.dataRecords.push_back(segmentDataRecord);

			if (childCount > 0) {
				// Add all triangles from the subsegments of the segment
				uint subSegmentNumber = 1;
				wxTreeItemIdValue subCookie;
				wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
				while (subChild.IsOk()) {
					SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
					if (subSegmentData) {
						// Create new subsegment
						BSSubIndexTriShape::BSSITSSubSegment subSegment;
						subSegment.arrayIndex = parentArrayIndex;
						subSegment.numPrimitives = subSegmentData->tris.size();
						subSegment.startIndex = triangleOffset;

						triangleOffset += subSegmentData->tris.size() * 3;
						for (auto &id : subSegmentData->tris)
							triangles.push_back(id);

						segment.subSegments.push_back(subSegment);

						// Create new subsegment data record
						BSSubIndexTriShape::BSSITSSubSegmentDataRecord subSegmentDataRecord;
						subSegmentDataRecord.segmentUser = subSegmentNumber;
						subSegmentDataRecord.unkInt2 = subSegmentData->type;
						subSegmentDataRecord.numData = subSegmentData->extraData.size();
						subSegmentDataRecord.extraData = subSegmentData->extraData;

						segmentation.subSegmentData.dataRecords.push_back(subSegmentDataRecord);
						subSegmentNumber++;
					}

					subChild = segmentTree->GetNextChild(activeSegment, subCookie);
				}
			}
			else {
				// No subsegments, add the triangles of the segment itself
				triangleOffset += segmentData->tris.size() * 3;
				for (auto &id : segmentData->tris)
					triangles.push_back(id);
			}

			segmentation.segments.push_back(segment);

			parentArrayIndex += childCount + 1;
			segmentIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}

	if (!project->GetWorkNif()->ReorderTriangles(activeItem->shapeName, triangles))
		return;

	segmentation.numPrimitives = triangles.size();
	segmentation.numSegments = segmentIndex;
	segmentation.numTotalSegments = parentArrayIndex;

	segmentation.subSegmentData.numSegments = segmentIndex;
	segmentation.subSegmentData.numTotalSegments = parentArrayIndex;

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	segmentation.subSegmentData.ssfFile.SetString(segmentSSF->GetValue().ToStdString());

	project->GetWorkNif()->SetShapeSegments(activeItem->shapeName, segmentation);
	CreateSegmentTree(activeItem->shapeName);
}

void OutfitStudio::OnSegmentReset(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);
	CreateSegmentTree(activeItem->shapeName);
}

void OutfitStudio::CreateSegmentTree(const string& shapeName) {
	if (segmentTree->GetChildrenCount(segmentRoot) > 0)
		segmentTree->DeleteChildren(segmentRoot);

	int arrayIndex = 0;
	BSSubIndexTriShape::BSSITSSegmentation segmentation;

	if (project->GetWorkNif()->GetShapeSegments(shapeName, segmentation)) {
		for (int i = 0; i < segmentation.segments.size(); i++) {
			uint startIndex = segmentation.segments[i].startIndex / 3;
			set<uint> tris;
			for (int id = startIndex; id < startIndex + segmentation.segments[i].numPrimitives; id++)
				tris.insert(id);

			wxTreeItemId segID = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(tris));
			if (segID.IsOk()) {
				for (int j = 0; j < segmentation.segments[i].subSegments.size(); j++) {
					startIndex = segmentation.segments[i].subSegments[j].startIndex / 3;
					set<uint> subTris;
					for (int id = startIndex; id < startIndex + segmentation.segments[i].subSegments[j].numPrimitives; id++)
						subTris.insert(id);

					arrayIndex++;
					segmentTree->AppendItem(segID, "Sub Segment", -1, -1,
						new SubSegmentItemData(subTris, segmentation.subSegmentData.dataRecords[arrayIndex].unkInt2, segmentation.subSegmentData.dataRecords[arrayIndex].extraData));
				}
			}
			arrayIndex++;
		}
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	segmentSSF->ChangeValue(segmentation.subSegmentData.ssfFile.GetString());

	UpdateSegmentNames();
	segmentTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	if (child.IsOk())
		segmentTree->SelectItem(child);
}

void OutfitStudio::ShowSegment(const wxTreeItemId& item, bool updateFromMask) {
	if (!glView->GetSegmentMode())
		return;

	unordered_map<ushort, float> mask;
	wxChoice* segmentType = nullptr;
	if (!updateFromMask) {
		segmentType = (wxChoice*)FindWindowByName("segmentType");
		segmentType->Disable();
		segmentType->SetSelection(0);
	}
	else
		glView->GetActiveMask(mask);

	if (item.IsOk())
		activeSegment = item;

	if (!activeSegment.IsOk() || !segmentTree->GetItemParent(activeSegment).IsOk())
		return;

	// Get all triangles of the active shape
	vector<Triangle> tris;
	project->GetWorkNif()->GetTrisForShape(activeItem->shapeName, &tris);

	wxTreeItemId parent;
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		// Active segment is a subsegment
		parent = segmentTree->GetItemParent(activeSegment);

		if (updateFromMask) {
			subSegmentData->tris.clear();

			// Add triangles from mask
			for (int t = 0; t < tris.size(); t++) {
				if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end())
					subSegmentData->tris.insert(t);
			}

			// Remove new triangles from all other subsegments of the parent segment
			wxTreeItemIdValue cookie;
			wxTreeItemId child = segmentTree->GetFirstChild(parent, cookie);
			while (child.IsOk()) {
				SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childSubSegmentData && childSubSegmentData != subSegmentData)
					for (auto &tri : subSegmentData->tris)
						if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
							childSubSegmentData->tris.erase(tri);

				child = segmentTree->GetNextChild(parent, cookie);
			}

			// Add new triangles to the parent segment as well
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
			if (segmentData) {
				for (auto &tri : subSegmentData->tris)
					segmentData->tris.insert(tri);
			}

			// Remove new triangles from all other segments and their subsegments
			wxTreeItemIdValue segCookie;
			wxTreeItemId segChild = segmentTree->GetFirstChild(segmentRoot, segCookie);
			while (segChild.IsOk()) {
				SegmentItemData* childSegmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(segChild));
				if (childSegmentData && childSegmentData != segmentData) {
					wxTreeItemIdValue subCookie;
					wxTreeItemId subChild = segmentTree->GetFirstChild(segChild, subCookie);
					while (subChild.IsOk()) {
						SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
						if (childSubSegmentData)
							for (auto &tri : segmentData->tris)
								if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
									childSubSegmentData->tris.erase(tri);

						subChild = segmentTree->GetNextChild(segChild, subCookie);
					}

					for (auto &parentTri : segmentData->tris)
						if (childSegmentData->tris.find(parentTri) != childSegmentData->tris.end())
							childSegmentData->tris.erase(parentTri);
				}

				segChild = segmentTree->GetNextChild(segmentRoot, segCookie);
			}
		}
		else {
			segmentType->Enable();
			segmentType->SetStringSelection(wxString::Format("%x", subSegmentData->type));
		}
	}
	else {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (segmentData) {
			// Active segment is a normal segment
			parent = activeSegment;

			if (updateFromMask) {
				segmentData->tris.clear();

				// Add triangles from mask
				for (int t = 0; t < tris.size(); t++) {
					if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end())
						segmentData->tris.insert(t);
				}

				// Remove new triangles from all other segments and their subsegments
				wxTreeItemIdValue segCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, segCookie);
				while (child.IsOk()) {
					SegmentItemData* childSegmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
					if (childSegmentData && childSegmentData != segmentData) {
						wxTreeItemIdValue subCookie;
						wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
						while (subChild.IsOk()) {
							SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
							if (childSubSegmentData)
								for (auto &tri : segmentData->tris)
									if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
										childSubSegmentData->tris.erase(tri);

							subChild = segmentTree->GetNextChild(child, subCookie);
						}

						for (auto &parentTri : segmentData->tris)
							if (childSegmentData->tris.find(parentTri) != childSegmentData->tris.end())
								childSegmentData->tris.erase(parentTri);
					}

					child = segmentTree->GetNextChild(segmentRoot, segCookie);
				}

				// Check if all triangles of the segment are assigned to any subsegment
				for (auto &parentTri : segmentData->tris) {
					bool found = false;
					wxTreeItemIdValue subCookie;
					wxTreeItemId subChild = segmentTree->GetFirstChild(activeSegment, subCookie);
					while (subChild.IsOk()) {
						SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
						if (childSubSegmentData)
							if (childSubSegmentData->tris.find(parentTri) != childSubSegmentData->tris.end())
								found = true;

						if (found)
							break;

						subChild = segmentTree->GetNextChild(activeSegment, subCookie);
					}

					// If not, add it to the last subsegment
					if (!found) {
						wxTreeItemId last = segmentTree->GetLastChild(activeSegment);
						if (last.IsOk()) {
							SubSegmentItemData* lastSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(last));
							if (lastSubSegmentData)
								lastSubSegmentData->tris.insert(parentTri);
						}
					}
				}
			}
		}
	}

	// Display segmentation colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->shapeName);
	if (m) {
		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));

		if (parent.IsOk()) {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
			if (segmentData) {
				size_t childCount = segmentTree->GetChildrenCount(parent);
				if (childCount > 0) {
					// Apply dynamic color to all subsegments of the segment
					float color = 0.0f;
					wxTreeItemIdValue cookie;
					wxTreeItemId child = segmentTree->GetFirstChild(parent, cookie);
					while (child.IsOk()) {
						SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
						if (childSubSegmentData) {
							color += 1.0f / childCount;

							if (childSubSegmentData != subSegmentData) {
								for (auto &id : childSubSegmentData->tris) {
									if (tris.size() <= id)
										continue;

									m->vcolors[tris[id].p1].y = color;
									m->vcolors[tris[id].p2].y = color;
									m->vcolors[tris[id].p3].y = color;
								}
							}
							else {
								for (auto &id : childSubSegmentData->tris) {
									if (tris.size() <= id)
										continue;

									m->vcolors[tris[id].p1].x = 1.0f;
									m->vcolors[tris[id].p2].x = 1.0f;
									m->vcolors[tris[id].p3].x = 1.0f;
									m->vcolors[tris[id].p1].z = color;
									m->vcolors[tris[id].p2].z = color;
									m->vcolors[tris[id].p3].z = color;
								}
							}
						}

						child = segmentTree->GetNextChild(activeSegment, cookie);
					}
				}
				else {
					// No subsegments, apply fixed color to segment
					for (auto &id : segmentData->tris) {
						if (tris.size() <= id)
							continue;

						m->vcolors[tris[id].p1].x = 1.0f;
						m->vcolors[tris[id].p2].x = 1.0f;
						m->vcolors[tris[id].p3].x = 1.0f;
						m->vcolors[tris[id].p1].z = 1.0f;
						m->vcolors[tris[id].p2].z = 1.0f;
						m->vcolors[tris[id].p3].z = 1.0f;
					}
				}
			}
		}
	}

	glView->Render();
}

void OutfitStudio::UpdateSegmentNames() {
	int segmentIndex = 0;
	int arrayIndex = 0;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		segmentTree->SetItemText(child, wxString::Format("Segment #%d, Array #%d", segmentIndex, arrayIndex));

		int subSegmentIndex = 0;
		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);

		while (subChild.IsOk()) {
			segmentTree->SetItemText(subChild, wxString::Format("Sub Segment #%d, Array #%d", subSegmentIndex, arrayIndex));

			subChild = segmentTree->GetNextChild(child, subCookie);
			subSegmentIndex++;
			arrayIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
		segmentIndex++;
		arrayIndex++;
	}
}

void OutfitStudio::OnPartitionSelect(wxTreeEvent& event) {
	ShowPartition(event.GetItem());

	wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
	partitionApply->Enable();

	wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");
	partitionReset->Enable();
}

void OutfitStudio::OnPartitionContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	partitionTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(event.GetItem()));
	if (partitionData)
		menu = wxXmlResource::Get()->LoadMenu("menuPartitionContext");

	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnPartitionTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuPartitionTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudio::OnAddPartition(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId newItem;
	if (!activePartition.IsOk() || partitionTree->GetChildrenCount(partitionRoot) <= 0) {
		int vertCount = project->GetWorkNif()->GetVertCountForShape(activeItem->shapeName);
		if (vertCount > 0) {
			vector<ushort> verts(vertCount);
			for (int id = 0; id < verts.size(); id++)
				verts[id] = id;

			vector<Triangle> tris;
			project->GetWorkNif()->GetTrisForShape(activeItem->shapeName, &tris);

			newItem = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(verts, tris, targetGame == SKYRIM || targetGame == SKYRIMSE ? 32 : 0));
		}
	}
	else
		newItem = partitionTree->InsertItem(partitionRoot, activePartition, "Partition", -1, -1, new PartitionItemData(vector<ushort>(), vector<Triangle>(), targetGame == SKYRIM || targetGame == SKYRIMSE ? 32 : 0));

	if (newItem.IsOk()) {
		partitionTree->UnselectAll();
		partitionTree->SelectItem(newItem);
	}
	
	UpdatePartitionNames();
}

void OutfitStudio::OnDeletePartition(wxCommandEvent& WXUNUSED(event)) {
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		wxTreeItemId sibling = partitionTree->GetPrevSibling(activePartition);
		if (!sibling.IsOk())
			sibling = partitionTree->GetNextSibling(activePartition);

		if (sibling.IsOk()) {
			PartitionItemData* siblingData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(sibling));
			if (siblingData) {
				for (auto &id : partitionData->verts)
					siblingData->verts.push_back(id);
			}

			partitionTree->UnselectAll();
			partitionTree->Delete(activePartition);
			partitionTree->SelectItem(sibling);

			// Force update from mask to fix triangles
			ShowPartition(sibling, true);
		}
		else {
			partitionTree->Delete(activePartition);
			activePartition.Unset();
		}
	}

	UpdatePartitionNames();
}

void OutfitStudio::OnPartitionTypeChanged(wxCommandEvent& event) {
	wxChoice* partitionType = (wxChoice*)event.GetEventObject();

	if (activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			unsigned long type = 0;
			partitionType->GetStringSelection().ToULong(&type);
			partitionData->type = type;
		}
	}

	UpdatePartitionNames();
}

void OutfitStudio::OnPartitionApply(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);

	vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	vector<vector<ushort>> partitionVerts;
	vector<vector<Triangle>> partitionTris;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			BSDismemberSkinInstance::PartitionInfo pInfo;
			pInfo.flags = 1;
			pInfo.partID = partitionData->type;
			partitionInfo.push_back(pInfo);

			partitionVerts.push_back(partitionData->verts);
			partitionTris.push_back(partitionData->tris);
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	project->GetWorkNif()->SetShapePartitions(activeItem->shapeName, partitionInfo, partitionVerts, partitionTris);
	CreatePartitionTree(activeItem->shapeName);
}

void OutfitStudio::OnPartitionReset(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);
	CreatePartitionTree(activeItem->shapeName);
}

void OutfitStudio::CreatePartitionTree(const string& shapeName) {
	if (partitionTree->GetChildrenCount(partitionRoot) > 0)
		partitionTree->DeleteChildren(partitionRoot);

	vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	vector<vector<ushort>> partitionVerts;
	vector<vector<Triangle>> partitionTris;
	if (project->GetWorkNif()->GetShapePartitions(shapeName, partitionInfo, partitionVerts, partitionTris)) {
		partitionInfo.resize(partitionVerts.size());

		for (int i = 0; i < partitionVerts.size(); i++)
			partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(partitionVerts[i], partitionTris[i], partitionInfo[i].partID));
	}

	UpdatePartitionNames();
	partitionTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	if (child.IsOk())
		partitionTree->SelectItem(child);
}

void OutfitStudio::ShowPartition(const wxTreeItemId& item, bool updateFromMask) {
	if (!glView->GetSegmentMode())
		return;

	unordered_map<ushort, float> mask;
	wxChoice* partitionType = nullptr;
	wxArrayString partitionStrings;
	if (!updateFromMask) {
		partitionType = (wxChoice*)FindWindowByName("partitionType");
		partitionType->Disable();
		partitionType->SetSelection(0);
		partitionStrings = partitionType->GetStrings();
	}
	else
		glView->GetActiveMask(mask);

	if (item.IsOk())
		activePartition = item;

	if (!activePartition.IsOk() || !partitionTree->GetItemParent(activePartition).IsOk())
		return;

	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		if (!updateFromMask) {
			for (auto &s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					// Show correct data in UI
					partitionType->Enable();
					partitionType->SetStringSelection(s);
				}
			}
		}
		else {
			// Get all triangles of the active shape
			vector<Triangle> tris;
			project->GetWorkNif()->GetTrisForShape(activeItem->shapeName, &tris);

			BlockType shapeType = project->GetWorkNif()->GetShapeType(activeItem->shapeName);
			bool isBSTri = (shapeType == BSTRISHAPE || shapeType == BSDYNAMICTRISHAPE);

			// Add vertices and triangles from mask
			set<Triangle> realTris;
			partitionData->verts.clear();
			partitionData->tris.clear();
			for (auto &tri : tris) {
				if (mask.find(tri.p1) != mask.end() && mask.find(tri.p2) != mask.end() && mask.find(tri.p3) != mask.end()) {
					Triangle partTri;

					auto p1Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p1);
					if (p1Find == partitionData->verts.end()) {
						partTri.p1 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p1);
					}
					else
						partTri.p1 = p1Find - partitionData->verts.begin();

					auto p2Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p2);
					if (p2Find == partitionData->verts.end()) {
						partTri.p2 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p2);
					}
					else
						partTri.p2 = p2Find - partitionData->verts.begin();

					auto p3Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p3);
					if (p3Find == partitionData->verts.end()) {
						partTri.p3 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p3);
					}
					else
						partTri.p3 = p3Find - partitionData->verts.begin();

					tri.rot();
					partTri.rot();

					if (isBSTri)
						partitionData->tris.push_back(tri);
					else
						partitionData->tris.push_back(partTri);

					realTris.insert(tri);
				}
			}

			// Vertex indices that are assigned
			set<ushort> vertsToCheck;
			if (!isBSTri) {
				for (auto &tri : partitionData->tris) {
					vertsToCheck.insert(partitionData->verts[tri.p1]);
					vertsToCheck.insert(partitionData->verts[tri.p2]);
					vertsToCheck.insert(partitionData->verts[tri.p3]);
				}
			}
			else {
				for (auto &tri : partitionData->tris) {
					vertsToCheck.insert(tri.p1);
					vertsToCheck.insert(tri.p2);
					vertsToCheck.insert(tri.p3);
				}
			}

			// Remove assigned verts/tris from all other partitions
			wxTreeItemIdValue cookie;
			wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
			while (child.IsOk()) {
				PartitionItemData* childPartition = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
				if (childPartition && childPartition != partitionData) {
					// Move triangles that match to the end
					auto removeTriEnd = childPartition->tris.end();
					if (!isBSTri) {
						removeTriEnd = partition(childPartition->tris.begin(), childPartition->tris.end(), [&childPartition, &realTris](const Triangle& tri) {
							Triangle t(childPartition->verts[tri.p1], childPartition->verts[tri.p2], childPartition->verts[tri.p3]);

							if (find_if(realTris.begin(), realTris.end(), [&t](const Triangle& tri) { return tri.CompareIndices(t); }) != realTris.end())
								return true;

							return false;
						});
					}
					else {
						removeTriEnd = partition(childPartition->tris.begin(), childPartition->tris.end(), [&childPartition, &realTris](const Triangle& tri) {
							if (find_if(realTris.begin(), realTris.end(), [&tri](const Triangle& t) { return t.CompareIndices(tri); }) != realTris.end())
								return true;

							return false;
						});
					}

					// Find vertices that need to be removed
					set<ushort> vertsToRemove;
					auto tri = removeTriEnd;
					auto triEnd = childPartition->tris.end();
					bool removeVert;
					for (auto &v : vertsToCheck) {
						removeVert = true;
						for (tri = removeTriEnd; tri < triEnd; ++tri) {
							const Triangle& t = (*tri);
							if (!isBSTri) {
								if (v == childPartition->verts[t.p1] ||
									v == childPartition->verts[t.p2] ||
									v == childPartition->verts[t.p3]) {
									removeVert = false;
									break;
								}
							}
							else {
								if (v == t.p1 || v == t.p2 || v == t.p3) {
									removeVert = false;
									break;
								}
							}
						}

						if (removeVert)
							vertsToRemove.insert(v);
					}

					// Find vertices that need to be decremented before erasing tris
					set<ushort> vertsToDecrement;
					if (!isBSTri) {
						for (auto &v : vertsToRemove) {
							for (auto itTri = childPartition->tris.begin(); itTri < removeTriEnd; ++itTri) {
								const Triangle& tri = (*itTri);
								if (v == childPartition->verts[tri.p1])
									vertsToDecrement.insert(tri.p1);
								else if (v == childPartition->verts[tri.p2])
									vertsToDecrement.insert(tri.p2);
								else if (v == childPartition->verts[tri.p3])
									vertsToDecrement.insert(tri.p3);
							}
						}
					}

					// Erase triangles from end
					childPartition->tris.erase(childPartition->tris.begin(), removeTriEnd);

					// Decrement vertex indices in tris
					if (!isBSTri) {
						for (auto &t : childPartition->tris) {
							ushort* p = &t.p1;
							for (int i = 0; i < 3; i++) {
								int pRem = 0;
								for (auto &remPos : vertsToDecrement)
									if (p[i] > remPos)
										pRem++;

								p[i] -= pRem;
							}
						}
					}

					// Erase vertices marked for removal
					auto removeVertEnd = remove_if(childPartition->verts.begin(), childPartition->verts.end(), [&vertsToRemove](const ushort& vert) {
						return (vertsToRemove.find(vert) != vertsToRemove.end());
					});

					childPartition->verts.erase(removeVertEnd, childPartition->verts.end());
				}

				child = partitionTree->GetNextChild(partitionRoot, cookie);
			}
		}
	}

	// Display partition colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->shapeName);
	if (m) {
		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));

		float color = 0.0f;
		wxTreeItemIdValue cookie;
		wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
		size_t childCount = partitionTree->GetChildrenCount(partitionRoot) + 1;
		while (child.IsOk()) {
			PartitionItemData* childPartition = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
			if (childPartition && childPartition != partitionData) {
				color += 1.0f / childCount;

				for (auto &v : childPartition->verts)
					m->vcolors[v].y = color;
			}

			child = partitionTree->GetNextChild(partitionRoot, cookie);
		}

		if (partitionData) {
			for (auto &v : partitionData->verts) {
				m->vcolors[v].x = 1.0f;
				m->vcolors[v].z = 1.0f;
			}
		}
	}

	glView->Render();
}

void OutfitStudio::UpdatePartitionNames() {
	wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
	wxArrayString partitionStrings = partitionType->GetStrings();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			bool found = false;
			for (auto &s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					partitionTree->SetItemText(child, s);
					found = true;
					break;
				}
			}

			if (!found)
				partitionTree->SetItemText(child, wxString::Format("%d Unknown", partitionData->type));
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
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

void OutfitStudio::OnSelectTool(wxCommandEvent& event) {
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
		
		ToggleBrushPane(true);
		return;
	}

	if (id == XRCID("btnTransform")) {
		bool checked = event.IsChecked();
		menuBar->Check(XRCID("btnTransform"), checked);
		toolBar->ToggleTool(XRCID("btnTransform"), checked);
		glView->SetTransformMode(checked);
		return;
	}

	if (id == XRCID("btnVertexEdit")) {
		bool checked = event.IsChecked();
		menuBar->Check(XRCID("btnVertexEdit"), checked);
		toolBar->ToggleTool(XRCID("btnVertexEdit"), checked);
		glView->SetVertexEdit(checked);
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
		glView->SetTransformMode(false);
		return;
	}

	// One of the brushes was activated
	glView->SetEditMode();
	glView->SetBrushSize(glView->GetBrushSize());

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
	int fieldOfView = fovSlider->GetValue();

	wxStaticText* fovLabel = (wxStaticText*)GetToolBar()->FindWindowByName("fovLabel");
	fovLabel->SetLabel(wxString::Format(_("Field of View: %d"), fieldOfView));

	glView->SetFieldOfView(fieldOfView);
}

void OutfitStudio::OnUpdateLights(wxCommandEvent& WXUNUSED(event)) {
	wxSlider* ambientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	wxSlider* frontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	wxSlider* directional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	wxSlider* directional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	wxSlider* directional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");

	int ambient = ambientSlider->GetValue();
	int frontal = frontalSlider->GetValue();
	int directional0 = directional0Slider->GetValue();
	int directional1 = directional1Slider->GetValue();
	int directional2 = directional2Slider->GetValue();
	glView->UpdateLights(ambient, frontal, directional0, directional1, directional2);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Frontal", frontal);
	Config.SetValue("Lights/Directional0", directional0);
	Config.SetValue("Lights/Directional1", directional1);
	Config.SetValue("Lights/Directional2", directional2);
}

void OutfitStudio::OnResetLights(wxCommandEvent& WXUNUSED(event)) {
	int ambient = 10;
	int frontal = 20;
	int directional0 = 60;
	int directional1 = 60;
	int directional2 = 85;

	glView->UpdateLights(ambient, frontal, directional0, directional1, directional2);

	wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	lightAmbientSlider->SetValue(ambient);

	wxSlider* lightFrontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	lightFrontalSlider->SetValue(frontal);

	wxSlider* lightDirectional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	lightDirectional0Slider->SetValue(directional0);

	wxSlider* lightDirectional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	lightDirectional1Slider->SetValue(directional1);

	wxSlider* lightDirectional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");
	lightDirectional2Slider->SetValue(directional2);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Frontal", frontal);
	Config.SetValue("Lights/Directional0", directional0);
	Config.SetValue("Lights/Directional1", directional1);
	Config.SetValue("Lights/Directional2", directional2);
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

	string sliderName = sn.ToStdString();

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	SliderDisplay* d = sliderDisplays[sliderName];
	d->slider->SetValue(v);

	int index = project->SliderIndexFromName(sliderName);
	project->SliderValue(index) = v / 100.0f;

	ApplySliders();
}

void OutfitStudio::OnTabButtonClick(wxCommandEvent& event) {
	int id = event.GetId();

	if (id != XRCID("segmentTabButton")) {
		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show(false);
		segmentType->Show(false);
		segmentSSFLabel->Show(false);
		segmentSSF->Show(false);
		segmentApply->Show(false);
		segmentReset->Show(false);

		if (glView->GetSegmentMode())
			glView->ClearColorChannels();

		glView->SetSegmentMode(false);
		glView->SetSegmentsVisible(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		GetMenuBar()->Check(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Check(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Enable(XRCID("btnSelect"), true);
		GetMenuBar()->Enable(XRCID("btnClearMask"), true);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), true);
		GetMenuBar()->Enable(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != XRCID("partitionTabButton")) {
		wxStaticText* partitionTypeLabel = (wxStaticText*)FindWindowByName("partitionTypeLabel");
		wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
		wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
		wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");

		partitionTypeLabel->Show(false);
		partitionType->Show(false);
		partitionApply->Show(false);
		partitionReset->Show(false);

		if (glView->GetSegmentMode())
			glView->ClearColorChannels();

		glView->SetSegmentMode(false);
		glView->SetSegmentsVisible(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		GetMenuBar()->Check(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Check(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Enable(XRCID("btnSelect"), true);
		GetMenuBar()->Enable(XRCID("btnClearMask"), true);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), true);
		GetMenuBar()->Enable(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != XRCID("boneTabButton")) {
		boneScale->Show(false);

		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");

		boneScaleLabel->Show(false);
		cbFixedWeight->Show(false);

		project->ClearBoneScale();
		project->workWeights.clear();

		glView->SetXMirror(previousMirror);
		glView->SetTransformMode(false);
		glView->SetActiveBrush(1);
		glView->SetEditMode();
		glView->SetWeightVisible(false);

		GetMenuBar()->Check(XRCID("btnXMirror"), previousMirror);
		GetMenuBar()->Check(XRCID("btnInflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnTransform"), true);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), true);
		GetMenuBar()->Enable(XRCID("btnWeightBrush"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), true);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnInflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnWeightBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), true);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), true);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), true);
	}

	if (id == XRCID("meshTabButton")) {
		outfitBones->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitShapes->Show();

		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);
	}
	else if (id == XRCID("boneTabButton")) {
		outfitShapes->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitBones->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		boneScale->SetValue(0);
		boneScale->Show();
		
		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");

		boneScaleLabel->Show();
		cbFixedWeight->Show();
		
		glView->SetTransformMode(false);
		glView->SetActiveBrush(10);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetEditMode();
		glView->SetWeightVisible();

		GetMenuBar()->Check(XRCID("btnWeightBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Enable(XRCID("btnWeightBrush"), true);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnWeightBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnWeightBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);
	}
	else if (id == XRCID("segmentTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		segmentTree->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show();
		segmentType->Show();
		segmentSSFLabel->Show();
		segmentSSF->Show();
		segmentApply->Show();
		segmentReset->Show();

		glView->SetActiveBrush(0);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetSegmentMode();
		glView->SetEditMode();
		glView->SetSegmentsVisible();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);

		GetMenuBar()->Check(XRCID("btnMaskBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Check(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Check(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("btnSelect"), false);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Enable(XRCID("btnClearMask"), false);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), false);
		GetMenuBar()->Enable(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnMaskBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), false);
		GetToolBar()->EnableTool(XRCID("btnSelect"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), false);

		ShowSegment(segmentTree->GetSelection());
	}
	else if (id == XRCID("partitionTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		segmentTree->Hide();
		lightSettings->Hide();
		partitionTree->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		wxStaticText* partitionTypeLabel = (wxStaticText*)FindWindowByName("partitionTypeLabel");
		wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
		wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
		wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");

		partitionTypeLabel->Show();
		partitionType->Show();
		partitionApply->Show();
		partitionReset->Show();

		glView->SetActiveBrush(0);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetSegmentMode();
		glView->SetEditMode();
		glView->SetSegmentsVisible();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);

		GetMenuBar()->Check(XRCID("btnMaskBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Check(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Check(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("btnSelect"), false);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Enable(XRCID("btnClearMask"), false);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), false);
		GetMenuBar()->Enable(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnMaskBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), false);
		GetToolBar()->EnableTool(XRCID("btnSelect"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), false);

		ShowPartition(partitionTree->GetSelection());
	}
	else if (id == XRCID("lightsTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
	}

	CheckBrushBounds();
	UpdateBrushPane();

	wxPanel* topSplitPanel = (wxPanel*)FindWindowByName("topSplitPanel");
	wxPanel* bottomSplitPanel = (wxPanel*)FindWindowByName("bottomSplitPanel");

	topSplitPanel->Layout();
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
		boneScale->SetValue(0);
	}

	if (sliderName == "boneScale") {
		if (!activeBone.empty())
			project->ApplyBoneScale(activeBone, event.GetPosition(), true);

		glView->Render();
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

		dlg.SetSize(wxSize(325, 175));
		dlg.SetSizeHints(wxSize(325, 175), wxSize(-1, 175));
		dlg.CenterOnParent();

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
		wxMessageBox(_("There are no sliders loaded!"), _("Error"), wxICON_ERROR, this);
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
		wxMessageBox(_("No changes were made to the sliders, so no preset was saved!"), _("Info"), wxICON_INFORMATION, this);
		return;
	}

	int error = presets.SavePreset(fileName, presetName, "OutfitStudio", groups);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to save preset (%d)!"), error), _("Error"), wxICON_ERROR, this);
	}
}

void OutfitStudio::OnSliderImportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Import .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", activeSlider, activeItem->shapeName, fn);
	project->SetSliderFromBSD(activeSlider, activeItem->shapeName, fn);
	ApplySliders();
}

void OutfitStudio::OnSliderImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Import .obj file for slider calculation"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromOBJ(activeSlider, activeItem->shapeName, fn)) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	ApplySliders();
}

void OutfitStudio::OnSliderImportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Import .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	int result = wxMessageBox(_("This will delete all loaded sliders. Are you sure?"), _("OSD Import"), wxYES_NO | wxCANCEL | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Importing morphs from OSD file '%s'...", fn);

	OSDataFile osd;
	if (!osd.Read(fn)) {
		wxLogError("Failed to import OSD file '%s'!", fn);
		wxMessageBox(_("Failed to import OSD file!"), _("Error"), wxICON_ERROR);
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

	wxString addedDiffs;
	auto diffs = osd.GetDataDiffs();

	for (auto &s : shapes) {
		bool added = false;
		for (auto &diff : diffs) {
			// Diff name is supposed to begin with matching shape name
			if (diff.first.substr(0, s.size()) != s)
				continue;

			string diffName = diff.first.substr(s.length(), diff.first.length() - s.length() + 1);
			if (!project->ValidSlider(diffName)) {
				createSliderGUI(diffName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(diffName);
				ShowSliderEffect(diffName);
			}

			project->SetSliderFromDiff(diffName, s, diff.second);
			added = true;
		}

		if (added)
			addedDiffs += s + "\n";
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();
	ApplySliders();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedDiffs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedDiffs), _("OSD Import"));
}

void OutfitStudio::OnSliderImportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Import .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	int result = wxMessageBox(_("This will delete all loaded sliders. Are you sure?"), _("TRI Import"), wxYES_NO | wxCANCEL | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Importing morphs from TRI file '%s'...", fn);

	TriFile tri;
	if (!tri.Read(fn)) {
		wxLogError("Failed to load TRI file '%s'!", fn);
		wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
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
			if (!project->ValidSlider(morphData->name)) {
				createSliderGUI(morphData->name, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(morphData->name);
				ShowSliderEffect(morphData->name);
			}

			unordered_map<ushort, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			project->SetSliderFromDiff(morphData->name, morph.first, diff);
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();
	ApplySliders();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedMorphs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedMorphs), _("TRI Import"));
}

void OutfitStudio::OnSliderImportFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Import .fbx file for slider calculation"), wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromFBX(activeSlider, activeItem->shapeName, fn)) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	ApplySliders();
}

void OutfitStudio::OnSliderExportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector(_("Export .bsd slider data to directory"), wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		for (auto &i : selectedItems) {
			string targetFile = dir + "\\" + i->shapeName + "_" + activeSlider + ".bsd";
			wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderBSD(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		string fn = wxFileSelector(_("Export .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.empty())
			return;

		wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		project->SaveSliderBSD(activeSlider, activeItem->shapeName, fn);
	}

	ApplySliders();
}

void OutfitStudio::OnSliderExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector(_("Export .obj slider data to directory"), wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.empty())
			return;

		for (auto &i : selectedItems) {
			string targetFile = dir + "\\" + i->shapeName + "_" + activeSlider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderOBJ(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		string fn = wxFileSelector(_("Export .obj slider data"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.empty())
			return;

		wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		if (project->SaveSliderOBJ(activeSlider, activeItem->shapeName, fn)) {
			wxLogError("Failed to export OBJ file '%s'!", fn);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}

	ApplySliders();
}

void OutfitStudio::OnSliderExportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Export .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	wxLogMessage("Exporting OSD file to '%s'...", fn);
	if (!project->SaveSliderData(fn)) {
		wxLogError("Failed to export OSD file to '%s'!", fn);
		wxMessageBox(_("Failed to export OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudio::OnSliderExportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	string fn = wxFileSelector(_("Export .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	wxLogMessage("Exporting TRI morphs to '%s'...", fn);
	if (!project->WriteMorphTRI(fn)) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudio::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to clear data from!"), _("Error"));
		return;
	}

	int result;
	if (selectedItems.size() > 1) {
		string prompt = _("Are you sure you wish to clear the unmasked slider data \"");
		prompt += activeSlider + _("\" for the selected shapes? This cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}
	else {
		string prompt = _("Are you sure you wish to clear the unmasked slider data \"");
		prompt += activeSlider + _("\" for the shape \"") + activeItem->shapeName + _("\"? This cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
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
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	string namebase = "NewZap";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;

	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	string finalName = wxGetTextFromUser(_("Enter a name for the new zap:"), _("Create New Zap"), thename, this);
	if (finalName.empty())
		return;

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

	string finalName = wxGetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), thename, this);
	if (finalName.empty())
		return;

	wxLogMessage("Creating new combined slider '%s'.", finalName);
	createSliderGUI(finalName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddCombinedSlider(finalName);
	sliderScroll->FitInside();
}

void OutfitStudio::OnSliderNegate(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to negate!"), _("Error"));
		return;
	}

	wxLogMessage("Negating slider '%s' for the selected shapes.", activeSlider);
	for (auto &i : selectedItems)
		project->NegateSlider(activeSlider, i->shapeName);
}

void OutfitStudio::OnMaskAffected(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to create a mask from!"), _("Error"));
		return;
	}

	wxLogMessage("Creating mask for affected vertices of the slider '%s'.", activeSlider);
	for (auto &i : selectedItems)
		project->MaskAffected(activeSlider, i->shapeName);
}

void OutfitStudio::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to delete!"), _("Error"));
		return;
	}

	string prompt = _("Are you sure you wish to delete the slider \"");
	prompt += activeSlider + _("\"? This cannot be undone.");
	int result = wxMessageBox(prompt, _("Confirm slider delete"), wxYES_NO | wxICON_WARNING, this);
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
		wxMessageBox(_("There is no slider in edit mode to show properties for!"), _("Error"));
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		int curSlider = project->SliderIndexFromName(activeSlider);
		long loVal = (int)(project->SliderDefault(curSlider, false));
		long hiVal = (int)(project->SliderDefault(curSlider, true));
		XRCCTRL(dlg, "edSliderName", wxTextCtrl)->SetLabel(activeSlider);
		XRCCTRL(dlg, "edValLo", wxTextCtrl)->SetValue(wxString::Format("%d", loVal));
		XRCCTRL(dlg, "edValHi", wxTextCtrl)->SetValue(wxString::Format("%d", hiVal));

		if (!project->mGenWeights) {
			XRCCTRL(dlg, "lbValLo", wxStaticText)->Hide();
			XRCCTRL(dlg, "edValLo", wxTextCtrl)->Hide();
			XRCCTRL(dlg, "lbValHi", wxStaticText)->SetLabel("Default");
		}

		wxCheckBox* chkHidden = XRCCTRL(dlg, "chkHidden", wxCheckBox);
		wxCheckBox* chkInvert = XRCCTRL(dlg, "chkInvert", wxCheckBox);
		wxCheckBox* chkZap = XRCCTRL(dlg, "chkZap", wxCheckBox);
		wxCheckBox* chkUV = XRCCTRL(dlg, "chkUV", wxCheckBox);

		wxCheckListBox* zapToggleList = XRCCTRL(dlg, "zapToggleList", wxCheckListBox);
		chkZap->Bind(wxEVT_CHECKBOX, [&zapToggleList](wxCommandEvent& event) {
			zapToggleList->Enable(event.IsChecked());
		});

		if (project->SliderHidden(curSlider))
			chkHidden->SetValue(true);

		if (project->SliderInvert(curSlider))
			chkInvert->SetValue(true);

		if (project->SliderUV(curSlider))
			chkUV->SetValue(true);

		if (project->SliderZap(curSlider)) {
			chkZap->SetValue(true);
			zapToggleList->Enable();

			for (int i = 0; i < project->SliderCount(); i++)
				if (i != curSlider && project->SliderZap(i))
					zapToggleList->Append(project->GetSliderName(i));

			for (auto &s : project->SliderZapToggles(curSlider)) {
				int stringId = zapToggleList->FindString(s, true);
				if (stringId != wxNOT_FOUND)
					zapToggleList->Check(stringId);
			}
		}

		XRCCTRL(dlg, "wxID_CANCEL", wxButton)->SetFocus();

		if (dlg.ShowModal() == wxID_OK) {
			if (chkZap->IsChecked()) {
				project->SetSliderZap(curSlider, chkZap->GetValue());

				wxArrayString zapToggles;
				wxArrayInt toggled;
				zapToggleList->GetCheckedItems(toggled);
				for (auto &i : toggled)
					zapToggles.Add(zapToggleList->GetString(i));

				project->SetSliderZapToggles(curSlider, zapToggles);
			}

			project->SetSliderInvert(curSlider, chkInvert->GetValue());
			project->SetSliderHidden(curSlider, chkHidden->GetValue());
			project->SetSliderUV(curSlider, chkUV->GetValue());
			XRCCTRL(dlg, "edValLo", wxTextCtrl)->GetValue().ToLong(&loVal);
			project->SetSliderDefault(curSlider, loVal, false);
			XRCCTRL(dlg, "edValHi", wxTextCtrl)->GetValue().ToLong(&hiVal);
			project->SetSliderDefault(curSlider, hiVal, true);

			string sliderName = XRCCTRL(dlg, "edSliderName", wxTextCtrl)->GetValue().ToStdString();
			if (activeSlider != sliderName) {
				project->SetSliderName(curSlider, sliderName);
				SliderDisplay* d = sliderDisplays[activeSlider];
				sliderDisplays[sliderName] = d;
				sliderDisplays.erase(activeSlider);
				d->slider->SetName(sliderName + "|slider");
				d->sliderName->SetName(sliderName + "|lbl");
				d->btnSliderEdit->SetName(sliderName + "|btn");
				d->sliderNameCheck->SetName(sliderName + "|check");
				d->sliderReadout->SetName(sliderName + "|readout");
				d->sliderName->SetLabel(sliderName);
				activeSlider = sliderName;
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

	wxLogMessage("Conforming all shapes...");
	StartProgress(_("Conforming all shapes..."));
	int inc = 100 / shapes.size() - 1;
	int pos = 0;

	auto selectedItemsSave = selectedItems;

	curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		selectedItems.clear();
		selectedItems.push_back((ShapeItemData*)outfitShapes->GetItemData(curItem));
		UpdateProgress(pos * inc, _("Conforming: ") + selectedItems.front()->shapeName);
		StartSubProgress(pos * inc, pos * inc + inc);
		OnSliderConform(event);
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
		pos++;
	}

	selectedItems = selectedItemsSave;

	if (statusBar)
		statusBar->SetStatusText(_("All shapes conformed."));

	wxLogMessage("All shapes conformed.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudio::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	StartProgress(_("Conforming..."));
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
		UpdateProgress(1, _("Initializing data..."));
		project->InitConform();

		UpdateProgress(50, _("Conforming: ") + i->shapeName);

		project->morpher.CopyMeshMask(glView->GetMesh(i->shapeName), i->shapeName);
		project->ConformShape(i->shapeName);

		UpdateProgress(99);
	}

	project->morpher.ClearProximityCache();

	if (statusBar)
		statusBar->SetStatusText(_("Shape(s) conformed."));

	wxLogMessage("%d shape(s) conformed.", selectedItems.size());
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudio::OnImportShape(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector(_("Import .obj file for new shape"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing shape(s) from OBJ file '%s'...", fn);

	int ret;
	if (activeItem)
		ret = project->AddShapeFromObjFile(fn, project->OutfitName(), activeItem->shapeName);
	else
		ret = project->AddShapeFromObjFile(fn, project->OutfitName());

	if (ret == 101) {
		RefreshGUIFromProj();
		wxLogMessage("Updated shape '%s' from OBJ.", activeItem->shapeName);
		glView->Render();
		return;
	}
	else if (ret)
		return;

	RefreshGUIFromProj();
	wxLogMessage("Imported shape(s) from OBJ.");
	glView->Render();
}

void OutfitStudio::OnExportShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		string dir = wxDirSelector(_("Export selected shapes as .obj files"), wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
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
		string fileName = wxFileSelector(_("Export shape as an .obj file"), wxEmptyString, wxString(activeItem->shapeName + ".obj"), "", "Obj Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (!fileName.empty()) {
			wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", activeItem->shapeName, fileName);
			project->ClearBoneScale();
			if (project->ExportShapeObj(fileName, activeItem->shapeName, Vector3(0.1f, 0.1f, 0.1f))) {
				wxLogError("Failed to export OBJ file '%s'!", fileName);
				wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
			}
		}
	}
}

void OutfitStudio::OnImportFBX(wxCommandEvent& WXUNUSED(event)) {
	string fn = wxFileSelector(_("Import .fbx file for new shape"), wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.empty())
		return;

	wxLogMessage("Importing shape(s) from FBX file '%s'...", fn);

	int ret;
	if (activeItem)
		ret = project->ImportShapeFBX(fn, project->OutfitName(), activeItem->shapeName);
	else
		ret = project->ImportShapeFBX(fn, project->OutfitName());

	if (ret == 101) {
		RefreshGUIFromProj();
		wxLogMessage("Updated shape '%s' from FBX.", activeItem->shapeName);
		glView->Render();
		return;
	}
	if (ret)
		return;

	RefreshGUIFromProj();
	wxLogMessage("Imported shape(s) from FBX.");
	glView->Render();
}

void OutfitStudio::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
	string defFile = "OutfitStudioShapes.fbx";
	if (activeItem)
		defFile = activeItem->shapeName + ".fbx";

	string fn = wxFileSelector(_("Export .fbx file for new shape"), wxEmptyString, defFile, "", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.empty())
		return;

	if (activeItem)
		project->ExportShapeFBX(fn, activeItem->shapeName);
	else
		project->ExportShapeFBX(fn, "");
}

void OutfitStudio::OnInvertUV(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
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
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	string newShapeName;
	do {
		wxString result = wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"));
		if (result.empty())
			return;

		newShapeName = result;
	} while (project->IsValidShape(newShapeName));

	wxLogMessage("Renaming shape '%s' to '%s'.", activeItem->shapeName, newShapeName);
	project->RenameShape(activeItem->shapeName, newShapeName);
	glView->RenameShape(activeItem->shapeName, newShapeName);

	activeItem->shapeName = newShapeName;
	outfitShapes->SetItemText(activeItem->GetId(), newShapeName);
}

void OutfitStudio::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (!project->IsBaseShape(activeItem->shapeName))
		project->SetBaseShape(activeItem->shapeName);
	else
		project->SetBaseShape("");

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
		wxMessageBox(_("There is no shape selected!"), _("Error"));
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
			offs.x = atof(XRCCTRL(dlg, "msTextX", wxTextCtrl)->GetValue().c_str());
			offs.y = atof(XRCCTRL(dlg, "msTextY", wxTextCtrl)->GetValue().c_str());
			offs.z = atof(XRCCTRL(dlg, "msTextZ", wxTextCtrl)->GetValue().c_str());
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

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudio::OnMoveShapeOldOffset(wxCommandEvent& event) {
	wxWindow* parent = ((wxButton*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	XRCCTRL(*parent, "msSliderX", wxSlider)->SetValue(0);
	XRCCTRL(*parent, "msSliderY", wxSlider)->SetValue(-2544);
	XRCCTRL(*parent, "msSliderZ", wxSlider)->SetValue(3287);
	XRCCTRL(*parent, "msTextX", wxTextCtrl)->ChangeValue("0.00000");
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->ChangeValue("-2.54431");
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->ChangeValue("3.28790");

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

	XRCCTRL(*parent, "msTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.x));
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.y));
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.z));

	PreviewMove(slider);
}

void OutfitStudio::OnMoveShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;
	
	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "msTextX", wxTextCtrl)->GetValue().c_str());
	changed.y = atof(XRCCTRL(*parent, "msTextY", wxTextCtrl)->GetValue().c_str());
	changed.z = atof(XRCCTRL(*parent, "msTextZ", wxTextCtrl)->GetValue().c_str());

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

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudio::OnScaleShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		XRCCTRL(dlg, "ssSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudio::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnScaleShapeText, this);
		XRCCTRL(dlg, "ssTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnScaleShapeText, this);
		XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudio::OnScaleShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		Vector3 scale(1.0f, 1.0f, 1.0f);
		if (dlg.ShowModal() == wxID_OK) {
			scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
			scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
			scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());
		}

		Vector3 scaleNew = scale;
		scaleNew.x *= 1.0f / previewScale.x;
		scaleNew.y *= 1.0f / previewScale.y;
		scaleNew.z *= 1.0f / previewScale.z;

		unordered_map<ushort, float> mask;
		unordered_map<ushort, float>* mptr = nullptr;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			vector<Vector3> verts;
			if (bEditSlider) {
				auto& diff = previewDiff[i->shapeName];
				for (auto &d : diff)
					d.second *= -1.0f;

				project->UpdateMorphResult(i->shapeName, activeSlider, diff);
				project->GetLiveVerts(i->shapeName, verts);
				diff.clear();

				Vector3 d;
				float diffX, diffY, diffZ;
				int vertexCount = project->GetVertexCount(i->shapeName);
				for (int j = 0; j < vertexCount; j++) {
					d.x = verts[j].x * scale.x - verts[j].x;
					d.y = verts[j].y * scale.y - verts[j].y;
					d.z = verts[j].z * scale.z - verts[j].z;
					d *= 1.0f - mask[j];
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
			else
				project->ScaleShape(i->shapeName, scaleNew, mptr);

			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewScale = Vector3(1.0f, 1.0f, 1.0f);
		previewDiff.clear();

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit();
	}
}

void OutfitStudio::OnScaleShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 scale(1.0f, 1.0f, 1.0f);

	bool uniform = XRCCTRL(*parent, "ssUniform", wxCheckBox)->IsChecked();
	if (uniform) {
		float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / 1000.0f;
		scale = Vector3(uniformValue, uniformValue, uniformValue);

		XRCCTRL(*parent, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
		XRCCTRL(*parent, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
		XRCCTRL(*parent, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);
	}
	else {
		scale.x = XRCCTRL(*parent, "ssSliderX", wxSlider)->GetValue() / 1000.0f;
		scale.y = XRCCTRL(*parent, "ssSliderY", wxSlider)->GetValue() / 1000.0f;
		scale.z = XRCCTRL(*parent, "ssSliderZ", wxSlider)->GetValue() / 1000.0f;
	}

	XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
	XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
	XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));

	PreviewScale(scale);
}

void OutfitStudio::OnScaleShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 scale(1.0f, 1.0f, 1.0f);

	bool uniform = XRCCTRL(*parent, "ssUniform", wxCheckBox)->IsChecked();
	if (uniform) {
		float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
		scale = Vector3(uniformValue, uniformValue, uniformValue);

		XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
		XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
		XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
	}
	else {
		scale.x = atof(XRCCTRL(*parent, "ssTextX", wxTextCtrl)->GetValue().c_str());
		scale.y = atof(XRCCTRL(*parent, "ssTextY", wxTextCtrl)->GetValue().c_str());
		scale.z = atof(XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->GetValue().c_str());
	}

	if (scale.x < 0.01f) {
		scale.x = 0.01f;
		XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
	}

	if (scale.y < 0.01f) {
		scale.y = 0.01f;
		XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
	}

	if (scale.z < 0.01f) {
		scale.z = 0.01f;
		XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
	}

	XRCCTRL(*parent, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
	XRCCTRL(*parent, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
	XRCCTRL(*parent, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);

	PreviewScale(scale);
}

void OutfitStudio::PreviewScale(const Vector3& scale) {
	Vector3 scaleNew = scale;
	scaleNew.x *= 1.0f / previewScale.x;
	scaleNew.y *= 1.0f / previewScale.y;
	scaleNew.z *= 1.0f / previewScale.z;

	unordered_map<ushort, float> mask;
	unordered_map<ushort, float>* mptr = nullptr;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		vector<Vector3> verts;
		if (bEditSlider) {
			auto& diff = previewDiff[i->shapeName];
			for (auto &d : diff)
				d.second *= -1.0f;

			project->UpdateMorphResult(i->shapeName, activeSlider, diff);
			project->GetLiveVerts(i->shapeName, verts);
			diff.clear();

			Vector3 d;
			float diffX, diffY, diffZ;
			int vertexCount = project->GetVertexCount(i->shapeName);
			for (int j = 0; j < vertexCount; j++) {
				d.x = verts[j].x * scale.x - verts[j].x;
				d.y = verts[j].y * scale.y - verts[j].y;
				d.z = verts[j].z * scale.z - verts[j].z;
				d *= 1.0f - mask[j];
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
		else
			project->ScaleShape(i->shapeName, scaleNew, mptr);

		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewScale = scale;

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudio::OnRotateShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("Rotating slider data is currently not possible using the Rotate Shape dialog."), _("Rotate"), wxICON_INFORMATION, this);
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
			angle.x = atof(XRCCTRL(dlg, "rsTextX", wxTextCtrl)->GetValue().c_str());
			angle.y = atof(XRCCTRL(dlg, "rsTextY", wxTextCtrl)->GetValue().c_str());
			angle.z = atof(XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->GetValue().c_str());
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

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit();
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

	XRCCTRL(*parent, "rsTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.x));
	XRCCTRL(*parent, "rsTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.y));
	XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.z));

	PreviewRotation(slider);
}

void OutfitStudio::OnRotateShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "rsTextX", wxTextCtrl)->GetValue().c_str());
	changed.y = atof(XRCCTRL(*parent, "rsTextY", wxTextCtrl)->GetValue().c_str());
	changed.z = atof(XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->GetValue().c_str());

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

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudio::OnDeleteVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	if (wxMessageBox(_("Are you sure you wish to delete the unmasked vertices of the selected shapes?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO) == wxNO)
		return;

	for (auto &i : selectedItems) {
		unordered_map<ushort, float> mask;
		glView->GetShapeUnmasked(mask, i->shapeName);
		glView->TriangulateMask(mask, i->shapeName);
		project->DeleteVerts(i->shapeName, mask);
	}

	RefreshGUIFromProj();

	for (auto &s : sliderDisplays) {
		glView->SetStrokeManager(&s.second->sliderStrokes);
		glView->GetStrokeManager()->Clear();
	}

	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();
	glView->ClearMask();
	ApplySliders();
}

void OutfitStudio::OnDupeShape(wxCommandEvent& WXUNUSED(event)) {
	string newName;
	wxTreeItemId subitem;
	if (activeItem) {
		if (!outfitRoot.IsOk()) {
			wxMessageBox(_("You can only copy shapes into an outfit, and there is no outfit in the current project. Load one first!"));
			return;
		}

		do {
			wxString result = wxGetTextFromUser(_("Please enter a unique name for the duplicated shape."), _("Duplicate Shape"));
			if (result.empty())
				return;

			newName = result;
		} while (project->IsValidShape(newName));

		wxLogMessage("Duplicating shape '%s' as '%s'.", activeItem->shapeName, newName);
		project->ClearBoneScale();
		project->DuplicateShape(activeItem->shapeName, newName);

		glView->AddMeshFromNif(project->GetWorkNif(), newName, false);
		project->SetTextures(newName);

		MaterialFile matFile;
		bool hasMatFile = project->GetShapeMaterialFile(newName, matFile);
		glView->SetMeshTextures(newName, project->GetShapeTextures(newName), hasMatFile, matFile);

		subitem = outfitShapes->AppendItem(outfitRoot, newName);
		outfitShapes->SetItemState(subitem, 0);
		outfitShapes->SetItemData(subitem, new ShapeItemData(newName));
	}
}

void OutfitStudio::OnDeleteShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (wxMessageBox(_("Are you sure you wish to delete the selected shapes?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO) == wxNO)
		return;

	vector<ShapeItemData> selected;
	for (auto &i : selectedItems)
		selected.push_back(*i);

	for (auto &i : selected) {
		wxLogMessage("Deleting shape '%s'.", i.shapeName);
		project->DeleteShape(i.shapeName);
		glView->DeleteMesh(i.shapeName);
		wxTreeItemId item = i.GetId();
		outfitShapes->Delete(item);
	}

	AnimationGUIFromProj();
	glView->Render();
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
				wxMessageBox(_("No bone name was entered!"), _("Error"), wxICON_INFORMATION, this);
				return;
			}

			wxTreeItemIdValue cookie;
			wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
			while (item.IsOk()) {
				if (outfitBones->GetItemText(item) == bone) {
					wxMessageBox(wxString::Format(_("Bone '%s' already exists in the project!"), bone), _("Error"), wxICON_INFORMATION, this);
					return;
				}
				item = outfitBones->GetNextChild(bonesRoot, cookie);
			}

			Vector3 translation;
			translation.x = atof(XRCCTRL(dlg, "textX", wxTextCtrl)->GetValue().c_str());
			translation.y = atof(XRCCTRL(dlg, "textY", wxTextCtrl)->GetValue().c_str());
			translation.z = atof(XRCCTRL(dlg, "textZ", wxTextCtrl)->GetValue().c_str());

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

	ReselectBone();
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

	ReselectBone();
}

bool OutfitStudio::ShowWeightCopy(WeightCopyOptions& options) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyWeights")) {
		XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			float changed = XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->GetValue() / 1000.0f;
			changed = min(changed, 15.0f);
			changed = max(changed, 0.0f);
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", changed));
		});

		XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			float changed = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			changed = min(changed, 15.0f);
			changed = max(changed, 0.0f);
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(changed * 1000);
		});

		XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			int changed = XRCCTRL(dlg, "maxResultsSlider", wxSlider)->GetValue();
			changed = min(changed, 12);
			changed = max(changed, 0);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue(wxString::Format("%d", changed));
		});

		XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			int changed = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			changed = min(changed, 12);
			changed = max(changed, 0);
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->SetValue(changed);
		});

		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudio::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			options.proximityRadius = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			options.maxResults = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			return true;
		}
	}

	return false;
}

void OutfitStudio::ReselectBone() {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (!selItems.empty()) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems.front());
		OnBoneSelect(treeEvent);
	}
}

void OutfitStudio::OnCopyBoneWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (project->GetBaseShape().empty())
		return;

	WeightCopyOptions options;
	if (ShowWeightCopy(options)) {
		StartProgress(_("Copying bone weights..."));

		unordered_map<ushort, float> mask;
		for (int i = 0; i < selectedItems.size(); i++) {
			if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
				wxLogMessage("Copying bone weights to '%s'...", selectedItems[i]->shapeName);
				mask.clear();
				glView->GetShapeMask(mask, selectedItems[i]->shapeName);
				project->CopyBoneWeights(selectedItems[i]->shapeName, options.proximityRadius, options.maxResults, &mask);
			}
			else
				wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
		}

		project->morpher.ClearProximityCache();
		ReselectBone();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}
}

void OutfitStudio::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
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

	WeightCopyOptions options;
	if (ShowWeightCopy(options)) {
		StartProgress(_("Copying selected bone weights..."));

		unordered_map<ushort, float> mask;
		for (int i = 0; i < selectedItems.size(); i++) {
			if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
				wxLogMessage("Copying selected bone weights to '%s' for %s...", selectedItems[i]->shapeName, bonesString);
				mask.clear();
				glView->GetShapeMask(mask, selectedItems[i]->shapeName);
				project->CopyBoneWeights(selectedItems[i]->shapeName, options.proximityRadius, options.maxResults, &mask, &selectedBones);
			}
			else
				wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
		}

		project->morpher.ClearProximityCache();
		ReselectBone();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}
}

void OutfitStudio::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (project->GetBaseShape().empty())
		return;

	if (project->IsBaseShape(activeItem->shapeName)) {
		wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself."), _("Error"));
		return;
	}

	int baseVertCount = project->GetVertexCount(project->GetBaseShape());
	int workVertCount = project->GetVertexCount(activeItem->shapeName);
	if (baseVertCount != workVertCount) {
		wxMessageBox(_("The vertex count of the reference and chosen shape is not the same!"), _("Error"));
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
	StartProgress(_("Transferring bone weights..."));

	unordered_map<ushort, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->shapeName, &mask, &selectedBones);
	ReselectBone();

	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudio::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
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

void OutfitStudio::OnShapeProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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

		if (!setNames.empty()) {
			setNameChoice->SetSelection(0);
			ssf.SetShapes(setNames.front(), shapes);
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
	ssf.SetShapes(chooser->GetStringSelection().ToStdString(), shapes);
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

	context = new wxGLContext(this, nullptr, &GLSurface::GetGLContextAttribs());
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
	transformMode = false;
	vertexEdit = false;
	segmentMode = false;
	bMaskPaint = false;
	bWeightPaint = false;
	isPainting = false;
	isTransforming = false;
	isSelecting = false;
	bAutoNormals = true;
	bXMirror = true;
	bConnectedEdit = false;
	bGlobalBrushCollision = true;

	lastCenterDistance = 0.0f;

	strokeManager = &baseStrokes;
}

wxGLPanel::~wxGLPanel() {
	delete context;
}

void wxGLPanel::OnShown() {
	if (!context->IsOK()) {
		wxLogError("Outfit Studio: OpenGL context is not OK.");
		wxMessageBox(_("Outfit Studio: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR, os);
	}

	gls.Initialize(this, context);
	auto size = GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight());
	gls.SetMaskVisible();

	int ambient = Config.GetIntValue("Lights/Ambient");
	int frontal = Config.GetIntValue("Lights/Frontal");

	int directional0 = Config.GetIntValue("Lights/Directional0");
	int directional0X = Config.GetIntValue("Lights/Directional0.x");
	int directional0Y = Config.GetIntValue("Lights/Directional0.y");
	int directional0Z = Config.GetIntValue("Lights/Directional0.z");

	int directional1 = Config.GetIntValue("Lights/Directional1");
	int directional1X = Config.GetIntValue("Lights/Directional1.x");
	int directional1Y = Config.GetIntValue("Lights/Directional1.y");
	int directional1Z = Config.GetIntValue("Lights/Directional1.z");

	int directional2 = Config.GetIntValue("Lights/Directional2");
	int directional2X = Config.GetIntValue("Lights/Directional2.x");
	int directional2Y = Config.GetIntValue("Lights/Directional2.y");
	int directional2Z = Config.GetIntValue("Lights/Directional2.z");

	Vector3 directional0Dir = Vector3(directional0X / 100.0f, directional0Y / 100.0f, directional0Z / 100.0f);
	Vector3 directional1Dir = Vector3(directional1X / 100.0f, directional1Y / 100.0f, directional1Z / 100.0f);
	Vector3 directional2Dir = Vector3(directional2X / 100.0f, directional2Y / 100.0f, directional2Z / 100.0f);

	UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directional2Dir);
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

		mesh* m = gls.GetMesh(shapeList[i]);
		if (m) {
			m->BuildTriAdjacency();
			m->BuildEdgeList();
			m->ColorFill(Vector3());
		}

		if (buildNormals)
			RecalcNormals(shapeList[i]);

		if (m)
			m->CreateBuffers();
	}
}

void wxGLPanel::SetMeshTextures(const string& shapeName, const vector<string>& textureFiles, const bool hasMatFile, const MaterialFile& matFile) {
	mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;
	
	string vShader = "res\\shaders\\default.vert";
	string fShader = "res\\shaders\\default.frag";

	TargetGame targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4) {
		vShader = "res\\shaders\\fo4_default.vert";
		fShader = "res\\shaders\\fo4_default.frag";
	}

	GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader);
	if (mat) {
		m->material = mat;
		
		if (hasMatFile)
			m->UpdateFromMaterialFile(matFile);

		gls.UpdateShaders(m);
	}
}

void wxGLPanel::UpdateMeshVertices(const string& shapeName, vector<Vector3>* verts, bool updateBVH, bool recalcNormals, bool render, vector<Vector2>* uvs) {
	int id = gls.GetMeshID(shapeName);
	gls.Update(id, verts, uvs);

	if (updateBVH)
		BVHUpdateQueue.insert(id);

	if (recalcNormals)
		RecalcNormals(shapeName);

	if (render)
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
		int vertIndex;

		if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex))
			return;

		if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
			os->project->GetLiveVerts(os->activeItem->shapeName, verts);
			XRCCTRL(dlg, "posX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].x));
			XRCCTRL(dlg, "posY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].y));
			XRCCTRL(dlg, "posZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].z));

			if (dlg.ShowModal() == wxID_OK) {
				newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
				newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
				newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

				if (os->bEditSlider) {
					diff[vertIndex] = newPos - verts[vertIndex];
					float diffY = diff[vertIndex].y / 10.0f;
					float diffZ = diff[vertIndex].z / 10.0f;
					diff[vertIndex].z = diffY;
					diff[vertIndex].y = diffZ;
					verts[vertIndex] = newPos;
					os->project->UpdateMorphResult(os->activeItem->shapeName, os->activeSlider, diff);
				}
				else {
					os->project->MoveVertex(os->activeItem->shapeName, newPos, vertIndex);
				}
				os->project->GetLiveVerts(os->activeItem->shapeName, verts);
				UpdateMeshVertices(os->activeItem->shapeName, &verts);
			}

			if (transformMode)
				ShowTransformTool();
			if (vertexEdit)
				ShowVertexEdit();
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

	TweakPickInfo tpi;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, nullptr, bGlobalBrushCollision, &tpi.facet);
	if (!hit)
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
		if (wxGetKeyState(WXK_ALT) && !segmentMode) {
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
	else if (wxGetKeyState(WXK_ALT) && !segmentMode) {
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

		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision);
		gls.RenderOneFrame();

		if (activeBrush->Type() == TBT_MOVE) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			if (!hit)
				return;

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
			string selectedBone = os->GetActiveBone();
			if (!selectedBone.empty()) {
				os->ActiveShapesUpdated(strokeManager->GetCurStateStroke(), false, false);

				int boneScalePos = os->boneScale->GetValue();
				if (boneScalePos != 0)
					os->project->ApplyBoneScale(selectedBone, boneScalePos);
			}
		}

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment(nullptr, true);
			os->ShowPartition(nullptr, true);
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (activeStroke) {
		activeStroke->endStroke();

		if (activeStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());

			if (activeStroke->BrushType() == TBT_WEIGHT) {
				string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
				}
			}

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

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment(nullptr, true);
			os->ShowPartition(nullptr, true);
		}
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh, &tpi.facet);
	if (!hit)
		return false;

	tpi.center = xformCenter;

	string mname = hitMesh->shapeName;
	if (mname.find("Move") != string::npos) {
		translateBrush.SetXFormType(0);
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
		if (mname.find("Uniform") == string::npos) {
			translateBrush.SetXFormType(2);
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
		else {
			translateBrush.SetXFormType(3);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), -tpi.center.x);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), -tpi.center.y);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), -tpi.center.z);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
		}
	}

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMeshes(), &translateBrush);

	activeStroke->beginStroke(tpi);
	activeStroke->updateStroke(tpi);

	XMoveMesh->bVisible = false;
	YMoveMesh->bVisible = false;
	ZMoveMesh->bVisible = false;
	XRotateMesh->bVisible = false;
	YRotateMesh->bVisible = false;
	ZRotateMesh->bVisible = false;
	XScaleMesh->bVisible = false;
	YScaleMesh->bVisible = false;
	ZScaleMesh->bVisible = false;
	ScaleUniformMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdateTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	translateBrush.GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, -pd);
	if (tpi.origin.x < 0)
		tpi.origin.x = tpi.origin.x;

	activeStroke->updateStroke(tpi);

	ShowTransformTool(true, true);
}

void wxGLPanel::EndTransform() {
	activeStroke->endStroke();
	activeStroke = nullptr;

	os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());
	if (!os->bEditSlider) {
		vector<string> shapes;
		os->project->GetShapes(shapes);
		for (auto &s : shapes) {
			os->UpdateShapeSource(s);
			os->project->RefreshMorphShape(s);
		}
	}

	ShowTransformTool();
}

bool wxGLPanel::SelectVertex(const wxPoint& screenPos) {
	int vertIndex;
	if (!gls.GetCursorVertex(screenPos.x, screenPos.y, &vertIndex))
		return false;

	if (os->activeItem) {
		mesh* m = GetMesh(os->activeItem->shapeName);
		if (m) {
			if (wxGetKeyState(WXK_CONTROL))
				m->vcolors[vertIndex].x = 1.0f;
			else if (!segmentMode)
				m->vcolors[vertIndex].x = 0.0f;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
	}

	if (transformMode)
		ShowTransformTool();

	return true;
}

bool wxGLPanel::UndoStroke() {
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	bool ret = strokeManager->backStroke(gls.GetActiveMeshes());

	if (ret && curStroke) {
		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke, true);

			if (!os->bEditSlider && curStroke->BrushType() != TBT_WEIGHT) {
				vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}
		}

		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = os->boneScale->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
			}
		}
	}

	if (transformMode)
		ShowTransformTool();
	else
		Render();

	return ret;
}

bool wxGLPanel::RedoStroke() {
	bool ret = strokeManager->forwardStroke(gls.GetActiveMeshes());
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();

	if (ret && curStroke) {
		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke);

			if (!os->bEditSlider && curStroke->BrushType() != TBT_WEIGHT) {
				vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}
		}

		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = os->boneScale->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
			}
		}
	}

	if (transformMode)
		ShowTransformTool();
	else
		Render();

	return ret;
}

void wxGLPanel::ShowTransformTool(bool show, bool keepVisibility) {
	string mode = Config.GetString("Editing/CenterMode");
	if (mode == "Object") {
		if (!gls.GetActiveMeshes().empty())
			xformCenter = gls.GetActiveMeshes().back()->CreateBVH()->Center();
	}
	else if (mode == "Selected")
		xformCenter = gls.GetActiveCenter();
	else
		xformCenter.Zero();

	if (show) {
		bool XMoveVis = true, YMoveVis = true, ZMoveVis = true;
		bool XRotateVis = true, YRotateVis = true, ZRotateVis = true;
		bool XScaleVis = true, YScaleVis = true, ZScaleVis = true;
		bool ScaleUniformVis = true;

		if (keepVisibility && XMoveMesh) {
			XMoveVis = XMoveMesh->bVisible;
			YMoveVis = YMoveMesh->bVisible;
			ZMoveVis = ZMoveMesh->bVisible;
			XRotateVis = XRotateMesh->bVisible;
			YRotateVis = YRotateMesh->bVisible;
			ZRotateVis = ZRotateMesh->bVisible;
			XScaleVis = XScaleMesh->bVisible;
			YScaleVis = YScaleMesh->bVisible;
			ZScaleVis = ZScaleMesh->bVisible;
			ScaleUniformVis = ScaleUniformMesh->bVisible;
		}

		XMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		YMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		ZMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");

		XRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 1.25f, 0.04f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		YRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 1.25f, 0.04f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		ZRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 1.25f, 0.04f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");

		XScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.75f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), 0.12f, Vector3(1.0f, 0.0f, 0.0f), "XScaleMesh");
		YScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.75f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.12f, Vector3(0.0f, 1.0f, 0.0f), "YScaleMesh");
		ZScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.0f, 0.75f), Vector3(0.0f, 0.0f, 1.0f), 0.12f, Vector3(0.0f, 0.0f, 1.0f), "ZScaleMesh");
		ScaleUniformMesh = gls.AddVis3dCube(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.15f, Vector3(0.0f, 0.0f, 0.0f), "ScaleUniformMesh");

		lastCenterDistance = 0.0f;

		if (keepVisibility) {
			XMoveMesh->bVisible = XMoveVis;
			YMoveMesh->bVisible = YMoveVis;
			ZMoveMesh->bVisible = ZMoveVis;
			XRotateMesh->bVisible = XRotateVis;
			YRotateMesh->bVisible = YRotateVis;
			ZRotateMesh->bVisible = ZRotateVis;
			XScaleMesh->bVisible = XScaleVis;
			YScaleMesh->bVisible = YScaleVis;
			ZScaleMesh->bVisible = ZScaleVis;
			ScaleUniformMesh->bVisible = ScaleUniformVis;
		}
	}
	else {
		if (XMoveMesh) {
			XMoveMesh->bVisible = false;
			YMoveMesh->bVisible = false;
			ZMoveMesh->bVisible = false;
			XRotateMesh->bVisible = false;
			YRotateMesh->bVisible = false;
			ZRotateMesh->bVisible = false;
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}
	}

	UpdateTransformTool();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdateTransformTool() {
	if (!transformMode)
		return;

	if (!XMoveMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterDistance != 0.0f) {
		float factor = 1.0f / lastCenterDistance;
		XMoveMesh->ScaleVertices(xformCenter, factor);
		YMoveMesh->ScaleVertices(xformCenter, factor);
		ZMoveMesh->ScaleVertices(xformCenter, factor);

		XRotateMesh->ScaleVertices(xformCenter, factor);
		YRotateMesh->ScaleVertices(xformCenter, factor);
		ZRotateMesh->ScaleVertices(xformCenter, factor);

		XScaleMesh->ScaleVertices(xformCenter, factor);
		YScaleMesh->ScaleVertices(xformCenter, factor);
		ZScaleMesh->ScaleVertices(xformCenter, factor);
		ScaleUniformMesh->ScaleVertices(xformCenter, factor);
	}

	lastCenterDistance = unprojected.DistanceTo(xformCenter) / 15.0f;

	XMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ScaleUniformMesh->ScaleVertices(xformCenter, lastCenterDistance);
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto &m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			mesh* m = GetMesh(os->activeItem->shapeName);
			if (m) {
				m->bShowPoints = true;
				m->QueueUpdate(mesh::UpdateType::VertexColors);
			}
		}
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

		if (editMode) {
			// Adjust brush size
			if (delt < 0)
				DecBrush();
			else
				IncBrush();

			os->CheckBrushBounds();
			os->UpdateBrushPane();
			gls.UpdateCursor(p.x, p.y, bGlobalBrushCollision);
		}
	}
	else {
		int delt = event.GetWheelRotation();
		gls.DollyCamera(delt);
		UpdateTransformTool();
	}

	gls.RenderOneFrame();
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	if (os->IsActive())
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

		UpdateTransformTool();
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

		UpdateTransformTool();
		gls.RenderOneFrame();
	}

	if (lbuttonDown) {
		isLDragging = true;
		if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isSelecting) {
			SelectVertex(event.GetPosition());
		}
		else {
			if (Config.MatchValue("Input/LeftMousePan", "true")) {
				gls.PanCamera(x - lastX, y - lastY);
				UpdateTransformTool();
			}
		}

		gls.RenderOneFrame();
	}

	if (!rbuttonDown && !lbuttonDown) {
		string hitMeshName;
		if (editMode) {
			cursorExists = gls.UpdateCursor(x, y, bGlobalBrushCollision, &hitMeshName, &t, &w, &m);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if (transformMode && !isTransforming) {
			if (XMoveMesh) {
				XMoveMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YMoveMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZMoveMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XRotateMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YRotateMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZRotateMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XScaleMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YScaleMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZScaleMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				ScaleUniformMesh->color = Vector3(0.0f, 0.0f, 0.0f);

				Vector3 outOrigin, outNormal;
				mesh* hitMesh = nullptr;
				if (gls.CollideOverlay(x, y, outOrigin, outNormal, &hitMesh)) {
					if (hitMesh) {
						hitMesh->color = Vector3(1.0f, 1.0f, 0.0f);
						gls.ShowCursor(false);
					}
				}
			}
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
	bool meshHit = false;

	if (transformMode) {
		meshHit = StartTransform(event.GetPosition());
		if (meshHit)
			isTransforming = true;
	}

	if (!meshHit) {
		if (editMode) {
			meshHit = StartBrushStroke(event.GetPosition());
			if (meshHit)
				isPainting = true;
		}
		else if (vertexEdit) {
			meshHit = SelectVertex(event.GetPosition());
			if (meshHit)
				isSelecting = true;
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

	if (!isLDragging && !isPainting && !activeBrush) {
		int x, y;
		event.GetPosition(&x, &y);
		wxPoint p = event.GetPosition();

		int meshID = gls.PickMesh(x, y);
		if (meshID > -1)
			os->SelectShape(gls.GetMeshName(meshID));
	}

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isSelecting)
		isSelecting = false;

	isLDragging = false;
	lbuttonDown = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isSelecting)
		isSelecting = false;

	isLDragging = false;
	lbuttonDown = false;

	isMDragging = false;
	mbuttonDown = false;

	rbuttonDown = false;

	gls.RenderOneFrame();
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
		string mergeShapeName = "";
		if (owner->activeItem && fileNames.GetCount() == 1)
			mergeShapeName = owner->activeItem->shapeName;

		for (auto &inputFile : fileNames) {
			wxString dataName = inputFile.AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			if (inputFile.Lower().EndsWith(".nif")) {
				owner->StartProgress(_("Adding NIF file..."));
				owner->UpdateProgress(1, _("Adding NIF file..."));
				owner->project->AddNif(inputFile.ToStdString(), false);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".obj")) {
				owner->StartProgress("Adding OBJ file...");
				owner->UpdateProgress(1, _("Adding OBJ file..."));
				owner->project->AddShapeFromObjFile(inputFile.ToStdString(), dataName.ToStdString(), mergeShapeName);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".fbx")) {
				owner->StartProgress(_("Adding FBX file..."));
				owner->UpdateProgress(1, _("Adding FBX file..."));
				owner->project->ImportShapeFBX(inputFile.ToStdString(), dataName.ToStdString(), mergeShapeName);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
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
					wxMessageBox(_("There is no shape selected!"), _("Error"));
					return false;
				}

				if (lastResult == wxDragCopy) {
					targetSlider = owner->NewSlider(dataName.ToStdString(), isMultiple);
				}

				if (targetSlider.empty())
					return false;

				owner->StartProgress(_("Loading slider file..."));
				owner->UpdateProgress(1, _("Loading slider file..."));

				if (isBSD)
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->shapeName, inputFile.ToStdString());
				else
					return false;


				owner->UpdateProgress(100, _("Finished"));
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
