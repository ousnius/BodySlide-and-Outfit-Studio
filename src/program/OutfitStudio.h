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

#include "OutfitProject.h"
#include "../ui/wxStateButton.h"
#include "../render/GLSurface.h"
#include "../components/TweakBrush.h"
#include "../components/UndoHistory.h"
#include "../components/Automorph.h"
#include "../components/RefTemplates.h"
#include "../utils/Log.h"
#include "../utils/ConfigurationManager.h"

#include "../FSEngine/FSManager.h"
#include "../FSEngine/FSEngine.h"

#include <wx/xrc/xmlres.h>
#include <wx/treectrl.h>
#include <wx/wizard.h>
#include <wx/filepicker.h>
#include <wx/grid.h>
#include <wx/spinctrl.h>
#include <wx/dataview.h>
#include <wx/splitter.h>
#include <wx/collpane.h>
#include <wx/clrpicker.h>
#include <wx/srchctrl.h>
#include <wx/tokenzr.h>
#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#ifdef _WINDOWS
#include <wx/msw/registry.h>
#endif

enum TargetGame {
	FO3, FONV, SKYRIM, FO4, SKYRIMSE, FO4VR, SKYRIMVR, FO76, OB
};


class ShapeItemData : public wxTreeItemData  {
	nifly::NiShape* shape = nullptr;

public:
	ShapeItemData(nifly::NiShape* inShape) {
		shape = inShape;
	}

	nifly::NiShape* GetShape() {
		return shape;
	}

	void SetShape(nifly::NiShape* newShape) {
		shape = newShape;
	}
};

struct ShapeItemState {
	nifly::NiShape* shape = nullptr;
	int state = 0;
	bool selected = false;
};

class SegmentItemData : public wxTreeItemData  {
public:
	// partID: a small nonnegative integer uniquely identifying this
	// segment among all the segments and subsegments.  Used as a value
	// in triSParts.  Not in the file.
	int partID;

	SegmentItemData(const int inPartitionID) {
		partID = inPartitionID;
	}
};

class SubSegmentItemData : public wxTreeItemData  {
public:
	// partID: a small nonnegative integer uniquely identifying this
	// subsegment among all the segments and subsegments.  Used as a value
	// in triSParts.  Not in the file.
	int partID;
	uint32_t userSlotID;
	uint32_t material;
	std::vector<float> extraData;

	SubSegmentItemData(const int inPartitionID, const uint32_t& inUserSlotID, const uint32_t& inMaterial, const std::vector<float>& inExtraData = std::vector<float>()) {
		partID = inPartitionID;
		userSlotID = inUserSlotID;
		material = inMaterial;
		extraData = inExtraData;
	}
};

class PartitionItemData : public wxTreeItemData  {
public:
	int index;
	uint16_t type;

	PartitionItemData(const int inIndex, const uint16_t& inType) {
		index = inIndex;
		type = inType;
	}
};

struct WeightCopyOptions {
	float proximityRadius = 0.0f;
	int maxResults = 0;
	bool showSkinTransOption = false;
	bool doSkinTransCopy = false;
	bool doTransformGeo = false;
};

enum class ToolID {
	Any = -1,
	Select = 0,
	MaskBrush,
	InflateBrush,
	DeflateBrush,
	MoveBrush,
	SmoothBrush,
	UndiffBrush,
	WeightBrush,
	ColorBrush,
	AlphaBrush,
	CollapseVertex,
	FlipEdge,
	SplitEdge,
	Transform,
	Pivot,
	VertexEdit
};


struct ConformOptions;
class OutfitStudioFrame;
class EditUV;

enum OverlayLayer : uint32_t {
	Default = 0,
	NodeSelection = 10,
};

enum RotationCenterMode {
	Zero,
	MeshCenter,
	Picked
};

class wxGLPanel : public wxGLCanvas {
public:
	GLSurface gls;
	RotationCenterMode rotationCenterMode = RotationCenterMode::Zero;

	wxGLPanel(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs);
	~wxGLPanel();

	void SetNotifyWindow(wxWindow* win);

	void AddMeshFromNif(nifly::NifFile* nif, const std::string& shapeName);

	void RenameShape(const std::string& shapeName, const std::string& newShapeName) {
		gls.RenameMesh(shapeName, newShapeName);
	}

	void SetMeshTextures(const std::string& shapeName, const std::vector<std::string>& textureFiles, const bool hasMatFile = false, const MaterialFile& matFile = MaterialFile(), const bool reloadTextures = false);

	mesh* GetMesh(const std::string& shapeName) {
		return gls.GetMesh(shapeName);
	}

	void UpdateMeshVertices(const std::string& shapeName, std::vector<nifly::Vector3>* verts, bool updateBVH = true, bool recalcNormals = true, bool render = true, std::vector<nifly::Vector2>* uvs = nullptr);
	void RecalculateMeshBVH(const std::string& shapeName);

	void ShowShape(const std::string& shapeName, bool show = true);
	void SetActiveShapes(const std::vector<std::string>& shapeNames);
	void SetSelectedShape(const std::string& shapeName);

	UndoHistory* GetUndoHistory() {
		return &undoHistory;
	}

	void SetActiveTool(ToolID brushID);
	ToolID GetActiveTool() {
		return activeTool;
	}

	void SetLastTool(ToolID tool);
	ToolID GetLastTool() {
		return lastTool;
	}

	TweakBrush* GetActiveBrush() {
		return activeBrush;
	}

