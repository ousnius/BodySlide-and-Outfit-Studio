/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "ToolID.h"
#include "components/TweakBrush.h"
#include "components/UndoHistory.h"
#include "files/MaterialFile.h"
#include "physics/Controller.h"
#include "render/GLSurface.h"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <wx/glcanvas.h>

class BrushStrokeController;
class OutfitStudioFrame;
class SFLayeredMaterial;

// The 3D viewport of the Outfit Studio window: drawing, the editing tools and their undo

enum OverlayLayer : uint32_t {
	Default = 0,
	NodeSelection = 10,
};

enum class RotationCenterMode { Zero, MeshCenter, Picked };

enum class MoveVertexOperation { None, Move, Weld, Merge };

constexpr float BrushSizeScale = 3.0f;

class wxGLPanel : public wxGLCanvas {
public:
	GLSurface gls;
	RotationCenterMode rotationCenterMode = RotationCenterMode::Zero;

	wxGLPanel(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs);
	~wxGLPanel();

	void SetNotifyWindow(wxWindow* win);

	void AddMeshFromNif(nifly::NifFile* nif, const std::string& shapeName);

	void RenameShape(const std::string& shapeName, const std::string& newShapeName) { gls.RenameMesh(shapeName, newShapeName); }

	void SetMeshTextures(const std::string& shapeName,
						 const std::vector<std::string>& textureFiles,
						 const bool hasMatFile = false,
						 const MaterialFile& matFile = MaterialFile(),
						 const bool reloadTextures = false,
						 const std::shared_ptr<const SFLayeredMaterial>& sfMaterial = nullptr);

	Mesh* GetMesh(const std::string& shapeName) { return gls.GetMesh(shapeName); }

	void UpdateMeshVertices(const std::string& shapeName,
							std::vector<nifly::Vector3>* verts,
							bool updateBVH = true,
							bool recalcNormals = true,
							bool render = true,
							std::vector<nifly::Vector2>* uvs = nullptr);
	void RecalculateMeshBVH(const std::string& shapeName);

	void ShowShape(const std::string& shapeName, bool show = true);
	void SetActiveShapes(const std::vector<std::string>& shapeNames);
	void SetSelectedShape(const std::string& shapeName);

	UndoHistory* GetUndoHistory();
	// A brush stroke or a transform is in progress
	bool IsStrokeActive() const;
	// Renders the view offscreen at the given size and saves it as a PNG
	bool SaveViewImage(const wxString& fileName, int width, int height);
	bool RenderViewImage(int width, int height, wxImage& outImage);

	void SetActiveTool(ToolID brushID);
	ToolID GetActiveTool() { return activeTool; }

	void SetLastTool(ToolID tool);
	ToolID GetLastTool() { return lastTool; }

	TweakBrush* GetActiveBrush() { return activeBrush; }

	std::vector<TweakBrush*> GetBrushList();

	void SetColorBrush(const nifly::Vector3& color) { colorBrush.color = color; }

	bool StartBrushStroke(const wxPoint& screenPos);
	void UpdateBrushStroke(const wxPoint& screenPos);
	void EndBrushStroke();

	bool StartTransform(const wxPoint& screenPos);
	void UpdateTransform(const wxPoint& screenPos);
	void EndTransform();

	bool StartPivotPosition(const wxPoint& screenPos);
	void UpdatePivotPosition(const wxPoint& screenPos);
	void EndPivotPosition();

	bool SelectVertex(const wxPoint& screenPos);

	bool StartPickVertex();
	void UpdatePickVertex(const wxPoint& screenPos);
	void EndPickVertex();
	void ClickCollapseVertex();

	bool StartPickEdge();
	void UpdatePickEdge(const wxPoint& screenPos);
	void EndPickEdge();
	void ClickFlipEdge();
	void ClickSplitEdge();

	// Takes hold of the running physics simulation where the cursor points and
	// drags it around, without touching the mesh data. Start fails when the
	// cursor is not over a shape the simulation moves, leaving the click to the
	// tool that would otherwise have had it.
	bool StartPhysicsGrab(const wxPoint& screenPos);
	void UpdatePhysicsGrab(const wxPoint& screenPos);
	void EndPhysicsGrab();

	// Draws (or removes) the ball the running simulation collides with, at a
	// position and radius given in NIF units. Placing it is the frame's job -
	// this only shows where it ended up.
	void ShowPhysicsProbe(const nifly::Vector3& nifPos, float nifRadius);
	void HidePhysicsProbe();

	bool StartMoveVertex(const wxPoint& screenPos);
	void UpdateMoveVertex(const wxPoint& screenPos);
	void EndMoveVertex();
	void CancelMoveVertex();

	bool StartEdgeSlide(const wxPoint& screenPos);
	void UpdateEdgeSlide(const wxPoint& screenPos);
	void EndEdgeSlide();
	void CancelEdgeSlide();

