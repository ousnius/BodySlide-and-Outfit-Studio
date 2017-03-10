/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include "../render/GLSurface.h"
#include "../render/GLOffscreenBuffer.h"
#include "NormalsGenDialog.h"

class BodySlideApp;
class PreviewCanvas;



class PreviewWindow : public wxFrame {
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr; 
	wxGLContext* context = nullptr;

	NormalsGenDialog* normalsGenDlg = nullptr;

	//GLOffScreenBuffer* offscreen;
	GLSurface gls;
	unordered_map<string, GLMaterial*> shapeMaterials;
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

	void ShowOptWindow(wxCommandEvent& event) { 
		normalsGenDlg->Show(true);		
	}

	void Cleanup() {
		gls.Cleanup();
		shapeMaterials.clear();
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
	void AddNifShapeTextures(NifFile* fromNif, const string& shapeName);

	void Update(string& shapeName, vector<Vector3>* verts, vector<Vector2>* uvs = nullptr) {
		set<int> changed;
		gls.Update(gls.GetMeshID(shapeName), verts, uvs, &changed);

		mesh* m = gls.GetMesh(shapeName);
		if (m)
			m->SmoothNormals(changed);
	}

	void SetShapeTextures(const string& shapeName, const vector<string>& textureFiles, const string& vShader, const string& fShader, const bool hasMatFile = false, const MaterialFile& matFile = MaterialFile()) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m)
			return;

		GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader);
		if (mat) {
			m->material = mat;
			shapeMaterials[shapeName] = mat;

			if (hasMatFile)
				m->UpdateFromMaterialFile(matFile);

			gls.UpdateShaders(m);
		}
	}

	void Render() {
		gls.RenderOneFrame();
	}

	void Resized(uint w, uint h) {
		gls.SetSize(w, h);
	}

	void ToggleSmoothSeams() {
		for (auto &s : shapeMaterials) {
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

	void RenderNormalMap() {

		string dest_tex = gls.GetMesh("BaseShapeHR")->material->GetTexName(1);
		GLuint w, h;
		gls.GetSize(w, h);	
		std::vector<GLuint> texIds;
		std::vector<string> normTextures;
		normTextures.push_back("");
		//normTextures.push_back("d:\\proj\\TangentNormalsTest.png");
		//normTextures.push_back("d:\\proj\\FemaleBodyt_n.dds");
		normTextures.push_back("d:\\proj\\bodyPaintDummy-N_u0_v0.png");
		//normTextures.push_back("d:\\proj\\masktest.png");
		GLMaterial* normMat = gls.AddMaterial(normTextures, "res\\shaders\\normalshade.vert", "res\\shaders\\normalshade.frag");

		std::vector<string> ppTex;
		ppTex.push_back("pproc");
		GLMaterial* ppMat = gls.AddMaterial(ppTex, "res\\shaders\\fullscreentri.vert", "res\\shaders\\fullscreentri.frag");

		
		//texIds.push_back(normMat->GetTexID(0));
		GLOffScreenBuffer offscreen(&gls, 4096, 4096, 2, texIds);
	

		gls.SetPerspective(false);
		offscreen.Start();
		gls.RenderToTexture(normMat);
		gls.GetResourceLoader()->RenameTexture(offscreen.texName(0), "pproc", true);
		//offscreen.SaveTexture("d:\\proj\\normstep.png", SOIL_SAVE_TYPE_PNG);
		offscreen.End();
		offscreen.NextBuffer();

		offscreen.Start();
		gls.RenderFullScreenQuad(ppMat, 4096, 4096);
		gls.GetResourceLoader()->RenameTexture(offscreen.texName(1), dest_tex, true);
		//offscreen.SaveTexture("d:\\proj\\outputnormals.png");
		offscreen.End();
	
		gls.SetPerspective(true);
		// rebuild viewport from original dimensions
		gls.SetSize(w, h);
		Render();
		
	}

	void RightDrag(int dX, int dY);
	void LeftDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);
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
