/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "project/StudioSession.h"

class OutfitStudioFrame;

// The session of an Outfit Studio window, for its MCP server and automation
class OutfitStudioSession : public IStudioSession {
public:
	explicit OutfitStudioSession(OutfitStudioFrame& inFrame)
		: frame(inFrame) {}

	OutfitProject* GetProject() override;
	EditorState& Editor() override;
	SwitchableHost& Host() override;
	PoseDataCollection& Poses() override;
	AnimSkeleton& Skeleton() override;
	const std::vector<RefTemplate>& GetRefTemplates() override;
	void UpdateReferenceTemplates() override;

	bool HasPendingChanges() override;
	void SetPendingChanges(bool pending = true) override;
	void UpdateTitle() override;

	MainThreadDispatcher& Dispatcher() override;
	std::string GetBusyReason() override;
	bool InProgress() override;
	void StartProgress(const std::string& msg) override;
	void EndAllProgress() override;
	void SetProgressListener(std::function<void(int value, const std::string& msg)> listener) override;

	bool LoadProject(const std::string& fileName, const std::string& projectName = "") override;
	void ResetProject() override;
	bool LoadMeshFromArchive(const std::string& relPath, const std::string& archivePath, bool import) override;
	int ImportOBJWithDialog(const std::string& fileName) override;
#ifdef USE_FBXSDK
	int ImportFBXWithDialog(const std::string& fileName) override;
#endif
	void RefreshArchiveBrowser() override;
	void RetryMissingTextures() override;

	nifly::NiShape* GetActiveShape() override;
	std::vector<std::string> GetSelectedShapeNames() override;
	bool SelectShapes(const std::vector<std::string>& shapeNames) override;
	void SetBaseShape() override;
	void UpdateShapeReference(nifly::NiShape* shape, nifly::NiShape* newShape) override;
	void RefreshGUIFromProj(bool render = true, bool stashMasks = true) override;
	void MeshFromProj(nifly::NiShape* shape, bool reloadTextures = false) override;
	void MeshesFromProj(bool reloadTextures = false) override;
	void RefreshSegmentTree() override;
	void RefreshPartitionTree() override;

	void CreateSetSliders() override;
	std::string NewSlider(const std::string& suggestedName = "", bool skipPrompt = false) override;
	void SetSliderValue(size_t index, int val) override;
	void SetSliderValue(const std::string& name, int val) override;
	void ApplySliders(bool recalcBVH = true) override;
	void EnterSliderEdit(const std::string& sliderName = "") override;
	void ExitSliderEdit() override;

	void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) override;
	void RefreshGUIWeightColors() override;
	void UpdateAnimationGUI() override;
	void UpdateBones() override;
	void PoseToGUI() override;
	std::string SaveCurrentPose(const std::string& poseName) override;
	std::string SaveCurrentPoseFile(const std::string& fileName) override;
	void PoseToMesh() override;

	Mesh* GetMesh(const std::string& shapeName) override;
	void RenameMesh(const std::string& shapeName, const std::string& newShapeName) override;
	void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override;
	void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override;
	void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override;
	void InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override;
	std::unordered_map<std::string, std::vector<float>> StashMasks() override;
	void UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) override;
	void ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) override;
	void FillVertexColors() override;

	void ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender = true) override;
	void ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo = false) override;
	bool UndoStroke() override;
	bool RedoStroke() override;
	void UpdateUndoTools() override;
	void RestoreEditSnapshot(const EditSnapshot& snapshot) override;
	void ApplyRefresh(RefreshFlags flags) override;
	bool GetSegmentMode() override;

	GLSurface* View() override;
	void Render() override;
	bool RenderViewImage(int width, int height, wxImage& outImage) override;
	ViewState GetViewState() override;
	void SetViewState(const ViewState& state) override;
	bool GetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode& outMode) override;
	bool SetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode mode) override;
	bool FrameShapes(const std::vector<std::string>& shapeNames) override;
	bool FrameVertices(const std::string& shapeName, const std::vector<int>& vertices) override;
	bool FrameBone(const std::string& boneName, float minWeight = 0.1f) override;
	bool IsOverlayShown(ViewOverlay overlay) override;
	void ShowOverlay(ViewOverlay overlay, bool show) override;

	Physics::XmlEditSession* PhysicsXmlSession() override;
	void PhysicsXmlChanged(const std::string& xmlPath) override;

private:
	OutfitStudioFrame& frame;
};
