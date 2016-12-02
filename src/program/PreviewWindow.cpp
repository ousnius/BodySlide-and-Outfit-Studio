/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "PreviewWindow.h"
#include "BodySlideApp.h"

#include <sstream>

wxBEGIN_EVENT_TABLE(PreviewWindow, wxFrame)
	EVT_CLOSE(PreviewWindow::OnClose)
	EVT_COMMAND_SCROLL(wxID_ANY, PreviewWindow::OnWeightSlider)
wxEND_EVENT_TABLE()

PreviewWindow::~PreviewWindow() {
}

wxSize PreviewWindow::GetDefaultSize() {
	int xborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_X);
	if (xborder < 0)
		xborder = 0;
	int yborder = wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_Y);
	if (yborder < 0)
		yborder = 0;
	return wxSize(720 + xborder * 2, 720 + yborder * 2);
}

PreviewWindow::PreviewWindow(BodySlideApp* a)
	: wxFrame(nullptr, wxID_ANY, "Preview", wxDefaultPosition, GetDefaultSize()), app(a) {
	SetIcon(wxIcon("res\\images\\OutfitStudio.png", wxBITMAP_TYPE_PNG));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerPanel = new wxBoxSizer(wxHORIZONTAL);

	wxPanel* uiPanel = new wxPanel(this);
	wxSlider* weightSlider = new wxSlider(uiPanel, wxID_ANY, 100, 0, 100,
		wxDefaultPosition, wxDefaultSize, wxSL_LABELS, wxDefaultValidator, "weightSlider");

	uiPanel->SetBackgroundColour(wxColour(210, 210, 210));

	canvas = new PreviewCanvas(this, GLSurface::GetGLAttribs());
	context = new wxGLContext(canvas, nullptr, &GLSurface::GetGLContextAttribs());
	
	sizerPanel->Add(weightSlider, 1, wxTOP | wxLEFT | wxRIGHT, 10);
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
		delete context;
		app->PreviewClosed();
		wxLogError("Preview failed: OpenGL context is not OK.");
		wxMessageBox(_("Preview failed: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR);
		return;
	}

	wxLogMessage("Initializing preview window...");
	gls.Initialize(canvas, context);
	auto size = canvas->GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight());

	int ambient = Config.GetIntValue("Lights/Ambient");
	int brightness1 = Config.GetIntValue("Lights/Brightness1");
	int brightness2 = Config.GetIntValue("Lights/Brightness2");
	int brightness3 = Config.GetIntValue("Lights/Brightness3");
	gls.UpdateLights(ambient, brightness1, brightness2, brightness3);

	app->InitPreview();
}

void PreviewWindow::AddMeshFromNif(NifFile *nif, char *shapeName) {
	vector<string> shapeList;
	nif->GetShapeList(shapeList);
	mesh* m = nullptr;
	for (int i = 0; i < shapeList.size(); i++) {
		if (shapeName && (shapeList[i] == shapeName)) {
			gls.AddMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeName);
			if (m) {
				m->BuildTriAdjacency();
				m->SmoothNormals();
				m->CreateBuffers();
			}
		}
		else if (shapeName) {
			continue;
		}
		else {
			gls.AddMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeList[i]);
			if (m) {
				m->BuildTriAdjacency();
				m->SmoothNormals();
				m->CreateBuffers();
			}
		}
	}
}

void PreviewWindow::RefreshMeshFromNif(NifFile* nif, char* shapeName) {
	vector<string> shapeList;
	nif->GetShapeList(shapeList);
	mesh* m = nullptr;
	if (shapeName == nullptr)
		gls.DeleteAllMeshes();

	for (int i = 0; i < shapeList.size(); i++) {
		if (shapeName && (shapeList[i] == shapeName)) {
			gls.ReloadMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeName);
			m->BuildTriAdjacency();
			m->SmoothNormals();
			m->CreateBuffers();

			auto iter = shapeMaterials.find(shapeName);
			if (iter != shapeMaterials.end())
				m->material = iter->second;
			else
				AddNifShapeTexture(nif, string(shapeName));

		}
		else if (shapeName) {
			continue;
		}
		else {
			gls.ReloadMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeList[i]);
			m->BuildTriAdjacency();
			m->SmoothNormals();
			m->CreateBuffers();

			auto iter = shapeMaterials.find(shapeList[i]);
			if (iter != shapeMaterials.end())
				m->material = iter->second;
			else
				AddNifShapeTexture(nif, shapeList[i]);
		}
	}

	gls.RenderOneFrame();
}

void PreviewWindow::AddNifShapeTexture(NifFile* fromNif, const string& shapeName) {
	bool isSkin = false;
	bool hasMat = false;
	wxString matFile;
	string texFile;

	NiShader* shader = fromNif->GetShader(shapeName);
	if (shader) {
		if (shader->IsSkinTint())
			isSkin = true;

		// Find material file
		if (shader->header->GetUserVersion() == 12 && shader->header->GetUserVersion2() >= 130) {
			matFile = shader->GetName();
			if (!matFile.IsEmpty())
				hasMat = true;
		}
	}

	if (hasMat) {
		matFile = matFile.Lower();
		matFile.Replace("\\", "/");

		// Attempt to read loose material file
		MaterialFile mat(MaterialFile::BGSM);
		mat = MaterialFile(baseDataPath + matFile.ToStdString());

		if (mat.Failed()) {
			// Search for material file in archives
			wxMemoryBuffer data;
			for (FSArchiveFile *archive : FSManager::archiveList()) {
				if (archive) {
					if (archive->hasFile(matFile.ToStdString())) {
						wxMemoryBuffer outData;
						archive->fileContents(matFile.ToStdString(), outData);

						if (!outData.IsEmpty()) {
							data = outData;
							break;
						}
					}
				}
			}

			if (!data.IsEmpty()) {
				string content((char*)data.GetData(), data.GetDataLen());
				istringstream contentStream(content, istringstream::binary);

				mat = MaterialFile(contentStream);
			}
		}

		if (!mat.Failed()) {
			if (mat.signature == MaterialFile::BGSM)
				texFile = mat.diffuseTexture;
			else if (mat.signature == MaterialFile::BGEM)
				texFile = mat.baseTexture;
		}
		else
			fromNif->GetTextureForShape(shapeName, texFile);
	}
	else
		fromNif->GetTextureForShape(shapeName, texFile);

	if (!texFile.empty()) {
		texFile = regex_replace(texFile, regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
		texFile = regex_replace(texFile, regex("^\\\\+", regex_constants::icase), ""); // Remove all backslashes from the front
		texFile = regex_replace(texFile, regex(".*?Data\\\\", regex_constants::icase), ""); // Remove everything before and including the data path root
		texFile = regex_replace(texFile, regex("^(?!^textures\\\\)", regex_constants::icase), "textures\\"); // Add textures root path if not existing}
	}

	string combinedTexFile = baseDataPath + texFile;
	SetShapeTexture(shapeName, combinedTexFile.c_str(), isSkin);
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

void PreviewWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
	Cleanup();
	Destroy();
	canvas = nullptr;
	delete context;
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
	case 'N':
		previewWindow->ToggleSmoothSeams();
		break;
	case 'T':
		previewWindow->ToggleTextures();
		break;
	case 'W':
		previewWindow->ToggleWireframe();
		break;
	case 'L':
		previewWindow->ToggleLighting();
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
