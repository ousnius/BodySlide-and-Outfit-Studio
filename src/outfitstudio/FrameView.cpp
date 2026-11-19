/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the view around the viewport such as camera, display options, lights, backgrounds and tabs

#include "OutfitStudioFrame.h"
#include "PosePanel.h"
#include "SegmentPartitionEditor.h"
#include "PhysicsPreview.h"
#include "OutfitStudioApp.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "ViewState.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "mcp/OutfitStudioMcp.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "files/SFMorphFile.h"
#include "files/GameDataStream.h"
#include "utils/PlatformUtil.h"
#include "ui/outfitstudio/AutomationDialog.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>
#include <wx/dir.h>
#include <wx/textctrl.h>

using namespace nifly;

bool OutfitStudioFrame::FrameShapes(const std::vector<std::string>& shapeNames) {
	std::vector<Vector3> points;
	for (auto& shapeName : shapeNames) {
		Mesh* m = glView->GetMesh(shapeName);
		if (!m)
			continue;

		for (int i = 0; i < m->nVerts; i++)
			points.push_back(m->TransformPosMeshToModel(m->verts[i]));
	}

	if (points.empty())
		return false;

	glView->gls.FrameModelPoints(points);
	glView->Render();
	return true;
}

bool OutfitStudioFrame::FrameVertices(const std::string& shapeName, const std::vector<int>& vertices) {
	Mesh* m = glView->GetMesh(shapeName);
	if (!m)
		return false;

	std::vector<Vector3> points;
	for (int v : vertices)
		if (v >= 0 && v < m->nVerts)
			points.push_back(m->TransformPosMeshToModel(m->verts[v]));

	if (points.empty())
		return false;

	glView->gls.FrameModelPoints(points);
	glView->Render();
	return true;
}

bool OutfitStudioFrame::FrameBone(const std::string& boneName, float minWeight) {
	std::vector<Vector3> points;
	for (auto& shapeName : project->GetWorkNif()->GetShapeNames()) {
		Mesh* m = glView->GetMesh(shapeName);
		auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, boneName);
		if (!m || !weights)
			continue;

		for (auto& w : *weights)
			if (w.second >= minWeight && w.first < m->nVerts)
				points.push_back(m->TransformPosMeshToModel(m->verts[w.first]));
	}

	if (points.empty())
		return false;

	glView->gls.FrameModelPoints(points);
	glView->Render();
	return true;
}

ViewState OutfitStudioFrame::GetViewState() {
	ViewState state;
	state.camera = glView->gls.GetCamera();
	state.display = glView->gls.GetDisplaySettings();

	for (auto& shapeName : project->GetWorkNif()->GetShapeNames()) {
		ShapeDisplayMode mode;
		if (GetShapeDisplayMode(shapeName, mode))
			state.shapes[shapeName] = mode;
	}

	return state;
}

void OutfitStudioFrame::SetViewState(const ViewState& state) {
	const bool hadVertexColors = glView->gls.GetDisplaySettings().vertexColors;

	glView->gls.SetCamera(state.camera);
	glView->gls.SetDisplaySettings(state.display);

	// Leaving the colors tab with vertex colors off clears them, so they need filling again
	if (state.display.vertexColors && !hadVertexColors && !colorSettings->IsShown())
		FillVertexColors();

	CheckDisplayToggle(XRCID("btnShowWireframe"), state.display.wireframe);
	CheckDisplayToggle(XRCID("btnEnableLighting"), state.display.lighting);
	CheckDisplayToggle(XRCID("btnEnableTextures"), state.display.textures);
	// The colors tab always shows vertex colors, that isn't the choice to keep for the other tabs
	if (!colorSettings->IsShown())
		CheckDisplayToggle(XRCID("btnEnableVertexColors"), state.display.vertexColors);
	CheckDisplayToggle(XRCID("btnEnableTints"), state.display.tints);

	for (auto& shape : state.shapes)
		SetShapeDisplayMode(shape.first, shape.second);

	glView->Render();
}

