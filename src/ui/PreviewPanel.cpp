/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PreviewPanel.h"
#include "../files/SFMaterialDatabase.h"
#include "../files/SFMaterialFile.h"
#include "../program/BodySlideApp.h"
#include "../utils/PlatformUtil.h"

#include <regex>
#include <sstream>

using namespace nifly;

extern ConfigurationManager Config;
extern ConfigurationManager BodySlideConfig;

wxDEFINE_EVENT(EVT_PREVIEW_POPOUT, wxCommandEvent);

wxBEGIN_EVENT_TABLE(PreviewPanel, wxPanel)
	EVT_COMMAND_SCROLL(wxID_ANY, PreviewPanel::OnWeightSlider)
wxEND_EVENT_TABLE()

PreviewPanel::~PreviewPanel() {}

PreviewPanel::PreviewPanel(wxWindow* parent, BodySlideApp* app)
	: wxPanel(parent, wxID_ANY)
	, app(app)
	, refNormalGenLayers(emptyLayers) {

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizerPanel = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizerProjectSelect = new wxBoxSizer(wxHORIZONTAL);

	wxPanel* projectSelectPanel = new wxPanel(this);
	projectLabel = new wxStaticText(projectSelectPanel, wxID_ANY, _("Outfit/Body"));
	projectChoice = new wxChoice(projectSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxArrayString(), 0, wxDefaultValidator, "projectChoice");
	projectChoice->Hide();
	projectLabel->Hide();

	presetLabel = new wxStaticText(projectSelectPanel, wxID_ANY, _("Preset"));
	presetChoice = new wxChoice(projectSelectPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxArrayString(), 0, wxDefaultValidator, "presetChoice");
	presetChoice->Hide();
	presetLabel->Hide();

	sizerProjectSelect->Add(projectLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	sizerProjectSelect->Add(projectChoice, 1, wxALL | wxEXPAND, 5);
	sizerProjectSelect->Add(presetLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	sizerProjectSelect->Add(presetChoice, 1, wxALL | wxEXPAND, 5);
	projectSelectPanel->SetSizer(sizerProjectSelect);
	projectSelectPanel->SetBackgroundColour(wxColour(210, 210, 210));

	wxPanel* uiPanel = new wxPanel(this);
	weightSlider = new wxSlider(uiPanel, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS, wxDefaultValidator, "weightSlider");

	optButton = new wxButton(uiPanel, wxID_ANY, "N", wxDefaultPosition, FromDIP(wxSize(25, 25)));
	optButton->SetToolTip(_("Show the Normal Map Generator dialog."));
	optButton->Bind(wxEVT_BUTTON, &PreviewPanel::ShowNormalGenWindow, this);
	optButton->Hide();

	lockShapeButton = new wxButton(uiPanel, wxID_ANY, _("Lock Shape"), wxDefaultPosition, wxDefaultSize);
	lockShapeButton->SetToolTip(_("Set the current preview shape to both low and high weight sliders."));
	lockShapeButton->Bind(wxEVT_BUTTON, &PreviewPanel::OnLockShape, this);
	lockShapeButton->Hide();

	showReferenceCheckbox = new wxCheckBox(uiPanel, wxID_ANY, _("Show Reference"), wxDefaultPosition, wxDefaultSize);
	showReferenceCheckbox->SetToolTip(_("Show the reference shape from the source project for clipping preview."));
	showReferenceCheckbox->Bind(wxEVT_CHECKBOX, &PreviewPanel::OnShowReference, this);
	showReferenceCheckbox->Hide();

	uiPanel->SetBackgroundColour(wxColour(210, 210, 210));

	// Pop-out button (placed in uiPanel, below Lock Shape)
	popoutButton = new wxBitmapButton(
		uiPanel,
		wxID_ANY,
		wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/PopOut.png", wxBITMAP_TYPE_PNG),
		wxDefaultPosition,
		FromDIP(wxSize(28, 28)));
	popoutButton->SetMinSize(FromDIP(wxSize(28, 28)));
	SetPopoutButtonDetachedState(false);
	popoutButton->Bind(wxEVT_BUTTON, &PreviewPanel::OnPopout, this);

	canvas = new PreviewCanvas(this, GLSurface::GetGLAttribs());
	context = std::make_unique<wxGLContext>(canvas, nullptr, &GLSurface::GetGLContextAttribs());

	sizerPanel->Add(weightSlider, 1, wxTOP | wxLEFT | wxRIGHT, 10);

	wxBoxSizer* sizerRight = new wxBoxSizer(wxVERTICAL);
	sizerRight->Add(showReferenceCheckbox, 0, wxALIGN_CENTER_HORIZONTAL);

	wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
	sizerButtons->Add(lockShapeButton, 0, wxALIGN_CENTER_VERTICAL);
	sizerButtons->Add(popoutButton, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 4);
	sizerRight->Add(sizerButtons, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 2);

	sizerPanel->Add(sizerRight, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	sizerPanel->Add(optButton, 0, wxALL | wxALIGN_BOTTOM, 10);
	uiPanel->SetSizer(sizerPanel);

	// Loading overlay (hidden by default, positioned over canvas)
	loadingOverlay = new wxPanel(this, wxID_ANY);
	loadingOverlay->SetBackgroundColour(wxColour(64, 64, 64));
	wxBoxSizer* loadingSizer = new wxBoxSizer(wxVERTICAL);
	loadingSizer->AddStretchSpacer();
	loadingIndicator = new wxActivityIndicator(loadingOverlay);
	loadingSizer->Add(loadingIndicator, 0, wxALIGN_CENTER_HORIZONTAL);
	wxStaticText* loadingText = new wxStaticText(loadingOverlay, wxID_ANY, _("Loading preview..."));
	loadingText->SetForegroundColour(*wxWHITE);
	loadingSizer->Add(loadingText, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);
	loadingSizer->AddStretchSpacer();
	loadingOverlay->SetSizer(loadingSizer);
	loadingOverlay->Hide();

	sizer->Add(projectSelectPanel, 0, wxEXPAND);
	sizer->Add(uiPanel, 0, wxEXPAND);
	sizer->Add(canvas, 1, wxEXPAND);

	SetSizer(sizer);

	// Reposition floating overlay when canvas resizes
	canvas->Bind(wxEVT_SIZE, [this](wxSizeEvent& evt) {
		if (loadingOverlay && loadingOverlay->IsShown()) {
			loadingOverlay->SetSize(canvas->GetRect());
		}
		evt.Skip();
	});

	projectChoice->Bind(wxEVT_CHOICE, &PreviewPanel::OnProjectChoice, this);
	presetChoice->Bind(wxEVT_CHOICE, &PreviewPanel::OnPresetChoice, this);
}

void PreviewPanel::ShowLoadingIndicator(bool show) {
	if (show) {
		// Position overlay on top of the canvas (canvas stays shown for GL ops)
		loadingOverlay->SetSize(canvas->GetRect());
		loadingOverlay->Show();
		loadingOverlay->Raise();
		loadingIndicator->Start();
	}
	else {
		loadingIndicator->Stop();
		loadingOverlay->Hide();
		canvas->Refresh();
	}
}

void PreviewPanel::OnShown() {
	if (glInitialized)
		return;

	if (!context->IsOK()) {
		canvas = nullptr;
		app->PreviewClosed();
		wxLogError("Preview failed: OpenGL context is not OK.");
		wxMessageBox(_("Preview failed: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR);
		return;
	}

	wxLogMessage("Initializing preview panel...");
	gls.Initialize(canvas, context.get());
	auto size = canvas->GetClientSize();
	int vpW = size.GetWidth() > 0 ? size.GetWidth() : 800;
	int vpH = size.GetHeight() > 0 ? size.GetHeight() : 600;
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), vpW, vpH, 65.0);
	gls.SetVertexColors(true);

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
		int colorR = Config.GetIntValue("Rendering/ColorBackground.r");
		int colorG = Config.GetIntValue("Rendering/ColorBackground.g");
		int colorB = Config.GetIntValue("Rendering/ColorBackground.b");
		gls.SetBackgroundColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorWire")) {
		int colorR = Config.GetIntValue("Rendering/ColorWire.r");
		int colorG = Config.GetIntValue("Rendering/ColorWire.g");
		int colorB = Config.GetIntValue("Rendering/ColorWire.b");
		gls.SetWireColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorPoints")) {
		int colorR = Config.GetIntValue("Rendering/ColorPoints.r");
		int colorG = Config.GetIntValue("Rendering/ColorPoints.g");
		int colorB = Config.GetIntValue("Rendering/ColorPoints.b");
		gls.SetPointColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorPointsMasked")) {
		int colorR = Config.GetIntValue("Rendering/ColorPointsMasked.r");
		int colorG = Config.GetIntValue("Rendering/ColorPointsMasked.g");
		int colorB = Config.GetIntValue("Rendering/ColorPointsMasked.b");
		gls.SetMaskedPointColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	gls.SetPointSizeParams(
		Config.GetFloatValue("Rendering/PointSizeMin", 4.0f),
		Config.GetFloatValue("Rendering/PointSizeMax", 14.0f),
		Config.GetFloatValue("Rendering/PointSizeScale", 0.6f));

	gls.SetPerspective(BodySlideConfig.GetBoolValue("Rendering/PerspectiveView", true));

	glInitialized = true;

	// Load deferred project entries
	if (!projectEntries.empty()) {
		if (loadAllProjects) {
			LoadProjects(projectEntries);
			loadAllProjects = false;
		}
		else {
			LoadProjects({projectEntries[0]});
		}
		return;
	}

	// Load deferred nifs if in NIF-only preview mode
	if (!extraNifPaths.empty()) {
		wxLogMessage("Loading %zu nif file(s) in preview mode...", extraNifPaths.size());
		ShowWeight(false);
		LoadNifFiles(extraNifPaths);
		gls.RenderOneFrame();
		return;
	}

	app->InitPreview();
}

void PreviewPanel::LoadNifFiles(const std::vector<std::string>& nifFilePaths) {
	for (const auto& nifPath : nifFilePaths) {
		std::fstream file;
		PlatformUtil::OpenFileStream(file, nifPath, std::ios::in | std::ios::binary);

		NifFile* nifFile = new NifFile();
		if (nifFile->Load(file)) {
			wxLogWarning("Failed to load nif file: %s", nifPath);
			delete nifFile;
			continue;
		}

		wxLogMessage("Loading nif: %s", nifPath);
		AddMeshFromNif(nifFile);

		for (auto& s : nifFile->GetShapeNames())
			AddNifShapeTextures(nifFile, s);

		delete nifFile;
	}
}

void PreviewPanel::SetProjectData(const std::vector<PreviewProjectEntry>& entries, bool loadAll, const std::string& initialPreset) {
	projectEntries = entries;
	loadAllProjects = loadAll;
	initialPresetName = initialPreset;

	if (!loadAll && projectChoice) {
		projectChoice->Clear();
		for (auto& entry : projectEntries)
			projectChoice->Append(wxString::FromUTF8(entry.setName));

		if (!projectEntries.empty())
			projectChoice->SetSelection(0);

		projectChoice->Show(projectEntries.size() > 1);
		projectLabel->Show(projectEntries.size() > 1);

		projectChoice->GetParent()->Show();
		Layout();
	}
}

void PreviewPanel::LoadProjects(const std::vector<PreviewProjectEntry>& entries) {
	wxLogMessage("Loading %zu project(s) in preview mode...", entries.size());

	app->CleanupPreview();

	for (auto& entry : entries) {
		if (app->AddProjectSliders(entry.projectFile, entry.setName)) {
			wxLogError("Failed to load slider set '%s' from file: %s", entry.setName, entry.projectFile);
			continue;
		}
	}

	if (entries.empty())
		return;

	app->LoadPresets(entries[0].setName);

	if (presetChoice) {
		wxString previousPreset = initialPresetName.empty() ? presetChoice->GetStringSelection() : wxString::FromUTF8(initialPresetName);
		presetChoice->Clear();

		std::vector<std::string> presetNames;
		app->GetPresetNames(presetNames);

		if (!presetNames.empty()) {
			for (auto& name : presetNames)
				presetChoice->Append(wxString::FromUTF8(name));

			int idx = presetChoice->FindString(previousPreset);
			if (idx == wxNOT_FOUND)
				idx = 0;

			presetChoice->SetSelection(idx);
			presetChoice->Show();
			presetLabel->Show();

			app->InitializeSliders(presetNames[idx]);
			initialPresetName.clear();
		}
		else {
			presetChoice->Hide();
			presetLabel->Hide();

			app->InitializeSliders();
		}

		presetChoice->GetParent()->Show();
		Layout();
	}

	multiProjectMode = true;

	app->InitPreview();
}

void PreviewPanel::OnProjectChoice(wxCommandEvent& WXUNUSED(event)) {
	if (!projectChoice)
		return;

	int selection = projectChoice->GetSelection();
	if (selection < 0 || selection >= (int)projectEntries.size())
		return;

	LoadProjects({projectEntries[selection]});
}

void PreviewPanel::OnPresetChoice(wxCommandEvent& WXUNUSED(event)) {
	if (!presetChoice)
		return;

	int selection = presetChoice->GetSelection();
	if (selection < 0)
		return;

	std::string presetName = presetChoice->GetStringSelection().ToUTF8().data();
	wxLogMessage("Applying preset '%s' to preview...", presetName);

	app->InitializeSliders(presetName);
	app->RebuildPreviewMeshes();
}

void PreviewPanel::SetNormalsGenerationLayers(std::vector<NormalGenLayer>& normalLayers) {
	refNormalGenLayers = normalLayers;
	if (normalLayers.size() != 0)
		optButton->Show();
	else
		optButton->Hide();

	if (normalsGenDlg)
		normalsGenDlg->Hide();

	Layout();
}

Mesh* PreviewPanel::GetMesh(const std::string& shapeName) {
	return gls.GetMesh(shapeName);
}

void PreviewPanel::AddMeshFromNif(NifFile* nif, char* shapeName) {
	if (!glInitialized || !gls.SetContext())
		return;

	std::vector<std::string> shapeList = nif->GetShapeNames();
	for (size_t i = 0; i < shapeList.size(); i++) {
		std::string& shapeListName = shapeList[i];
		if (!shapeName || (shapeName && shapeListName == shapeName)) {
			Mesh* m = gls.AddMeshFromNif(nif, shapeListName);
			if (!m)
				continue;

			SetShapeVertexColors(nif, shapeListName, m);
			m->BuildVertexAdjacency();
			m->CreateBuffers();
		}
	}
}

void PreviewPanel::RefreshMeshFromNif(const std::vector<NifFile*>& nifs) {
	if (!glInitialized || !gls.SetContext())
		return;

	gls.ClearMeshes();

	for (auto* nif : nifs) {
		for (auto& shapeListName : nif->GetShapeNames()) {
			Mesh* m = gls.AddMeshFromNif(nif, shapeListName);
			if (!m)
				continue;

			SetShapeVertexColors(nif, shapeListName, m);
			m->BuildVertexAdjacency();
			m->SmoothNormals();
			m->CreateBuffers();

			auto iter = shapeMaterials.find(shapeListName);
			if (iter != shapeMaterials.end())
				m->material = iter->second;
			else
				AddNifShapeTextures(nif, shapeListName);
		}
	}

	LoadNifFiles(extraNifPaths);

	gls.RenderOneFrame();
}

SFMaterialDatabase* PreviewPanel::GetSFMaterialDatabase() {
	if (sfMaterialDb)
		return sfMaterialDb->Failed() ? nullptr : sfMaterialDb.get();

	sfMaterialDb = std::make_unique<SFMaterialDatabase>();

	wxMemoryBuffer data;
	for (FSArchiveFile* archive : FSManager::archiveList()) {
		if (archive && archive->hasFile("materials/materialsbeta.cdb")) {
			wxMemoryBuffer outData;
			archive->fileContents("materials/materialsbeta.cdb", outData);
			if (!outData.IsEmpty()) {
				data = std::move(outData);
				break;
			}
		}
	}

	if (data.IsEmpty())
		return nullptr;

	sfMaterialDbContent.assign(static_cast<const char*>(data.GetData()), data.GetDataLen());
	sfMaterialDbStream = std::make_unique<std::istringstream>(sfMaterialDbContent, std::ios::in | std::ios::binary);

	if (!sfMaterialDb->Load(*sfMaterialDbStream) || sfMaterialDb->Failed()) {
		sfMaterialDbContent.clear();
		sfMaterialDbStream.reset();
		return nullptr;
	}

	return sfMaterialDb.get();
}

void PreviewPanel::AddNifShapeTextures(NifFile* fromNif, const std::string& shapeName) {
	bool hasMat = false;
	bool hasSFMat = false;
	std::string matFile;

	const uint8_t MAX_TEXTURE_PATHS = 10;
	std::vector<std::string> texFiles(MAX_TEXTURE_PATHS);

	NiShader* shader = nullptr;
	auto shape = fromNif->FindBlockByName<NiShape>(shapeName);
	if (shape) {
		shader = fromNif->GetShader(shape);
		if (shader) {
			if (fromNif->GetHeader().GetVersion().IsFO4() || fromNif->GetHeader().GetVersion().IsFO76()) {
				matFile = shader->name.get();
				if (!matFile.empty())
					hasMat = true;
			}
			else if (fromNif->GetHeader().GetVersion().IsSF()) {
				matFile = shader->name.get();
				if (!matFile.empty()) {
					hasMat = true;
					hasSFMat = true;
				}
			}
		}
	}

	MaterialFile mat(MaterialFile::BGSM);
	if (hasMat) {
		matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");
		matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");
		matFile = std::regex_replace(matFile, std::regex("^/+"), "");
		matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");

		if (hasSFMat) {
			if (!std::regex_search(matFile, std::regex("\\.mat$", std::regex_constants::icase)))
				matFile += ".mat";

			SFMaterialFile sfMat(baseDataPath + matFile);
			if (!sfMat.Failed()) {
				texFiles = sfMat.GetTextureFiles(MAX_TEXTURE_PATHS);
			}
			else {
				bool resolvedFromArchive = false;
				for (FSArchiveFile* archive : FSManager::archiveList()) {
					if (archive && archive->hasFile(matFile)) {
						wxMemoryBuffer outData;
						archive->fileContents(matFile, outData);
						if (!outData.IsEmpty()) {
							std::string content(static_cast<const char*>(outData.GetData()), outData.GetDataLen());
							std::istringstream contentStream(content, std::istringstream::binary);
							SFMaterialFile archiveMat(contentStream);
							if (!archiveMat.Failed()) {
								texFiles = archiveMat.GetTextureFiles(MAX_TEXTURE_PATHS);
								resolvedFromArchive = true;
							}
							break;
						}
					}
				}

				if (!resolvedFromArchive) {
					std::string materialJson;
					SFMaterialDatabase* cdb = GetSFMaterialDatabase();
					if (cdb && cdb->GetMaterialJSON(matFile, materialJson)) {
						std::istringstream materialStream(materialJson);
						SFMaterialFile cdbMat(materialStream);
						if (!cdbMat.Failed())
							texFiles = cdbMat.GetTextureFiles(MAX_TEXTURE_PATHS);
					}
				}

				bool hasAnyTex = false;
				for (int i = 0; i < MAX_TEXTURE_PATHS && !hasAnyTex; i++)
					hasAnyTex = !texFiles[i].empty();

				if (!hasAnyTex && shader) {
					for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
						fromNif->GetTextureSlot(shape, texFiles[i], i);
				}
			}

			hasMat = false;
		}
		else {
			mat = MaterialFile(baseDataPath + matFile);

			if (mat.Failed()) {
				wxMemoryBuffer data;
				for (FSArchiveFile* archive : FSManager::archiveList()) {
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
					fromNif->GetTextureSlot(shape, texFiles[i], i);
			}
		}
	}
	else if (shader) {
		for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
			fromNif->GetTextureSlot(shape, texFiles[i], i);
	}

	for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
		if (!texFiles[i].empty()) {
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("\\\\+"), "/");
			texFiles[i] = std::regex_replace(texFiles[i],
											 std::regex("^(.*?)/textures/", std::regex_constants::icase),
											 "");
			texFiles[i] = std::regex_replace(texFiles[i], std::regex("^/+"), "");
			texFiles[i] = std::regex_replace(texFiles[i],
											 std::regex("^(?!^textures/)", std::regex_constants::icase),
											 "textures/");

			texFiles[i] = baseDataPath + texFiles[i];
		}
	}

	std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
	std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";

	TargetGame targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		vShader = Config["AppDir"] + "/res/shaders/fo4_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/fo4_default.frag";
	}
	else if (targetGame == SF) {
		vShader = Config["AppDir"] + "/res/shaders/sf_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/sf_default.frag";
	}
	else if (targetGame == OB) {
		vShader = Config["AppDir"] + "/res/shaders/ob_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/ob_default.frag";
	}

	SetShapeTextures(shapeName, texFiles, vShader, fShader, hasMat, mat);
}

void PreviewPanel::RenderNormalMap(const std::string& outfilename) {
	wxBusyCursor busycursor;

	std::string dest_tex = gls.GetMesh("CBBE")->material->GetTexName(1);
	GLuint w, h;
	gls.GetSize(w, h);
	std::vector<GLuint> texIds;
	std::vector<std::string> normTextures;
	normTextures.resize(20);

	GLMaterial* normMat = gls.AddMaterial(normTextures, Config["AppDir"] + "/res/shaders/normalshade.vert", Config["AppDir"] + "/res/shaders/normalshade.frag");

	std::vector<std::string> ppTex;
	ppTex.push_back("pproc");
	GLMaterial* ppMat = gls.AddMaterial(ppTex, Config["AppDir"] + "/res/shaders/fullscreentri.vert", Config["AppDir"] + "/res/shaders/fullscreentri.frag");

	GLOffScreenBuffer offscreen(&gls, 4096, 4096, 2, texIds);

	gls.SetPerspective(false);
	offscreen.Start();
	gls.RenderToTexture(normMat);
	gls.GetResourceLoader()->RenameTexture(offscreen.texName(0), "pproc", true);
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
	gls.SetSize(w, h);
	Render();
}

void PreviewPanel::RightDrag(int dX, int dY) {
	gls.TurnTableCamera(dX);
	gls.PitchCamera(dY);
	gls.RenderOneFrame();
}

void PreviewPanel::LeftDrag(int dX, int dY) {
	gls.PanCamera(dX, dY);
	gls.RenderOneFrame();
}

void PreviewPanel::TrackMouse(int X, int Y) {
	gls.UpdateCursor(X, Y);
	gls.RenderOneFrame();
}

void PreviewPanel::MouseWheel(int dW) {
	gls.DollyCamera(dW);
	gls.RenderOneFrame();
}

void PreviewPanel::OnWeightSlider(wxScrollEvent& event) {
	weight = event.GetPosition();
	app->UpdatePreview();
}

void PreviewPanel::OnLockShape(wxCommandEvent& WXUNUSED(event)) {
	app->CopyPreviewWeightToSliders();
}

void PreviewPanel::OnShowReference(wxCommandEvent& WXUNUSED(event)) {
	app->UpdatePreview();
}

void PreviewPanel::OnPopout(wxCommandEvent& WXUNUSED(event)) {
	wxCommandEvent evt(EVT_PREVIEW_POPOUT);
	wxPostEvent(GetParent(), evt);
}

void PreviewPanel::ShowPopoutButton(bool show) {
	if (popoutButton)
		popoutButton->Show(show);
}

void PreviewPanel::SetPopoutButtonDetachedState(bool detached) {
	if (!popoutButton)
		return;

	const wxString imageName = detached ? "PopIn.png" : "PopOut.png";
	wxBitmap bitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/" + imageName, wxBITMAP_TYPE_PNG);
	if (bitmap.IsOk())
		popoutButton->SetBitmap(bitmap);

	if (detached)
		popoutButton->SetToolTip(_("Pop preview back into the main window"));
	else
		popoutButton->SetToolTip(_("Pop out preview into a separate window"));
}

void PreviewPanel::ShowNormalGenWindow(wxCommandEvent& WXUNUSED(event)) {
	if (!normalsGenDlg)
		normalsGenDlg = new NormalsGenDialog(this, refNormalGenLayers);
	else
		normalsGenDlg->SetLayersRef(refNormalGenLayers);

	normalsGenDlg->Show();
}

void PreviewPanel::Cleanup() {
	if (canvas && context)
		canvas->SetCurrent(*context);

	gls.Cleanup();
	shapeMaterials.clear();
	gls.RenderOneFrame();
}

wxBEGIN_EVENT_TABLE(PreviewCanvas, wxGLCanvas)
	EVT_KEY_UP(PreviewCanvas::OnKeyUp)
	EVT_MOTION(PreviewCanvas::OnMotion)
	EVT_MOUSEWHEEL(PreviewCanvas::OnMouseWheel)
	EVT_PAINT(PreviewCanvas::OnPaint)
	EVT_SIZE(PreviewCanvas::OnResized)
wxEND_EVENT_TABLE()

PreviewCanvas::PreviewCanvas(PreviewPanel* pp, const wxGLAttributes& attribs)
	: wxGLCanvas(pp, attribs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
	, previewPanel(pp) {}

void PreviewCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	// Initialize OpenGL the first time the window is painted.
	if (firstPaint) {
		firstPaint = false;
		previewPanel->OnShown();
	}
	previewPanel->Render();
}

void PreviewCanvas::OnKeyUp(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	switch (key) {
		case 'T': previewPanel->ToggleTextures(); break;
		case 'W': previewPanel->ToggleWireframe(); break;
		case 'L': previewPanel->ToggleLighting(); break;
		case 'G':
			break;
	}
}

void PreviewCanvas::OnMotion(wxMouseEvent& event) {
	if (!HasFocus()) {
		auto topLevel = dynamic_cast<wxTopLevelWindow*>(wxGetTopLevelParent(previewPanel));
		if (topLevel && topLevel->IsActive())
			SetFocus();
	}

	auto delta = event.GetPosition() - lastMousePosition;

	if (event.LeftIsDown()) {
		previewPanel->LeftDrag(delta.x, delta.y);
	}
	else if (event.MiddleIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			previewPanel->MouseWheel(delta.y);
		else
			previewPanel->LeftDrag(delta.x, delta.y);
	}
	else if (event.RightIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			previewPanel->LeftDrag(delta.x, delta.y);
		else
			previewPanel->RightDrag(delta.x, delta.y);
	}
	else
		previewPanel->TrackMouse(event.GetX(), event.GetY());

	lastMousePosition = event.GetPosition();
}

void PreviewCanvas::OnMouseWheel(wxMouseEvent& event) {
	previewPanel->MouseWheel(event.GetWheelRotation());
}

void PreviewCanvas::OnResized(wxSizeEvent& event) {
	previewPanel->Resized(event.GetSize().GetWidth(), event.GetSize().GetHeight());
}
