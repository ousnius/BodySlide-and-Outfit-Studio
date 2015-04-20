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

#define SMALL_PREVIEW 0
#define BIG_PREVIEW 1

LRESULT CALLBACK GLPreviewWindowWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GLWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class PreviewWindow
{
	static bool has_registered;
	HWND mHwnd;
	HWND mGLWindow;
	GLSurface gls;
	void registerClass();
	unordered_map<string, int> shapeTexIds;
	string baseDataPath;

public:
	bool isSmall;
	HWND hOwner;
	PreviewWindow();
	PreviewWindow(HWND owner, NifFile* nif, char PreviewType = SMALL_PREVIEW, char* shapeName = NULL);
	PreviewWindow(HWND owner, vector<vector3>* verts, vector<triangle>* tris, float scale = 1.0f);
	~PreviewWindow();

	void SetBaseDataPath(const string& path) {
		baseDataPath = path;
	}

	void Create(const string& title);

	void Update(int shapeIndex, vector<vector3>* verts, vector<vector2>* uvs = NULL) {
		gls.Update(shapeIndex, verts, uvs);
		string n = gls.GetMeshName(shapeIndex);
		gls.GetMesh(n)->SmoothNormals();
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void Refresh() {
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void AddMeshDirect(mesh* m);
	void AddMeshFromNif(NifFile* nif, char* shapeName = NULL);
	void RefreshMeshFromNif(NifFile* nif, char* shapeName = NULL);
	void AddNifShapeTexture(NifFile* fromNif, const string& shapeName);

	void Update(string& shapeName, vector<vector3>* verts, vector<vector2>* uvs = NULL) {
		gls.Update(gls.GetMeshID(shapeName), verts, uvs);
		mesh* m = gls.GetMesh(shapeName);
		if (m) { // the mesh could be missing if a zap slider removes it
			gls.GetMesh(shapeName)->SmoothNormals();
		}
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void SetShapeTexture(string& shapeName, const string& texturefile, int shaderType = 0) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		int mat;
		if (shaderType == 0)
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\defshader.fs");
		else
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\skinshader.fs");

		m->MatRef = mat;
		shapeTexIds[shapeName] = mat;
	}

	void Render() {
		gls.RenderOneFrame();
	}

	void SetSize(unsigned int w, unsigned int h) {
		SetWindowPos(mGLWindow, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
		gls.SetSize(w, h);
	}

	void ToggleSmoothSeams() {
		mesh* m;
		for (auto s : shapeTexIds) {
			m = gls.GetMesh(s.first);
			if (m) {
				if (m->smoothSeamNormals)
					m->smoothSeamNormals = false;
				else
					m->smoothSeamNormals = true;
				m->SmoothNormals();
			}
		}
		InvalidateRect(mGLWindow, NULL, FALSE);
	}
	void ToggleSmoothSeams(mesh* m) {
		if (m) {
			if (m->smoothSeamNormals)
				m->smoothSeamNormals = false;
			else
				m->smoothSeamNormals = true;
			m->SmoothNormals();
		}
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void ToggleTextures() {
		gls.ToggleTextures();
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void ToggleWireframe() {
		gls.ToggleWireframe();
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void ToggleLighting(){
		gls.ToggleLighting();
		InvalidateRect(mGLWindow, NULL, FALSE);
	}

	void ToggleEditMode() {
		if (gls.bEditMode == false)
			gls.BeginEditMode();
		else
			gls.EndEditMode();

	}

	void Close();

	bool HasFocus() {
		return(mHwnd == GetFocus());
	}

	void RightDrag(int dX, int dY);
	void LeftDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);

	void Pick(int X, int Y);
};
