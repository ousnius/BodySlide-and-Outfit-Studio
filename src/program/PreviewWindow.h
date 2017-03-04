/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include "../render/GLSurface.h"

class BodySlideApp;
class PreviewCanvas;

#include <gli/save.hpp>
#include <SOIL2\SOIL2.h>


/* Class for managing offscreen rendering buffers.  
	Allows more than one buffer to be created, and automatically cycled for a multipass rendering chain
	Enables exporting buffers to disk files 
*/
class GLOffScreenBuffer {
	GLSurface* glsRef;
	GLuint* pmfbo;
	GLuint* pmtex;
	GLuint mrbo;
	bool isBound;
	int current;
	int numBuffers;
	int w, h;
	int GLOBCount;

	void createTextures() {
		for (int i = 0; i < numBuffers;i++) {
			pmtex[i] = glsRef->GetResourceLoader()->GenerateTextureID(texName(i));
		}
	}

	// Delete textures calls resource loader to remove textures with the appropriate name, and is only
	// called during destruction. 
	//  note, you can save textures by renaming them while GLOB exists.  
	void deleteTextures() {	
		for (int i = 0; i < numBuffers;i++) {
			glsRef->GetResourceLoader()->DeleteTexture(texName(i));
		}		
	}

public:
	GLOffScreenBuffer(GLSurface* gls, int width, int height, unsigned int count = 1, const std::vector<GLuint>& texIds = std::vector<GLuint>()) {
		// for naming textures in CreateTextures
		static int globcount = 0;
		globcount++;
		GLOBCount = globcount;

		glsRef = gls;

		w = width;
		h = height;
		// current active buffer index starts at -1 to detect if it's been set yet or not
		current = -1;
		isBound = false;
		numBuffers = count;
		pmtex = new GLuint[count];
		pmfbo = new GLuint[count];
		// Create Texture destination
		createTextures();
		glGenFrameBuffers(count, pmfbo);

		// Create a renderbuffer for depth information, and attach it
		glGenRenderbuffers(1, &mrbo);
		glBindRenderbuffer(GL_RENDERBUFFER, mrbo);
		glRenderbufferstorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

		for (int i = 0; i < count; i++) {
			if (texIds.size() > i && texIds[i] != 0) {
				pmtex[i] = texIds[i];
			}
			else {
				glBindTexture(GL_TEXTURE_2D, pmtex[i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			// Create frame buffer for rendering destination, and attach texture to it
			glBindFramebuffer(GL_FRAMEBUFFER, pmfbo[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pmtex[i], 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mrbo);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	
	std::string texName(int index) {
		std::string texname = "GLOB_" + std::to_string(GLOBCount) + "_" + std::to_string(index);
		return texname;
	}

	bool SetCurrentBuffer(int bufferIndex = 0) {
		if (bufferIndex < 0 || bufferIndex >= numBuffers)
			return false;
		current = bufferIndex;	
		return true;
	}

	bool NextBuffer(bool cycle = true) {
		if (current < -1) {
			current = -1;		// setting to -1 because we want the increment to put it at 0.
		}
		current++;
		// cycle buffers automatically.
		if (current >= numBuffers) {
			if(cycle)
				current = 0;
			else {
				current--;
				return false;
			}
		}
		return true;
	}

	void Start() {
		if (current < 0) {
			NextBuffer();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, pmfbo[current]);
		isBound = true;
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, w, 0.0, h, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

	}

	// Retrieves the current texture ID.  Useful for externally selecting as a texture source before moving to another 
	// buffer in a multi rendering chain.
	GLuint GetTexID() {
		if(current >-1)
			return pmtex[current];
		return 0;
	}

	void SaveTexture(const string& filename, int SOIL_ImageType) {
		if (!isBound) {
			Start();		//not bound, bind the current framebuffer to read it's pixels.
		}
		GLubyte*  pixels = new GLubyte[w * h * 4];
		glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		SOIL_save_image(filename.c_str(), SOIL_ImageType, w, h, 4, pixels);

		delete[] pixels;

	}
	void End() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		isBound = false;
	}

	~GLOffScreenBuffer() {
		if (isBound) {
			End();
		}
		deleteTextures();
		glDeleteRenderbuffers(1, &mrbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFrameBuffers(numBuffers, pmfbo);
		delete[] pmfbo;
		delete[] pmtex;
	}

};


class PreviewWindow : public wxFrame {
	BodySlideApp* app = nullptr;
	PreviewCanvas* canvas = nullptr; 
	wxGLContext* context = nullptr;

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
		GLuint w, h;
		std::vector<GLuint> texIds;
		std::vector<string> normTextures;
		normTextures.push_back("");
		normTextures.push_back("d:\\proj\\TangentNormalsTest.png");
		//normTextures.push_back("d:\\proj\\masktest.png");
		GLMaterial* normMat = gls.AddMaterial(normTextures, "res\\shaders\\normalshade.vert", "res\\shaders\\normalshade.frag");

		std::vector<string> ppTex;
		ppTex.push_back("d:\\proj\\infile.png");
		GLMaterial* ppMat = gls.AddMaterial(ppTex, "res\\shaders\\fullscreentri.vert", "res\\shaders\\fullscreentri.frag");

		//texIds.push_back(normMat->GetTexID(0));
		GLOffScreenBuffer offscreen(&gls, 1024, 1024, 1, texIds);
		offscreen.Start();
		gls.RenderFullScreenQuad(ppMat);
		offscreen.SaveTexture("d:\\proj\\outfile.png", SOIL_SAVE_TYPE_PNG);
		offscreen.End();

		//gls.GetSize(w, h);		
		//gls.SetPerspective(false);
		//offscreen.Start();
		//gls.RenderToTexture(normMat);
		//gls.GetResourceLoader()->RenameTexture(offscreen.texName(0), "d:\\proj\\femalebody_1_msn.dds", true);
		//offscreen.SaveTexture("d:\\proj\\outfile.png", SOIL_SAVE_TYPE_PNG);
		//offscreen.End();
		//gls.SetPerspective(true);
		// rebuild viewport from original dimensions
		//gls.SetSize(w, h);
		
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
