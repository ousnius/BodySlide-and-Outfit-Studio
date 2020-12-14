/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PreviewWindow.h"
#include "BodySlideApp.h"

#include <sstream>
#include <regex>

extern ConfigurationManager Config;
extern ConfigurationManager BodySlideConfig;

wxBEGIN_EVENT_TABLE(PreviewWindow, wxFrame)
	EVT_CLOSE(PreviewWindow::OnClose)
	EVT_COMMAND_SCROLL(wxID_ANY, PreviewWindow::OnWeightSlider)
	EVT_MOVE_END(PreviewWindow::OnMoveWindow)
	EVT_SIZE(PreviewWindow::OnSetSize)
wxEND_EVENT_TABLE()

PreviewWindow::~PreviewWindow() {
}

PreviewWindow::PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app)
	: wxFrame(nullptr, wxID_ANY, _("Preview"), pos, size), app(app), refNormalGenLayers(emptyLayers) {
	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/BodySlide.png", wxBITMAP_TYPE_PNG));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerPanel = new wxBoxSizer(wxHORIZONTAL);

	wxPanel* uiPanel = new wxPanel(this);
	wxSlider* weightSlider = new wxSlider(uiPanel, wxID_ANY, 100, 0, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_LABELS, wxDefaultValidator, "weightSlider");

	optButton = new wxButton(uiPanel, wxID_ANY, "N", wxDefaultPosition, wxSize(25, 25));
	optButton->SetToolTip(_("Show the Normal Map Generator dialog."));
	optButton->Bind(wxEVT_BUTTON, &PreviewWindow::ShowNormalGenWindow, this);
	optButton->Hide();

	uiPanel->SetBackgroundColour(wxColour(210, 210, 210));

	canvas = new PreviewCanvas(this, GLSurface::GetGLAttribs());
	context = std::make_unique<wxGLContext>(canvas, nullptr, &GLSurface::GetGLContextAttribs());

	sizerPanel->Add(weightSlider, 1, wxTOP | wxLEFT | wxRIGHT, 10);
	sizerPanel->Add(optButton, 0, wxTOP | wxLEFT | wxRIGHT, 10);
	uiPanel->SetSizer(sizerPanel);

	sizer->Add(uiPanel, 0, wxEXPAND);
	sizer->Add(canvas, 1, wxEXPAND);

	SetSizer(sizer);
	Show();
}

