#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include "wx/wx.h"
#include "wx/image.h"
#include "wx/xrc/xmlres.h"
#include "wx/treectrl.h"
#include "wx/wizard.h"
#include "wx/filepicker.h"
#include "wx/grid.h"
#include "wx/progdlg.h"
#include "wx/spinctrl.h"
#include "wxStateButton.h"
//#include "wx/treelistctrl.h"   // from WXCode... 
#include "wx/dataview.h"
#include "GLSurface.h"
#include "SliderData.h"
#include "ObjFile.h"
#include "TweakBrush.h"
#include "Automorph.h"
#include "OutfitProject.h"
#include "ConfigurationManager.h"

class ShapeItemData : public wxTreeItemData  {
public:
	bool bIsOutfitShape;
	NifFile* refFile;
	string shapeName;
	ShapeItemData(bool isOutfit = false, NifFile* inRefFile = NULL, const string& inShapeName="" ){
		bIsOutfitShape = isOutfit;
		refFile = inRefFile;
		shapeName = inShapeName;
	}
};


class wxGLPanel : public wxPanel 
{
public:
	wxGLPanel(wxWindow* parent, const wxSize& size);

	void SetNotifyWindow(wxWindow* win);

	void AddMeshFromNif(NifFile* nif, char* shapename);
	void AddExplicitMesh(vector<vector3>* v, vector<tri>* t, vector<vector2>* uv = NULL, const string& shapename= "");

	void RenameShape(const string& shapeName, const string& newShapeName) {
		gls.RenameMesh(shapeName,newShapeName);
	}
	
	void SetMeshTexture(const string& shapeName, const string& texturefile, int shaderType=0);

	mesh* GetMesh(const string& shapeName) {
		return gls.GetMesh(shapeName);
	}

	void UpdateMeshVertices(const string& shapeName, vector<vector3>* verts, bool updateBVH=true);
	void RecalculateMeshBVH(const string& shapeName);

	void ShowShape(const string& shapeName, bool show=true);
	void SetActiveShape(const string& shapeName);

	TweakUndo* GetStrokeManager() {
		return strokeManager;
	}
	void SetStrokeManager(TweakUndo* manager) {
		if (manager == NULL) 
			strokeManager = &baseStrokes;
		else
			strokeManager = manager;
	}

	void SetActiveBrush(int brushID);
	TweakBrush* GetActiveBrush() {
		return activeBrush;
	}

	bool StartBrushStroke(wxPoint& screenPos);
	void UpdateBrushStroke(wxPoint& screenPos);
	void EndBrushStroke(wxPoint& screenPos);

	bool StartTransform(wxPoint& screenPos);
	void UpdateTransform(wxPoint& screenPos);
	void EndTransform(wxPoint& screenPos);

	bool UndoStroke();
	bool RedoStroke();

	void ShowTransformTool(bool show = true, bool updateBrush = true);

	void SetEditMode(bool on = true) {
		editMode = on;
	}
	void ToggleEditMode() {
		if(transformMode != 0) {
			transformMode = 0;
			editMode=true;
			ShowTransformTool(false);
		} else {
			editMode=false;
			transformMode = 1;
			ShowTransformTool();
		//	if(activeBrush == NULL) {
		//		activeBrush = &standardBrush;
		//	}
		}
	}
	void ToggleXMirror() {
		if(bXMirror)
			bXMirror = false;
		else 
			bXMirror = true;
	}
	void ToggleConnectedEdit() {
		if(bConnectedEdit)
			bConnectedEdit = false;
		else 
			bConnectedEdit = true;
	}

	void SetShapeGhostMode(const string& shapeName, bool on = true) {
		mesh* m = gls.GetMesh(shapeName);
		if(!m) return;
		if(on) 
			m->rendermode = RenderMode::LitWire;
		else
			m->rendermode = RenderMode::Normal;
	}
	void ToggleGhostMode() {
		mesh* m = gls.GetActiveMesh();
		if(!m) return;
		if(m->rendermode == RenderMode::Normal) {
			m->rendermode = RenderMode::LitWire;
		} else if(m->rendermode == RenderMode::LitWire) {
			m->rendermode = RenderMode::Normal;
		}
	}

