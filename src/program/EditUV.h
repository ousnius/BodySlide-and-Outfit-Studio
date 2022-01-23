/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitStudio.h"

const int EDITUV_MAX_UNDO = 200;

const int EDITUV_DIRECTION_NONE = 0x0;
const int EDITUV_DIRECTION_RIGHT = 0x1;
const int EDITUV_DIRECTION_LEFT = 0x2;
const int EDITUV_DIRECTION_UP = 0x4;
const int EDITUV_DIRECTION_DOWN = 0x8;

enum EditUVTool { BoxSelection, VertexSelection, Move, Scale, Rotate };

class EditUVAction {
	mesh* actionMesh = nullptr;
	std::unordered_map<int, nifly::Vector2> startState;
	std::unordered_map<int, nifly::Vector2> endState;

public:
	void SetActionMesh(mesh* m) { actionMesh = m; }

	std::unordered_map<int, nifly::Vector2>& GetStartState();
	std::unordered_map<int, nifly::Vector2>& GetEndState();
	void SetStartState(const std::unordered_map<int, nifly::Vector2>& state);
	void SetEndState(const std::unordered_map<int, nifly::Vector2>& state);
	void RestoreStartState();
	void RestoreEndState();
};

class EditUVHistory {
	int curState = -1;
	std::vector<EditUVAction*> actions;

public:
	EditUVHistory();
	~EditUVHistory();

	void Add(EditUVAction* action);
	bool Back();
	bool Forward();
	void Clear();

	EditUVAction* GetCurState() {
		if (curState == -1)
			return nullptr;

		return actions[curState];
	}
};

class EditUVCanvas;

class EditUV : public wxFrame {
public:
	OutfitStudioFrame* os = nullptr;
	EditUVCanvas* canvas = nullptr;
	nifly::NifFile* nif = nullptr;
	nifly::NiShape* shape = nullptr;
	mesh* shapeMesh = nullptr;
	std::string sliderName;

	EditUVAction currentAction;
	EditUVHistory history;

	wxToolBar* uvToolBar = nullptr;
	wxMenuBar* uvMenuBar = nullptr;

	EditUV(wxWindow*, nifly::NifFile*, nifly::NiShape*, mesh*, const std::string&);
	~EditUV();

	void StartTool() { toolActive = toolSelected; }

	EditUVTool GetSelectedTool() { return toolSelected; }

	EditUVTool GetActiveTool() { return toolActive; }

	void SelectTool(EditUVTool tool);
	void Undo();
	void Redo();

private:
	EditUVTool toolSelected = BoxSelection;
	EditUVTool toolActive = BoxSelection;

	void UpdateShapeMesh(bool apply = true);

	void OnSelectTool(wxCommandEvent& event);
	void OnSeamEdges(wxCommandEvent& event);
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);
	void OnSelectAll(wxCommandEvent& event);
	void OnSelectInvert(wxCommandEvent& event);
	void OnSelectLess(wxCommandEvent& event);
	void OnSelectMore(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	wxDECLARE_EVENT_TABLE();
};

class EditUVCanvas : public wxGLCanvas {
public:
	mesh* seamEdgesMesh = nullptr;
	mesh* planeMesh = nullptr;
	mesh* uvGridMesh = nullptr;
	mesh* boxSelectMesh = nullptr;

	EditUVCanvas(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs);
	~EditUVCanvas();

public:
	void SetNotifyWindow(wxWindow*);
	void SelectAll();
	void SelectInvert();
	void SelectLess();
	void SelectMore();

	void SetCursorType(GLSurface::CursorType cursorType) { uvSurface.SetCursorType(cursorType); }

	void Render() { uvSurface.RenderOneFrame(); }

private:
	EditUV* editUV = nullptr;

	GLSurface uvSurface;
	std::unique_ptr<wxGLContext> context;

	bool firstPaint = true;

	bool rbuttonDown = false;
	bool lbuttonDown = false;
	bool mbuttonDown = false;
	bool isLDragging = false;
	bool isMDragging = false;
	bool isRDragging = false;

	int clickX = 0;
	int clickY = 0;
	int upX = 0;
	int upY = 0;
	int lastX = 0;
	int lastY = 0;

	nifly::Vector3 currentCenter;
	int lastDirection;
	float lastAngle;

	int hoverPoint = -1;

	GLMaterial uvGridMaterial;
	GLMaterial boxSelectMaterial;

	void OnShown();
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnMouseWheel(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);
	void OnMiddleDown(wxMouseEvent& event);
	void OnMiddleUp(wxMouseEvent& event);
	void OnRightDown(wxMouseEvent& event);
	void OnRightUp(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void InitMeshes();
	void UpdateCursor(int ScreenX, int ScreenY, const std::string& meshName);
	bool SelectVertex(const wxPoint& screenPos, bool unselect = false);

	wxDECLARE_EVENT_TABLE();
};
