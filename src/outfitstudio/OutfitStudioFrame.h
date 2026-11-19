/*
BodySlide and Outfit Studio

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

#pragma once

#include "OutfitStudioProjectHost.h"
#include "ToolID.h"
#include "components/PoseData.h"
#include "components/RefTemplates.h"
#include "physics/PumpClock.h"
#include "project/EditorState.h"
#include "project/ProjectOperations.h"
#include "ui/wxSliderPanel.h"
#include "utils/MainThreadDispatcher.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <wx/clrpicker.h>
#include <wx/collpane.h>
#include <wx/dataview.h>
#include <wx/filepicker.h>
#include <wx/grid.h>
#include <wx/html/htmlwin.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>
#include <wx/timer.h>
#include <wx/tokenzr.h>
#include <wx/treectrl.h>
#include <wx/wizard.h>
#include <wx/xrc/xmlres.h>
#ifdef _WINDOWS
#include <wx/msw/registry.h>
#endif

#define ANIM_PLAYBACK_TIMER 300
#define PHYSICS_TIMER 301

struct MergeCheckErrors;

// NIF and .osd file that saving writes, and the project that writes them
struct SharedFilesTarget {
	std::string dataDir; // Relative to the "ShapeData" folder
	std::string nifName;
	std::string osdName;
	std::string setFile;
	std::string setName;
	bool addOn = false;
	bool hasBase = false;
};

class Mesh;
class ShapeItemData;
struct UndoStateProject;
class wxGLPanel;
class wxStateButton;
struct ViewState;
struct WeightCopyOptions;
enum class ShapeDisplayMode;

namespace Physics {
class Controller;
struct GrabTarget;
} // namespace Physics

struct ShapeTransformDialogSpec;
struct ConformOptions;
struct ClippingFixOptions;
class OutfitStudioFrame;
class ArchiveBrowser;
class BrushStrokeController;
class EditUV;
class OutfitStudioMcp;
struct SymmetricVertices;
struct VertexAsymmetries;
struct VertexAsymmetryTasks;

struct ProjectHistoryEntry {
	std::string fileName;
	std::string projectName;
};

class OutfitProject;
class wxBrushSettingsPopupTransient;

class ToolBarButtonHider {
public:
	struct ButDat {
		int id = 0;
		std::unique_ptr<wxToolBarToolBase> but;
	};
	std::vector<ButDat> butdats;
	wxToolBar* tb = nullptr;

	void Init(wxToolBar* tbi);
	void Show(int toolId, bool show);
};

class OutfitStudioFrame : public wxFrame {
public:
	OutfitStudioFrame(const wxPoint& pos, const wxSize& size);
	~OutfitStudioFrame();

	// Runs work on this window's thread for callers on other threads, like an external API
	MainThreadDispatcher& GetDispatcher() { return *dispatcher; }

	// Why the editor can't take an operation from outside right now, or empty if it can.
	// Main thread only.
	std::string GetBusyReason();

	// Starts the MCP server that lets AI agents control this window, on the configured port
	// for -1. Shows what went wrong if it can't start, in a message box when showError is set
	// and otherwise only in the status bar and log. Main thread only.
	bool StartMcpServer(int port = -1, bool showError = true);
	void StopMcpServer();

	bool HasPendingChanges() const { return pendingChanges; }

	// Saves the current skeleton pose to the pose list and the PoseData folder, replacing a pose of the
	// same name unless it's read-only. Returns the error, or an empty string.
	std::string SaveCurrentPose(const std::string& poseName);
	// Saves the current skeleton pose to a .hkx, .json, .yaml or .yml file. HKX needs the Havok skeleton
	// next to the reference skeleton. Returns the error, or an empty string.
	std::string SaveCurrentPoseFile(const std::string& fileName);

	void LoadFiles(const wxArrayString& files, const wxString& projectName = "");

	// Import a file after asking for the import options. Returns 1 if the dialog was cancelled.
	int ImportOBJWithDialog(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
#ifdef USE_FBXSDK
	int ImportFBXWithDialog(const std::string& fileName, const std::string& shapeName = "", nifly::NiShape* mergeShape = nullptr);
#endif

	// Loads a mesh out of a BSA/BA2 archive as the outfit, or imports it into
	// the project when "import" is set.
	// Returns whether the mesh was loaded
	bool LoadMeshFromArchive(const std::string& relPath, const std::string& archivePath, bool import);

	const std::vector<RefTemplate>& GetRefTemplates() const { return refTemplates; }

	// True while an HKX animation is being played back; all editing is locked
	// for the duration.
	bool IsAnimationPlaying() const { return animPlaying; }

	// Advances playback to whatever frame the wall clock says is due. Safe to
	// call from any event source: the frame comes from elapsed time rather than
	// a tick count, so the speed stays correct however often it arrives.
	void PumpAnimationPlayback();

	// Feeds a horizontal camera rotation into the physics preview so cloth
	// and hair react as if the character turned under a fixed camera. No-op
	// while physics is not simulating.
	void InjectPhysicsCameraYaw(float deltaDegrees);

	// The per-frame physics tick while physics runs without animation
	// playback; while an animation plays, ApplyPose steps the simulation in
	// lockstep instead. Internally paced, so like PumpAnimationPlayback it is
	// safe to call from any event source at any rate.
	void PumpPhysics();

	// Whether the viewport should treat a left drag as a grab of the running
	// simulation rather than as whatever the active tool does.
	bool IsPhysicsGrabEnabled() const;

	// The shapes the simulation moves; empty while it is not running.
	const std::unordered_set<std::string>& GetPhysicsAffectedShapes() const;

	// Fits the BVHs of those shapes to where the simulation has moved them.
	// The pump leaves them behind on purpose - rebuilding a tree per frame
	// costs far more than it is worth for trees only picking uses - so
	// anything that does pick the simulated mesh has to catch them up first.
	void RefitPhysicsBVH();

	// Grabbing the mesh with the cursor: drags the physics bones the grabbed
	// patch is skinned to, leaving what the mesh does on the way to the
	// constraints, collisions and gravity of its physics XML. Purely a
	// simulation input - none of it reaches the mesh data or the undo history.
	bool BeginPhysicsGrab(const std::vector<Physics::GrabTarget>& targets);
	void UpdatePhysicsGrab(const nifly::Vector3& offset);
	void EndPhysicsGrab();

	wxGLPanel* glView = nullptr;
	EditUV* editUV = nullptr;
	ArchiveBrowser* archiveBrowser = nullptr;
	OutfitStudioProjectHost projectHost{this};
	std::unique_ptr<MainThreadDispatcher> dispatcher;
	std::unique_ptr<OutfitStudioMcp> mcp;
	OutfitProject* project = nullptr;
	EditorState editor;
	ShapeItemData* activeItem = nullptr;
	bool autoFrameSelected = false;
	std::vector<int> triParts;	// the partition index for each triangle, or -1 for none
	std::vector<int> triSParts; // the segment partition index for each triangle, or -1 for none

	std::deque<ProjectHistoryEntry> projectHistory;

	PoseDataCollection poseDataCollection;
	wxSliderPanelPool sliderPool;

	wxTreeCtrl* outfitShapes = nullptr;
	wxTreeCtrl* outfitBones = nullptr;
	wxPanel* colorSettings = nullptr;
	wxTreeCtrl* segmentTree = nullptr;
	wxTreeCtrl* partitionTree = nullptr;
	wxPanel* lightSettings = nullptr;
	wxChoice* hdriBackground = nullptr;
	// Set when the user picks "No background" themselves. A project full of Complex Materials turns
	// an HDRi on by itself, which would otherwise keep overriding somebody who turned it off.
	bool hdriBackgroundCleared = false;
	wxChoice* cXMirrorBone = nullptr;
	wxChoice* cPoseBone = nullptr;
	wxSlider* rxPoseSlider = nullptr;
	wxSlider* ryPoseSlider = nullptr;
	wxSlider* rzPoseSlider = nullptr;
	wxSlider* txPoseSlider = nullptr;
	wxSlider* tyPoseSlider = nullptr;
	wxSlider* tzPoseSlider = nullptr;
	wxSlider* scPoseSlider = nullptr;
	wxTextCtrl* rxPoseText = nullptr;
	wxTextCtrl* ryPoseText = nullptr;
	wxTextCtrl* rzPoseText = nullptr;
	wxTextCtrl* txPoseText = nullptr;
	wxTextCtrl* tyPoseText = nullptr;
	wxTextCtrl* tzPoseText = nullptr;
	wxTextCtrl* scPoseText = nullptr;
	wxCheckBox* cbPose = nullptr;
	wxCheckBox* cbPhysics = nullptr;
	wxCheckBox* cbPhysicsVis = nullptr;
	wxCheckBox* cbPhysicsGrab = nullptr;
	wxCheckBox* cbPhysicsProbe = nullptr;
	wxPanel* physicsProbePanel = nullptr;
	wxSlider* physicsProbeX = nullptr;
	wxSlider* physicsProbeY = nullptr;
	wxSlider* physicsProbeZ = nullptr;
	wxSlider* physicsProbeSize = nullptr;
	wxTextCtrl* physicsProbeXText = nullptr;
	wxTextCtrl* physicsProbeYText = nullptr;
	wxTextCtrl* physicsProbeZText = nullptr;
	wxTextCtrl* physicsProbeSizeText = nullptr;
	wxSlider* physicsWindSlider = nullptr;
	wxChoice* physicsWindDir = nullptr;
	wxButton* poseToMesh = nullptr;
	wxComboBox* cAnimationName = nullptr;
	wxButton* animFavoriteButton = nullptr;
	wxButton* animPlayPauseButton = nullptr;
	wxSlider* animFrameSlider = nullptr;
	wxStaticText* animFrameText = nullptr;
	wxChoice* animSpeedChoice = nullptr;
	wxCheckBox* animInterpolateCheck = nullptr;
	wxScrolledWindow* sliderScroll = nullptr;
	wxMenuBar* menuBar = nullptr;
	wxToolBar* toolBarH = nullptr;
	wxToolBar* toolBarV = nullptr;
	ToolBarButtonHider tbvHider;
	wxStatusBar* statusBar = nullptr;

	wxSearchCtrl* sliderFilter = nullptr;
	wxSearchCtrl* bonesFilter = nullptr;

	wxStateButton* currentTabButton = nullptr;
	wxStateButton* meshTabButton = nullptr;
	wxStateButton* boneTabButton = nullptr;
	wxStateButton* colorsTabButton = nullptr;
	wxStateButton* segmentTabButton = nullptr;
	wxStateButton* partitionTabButton = nullptr;
	wxStateButton* lightsTabButton = nullptr;
	wxButton* brushSettings = nullptr;
	wxCheckBox* cbEdgeSlideCorrectUV = nullptr;
	wxSlider* fovSlider = nullptr;
	wxCheckBox* cbDepthClip = nullptr;
	wxBrushSettingsPopupTransient* brushSettingsPopupTransient = nullptr;
	wxScrolledWindow* toolScroll = nullptr;
	wxCollapsiblePane* masksPane = nullptr;
	wxCollapsiblePane* posePane = nullptr;
	wxCollapsiblePane* physicsPane = nullptr;
	wxCollapsiblePane* animationPane = nullptr;
	wxCollapsiblePane* notesPane = nullptr;
	wxTextCtrl* projectNotes = nullptr;

	wxTreeItemId shapesRoot;
	wxTreeItemId outfitRoot;
	wxTreeItemId bonesRoot;
	wxTreeItemId segmentRoot;
	wxTreeItemId partitionRoot;

	std::map<std::string, wxSliderPanel*> sliderPanels;

	void SetPendingChanges(bool pending = true);
	bool CheckPendingChanges();

	void UpdateUndoTools();

	bool SaveProject();
	bool SaveProjectAs();
	// Files the Save Project As dialog would write, and the project that writes them
	SharedFilesTarget SaveAsSharedFilesTarget(wxDialog& dlg, bool hasBase, bool addOn);
	bool LoadProject(const std::string& fileName,
					 const std::string& projectName = "",
					 bool clearProject = true,
					 bool newDataLocal = false,
					 bool appendNewSliders = true,
					 bool setAsReference = true);
	// Resolves the slider set to load from the given file, prompting the user if there is
	// more than one. If preferredName is given and present in the file, it is used without
	// prompting. Returns an empty string on failure (an error is shown) or if cancelled.
	std::string ChooseSliderSetName(const std::string& fileName, const std::string& preferredName = "");
	// The project file and set the New Project wizard has as the base project of an add-on or variant.
	bool GetNewProjectBase(wxWizard& wiz, std::string& outFile, std::string& outSet);
	// Asks before renaming a slider or shape takes it from the base project. Pass one of the names.
	bool ConfirmRenameFromBase(const std::string& sliderName, const std::string& shapeName);
	void CreateSetSliders();

	void UpdateReferenceTemplates();
	void ResetProject();

	std::string NewSlider(const std::string& suggestedName = "", bool skipPrompt = false);

	void SetSliderValue(const size_t index, int val);
	void SetSliderValue(const std::string& name, int val);
	void ZeroSliders();
	// Shows the slider values of the project on the slider panels
	void SyncSliderPanels();

	// Shows the shape as the sliders and pose have it, along with the pose rotations its model space
	// normals need
	void UpdateLiveMesh(nifly::NiShape* shape, bool updateBVH, bool updateUVs = false);
	void ApplySliders(bool recalcBVH = true);

	void ShowSliderEffect(const std::string& sliderName, bool show = true);
	wxSliderPanel* GetSliderPanel(const std::string& sliderName);

	void SelectShape(const std::string& shapeName);
	// Selects the shapes by name, the first one becomes the active shape.
	// Returns false without changing the selection if a shape doesn't exist.
	bool SelectShapes(const std::vector<std::string>& shapeNames);
	std::vector<std::string> GetSelectedShapeNames();

	bool GetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode& outMode);
	bool SetShapeDisplayMode(const std::string& shapeName, ShapeDisplayMode mode);

	// Point the camera at shapes, at vertices of a shape, or at the area weighted to a bone.
	// Keep the camera rotation and return false if there is nothing to look at.
	bool FrameShapes(const std::vector<std::string>& shapeNames);
	bool FrameVertices(const std::string& shapeName, const std::vector<int>& vertices);
	bool FrameBone(const std::string& boneName, float minWeight = 0.1f);

	// Shows the shapes' own vertex colors again, e.g. after a heatmap
	void FillVertexColors();

	ViewState GetViewState();
	// Also updates the menu and toolbar items of the display toggles
	void SetViewState(const ViewState& state);
	std::vector<std::string> GetShapeList();

	void UpdateShapeSource(nifly::NiShape* shape);
	void UpdateVertexColors();

	void ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo = false);
	// Refreshes the parts of the GUI that an operation reported as changed
	void ApplyRefresh(RefreshFlags flags);
	// Returns the project to a snapshot and rebuilds the GUI for it. Clears the undo history.
	void RestoreEditSnapshot(const EditSnapshot& snapshot);
	void UpdateActiveShape();
	bool ConfirmSliderDataLocalForEdit(nifly::NiShape* shape, const std::string& sliderName);
	bool ConfirmSliderDataLocalForEdit(const std::vector<nifly::NiShape*>& shapes, const std::vector<std::string>& sliderNames);
	void UpdateBoneCounts();
	void HighlightSliderData();
	void HighlightBoneNamesWithWeights();
	void RefreshGUIWeightColors();
	void PoseToGUI();
	void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones);
	std::vector<std::string> GetSelectedBones();
	void CalcAutoXMirrorBone();
	std::string GetXMirrorBone();

	void ShowSegment(const wxTreeItemId& item = nullptr);
	void UpdateSegmentNames();
	void ApplySegmentVisibility(Mesh* m);
	void ResetSegmentVisibility();
	bool GrowShrinkSegmentPartitionSelection(bool grow);
	bool PaintSegmentPartitionTriangles(Mesh* hitMesh, int hitTri, const nifly::Vector3& hitPointModel, float radiusModel);

	void ShowPartition(const wxTreeItemId& item = nullptr);
	void RefreshActivePartitionTree();
	void RefreshActiveSegmentTree();
	void UpdatePartitionNames();
	void ApplyPartitionVisibility(Mesh* m);
	void ResetPartitionVisibility();

	void SetSubMeshesForPartitions(Mesh* m, const std::vector<int>& tp);
	void SetNoSubMeshes(Mesh* m);
	void SetNoSubMeshes();

	void LockShapeSelect();
	void UnlockShapeSelect();
	void UpdateAnimationGUI();
	void UpdateBoneItemState(const wxTreeItemId& item, const std::string& boneName);
	void UpdateBoneTree();
	void RefreshGUIFromProj(bool render = true, bool stashMasks = true);
	void MeshesFromProj(const bool reloadTextures = false);
	void MeshFromProj(nifly::NiShape* shape, const bool reloadTextures = false);

	void UpdateShapeReference(nifly::NiShape* shape, nifly::NiShape* newShape);
	std::vector<ShapeItemData*>& GetSelectedItems();
	void ClearSelected(nifly::NiShape* shape);
	std::string GetActiveBone();
	bool IsBoneSelected(const std::string& bone) const { return editor.lastSelectedBones.count(bone) != 0; }

	// Bone list item of a project bone, or an invalid id
	wxTreeItemId FindBoneItem(const std::string& bone);
	// Selects a project bone picked in the viewport, as clicking it in the bone
	// list would. With toggle, it is added to or removed from the selection.
	// With keepSelection, an already selected bone only becomes the active one.
	// False when the bone is not in the project.
	bool SelectBoneFromViewport(const std::string& bone, bool toggle = false, bool keepSelection = false);
	// Bone context menu for the bone list or, with a window and position, for a
	// bone right-clicked in the viewport. Bones that aren't in the project (only
	// shown to connect the hierarchy) get the add menu, adding children to them.
	void ShowBoneContextMenu(const std::string& bone = std::string(), wxWindow* at = nullptr, const wxPoint& pos = wxDefaultPosition);

	bool CheckEditableState();

	void HideSliderPanel(wxSliderPanel* sliderPanel);
	void EnterSliderEdit(const std::string& sliderName = "");
	void ExitSliderEdit();
	void MenuEnterSliderEdit();
	void MenuExitSliderEdit();
	void ScrollToActiveSlider();

	void SelectTool(ToolID tool);
	void UpdateEdgeSlideToolOptionsUI(ToolID tool);
	void ReEnableToolOptionsUI();
	void ReToggleToolOptionsUI();

	void CloseBrushSettings();
	void PopupBrushSettings(wxWindow* popupAt = nullptr);
	void UpdateBrushSettings();
	void DeleteSliders(bool keepSliders = false, bool keepZaps = false);
	int CopyBoneWeightForShapes(std::vector<nifly::NiShape*> shapes, bool silent = false, const std::vector<std::string>& preselectedBones = {});
	int ConformShapes(std::vector<nifly::NiShape*> shapes, bool silent = false);
	void SetBaseShape();
	void UpdateTitle();
	// "Add-on of <base>" or "Variant of <base>", empty for a project without a base project
	wxString BaseProjectLabel() const;
	void UpdateProjectNode();
	wxString ProjectNodeTooltip() const;

	void CheckBrushBounds();

	wxGauge* progressBar = nullptr;
	std::vector<std::pair<int, int>> progressStack;
	int progressVal = 0;
	// Told the progress (0 to 10000) and message while a progress bar shows, e.g. to report it over MCP
	std::function<void(int value, const wxString& msg)> progressListener;

	void StartProgress(const wxString& msg = "");

	void StartSubProgress(int min, int max);

	void EndProgress(const wxString& msg = "", bool forceEmpty = false);

	void UpdateProgress(int val, const wxString& msg = "");

private:
	bool pendingChanges = false;

	bool selectionLocked = false;
	std::vector<ShapeItemData*> selectedItems;
	bool recursingUI = false;
	// The transform tool was on when hiding bones/nodes on the bones tab turned it off
	bool boneTransformSuspended = false;
	// Parent for bones added from a viewport context menu opened on a bone that
	// isn't in the project, used instead of the active bone
	std::string contextParentBone;
	wxTreeItemId activeSegment;
	wxTreeItemId activePartition;
	std::string lastCheckedSlider;

	std::vector<RefTemplate> refTemplates;

	wxBitmap* bmpEditSlider = nullptr;
	wxBitmap* bmpEditSliderGreen = nullptr;
	wxBitmap* bmpSliderSettings = nullptr;

	void createSliderGUI(const std::string& name, wxScrolledWindow* wnd, wxSizer* rootSz);

	void ScrollWindowIntoView(wxScrolledWindow* scrolled, wxWindow* window);
	void HighlightSlider(const std::string& name);

	void ClearProject();
	void RenameProject(const std::string& projectName);

	void UpdateMeshFromSet(nifly::NiShape* shape);

	bool ShapeSelectionCheck();

	bool HasUnweightedCheck();
	void CalcCopySkinTransOption(WeightCopyOptions& options);
	void CopyWeightsToSelectedShapes(bool selectedBonesOnly);
	void ReselectBone();

	int CopySegPartForShapes(std::vector<nifly::NiShape*> shapes, bool silent = false);

	bool ShowVertexAsym(Mesh* m, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, VertexAsymmetryTasks& tasks, const std::vector<bool>& selVerts, bool trize);

	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	bool CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size);

	void OnMenuItem(wxCommandEvent& event);
	void OnPackProjects(wxCommandEvent& event);
	void OnChooseTargetGame(wxCommandEvent& event);
	void SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame);
	void OnSettings(wxCommandEvent& event);
	void OnMcpServer(wxCommandEvent& event);
	void UpdateMcpMenu();
	void OnAbout(wxCommandEvent& event);
	void OnLinkClicked(wxHtmlLinkEvent& link);

	void OnSashPosChanged(wxSplitterEvent& event);
	void OnMoveWindowStart(wxMoveEvent& event);
	void OnMoveWindowEnd(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void AddProjectHistory(const std::string& fileName, const std::string& projectName);
	void UpdateProjectHistory();

	void OnNewProject(wxCommandEvent& event);
	void OnLoadProject(wxCommandEvent& event);
	void OnAddProject(wxCommandEvent& event);
	void OnLoadReference(wxCommandEvent& event);
	void OnConvertBodyReference(wxCommandEvent& event);
	void OnRunAutomation(wxCommandEvent& event);
	void OnLoadOutfit(wxCommandEvent& event);
	void OnBrowseArchives(wxCommandEvent& event);
	void OnUnloadProject(wxCommandEvent& event);

	void OnLoadOutfitFP_File(wxFileDirPickerEvent& event);
	void OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event);

	void OnSetBaseShape(wxCommandEvent& event);
	void OnMakeConvRef(wxCommandEvent& event);

	void OnImportNIF(wxCommandEvent& event);
	void OnExportNIF(wxCommandEvent& event);
	void OnExportNIFWithRef(wxCommandEvent& event);
	void OnExportShapeNIF(wxCommandEvent& event);

	std::optional<bool> PromptStarfieldGeometryMode();

	void OnImportOBJ(wxCommandEvent& event);
	void OnExportOBJ(wxCommandEvent& event);
	void OnExportShapeOBJ(wxCommandEvent& event);

	void OnImportFBX(wxCommandEvent& event);
	void OnExportFBX(wxCommandEvent& event);
	void OnExportShapeFBX(wxCommandEvent& event);

	void OnImportTRIHead(wxCommandEvent& event);
	void OnExportTRIHead(wxCommandEvent& event);
	void OnExportShapeTRIHead(wxCommandEvent& event);

	void OnImportPhysicsData(wxCommandEvent& event);
	void OnExportPhysicsData(wxCommandEvent& event);

	void OnSSSNameCopy(wxCommandEvent& event);
	void OnSSSGenWeightsTrue(wxCommandEvent& event);
	void OnSSSGenWeightsFalse(wxCommandEvent& event);
	void OnSaveSliderSet(wxCommandEvent& event);
	void OnSaveSliderSetAs(wxCommandEvent& event);

	void OnSlider(wxCommandEvent& event);
	void OnClickSliderButton(wxCommandEvent& event);
	void OnEnterHoverSlider(wxMouseEvent& event);
	void OnLeaveHoverSlider(wxMouseEvent& event);
	void OnReadoutChange(wxCommandEvent& event);
	void OnSliderCheckBox(wxCommandEvent& event);

	void OnTabButtonClick(wxCommandEvent& event);
	// Parts of switching tabs
	bool ResolvePendingTabChanges();
	void ShowTab(wxStateButton* tab, wxWindow* panel);
	void LeaveSegmentTab();
	void LeavePartitionTab();
	void LeaveSegmentPartitionMode();
	void LeaveBoneTab();
	void EnableStandardTools();
	void EnterBoneTab();
	void EnterColorsTab();
	void EnterSegmentTab();
	void EnterPartitionTab();
	void EnterSegmentPartitionMode();
	// Enables or disables tools in the menu and in their tool bar, and checks a tool in both
	void EnableTools(std::initializer_list<const char*> names, bool enable);
	void CheckTool(const char* name);
	void ShowWindowsByName(std::initializer_list<const char*> names, bool show);
	void OnBrushColorChanged(wxColourPickerEvent& event);
	void OnColorClampMaxValueSlider(wxCommandEvent& event);
	void OnColorClampMaxValueChanged(wxCommandEvent& event);
	void OnSwapBrush(wxCommandEvent& event);
	void OnMaskVertexColor(wxCommandEvent& event);
	void OnFixedWeight(wxCommandEvent& event);
	void OnCBNormalizeWeights(wxCommandEvent& event);
	void OnSelectSliders(wxCommandEvent& event);

	void OnSliderFilterChanged(wxCommandEvent&);
	void DoFilterSliders();

	void OnBonesFilterChanged(wxCommandEvent&);

	void ToggleVisibility(wxTreeItemId firstItem = wxTreeItemId());
	void OnShapeVisToggle(wxTreeEvent& event);
	void OnShapeSelect(wxTreeEvent& event);
	void OnShapeActivated(wxTreeEvent& event);
	void OnShapeContext(wxTreeEvent& event);
	void OnShapeDrag(wxTreeEvent& event);
	void OnShapeDrop(wxTreeEvent& event);
	void OnShapeTooltip(wxTreeEvent& event);
	void OnCheckTreeSel(wxTreeEvent& event);

	void OnShapeTreeMotion(wxMouseEvent& event);
	void OnShapeTreeLeave(wxMouseEvent& event);

	void ToggleBoneState(wxTreeItemId firstItem = wxTreeItemId());
	void OnBoneStateToggle(wxTreeEvent& event);
	void OnBoneSelect(wxTreeEvent& event);
	void OnBoneActivated(wxTreeEvent& event);
	void OnBoneContext(wxTreeEvent& event);
	void OnBoneTreeContext(wxCommandEvent& event);

	int CalcMaxSegPartID();
	void OnSegmentSelect(wxTreeEvent& event);
	void OnSegmentVisToggle(wxTreeEvent& event);
	void OnSegmentContext(wxTreeEvent& event);
	void OnSegmentTreeContext(wxCommandEvent& event);
	void OnAddSegment(wxCommandEvent& event);
	void OnAddSubSegment(wxCommandEvent& event);
	void OnDeleteSegment(wxCommandEvent& event);
	void OnDeleteSubSegment(wxCommandEvent& event);

	void UpdateActiveSlotID();
	void OnSegmentSlotChanged(wxCommandEvent& event);

	void OnSegmentTypeChanged(wxCommandEvent& event);
	void OnSegmentApply(wxCommandEvent& event);
	void ApplySegments();
	void OnSegmentReset(wxCommandEvent& event);
	void ResetSegments();
	void OnSegmentEditSSF(wxCommandEvent& event);

	void OnPartitionSelect(wxTreeEvent& event);
	void OnPartitionVisToggle(wxTreeEvent& event);
	void OnPartitionContext(wxTreeEvent& event);
	void OnPartitionTreeContext(wxCommandEvent& event);
	void OnAddPartition(wxCommandEvent& event);
	void OnDeletePartition(wxCommandEvent& event);
	void OnPartitionTypeChanged(wxCommandEvent& event);
	void OnPartitionApply(wxCommandEvent& event);
	void ApplyPartitions();
	void OnPartitionReset(wxCommandEvent& event);
	void ResetPartitions();

	void CreateSegmentTree(nifly::NiShape* shape = nullptr);
	void CreatePartitionTree(nifly::NiShape* shape = nullptr);

	void OnSelectTool(wxCommandEvent& event);

	void OnSetView(wxCommandEvent& event);
	void OnTogglePerspective(wxCommandEvent& event);
	void OnToggleRotationCenter(wxCommandEvent& event);
	void OnFrameSelected(wxCommandEvent& event);
	void FrameSelected();
	void OnShowNodes(wxCommandEvent& event);
	void OnShowBones(wxCommandEvent& event);
	void OnShowFloor(wxCommandEvent& event);
	void OnBrushSettings(wxCommandEvent& event);
	void OnEdgeSlideCorrectUV(wxCommandEvent& event);
	void OnFieldOfViewSlider(wxCommandEvent& event);
	void OnDepthClip(wxCommandEvent& event);
	void OnUpdateLights(wxCommandEvent& event);
	void OnResetLights(wxCommandEvent& event);
	void OnSaveLights(wxCommandEvent& event);
	void OnHDRiBackground(wxCommandEvent& event);

	// Fills the background choice from the .exr files in res/hdri, so a file the user dropped in
	// there is listed next to the ones that ship.
	void PopulateHDRiBackgrounds();
	// Applies an HDRi by file name, empty for none. Leaves the choice showing what is actually
	// loaded, which after a failure is no background rather than what was asked for. remember writes
	// the name to the config, and belongs only to a background the user picked themselves.
	bool SetHDRiBackground(const std::string& fileName, bool remember);
	// Turns the last used background on by itself once a project turns out to have a Complex
	// Material in it - the shading those want is hard to judge with nothing to reflect.
	void ApplyAutoHDRiBackground();

	void OnLoadPreset(wxCommandEvent& event);
	void OnSavePreset(wxCommandEvent& event);
	bool ShowConform(ConformOptions& options, bool silent = false);
	void OnSliderConform(wxCommandEvent& event);
	void OnSliderConformAll(wxCommandEvent& event);
	void OnSliderImportNIF(wxCommandEvent& event);
	void OnSliderImportBSD(wxCommandEvent& event);
	void OnSliderImportOBJ(wxCommandEvent& event);
	void OnSliderImportOSD(wxCommandEvent& event);
	void OnSliderImportTRI(wxCommandEvent& event);
	void OnSliderImportMorphsSF(wxCommandEvent& event);
	void OnSliderImportFBX(wxCommandEvent& event);
	void OnSliderExportNIF(wxCommandEvent& event);
	void OnSliderExportBSD(wxCommandEvent& event);
	void OnSliderExportOBJ(wxCommandEvent& event);
	void OnSliderExportOSD(wxCommandEvent& event);
	void OnSliderExportTRI(wxCommandEvent& event);
	void OnSliderExportMorphsSF(wxCommandEvent& event);
	void OnSliderExportToOBJs(wxCommandEvent& event);

	void OnNewSlider(wxCommandEvent& event);
	void OnNewZapSlider(wxCommandEvent& event);
	void OnNewCombinedSlider(wxCommandEvent& event);
	void OnSliderClone(wxCommandEvent& event);
	void OnSliderNegate(wxCommandEvent& event);
	void OnSliderRigidify(wxCommandEvent& event);
	void OnMaskAffected(wxCommandEvent& event);
	void OnClearSlider(wxCommandEvent& event);
	void OnDeleteSlider(wxCommandEvent& event);

	void ShowSliderProperties(const std::string& sliderName);
	void OnSliderProperties(wxCommandEvent& event);
	void OnSliderDataLocations(wxCommandEvent& event);
	void OnSliderFixClipping(wxCommandEvent& event);

	bool ShowClippingFixStrength(float& outStrength);
	void FixClippingForShape(const std::vector<nifly::Vector3>& bodyVerts,
							const std::vector<nifly::Triangle>& bodyTris,
							nifly::NiShape* shape,
							const std::vector<nifly::Vector3>& outfitVerts,
							const ClippingFixOptions& options,
							UndoStateProject* usp,
							const std::vector<float>& fixWeights = {});

	void OnInvertUV(wxCommandEvent& event);
	void OnMirrorShape(wxCommandEvent& event);

	void OnEnterClose(wxKeyEvent& event);

	void OnMoveShape(wxCommandEvent& event);
	void OnScaleShape(wxCommandEvent& event);
	void OnRotateShape(wxCommandEvent& event);
	void OnInflateShape(wxCommandEvent& event);
	// The Move, Scale, Rotate and Inflate Shape dialogs, with a live preview of the change
	void RunShapeTransformDialog(const ShapeTransformDialogSpec& spec);
	void OnFixClippingShape(wxCommandEvent& event);

	void OnRenameShape(wxCommandEvent& event);
	void OnSetReference(wxCommandEvent& event);
	void OnDeleteVerts(wxCommandEvent& event);
	void OnSeparateVerts(wxCommandEvent& event);
	MergeCheckErrors CheckCopyGeo(wxDialog& dlg);
	void OnCopyGeo(wxCommandEvent& event);
	void OnDupeShape(wxCommandEvent& event);
	void OnDeleteShape(wxCommandEvent& event);
	void OnRefineMesh(wxCommandEvent& event);
	void OnSetBoneSkin(wxCommandEvent& event);
	void OnSetBoneNode(wxCommandEvent& event);
	void OnAddBone(wxCommandEvent& event);
	void OnAddCustomBone(wxCommandEvent& event);
	void OnDeleteBone(wxCommandEvent& event);
	void OnDeleteBoneFromSelected(wxCommandEvent& event);
	void FillParentBoneChoice(wxDialog& dlg, const std::string& selBone = "");
	void GetBoneDlgData(wxDialog& dlg, nifly::MatTransform& xform, std::string& parentBone, int& addCount);
	void OnEditBone(wxCommandEvent& event);
	// Adds a bone that was added to the project to the bone list and the mirror and pose bone choices
	wxTreeItemId AddBoneItemToGUI(const std::string& bone);
	// Names of the bones selected in the bone list, in list order
	std::vector<std::string> GetSelectedBoneNames();
	void DuplicateSelectedBones(bool mirror);
	void OnDuplicateBones(wxCommandEvent& event);
	void OnDuplicateBonesMirrored(wxCommandEvent& event);
	void OnMirrorBonesX(wxCommandEvent& event);
	void OnCopyBoneWeight(wxCommandEvent& event);
	void OnCopySelectedWeight(wxCommandEvent& event);
	void OnTransferSelectedWeight(wxCommandEvent& event);
	void OnMaskWeighted(wxCommandEvent& event);
	void OnCheckBadBones(wxCommandEvent& event);
	void OnMaskBoneWeighted(wxCommandEvent& event);
	void OnCopySegPart(wxCommandEvent& event);
	void OnMaskSymVert(wxCommandEvent& event);
	void OnSymVert(wxCommandEvent& event);
	void OnMaskSymTri(wxCommandEvent& event);
	void OnResetTransforms(wxCommandEvent& event);
	void OnDeleteUnreferencedNodes(wxCommandEvent& event);
	void OnRemoveSkinning(wxCommandEvent& event);
	void OnShapeProperties(wxCommandEvent& event);

	void OnMaskLess(wxCommandEvent& event);
	void OnMaskMore(wxCommandEvent& event);

	void OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event);
	void OnNPWizChangeSetNameChoice(wxCommandEvent& event);

	void OnXMirror(wxCommandEvent& event);

	void OnConnectedOnly(wxCommandEvent& event);

	void OnToolOptionMerge(wxCommandEvent& event);

	void OnToolOptionWeld(wxCommandEvent& event);

	void OnToolOptionRestrictSurface(wxCommandEvent& event);

	void OnToolOptionRestrictPlane(wxCommandEvent& event);

	void OnToolOptionRestrictNormal(wxCommandEvent& event);

	void OnToolOptionEdgeSlide(wxCommandEvent& event);

	void OnUndo(wxCommandEvent& WXUNUSED(event));

	void OnRedo(wxCommandEvent& WXUNUSED(event));

	void OnRecalcNormals(wxCommandEvent& WXUNUSED(event));
	void OnDisableNormalsCalc(wxCommandEvent& WXUNUSED(event));
	void OnSmoothNormalSeams(wxCommandEvent& event);
	void OnSmoothSeamsAngle(wxCommandEvent& event);
	void OnLockNormals(wxCommandEvent& event);

	void OnEditUV(wxCommandEvent& event);

	void OnToggleVisibility(wxCommandEvent& WXUNUSED(event)) { ToggleVisibility(); }

	// The display toggles are in both the view menu and the vertical toolbar, keep the two in step
	void CheckDisplayToggle(int id, bool enabled);

	void OnShowWireframe(wxCommandEvent& event);

	void OnEnableLighting(wxCommandEvent& event);

	void OnEnableTextures(wxCommandEvent& event);

	void OnEnableTints(wxCommandEvent& event);

	void OnEnableVertexColors(wxCommandEvent& e);

	void OnIncBrush(wxCommandEvent& WXUNUSED(event));
	void OnDecBrush(wxCommandEvent& WXUNUSED(event));
	void OnIncStr(wxCommandEvent& WXUNUSED(event));
	void OnDecStr(wxCommandEvent& WXUNUSED(event));

	void OnClearMask(wxCommandEvent& WXUNUSED(event));

	void OnInvertMask(wxCommandEvent& WXUNUSED(event));

	void OnDiscord(wxCommandEvent& WXUNUSED(event));

	void OnGitHub(wxCommandEvent& WXUNUSED(event));

	void OnPayPal(wxCommandEvent& WXUNUSED(event));

	void OnSelectMask(wxCommandEvent& event);
	void OnSaveMask(wxCommandEvent& event);
	void OnDeleteMask(wxCommandEvent& event);
	void OnExportMask(wxCommandEvent& event);
	void OnImportMask(wxCommandEvent& event);
	void OnCopyMask(wxCommandEvent& event);
	void OnPaneCollapse(wxCollapsiblePaneEvent& event);
	void OnBottomPanelResize(wxSizeEvent& event);
	// Re-lays out the bottom panel, capping the scrolled tool area so the
	// slider list below it keeps its room no matter how many panes are open.
	void UpdateToolScrollLayout();
	void ApplyPose();

public:
	// Called after a bone/node transform is applied, to re-enable the
	// transform tool on the bones tab and refresh the bone tree icons so
	// that any newly-introduced bad-bones state is visible.
	void UpdateBoneTransformToolEnabled();
	void RefreshBoneTreeBadBoneIcons();

private:
	AnimBone* GetPoseBonePtr();
	void OnPoseBoneChanged(wxCommandEvent& event);
	void OnPoseValChanged(int cind, float val);
	void OnAnyPoseSlider(wxScrollEvent& e, wxTextCtrl* t, int cind);
	void OnPoseSlider(wxScrollEvent& event);
	void OnAnyPoseTextChanged(wxTextCtrl* t, wxSlider* s, int cind);
	void OnPoseTextChanged(wxCommandEvent& event);
	void OnResetBonePose(wxCommandEvent& event);
	void OnResetAllPose(wxCommandEvent& event);
	void OnPoseToMesh(wxCommandEvent& event);
	void ResetAllPoseBones();
	void ActivatePose(bool checked);
	void OnPoseCheckBox(wxCommandEvent& event);
	void OnSelectPose(wxCommandEvent& event);
	void OnSavePose(wxCommandEvent& event);
	void OnDeletePose(wxCommandEvent& event);
	void OnSaveHkxPose(wxCommandEvent& event);
	void OnLoadHkxPose(wxCommandEvent& event);

	// Updates enabled state of the Save/Delete pose buttons based on
	// whether the currently selected pose is read-only (e.g. SAM YAML).
	void UpdatePoseButtonStates();

	// HKX animation playback. Every frame of a loaded animation is stored as a
	// regular PoseData and applied through the same code path as poses, so a
	// paused animation behaves exactly like an applied pose.
	AnimationData* GetSelectedAnimation();
	double GetAnimPlaybackSpeed();
	bool IsAnimationInterpolated() const;
	// Refresh rate of the display the window is on, capped at 120 and falling
	// back to 60 when the display does not report one.
	int GetAnimTargetFps();
	// Microseconds until the next frame should be drawn, 0 when it is due.
	wxLongLong GetAnimFrameDueInMicro();
	// Applies the pose at the given frame position, blending the two frames
	// around a fractional one when interpolation is on.
	void ApplyAnimationFrame(double framePos, bool updatePoseGUI = true);
	// Restarts the playback clock at the current frame. Needed whenever the
	// position or the speed changes while playing, since the frame is derived
	// from the time elapsed since the clock was last started.
	void RestartAnimationClock();
	void StartAnimationPlayback();
	void PauseAnimationPlayback();
	// Empties the animation list down to the favorites, which are listed but
	// only read once selected
	void ResetAnimationList();
	// Refills the animation drop-down from the animation entries, keeping the
	// selection
	void PopulateAnimationList();
	void UpdateAnimationPlayerUI();
	// Enables/disables everything that could edit meshes, bones or the project
	// while an animation is playing. The player controls stay usable.
	void SetAnimationPlaybackLock(bool locked);
	// Resolves the .hkx file next to the configured reference skeleton, showing
	// an error dialog with the given caption on failure.
	bool GetReferenceSkeletonHkxPath(std::string& outPath, const wxString& caption);
	void OnSelectAnimation(wxCommandEvent& event);
	void OnLoadHkxAnimation(wxCommandEvent& event);
	void OnAnimationFavorite(wxCommandEvent& event);
	void OnAnimPlayPause(wxCommandEvent& event);
	void OnAnimFrameSlider(wxScrollEvent& event);
	void OnAnimSpeedChanged(wxCommandEvent& event);
	void OnAnimInterpolateChanged(wxCommandEvent& event);
	void OnAnimPlaybackTimer(wxTimerEvent& event);
	// Bound only while an animation plays; this is what drives the frame rate,
	// the timer being a fallback.
	void OnAnimIdle(wxIdleEvent& event);
	// Bound while an animation plays; swallows every menu, toolbar and
	// accelerator command so nothing can edit the project.
	void OnBlockedCommandDuringPlayback(wxCommandEvent& event);

	wxTimer animPlaybackTimer;
	bool animPlaying = false;
	// Frame position, fractional while interpolating.
	double animCurrentFrame = 0.0;
	// Runs while playing; together with animClockBaseFrame it tells which frame
	// is due, independent of how often ticks actually arrive.
	wxStopWatch animPlaybackWatch;
	double animClockBaseFrame = 0.0;
	// Frame pacing: the animPlaybackWatch reading at which the last frame was
	// drawn, and the rate being aimed for, resolved once per playback.
	wxLongLong animLastDrawMicro = 0;
	int animTargetFps = 60;
	// Scratch pose for interpolated frames, kept around so that playback does
	// not reallocate the bone list every tick.
	PoseData animBlendPose;

	// Physics preview (HDT-SMP): simulation runs while the physics checkbox
	// is set and either pose mode is active or an animation is playing.
	// Single authority over building/tearing down the simulation; call after
	// anything that changes one of those conditions or the loaded meshes.
	void UpdatePhysicsState();
	// Hands the shapes as the sliders morph them to the running simulation.
	void UpdatePhysicsShapes();
	// Shows the physics controls only while the loaded meshes reference a
	// physics XML, since there is nothing to simulate otherwise. Stops a running
	// simulation when the last of them goes away.
	void UpdatePhysicsControlsVisibility();
	// Hard teardown for project unload/close: stops the pump and drops all
	// simulation state without touching the (possibly dying) project meshes.
	void ShutdownPhysics();
	void StartPhysicsPump();
	void StopPhysicsPump();
	void OnPhysicsCheckBox(wxCommandEvent& event);
	void OnPhysicsVisCheckBox(wxCommandEvent& event);
	void OnPhysicsGrabCheckBox(wxCommandEvent& event);
	// Puts the grab checkbox in the state a (not) running simulation allows
	void UpdatePhysicsGrabControl(bool enabled);
	void OnPhysicsProbeCheckBox(wxCommandEvent& event);
	// Each ball slider writes its value into the field beside it, and each field
	// puts the handle back where the value is; either way the field is what is read.
	void OnPhysicsProbeSlider(wxScrollEvent& event, wxTextCtrl* text);
	void OnPhysicsProbeAxisSlider(wxScrollEvent& event);
	void OnPhysicsProbeText(wxTextCtrl* text, wxSlider* slider);
	void OnPhysicsProbeAxisText(wxCommandEvent& event);
	// The same for the collision ball checkbox
	void UpdatePhysicsProbeControl(bool enabled);
	// Shows or hides the ball, its controls and its collider to match the box
	void UpdatePhysicsProbeState();
	// Pushes the ball's position and size into the simulation and the viewport
	void ApplyPhysicsProbe();
	void OnPhysicsWindSlider(wxScrollEvent& event);
	void OnPhysicsWindDir(wxCommandEvent& event);
	// Pushes the wind controls into the simulation; the controller is built
	// fresh every time physics starts, so it has no idea of either value.
	void ApplyPhysicsWind();
	void OnPhysicsTimer(wxTimerEvent& event);
	void OnPhysicsIdle(wxIdleEvent& event);

	std::unique_ptr<Physics::Controller> physics;
	// Whether any loaded mesh references a physics XML. The physics pane only
	// exists on the bones tab, so showing it takes both this and the tab.
	bool physicsAvailable = false;
	wxTimer physicsTimer;
	// Simulation built and active (either pump mode or animation lockstep).
	bool physicsRunning = false;
	// Own idle+timer pump bound (pose mode without animation playback).
	bool physicsPumpActive = false;
	Physics::PumpClock physicsClock;

	wxDECLARE_EVENT_TABLE();
};

