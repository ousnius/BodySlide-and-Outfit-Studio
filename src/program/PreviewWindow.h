/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../render/GLOffscreenBuffer.h"
#include "../render/GLSurface.h"
#include "../utils/ConfigurationManager.h"
#include "NormalsGenDialog.h"
#include <wx/wx.h>

class BodySlideApp;
class PreviewCanvas;

extern ConfigurationManager Config;


class PreviewWindow : public wxFrame {
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr;
	std::unique_ptr<wxGLContext> context;
	wxButton* optButton = nullptr;

	NormalsGenDialog* normalsGenDlg = nullptr;
	// empty normal gen layers vectory to temporarily occupy the reference until assigned by bodyslide.
	std::vector<NormalGenLayer> emptyLayers;
	// Foreign normals layer generation reference, stored in SliderSet, but configured and used within the preview window.
	std::vector<NormalGenLayer>& refNormalGenLayers;

	//GLOffScreenBuffer* offscreen;
	GLSurface gls;
	std::unordered_map<std::string, GLMaterial*> shapeMaterials;
	std::string baseDataPath;
	int weight = 100;

	wxDECLARE_EVENT_TABLE();

public:
	PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app);
	~PreviewWindow();

	void OnShown();
	void OnClose(wxCloseEvent& event);
	void OnWeightSlider(wxScrollEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void ShowNormalGenWindow(wxCommandEvent& event);

	void Cleanup();

	int GetWeight() { return weight; }

	void ShowWeight(bool show = true) {
		weight = 100;
		wxSlider* weightSlider = (wxSlider*)FindWindowByName("weightSlider", this);
		if (weightSlider) {
			weightSlider->SetValue(weight);
			weightSlider->GetParent()->Show(show);
			Layout();
		}
	}

	void SetBaseDataPath(const std::string& path) { baseDataPath = path; }

	void SetNormalsGenerationLayers(std::vector<NormalGenLayer>& normalLayers);

	Mesh* GetMesh(const std::string& shapeName);
	void AddMeshFromNif(nifly::NifFile* nif, char* shapeName = nullptr);
	void RefreshMeshFromNif(nifly::NifFile* nif, char* shapeName = nullptr);
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
						  const MaterialFile& matFile = MaterialFile()) {
		Mesh* m = gls.GetMesh(shapeName);
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

	void RenderNormalMap(const std::string& outfilename = "") {
		wxBusyCursor busycursor;

		std::string dest_tex = gls.GetMesh("CBBE")->material->GetTexName(1);
		GLuint w, h;
		gls.GetSize(w, h);
		std::vector<GLuint> texIds;
		std::vector<std::string> normTextures;
		normTextures.resize(20);
		//"d:\\proj\\TangentNormalsTest.png"
		//"d:\\proj\\FemaleBodyt_n.dds"
		//"d:\\proj\\bodyPaintDummy-N_u0_v0.png"
		//normTextures[20] = "d:\\proj\\masktest.png";
		GLMaterial* normMat = gls.AddMaterial(normTextures, Config["AppDir"] + "/res/shaders/normalshade.vert", Config["AppDir"] + "/res/shaders/normalshade.frag");

		std::vector<std::string> ppTex;
		ppTex.push_back("pproc");
		GLMaterial* ppMat = gls.AddMaterial(ppTex, Config["AppDir"] + "/res/shaders/fullscreentri.vert", Config["AppDir"] + "/res/shaders/fullscreentri.frag");


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
		if (!outfilename.empty()) {
			offscreen.SaveTexture(outfilename);
		}
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