	void SyncBrushStates(TweakBrush* brush) {
		if (!brush)
			return;

		int brushType = brush->Type();
		if (brushType == TBT_STANDARD || brushType == TBT_MOVE || brushType == TBT_MASK) {
			bool isMirrored = brush->isMirrored();
			bool isConnected = brush->isConnected();

			standardBrush.setMirror(isMirrored);
			standardBrush.setConnected(isConnected);

			deflateBrush.setMirror(isMirrored);
			deflateBrush.setConnected(isConnected);

			moveBrush.setMirror(isMirrored);
			moveBrush.setConnected(isConnected);

			smoothBrush.setMirror(isMirrored);
			smoothBrush.setConnected(isConnected);

			maskBrush.setMirror(isMirrored);
			maskBrush.setConnected(isConnected);

			UnMaskBrush.setMirror(isMirrored);
			UnMaskBrush.setConnected(isConnected);

			smoothMaskBrush.setMirror(isMirrored);
			smoothMaskBrush.setConnected(isConnected);
		}
	}

	void SetColorBrush(const nifly::Vector3& color) {
		colorBrush.color = color;
	}

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

	bool RestoreMode(UndoStateProject *usp);
	void ApplyUndoState(UndoStateProject *usp, bool bUndo, bool bRender = true);
	bool UndoStroke();
	bool RedoStroke();

	void ShowRotationCenter(bool show = true);

	void ShowTransformTool(bool show = true);
	void UpdateTransformTool();

	void ShowPivot(bool show = true);
	void UpdatePivot();

	void ShowNodes(bool show = true);
	void UpdateNodes();
	void UpdateNodeColors();

	void ShowBones(bool show = true);
	void UpdateBones();

	void ShowFloor(bool show = true);
	void UpdateFloor();

	void ShowVertexEdit(bool show = true);
	
	bool GetEditMode() {
		return editMode;
	}
	void SetEditMode(bool on = true) {
		editMode = on;
	}

	bool GetBrushMode() {
		return brushMode;
	}
	void SetBrushMode(bool on = true) {
		brushMode = on;
	}

	bool GetVertexEdit() {
		return vertexEdit;
	}
	void SetVertexEdit(bool on = true) {
		vertexEdit = on;
		ShowVertexEdit(on);
		Render();
	}

	bool GetTransformMode() {
		return transformMode;
	}
	void SetTransformMode(bool on = true) {
		transformMode = on;
		ShowTransformTool(on);
	}

	bool GetPivotMode() {
		return pivotMode;
	}
	void SetPivotMode(bool on = true) {
		pivotMode = on;
		ShowPivot(on);

		if (transformMode)
			ShowTransformTool();
	}

	bool GetSegmentMode() {
		return segmentMode;
	}
	void SetSegmentMode(bool on = true) {
		segmentMode = on;
	}

	bool GetGlobalBrushCollision() {
		return bGlobalBrushCollision;
	}
	void SetGlobalBrushCollision(bool on = true) {
		bGlobalBrushCollision = on;
	}