	bool RestoreMode(UndoStateProject* usp);
	void ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender = true);
	bool UndoStroke();
	bool RedoStroke();

	void ShowRotationCenter(bool show = true);

	void ShowTransformTool(bool show = true);
	void UpdateTransformCenter();
	void UpdateTransformTool();

	void ShowPivot(bool show = true);
	void UpdatePivot();

	bool GetNodesMode() { return nodesMode; }
	void ShowNodes(bool show = true);
	void UpdateNodes();
	void UpdateNodeColors();

	bool GetBonesMode() { return bonesMode; }
	void ShowBones(bool show = true);
	bool IsBonesMode() { return bonesMode; }
	void UpdateBones();

	void ShowFloor(bool show = true);
	void UpdateFloor();

	void ShowVertexEdit(bool show = true);

	bool GetEditMode() { return editMode; }
	void SetEditMode(bool on = true) { editMode = on; }

	bool GetBrushMode() { return brushMode; }
	void SetBrushMode(bool on = true) { brushMode = on; }

	bool GetVertexEdit() { return vertexEdit; }
	void SetVertexEdit(bool on = true);

	bool GetTransformMode() { return transformMode; }
	void SetTransformMode(bool on = true);

	bool GetPivotMode() { return pivotMode; }
	void SetPivotMode(bool on = true);

	bool GetSegmentMode() { return segmentMode; }
	void SetSegmentMode(bool on = true) { segmentMode = on; }

	bool GetToolOptionXMirror();
	void SetToolOptionXMirror(bool on = true);

	bool GetToolOptionConnectedOnly() { return toolOptionConnectedOnly; }
	void SetToolOptionConnectedOnly(bool on = true) { toolOptionConnectedOnly = on; }

	bool GetToolOptionMerge() { return toolOptionMerge; }
	void SetToolOptionMerge(bool on = true) { toolOptionMerge = on; }

	bool GetToolOptionWeld() { return toolOptionWeld; }
	void SetToolOptionWeld(bool on = true) { toolOptionWeld = on; }

	bool GetToolOptionRestrictSurface() { return toolOptionRestrictSurface; }
	void SetToolOptionRestrictSurface(bool on = true) { toolOptionRestrictSurface = on; }

	bool GetToolOptionRestrictPlane() { return toolOptionRestrictPlane; }
	void SetToolOptionRestrictPlane(bool on = true) { toolOptionRestrictPlane = on; }

	bool GetToolOptionRestrictNormal() { return toolOptionRestrictNormal; }
	void SetToolOptionRestrictNormal(bool on = true) { toolOptionRestrictNormal = on; }

	bool GetToolOptionEdgeSlide() { return toolOptionEdgeSlide; }
	void SetToolOptionEdgeSlide(bool on = true) { toolOptionEdgeSlide = on; }

	bool GetToolOptionEdgeSlideUVCorrection() { return toolOptionEdgeSlideUVCorrection; }
	void SetToolOptionEdgeSlideUVCorrection(bool on = true) { toolOptionEdgeSlideUVCorrection = on; }

	void SetShapeGhostMode(const std::string& shapeName, bool on = true);

	void SetNormalSeamSmoothMode(bool enable);

	void SetLockNormalsMode(bool enable);

	void RecalcNormals(const std::string& shapeName, bool force = false);

	float GetBrushSize() { return brushSize / BrushSizeScale; }
	void SetBrushSize(float val);
	void ResetBrushSize();

	float IncBrush(float step = 0.010f);
	float DecBrush(float step = 0.010f);

	void LimitBrushSize();

	float IncStr();
	float DecStr();

	void SetCursorType(GLSurface::CursorType cursorType) { gls.SetCursorType(cursorType); }

	void SetWireframe(bool bEnable = true) { gls.SetWireframe(bEnable); }

	void SetLighting(bool bEnable = true) { gls.SetLighting(bEnable); }

	void SetTextures(bool bEnable = true) { gls.SetTextures(bEnable); }

	void SetTints(bool bEnable = true) { gls.SetTints(bEnable); }

	void SetMaskVisible(bool bVisible = true) { gls.SetMaskVisible(bVisible); }

	void SetWeightVisible(bool bVisible = true) { gls.SetWeightColors(bVisible); }

	void SetColorsVisible(bool bVisible = true) { gls.SetVertexColors(bVisible); }

	void ClearMasks();

	void ClearActiveMask();

	void ClearColors();

	void ClearActiveColors();

	void GetActiveMask(std::unordered_map<uint16_t, float>& mask);

	void GetActiveUnmasked(std::unordered_map<uint16_t, float>& mask);

	void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName);

	void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName);

	void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName);

	std::unordered_map<std::string, std::vector<float>> StashMasks();
	void UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash);

	// Applies the mask changes of each active mesh as one undo step
	void ApplyMaskChanges(const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh);
	void ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh);
	void MaskLess();
	void MaskMore();
	void InvertMask();
	void ClearMask();

	void InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName);

	nifly::Vector3 CreateColorRamp(const float value);

	void DeleteMesh(const std::string& shape);

	void Cleanup();

	void SetView(const char type);

	void SetPerspective(const bool enabled);

	void SetFieldOfView(const int fieldOfView);

	void SetDepthClip(const float zNear, const float zFar);

	void UpdateLights(const int ambient,
					  const int frontal,
					  const int directional0,
					  const int directional1,
					  const int directional2,
					  const nifly::Vector3& directional0Dir = nifly::Vector3(),
					  const nifly::Vector3& directional1Dir = nifly::Vector3(),
					  const nifly::Vector3& directonal2Dir = nifly::Vector3());

	// Loads an HDRi from res/hdri as the environment, empty for none. Also changes what shapes with a
	// dynamic cubemap reflect, but not the meshes or their textures.
	bool SetHDRiBackground(const std::string& fileName);

	bool HasHDRiBackground() const { return gls.HasHDRiBackground(); }

	void Render() { gls.RenderOneFrame(); }

	// Hover highlight: overlays a light green copy of the given shape's mesh.
	// Pass an empty string or call ClearHoverHighlight() to remove it.
	void SetHoverHighlight(const std::string& shapeName);
	void ClearHoverHighlight();

	// The displayed bone whose joint or segment is under the screen position,
	// or empty. Only while bones are shown and no animation plays. Connector
	// bones, drawn only to link the selection's bones to their ancestors, are
	// only picked with includeConnectors.
	std::string PickBone(int x, int y, bool includeConnectors = false);
	void SetHoverBone(const std::string& bone);

