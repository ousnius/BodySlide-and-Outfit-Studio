/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioSession.h"
#include "GLPanel.h"
#include "OutfitStudioFrame.h"
#include "PosePanel.h"
#include "PhysicsXmlEditor.h"
#include "SegmentPartitionEditor.h"
#include "ShapeTreeItems.h"
#include "render/ViewState.h"
#include "ui/outfitstudio/ArchiveBrowser.h"

using namespace nifly;

namespace {

// Menu items of the overlays, whose handlers keep the menu, the toolbar and the view in step
const char* OverlayMenuItem(ViewOverlay overlay) {
	switch (overlay) {
		case ViewOverlay::Bones: return "btnShowBones";
		case ViewOverlay::Nodes: return "btnShowNodes";
		default: return "btnShowFloor";
	}
}

} // namespace

OutfitProject* OutfitStudioSession::GetProject() {
	return frame.project;
}

EditorState& OutfitStudioSession::Editor() {
	return frame.editor;
}

SwitchableHost& OutfitStudioSession::Host() {
	return frame.projectHost;
}

PoseDataCollection& OutfitStudioSession::Poses() {
	return frame.poseDataCollection;
}

AnimSkeleton& OutfitStudioSession::Skeleton() {
	return frame.skeleton;
}

const std::vector<RefTemplate>& OutfitStudioSession::GetRefTemplates() {
	return frame.GetRefTemplates();
}

void OutfitStudioSession::UpdateReferenceTemplates() {
	frame.UpdateReferenceTemplates();
}

bool OutfitStudioSession::HasPendingChanges() {
	return frame.HasPendingChanges();
}

void OutfitStudioSession::SetPendingChanges(bool pending) {
	frame.SetPendingChanges(pending);
}

void OutfitStudioSession::UpdateTitle() {
	frame.UpdateTitle();
}

MainThreadDispatcher& OutfitStudioSession::Dispatcher() {
	return frame.GetDispatcher();
}

std::string OutfitStudioSession::GetBusyReason() {
	return frame.GetBusyReason();
}

bool OutfitStudioSession::InProgress() {
	return !frame.progressStack.empty();
}

void OutfitStudioSession::StartProgress(const std::string& msg) {
	frame.StartProgress(wxString::FromUTF8(msg));
}

void OutfitStudioSession::EndAllProgress() {
	frame.EndProgress("", true);
}

void OutfitStudioSession::SetProgressListener(std::function<void(int value, const std::string& msg)> listener) {
	if (!listener) {
		frame.progressListener = nullptr;
		return;
	}

	frame.progressListener = [listener](int value, const wxString& msg) { listener(value, msg.ToUTF8().data()); };
}

bool OutfitStudioSession::LoadProject(const std::string& fileName, const std::string& projectName) {
	return frame.LoadProject(fileName, projectName);
}

void OutfitStudioSession::ResetProject() {
	frame.ResetProject();
}

bool OutfitStudioSession::LoadMeshFromArchive(const std::string& relPath, const std::string& archivePath, bool import) {
	return frame.LoadMeshFromArchive(relPath, archivePath, import);
}

int OutfitStudioSession::ImportOBJWithDialog(const std::string& fileName) {
	return frame.ImportOBJWithDialog(fileName);
}

#ifdef USE_FBXSDK
int OutfitStudioSession::ImportFBXWithDialog(const std::string& fileName) {
	return frame.ImportFBXWithDialog(fileName);
}
#endif

void OutfitStudioSession::RefreshArchiveBrowser() {
	if (frame.archiveBrowser)
		frame.archiveBrowser->Rebuild();
}

void OutfitStudioSession::RetryMissingTextures() {
	frame.glView->gls.GetResourceLoader()->RetryMissingTextures();
}

NiShape* OutfitStudioSession::GetActiveShape() {
	return frame.activeItem ? frame.activeItem->GetShape() : nullptr;
}

std::vector<std::string> OutfitStudioSession::GetSelectedShapeNames() {
	return frame.GetSelectedShapeNames();
}

bool OutfitStudioSession::SelectShapes(const std::vector<std::string>& shapeNames) {
	return frame.SelectShapes(shapeNames);
}

void OutfitStudioSession::SetBaseShape() {
	frame.SetBaseShape();
}

void OutfitStudioSession::UpdateShapeReference(NiShape* shape, NiShape* newShape) {
	frame.UpdateShapeReference(shape, newShape);
}

void OutfitStudioSession::RefreshGUIFromProj(bool render, bool stashMasks) {
	frame.RefreshGUIFromProj(render, stashMasks);
}

void OutfitStudioSession::MeshFromProj(NiShape* shape, bool reloadTextures) {
	frame.MeshFromProj(shape, reloadTextures);
}

void OutfitStudioSession::MeshesFromProj(bool reloadTextures) {
	frame.MeshesFromProj(reloadTextures);
}

void OutfitStudioSession::RefreshSegmentTree() {
	frame.segPartEditor->RefreshActiveSegmentTree();
}

void OutfitStudioSession::RefreshPartitionTree() {
	frame.segPartEditor->RefreshActivePartitionTree();
}

void OutfitStudioSession::CreateSetSliders() {
	frame.CreateSetSliders();
}

std::string OutfitStudioSession::NewSlider(const std::string& suggestedName, bool skipPrompt) {
	return frame.NewSlider(suggestedName, skipPrompt);
}

void OutfitStudioSession::SetSliderValue(size_t index, int val) {
	frame.SetSliderValue(index, val);
}

void OutfitStudioSession::SetSliderValue(const std::string& name, int val) {
	frame.SetSliderValue(name, val);
}

