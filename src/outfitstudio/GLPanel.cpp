/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLPanel.h"
#include "OutfitStudioFrame.h"
#include "ui/outfitstudio/wxBrushSettingsPopup.h"
#include "OutfitStudioApp.h"
#include "ShapeTreeItems.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "physics/Controller.h"
#include "components/TweakBrush.h"
#include "components/UndoHistory.h"
#include "render/GLSurface.h"
#include "utils/TargetGame.h"
#include "mcp/OutfitStudioMcp.h"
#include "components/MaskOps.h"
#include "files/HkxFile.h"
#include "files/SFMorphFile.h"
#include "files/GameDataStream.h"
#include "ui/outfitstudio/EditUV.h"
#include "project/BrushStrokeController.h"
#include "ui/outfitstudio/AutomationDialog.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;
wxBEGIN_EVENT_TABLE(wxGLPanel, wxGLCanvas)
	EVT_PAINT(wxGLPanel::OnPaint)
	EVT_SIZE(wxGLPanel::OnSize)
	EVT_MOUSEWHEEL(wxGLPanel::OnMouseWheel)
	EVT_MOTION(wxGLPanel::OnMouseMove)
	EVT_LEFT_DOWN(wxGLPanel::OnLeftDown)
	EVT_LEFT_DCLICK(wxGLPanel::OnLeftDown)
	EVT_LEFT_UP(wxGLPanel::OnLeftUp)
	EVT_MIDDLE_DOWN(wxGLPanel::OnMiddleDown)
	EVT_MIDDLE_UP(wxGLPanel::OnMiddleUp)
	EVT_RIGHT_DOWN(wxGLPanel::OnRightDown)
	EVT_RIGHT_UP(wxGLPanel::OnRightUp)
	EVT_LEAVE_WINDOW(wxGLPanel::OnLeaveWindow)
	EVT_CHAR_HOOK(wxGLPanel::OnKeys)
	EVT_IDLE(wxGLPanel::OnIdle)
	EVT_MOUSE_CAPTURE_LOST(wxGLPanel::OnCaptureLost)
wxEND_EVENT_TABLE()

wxGLPanel::wxGLPanel(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs)
	: wxGLCanvas(parent, attribs, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE) {
	context = std::make_unique<wxGLContext>(this, nullptr, &GLSurface::GetGLContextAttribs());
}

wxGLPanel::~wxGLPanel() {
	Cleanup();
	gls.RenderOneFrame();
}

bool wxGLPanel::SaveViewImage(const wxString& fileName, int width, int height) {
	wxImage image;
	return RenderViewImage(width, height, image) && image.SaveFile(fileName, wxBITMAP_TYPE_PNG);
}

bool wxGLPanel::RenderViewImage(int width, int height, wxImage& outImage) {
	std::vector<uint8_t> rgba;
	if (!gls.RenderToImage(static_cast<uint32_t>(width), static_cast<uint32_t>(height), rgba))
		return false;

	// wxImage takes ownership of malloc'd RGB data
	unsigned char* rgb = static_cast<unsigned char*>(malloc(static_cast<size_t>(width) * height * 3));
	for (size_t i = 0; i < static_cast<size_t>(width) * height; i++) {
		rgb[i * 3] = rgba[i * 4];
		rgb[i * 3 + 1] = rgba[i * 4 + 1];
		rgb[i * 3 + 2] = rgba[i * 4 + 2];
	}

	outImage = wxImage(width, height, rgb);
	return true;
}

bool wxGLPanel::IsStrokeActive() const {
	return activeStroke || (brushStroke && brushStroke->IsActive());
}

UndoHistory* wxGLPanel::GetUndoHistory() {
	return &os->editor.undoHistory;
}