	void SetShapeGhostMode(const std::string& shapeName, bool on = true) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		if (on)
			m->rendermode = RenderMode::LitWire;
		else
			m->rendermode = RenderMode::Normal;
	}

	void ToggleNormalSeamSmoothMode() {
		for (auto &m : gls.GetActiveMeshes()) {
			if (m->smoothSeamNormals == true)
				m->smoothSeamNormals = false;
			else
				m->smoothSeamNormals = true;

			m->SmoothNormals();
		}
	}

	void ToggleLockNormalsMode() {
		for (auto &m : gls.GetActiveMeshes()) {
			if (m->lockNormals == true)
				m->lockNormals = false;
			else
				m->lockNormals = true;
		}
	}

	void RecalcNormals(const std::string& shapeName) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		m->SmoothNormals();
	}

	float GetBrushSize() {
		return brushSize / 3.0f;
	}
	void SetBrushSize(float val) {
		val *= 3.0f;
		brushSize = val;
		gls.SetCursorSize(val);
	}

	float IncBrush() {
		gls.SetCursorSize(gls.GetCursorSize() + 0.010f);
		brushSize += 0.010f;
		LimitBrushSize();
		return brushSize;
	}
	float DecBrush() {
		gls.SetCursorSize(gls.GetCursorSize() - 0.010f);
		brushSize -= 0.010f;
		LimitBrushSize();
		return brushSize;
	}

	void LimitBrushSize() {
		if (brushSize < 0.000f) {
			gls.SetCursorSize(0.000f);
			brushSize = 0.000f;
		}
		else if (brushSize > 3.000f) {
			gls.SetCursorSize(3.000f);
			brushSize = 3.000f;
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

	void SetCursorType(GLSurface::CursorType cursorType) {
		gls.SetCursorType(cursorType);
	}

	void ShowWireframe() {
		gls.ToggleWireframe();
	}

	void ToggleLighting() {
		gls.ToggleLighting();
	}

	void ToggleTextures() {
		gls.ToggleTextures();
	}

	void SetMaskVisible(bool bVisible = true) {
		gls.SetMaskVisible(bVisible);
	}

	void SetWeightVisible(bool bVisible = true) {
		gls.SetWeightColors(bVisible);
	}

	void SetColorsVisible(bool bVisible = true) {
		gls.SetVertexColors(bVisible);
	}

	void ClearMasks() {
		for (auto &m : gls.GetMeshes())
			m->ColorChannelFill(0, 0.0f);
	}

	void ClearActiveMask() {
		for (auto &m : gls.GetActiveMeshes())
			m->ColorChannelFill(0, 0.0f);
	}

	void ClearColors() {
		for (auto &m : gls.GetMeshes())
			m->ColorFill(nifly::Vector3());
	}
	
	void ClearActiveColors() {
		for (auto &m : gls.GetActiveMeshes())
			m->ColorFill(nifly::Vector3());
	}

	void GetActiveMask(std::unordered_map<uint16_t, float>& mask) {
		if (gls.GetActiveMeshes().empty())
			return;

		mesh* m = gls.GetActiveMeshes().back();

		for (int i = 0; i < m->nVerts; i++) {
			if (m->vcolors[i].x != 0.0f)
				mask[i] = m->vcolors[i].x;
		}
	}

	void GetActiveUnmasked(std::unordered_map<uint16_t, float>& mask) {
		if (gls.GetActiveMeshes().empty())
			return;

		mesh* m = gls.GetActiveMeshes().back();

		for (int i = 0; i < m->nVerts; i++)
			if (m->vcolors[i].x == 0.0f)
				mask[i] = m->vcolors[i].x;
	}

	void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++) {
			if (m->vcolors[i].x != 0.0f)
				mask[i] = m->vcolors[i].x;
		}
	}

	void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++)
			if (m->vcolors[i].x == 0.0f)
				mask[i] = m->vcolors[i].x;
	}

	void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		for (int i = 0; i < m->nVerts; i++) {
			if (mask.find(i) != mask.end())
				m->vcolors[i].x = mask[i];
			else
				m->vcolors[i].x = 0.0f;
		}

		m->QueueUpdate(mesh::UpdateType::VertexColors);
	}

	void MaskLess() {
		for (auto &m : gls.GetActiveMeshes()) {
			std::set<int> unmaskPoints;
			for (int i = 0; i < m->nVerts; i++) {
				if (m->vcolors[i].x > 0.0f) {
					std::set<int> adjacentPoints;
					m->GetAdjacentPoints(i, adjacentPoints);

					for (auto& adj : adjacentPoints) {
						if (m->vcolors[adj].x == 0.0f) {
							unmaskPoints.insert(i);
							break;
						}
					}
				}
			}

			for (auto& up : unmaskPoints)
				m->vcolors[up].x = 0.0f;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
	}

	void MaskMore() {
		for (auto &m : gls.GetActiveMeshes()) {
			std::set<int> adjacentPoints;
			for (int i = 0; i < m->nVerts; i++)
				if (m->vcolors[i].x > 0.0f)
					m->GetAdjacentPoints(i, adjacentPoints);

			for (auto& adj : adjacentPoints)
				m->vcolors[adj].x = 1.0f;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
	}

	void InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		std::unordered_map<uint16_t, float> triMask;
		for (int t = 0; t < m->nTris; t++) {
			auto& tri = m->tris[t];
			if (mask.find(tri.p1) != mask.end() ||
				mask.find(tri.p2) != mask.end() ||
				mask.find(tri.p3) != mask.end()) {
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

	void InvertMask() {
		for (auto &m : gls.GetActiveMeshes()) {
			for (int i = 0; i < m->nVerts; i++)
				m->vcolors[i].x = 1.0f - m->vcolors[i].x;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
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
		mesh* m = gls.GetMesh(shape);
		if (m) {
			BVHUpdateQueue.erase(m);
			gls.DeleteMesh(m);
		}
	}

	void ClearOverlays() {
		gls.ClearOverlays();
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

	void UpdateLights(const int ambient, const int frontal, const int directional0, const int directional1, const int directional2,
		const nifly::Vector3& directional0Dir = nifly::Vector3(), const nifly::Vector3& directional1Dir = nifly::Vector3(), const nifly::Vector3& directonal2Dir = nifly::Vector3())
	{
		gls.UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directonal2Dir);
		gls.RenderOneFrame();
	}

	void Render() {
		gls.RenderOneFrame();
	}


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

	void OnKeys(wxKeyEvent& event);
	void OnIdle(wxIdleEvent& event);

	void OnCaptureLost(wxMouseCaptureLostEvent& event);

	std::unique_ptr<wxGLContext> context;

	bool rbuttonDown = false;
	bool lbuttonDown = false;
	bool mbuttonDown = false;
	bool isLDragging = false;
	bool isMDragging = false;
	bool isRDragging = false;
	bool firstPaint = true;

	int lastX = 0;
	int lastY = 0;
	std::string hoverMeshName, mouseDownMeshName;
	int hoverPoint, mouseDownPoint;
	nifly::Edge hoverEdge, mouseDownEdge;
	nifly::Vector3 hoverCoord;

	std::set<mesh*> BVHUpdateQueue;

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
	bool bGlobalBrushCollision = true;

	TweakBrush* activeBrush = nullptr;
	TweakBrush* savedBrush;
	TweakBrush standardBrush;
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
	UndoHistory undoHistory;

	mesh* RotationCenterMesh = nullptr;
	mesh* RotationCenterMeshRingX = nullptr;
	mesh* RotationCenterMeshRingY = nullptr;
	mesh* RotationCenterMeshRingZ = nullptr;

	mesh* XMoveMesh = nullptr;
	mesh* YMoveMesh = nullptr;
	mesh* ZMoveMesh = nullptr;
	mesh* XRotateMesh = nullptr;
	mesh* YRotateMesh = nullptr;
	mesh* ZRotateMesh = nullptr;
	mesh* XScaleMesh = nullptr;
	mesh* YScaleMesh = nullptr;
	mesh* ZScaleMesh = nullptr;
	mesh* ScaleUniformMesh = nullptr;
	nifly::Vector3 xformCenter;		// Transform center for transform brushes (rotate, specifically cares about this)
	float lastCenterDistance = 0.0f;

	mesh* XPivotMesh = nullptr;
	mesh* YPivotMesh = nullptr;
	mesh* ZPivotMesh = nullptr;
	mesh* PivotCenterMesh = nullptr;
	nifly::Vector3 pivotPosition;
	float lastCenterPivotDistance = 0.0f;

	std::vector<mesh*> nodesPoints;
	std::vector<mesh*> nodesLines;
	std::vector<mesh*> bonesPoints;
	std::vector<mesh*> bonesLines;
	std::vector<mesh*> floorMeshes;

	wxDECLARE_EVENT_TABLE();
};


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
	{ wxCMD_LINE_OPTION, "proj", "project", "Project Name", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_PARAM, nullptr, nullptr, "Files", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
	{ wxCMD_LINE_NONE }
};

std::string GetProjectPath();

class OutfitStudio : public wxApp {
public:
	virtual ~OutfitStudio();

	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	virtual bool OnExceptionInMainLoop();
	virtual void OnUnhandledException();
	virtual void OnFatalException();

	void CharHook(wxKeyEvent& event);

	wxString GetGameDataPath(TargetGame targ);

	void InitLanguage();

	bool SetDefaultConfig();
	bool ShowSetup();

	void InitArchives();
	void GetArchiveFiles(std::vector<std::string>& outList);

	TargetGame targetGame;

private:
	OutfitStudioFrame* frame = nullptr;

	Log logger;
	wxLocale* locale = nullptr;
	int language = 0;

	wxArrayString cmdFiles;
	wxString cmdProject;
};


class OutfitProject;

class OutfitStudioFrame : public wxFrame {
public:
	OutfitStudioFrame(const wxPoint& pos, const wxSize& size);
	~OutfitStudioFrame() {}

	wxGLPanel* glView = nullptr;
	EditUV* editUV = nullptr;
	OutfitProject* project = nullptr;
	ShapeItemData* activeItem = nullptr;
	std::string activeSlider;
	std::string lastActiveSlider;
	bool bEditSlider;
	std::string contextBone;
	std::vector<int> triParts;  // the partition index for each triangle, or -1 for none
	std::vector<int> triSParts;  // the segment partition index for each triangle, or -1 for none

	std::unordered_set<std::string> lastSelectedBones;
	std::unordered_set<std::string> lastNormalizeBones;

	wxTreeCtrl* outfitShapes = nullptr;
	wxTreeCtrl* outfitBones = nullptr;
	wxPanel* colorSettings = nullptr;
	wxTreeCtrl* segmentTree = nullptr;
	wxTreeCtrl* partitionTree = nullptr;
	wxPanel* lightSettings = nullptr;
	wxSlider* boneScale = nullptr;
	wxChoice* cXMirrorBone = nullptr;
	wxChoice* cPoseBone = nullptr;
	wxSlider *rxPoseSlider = nullptr;
	wxSlider *ryPoseSlider = nullptr;
	wxSlider *rzPoseSlider = nullptr;
	wxSlider *txPoseSlider = nullptr;
	wxSlider *tyPoseSlider = nullptr;
	wxSlider *tzPoseSlider = nullptr;
	wxTextCtrl *rxPoseText = nullptr;
	wxTextCtrl *ryPoseText = nullptr;
	wxTextCtrl *rzPoseText = nullptr;
	wxTextCtrl *txPoseText = nullptr;
	wxTextCtrl *tyPoseText = nullptr;
	wxTextCtrl *tzPoseText = nullptr;
	wxCheckBox *cbPose = nullptr;
	wxScrolledWindow* sliderScroll = nullptr;
	wxMenuBar* menuBar = nullptr;
	wxToolBar* toolBarH = nullptr;
	wxToolBar* toolBarV = nullptr;
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
	wxCollapsiblePane* masksPane = nullptr;
	wxCollapsiblePane* posePane = nullptr;

	wxTreeItemId shapesRoot;
	wxTreeItemId outfitRoot;
	wxTreeItemId bonesRoot;
	wxTreeItemId segmentRoot;
	wxTreeItemId partitionRoot;

	class SliderDisplay {
	public:
		bool hilite;
		wxPanel* sliderPane;
		wxBoxSizer* paneSz;

		size_t sliderNameCheckID;
		size_t sliderID;

		wxBitmapButton* btnSliderEdit;
		wxBitmapButton* btnSliderProp;
		wxButton* btnMinus;
		wxButton* btnPlus;
		wxCheckBox* sliderNameCheck;
		wxStaticText* sliderName;
		wxSlider* slider;
		wxTextCtrl* sliderReadout;
	};

	std::map<std::string, SliderDisplay*> sliderDisplays;

	void SetPendingChanges(bool pending = true);
	bool CheckPendingChanges();

	void UpdateUndoTools();

	bool SaveProject();
	bool SaveProjectAs();
	bool LoadProject(const std::string& fileName, const std::string& projectName = "", bool clearProject = true);
	void CreateSetSliders();

	std::string NewSlider(const std::string& suggestedName = "", bool skipPrompt = false);

	void SetSliderValue(const size_t index, int val);
	void SetSliderValue(const std::string& name, int val);
	void ZeroSliders();

	void ApplySliders(bool recalcBVH = true);

	void ShowSliderEffect(const size_t slider, bool show = true);
	void ShowSliderEffect(const std::string& sliderName, bool show = true);

	void SelectShape(const std::string& shapeName);
	std::vector<std::string> GetShapeList();

	void UpdateShapeSource(nifly::NiShape* shape);

	void ActiveShapesUpdated(UndoStateProject *usp, bool bIsUndo = false);
	void UpdateActiveShape();
	void UpdateBoneCounts();
	void HighlightBoneNamesWithWeights();
	void RefreshGUIWeightColors();
	void GetNormalizeBones(std::vector<std::string> *normBones, std::vector<std::string> *notNormBones);
	std::vector<std::string> GetSelectedBones();
	void CalcAutoXMirrorBone();
	std::string GetXMirrorBone();

	void ShowSegment(const wxTreeItemId& item = nullptr, bool updateFromMask = false);
	void UpdateSegmentNames();

	void ShowPartition(const wxTreeItemId& item = nullptr, bool updateFromMask = false);
	void UpdatePartitionNames();

	void SetSubMeshesForPartitions(mesh *m, const std::vector<int> &tp);
	void SetNoSubMeshes(mesh *m);
	void SetNoSubMeshes();

	void LockShapeSelect();
	void UnlockShapeSelect();
	void UpdateAnimationGUI();
	void UpdateBoneTree();
	void RefreshGUIFromProj(bool render = true);
	void MeshesFromProj(const bool reloadTextures = false);
	void MeshFromProj(nifly::NiShape* shape, const bool reloadTextures = false);

	void UpdateShapeReference(nifly::NiShape* shape, nifly::NiShape* newShape);
	std::vector<ShapeItemData*>& GetSelectedItems();
	void ClearSelected(nifly::NiShape* shape);
	std::string GetActiveBone();

	bool CheckEditableState();

	void EnterSliderEdit(const std::string& sliderName = "");
	void ExitSliderEdit();
	void MenuEnterSliderEdit();
	void MenuExitSliderEdit();
	void ScrollToActiveSlider();

	void SelectTool(ToolID tool);

	void ToggleBrushPane(bool forceCollapse = false) {
		wxCollapsiblePane* brushPane = (wxCollapsiblePane*)FindWindowByName("brushPane");
		if (!brushPane)
			return;

		brushPane->Collapse(!brushPane->IsCollapsed() || forceCollapse);

		wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
		if (leftPanel)
			leftPanel->Layout();
	}

	void UpdateBrushPane() {
		TweakBrush* brush = glView->GetActiveBrush();
		if (!brush)
			return;

		wxCollapsiblePane* parent = (wxCollapsiblePane*)FindWindowByName("brushPane");
		if (!parent)
			return;

		XRCCTRL(*parent, "brushSize", wxSlider)->SetValue(glView->GetBrushSize() * 1000.0f);
		wxStaticText* valSize = (wxStaticText*)XRCCTRL(*parent, "valSize", wxStaticText);
		wxString valSizeStr = wxString::Format("%0.3f", glView->GetBrushSize());
		valSize->SetLabel(valSizeStr);

		XRCCTRL(*parent, "brushStr", wxSlider)->SetValue(brush->getStrength() * 1000.0f);
		wxStaticText* valStr = (wxStaticText*)XRCCTRL(*parent, "valStr", wxStaticText);
		wxString valStrengthStr = wxString::Format("%0.3f", brush->getStrength());
		valStr->SetLabel(valStrengthStr);

		XRCCTRL(*parent, "brushFocus", wxSlider)->SetValue(brush->getFocus() * 1000.0f);
		wxStaticText* valFocus = (wxStaticText*)XRCCTRL(*parent, "valFocus", wxStaticText);
		wxString valFocusStr = wxString::Format("%0.3f", brush->getFocus());
		valFocus->SetLabel(valFocusStr);

		XRCCTRL(*parent, "brushSpace", wxSlider)->SetValue(brush->getSpacing() * 1000.0f);
		wxStaticText* valSpace = (wxStaticText*)XRCCTRL(*parent, "valSpace", wxStaticText);
		wxString valSpacingStr = wxString::Format("%0.3f", brush->getSpacing());
		valSpace->SetLabel(valSpacingStr);
	}

	void CheckBrushBounds() {
		TweakBrush* brush = glView->GetActiveBrush();
		if (!brush)
			return;

		float size = glView->GetBrushSize();
		float strength = brush->getStrength();
		//float focus = brush->getFocus();
		//float spacing = brush->getSpacing();

		if (size >= 1.0f)
			menuBar->Enable(XRCID("btnIncreaseSize"), false);
		else
			menuBar->Enable(XRCID("btnIncreaseSize"), true);

		if (size <= 0.0f)
			menuBar->Enable(XRCID("btnDecreaseSize"), false);
		else
			menuBar->Enable(XRCID("btnDecreaseSize"), true);

		if (strength >= 1.0f)
			menuBar->Enable(XRCID("btnIncreaseStr"), false);
		else
			menuBar->Enable(XRCID("btnIncreaseStr"), true);

		if (strength <= 0.0f)
			menuBar->Enable(XRCID("btnDecreaseStr"), false);
		else
			menuBar->Enable(XRCID("btnDecreaseStr"), true);
	}

	wxGauge* progressBar = nullptr;
	std::vector<std::pair<int, int>> progressStack;
	int progressVal = 0;

	void StartProgress(const wxString& msg = "") {
		if (progressStack.empty()) {
			progressVal = 0;
			progressStack.emplace_back(0, 10000);

			wxRect rect;
			statusBar->GetFieldRect(1, rect);

			wxBeginBusyCursor();
			progressBar = new wxGauge(statusBar, wxID_ANY, 10000, rect.GetPosition(), rect.GetSize());

			if (msg.IsEmpty())
				statusBar->SetStatusText(_("Starting..."));
			else
				statusBar->SetStatusText(msg);
		}
	}

	void StartSubProgress(int min, int max) {
		int range = progressStack.back().second - progressStack.front().first;
		float mindiv = min / 100.0f;
		float maxdiv = max / 100.0f;
		int minoff = mindiv * range + 1;
		int maxoff = maxdiv * range + 1;
		progressStack.emplace_back(progressStack.front().first + minoff, progressStack.front().first + maxoff);
	}

	void EndProgress(const wxString& msg = "") {
		if (progressStack.empty())
			return;

		progressBar->SetValue(progressStack.back().second);
		progressStack.pop_back();

		if (progressStack.empty()) {
			if (msg.IsEmpty())
				statusBar->SetStatusText(_("Ready!"));
			else
				statusBar->SetStatusText(msg);

			delete progressBar;
			progressBar = nullptr;
			wxEndBusyCursor();
		}
	}

	void UpdateProgress(int val, const wxString& msg = "") {
		if (progressStack.empty())
			return;

		int range = progressStack.back().second - progressStack.back().first;
		float div = val / 100.0f;
		int offset = range * div + 1;

		progressVal = progressStack.back().first + offset;
		if (progressVal > 10000)
			progressVal = 10000;

		statusBar->SetStatusText(msg);
		progressBar->SetValue(progressVal);
	}

private:
	bool pendingChanges = false;

	bool selectionLocked = false;
	std::vector<ShapeItemData*> selectedItems;
	bool recursingUI = false;
	std::string activeBone;
	std::string autoXMirrorBone;
	wxTreeItemId activeSegment;
	wxTreeItemId activePartition;

	std::vector<RefTemplate> refTemplates;

	void createSliderGUI(const std::string& name, const size_t id, wxScrolledWindow* wnd, wxSizer* rootSz);

	void ScrollWindowIntoView(wxScrolledWindow* scrolled, wxWindow* window);
	void HighlightSlider(const std::string& name);

	void UpdateReferenceTemplates();

	void ClearProject();
	void RenameProject(const std::string& projectName);

	void UpdateMeshFromSet(nifly::NiShape* shape);
	void FillVertexColors();

	bool HasUnweightedCheck();
	void CalcCopySkinTransOption(WeightCopyOptions &options);
	bool ShowWeightCopy(WeightCopyOptions& options);
	void ReselectBone();

	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	bool CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size);
	void OnPackProjects(wxCommandEvent &event);
	void OnChooseTargetGame(wxCommandEvent& event);
	void SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame);
	void OnSettings(wxCommandEvent& event);

	void OnSashPosChanged(wxSplitterEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void UpdateTitle();

	void OnNewProject(wxCommandEvent& event);
	void OnLoadProject(wxCommandEvent &event);
	void OnAddProject(wxCommandEvent &event);
	void OnLoadReference(wxCommandEvent &event);
	void OnLoadOutfit(wxCommandEvent& event);
	void OnUnloadProject(wxCommandEvent &event);

	void OnLoadOutfitFP_File(wxFileDirPickerEvent& event);
	void OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event);

	void OnSetBaseShape(wxCommandEvent &event);
	void OnMakeConvRef(wxCommandEvent& event);

	void OnImportNIF(wxCommandEvent &event);
	void OnExportNIF(wxCommandEvent &event);
	void OnExportNIFWithRef(wxCommandEvent &event);
	void OnExportShapeNIF(wxCommandEvent& event);

	void OnImportOBJ(wxCommandEvent& event);
	void OnExportOBJ(wxCommandEvent& event);
	void OnExportShapeOBJ(wxCommandEvent& event);

	void OnImportFBX(wxCommandEvent& event);
	void OnExportFBX(wxCommandEvent& event);
	void OnExportShapeFBX(wxCommandEvent& event);

	void OnImportTRIHead(wxCommandEvent& event);
	void OnExportTRIHead(wxCommandEvent& event);
	void OnExportShapeTRIHead(wxCommandEvent& event);

	void OnImportPhysicsData(wxCommandEvent &event);
	void OnExportPhysicsData(wxCommandEvent &event);

	void OnSSSNameCopy(wxCommandEvent& event);
	void OnSSSGenWeightsTrue(wxCommandEvent& event);
	void OnSSSGenWeightsFalse(wxCommandEvent& event);
	void OnSaveSliderSet(wxCommandEvent &event);
	void OnSaveSliderSetAs(wxCommandEvent &event);

	void OnBrushPane(wxCollapsiblePaneEvent &event);

	void OnSlider(wxScrollEvent& event);
	void OnClickSliderButton(wxCommandEvent &event);
	void OnReadoutChange(wxCommandEvent& event);
	void OnCheckBox(wxCommandEvent& event);

	void OnTabButtonClick(wxCommandEvent& event);
	void OnBrushColorChanged(wxColourPickerEvent& event);
	void OnSwapBrush(wxCommandEvent& event);
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
	void OnCheckTreeSel(wxTreeEvent& event);

	void ToggleBoneState(wxTreeItemId firstItem = wxTreeItemId());
	void OnBoneStateToggle(wxTreeEvent& event);
	void OnBoneSelect(wxTreeEvent& event);
	void OnBoneContext(wxTreeEvent& event);
	void OnBoneTreeContext(wxCommandEvent& event);

	int CalcMaxSegPartID();
	void OnSegmentSelect(wxTreeEvent& event);
	void OnSegmentContext(wxTreeEvent& event);
	void OnSegmentTreeContext(wxCommandEvent& event);
	void OnAddSegment(wxCommandEvent& event);
	void OnAddSubSegment(wxCommandEvent& event);
	void OnDeleteSegment(wxCommandEvent& event);
	void OnDeleteSubSegment(wxCommandEvent& event);
	void OnSegmentSlotChanged(wxCommandEvent& event);
	void OnSegmentTypeChanged(wxCommandEvent& event);
	void OnSegmentApply(wxCommandEvent& event);
	void ApplySegments();
	void OnSegmentReset(wxCommandEvent& event);
	void ResetSegments();
	void OnSegmentEditSSF(wxCommandEvent& event);

	void OnPartitionSelect(wxTreeEvent& event);
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
	void OnShowNodes(wxCommandEvent& event);
	void OnShowBones(wxCommandEvent& event);
	void OnShowFloor(wxCommandEvent& event);
	void OnFieldOfViewSlider(wxCommandEvent& event);
	void OnUpdateLights(wxCommandEvent& event);
	void OnResetLights(wxCommandEvent& event);

	void OnLoadPreset(wxCommandEvent& event);
	void OnSavePreset(wxCommandEvent& event);
	bool ShowConform(ConformOptions& options);
	void ConformSliders(nifly::NiShape* shape, const ConformOptions& options);
	void OnSliderConform(wxCommandEvent& event);
	void OnSliderConformAll(wxCommandEvent& event);
	void OnSliderImportNIF(wxCommandEvent& event);
	void OnSliderImportBSD(wxCommandEvent& event);
	void OnSliderImportOBJ(wxCommandEvent& event);
	void OnSliderImportOSD(wxCommandEvent& event);
	void OnSliderImportTRI(wxCommandEvent& event);
	void OnSliderImportFBX(wxCommandEvent& event);
	void OnSliderExportNIF(wxCommandEvent& event);
	void OnSliderExportBSD(wxCommandEvent& event);
	void OnSliderExportOBJ(wxCommandEvent& event);
	void OnSliderExportOSD(wxCommandEvent& event);
	void OnSliderExportTRI(wxCommandEvent& event);
	void OnSliderExportToOBJs(wxCommandEvent& event);

	void OnNewSlider(wxCommandEvent& event);
	void OnNewZapSlider(wxCommandEvent& event);
	void OnNewCombinedSlider(wxCommandEvent& event);
	void OnSliderNegate(wxCommandEvent& event);
	void OnMaskAffected(wxCommandEvent& event);
	void OnClearSlider(wxCommandEvent& event);
	void OnDeleteSlider(wxCommandEvent& event);

	void ShowSliderProperties(const std::string& sliderName);
	void OnSliderProperties(wxCommandEvent& event);

	void OnInvertUV(wxCommandEvent& event);
	void OnMirror(wxCommandEvent& event);

	void OnEnterClose(wxKeyEvent& event);

	void OnMoveShape(wxCommandEvent& event);
	void OnScaleShape(wxCommandEvent& event);
	void OnRotateShape(wxCommandEvent& event);

	void OnRenameShape(wxCommandEvent& event);
	void OnSetReference(wxCommandEvent& event);
	void OnDeleteVerts(wxCommandEvent& event);
	void OnSeparateVerts(wxCommandEvent& event);
	void CheckCopyGeo(wxDialog &dlg);
	void OnCopyGeo(wxCommandEvent& event);
	void OnDupeShape(wxCommandEvent& event);
	void OnDeleteShape(wxCommandEvent& event);
	void OnAddBone(wxCommandEvent& event);
	void OnAddCustomBone(wxCommandEvent& event);
	void OnDeleteBone(wxCommandEvent& event);
	void OnDeleteBoneFromSelected(wxCommandEvent& event);
	void FillParentBoneChoice(wxDialog &dlg, const std::string &selBone = "");
	void GetBoneDlgData(wxDialog &dlg, nifly::MatTransform &xform, std::string &parentBone);
	void OnEditBone(wxCommandEvent& event);
	void OnCopyBoneWeight(wxCommandEvent& event);
	void OnCopySelectedWeight(wxCommandEvent& event);
	void OnTransferSelectedWeight(wxCommandEvent& event);
	void OnMaskWeighted(wxCommandEvent& event);
	void OnMaskBoneWeighted(wxCommandEvent& event);
	void OnResetTransforms(wxCommandEvent& event);
	void OnDeleteUnreferencedNodes(wxCommandEvent& event);
	void OnRemoveSkinning(wxCommandEvent& event);
	void OnShapeProperties(wxCommandEvent& event);

	void OnMaskLess(wxCommandEvent& event);
	void OnMaskMore(wxCommandEvent& event);

	void OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event);
	void OnNPWizChangeSetNameChoice(wxCommandEvent& event);

	void OnBrushSettingsSlider(wxScrollEvent& event);

	void OnXMirror(wxCommandEvent& event) {
		bool enabled = event.IsChecked();
		menuBar->Check(event.GetId(), enabled);
		toolBarV->ToggleTool(event.GetId(), enabled);

		auto activeBrush = glView->GetActiveBrush();
		if (activeBrush) {
			activeBrush->setMirror(enabled);
			glView->SyncBrushStates(activeBrush);
		}
	}

	void OnConnectedOnly(wxCommandEvent& event) {
		bool enabled = event.IsChecked();
		menuBar->Check(event.GetId(), enabled);
		toolBarV->ToggleTool(event.GetId(), enabled);

		auto activeBrush = glView->GetActiveBrush();
		if (activeBrush) {
			activeBrush->setConnected(enabled);
			glView->SyncBrushStates(activeBrush);
		}
	}

	void OnGlobalBrushCollision(wxCommandEvent& event) {
		bool enabled = event.IsChecked();
		menuBar->Check(event.GetId(), enabled);
		toolBarV->ToggleTool(event.GetId(), enabled);
		glView->SetGlobalBrushCollision(enabled);
	}

	void OnUndo(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetSegmentMode())
			return;

		glView->UndoStroke();
		UpdateUndoTools();
	}

	void OnRedo(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetSegmentMode())
			return;

		glView->RedoStroke();
		UpdateUndoTools();
	}

	void OnRecalcNormals(wxCommandEvent& WXUNUSED(event));
	void OnSmoothNormalSeams(wxCommandEvent& event);
	void OnLockNormals(wxCommandEvent& event);

	void OnEditUV(wxCommandEvent& event);

	void OnToggleVisibility(wxCommandEvent& WXUNUSED(event)) {
		ToggleVisibility();
	}

	void OnShowWireframe(wxCommandEvent& WXUNUSED(event)) {
		glView->ShowWireframe();
		glView->Render();
	}

	void OnEnableLighting(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleLighting();
		glView->Render();
	}

	void OnEnableTextures(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleTextures();
		glView->Render();
	}

	void OnIncBrush(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetBrushSize() < 1.0f) {
			float v = glView->IncBrush() / 3.0f;
			if (statusBar)
				statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);

			CheckBrushBounds();
			UpdateBrushPane();
		}
	}
	void OnDecBrush(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetBrushSize() > 0.0f) {
			float v = glView->DecBrush() / 3.0f;
			if (statusBar)
				statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);

			CheckBrushBounds();
			UpdateBrushPane();
		}
	}
	void OnIncStr(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() < 1.0f) {
			float v = glView->IncStr();
			if (statusBar)
				statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);

			CheckBrushBounds();
			UpdateBrushPane();
		}
	}
	void OnDecStr(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() > 0.0f) {
			float v = glView->DecStr();
			if (statusBar)
				statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);

			CheckBrushBounds();
			UpdateBrushPane();
		}
	}

	void OnClearMask(wxCommandEvent& WXUNUSED(event)) {
		if (!activeItem)
			return;

		glView->ClearActiveMask();

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
	}

	void OnInvertMask(wxCommandEvent& WXUNUSED(event)) {
		if (!activeItem)
			return;

		glView->InvertMask();

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
	}

	void OnDiscord(wxCommandEvent& WXUNUSED(event)) {
		wxString url = "https://discordapp.com/invite/2qg2Rzw";
		wxLaunchDefaultBrowser(url);
	}

	void OnGitHub(wxCommandEvent& WXUNUSED(event)) {
		wxString url = "https://github.com/ousnius/BodySlide-and-Outfit-Studio";
		wxLaunchDefaultBrowser(url);
	}

	void OnPayPal(wxCommandEvent& WXUNUSED(event)) {
		wxString url = "https://www.paypal.me/ousnius";
		wxLaunchDefaultBrowser(url);
	}

	void OnSelectMask(wxCommandEvent& event);
	void OnSaveMask(wxCommandEvent& event);
	void OnSaveAsMask(wxCommandEvent& event);
	void OnDeleteMask(wxCommandEvent& event);
	void OnPaneCollapse(wxCollapsiblePaneEvent& event);
	void ApplyPose();
	AnimBone *GetPoseBonePtr();
	void PoseToGUI();
	void OnPoseBoneChanged(wxCommandEvent& event);
	void OnPoseValChanged(int cind, float val);
	void OnAnyPoseSlider(wxScrollEvent &e, wxTextCtrl *t, int cind);
	void OnRXPoseSlider(wxScrollEvent& event);
	void OnRYPoseSlider(wxScrollEvent& event);
	void OnRZPoseSlider(wxScrollEvent& event);
	void OnTXPoseSlider(wxScrollEvent& event);
	void OnTYPoseSlider(wxScrollEvent& event);
	void OnTZPoseSlider(wxScrollEvent& event);
	void OnAnyPoseTextChanged(wxTextCtrl *t, wxSlider *s, int cind);
	void OnRXPoseTextChanged(wxCommandEvent& event);
	void OnRYPoseTextChanged(wxCommandEvent& event);
	void OnRZPoseTextChanged(wxCommandEvent& event);
	void OnTXPoseTextChanged(wxCommandEvent& event);
	void OnTYPoseTextChanged(wxCommandEvent& event);
	void OnTZPoseTextChanged(wxCommandEvent& event);
	void OnResetBonePose(wxCommandEvent& event);
	void OnResetAllPose(wxCommandEvent& event);
	void OnPoseToMesh(wxCommandEvent& event);
	void OnPoseCheckBox(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

class DnDFile : public wxFileDropTarget {
public:
	DnDFile(OutfitStudioFrame *pOwner = nullptr) { owner = pOwner; }

	virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileNames);

private:
	OutfitStudioFrame *owner = nullptr;
};

class DnDSliderFile : public wxFileDropTarget {
public:
	DnDSliderFile(OutfitStudioFrame *pOwner = nullptr) { owner = pOwner; }

	virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& fileNames);
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult);

private:
	OutfitStudioFrame *owner = nullptr;
	wxDragResult lastResult = wxDragNone;
	std::string targetSlider;
};

std::string JoinStrings(const std::vector<std::string>& elements, const char* const separator);