void OutfitStudioSession::ApplySliders(bool recalcBVH) {
	frame.ApplySliders(recalcBVH);
}

void OutfitStudioSession::EnterSliderEdit(const std::string& sliderName) {
	frame.EnterSliderEdit(sliderName);
}

void OutfitStudioSession::ExitSliderEdit() {
	frame.ExitSliderEdit();
}

void OutfitStudioSession::GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) {
	frame.GetNormalizeBones(normBones, notNormBones);
}

void OutfitStudioSession::RefreshGUIWeightColors() {
	frame.RefreshGUIWeightColors();
}

void OutfitStudioSession::UpdateAnimationGUI() {
	frame.UpdateAnimationGUI();
}

void OutfitStudioSession::UpdateBones() {
	frame.glView->UpdateBones();
}

void OutfitStudioSession::PoseToGUI() {
	frame.posePanel->PoseToGUI();
}

std::string OutfitStudioSession::SaveCurrentPose(const std::string& poseName) {
	return frame.posePanel->SaveCurrentPose(poseName);
}

std::string OutfitStudioSession::SaveCurrentPoseFile(const std::string& fileName) {
	return frame.posePanel->SaveCurrentPoseFile(fileName);
}

void OutfitStudioSession::PoseToMesh() {
	wxCommandEvent event(wxEVT_BUTTON, XRCID("poseToMesh"));
	frame.GetEventHandler()->ProcessEvent(event);
}

Mesh* OutfitStudioSession::GetMesh(const std::string& shapeName) {
	return frame.glView->GetMesh(shapeName);
}

void OutfitStudioSession::RenameMesh(const std::string& shapeName, const std::string& newShapeName) {
	frame.glView->RenameShape(shapeName, newShapeName);
}

void OutfitStudioSession::GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	frame.glView->GetShapeMask(mask, shapeName);
}

void OutfitStudioSession::GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	frame.glView->GetShapeUnmasked(mask, shapeName);
}

void OutfitStudioSession::SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	frame.glView->SetShapeMask(mask, shapeName);
}

void OutfitStudioSession::InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	frame.glView->InvertMaskTris(mask, shapeName);
}

std::unordered_map<std::string, std::vector<float>> OutfitStudioSession::StashMasks() {
	return frame.glView->StashMasks();
}

void OutfitStudioSession::UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) {
	frame.glView->UnstashMasks(stash);
}

void OutfitStudioSession::ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) {
	frame.glView->ApplyMaskChanges(meshes, changesForMesh);
}

void OutfitStudioSession::FillVertexColors() {
	frame.FillVertexColors();
}

void OutfitStudioSession::ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender) {
	frame.glView->ApplyUndoState(usp, bUndo, bRender);
}

void OutfitStudioSession::ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo) {
	frame.ActiveShapesUpdated(usp, bIsUndo);
}

bool OutfitStudioSession::UndoStroke() {
	return frame.glView->UndoStroke();
}

bool OutfitStudioSession::RedoStroke() {
	return frame.glView->RedoStroke();
}

void OutfitStudioSession::UpdateUndoTools() {
	frame.UpdateUndoTools();
}

void OutfitStudioSession::RestoreEditSnapshot(const EditSnapshot& snapshot) {
	frame.RestoreEditSnapshot(snapshot);
}

void OutfitStudioSession::ApplyRefresh(RefreshFlags flags) {
	frame.ApplyRefresh(flags);
}

bool OutfitStudioSession::GetSegmentMode() {
	return frame.glView->GetSegmentMode();
}

GLSurface* OutfitStudioSession::View() {
	return frame.glView ? &frame.glView->gls : nullptr;
}

void OutfitStudioSession::Render() {
	frame.glView->Render();
}

bool OutfitStudioSession::RenderViewImage(int width, int height, wxImage& outImage) {
	return frame.glView->RenderViewImage(width, height, outImage);
}

ViewState OutfitStudioSession::GetViewState() {
	return frame.GetViewState();
}

void OutfitStudioSession::SetViewState(const ViewState& state) {
	frame.SetViewState(state);
}

bool OutfitStudioSession::GetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode& outMode) {
	return frame.GetShapeDisplayMode(shapeName, outMode);
}

bool OutfitStudioSession::SetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode mode) {
	return frame.SetShapeDisplayMode(shapeName, mode);
}

bool OutfitStudioSession::FrameShapes(const std::vector<std::string>& shapeNames) {
	return frame.FrameShapes(shapeNames);
}

bool OutfitStudioSession::FrameVertices(const std::string& shapeName, const std::vector<int>& vertices) {
	return frame.FrameVertices(shapeName, vertices);
}

bool OutfitStudioSession::FrameBone(const std::string& boneName, float minWeight) {
	return frame.FrameBone(boneName, minWeight);
}

bool OutfitStudioSession::IsOverlayShown(ViewOverlay overlay) {
	return frame.GetMenuBar()->IsChecked(XRCID(OverlayMenuItem(overlay)));
}

void OutfitStudioSession::ShowOverlay(ViewOverlay overlay, bool show) {
	wxCommandEvent event(wxEVT_MENU, XRCID(OverlayMenuItem(overlay)));
	event.SetInt(show ? 1 : 0);
	frame.GetEventHandler()->ProcessEvent(event);
}

Physics::XmlEditSession* OutfitStudioSession::PhysicsXmlSession() {
	return &frame.physicsXml->Session();
}

void OutfitStudioSession::PhysicsXmlChanged(const std::string& WXUNUSED(xmlPath)) {
	frame.physicsXml->FilesChanged();
}