private:
	void OnShown();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnMouseWheel(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	void OnMiddleDown(wxMouseEvent& event);
	void OnMiddleUp(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);

	void OnRightDown(wxMouseEvent& event);
	void OnRightUp(wxMouseEvent& event);
	void OnLeaveWindow(wxMouseEvent& event);

	void OnKeys(wxKeyEvent& event);
	void OnIdle(wxIdleEvent& event);

	void OnCaptureLost(wxMouseCaptureLostEvent& event);

	std::unique_ptr<wxGLContext> context;

	std::string hoverHighlightName;
	// Bone under the cursor while bones are shown, highlighted in the viewport
	std::string hoverBone;

	bool rbuttonDown = false;
	bool lbuttonDown = false;
	bool mbuttonDown = false;
	bool isLDragging = false;
	bool isMDragging = false;
	bool isRDragging = false;
	wxPoint rDownPos;
	bool firstPaint = true;

	int lastX = 0;
	int lastY = 0;
	GLSurface::CursorHitResult lastHitResult{};
	std::string mouseDownMeshName;
	int mouseDownPoint;
	int mouseDownMirrorPoint;
	nifly::Edge mouseDownEdge;
	nifly::Vector3 mouseDownViewDir;
	bool mouseHasMovedSinceStart = false;
	wxPoint mouseDownOffset;
	nifly::Vector3 mouseDownPointNormal;
	float snapDistance = 0.0f;
	MoveVertexOperation moveVertexOperation = MoveVertexOperation::None;
	int moveVertexTarget;
	std::string moveVertexWeldTargetMeshName;
	nifly::Vector3 edgeSlideStartPosition;
	nifly::Vector2 edgeSlideStartUV;
	nifly::Edge edgeSlideCurrentEdge;
	int edgeSlideTarget = -1;
	bool edgeSlideHasUV = false;

	// Where a physics grab took hold and the plane it reads the cursor
	// against, both in model space. The simulation pulls the grabbed patch out
	// from under the cursor immediately, so there is nothing left to hit-test
	// against - the move brush freezes a plane for the same reason.
	nifly::Vector3 physicsGrabStart;
	nifly::Vector3 physicsGrabPlaneNormal;
	float physicsGrabPlaneDist = 0.0f;

	// Fills "outTargets" with the physics bones holding the patch of "m"
	// around "meshPos", each weighted by how much of the patch it carries.
	bool CollectPhysicsGrabTargets(Mesh* m, const nifly::Vector3& meshPos, std::vector<Physics::GrabTarget>& outTargets);
	void ShowPhysicsGrabMarker(const nifly::Vector3& modelPos);
	void HidePhysicsGrabMarker();

	std::set<Mesh*> BVHUpdateQueue;

	OutfitStudioFrame* os = nullptr;

	float brushSize = 0.45f;

	bool editMode = false;
	bool brushMode = false;
	bool transformMode = false;
	bool pivotMode = false;
	bool nodesMode = false;
	bool bonesMode = false;
	bool floorMode = false;
	bool vertexEdit = false;
	bool segmentMode = false;

	ToolID activeTool = ToolID::Select;
	ToolID lastTool = ToolID::Select;
	bool isPainting = false;
	bool isTransforming = false;
	bool isMovingPivot = false;
	bool isSelecting = false;
	bool isPickingVertex = false;
	bool isPickingEdge = false;
	bool isPickingBone = false;
	bool isMovingVertex = false;
	bool isSlidingEdge = false;
	bool isPhysicsGrabbing = false;
	bool toolOptionXMirror = true;
	bool toolOptionXMirrorWeight = false;
	bool toolOptionConnectedOnly = false;
	bool toolOptionMerge = false;
	bool toolOptionWeld = false;
	bool toolOptionRestrictSurface = false;
	bool toolOptionRestrictPlane = false;
	bool toolOptionRestrictNormal = false;
	bool toolOptionEdgeSlide = false;
	bool toolOptionEdgeSlideUVCorrection = true;

	TweakBrush* activeBrush = nullptr;
	TweakBrush* savedBrush;

	TB_Inflate inflateBrush;
	TB_Deflate deflateBrush;
	TB_Move moveBrush;
	TB_Smooth smoothBrush;
	TB_Undiff undiffBrush;
	TB_Mask maskBrush;
	TB_Unmask UnMaskBrush;
	TB_SmoothMask smoothMaskBrush;
	TB_Weight weightBrush;
	TB_Unweight unweightBrush;
	TB_SmoothWeight smoothWeightBrush;
	TB_Color colorBrush;
	TB_Uncolor uncolorBrush;
	TB_Alpha alphaBrush;
	TB_Unalpha unalphaBrush;
	TB_XForm translateBrush;

	std::unique_ptr<TweakStroke> activeStroke;
	std::unique_ptr<BrushStrokeController> brushStroke;

	Mesh* RotationCenterMesh = nullptr;
	Mesh* RotationCenterMeshRingX = nullptr;
	Mesh* RotationCenterMeshRingY = nullptr;
	Mesh* RotationCenterMeshRingZ = nullptr;

	Mesh* XMoveMesh = nullptr;
	Mesh* YMoveMesh = nullptr;
	Mesh* ZMoveMesh = nullptr;
	Mesh* XRotateMesh = nullptr;
	Mesh* YRotateMesh = nullptr;
	Mesh* ZRotateMesh = nullptr;
	Mesh* XScaleMesh = nullptr;
	Mesh* YScaleMesh = nullptr;
	Mesh* ZScaleMesh = nullptr;
	Mesh* ScaleUniformMesh = nullptr;
	nifly::Vector3 xformCenter; // Transform center for transform brushes (rotate, specifically cares about this)
	nifly::Vector3 xformCenterInitial;
	// Snapshots of the active bone/node's transforms at the start of a
	// bones/nodes-mode transform stroke.  Used to apply the accumulated drag
	// offset as an absolute change from the initial pose each frame, so that
	// the bone/node does not double-integrate its own movement.
	nifly::MatTransform xformInitialLocalToParent;
	nifly::MatTransform xformInitialParentToGlobal;
	// Active transform kind while editing a bone/node in bones/nodes mode.
	// 0 = translate, 1 = rotate.
	int boneXformType = 0;
	// Initial pick origin (model space) and rotation-plane normal (model
	// space) for a bones/nodes-mode rotation stroke.
	nifly::Vector3 boneXformPickStart;
	nifly::Vector3 boneXformPlaneNormalModel;
	nifly::Vector3 boneXformAxisModel;
	float boneXformPlaneDist = 0.0f;
	// Pose-mode transform: editing the bone's poseTranVec/poseRotVec only.
	// No NIF changes, no undo state.
	bool boneXformPoseMode = false;
	nifly::Vector3 boneXformPoseInitialTran;
	nifly::Vector3 boneXformPoseInitialRot;
	float lastCenterDistance = 0.0f;

	Mesh* XPivotMesh = nullptr;
	Mesh* YPivotMesh = nullptr;
	Mesh* ZPivotMesh = nullptr;
	Mesh* PivotCenterMesh = nullptr;
	nifly::Vector3 pivotPosition;
	float lastCenterPivotDistance = 0.0f;

	std::vector<Mesh*> nodesPoints;
	std::vector<Mesh*> nodesLines;
	std::vector<Mesh*> bonesPoints;
	std::vector<Mesh*> bonesLines;
	// Drawn bones that the selected shapes don't use, shown dimmed only to
	// connect the bones below them to the hierarchy
	std::set<std::string> connectorBones;
	std::vector<Mesh*> floorMeshes;

	wxDECLARE_EVENT_TABLE();
};