void OutfitStudioFrame::OnSetView(wxCommandEvent& event) {
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

void OutfitStudioFrame::OnTogglePerspective(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);

	glView->SetPerspective(enabled);
	OutfitStudioConfig.SetBoolValue("Rendering/PerspectiveView", enabled);
}

void OutfitStudioFrame::OnToggleRotationCenter(wxCommandEvent& WXUNUSED(event)) {
	autoFrameSelected = false;

	if (glView->rotationCenterMode != RotationCenterMode::Zero) {
		glView->rotationCenterMode = RotationCenterMode::Zero;
		glView->gls.camRotOffset.Zero();
	}
	else {
		glView->rotationCenterMode = RotationCenterMode::MeshCenter;
		glView->gls.camRotOffset = glView->gls.GetActiveCenter();
	}

	glView->Render();
}

void OutfitStudioFrame::OnFrameSelected(wxCommandEvent& WXUNUSED(event)) {
	autoFrameSelected = !autoFrameSelected;
	FrameSelected();
}

void OutfitStudioFrame::FrameSelected() {
	auto& gls = glView->gls;
	const auto& meshes = gls.GetActiveMeshes();
	if (meshes.empty())
		return;

	nifly::Vector3 bbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	nifly::Vector3 bbMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
	int count = 0;

	for (auto& m : meshes) {
		for (int i = 0; i < m->nVerts; i++) {
			if (m->mask && m->mask[i] != 0.0f)
				continue;
			nifly::Vector3 v = m->TransformPosMeshToModel(m->verts[i]);
			bbMin.x = std::min(bbMin.x, v.x);
			bbMin.y = std::min(bbMin.y, v.y);
			bbMin.z = std::min(bbMin.z, v.z);
			bbMax.x = std::max(bbMax.x, v.x);
			bbMax.y = std::max(bbMax.y, v.y);
			bbMax.z = std::max(bbMax.z, v.z);
			count++;
		}
	}

	if (count == 0)
		return;

	nifly::Vector3 center = (bbMin + bbMax) / 2.0f;
	float extentX = bbMax.x - bbMin.x;
	float extentY = bbMax.y - bbMin.y;
	float extentZ = bbMax.z - bbMin.z;
	float radius = std::max({extentX, extentY, extentZ}) / 2.0f;
	if (radius < 0.001f)
		radius = 0.001f;

	gls.camRotOffset = center;
	glView->rotationCenterMode = RotationCenterMode::MeshCenter;

	gls.camPos.x = -center.x;
	gls.camPos.y = -center.y;

	if (gls.perspective) {
		float fovRad = gls.mFov * DEG2RAD;
		float distance = radius / std::tan(fovRad / 2.0f);
		gls.camPos.z = -(distance + radius);
	}
	else {
		gls.camPos.z = -(radius * 2.5f);
	}

	glView->Render();
}

void OutfitStudioFrame::OnShowNodes(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowNodes(enabled);
	UpdateBoneTransformToolEnabled();
}

void OutfitStudioFrame::OnShowBones(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowBones(enabled);
	UpdateBoneTransformToolEnabled();
}

void OutfitStudioFrame::OnShowFloor(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowFloor(enabled);
}

void OutfitStudioFrame::OnFieldOfViewSlider(wxCommandEvent& WXUNUSED(event)) {
	int fieldOfView = fovSlider->GetValue();

	wxStaticText* fovLabel = (wxStaticText*)toolBarH->FindWindowByName("fovLabel");
	fovLabel->SetLabel(wxString::Format(_("Field of View: %d"), fieldOfView));

	glView->SetFieldOfView(fieldOfView);
}

void OutfitStudioFrame::OnDepthClip(wxCommandEvent& WXUNUSED(event)) {
	float zNear = cbDepthClip->IsChecked() ? 0.001f : 0.1f;
	glView->SetDepthClip(zNear, glView->gls.zFar);
}