	void ToggleNormalSeamSmoothMode() {
		mesh* m = gls.GetActiveMesh();
		if(!m) return;
		if(m->smoothSeamNormals == true) {
			m->smoothSeamNormals = false;
		} else {
			m->smoothSeamNormals = true;
		}
		m->SmoothNormals();
	}

	void RecalcNormals(const string& shape) {
		mesh* m = gls.GetMesh(shape);
		if(!m) return;
		m->SmoothNormals();
	}
	void ToggleAutoNormals() {
		if(bAutoNormals) {
			bAutoNormals = false;
		} else 
			bAutoNormals = true;
	}
	float GetBrushSize() {
		return brushSize;
	}
	void SetBrushSize(float val) {
		brushSize = val;
		gls.SetCursorSize(val);
	}

	float IncBrush() {
		float sz = brushSize + 0.01f;
		gls.SetCursorSize(gls.GetCursorSize() + 0.01f);
		brushSize += 0.01f;
		return sz;
	}
	float DecBrush() {		
		float sz = brushSize - 0.01f;
		gls.SetCursorSize(gls.GetCursorSize() - 0.01f);
		brushSize -= 0.01f;
		return sz;
	}
	float IncStr() {
		if (!activeBrush) return 0.0f;
		float str = activeBrush->getStrength();
		str += 0.001f;
		activeBrush->setStrength(str);
		return str;
	}
	float DecStr() {
		if (!activeBrush) return 0.0f;
		float str = activeBrush->getStrength();
		str -= 0.001f;
		activeBrush->setStrength(str);
		return str;
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
	void ToggleMaskVisible() {
		mesh* m = gls.GetActiveMesh();
		if (!m->vcolors) {
			m->vcolors = new vec3[m->nVerts];
			memset(m->vcolors, 0, sizeof(vec3) * m->nVerts);
		}
		gls.ToggleMask();
	}
	void SetWeightVisible(bool bVisible = true) {
		gls.SetWeightColors(bVisible);
	}

	void ClearMask() {
		mesh* m = gls.GetActiveMesh();
		if (m->vcolors) {
			m->ColorChannelFill(0, 0);
		}
	}

	void GetActiveMask( unordered_map<int, float>& mask) {
		mesh* m = gls.GetActiveMesh();
		if (!m->vcolors)
			return;
		for (int i = 0; i < m->nVerts; i++) {
			if (m->vcolors[i].x != 0.0f) {
				mask[i] = m->vcolors[i].x;
			}
		}
	}

	void GetActiveUnmasked( unordered_map<int, float>& mask) {
		mesh* m = gls.GetActiveMesh();
		if (!m->vcolors) {
			for (int i = 0; i < m->nVerts; i++) 
				mask[i] = 0.0f;
			return;
		}
		for (int i = 0; i < m->nVerts; i++) {
			if (m->vcolors[i].x == 0.0f) {
				mask[i] = m->vcolors[i].x;
			}
		}
	}

	void InvertMask() {
		mesh* m = gls.GetActiveMesh();
		if (!m->vcolors) {
			m->ColorFill(vec3(0, 0, 0));
		} 
		for (int i = 0; i < m->nVerts; i++) {
			m->vcolors[i].x = 1 - m->vcolors[i].x;
		}
	}

	void DeleteMesh(const string& shape) {
		gls.DeleteMesh(shape);
		Refresh();
	}	
	void DestroyOverlays() {
		gls.DeleteOverlays();
		Refresh();
	}


private:
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	void OnSize(wxSizeEvent& event);

	void OnMouseWheel(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event); 

	void OnMiddleDown(wxMouseEvent& event);
	void OnMiddleUp(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);

	void OnRightDown(wxMouseEvent& event);
	void OnRightUp(wxMouseEvent& event);

	void OnIdle(wxIdleEvent& event);
	
	void OnCaptureLost(wxMouseCaptureLostEvent& event);

	GLSurface gls;

	bool rbuttonDown;
	bool lbuttonDown;
	bool mbuttonDown;
	bool isLDragging;
	bool isMDragging;
	bool isRDragging;

	int lastX;
	int lastY;

	set<int> BVHUpdateQueue;

	wxWindow* notifyWindow;

	float brushSize;
	bool editMode;
	bool transformMode;		// 0 = off, 1 = move, 2 = rotate, 3 = scale
	bool bWeightPaint;
	bool isPainting;
	bool isTransforming;
	bool bXMirror;
	bool bAutoNormals;
	bool bConnectedEdit;

	TweakBrush* activeBrush;
	TweakBrush* savedBrush;
	TweakBrush standardBrush;
	TB_Deflate deflateBrush;
	TB_Move moveBrush;
	TB_Smooth smoothBrush;
	TB_Mask maskBrush;
	TB_Unmask UnMaskBrush;
	TB_Weight weightBrush;
	TB_Unweight unweightBrush;
	TB_XForm translateBrush;

	TweakStroke* activeStroke;
	TweakUndo* strokeManager;
	TweakUndo baseStrokes;

	vec3 xformCenter;		// transform center for transform brushes (rotate, specifically cares about this)
	
	DECLARE_EVENT_TABLE()
};

class OutfitProject;

// Define a new frame type: this is going to be our main frame
class OutfitStudio : public wxFrame
{
public:
    // ctor(s)
    OutfitStudio(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size, ConfigurationManager& inConfig);
	~OutfitStudio();
	wxGLPanel* glView;
	wxTreeCtrl* outfitShapes;
	wxTreeCtrl* outfitBones;
	wxScrolledWindow* sliderScroll;
	wxStatusBar* statusBar;
	wxToolBar* toolBar;
	wxTreeItemId shapesRoot;
	wxTreeItemId refRoot;
	wxTreeItemId outfitRoot;
	wxTreeItemId bonesRoot;
	//wxTreemListCtrl* outfitShapes;
	wxImageList* visStateImages;