void PreviewWindow::OnShown() {
	if (!context->IsOK()) {
		Destroy();
		canvas = nullptr;
		//delete offscreen;
		app->PreviewClosed();
		wxLogError("Preview failed: OpenGL context is not OK.");
		wxMessageBox(_("Preview failed: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR);
		return;
	}

	wxLogMessage("Initializing preview window...");
	gls.Initialize(canvas, context.get());
	auto size = canvas->GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight(), 65.0);

	//offscreen = new GLOffScreenBuffer(4096, 4096);

	int ambient = Config.GetIntValue("Lights/Ambient");
	int frontal = Config.GetIntValue("Lights/Frontal");

	int directional0 = Config.GetIntValue("Lights/Directional0");
	int directional0X = Config.GetIntValue("Lights/Directional0.x");
	int directional0Y = Config.GetIntValue("Lights/Directional0.y");
	int directional0Z = Config.GetIntValue("Lights/Directional0.z");

	int directional1 = Config.GetIntValue("Lights/Directional1");
	int directional1X = Config.GetIntValue("Lights/Directional1.x");
	int directional1Y = Config.GetIntValue("Lights/Directional1.y");
	int directional1Z = Config.GetIntValue("Lights/Directional1.z");

	int directional2 = Config.GetIntValue("Lights/Directional2");
	int directional2X = Config.GetIntValue("Lights/Directional2.x");
	int directional2Y = Config.GetIntValue("Lights/Directional2.y");
	int directional2Z = Config.GetIntValue("Lights/Directional2.z");

	Vector3 directional0Dir = Vector3(directional0X / 100.0f, directional0Y / 100.0f, directional0Z / 100.0f);
	Vector3 directional1Dir = Vector3(directional1X / 100.0f, directional1Y / 100.0f, directional1Z / 100.0f);
	Vector3 directional2Dir = Vector3(directional2X / 100.0f, directional2Y / 100.0f, directional2Z / 100.0f);
	gls.UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directional2Dir);

	if (Config.Exists("Rendering/ColorBackground")) {
		int colorBackgroundR = Config.GetIntValue("Rendering/ColorBackground.r");
		int colorBackgroundG = Config.GetIntValue("Rendering/ColorBackground.g");
		int colorBackgroundB = Config.GetIntValue("Rendering/ColorBackground.b");
		gls.SetBackgroundColor(Vector3(colorBackgroundR / 255.0f, colorBackgroundG / 255.0f, colorBackgroundB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorWire")) {
		int colorWireR = Config.GetIntValue("Rendering/ColorWire.r");
		int colorWireG = Config.GetIntValue("Rendering/ColorWire.g");
		int colorWireB = Config.GetIntValue("Rendering/ColorWire.b");
		gls.SetWireColor(Vector3(colorWireR / 255.0f, colorWireG / 255.0f, colorWireB / 255.0f));
	}

	app->InitPreview();
}

void PreviewWindow::SetNormalsGenerationLayers(std::vector<NormalGenLayer>& normalLayers) {
	refNormalGenLayers = normalLayers;
	if (normalLayers.size() != 0)
		optButton->Show();
	else
		optButton->Hide();

	if (normalsGenDlg)
		normalsGenDlg->Hide();

	Layout();
}

mesh* PreviewWindow::GetMesh(const std::string& shapeName) {
	return gls.GetMesh(shapeName);
}

void PreviewWindow::AddMeshFromNif(NifFile *nif, char *shapeName) {
	std::vector<std::string> shapeList = nif->GetShapeNames();
	for (int i = 0; i < shapeList.size(); i++) {
		std::string& shapeListName = shapeList[i];
		if (!shapeName || (shapeName && shapeListName == shapeName)) {
			mesh* m = gls.AddMeshFromNif(nif, shapeListName);
			if (!m)
				continue;

			NiShape* shape = nif->FindBlockByName<NiShape>(shapeListName);
			if (shape && shape->IsSkinned()) {
				MatTransform xformGlobalToSkin;
				if (nif->CalcShapeTransformGlobalToSkin(shape, xformGlobalToSkin))
					gls.SetSkinModelMat(m, xformGlobalToSkin);
			}

			m->BuildTriAdjacency();
			m->CreateBuffers();
		}
	}
}

void PreviewWindow::RefreshMeshFromNif(NifFile* nif, char* shapeName) {
	std::vector<std::string> shapeList = nif->GetShapeNames();
	if (shapeName == nullptr)
		gls.DeleteAllMeshes();

	for (int i = 0; i < shapeList.size(); i++) {
		std::string& shapeListName = shapeList[i];
		if (!shapeName || (shapeName && shapeListName == shapeName)) {
			mesh* m = gls.ReloadMeshFromNif(nif, shapeListName);
			if (!m)
				continue;

			NiShape* shape = nif->FindBlockByName<NiShape>(shapeListName);
			if (shape && shape->IsSkinned()) {
				MatTransform xformGlobalToSkin;
				if (nif->CalcShapeTransformGlobalToSkin(shape, xformGlobalToSkin))
					gls.SetSkinModelMat(m, xformGlobalToSkin);
			}

			m->BuildTriAdjacency();
			m->SmoothNormals();
			m->CreateBuffers();

			auto iter = shapeMaterials.find(shapeListName);
			if (iter != shapeMaterials.end())
				m->material = iter->second;
			else
				AddNifShapeTextures(nif, std::string(shapeListName));
		}
	}

	gls.RenderOneFrame();
}

void PreviewWindow::AddNifShapeTextures(NifFile* fromNif, const std::string& shapeName) {
	bool hasMat = false;
	std::string matFile;

	const byte MAX_TEXTURE_PATHS = 10;
	std::vector<std::string> texFiles(MAX_TEXTURE_PATHS);

	NiShader* shader = nullptr;
	auto shape = fromNif->FindBlockByName<NiShape>(shapeName);
	if (shape) {
		shader = fromNif->GetShader(shape);
		if (shader) {
			// Find material file
			if (fromNif->GetHeader().GetVersion().User() == 12 && fromNif->GetHeader().GetVersion().Stream() >= 130) {
				matFile = shader->GetName();
				if (!matFile.empty())
					hasMat = true;
			}
		}
	}

	MaterialFile mat(MaterialFile::BGSM);
	if (hasMat) {
		matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");													// Replace all backward slashes with one forward slash
		matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");			// Remove everything before the first occurence of "/materials/"
		matFile = std::regex_replace(matFile, std::regex("^/+"), "");														// Remove all slashes from the front
		matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");	// If the path doesn't start with "materials/", add it to the front

		// Attempt to read loose material file
		mat = MaterialFile(baseDataPath + matFile);

		if (mat.Failed()) {
			// Search for material file in archives
			wxMemoryBuffer data;
			for (FSArchiveFile *archive : FSManager::archiveList()) {
				if (archive) {
					if (archive->hasFile(matFile)) {
						wxMemoryBuffer outData;
						archive->fileContents(matFile, outData);

						if (!outData.IsEmpty()) {
							data = std::move(outData);
							break;
						}
					}
				}
			}

			if (!data.IsEmpty()) {
				std::string content((char*)data.GetData(), data.GetDataLen());
				std::istringstream contentStream(content, std::istringstream::binary);

				mat = MaterialFile(contentStream);
			}
		}

		if (!mat.Failed()) {
			if (mat.signature == MaterialFile::BGSM) {
				texFiles[0] = mat.diffuseTexture.c_str();
				texFiles[1] = mat.normalTexture.c_str();
				texFiles[2] = mat.glowTexture.c_str();
				texFiles[3] = mat.greyscaleTexture.c_str();
				texFiles[4] = mat.envmapTexture.c_str();
				texFiles[7] = mat.smoothSpecTexture.c_str();
			}
			else if (mat.signature == MaterialFile::BGEM) {
				texFiles[0] = mat.baseTexture.c_str();
				texFiles[1] = mat.fxNormalTexture.c_str();
				texFiles[3] = mat.grayscaleTexture.c_str();
				texFiles[4] = mat.fxEnvmapTexture.c_str();
				texFiles[5] = mat.envmapMaskTexture.c_str();
			}
		}
		else if (shader) {
			hasMat = false;

			for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
				fromNif->GetTextureSlot(shader, texFiles[i], i);
		}
	}
	else if (shader) {
		for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
			fromNif->GetTextureSlot(shader, texFiles[i], i);
	}

	for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
		if (!texFiles[i].empty()) {
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("\\\\+"), "/");													// Replace all backward slashes with one forward slash
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("^(.*?)/textures/", std::regex_constants::icase), "");				// Remove everything before the first occurence of "/textures/"
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("^/+"), "");														// Remove all slashes from the front
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("^(?!^textures/)", std::regex_constants::icase), "textures/");		// If the path doesn't start with "textures/", add it to the front
			
			texFiles[i] = baseDataPath + texFiles[i];
		}
	}

	//texFiles[0] = "d:\\proj\\flatgrey.png";
	//texFiles[1] = "d:\\proj\\bodyPaintDummy-N_u0_v0.png";
	//texFiles[1] = "d:\\proj\\outfile.png";

	//texFiles[1] = "d:\\proj\\FemaleBody_2_msn.dds";
	//texFiles[1] = "d:\\proj\\TangentNormalsTest.png";

	std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
	std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";

	TargetGame targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		vShader = Config["AppDir"] + "/res/shaders/fo4_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/fo4_default.frag";
	}

	SetShapeTextures(shapeName, texFiles, vShader, fShader, hasMat, mat);
}