void wxGLPanel::OnShown() {
	if (!context->IsOK()) {
		wxLogError("Outfit Studio: OpenGL context is not OK.");
		wxMessageBox(_("Outfit Studio: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR, os);
	}

	gls.Initialize(this, context.get());
	auto size = GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight());
	gls.SetMaskVisible();

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

	UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directional2Dir);

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

	bool perspectiveView = OutfitStudioConfig.GetBoolValue("Rendering/PerspectiveView", true);
	os->menuBar->Check(XRCID("btnViewPerspective"), perspectiveView);
	os->toolBarV->ToggleTool(XRCID("btnViewPerspective"), perspectiveView);
	gls.SetPerspective(perspectiveView);

	gls.SetComplexMaterialEnabled(OutfitStudioConfig.GetBoolValue("Rendering/ComplexMaterial", true));
	gls.SetPBREnabled(OutfitStudioConfig.GetBoolValue("Rendering/TruePBR", true));

	os->MeshesFromProj();

	UpdateFloor();
	UpdateNodes();
	UpdateBones();

	Render();
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	os = dynamic_cast<OutfitStudioFrame*>(win);
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, const std::string& shapeName) {
	std::vector<std::string> shapeList = nif->GetShapeNames();

	for (size_t i = 0; i < shapeList.size(); i++) {
		if (!shapeName.empty() && shapeList[i] != shapeName)
			continue;

		Mesh* m = gls.AddMeshFromNif(nif, shapeList[i]);
		if (!m)
			continue;

		NiShape* shape = nif->FindBlockByName<NiShape>(shapeList[i]);
		if (shape && shape->IsSkinned()) {
			// Overwrite skin matrix with the one from AnimInfo
			MatTransform globalToShape = os->project->GetWorkAnim()->GetTransformGlobalToShape(shape);
			if (nif->GetHeader().GetVersion().IsSF())
				globalToShape.translation *= sfHavokScale;
			m->SetXformModelToMesh(Mesh::xformNifToMesh.ComposeTransforms(globalToShape.ComposeTransforms(Mesh::xformMeshToNif)));
		}

		m->BuildVertexAdjacency();
		m->BuildEdgeList();
		m->MaskFill(0.0f);
		m->WeightFill(0.0f);

		if (extInitialized) {
			gls.SetContext();
			m->CreateBuffers();
		}
	}
}

void wxGLPanel::SetMeshTextures(const std::string& shapeName,
								const std::vector<std::string>& textureFiles,
								const bool hasMatFile,
								const MaterialFile& matFile,
								const bool reloadTextures,
								const std::shared_ptr<const SFLayeredMaterial>& sfMaterial) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == SF) {
		// Every Starfield shape renders through its layered material. One that has none - its material
		// couldn't be read, or it never had a shader - is a single layer of whatever textures it has.
		const SFLayeredMaterial material = sfMaterial ? *sfMaterial : SFLayeredMaterial::FromTextureFiles(textureFiles);
		gls.AssignSFMaterial(m, material, Config["AppDir"] + "/res/shaders/sf_default.vert", Config["AppDir"] + "/res/shaders/sf_default.frag", reloadTextures);
		gls.UpdateShaders(m);
		return;
	}

	m->sfLayered = false;
	m->sfRenderData.reset();

	std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
	std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";
	bool renderAsPBR = false;

	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		vShader = Config["AppDir"] + "/res/shaders/fo4_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/fo4_default.frag";
	}
	else if (targetGame == OB) {
		vShader = Config["AppDir"] + "/res/shaders/ob_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/ob_default.frag";
	}
	else if (m->pbr && gls.IsPBREnabled()) {
		// A True PBR shape reads its texture slots differently enough from every other Skyrim shape
		// that it gets its own pair rather than another branch inside the shared one.
		vShader = Config["AppDir"] + "/res/shaders/sk_truepbr.vert";
		fShader = Config["AppDir"] + "/res/shaders/sk_truepbr.frag";
		renderAsPBR = true;
	}

	GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader, reloadTextures, m->hasShader, renderAsPBR);
	if (mat) {
		m->material = mat;

		if (hasMatFile)
			m->UpdateFromMaterialFile(matFile);

		gls.UpdateShaders(m);
	}
}

void wxGLPanel::UpdateMeshVertices(const std::string& shapeName, std::vector<Vector3>* verts, bool updateBVH, bool recalcNormals, bool render, std::vector<Vector2>* uvs) {
	Mesh* m = gls.GetMesh(shapeName);
	if (m) {
		gls.Update(m, verts, uvs);

		if (updateBVH)
			BVHUpdateQueue.insert(m);

		if (recalcNormals)
			m->SmoothNormals();
	}

	if (render)
		gls.RenderOneFrame();
}

void wxGLPanel::RecalculateMeshBVH(const std::string& shapeName) {
	gls.RecalculateMeshBVH(shapeName);
}

void wxGLPanel::ShowShape(const std::string& shapeName, bool show) {
	gls.SetMeshVisibility(shapeName, show);
}

void wxGLPanel::SetActiveShapes(const std::vector<std::string>& shapeNames) {
	gls.SetActiveMeshes(shapeNames);
}

