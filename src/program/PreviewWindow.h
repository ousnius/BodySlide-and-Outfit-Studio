/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../render/GLSurface.h"
#include "../files/MaterialFile.h"

class BodySlideApp;
class PreviewCanvas;

class PreviewWindow : public wxFrame {
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr; 
	wxGLContext* context = nullptr;

	GLSurface gls;
	unordered_map<string, GLMaterial*> shapeTextures;
	string baseDataPath;
	int weight = 100;

	wxDECLARE_EVENT_TABLE();
	static wxSize GetDefaultSize();

public:
	PreviewWindow(BodySlideApp* app);
	~PreviewWindow();

	void OnShown();
	void OnClose(wxCloseEvent& event);
	void OnWeightSlider(wxScrollEvent& event);

	void Cleanup() {
		gls.Cleanup();
		shapeTextures.clear();
		gls.RenderOneFrame();
	}

	int GetWeight() {
		return weight;
	}
	
	void ShowWeight(bool show = true) {
		weight = 100;
		wxSlider* weightSlider = (wxSlider*)FindWindowByName("weightSlider", this);
		if (weightSlider) {
			weightSlider->SetValue(weight);
			weightSlider->GetParent()->Show(show);
			Layout();
		}
	}

	void SetBaseDataPath(const string& path) {
		baseDataPath = path;
	}

	void AddMeshFromNif(NifFile* nif, char* shapeName = nullptr);
	void RefreshMeshFromNif(NifFile* nif, char* shapeName = nullptr);
	void AddNifShapeTexture(NifFile* fromNif, const string& shapeName);

	void Update(string& shapeName, vector<Vector3>* verts, vector<Vector2>* uvs = nullptr) {
		vector<int> changed;
		gls.Update(gls.GetMeshID(shapeName), verts, uvs, &changed);

		mesh* m = gls.GetMesh(shapeName);
		if (m)
			m->SmoothNormals(changed);
	}

	void SetShapeTexture(const string& shapeName, const string& texturefile, bool isSkin = false) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		GLMaterial* mat;
		if (!isSkin)
			mat = gls.AddMaterial(texturefile, "res\\shaders\\default.vert", "res\\shaders\\default.frag");
		else
			mat = gls.AddMaterial(texturefile, "res\\shaders\\default.vert", "res\\shaders\\skin.frag");

		m->material = mat;
		shapeTextures[shapeName] = mat;
	}

	void Render() {
		gls.RenderOneFrame();
	}

	void Resized(uint w, uint h) {
		gls.SetSize(w, h);
	}

	void ToggleSmoothSeams() {
		for (auto &s : shapeTextures) {
			mesh* m = gls.GetMesh(s.first);
			if (m) {
				if (m->smoothSeamNormals)
					m->smoothSeamNormals = false;
				else
					m->smoothSeamNormals = true;

				m->SmoothNormals();
			}
		}
		gls.RenderOneFrame();
	}

	void ToggleTextures() {
		gls.ToggleTextures();
		gls.RenderOneFrame();
	}

	void ToggleWireframe() {
		gls.ToggleWireframe();
		gls.RenderOneFrame();
	}

	void ToggleLighting(){
		gls.ToggleLighting();
		gls.RenderOneFrame();
	}

	void ToggleEditMode() {
		if (gls.bEditMode == false)
			gls.BeginEditMode();
		else
			gls.EndEditMode();

	}

	void RightDrag(int dX, int dY);
	void LeftDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);

	void Pick(int X, int Y);
};

class PreviewCanvas : public wxGLCanvas {
	PreviewWindow* previewWindow = nullptr;
	bool firstPaint = true;
	wxPoint lastMousePosition;

public:
	PreviewCanvas(PreviewWindow* pw, const wxGLAttributes& attribs);

	void OnPaint(wxPaintEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnResized(wxSizeEvent& event);

	wxDECLARE_EVENT_TABLE();
};