void PreviewWindow::RightDrag(int dX, int dY) {
	gls.TurnTableCamera(dX);
	gls.PitchCamera(dY);
	gls.RenderOneFrame();
}

void PreviewWindow::LeftDrag(int dX, int dY) {
	gls.PanCamera(dX, dY);
	gls.RenderOneFrame();
}

void PreviewWindow::TrackMouse(int X, int Y) {
	gls.UpdateCursor(X, Y);
	gls.RenderOneFrame();
}

void PreviewWindow::MouseWheel(int dW) {
	gls.DollyCamera(dW);
	gls.RenderOneFrame();
}

void PreviewWindow::OnWeightSlider(wxScrollEvent& event) {
	weight = event.GetPosition();
	app->UpdatePreview();
}

void PreviewWindow::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = GetPosition();
	BodySlideConfig.SetValue("PreviewFrame.x", p.x);
	BodySlideConfig.SetValue("PreviewFrame.y", p.y);
	event.Skip();
}

void PreviewWindow::OnSetSize(wxSizeEvent& event) {
	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		BodySlideConfig.SetValue("PreviewFrame.width", p.x);
		BodySlideConfig.SetValue("PreviewFrame.height", p.y);
	}

	BodySlideConfig.SetBoolValue("PreviewFrame.maximized", maximized);
	event.Skip();
}

