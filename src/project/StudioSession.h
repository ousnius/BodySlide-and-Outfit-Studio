/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class GLSurface;
class MainThreadDispatcher;
class Mesh;
class OutfitProject;
class PoseDataCollection;
class RefTemplate;
class SwitchableHost;
class wxImage;
enum class RefreshFlags : uint32_t;
enum class ShapeDisplayMode;
struct EditSnapshot;
struct EditorState;
struct UndoStateProject;
struct ViewState;

namespace nifly {
class NiShape;
}

// Overlays of the view that aren't shapes
enum class ViewOverlay { Bones, Nodes, Floor };

// What the MCP tools and the automation runner work with: the project, the editing state, the render meshes with
// their masks, undo and the view. The Outfit Studio window implements it (OutfitStudioSession). Everything is main
// thread only.
class IStudioSession {
public:
	virtual ~IStudioSession() = default;

	// The open project, or nullptr. Loading a project replaces it, so it is looked up again after that.
	virtual OutfitProject* GetProject() = 0;
	virtual EditorState& Editor() = 0;
	// Answers the project's prompts, also while they are switched off for an unattended call
	virtual SwitchableHost& Host() = 0;
	virtual PoseDataCollection& Poses() = 0;
	virtual const std::vector<RefTemplate>& GetRefTemplates() = 0;
	virtual void UpdateReferenceTemplates() = 0;

	virtual bool HasPendingChanges() = 0;
	virtual void SetPendingChanges(bool pending = true) = 0;
	virtual void UpdateTitle() = 0;

	// Runs calls of other threads on the main thread
	virtual MainThreadDispatcher& Dispatcher() = 0;
	// Why nothing can run now, like an open dialog or a brush stroke. Empty when something can.
	virtual std::string GetBusyReason() = 0;
	// Whether a progress bar runs
	virtual bool InProgress() = 0;
	virtual void StartProgress(const std::string& msg) = 0;
	// Ends every progress bar that still runs
	virtual void EndAllProgress() = 0;
	// Receives the progress while it is set, from 0 to 10000
	virtual void SetProgressListener(std::function<void(int value, const std::string& msg)> listener) = 0;

	// Project files
	virtual bool LoadProject(const std::string& fileName, const std::string& projectName = "") = 0;
	virtual void ResetProject() = 0;
	virtual bool LoadMeshFromArchive(const std::string& relPath, const std::string& archivePath, bool import) = 0;
	// Imports with the options the user picks, or with options given before
	virtual int ImportOBJWithDialog(const std::string& fileName) = 0;
#ifdef USE_FBXSDK
	virtual int ImportFBXWithDialog(const std::string& fileName) = 0;
#endif
	// The list of archives changed
	virtual void RefreshArchiveBrowser() = 0;
	virtual void RetryMissingTextures() = 0;

	// Shapes
	virtual nifly::NiShape* GetActiveShape() = 0;
	virtual std::vector<std::string> GetSelectedShapeNames() = 0;
	virtual bool SelectShapes(const std::vector<std::string>& shapeNames) = 0;
	virtual void SetBaseShape() = 0;
	// A shape was replaced by another, like when its type changed
	virtual void UpdateShapeReference(nifly::NiShape* shape, nifly::NiShape* newShape) = 0;
	virtual void RefreshGUIFromProj(bool render = true, bool stashMasks = true) = 0;
	virtual void MeshFromProj(nifly::NiShape* shape, bool reloadTextures = false) = 0;
	virtual void MeshesFromProj(bool reloadTextures = false) = 0;
	virtual void RefreshSegmentTree() = 0;
	virtual void RefreshPartitionTree() = 0;

	// Sliders
	virtual void CreateSetSliders() = 0;
	virtual std::string NewSlider(const std::string& suggestedName = "", bool skipPrompt = false) = 0;
	// Values in percent, like the slider controls
	virtual void SetSliderValue(size_t index, int val) = 0;
	virtual void SetSliderValue(const std::string& name, int val) = 0;
	virtual void ApplySliders(bool recalcBVH = true) = 0;
	virtual void EnterSliderEdit(const std::string& sliderName = "") = 0;
	virtual void ExitSliderEdit() = 0;

	// Bones, poses and animation
	virtual void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) = 0;
	virtual void RefreshGUIWeightColors() = 0;
	virtual void UpdateAnimationGUI() = 0;
	virtual void UpdateBones() = 0;
	// Shows the pose of the project in the pose controls
	virtual void PoseToGUI() = 0;
	// Return an error, empty on success
	virtual std::string SaveCurrentPose(const std::string& poseName) = 0;
	virtual std::string SaveCurrentPoseFile(const std::string& fileName) = 0;
	// Bakes the preview pose into the shapes, like the Pose to Mesh button
	virtual void PoseToMesh() = 0;

	// Render meshes and their masks
	virtual Mesh* GetMesh(const std::string& shapeName) = 0;
	virtual void RenameMesh(const std::string& shapeName, const std::string& newShapeName) = 0;
	virtual void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) = 0;
	virtual void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) = 0;
	virtual void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) = 0;
	virtual void InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) = 0;
	virtual std::unordered_map<std::string, std::vector<float>> StashMasks() = 0;
	virtual void UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) = 0;
	// Changes the masks of the meshes as one undo step
	virtual void ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) = 0;
	virtual void FillVertexColors() = 0;

	// Undo and refreshes
	virtual void ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender = true) = 0;
	virtual void ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo = false) = 0;
	virtual bool UndoStroke() = 0;
	virtual bool RedoStroke() = 0;
	virtual void UpdateUndoTools() = 0;
	virtual void RestoreEditSnapshot(const EditSnapshot& snapshot) = 0;
	virtual void ApplyRefresh(RefreshFlags flags) = 0;
	// Segment or partition editing, which has no undo
	virtual bool GetSegmentMode() = 0;

	// The view, nullptr without one
	virtual GLSurface* View() = 0;
	virtual void Render() = 0;
	virtual bool RenderViewImage(int width, int height, wxImage& outImage) = 0;
	virtual ViewState GetViewState() = 0;
	virtual void SetViewState(const ViewState& state) = 0;
	virtual bool GetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode& outMode) = 0;
	virtual bool SetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode mode) = 0;
	virtual bool FrameShapes(const std::vector<std::string>& shapeNames) = 0;
	virtual bool FrameVertices(const std::string& shapeName, const std::vector<int>& vertices) = 0;
	virtual bool FrameBone(const std::string& boneName, float minWeight = 0.1f) = 0;
	virtual bool IsOverlayShown(ViewOverlay overlay) = 0;
	virtual void ShowOverlay(ViewOverlay overlay, bool show) = 0;
};
