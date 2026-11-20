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
#include "OutfitStudioSession.h"
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
struct SliderDataExportFormat;
struct ConformOptions;
struct ClippingFixOptions;
class OutfitStudioFrame;
class AnimationPlayer;
class PhysicsPreview;
class SegmentPartitionEditor;
class PosePanel;
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
	// The skeleton the projects of the window are bound to. Declared first, so that it goes after everything that
	// holds on to its bones.
	AnimSkeleton skeleton;

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

	// The pose pane: posing the bones by hand, and the pose list
	std::unique_ptr<PosePanel> posePanel;

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

	// Plays HKX animations on the skeleton, with the player controls of the pose pane
	std::unique_ptr<AnimationPlayer> animation;

	// The physics preview, with the controls of the physics pane
	std::unique_ptr<PhysicsPreview> physicsPreview;

	wxGLPanel* glView = nullptr;
	EditUV* editUV = nullptr;
	ArchiveBrowser* archiveBrowser = nullptr;
	OutfitStudioProjectHost projectHost{this};
	std::unique_ptr<MainThreadDispatcher> dispatcher;
	// What the MCP server and automation work with
	OutfitStudioSession session{*this};
	std::unique_ptr<OutfitStudioMcp> mcp;
	OutfitProject* project = nullptr;
	EditorState editor;
	ShapeItemData* activeItem = nullptr;
	std::vector<ShapeItemData*> selectedItems;
	bool ShapeSelectionCheck();
	bool autoFrameSelected = false;

	std::deque<ProjectHistoryEntry> projectHistory;

	PoseDataCollection poseDataCollection;
	wxSliderPanelPool sliderPool;

	wxTreeCtrl* outfitShapes = nullptr;
	wxTreeCtrl* outfitBones = nullptr;
	wxPanel* colorSettings = nullptr;
	wxPanel* lightSettings = nullptr;
	wxChoice* hdriBackground = nullptr;
	// Set when the user picks "No background" themselves. A project full of Complex Materials turns
	// an HDRi on by itself, which would otherwise keep overriding somebody who turned it off.
	bool hdriBackgroundCleared = false;
	wxChoice* cXMirrorBone = nullptr;
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
	void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones);
	std::vector<std::string> GetSelectedBones();
	void CalcAutoXMirrorBone();
	std::string GetXMirrorBone();

	// The segments and partitions of the active shape, with the trees and controls of their tabs
	std::unique_ptr<SegmentPartitionEditor> segPartEditor;

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
	bool recursingUI = false;
	// The transform tool was on when hiding bones/nodes on the bones tab turned it off
	bool boneTransformSuspended = false;
	// Parent for bones added from a viewport context menu opened on a bone that
	// isn't in the project, used instead of the active bone
	std::string contextParentBone;
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


	bool HasUnweightedCheck();
	void CalcCopySkinTransOption(WeightCopyOptions& options);
	void CopyWeightsToSelectedShapes(bool selectedBonesOnly);
	void ReselectBone();


	bool ShowVertexAsym(Mesh* m, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, VertexAsymmetryTasks& tasks, const std::vector<bool>& selVerts, bool trize);

	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);


	void OnMenuItem(wxCommandEvent& event);
	void OnPackProjects(wxCommandEvent& event);
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
	std::vector<nifly::NiShape*> GetSelectedShapes();
	// Asks whether to transform the shapes to global coordinates if any has a skin coordinate system of its own.
	// False if the user cancelled.
	bool AskTransformToGlobal(const std::vector<nifly::NiShape*>& shapes, const wxString& question, bool& transToGlobal);
	// Asks for the file to export the selected shapes to. One shape suggests its own name for the file.
	wxString ChooseShapeExportFile(const wxString& severalTitle, const wxString& oneTitle, const wxString& extension, const wxString& wildcard);
	// The selected shapes, or the active shape if only one is selected, for exporting them to the file
	std::vector<nifly::NiShape*> GetShapesToExport(const wxString& fileName, const char* format);

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
	void OnCBAccumulateSmooth(wxCommandEvent& event);
	void OnNormalizeWeights(wxCommandEvent& event);
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
	// Exports the data of the slider in edit mode for the selected shapes: to a chosen file for one shape,
	// or into a chosen folder as <shape>_<slider>.<extension> for several
	void ExportActiveSliderData(const SliderDataExportFormat& format);
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

public:
	// Re-lays out the bottom panel, capping the scrolled tool area so the
	// slider list below it keeps its room no matter how many panes are open.
	void UpdateToolScrollLayout();
	void ApplyPose();
	// Called after a bone/node transform is applied, to re-enable the
	// transform tool on the bones tab and refresh the bone tree icons so
	// that any newly-introduced bad-bones state is visible.
	void UpdateBoneTransformToolEnabled();
	void RefreshBoneTreeBadBoneIcons();
	// Enables/disables everything that could edit meshes, bones or the project
	// while an animation is playing. The player controls stay usable.
	void SetAnimationPlaybackLock(bool locked);

private:

	// Bound while an animation plays; swallows every menu, toolbar and
	// accelerator command so nothing can edit the project.
	void OnBlockedCommandDuringPlayback(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