void OutfitStudioFrame::OnUpdateLights(wxCommandEvent& WXUNUSED(event)) {
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

void OutfitStudioFrame::OnResetLights(wxCommandEvent& WXUNUSED(event)) {
	int ambient = 15;
	int frontal = 100;
	int directional0 = 0;
	int directional1 = 0;
	int directional2 = 0;

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

void OutfitStudioFrame::OnSaveLights(wxCommandEvent& event) {
	// Make sure the current slider values are in the configuration before writing it out
	OnUpdateLights(event);

	int ret = Config.SaveConfig(Config["AppDir"] + "/Config.xml");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);
}

void OutfitStudioFrame::PopulateHDRiBackgrounds() {
	if (!hdriBackground)
		return;

	hdriBackground->Clear();

	// Client data carries the file name; an empty one is what turns the environment off.
	hdriBackground->Append(_("No background"), new wxStringClientData(""));

#ifdef USE_OPENEXR
	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(Config["AppDir"]) + "/res/hdri", &files, "*.exr", wxDIR_FILES);
	files.Sort();

	for (auto& file : files) {
		const wxFileName fileName(file);

		// The name on disk, not a translated one - these are files the user can add to and rename.
		hdriBackground->Append(fileName.GetName(), new wxStringClientData(fileName.GetFullName()));
	}
#else
	// Nothing can be loaded without an EXR decoder, and a choice with one entry reads as broken.
	hdriBackground->Hide();

	if (lightSettings) {
		if (auto label = lightSettings->FindWindowByName("hdriBackgroundLabel"))
			label->Hide();
	}
#endif

	hdriBackground->SetSelection(0);
}

bool OutfitStudioFrame::SetHDRiBackground(const std::string& fileName, const bool remember) {
	if (!glView->SetHDRiBackground(fileName) && !fileName.empty()) {
		// The environment failed to load and was left off, so the choice has to say so too.
		if (hdriBackground)
			hdriBackground->SetSelection(0);

		return false;
	}

	// Only a background the user picked is worth writing down. The automatic one is a convenience
	// for the project at hand, and remembering it would make the setting say the user chose an HDRi
	// they never asked for - and would rewrite the config on every project that has a Complex
	// Material in it.
	if (remember) {
		Config.SetValue("Rendering/HDRIBackground", fileName);

		int ret = Config.SaveConfig(Config["AppDir"] + "/Config.xml");
		if (ret)
			wxLogWarning("Failed to save configuration (%d)!", ret);
	}

	if (!hdriBackground)
		return true;

	for (unsigned int i = 0; i < hdriBackground->GetCount(); i++) {
		auto data = static_cast<wxStringClientData*>(hdriBackground->GetClientObject(i));
		if (data && data->GetData().ToStdString() == fileName) {
			hdriBackground->SetSelection(i);
			break;
		}
	}

	return true;
}

void OutfitStudioFrame::OnHDRiBackground(wxCommandEvent& WXUNUSED(event)) {
	if (!hdriBackground)
		return;

	auto data = static_cast<wxStringClientData*>(hdriBackground->GetClientObject(hdriBackground->GetSelection()));
	const std::string fileName = data ? data->GetData().ToStdString() : std::string();

	// Turning it off by hand is a decision the automatic activation must not talk the user out of.
	hdriBackgroundCleared = fileName.empty();

	// Only a background the user asked for is worth interrupting them over. The automatic one below
	// leaves its reason in the log and moves on.
	if (!SetHDRiBackground(fileName, true))
		wxMessageBox(wxString::Format(_("Failed to load the HDRi '%s'. See the log for details."), fileName), _("Error"), wxICON_ERROR, this);
}

