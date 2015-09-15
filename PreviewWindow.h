/*
Caliente's BodySlide
by Caliente

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once

#include "stdafx.h"
#include "GLSurface.h"
#include "NifFile.h"

#include <wx/glcanvas.h>

class BodySlideApp;
class PreviewCanvas;

class PreviewWindow : public wxFrame
{
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr; 
	wxGLContext* context = nullptr;

	GLSurface gls;
	unordered_map<string, GLMaterial*> shapeTextures;
	string baseDataPath;
	int weight = 100;

	DECLARE_EVENT_TABLE();
	static wxSize GetDefaultSize();

public:
	PreviewWindow(BodySlideApp* app);
	~PreviewWindow();

	void OnShown();

	void Cleanup() {
		gls.Cleanup();
		shapeTextures.clear();
	}

	int GetWeight() {
		return weight;
	}

	void SetBaseDataPath(const string& path) {
		baseDataPath = path;
	}

	void Update(int shapeIndex, vector<Vector3>* verts, vector<Vector2>* uvs = nullptr) {
		gls.Update(shapeIndex, verts, uvs);
		string n = gls.GetMeshName(shapeIndex);
		gls.GetMesh(n)->SmoothNormals();
		Refresh();
	}

	void Refresh() {
		Render();
	}

	void AddMeshDirect(mesh* m);
	void AddMeshFromNif(NifFile* nif, char* shapeName = nullptr);
	void RefreshMeshFromNif(NifFile* nif, char* shapeName = nullptr);
	void AddNifShapeTexture(NifFile* fromNif, const string& shapeName);

	void Update(string& shapeName, vector<Vector3>* verts, vector<Vector2>* uvs = nullptr) {
		gls.Update(gls.GetMeshID(shapeName), verts, uvs);
		mesh* m = gls.GetMesh(shapeName);
		if (m) {	// The mesh could be missing if a zap removes it
			gls.GetMesh(shapeName)->SmoothNormals();
		}
		Refresh();
	}

	void SetShapeTexture(const string& shapeName, const string& texturefile, bool isSkin = false) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		GLMaterial* mat;
		if (!isSkin)
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\defshader.fs");
		else
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\skinshader.fs");

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
		mesh* m;
		for (auto &s : shapeTextures) {
			m = gls.GetMesh(s.first);
			if (m) {
				if (m->smoothSeamNormals)
					m->smoothSeamNormals = false;
				else
					m->smoothSeamNormals = true;
				m->SmoothNormals();
			}
		}
		Refresh();
	}
	void ToggleSmoothSeams(mesh* m) {
		if (m) {
			if (m->smoothSeamNormals)
				m->smoothSeamNormals = false;
			else
				m->smoothSeamNormals = true;
			m->SmoothNormals();
		}
	}

	void ToggleTextures() {
		gls.ToggleTextures();
		Refresh();
	}

	void ToggleWireframe() {
		gls.ToggleWireframe();
		Refresh();
	}

	void ToggleLighting(){
		gls.ToggleLighting();
		Refresh();
	}

	void ToggleEditMode() {
		if (gls.bEditMode == false)
			gls.BeginEditMode();
		else
			gls.EndEditMode();

	}

	void OnWeightSlider(wxScrollEvent& event);
	void OnClose(wxCloseEvent& event);

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
	PreviewCanvas(PreviewWindow* pw, const int* attribs);

	void OnPaint(wxPaintEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnResized(wxSizeEvent& event);

	DECLARE_EVENT_TABLE();
};
