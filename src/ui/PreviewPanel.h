/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../render/GLOffscreenBuffer.h"
#include "../render/GLSurface.h"
#include "../utils/ConfigurationManager.h"
#include "../program/NormalsGenDialog.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <wx/wx.h>
#include <wx/activityindicator.h>
#include <wx/popupwin.h>
#include <wx/timer.h>
#include <wx/weakref.h>

class BodySlideApp;
class PreviewCanvas;
class SFMaterialDatabase;

extern ConfigurationManager Config;

struct PreviewProjectEntry {
	std::string projectFile;
	std::string setName;
};

wxDECLARE_EVENT(EVT_PREVIEW_POPOUT, wxCommandEvent);

class PreviewPanel : public wxPanel {
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr;
	std::unique_ptr<wxGLContext> context;
	wxPanel* toolBarPanel = nullptr;
	wxButton* optButton = nullptr;
	wxButton* lockShapeButton = nullptr;
	wxCheckBox* showReferenceCheckbox = nullptr;
	wxCheckBox* showHelperShapesCheckbox = nullptr;
	wxCheckBox* physicsCheckbox = nullptr;
	wxButton* physicsWindButton = nullptr;
	// Created on demand under the window the preview currently lives in, so the
	// controls only exist while the drop-down is around. The values are kept
	// here instead. Weak references: docking the preview back destroys the
	// window the popup hangs off, and the popup with it.
	wxWeakRef<wxPopupTransientWindow> physicsWindPopup;
	wxWeakRef<wxSlider> physicsWindSlider;
	wxWeakRef<wxChoice> physicsWindDir;
	int physicsWindStrength = 0;
	int physicsWindDirIndex = 0;
	wxTimer physicsTimer;
	wxStaticText* projectLabel = nullptr;
	wxChoice* projectChoice = nullptr;
	wxStaticText* presetLabel = nullptr;
	wxChoice* presetChoice = nullptr;
	wxSlider* weightSlider = nullptr;
	wxBitmapButton* popoutButton = nullptr;

	wxPanel* loadingOverlay = nullptr;
	wxActivityIndicator* loadingIndicator = nullptr;

	NormalsGenDialog* normalsGenDlg = nullptr;
	std::vector<NormalGenLayer> emptyLayers;
	std::vector<NormalGenLayer>& refNormalGenLayers;

	GLSurface gls;
	std::unordered_map<std::string, GLMaterial*> shapeMaterials;
	std::string baseDataPath;

	std::vector<std::unique_ptr<SFMaterialDatabase>> sfMaterialDbs;
	std::vector<std::string> sfMaterialDbContents;
	std::vector<std::unique_ptr<std::istringstream>> sfMaterialDbStreams;
	bool sfMaterialDbsLoaded = false;
	bool GetSFMaterialJSON(const std::string& matPath, std::string& jsonOutput);
	void CreatePhysicsWindPopup();
	void DestroyPhysicsWindPopup();
	void ApplyPhysicsWind();
	// Re-flows the tool bar above the canvas after a control was shown or
	// hidden. The panel itself keeps its size, so its sizer needs the explicit
	// nudge.
	void LayoutToolBar();
	std::vector<std::string> extraNifPaths;
	std::vector<PreviewProjectEntry> projectEntries;
	std::string initialPresetName;
	bool loadAllProjects = false;
	bool multiProjectMode = false;
	bool readOnlyMode = false;
	int weight = 100;
	bool glInitialized = false;

public:
	PreviewPanel(wxWindow* parent, BodySlideApp* app);
	~PreviewPanel();

	void OnShown();
	void OnWeightSlider(wxScrollEvent& event);
	void OnProjectChoice(wxCommandEvent& event);
	void OnPresetChoice(wxCommandEvent& event);

	void ShowNormalGenWindow(wxCommandEvent& event);
	void OnLockShape(wxCommandEvent& event);
	void OnShowReference(wxCommandEvent& event);
	void OnShowHelperShapes(wxCommandEvent& event);
	void OnPopout(wxCommandEvent& event);

	void OnPhysics(wxCommandEvent& event);
	void OnPhysicsWindButton(wxCommandEvent& event);
	void OnPhysicsWind(wxScrollEvent& event);
	void OnPhysicsWindDir(wxCommandEvent& event);
	void OnPhysicsTimer(wxTimerEvent& event);

	// One physics tick. Internally paced, so any event source may call it at
	// any rate; a no-op while physics is off.
	void PumpPhysics();

	// Shows or hides the whole physics block. Only meshes that reference a
	// physics XML can be simulated, so the controls stay out of the way for
	// everything else.
	void ShowPhysicsControls(bool show);
	// Reflects whether the simulation is actually running, and shows the wind
	// controls only while it is.
	void SetPhysicsChecked(bool checked);

	void ShowPopoutButton(bool show);
	void SetPopoutButtonDetachedState(bool detached);

	void SetReadOnlyMode(bool readOnly) {
		readOnlyMode = readOnly;
		if (readOnly) {
			ShowLockShapeButton(false);
			ShowPopoutButton(false);
		}
	}

	bool IsReadOnlyMode() const { return readOnlyMode; }

	void Cleanup();