void OutfitStudioFrame::ApplyAutoHDRiBackground() {
#ifndef USE_OPENEXR
	// Nothing to activate, and trying would log a failure for every project that has a Complex
	// Material in it.
	return;
#else
	if (hdriBackgroundCleared || !hdriBackground || glView->HasHDRiBackground())
		return;

	// Complex Material and True PBR shading both live off what they reflect, so a project full of
	// either is nearly impossible to judge against the flat background. A True PBR shape has the
	// stronger claim of the two: its cube map slot is deliberately empty, so without an HDRi there is
	// no image based lighting for it at all. Starfield materials are in the same position, since they
	// are lit the True PBR way and never have a cube map of their own. Anything else is left as it was.
	const auto& meshes = glView->gls.GetMeshes();
	if (std::none_of(meshes.begin(), meshes.end(), [](const Mesh* m) { return m->complexMaterial || m->pbr || (m->sfLayered && m->sfRenderData && m->sfRenderData->HasAnyTexture()); }))
		return;

	// Whatever the user last picked by hand, which is the only thing that ever gets written there.
	std::string fileName = Config["Rendering/HDRIBackground"];
	if (fileName.empty() || !PlatformUtil::FileExists(Config["AppDir"] + "/res/hdri/" + fileName))
		fileName = "sunrise.exr";

	SetHDRiBackground(fileName, false);
#endif
}

// Tool menu items without a tool bar button, and the ones whose button is in the vertical tool bar.
// The buttons of all other tools are in the horizontal tool bar.
static bool IsMenuOnlyTool(const char* name) {
	return strcmp(name, "deleteVerts") == 0 || strcmp(name, "refineMesh") == 0 || strcmp(name, "btnClearMask") == 0 || strcmp(name, "btnInvertMask") == 0;
}

static bool IsVerticalToolBarTool(const char* name) {
	return strcmp(name, "btnTransform") == 0 || strcmp(name, "btnPivot") == 0 || strcmp(name, "btnVertexEdit") == 0;
}

void OutfitStudioFrame::EnableTools(std::initializer_list<const char*> names, bool enable) {
	for (const char* name : names) {
		const int id = XRCID(name);
		menuBar->Enable(id, enable);
		if (IsVerticalToolBarTool(name))
			toolBarV->EnableTool(id, enable);
		else if (!IsMenuOnlyTool(name))
			toolBarH->EnableTool(id, enable);
	}
}

void OutfitStudioFrame::CheckTool(const char* name) {
	menuBar->Check(XRCID(name), true);
	toolBarH->ToggleTool(XRCID(name), true);
}

void OutfitStudioFrame::ShowWindowsByName(std::initializer_list<const char*> names, bool show) {
	for (const char* name : names)
		FindWindowByName(name)->Show(show);
}