	ConfigurationManager& appConfig;

	void CreateSetSliders();

	void SetSliderValue(int index, int val);
	void SetSliderValue(const string& name, int val);

	void ApplySliders(bool recalcBVH = true);

	void ShowSliderEffect(int slider, bool show = true);
	void ShowSliderEffect(const string& sliderName, bool show = true);

	void SelectShape(const string& shapeName);

	void OnShapeLoadDone(AsyncMonitor* monitor);

	void UpdateShapeSource(const string& shapeName, bool bIsOutfit);
	int PromptUpdateBase();

	void ActiveShapeUpdated (TweakStroke* refStroke, bool bIsUndo = false);

	bool NotifyStrokeStarting();

	void UpdateActiveShapeUI();

	string GetActiveShape();
	string GetActiveBone();


	bool IsDirty() ;
	bool IsDirty(const string& shapeName);
	void SetClean(const string& shapeName);
	
	// slider edit states -- enable/disable menu items
	void EnterSliderEdit();
	void ExitSliderEdit();

	wxProgressDialog* progWnd;
	vector<pair<float, float>> progressStack;
	int progressVal;
	void StartProgress(const string& title) {
		if(progressStack.size() == 0) {
			progWnd = new wxProgressDialog(title,"Starting...",10000,this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_ELAPSED_TIME);
			progWnd->SetSize(400,150);	
			progressVal = 0;
			progressStack.emplace_back(0.0f,10000.0f);
		}
	}
	void StartSubProgress(float min, float max) {
		float range = progressStack.back().second - progressStack.front().first;
		float mindiv = min / 100.0f;
		float maxdiv = max / 100.0f;
		float minoff = mindiv * range;
		float maxoff = maxdiv * range;
		progressStack.emplace_back(progressStack.front().first+minoff, progressStack.front().first+maxoff);
	}
	void EndProgress() {
		if(progressStack.size() == 0)
			return;
		progWnd->Update(progressStack.back().second);
		progressStack.pop_back();
		if(progressStack.size() == 0)  {
			delete progWnd;
			progWnd = NULL;
		}
	}
	void UpdateProgress(float val, const string& msg) {
		if(progressStack.size() == 0) return;
		float range = progressStack.back().second - progressStack.back().first;
		float div = val / 100.0f;
		float offset = range * div;


		progressVal = progressStack.back().first +  offset;
		if(progressVal > 10000) 
			progressVal = 10000;
		progWnd->Update(progressVal,msg);		
	}

private:

	class SliderDisplay {
	public:
		bool hilite;
		wxPanel* sliderPane;
		wxBoxSizer* paneSz;

		int btnSliderEditID; 
		wxBitmapButton* btnSliderEdit;
		int sliderNameCheckID;
		wxCheckBox* sliderNameCheck;
		int sliderNameID;
		wxStaticText* sliderName;
		int sliderID;
		wxSlider* slider;
		int sliderReadoutID;
		wxTextCtrl* sliderReadout;

		TweakUndo sliderStrokes;			// this probably shouldn't be here, but it's a convenient location to store undo info.
	};

	/*ObjFile testFile;

	NifFile baseNif;
	NifFile workNif;

	DiffDataSets baseDiffData;
	SliderSet activeSet;

	Automorph morpher;
	
	map<string, bool> shapeDirty;

	*/
	OutfitProject* Proj;			// Always assumed to exist!  blank one created in constructor, replaced on load/new

	string activeShape;
	ShapeItemData* activeItem;
	string activeSlider;
	string activeBone;
	bool bEditSlider;

	vec3 previewMove;
	float previewScale;

	map<string, SliderDisplay*> sliderDisplays;

	void createSliderGUI(const string& name, int id, wxScrolledWindow* wnd, wxSizer* rootSz);
	void HighlightSlider(const string& name);
	void ZeroSliders();

	void ClearProject();
	
	void RefreshGUIFromProj();
	void AnimationGUIFromProj();
	void ReferenceGUIFromProj();
	void WorkingGUIFromProj();
	
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void OnExit(wxCommandEvent& event);
	void OnNewProject(wxCommandEvent& event);
	void OnLoadProject(wxCommandEvent &event);
	void OnLoadReference(wxCommandEvent &event);
	void OnLoadOutfit(wxCommandEvent& event);

	void OnNewProject2FP_NIF(wxFileDirPickerEvent& event);
	void OnNewProject2FP_OBJ(wxFileDirPickerEvent& event);
	void OnNewProject2FP_Texture(wxFileDirPickerEvent& event);
	void OnLoadOutfitFP_NIF(wxFileDirPickerEvent& event);
	void OnLoadOutfitFP_OBJ(wxFileDirPickerEvent& event);
	void OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event);

	void OnSaveBaseOutfit(wxCommandEvent &event);
	void OnExportCurrentShapeNif(wxCommandEvent &event);
	void OnMakeConvRef(wxCommandEvent& event);

	void OnSSSNameCopy(wxCommandEvent& event);
	void OnSaveSliderSet(wxCommandEvent &event);
	void OnSaveSliderSetAs(wxCommandEvent &event);


	void OnSlider(wxScrollEvent& event);
	void OnClickSliderButton(wxCommandEvent &event);
	void OnReadoutChange(wxCommandEvent& event);

	void OnMeshBoneButtonClick(wxCommandEvent& event);

	void OnSelectSliders(wxCommandEvent& event);
	void OnOutfitVisToggle(wxTreeEvent& event);
	void OnOutfitShapeSelect(wxTreeEvent& event) ;
	void OnOutfitBoneSelect(wxTreeEvent& event) ;
	void OnOutfitShapeContext(wxTreeEvent& event) ;	
	void OnBoneContext(wxTreeEvent& event) ;
	void OnCheckTreeSel(wxTreeEvent& event);
	void OnCheckBox(wxCommandEvent& event);
	void OnKillFocusOutfitShapes(wxCommandEvent& event);
	
	void OnSelectBrush(wxCommandEvent& event);

	void OnLoadPreset(wxCommandEvent& event);
	void OnSliderConform(wxCommandEvent& event);
	void OnSliderConformAll(wxCommandEvent& event);
	void OnSliderImportBSD(wxCommandEvent& event);
	void OnSliderExportBSD(wxCommandEvent& event);
	void OnSliderImportOBJ(wxCommandEvent& event);