void wxGLPanel::SetSelectedShape(const std::string& shapeName) {
	gls.SetSelectedMesh(shapeName);
}

void wxGLPanel::DeleteMesh(const std::string& shape) {
	Mesh* m = gls.GetMesh(shape);
	if (m) {
		BVHUpdateQueue.erase(m);
		gls.DeleteMesh(m);
	}
}

void wxGLPanel::Cleanup() {
	XMoveMesh = nullptr;
	YMoveMesh = nullptr;
	ZMoveMesh = nullptr;
	XRotateMesh = nullptr;
	YRotateMesh = nullptr;
	ZRotateMesh = nullptr;
	XScaleMesh = nullptr;
	YScaleMesh = nullptr;
	ZScaleMesh = nullptr;
	ScaleUniformMesh = nullptr;

	XPivotMesh = nullptr;
	YPivotMesh = nullptr;
	ZPivotMesh = nullptr;
	PivotCenterMesh = nullptr;

	RotationCenterMesh = nullptr;
	RotationCenterMeshRingX = nullptr;
	RotationCenterMeshRingY = nullptr;
	RotationCenterMeshRingZ = nullptr;

	nodesPoints.clear();
	nodesLines.clear();
	bonesPoints.clear();
	bonesLines.clear();
	physicsBones.clear();
	floorMeshes.clear();

	BVHUpdateQueue.clear();

	gls.Cleanup();
}

void wxGLPanel::SetView(const char type) {
	gls.SetView(type);

	if (transformMode)
		ShowTransformTool();
	else
		Render();
}

void wxGLPanel::SetPerspective(const bool enabled) {
	gls.SetPerspective(enabled);
	gls.RenderOneFrame();
}

void wxGLPanel::SetFieldOfView(const int fieldOfView) {
	gls.SetFieldOfView(fieldOfView);
	gls.RenderOneFrame();
}

void wxGLPanel::SetDepthClip(const float zNear, const float zFar) {
	gls.SetDepthClip(zNear, zFar);
	gls.RenderOneFrame();
}

void wxGLPanel::UpdateLights(const int ambient, const int frontal, const int directional0, const int directional1, const int directional2, const nifly::Vector3& directional0Dir, const nifly::Vector3& directional1Dir, const nifly::Vector3& directonal2Dir) {
	gls.UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directonal2Dir);
	gls.RenderOneFrame();
}

bool wxGLPanel::SetHDRiBackground(const std::string& fileName) {
	const bool loaded = gls.SetHDRiBackground(fileName);
	gls.RenderOneFrame();
	return loaded;
}

void wxGLPanel::SetShapeGhostMode(const std::string& shapeName, bool on) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	if (on)
		m->rendermode = Mesh::RenderMode::LitWire;
	else
		m->rendermode = Mesh::RenderMode::Normal;
}

void wxGLPanel::SetNormalSeamSmoothMode(bool enable) {
	for (auto& m : gls.GetActiveMeshes()) {
		m->smoothSeamNormals = enable;
		m->SmoothNormals();
	}
}

void wxGLPanel::SetLockNormalsMode(bool enable) {
	for (auto& m : gls.GetActiveMeshes())
		m->lockNormals = enable;
}

void wxGLPanel::RecalcNormals(const std::string& shapeName, bool force) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	bool oldLockNormals = m->lockNormals;
	if (m->lockNormals && force)
		m->lockNormals = false;

	m->SmoothNormals();
	m->lockNormals = oldLockNormals;
}

void wxGLPanel::OnIdle(wxIdleEvent& WXUNUSED(event)) {
	if (wxGetKeyState(wxKeyCode::WXK_SHIFT) || wxGetKeyState(wxKeyCode::WXK_CONTROL) || wxGetKeyState(wxKeyCode::WXK_ALT) || lbuttonDown || rbuttonDown || mbuttonDown)
		return;

	for (auto& m : BVHUpdateQueue)
		m->CreateBVH();

	BVHUpdateQueue.clear();
}

void wxGLPanel::OnPaint(wxPaintEvent& event) {
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		OnShown();
	}

	gls.RenderOneFrame();
	event.Skip();
}

void wxGLPanel::OnSize(wxSizeEvent& event) {
	wxSize sz = event.GetSize();
	gls.SetSize(sz.GetX(), sz.GetY());
	gls.RenderOneFrame();
}