void PreviewWindow::ShowNormalGenWindow(wxCommandEvent & WXUNUSED(event)) {
	if (!normalsGenDlg)
		normalsGenDlg = new NormalsGenDialog(this, refNormalGenLayers);
	else
		normalsGenDlg->SetLayersRef(refNormalGenLayers);

	normalsGenDlg->Show();
}

void PreviewWindow::Cleanup() {
	// Set current context for resource deletion
	if (canvas && context)
		canvas->SetCurrent(*context);

	gls.Cleanup();
	shapeMaterials.clear();
	gls.RenderOneFrame();
}

void PreviewWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
	Cleanup();
	Destroy();
	//delete offscreen;
	app->PreviewClosed();
	wxLogMessage("Preview closed.");
}

wxBEGIN_EVENT_TABLE(PreviewCanvas, wxGLCanvas)
	EVT_KEY_UP(PreviewCanvas::OnKeyUp)
	EVT_MOTION(PreviewCanvas::OnMotion)
	EVT_MOUSEWHEEL(PreviewCanvas::OnMouseWheel)
	EVT_PAINT(PreviewCanvas::OnPaint)
	EVT_SIZE(PreviewCanvas::OnResized)
wxEND_EVENT_TABLE()

PreviewCanvas::PreviewCanvas(PreviewWindow* pw, const wxGLAttributes& attribs)
	: wxGLCanvas(pw, attribs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
		previewWindow(pw) {
}

void PreviewCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		previewWindow->OnShown();
	}
	previewWindow->Render();
}

void PreviewCanvas::OnKeyUp(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	switch (key) {
	case 'T':
		previewWindow->ToggleTextures();
		break;
	case 'W':
		previewWindow->ToggleWireframe();
		break;
	case 'L':
		previewWindow->ToggleLighting();
		break;
	case 'G':
		//previewWindow->RenderNormalMap();
		break;
	}
}

void PreviewCanvas::OnMotion(wxMouseEvent& event) {
	if (previewWindow->IsActive())
		SetFocus();

	auto delta = event.GetPosition() - lastMousePosition;

	if (event.LeftIsDown()) {
		previewWindow->LeftDrag(delta.x, delta.y);
	}
	else if (event.MiddleIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			previewWindow->MouseWheel(delta.y);
		else
			previewWindow->LeftDrag(delta.x, delta.y);
	}
	else if (event.RightIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			previewWindow->LeftDrag(delta.x, delta.y);
		else
			previewWindow->RightDrag(delta.x, delta.y);
	}
	else
		previewWindow->TrackMouse(event.GetX(), event.GetY());

	lastMousePosition = event.GetPosition();
}

void PreviewCanvas::OnMouseWheel(wxMouseEvent& event) {
	previewWindow->MouseWheel(event.GetWheelRotation());
}

void PreviewCanvas::OnResized(wxSizeEvent& event) {
	previewWindow->Resized(event.GetSize().GetWidth(), event.GetSize().GetHeight());
}