	void OnNewSlider(wxCommandEvent& event);
	void OnNewZapSlider(wxCommandEvent& event);	
	void OnNewCombinedSlider(wxCommandEvent& event);
	void OnSliderNegate(wxCommandEvent& event);
	void OnClearSlider(wxCommandEvent& event);
	void OnDeleteSlider(wxCommandEvent& event);
	void OnSliderProperties(wxCommandEvent& event);

	void OnImportShape(wxCommandEvent& event);
	void OnExportShape(wxCommandEvent& event);

	void OnMoveShape(wxCommandEvent& event);
	void OnOffsetShape(wxCommandEvent& event);
	void OnPreviewMove(wxCommandEvent& event);
	void OnPreviewOffset(wxCommandEvent& event);

	void OnScaleShape(wxCommandEvent& event);
	void OnVirtScaleShape(wxCommandEvent& event);
	void OnPreviewScale(wxCommandEvent& event);
	void OnPreviewVirtScale(wxCommandEvent& event);

	void OnRotateShape(wxCommandEvent& event);
	void OnVirtRotateShape(wxCommandEvent& event);
	
	void OnRenameShape(wxCommandEvent& event);
	void OnSetShapeTexture(wxCommandEvent& event);
	void OnDupeShape(wxCommandEvent& event);
	void OnDeleteShape(wxCommandEvent& event);
	void OnAddBone(wxCommandEvent& event);
	void OnDeleteBone(wxCommandEvent& event);
	void OnCopyBoneWeight(wxCommandEvent& event);
	void OnCopySelectedWeight(wxCommandEvent& event);
	void OnMaskWeighted(wxCommandEvent& event);
	void OnBuildSkinPartitions(wxCommandEvent& event);