	bool IsGLInitialized() const { return glInitialized; }

	int GetWeight() { return weight; }

	void ShowWeight(bool show = true) {
		weight = 100;
		if (weightSlider) {
			weightSlider->SetValue(weight);
			weightSlider->Show(show);
			LayoutToolBar();
		}
	}

	void ShowLockShapeButton(bool show = true) {
		if (lockShapeButton) {
			lockShapeButton->Show(show);
			LayoutToolBar();
		}
	}

	void ShowReferenceCheckbox(bool show = true) {
		if (showReferenceCheckbox) {
			showReferenceCheckbox->Show(show);
			LayoutToolBar();
		}
	}

	void SetReferenceCheckboxState(bool checked, bool enabled) {
		if (showReferenceCheckbox) {
			showReferenceCheckbox->SetValue(checked);
			showReferenceCheckbox->Enable(enabled);
		}
	}

	bool IsShowReferenceChecked() const {
		return showReferenceCheckbox && showReferenceCheckbox->IsShown() && showReferenceCheckbox->GetValue();
	}

	void SetMeshVisibility(const std::string& shapeName, bool visible) {
		gls.SetMeshVisibility(shapeName, visible);
	}

	void SetComplexMaterialEnabled(bool enabled) {
		gls.SetComplexMaterialEnabled(enabled);
		Render();
	}

	// Unlike the Complex Material switch this decides which shader files a True PBR shape is given, so
	// the caller has to rebuild the preview meshes afterwards for it to take effect.
	void SetPBREnabled(bool enabled) { gls.SetPBREnabled(enabled); }
	bool IsPBREnabled() const { return gls.IsPBREnabled(); }

	void SetBaseDataPath(const std::string& path) { baseDataPath = path; }

	void SetExtraNifPaths(const std::vector<std::string>& paths) { extraNifPaths = paths; }
	const std::vector<std::string>& GetExtraNifPaths() const { return extraNifPaths; }
	void SetProjectData(const std::vector<PreviewProjectEntry>& entries, bool loadAll = false, const std::string& initialPreset = "");
	void SetNormalsGenerationLayers(std::vector<NormalGenLayer>& normalLayers);
	void LoadNifFiles(const std::vector<std::string>& nifFilePaths);
	void LoadProjects(const std::vector<PreviewProjectEntry>& entries);

	Mesh* GetMesh(const std::string& shapeName);
	void AddMeshFromNif(nifly::NifFile* nif, char* shapeName = nullptr);
	void RefreshMeshFromNif(const std::vector<nifly::NifFile*>& nifs);
	void AddNifShapeTextures(nifly::NifFile* fromNif, const std::string& shapeName);

	void UpdateMeshes(const std::string& shapeName, std::vector<nifly::Vector3>* verts, std::vector<nifly::Vector2>* uvs = nullptr) {
		std::unordered_set<int> changed;
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		gls.Update(m, verts, uvs, &changed);
		m->SmoothNormals(changed);
	}

	void SetShapeTextures(const std::string& shapeName,
						  const std::vector<std::string>& textureFiles,
						  const std::string& vShader,
						  const std::string& fShader,
						  const bool hasMatFile = false,
						  const MaterialFile& matFile = MaterialFile(),
						  const bool isPBR = false) {
		Mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader, false, m->hasShader, isPBR);
		if (mat) {
			m->material = mat;
			shapeMaterials[shapeName] = mat;

			if (hasMatFile)
				m->UpdateFromMaterialFile(matFile);

			gls.UpdateShaders(m);
		}
	}

	void SetShapeVertexColors(nifly::NifFile* nif, const std::string& shapeName, Mesh* mesh) {
		const std::vector<nifly::Color4>* vcolors = nif->GetColorsForShape(shapeName);
		if (vcolors) {
			for (size_t v = 0; v < vcolors->size(); v++) {
				mesh->vcolors[v].x = vcolors->at(v).r;
				mesh->vcolors[v].y = vcolors->at(v).g;
				mesh->vcolors[v].z = vcolors->at(v).b;
				mesh->valpha[v] = vcolors->at(v).a;
			}
		}
	}

	void Render() { gls.RenderOneFrame(); }

	void Resized(uint32_t w, uint32_t h) { gls.SetSize(w, h); }

	void ToggleTextures() {
		gls.ToggleTextures();
		gls.RenderOneFrame();
	}

	void ToggleWireframe() {
		gls.ToggleWireframe();
		gls.RenderOneFrame();
	}

	void ToggleLighting() {
		gls.ToggleLighting();
		gls.RenderOneFrame();
	}

	void RenderNormalMap(const std::string& outfilename = "");

	void RightDrag(int dX, int dY);
	void LeftDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);

	void ShowLoadingIndicator(bool show = true);

	wxDECLARE_EVENT_TABLE();
};

class PreviewCanvas : public wxGLCanvas {
	PreviewPanel* previewPanel = nullptr;
	bool firstPaint = true;
	wxPoint lastMousePosition;

public:
	PreviewCanvas(PreviewPanel* pp, const wxGLAttributes& attribs);

	void OnPaint(wxPaintEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnResized(wxSizeEvent& event);

	wxDECLARE_EVENT_TABLE();
};
