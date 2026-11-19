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

	std::vector<TweakBrush*> GetBrushList() {
		std::vector<TweakBrush*> brushList{&inflateBrush,
										   &deflateBrush,
										   &moveBrush,
										   &smoothBrush,
										   &undiffBrush,
										   &maskBrush,
										   &weightBrush,
										   &colorBrush,
										   &alphaBrush};

		return brushList;
	}

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
	void SetVertexEdit(bool on = true) {
		vertexEdit = on;
		ShowVertexEdit(on);
		Render();
	}

	bool GetTransformMode() { return transformMode; }
	void SetTransformMode(bool on = true) {
		transformMode = on;
		ShowTransformTool(on);
	}

	bool GetPivotMode() { return pivotMode; }
	void SetPivotMode(bool on = true) {
		pivotMode = on;
		ShowPivot(on);

		if (transformMode)
			ShowTransformTool();
	}

	bool GetSegmentMode() { return segmentMode; }
	void SetSegmentMode(bool on = true) { segmentMode = on; }

	bool GetToolOptionXMirror() {
		if (activeTool == ToolID::WeightBrush)
			return toolOptionXMirrorWeight;
		return toolOptionXMirror;
	}
	void SetToolOptionXMirror(bool on = true) {
		if (activeTool == ToolID::WeightBrush)
			toolOptionXMirrorWeight = on;
		else
			toolOptionXMirror = on;
		gls.SetXMirrorCursor(on);
	}

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

	void SetShapeGhostMode(const std::string& shapeName, bool on = true) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		if (on)
			m->rendermode = Mesh::RenderMode::LitWire;
		else
			m->rendermode = Mesh::RenderMode::Normal;
	}

	void SetNormalSeamSmoothMode(bool enable) {
		for (auto& m : gls.GetActiveMeshes()) {
			m->smoothSeamNormals = enable;
			m->SmoothNormals();
		}
	}

	void SetLockNormalsMode(bool enable) {
		for (auto& m : gls.GetActiveMeshes())
			m->lockNormals = enable;
	}

	void RecalcNormals(const std::string& shapeName, bool force = false) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		bool oldLockNormals = m->lockNormals;
		if (m->lockNormals && force)
			m->lockNormals = false;

		m->SmoothNormals();
		m->lockNormals = oldLockNormals;
	}

	float GetBrushSize() { return brushSize / BrushSizeScale; }
	void SetBrushSize(float val) {
		val *= BrushSizeScale;
		brushSize = val;
		gls.SetCursorSize(val);
	}
	void ResetBrushSize() {
		brushSize = 0.45f;
		gls.SetCursorSize(brushSize);
	}

	float IncBrush(float step = 0.010f) {
		gls.SetCursorSize(gls.GetCursorSize() + step);
		brushSize += step;
		LimitBrushSize();
		return brushSize;
	}
	float DecBrush(float step = 0.010f) {
		gls.SetCursorSize(gls.GetCursorSize() - step);
		brushSize -= step;
		LimitBrushSize();
		return brushSize;
	}

	void LimitBrushSize() {
		if (brushSize < 0.000f) {
			gls.SetCursorSize(0.000f);
			brushSize = 0.000f;
		}
		else if (brushSize > BrushSizeScale) {
			gls.SetCursorSize(BrushSizeScale);
			brushSize = BrushSizeScale;
		}
	}

	float IncStr() {
		if (!activeBrush)
			return 0.0f;

		float str = activeBrush->getStrength();
		str += 0.010f;
		activeBrush->setStrength(str);
		return str;
	}
	float DecStr() {
		if (!activeBrush)
			return 0.0f;

		float str = activeBrush->getStrength();
		str -= 0.010f;
		activeBrush->setStrength(str);
		return str;
	}

	void SetCursorType(GLSurface::CursorType cursorType) { gls.SetCursorType(cursorType); }

	void SetWireframe(bool bEnable = true) { gls.SetWireframe(bEnable); }

	void SetLighting(bool bEnable = true) { gls.SetLighting(bEnable); }

	void SetTextures(bool bEnable = true) { gls.SetTextures(bEnable); }

	void SetTints(bool bEnable = true) { gls.SetTints(bEnable); }

	void SetMaskVisible(bool bVisible = true) { gls.SetMaskVisible(bVisible); }

	void SetWeightVisible(bool bVisible = true) { gls.SetWeightColors(bVisible); }

	void SetColorsVisible(bool bVisible = true) { gls.SetVertexColors(bVisible); }

	void ClearMasks() {
		for (auto& m : gls.GetMeshes())
			if (!m->bPrimitive)
				m->MaskFill(0.0f);
	}

	void ClearActiveMask() {
		for (auto& m : gls.GetActiveMeshes())
			m->MaskFill(0.0f);
	}

	void ClearColors() {
		for (auto& m : gls.GetMeshes())
			if (!m->bPrimitive)
				m->ColorFill(nifly::Vector3());
	}

	void ClearActiveColors() {
		for (auto& m : gls.GetActiveMeshes())
			m->ColorFill(nifly::Vector3());
	}

	void GetActiveMask(std::unordered_map<uint16_t, float>& mask) {
		if (gls.GetActiveMeshes().empty())
			return;

		Mesh* m = gls.GetActiveMeshes().back();

		for (int i = 0; i < m->nVerts; i++) {
			if (m->mask[i] != 0.0f)
				mask[i] = m->mask[i];
		}
	}

	void GetActiveUnmasked(std::unordered_map<uint16_t, float>& mask) {
		if (gls.GetActiveMeshes().empty())
			return;

		Mesh* m = gls.GetActiveMeshes().back();

		for (int i = 0; i < m->nVerts; i++)
			if (m->mask[i] == 0.0f)
				mask[i] = m->mask[i];
	}

	void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++) {
			if (m->mask[i] != 0.0f)
				mask[i] = m->mask[i];
		}
	}

	void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++)
			if (m->mask[i] == 0.0f)
				mask[i] = m->mask[i];
	}

	void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++) {
			if (mask.find(i) != mask.end())
				m->mask[i] = mask[i];
			else
				m->mask[i] = 0.0f;
		}

		m->QueueUpdate(Mesh::UpdateType::Mask);
	}

	std::unordered_map<std::string, std::vector<float>> StashMasks();
	void UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash);

	// Applies the mask changes of each active mesh as one undo step
	void ApplyMaskChanges(const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh);
	void ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh);
	void MaskLess();
	void MaskMore();
	void InvertMask();
	void ClearMask();

	void InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		std::unordered_map<uint16_t, float> triMask;
		for (int t = 0; t < m->nTris; t++) {
			auto& tri = m->tris[t];
			if (mask.find(tri.p1) != mask.end() || mask.find(tri.p2) != mask.end() || mask.find(tri.p3) != mask.end()) {
				triMask.emplace(tri.p1, 1.0f);
				triMask.emplace(tri.p2, 1.0f);
				triMask.emplace(tri.p3, 1.0f);
			}
		}

		std::unordered_map<uint16_t, float> invertMask;
		for (int t = 0; t < m->nTris; t++) {
			auto& tri = m->tris[t];
			if (triMask.find(tri.p1) == triMask.end())
				invertMask.emplace(tri.p1, 1.0f);
			if (triMask.find(tri.p2) == triMask.end())
				invertMask.emplace(tri.p2, 1.0f);
			if (triMask.find(tri.p3) == triMask.end())
				invertMask.emplace(tri.p3, 1.0f);
		}

		mask = std::move(invertMask);
	}

	nifly::Vector3 CreateColorRamp(const float value) {
		float r;
		float g;
		float b;

		if (value <= 0.0f) {
			r = g = b = 1.0;
		}
		else if (value <= 0.25) {
			r = 0.0;
			b = 1.0;
			g = value / 0.25;
		}
		else if (value <= 0.5) {
			r = 0.0;
			g = 1.0;
			b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
		}
		else if (value <= 0.75) {
			r = (value - 0.5) / 0.25;
			g = 1.0;
			b = 0.0;
		}
		else {
			r = 1.0;
			g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
			b = 0.0;
		}

		return nifly::Vector3(r, g, b);
	}

	void DeleteMesh(const std::string& shape) {
		Mesh* m = gls.GetMesh(shape);
		if (m) {
			BVHUpdateQueue.erase(m);
			gls.DeleteMesh(m);
		}
	}

	void Cleanup() {
		XMoveMesh = nullptr;
		YMoveMesh = nullptr;
		ZMoveMesh = nullptr;
		XRotateMesh = nullptr;
		YRotateMesh = nullptr;
		ZRotateMesh = nullptr;
		XScaleMesh = nullptr;
		YScaleMesh = nullptr;
		ZScaleMesh = nullptr;
		ScaleUniformMesh = nullptr;

		XPivotMesh = nullptr;
		YPivotMesh = nullptr;
		ZPivotMesh = nullptr;
		PivotCenterMesh = nullptr;

		RotationCenterMesh = nullptr;
		RotationCenterMeshRingX = nullptr;
		RotationCenterMeshRingY = nullptr;
		RotationCenterMeshRingZ = nullptr;

		nodesPoints.clear();
		nodesLines.clear();
		bonesPoints.clear();
		bonesLines.clear();
		floorMeshes.clear();

		BVHUpdateQueue.clear();

		gls.Cleanup();
	}

	void SetView(const char type) {
		gls.SetView(type);

		if (transformMode)
			ShowTransformTool();
		else
			Render();
	}

	void SetPerspective(const bool enabled) {
		gls.SetPerspective(enabled);
		gls.RenderOneFrame();
	}

	void SetFieldOfView(const int fieldOfView) {
		gls.SetFieldOfView(fieldOfView);
		gls.RenderOneFrame();
	}

	void SetDepthClip(const float zNear, const float zFar) {
		gls.SetDepthClip(zNear, zFar);
		gls.RenderOneFrame();
	}

	void UpdateLights(const int ambient,
					  const int frontal,
					  const int directional0,
					  const int directional1,
					  const int directional2,
					  const nifly::Vector3& directional0Dir = nifly::Vector3(),
					  const nifly::Vector3& directional1Dir = nifly::Vector3(),
					  const nifly::Vector3& directonal2Dir = nifly::Vector3()) {
		gls.UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directonal2Dir);
		gls.RenderOneFrame();
	}

	// Loads an HDRi from res/hdri as the environment, empty for none. Also changes what shapes with a
	// dynamic cubemap reflect, but not the meshes or their textures.
	bool SetHDRiBackground(const std::string& fileName) {
		const bool loaded = gls.SetHDRiBackground(fileName);
		gls.RenderOneFrame();
		return loaded;
	}

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