	void OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event);
	void OnNPWizChangeSetNameChoice(wxCommandEvent& event);
	
	void OnBrushSettingsSlider(wxScrollEvent& event);

	void OnEditMode(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleEditMode();
	}

	void OnXMirror(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleXMirror();
	}

	void OnConnectedOnly(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleConnectedEdit();
	}

	void OnUndo(wxCommandEvent& WXUNUSED(event)) {
		glView->UndoStroke();
		//ActiveShapeUpdated();
	}
	void OnRedo(wxCommandEvent& WXUNUSED(event)) {
		glView->RedoStroke();
		//ActiveShapeUpdated();
	}

	void OnGhostMesh(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleGhostMode();
		glView->Refresh();
	}
	
	void OnRecalcNormals(wxCommandEvent& WXUNUSED(event)) {
		glView->RecalcNormals(activeShape);
		glView->Refresh();
	}
	
	void OnAutoNormals(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleAutoNormals();
		glView->Refresh();
	}

	void OnSmoothNormalSeams(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleNormalSeamSmoothMode();
		glView->Refresh();
	}

	void OnSetSmoothThreshold(wxCommandEvent& WXUNUSED(event)) {
		if (activeItem == NULL)
			return;

		mesh* m = glView->GetMesh(activeItem->shapeName);
		double a; 
		a = m->GetSmoothThreshold();
		wxString resp = wxString::FromDouble(a,4);
		resp = wxGetTextFromUser("Enter the maximum angle between faces for smoothing to take place", "Set Smooth Angle", resp, this);
		if (resp.empty())
			return;
		resp.ToDouble(&a);
		m->SetSmoothThreshold((float)a);	
		m->SmoothNormals();
		glView->Refresh();
	}

	void OnShowWireframe(wxCommandEvent& WXUNUSED(event)) {
		glView->ShowWireframe();
		glView->Refresh();
	}

	void OnEnableLighting(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleLighting();
		glView->Refresh();
	}

	void OnEnableTextures(wxCommandEvent& WXUNUSED(event)) {
		glView->ToggleTextures();
		glView->Refresh();
	}

	void OnShowMask(wxCommandEvent& WXUNUSED(event)) {
		if (activeShape.empty()) 
			return;
		glView->ToggleMaskVisible();
		glView->Refresh();
	}

	void OnBrushSettings(wxCommandEvent& event);

	void OnIncBrush(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetBrushSize() < 4.0f) {
			float v = glView->IncBrush();
			statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);
			CheckBrushBounds();
		}
	}
	void OnDecBrush(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetBrushSize() > 0.0f) {
			float v = glView->DecBrush();
			statusBar->SetStatusText(wxString::Format("Rad: %f", v), 2);
			CheckBrushBounds();
		}
	}
	void OnIncStr(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() < 0.1f) {
			float v = glView->IncStr();
			statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);
			CheckBrushBounds();
		}
	}
	void OnDecStr(wxCommandEvent& WXUNUSED(event)) {
		if (glView->GetActiveBrush() && glView->GetActiveBrush()->getStrength() > 0.0f) {
			float v = glView->DecStr();
			statusBar->SetStatusText(wxString::Format("Str: %f", v), 2);
			CheckBrushBounds();
		}
	}

	void CheckBrushBounds() {
		TweakBrush* brush = glView->GetActiveBrush();
		float size = glView->GetBrushSize();
		float strength = brush->getStrength();
		//float focus = brush->getFocus();
		//float spacing = brush->getSpacing();

		if (size >= 4.0f)
			GetMenuBar()->Enable(XRCID("btnIncreaseSize"), false);
		else
			GetMenuBar()->Enable(XRCID("btnIncreaseSize"), true);

		if (size <= 0.0f)
			GetMenuBar()->Enable(XRCID("btnDecreaseSize"), false);
		else
			GetMenuBar()->Enable(XRCID("btnDecreaseSize"), true);

		if (strength >= 0.1f)
			GetMenuBar()->Enable(XRCID("btnIncreaseStr"), false);
		else
			GetMenuBar()->Enable(XRCID("btnIncreaseStr"), true);

		if (strength <= 0.0f)
			GetMenuBar()->Enable(XRCID("btnDecreaseStr"), false);
		else
			GetMenuBar()->Enable(XRCID("btnDecreaseStr"), true);
	}

	void OnClearMask(wxCommandEvent& WXUNUSED(event)) {
		if (activeShape.empty()) 
			return;
		glView->ClearMask();
		glView->Refresh();
	}

	void OnInvertMask(wxCommandEvent& WXUNUSED(event)) {	
		if (activeShape.empty()) 
			return;
		glView->InvertMask();
		glView->Refresh();
	}

	void OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
		int a;
		a = 1;
	}

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

class OutfitStudioThreadMonitor : public AsyncMonitor {
protected:
	wxStatusBar* notifyBar;
	OutfitStudio* osRef;
	bool cleanOK;
public:
	OutfitStudioThreadMonitor(OutfitStudio* outfitStudio, wxStatusBar* notifyBar) {
		osRef = outfitStudio;
		this->notifyBar = notifyBar;
		cleanOK = false;
	}
	virtual ~OutfitStudioThreadMonitor() {} 
	/* Notify the host process that a thread has begun execution. */
	virtual void Begin(const string& startStateMessage) {
		stateMessage = startStateMessage;
		notifyBar->SetLabel(stateMessage);
	}
	
	/* Update the status of the thread's ongoing execution.  */
	virtual void Update(const string& updateStateMessage) {
		stateMessage = updateStateMessage;
		notifyBar->SetLabel(stateMessage);
	}

	/* report an error state to the host process. */
	virtual void Error(const string& errorStateMessage, int error) {
		errorMessage = errorStateMessage;
		errorCode = error;
		notifyBar->SetLabel(errorStateMessage);
	}

	/* report successful completion of the thread's work, along with an error code */
	virtual void End (const string& endStateMessage, int code) {
		stateMessage = endStateMessage;
		errorCode = code;
		notifyBar->SetLabel(stateMessage);
		osRef->OnShapeLoadDone(this);
		if(cleanOK) {
			delete this;
		}
	}

	void Delete() {
		cleanOK = true;
	}
};