bool OutfitStudioFrame::ResolvePendingTabChanges() {
	if (!currentTabButton || !currentTabButton->HasPendingChanges())
		return true;

	wxMessageDialog dlg(this,
						_("Your changes were not applied yet. Do you want to apply or reset them?"),
						_("Pending Changes"),
						wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetYesNoCancelLabels(_("Apply"), _("Reset"), _("Cancel"));

	int res = dlg.ShowModal();
	if (res == wxID_YES) {
		if (currentTabButton == partitionTabButton)
			segPartEditor->ApplyPartitions();
		else if (currentTabButton == segmentTabButton)
			segPartEditor->ApplySegments();
	}
	else if (res == wxID_NO) {
		if (currentTabButton == partitionTabButton)
			segPartEditor->ResetPartitions();
		else if (currentTabButton == segmentTabButton)
			segPartEditor->ResetSegments();
	}
	else
		return false;

	return true;
}

static const std::initializer_list<const char*> SegmentControls = {"segmentTypeLabel", "segmentType", "segmentSlotLabel", "segmentSlot", "segmentSSFLabel",
																   "segmentSSFEdit", "segmentSSF", "segmentApply", "segmentReset"};
static const std::initializer_list<const char*> PartitionControls = {"partitionTypeLabel", "partitionType", "partitionApply", "partitionReset"};
static const std::initializer_list<const char*> BoneControls = {"totalBoneCountLabel", "selectedBoneCountLabel", "cbFixedWeight", "cbNormalizeWeights", "xMirrorBoneLabel"};

// What leaving the segment or partition tab turns back on
void OutfitStudioFrame::LeaveSegmentPartitionMode() {
	glView->SetSegmentMode(false);
	glView->SetMaskVisible();

	EnableTools({"btnSelect", "btnClearMask", "btnInvertMask", "deleteVerts", "refineMesh"}, true);
}

void OutfitStudioFrame::LeaveSegmentTab() {
	ShowWindowsByName(SegmentControls, false);

	if (currentTabButton == segmentTabButton)
		segPartEditor->ResetSegmentVisibility();

	LeaveSegmentPartitionMode();
}

void OutfitStudioFrame::LeavePartitionTab() {
	ShowWindowsByName(PartitionControls, false);

	if (currentTabButton == partitionTabButton)
		segPartEditor->ResetPartitionVisibility();

	LeaveSegmentPartitionMode();
}

void OutfitStudioFrame::LeaveBoneTab() {
	boneTransformSuspended = false;
	cXMirrorBone->Show(false);

	ShowWindowsByName(BoneControls, false);
	posePane->Show(false);
	if (physicsPane)
		physicsPane->Show(false);
	if (animationPane)
		animationPane->Show(false);
	bonesFilter->GetParent()->Show(false);

	if (project->bPose) {
		project->bPose = false;
		physicsPreview->UpdateState();
		ApplyPose();
	}

	glView->SetWeightVisible(false);
}

// The tools of the tabs other than the bone and colors tabs
void OutfitStudioFrame::EnableStandardTools() {
	CheckTool("btnInflateBrush");
	EnableTools({"btnTransform",
				 "btnPivot",
				 "btnVertexEdit",
				 "btnInflateBrush",
				 "btnDeflateBrush",
				 "btnMoveBrush",
				 "btnSmoothBrush",
				 "btnUndiffBrush",
				 "btnCollapseVertex",
				 "btnFlipEdgeTool",
				 "btnSplitEdgeTool",
				 "btnMoveVertexTool",
				 "deleteVerts",
				 "refineMesh"},
				true);
	EnableTools({"btnWeightBrush", "btnColorBrush", "btnAlphaBrush"}, false);
}

void OutfitStudioFrame::ShowTab(wxStateButton* tab, wxWindow* panel) {
	currentTabButton = tab;

	for (wxWindow* other : std::initializer_list<wxWindow*>{outfitShapes, outfitBones, colorSettings, segPartEditor->GetSegmentTree(), segPartEditor->GetPartitionTree(), lightSettings})
		if (other != panel)
			other->Hide();
	panel->Show();

	for (wxStateButton* other : {meshTabButton, boneTabButton, colorsTabButton, segmentTabButton, partitionTabButton, lightsTabButton})
		if (other != tab)
			other->SetCheck(false);
}

void OutfitStudioFrame::EnterBoneTab() {
	ShowTab(boneTabButton, outfitBones);

	cXMirrorBone->Show();

	ShowWindowsByName(BoneControls, true);
	posePane->Show();
	if (physicsPane)
		physicsPane->Show(physicsPreview->IsAvailable());
	if (animationPane)
		animationPane->Show();
	bonesFilter->GetParent()->Show();

	UpdateBoneTransformToolEnabled();

	SelectTool(ToolID::WeightBrush);
	glView->SetWeightVisible();

	project->bPose = posePanel->IsShowPoseChecked();

	if (project->bPose)
		ApplyPose();

	CheckTool("btnWeightBrush");
	EnableTools({"btnWeightBrush", "btnInflateBrush", "btnDeflateBrush", "btnMoveBrush", "btnSmoothBrush", "btnUndiffBrush", "btnMoveVertexTool"}, true);
	EnableTools({"btnColorBrush", "btnAlphaBrush", "btnPivot", "btnVertexEdit", "btnCollapseVertex", "btnFlipEdgeTool", "btnSplitEdgeTool", "deleteVerts", "refineMesh"},
				false);

	segPartEditor->SetNoSubMeshes();

	ReselectBone();
	UpdateUndoTools();
}

void OutfitStudioFrame::EnterColorsTab() {
	ShowTab(colorsTabButton, colorSettings);

	glView->SetTransformMode(false);
	SelectTool(ToolID::ColorBrush);
	glView->SetColorsVisible();

	wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
	btnSwapBrush->SetLabel(_("Edit Alpha"));

	FillVertexColors();

	CheckTool("btnColorBrush");
	EnableTools({"btnColorBrush", "btnAlphaBrush"}, true);
	EnableTools({"btnWeightBrush",
				 "btnTransform",
				 "btnPivot",
				 "btnVertexEdit",
				 "btnInflateBrush",
				 "btnDeflateBrush",
				 "btnMoveBrush",
				 "btnSmoothBrush",
				 "btnUndiffBrush",
				 "btnCollapseVertex",
				 "btnFlipEdgeTool",
				 "btnSplitEdgeTool",
				 "btnMoveVertexTool",
				 "deleteVerts",
				 "refineMesh"},
				false);

	segPartEditor->SetNoSubMeshes();
}

// Segments and partitions are painted with the mask brush (see PaintSegmentPartitionTriangles), every other tool is off
void OutfitStudioFrame::EnterSegmentPartitionMode() {
	glView->SetSegmentMode();
	SelectTool(ToolID::MaskBrush);
	glView->SetMaskVisible(false);

	CheckTool("btnMaskBrush");
	EnableTools({"btnSelect",
				 "btnTransform",
				 "btnPivot",
				 "btnVertexEdit",
				 "btnInflateBrush",
				 "btnDeflateBrush",
				 "btnMoveBrush",
				 "btnSmoothBrush",
				 "btnUndiffBrush",
				 "btnClearMask",
				 "btnInvertMask",
				 "btnCollapseVertex",
				 "btnFlipEdgeTool",
				 "btnSplitEdgeTool",
				 "btnMoveVertexTool",
				 "deleteVerts",
				 "refineMesh"},
				false);
}

void OutfitStudioFrame::EnterSegmentTab() {
	ShowTab(segmentTabButton, segPartEditor->GetSegmentTree());
	ShowWindowsByName(SegmentControls, true);
	EnterSegmentPartitionMode();

	segPartEditor->ResetSegmentVisibility();
	segPartEditor->ShowSegment(segPartEditor->GetSegmentTree()->GetSelection());
}

void OutfitStudioFrame::EnterPartitionTab() {
	ShowTab(partitionTabButton, segPartEditor->GetPartitionTree());
	ShowWindowsByName(PartitionControls, true);
	EnterSegmentPartitionMode();

	segPartEditor->ResetPartitionVisibility();
	segPartEditor->ShowPartition(segPartEditor->GetPartitionTree()->GetSelection());
}

void OutfitStudioFrame::OnTabButtonClick(wxCommandEvent& event) {
	int id = event.GetId();

	if (!ResolvePendingTabChanges()) {
		event.Skip();
		return;
	}

	if ((id == segmentTabButton->GetId() || id == partitionTabButton->GetId()) && selectedItems.size() != 1) {
		wxMessageBox(_("You must have exactly one mesh selected in order to edit partitions or segments."), _("Info"), wxICON_INFORMATION, this);
		event.Skip();
		return;
	}

	if (id != meshTabButton->GetId())
		masksPane->Hide();

	if (id != segmentTabButton->GetId())
		LeaveSegmentTab();

	if (id != partitionTabButton->GetId())
		LeavePartitionTab();

	if (id != boneTabButton->GetId())
		LeaveBoneTab();

	if (id != colorsTabButton->GetId())
		UpdateVertexColors();

	if (id != boneTabButton->GetId() && id != colorsTabButton->GetId())
		EnableStandardTools();

	if (id == meshTabButton->GetId()) {
		ShowTab(meshTabButton, outfitShapes);
		masksPane->Show();

		segPartEditor->SetNoSubMeshes();
		SelectTool(glView->GetLastTool());
	}
	else if (id == boneTabButton->GetId())
		EnterBoneTab();
	else if (id == colorsTabButton->GetId())
		EnterColorsTab();
	else if (id == segmentTabButton->GetId())
		EnterSegmentTab();
	else if (id == partitionTabButton->GetId())
		EnterPartitionTab();
	else if (id == lightsTabButton->GetId()) {
		ShowTab(lightsTabButton, lightSettings);

		segPartEditor->SetNoSubMeshes();
		SelectTool(glView->GetLastTool());
	}

	CheckBrushBounds();
	UpdateBrushSettings();

	wxPanel* topSplitPanel = (wxPanel*)FindWindowByName("topSplitPanel");
	topSplitPanel->Layout();

	UpdateToolScrollLayout();

	Refresh();
}

void OutfitStudioFrame::CheckDisplayToggle(int id, bool enabled) {
	menuBar->Check(id, enabled);
	toolBarV->ToggleTool(id, enabled);
}

void OutfitStudioFrame::OnShowWireframe(wxCommandEvent& event) {
	CheckDisplayToggle(event.GetId(), event.IsChecked());
	glView->SetWireframe(event.IsChecked());
	glView->Render();
}

void OutfitStudioFrame::OnEnableLighting(wxCommandEvent& event) {
	CheckDisplayToggle(event.GetId(), event.IsChecked());
	glView->SetLighting(event.IsChecked());
	glView->Render();
}

void OutfitStudioFrame::OnEnableTextures(wxCommandEvent& event) {
	CheckDisplayToggle(event.GetId(), event.IsChecked());
	glView->SetTextures(event.IsChecked());
	glView->Render();
}

void OutfitStudioFrame::OnEnableTints(wxCommandEvent& event) {
	CheckDisplayToggle(event.GetId(), event.IsChecked());
	glView->SetTints(event.IsChecked());
	glView->Render();
}

void OutfitStudioFrame::OnEnableVertexColors(wxCommandEvent& e) {
	// The colors tab always shows them, the choice applies when leaving it
	CheckDisplayToggle(e.GetId(), e.IsChecked());
	if (colorSettings->IsShown())
		return;

	glView->SetColorsVisible(e.IsChecked());
	if (e.IsChecked())
		FillVertexColors();
	else
		glView->ClearColors();

	glView->Render();
}

void OutfitStudioFrame::SetAnimationPlaybackLock(bool locked) {
	const bool enable = !locked;

	if (menuBar) {
		for (size_t i = 0; i < menuBar->GetMenuCount(); ++i)
			menuBar->EnableTop(i, enable);
	}

	// Disabling a container disables its children without discarding their own
	// enabled state, so whatever the current tab, tool and pose selection had
	// decided (per-tool toolbar entries, the save/delete pose buttons, Pose to
	// Mesh) comes back unchanged on unlock.
	const std::initializer_list<wxWindow*> lockedWindows = {toolBarH,
															toolBarV,
															meshTabButton,
															boneTabButton,
															colorsTabButton,
															segmentTabButton,
															partitionTabButton,
															lightsTabButton,
															outfitShapes,
															outfitBones,
															sliderScroll,
															sliderFilter,
															bonesFilter,
															masksPane,
															notesPane,
															posePane,
															physicsPane,
															FindWindow(XRCID("cbFixedWeight")),
															FindWindow(XRCID("cbNormalizeWeights")),
															// The player itself stays usable; only the
															// controls that would swap the animation out
															// from under it are locked.
															FindWindow(XRCID("cAnimationName")),
															FindWindow(XRCID("importAnimationFile"))};

	for (wxWindow* w : lockedWindows) {
		if (w)
			w->Enable(enable);
	}

	// Swallow every menu, toolbar and accelerator command while playing.
	// Dynamically bound handlers run before the static event table, so this
	// blocks all of them in one place.
	if (locked)
		Bind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
	else
		Unbind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
}

void OutfitStudioFrame::OnBlockedCommandDuringPlayback(wxCommandEvent& WXUNUSED(event)) {
	// Intentionally empty: not calling Skip() drops the command.
}
