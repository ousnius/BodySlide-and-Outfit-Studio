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

static const char* kHoverHighlightOverlayName = "_shapehoverhilite";

void wxGLPanel::SetHoverHighlight(const std::string& shapeName) {
	if (shapeName == hoverHighlightName)
		return;

	// Drop any existing overlay first.
	if (!hoverHighlightName.empty()) {
		gls.DeleteOverlay(kHoverHighlightOverlayName);
		hoverHighlightName.clear();
	}

	if (shapeName.empty())
		return;

	Mesh* src = gls.GetMesh(shapeName);
	if (!src || src->nVerts <= 0 || src->nTris <= 0 || !src->verts || !src->tris)
		return;

	// Need an active GL context to create buffers for the overlay mesh.
	if (!gls.SetContext())
		return;

	auto* hov = new Mesh();
	hov->nVerts = src->nVerts;
	hov->nTris = src->nTris;
	hov->verts = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->norms = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->tris = std::make_unique<nifly::Triangle[]>(hov->nTris);

	for (int i = 0; i < hov->nVerts; i++) {
		hov->verts[i] = src->verts[i];
		if (src->norms)
			hov->norms[i] = src->norms[i];
	}
	for (int t = 0; t < hov->nTris; t++)
		hov->tris[t] = src->tris[t];

	// Inherit the source's model-space transform so the overlay sits
	// exactly on top of the original shape (skinned shapes have a
	// non-identity matModel).
	hov->matModel = src->matModel;
	hov->xformMeshToModel = src->xformMeshToModel;
	hov->xformModelToMesh = src->xformModelToMesh;

	hov->shapeName = kHoverHighlightOverlayName;
	hov->color = nifly::Vector3(0.25f, 1.0f, 0.25f); // Light green
	hov->prop.alpha = 0.5f;                           // Semi-transparent blend
	hov->material = gls.GetPrimitiveMaterial();       // Unlit, solid tint
	hov->rendermode = Mesh::RenderMode::UnlitSolid;
	hov->overlayLayer = 100;                          // Draw on top of other overlays
	hov->doublesided = true;
	hov->CreateBuffers();

	gls.AddOverlay(hov);
	hoverHighlightName = shapeName;
	Render();
}

void wxGLPanel::ClearHoverHighlight() {
	if (hoverHighlightName.empty())
		return;

	gls.DeleteOverlay(kHoverHighlightOverlayName);
	hoverHighlightName.clear();
	Render();
}

void wxGLPanel::SetActiveTool(ToolID brushID) {
	activeTool = brushID;

	switch (brushID) {
		case ToolID::MaskBrush: activeBrush = &maskBrush; break;
		case ToolID::InflateBrush: activeBrush = &inflateBrush; break;
		case ToolID::DeflateBrush: activeBrush = &deflateBrush; break;
		case ToolID::MoveBrush: activeBrush = &moveBrush; break;
		case ToolID::SmoothBrush: activeBrush = &smoothBrush; break;
		case ToolID::UndiffBrush: activeBrush = &undiffBrush; break;
		case ToolID::WeightBrush: activeBrush = &weightBrush; break;
		case ToolID::ColorBrush: activeBrush = &colorBrush; break;
		case ToolID::AlphaBrush: activeBrush = &alphaBrush; break;
		default: activeBrush = nullptr; break;
	}

	gls.SetXMirrorCursor(GetToolOptionXMirror());
}

void wxGLPanel::SetLastTool(ToolID tool) {
	lastTool = tool;
}

std::vector<TweakBrush*> wxGLPanel::GetBrushList() {
	std::vector<TweakBrush*> brushList{&inflateBrush,
									   &deflateBrush,
									   &moveBrush,
									   &smoothBrush,
									   &undiffBrush,
									   &maskBrush,
									   &weightBrush,
									   &colorBrush,
									   &alphaBrush};

	return brushList;
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	// No editing shortcuts while an animation is playing.
	if (os && os->IsAnimationPlaying()) {
		event.Skip();
		return;
	}

	if (event.GetKeyCode() == WXK_F12 && event.ControlDown() && event.AltDown() && event.ShiftDown()) {
		// Hidden: saves an offscreen render of the view, to check rendering without screen captures
		wxString fileName = wxString::FromUTF8(Config["AppDir"]) + "/ViewportCapture.png";
		if (SaveViewImage(fileName, 1024, 1024))
			wxLogMessage("Saved view image to '%s'.", fileName);
		return;
	}

	if (event.GetKeyCode() == WXK_SPACE && event.GetModifiers() == wxMOD_CONTROL) {
		if (!os->editor.activeSlider.empty()) {
			os->ExitSliderEdit();
		}
		else {
			os->EnterSliderEdit();
			os->ScrollToActiveSlider();
		}
	}
	else if (!event.HasAnyModifiers()) {
		if (event.GetUnicodeKey() == 'V') {
			wxPoint cursorPos(event.GetPosition());

			int vertIndex;
			Mesh* outHitMesh;
			if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex, nullptr, &outHitMesh))
				return;

			if (os->currentTabButton == os->colorsTabButton) {
				auto cpBrushColor = (wxColourPickerCtrl*)os->FindWindowById(XRCID("cpBrushColor"));
				if (!cpBrushColor)
					return;

				if (outHitMesh->vcolors) {
					// Set color for brush to vertex color
					Vector3 vcolor = outHitMesh->vcolors[vertIndex];
					SetColorBrush(vcolor);

					// Set color for picker to vertex color
					wxColour color(vcolor.x * 255.0f, vcolor.y * 255.0f, vcolor.z * 255.0f);
					cpBrushColor->SetColour(color);
				}
			}
			else {
				// Find shape for hit mesh name
				NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(outHitMesh->shapeName);
				if (!shape)
					return;

				os->CloseBrushSettings();

				wxDialog dlg;
				if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(shape, verts);

					Vector3 oldPos = verts[vertIndex];
					XRCCTRL(dlg, "posX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.x));
					XRCCTRL(dlg, "posY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.y));
					XRCCTRL(dlg, "posZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.z));

					if (dlg.ShowModal() == wxID_OK) {
						Vector3 newPos;
						newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
						newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
						newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

						// Move vertex in shape directly
						if (!os->editor.bEditSlider)
							os->project->MoveVertex(shape, newPos, vertIndex);

						// To mesh coordinates
						oldPos = Mesh::TransformPosNifToMesh(oldPos);
						newPos = Mesh::TransformPosNifToMesh(newPos);

						UndoStateShape uss;
						uss.shapeName = shape->name.get();
						uss.pointStartState[vertIndex] = oldPos;
						uss.pointEndState[vertIndex] = newPos;

						// Push changes onto undo stack and execute
						UndoStateProject* usp = GetUndoHistory()->PushState();
						usp->undoType = UndoType::VertexPosition;
						usp->usss.push_back(std::move(uss));

						if (os->editor.bEditSlider) {
							usp->sliderName = os->editor.activeSlider;

							float sliderscale = os->project->SliderValue(os->editor.activeSlider);
							if (sliderscale == 0.0)
								sliderscale = 1.0;

							usp->sliderscale = sliderscale;
						}

						ApplyUndoState(usp, false);
						os->UpdateUndoTools();
					}

					if (transformMode)
						ShowTransformTool();
				}
			}
		}
		else if (event.GetUnicodeKey() == '0' && os->menuBar->IsEnabled(XRCID("btnSelect")))
			os->SelectTool(ToolID::Select);
		else if (event.GetUnicodeKey() == '1' && os->menuBar->IsEnabled(XRCID("btnMaskBrush")))
			os->SelectTool(ToolID::MaskBrush);
		else if (event.GetUnicodeKey() == '2' && os->menuBar->IsEnabled(XRCID("btnInflateBrush")))
			os->SelectTool(ToolID::InflateBrush);
		else if (event.GetUnicodeKey() == '3' && os->menuBar->IsEnabled(XRCID("btnDeflateBrush")))
			os->SelectTool(ToolID::DeflateBrush);
		else if (event.GetUnicodeKey() == '4' && os->menuBar->IsEnabled(XRCID("btnMoveBrush")))
			os->SelectTool(ToolID::MoveBrush);
		else if (event.GetUnicodeKey() == '5' && os->menuBar->IsEnabled(XRCID("btnSmoothBrush")))
			os->SelectTool(ToolID::SmoothBrush);
		else if (event.GetUnicodeKey() == '6' && os->menuBar->IsEnabled(XRCID("btnUndiffBrush")))
			os->SelectTool(ToolID::UndiffBrush);
		else if (event.GetUnicodeKey() == '7' && os->menuBar->IsEnabled(XRCID("btnWeightBrush")))
			os->SelectTool(ToolID::WeightBrush);
		else if (event.GetUnicodeKey() == '8' && os->menuBar->IsEnabled(XRCID("btnColorBrush")))
			os->SelectTool(ToolID::ColorBrush);
		else if (event.GetUnicodeKey() == '9' && os->menuBar->IsEnabled(XRCID("btnAlphaBrush")))
			os->SelectTool(ToolID::AlphaBrush);
		else if (event.GetKeyCode() == WXK_SPACE) {
			if (os->brushSettingsPopupTransient && os->brushSettingsPopupTransient->IsShown()) {
				os->CloseBrushSettings();
			}
			else {
				bool brushSettingsNearCursor = Config.GetBoolValue("Input/BrushSettingsNearCursor");
				if (brushSettingsNearCursor)
					os->PopupBrushSettings();
				else
					os->PopupBrushSettings(os->brushSettings);
			}
		}
		else if (event.GetKeyCode() == WXK_ESCAPE) {
			if (isMovingVertex) {
				CancelMoveVertex();
				isMovingVertex = false;
			}
			if (isSlidingEdge) {
				CancelEdgeSlide();
				isSlidingEdge = false;
			}
			os->CloseBrushSettings();
		}
	}

	event.Skip();
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	// Check if brush strokes are currently allowed
	if (activeBrush == &weightBrush) {
		std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;
	}

	TweakPickInfo tpi;
	Mesh* hitMesh = nullptr;
	int hitTri = -1;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
	if (!hit || !hitMesh)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
	tpi.normal.Normalize();
	tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);

	Vector3 v;
	Vector3 vo;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

	v = v * -1.0f;
	tpi.view = v;

	savedBrush = activeBrush;

	if (wxGetKeyState(WXK_CONTROL)) {
		if (wxGetKeyState(WXK_ALT) && !segmentMode) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			UnMaskBrush.setFocus(maskBrush.getFocus());
			UnMaskBrush.setSpacing(maskBrush.getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		WeightBrushBones bones = ProjectOperations(*os->project, os->editor).GetWeightBrushBones(os->GetXMirrorBone());
		AnimInfo* animInfo = os->project->GetWorkAnim();

		if (wxGetKeyState(WXK_ALT)) {
			bones.ApplyTo(unweightBrush, animInfo);
			unweightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			unweightBrush.setStrength(-weightBrush.getStrength());
			unweightBrush.setFocus(weightBrush.getFocus());
			unweightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &unweightBrush;
		}
		else if (wxGetKeyState(WXK_SHIFT)) {
			bones.ApplyTo(smoothWeightBrush, animInfo);
			smoothWeightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			smoothWeightBrush.setStrength(weightBrush.getStrength() * 15.0f);
			smoothWeightBrush.setFocus(weightBrush.getFocus());
			smoothWeightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &smoothWeightBrush;
		}
		else {
			bones.ApplyTo(weightBrush, animInfo);
		}
	}
	else if (wxGetKeyState(WXK_ALT) && !segmentMode) {
		if (activeBrush == &inflateBrush) {
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			activeBrush = &inflateBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-activeBrush->getStrength());
			UnMaskBrush.setFocus(activeBrush->getFocus());
			UnMaskBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else if (activeBrush == &colorBrush) {
			uncolorBrush.setStrength(-activeBrush->getStrength());
			uncolorBrush.setFocus(activeBrush->getFocus());
			uncolorBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &uncolorBrush;
		}
		else if (activeBrush == &alphaBrush) {
			unalphaBrush.setStrength(-activeBrush->getStrength());
			unalphaBrush.setFocus(activeBrush->getFocus());
			unalphaBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &unalphaBrush;
		}
	}
	else if (activeBrush == &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothMaskBrush.setStrength(activeBrush->getStrength());
		smoothMaskBrush.setFocus(activeBrush->getFocus());
		smoothMaskBrush.setSpacing(activeBrush->getSpacing());
		activeBrush = &smoothMaskBrush;
	}
	else if (activeBrush != &weightBrush && activeBrush != &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		activeBrush = &smoothBrush;
	}

	activeBrush->setRadius(brushSize);
	activeBrush->setMirror(GetToolOptionXMirror());
	activeBrush->setConnected(toolOptionConnectedOnly);
	activeBrush->setRestrictPlane(toolOptionRestrictPlane);
	activeBrush->setRestrictNormal(toolOptionRestrictNormal);

	brushStroke = std::make_unique<BrushStrokeController>(*os->project, os->editor);

	if (activeBrush->Type() == TweakBrush::BrushType::Weight) {
		std::vector<NiShape*> shapes;
		for (auto& sel : os->GetSelectedItems())
			shapes.push_back(sel->GetShape());

		brushStroke->PrepareWeightStroke(shapes);
	}

	bool applyFirstDab = activeBrush->Type() != TweakBrush::BrushType::Move && !segmentMode;
	brushStroke->Begin(gls.GetActiveMeshes(), activeBrush, tpi, applyFirstDab);

	if (activeBrush->Type() != TweakBrush::BrushType::Move && segmentMode) {
		{
			bool changed = os->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->ShowSegment();
				os->ShowPartition();
			}
		}
	}

	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	TweakPickInfo tpi;

	if (brushStroke && brushStroke->IsActive()) {
		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true);
		Mesh* hitMesh = nullptr;
		int hitTri = -1;

		if (activeBrush->Type() == TweakBrush::BrushType::Move) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			if (!hit)
				return;

			hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
			if (!hit || !hitMesh)
				return;

			tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
			tpi.normal.Normalize();
			tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);
		}

		Vector3 v;
		Vector3 vo;
		gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

		v = v * -1.0f;
		tpi.view = v;
		if (segmentMode) {
			bool changed = os->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->ShowSegment();
				os->ShowPartition();
			}
		}
		else {
			os->ApplyRefresh(brushStroke->Update(tpi));
		}

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment();
			os->ShowPartition();
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (brushStroke && brushStroke->IsActive()) {
		os->ApplyRefresh(brushStroke->End());
		brushStroke.reset();
		activeBrush = savedBrush;

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment();
			os->ShowPartition();

			if (os->currentTabButton)
				os->currentTabButton->SetPendingChanges();
		}

		os->UpdateUndoTools();
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
	if (!hit)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.center = xformCenter;
	xformCenterInitial = xformCenter;

	std::string mname = hitMesh->shapeName;
	if (mname.find("Move") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
			case 'X':
				tpi.view = Vector3(1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Y':
				tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Z':
				tpi.view = Vector3(0.0f, 0.0f, 1.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
		}
	}
	else if (mname.find("Rotate") != std::string::npos) {
		translateBrush.SetXFormType(1);
		switch (mname[0]) {
			case 'X':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
				//tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
			case 'Y':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
				//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 1.0f, 0.0f);
				break;
			case 'Z':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
				//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
		}
	}
	else if (mname.find("Scale") != std::string::npos) {
		if (mname.find("Uniform") == std::string::npos) {
			translateBrush.SetXFormType(2);
			switch (mname[0]) {
				case 'X':
					tpi.view = Vector3(1.0f, 0.0f, 0.0f);
					tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
					break;
				case 'Y':
					tpi.view = Vector3(0.0f, 1.0f, 0.0f);
					tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
					break;
				case 'Z':
					tpi.view = Vector3(0.0f, 0.0f, 1.0f);
					tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
					break;
			}
		}
		else {
			translateBrush.SetXFormType(3);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	else
		return false;

	if (!nodesMode && !bonesMode) {
		activeStroke = std::make_unique<TweakStroke>(gls.GetActiveMeshes(), &translateBrush, *os->editor.undoHistory.PushState());

		if (os->editor.bEditSlider) {
			activeStroke->usp.sliderName = os->editor.activeSlider;

			float sliderscale = os->project->SliderValue(os->editor.activeSlider);
			if (sliderscale == 0.0)
				sliderscale = 1.0;

			activeStroke->usp.sliderscale = sliderscale;
		}

		activeStroke->beginStroke(tpi);
	}
	else {
		// Bones/nodes-mode transform: edit the active bone/node directly.
		// The TweakStroke machinery isn't used here because we don't need
		// to modify any mesh vertices through a brush -- we change the
		// bone's local transform and let skinning update the meshes.
		const std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;

		// Only translate and rotate are supported for bones/nodes.
		if (mname.find("Move") != std::string::npos)
			boneXformType = 0;
		else if (mname.find("Rotate") != std::string::npos)
			boneXformType = 1;
		else
			return false;

		boneXformPickStart = tpi.origin;
		boneXformPlaneNormalModel = tpi.normal;
		boneXformAxisModel = tpi.view;
		boneXformPlaneDist = tpi.origin.dot(tpi.normal);

		// Pose mode: edit poseTranVec/poseRotVec only (no NIF writes, no
		// undo).  Only AnimBones have pose data; plain NIF nodes do not.
		boneXformPoseMode = false;
		AnimBone* poseBPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (os->project->bPose && poseBPtr) {
			boneXformPoseMode = true;
			xformInitialLocalToParent = poseBPtr->xformToParent;
			// M = parent_pose_to_global ∘ xformToParent
			MatTransform parentPoseToGlobal;
			if (poseBPtr->parent)
				parentPoseToGlobal = poseBPtr->parent->xformPoseToGlobal;
			xformInitialParentToGlobal = parentPoseToGlobal.ComposeTransforms(poseBPtr->xformToParent);
			boneXformPoseInitialTran = poseBPtr->poseTranVec;
			boneXformPoseInitialRot = poseBPtr->poseRotVec;
		}
		else {
			// Capture the initial transforms of the active bone/node.
			xformInitialLocalToParent = MatTransform();
			xformInitialParentToGlobal = MatTransform();

			const AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
			if (bPtr) {
				xformInitialLocalToParent = bPtr->xformToParent;
				if (bPtr->parent)
					xformInitialParentToGlobal = bPtr->parent->xformToGlobal;
			}
			else if (auto* nif = os->project->GetWorkNif()) {
				if (NiNode* node = nif->FindBlockByName<NiNode>(activeBone)) {
					xformInitialLocalToParent = node->GetTransformToParent();
					// Compose the transforms of all ancestor nodes to build
					// parent-to-global.
					NiNode* p = nif->GetParentNode(node);
					while (p) {
						xformInitialParentToGlobal = p->GetTransformToParent().ComposeTransforms(xformInitialParentToGlobal);
						p = nif->GetParentNode(p);
					}
				}
				else {
					// No AnimBone and no NiNode -- nothing to edit.
					return false;
				}
			}
			else
				return false;

			// Push an undo state.  nodeEndXformToParent is filled in in
			// EndTransform; until then it equals nodeStartXformToParent.
			UndoStateProject* usp = os->editor.undoHistory.PushState();
			usp->undoType = UndoType::NodeTransform;
			usp->boneName = activeBone;
			usp->nodeStartXformToParent = xformInitialLocalToParent;
			usp->nodeEndXformToParent = xformInitialLocalToParent;
		}
	}

	XMoveMesh->bVisible = false;
	YMoveMesh->bVisible = false;
	ZMoveMesh->bVisible = false;
	XRotateMesh->bVisible = false;
	YRotateMesh->bVisible = false;
	ZRotateMesh->bVisible = false;
	XScaleMesh->bVisible = false;
	YScaleMesh->bVisible = false;
	ZScaleMesh->bVisible = false;
	ScaleUniformMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdateTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	if (!nodesMode && !bonesMode) {
		translateBrush.GetWorkingPlane(pn, pd);
		gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		activeStroke->updateStroke(tpi);
		ShowTransformTool();
		return;
	}

	// Bones/nodes-mode transform: compute the bone/node's new local transform
	// directly from the drag and apply it.  Meshes update live via skinning.
	const std::string activeBone = os->GetActiveBone();
	if (activeBone.empty()) {
		ShowTransformTool();
		return;
	}

	// Intersect the mouse ray with the pick plane captured in StartTransform.
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, boneXformPlaneNormalModel, boneXformPlaneDist);

	if (boneXformPoseMode) {
		AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (!bPtr) {
			ShowTransformTool();
			return;
		}

		// M = parent_pose_to_global ∘ xformToParent  (captured in
		// xformInitialParentToGlobal).  Edits go into poseTranVec/poseRotVec
		// which are applied in the bone's own CS before xformToParent.
		if (boneXformType == 0) {
			const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
			const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
			const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
			const Vector3 localOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
			bPtr->poseTranVec = boneXformPoseInitialTran + localOffset;
		}
		else if (boneXformType == 1) {
			const Vector3 a = boneXformPickStart - xformCenterInitial;
			const Vector3 b = tpi.origin - xformCenterInitial;
			if (a.length() > EPSILON && b.length() > EPSILON) {
				const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
				const float cosA = a.dot(b);
				const float angle = std::atan2(sinA, cosA);

				Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
				Vector3 axisLocal = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
				const float axisLen = axisLocal.length();
				if (axisLen > EPSILON) {
					axisLocal /= axisLen;
					const Matrix3 rot = RotVecToMat(axisLocal * angle);
					const Matrix3 newPoseRot = rot * RotVecToMat(boneXformPoseInitialRot);
					bPtr->poseRotVec = RotMatToVec(newPoseRot);
				}
			}
		}

		bPtr->UpdatePoseTransform();

		// Live mesh update to show the pose change immediately.
		if (auto* nif = os->project->GetWorkNif()) {
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		// Keep the pose-sliders GUI in sync with the new values.
		os->PoseToGUI();

		UpdateBones();
		UpdateNodes();
		ShowTransformTool();
		return;
	}

	MatTransform newToParent = xformInitialLocalToParent;

	if (boneXformType == 0) {
		// Translate along the gizmo's axis.  Project the drag vector onto
		// the axis in model space, convert to nif distance, then map the
		// resulting global offset into parent space (so scale/rotation of
		// ancestor nodes is handled correctly).
		const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
		const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
		const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
		const Vector3 parentOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
		newToParent.translation = xformInitialLocalToParent.translation + parentOffset;
	}
	else if (boneXformType == 1) {
		// Rotate about the gizmo ring's axis.  Angle is the signed angle
		// between the initial and current pick vectors in the rotation plane.
		const Vector3 a = boneXformPickStart - xformCenterInitial;
		const Vector3 b = tpi.origin - xformCenterInitial;
		if (a.length() > EPSILON && b.length() > EPSILON) {
			const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
			const float cosA = a.dot(b);
			const float angle = std::atan2(sinA, cosA);

			// Convert axis from model to global, then to parent.
			Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
			Vector3 axisParent = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
			const float axisLen = axisParent.length();
			if (axisLen > EPSILON) {
				axisParent /= axisLen;
				const Matrix3 rot = RotVecToMat(axisParent * angle);
				newToParent.rotation = rot * xformInitialLocalToParent.rotation;
			}
		}
	}

	// Apply the new transform to the AnimBone (if any) and to the NIF node.
	// We deliberately do not recalculate xformSkinToBone: that way the mesh
	// follows the bone through skinning, the active pose updates live, and
	// the "check for bad bones" residual correctly flags the new mismatch.
	auto* nif = os->project->GetWorkNif();
	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
	if (bPtr)
		bPtr->SetTransformBoneToParent(newToParent);
	if (nif)
		nif->SetNodeTransformToParent(activeBone, newToParent);

	// Record the end state on the open undo entry so an in-progress drag is
	// already undo-able if the stroke is canceled.
	if (UndoStateProject* usp = os->editor.undoHistory.GetCurState()) {
		if (usp->undoType == UndoType::NodeTransform && usp->boneName == activeBone)
			usp->nodeEndXformToParent = newToParent;
	}

	// Live mesh update (so skinned vertices follow the bone).
	if (nif) {
		for (auto& s : nif->GetShapes()) {
			std::vector<Vector3> verts;
			os->project->GetLiveVerts(s, verts);
			UpdateMeshVertices(s->name.get(), &verts, true, true, false);
		}
	}

	UpdateBones();
	UpdateNodes();
	ShowTransformTool();
}

void wxGLPanel::EndTransform() {
	if (!nodesMode && !bonesMode) {
		activeStroke->endStroke();
		activeStroke = nullptr;

		os->ActiveShapesUpdated(os->editor.undoHistory.GetCurState());
		if (!os->editor.bEditSlider) {
			{
				UndoStateProject* usp = os->editor.undoHistory.GetCurState();
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						os->project->ComputeUndoRestDiffs(shape, uss);
				}
			}

			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				os->UpdateShapeSource(s);
				os->project->RefreshMorphShape(s);
			}

			if (os->project->bPose) {
				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(s, verts);
					UpdateMeshVertices(s->name.get(), &verts, true, true, false);
				}
			}
		}
		else if (os->project->bPose) {
			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}
	}
	else {
		// Bones/nodes-mode: sync the AnimInfo's bone transforms out to the
		// NIF, refresh the bone-tree bad-bone icons, and mark the project
		// as modified.  In pose mode we only edited poseTranVec/poseRotVec
		// on the AnimBone -- no NIF writes, no undo, no pending changes.
		if (!boneXformPoseMode) {
			if (auto* nif = os->project->GetWorkNif()) {
				if (auto* workAnim = os->project->GetWorkAnim())
					workAnim->WriteNodesToNif(nif);
			}
			os->RefreshBoneTreeBadBoneIcons();
			os->SetPendingChanges();
		}
		boneXformPoseMode = false;
	}

	ShowTransformTool();
	os->UpdateUndoTools();
}

bool wxGLPanel::StartPivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
	if (!hit)
		return false;

	tpi.center = pivotPosition;

	std::string mname = hitMesh->shapeName;
	if (mname.find("PivotMesh") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
			case 'X':
				tpi.view = Vector3(1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Y':
				tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Z':
				tpi.view = Vector3(0.0f, 0.0f, 1.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
		}
	}
	else
		return false;

	std::vector<Mesh*> strokeMeshes{hitMesh};
	activeStroke = std::make_unique<TweakStroke>(strokeMeshes, &translateBrush, *os->editor.undoHistory.PushState());
	activeStroke->beginStroke(tpi);

	XPivotMesh->bVisible = false;
	YPivotMesh->bVisible = false;
	ZPivotMesh->bVisible = false;
	PivotCenterMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdatePivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	translateBrush.GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);

	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndPivotPosition() {
	activeStroke->endStroke();

	std::vector<Mesh*> refMeshes = activeStroke->GetRefMeshes();
	if (refMeshes.size() > 0) {
		UndoStateShape& uss = activeStroke->usp.usss[0];
		if (uss.pointStartState.size() > 0 && uss.pointEndState.size() > 0) {
			Vector3& pivotStartStatePos = uss.pointStartState[0];
			Vector3& pivotEndStatePos = uss.pointEndState[0];
			Vector3 pivotDiff = pivotEndStatePos - pivotStartStatePos;
			pivotPosition += pivotDiff;
		}
	}

	activeStroke = nullptr;
	ShowPivot();
}

bool wxGLPanel::SelectVertex(const wxPoint& screenPos) {
	int vertIndex;
	if (!gls.GetCursorVertex(screenPos.x, screenPos.y, &vertIndex))
		return false;

	if (os->activeItem) {
		Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
		if (m) {
			double newval = m->mask[vertIndex];
			if (wxGetKeyState(WXK_CONTROL))
				newval = 1.0f;
			else if (!segmentMode)
				newval = 0.0f;

			if (m->mask[vertIndex] != newval) {
				UndoStateProject* usp = GetUndoHistory()->PushState();
				usp->undoType = UndoType::Mask;
				usp->usss.emplace_back();
				UndoStateShape& uss = usp->usss.back();
				uss.shapeName = m->shapeName;
				uss.pointStartState[vertIndex].x = m->mask[vertIndex];
				uss.pointEndState[vertIndex].x = newval;
				ApplyUndoState(usp, false);

				if (!Config.GetBoolValue("Input/MaskHistory"))
					GetUndoHistory()->PopState();

				os->UpdateUndoTools();
			}
		}
	}

	if (transformMode)
		ShowTransformTool();

	return true;
}

bool wxGLPanel::StartPickVertex() {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	return true;
}

void wxGLPanel::UpdatePickVertex(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || hitResult.hoverPoint != mouseDownPoint)
		gls.HidePointCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickVertex() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || lastHitResult.hoverPoint != mouseDownPoint)
		return;

	// Clear PickVertex state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HidePointCursor();

	if (activeTool == ToolID::CollapseVertex)
		ClickCollapseVertex();
}

void wxGLPanel::ClickCollapseVertex() {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Make list of this vertex and its welded vertices.
	std::vector<uint16_t> verts;
	m->GetWeldSet(mouseDownPoint, verts);

	std::sort(verts.begin(), verts.end());

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareCollapseVertex(shape, uss, verts)) {
		wxMessageBox(_("The vertex picked has more than three connections."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

struct EdgeSlideCandidate {
	Edge edge;
	int targetPoint = -1;
	float t = 0.0f;
	float distanceSquared = 0.0f;
};

static float ClampEdgeSlideT(float t) {
	if (t < 0.0f)
		return 0.0f;

	if (t > 1.0f)
		return 1.0f;

	return t;
}

static bool FindEdgeSlideCandidate(GLSurface& gls, Mesh* m, int slidePoint, const Vector3& startPosition, const wxPoint& screenPos, EdgeSlideCandidate& outCandidate) {
	if (!m || slidePoint < 0 || slidePoint >= m->nVerts)
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges)
		return false;

	int startX = 0;
	int startY = 0;
	gls.ProjectPointToScreen(m->TransformPosMeshToModel(startPosition), startX, startY);

	bool foundCandidate = false;
	std::unordered_set<int> seenTargets;
	for (int edgeIndex : m->vertEdges[slidePoint]) {
		if (edgeIndex < 0 || edgeIndex >= m->nEdges)
			continue;

		const Edge& edge = m->edges[edgeIndex];
		int targetPoint = -1;
		if (edge.p1 == slidePoint)
			targetPoint = edge.p2;
		else if (edge.p2 == slidePoint)
			targetPoint = edge.p1;
		else
			continue;

		if (targetPoint < 0 || targetPoint >= m->nVerts || !seenTargets.insert(targetPoint).second)
			continue;

		int endX = 0;
		int endY = 0;
		gls.ProjectPointToScreen(m->TransformPosMeshToModel(m->verts[targetPoint]), endX, endY);

		float dx = static_cast<float>(endX - startX);
		float dy = static_cast<float>(endY - startY);
		float edgeLengthSquared = dx * dx + dy * dy;
		if (edgeLengthSquared <= 0.0001f)
			continue;

		float cursorDx = static_cast<float>(screenPos.x - startX);
		float cursorDy = static_cast<float>(screenPos.y - startY);
		float t = ClampEdgeSlideT((cursorDx * dx + cursorDy * dy) / edgeLengthSquared);
		float closestX = static_cast<float>(startX) + dx * t;
		float closestY = static_cast<float>(startY) + dy * t;
		float distX = static_cast<float>(screenPos.x) - closestX;
		float distY = static_cast<float>(screenPos.y) - closestY;
		float distanceSquared = distX * distX + distY * distY;

		if (!foundCandidate || distanceSquared < outCandidate.distanceSquared) {
			outCandidate.edge = edge;
			outCandidate.targetPoint = targetPoint;
			outCandidate.t = t;
			outCandidate.distanceSquared = distanceSquared;
			foundCandidate = true;
		}
	}

	return foundCandidate;
}

bool wxGLPanel::StartEdgeSlide(const wxPoint& WXUNUSED(screenPos)) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges || m->vertEdges[lastHitResult.hoverPoint].empty())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	edgeSlideStartPosition = lastHitResult.hoverMeshCoord;
	edgeSlideStartUV = Vector2();
	edgeSlideCurrentEdge = Edge();
	edgeSlideTarget = -1;
	edgeSlideHasUV = !os->editor.bEditSlider && GetToolOptionEdgeSlideUVCorrection() && m->texcoord != nullptr;
	if (edgeSlideHasUV)
		edgeSlideStartUV = m->texcoord[mouseDownPoint];

	UndoStateProject* usp = os->editor.undoHistory.PushState();
	usp->undoType = UndoType::VertexPosition;
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = edgeSlideStartPosition;
	if (edgeSlideHasUV)
		usp->usss[0].uvEndState[mouseDownPoint] = usp->usss[0].uvStartState[mouseDownPoint] = edgeSlideStartUV;

	if (os->editor.bEditSlider) {
		usp->sliderName = os->editor.activeSlider;
		float sliderscale = os->project->SliderValue(os->editor.activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateEdgeSlide(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];

	EdgeSlideCandidate candidate;
	if (!FindEdgeSlideCandidate(gls, m, mouseDownPoint, edgeSlideStartPosition, screenPos, candidate)) {
		gls.HideSegCursor();
		return;
	}

	edgeSlideCurrentEdge = candidate.edge;
	edgeSlideTarget = candidate.targetPoint;

	Vector3 targetPosition = m->verts[edgeSlideTarget];
	Vector3 newPosition = edgeSlideStartPosition + (targetPosition - edgeSlideStartPosition) * candidate.t;
	uss.pointEndState[mouseDownPoint] = newPosition;
	m->verts[mouseDownPoint] = newPosition;
	m->QueueUpdate(Mesh::UpdateType::Position);

	if (edgeSlideHasUV && m->texcoord) {
		Vector2 targetUV = m->texcoord[edgeSlideTarget];
		Vector2 newUV = edgeSlideStartUV + (targetUV - edgeSlideStartUV) * candidate.t;
		uss.uvEndState[mouseDownPoint] = newUV;
		m->texcoord[mouseDownPoint] = newUV;
		m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	}

	Vector3 newPositionModel = m->TransformPosMeshToModel(newPosition);
	gls.SetPointCursor(newPositionModel);
	gls.SetCenterCursor(newPositionModel);
	gls.AddVisSeg(m->TransformPosMeshToModel(edgeSlideStartPosition), m->TransformPosMeshToModel(targetPosition), "seghilite");
	gls.ShowCursor(true);
}

void wxGLPanel::EndEdgeSlide() {
	isSlidingEdge = false;
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];
	bool positionChanged = uss.pointEndState[mouseDownPoint] != uss.pointStartState[mouseDownPoint];
	bool uvChanged = false;
	auto uvStart = uss.uvStartState.find(mouseDownPoint);
	auto uvEnd = uss.uvEndState.find(mouseDownPoint);
	if (uvStart != uss.uvStartState.end() && uvEnd != uss.uvEndState.end())
		uvChanged = uvStart->second != uvEnd->second;

	if (!positionChanged && !uvChanged) {
		CancelEdgeSlide();
		return;
	}

	if (!os->editor.bEditSlider) {
		NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		if (shape)
			os->project->ComputeUndoRestDiffs(shape, uss);
	}

	ApplyUndoState(usp, false);
	gls.HideSegCursor();

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelEdgeSlide() {
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	os->editor.undoHistory.PopState();
	gls.HideSegCursor();
}

bool wxGLPanel::StartPhysicsGrab(const wxPoint& screenPos) {
	const auto& affectedShapes = os->GetPhysicsAffectedShapes();
	if (affectedShapes.empty())
		return false;

	// The pump skips the BVH update every frame, so the trees still describe
	// the pose the simulation started from. Fit them to where the mesh is now,
	// or the grab misses the cloth that has swung away since.
	os->RefitPhysicsBVH();

	// Any shape the simulation moves can be grabbed, whether or not it is one
	// of the shapes picked for editing: what is being grabbed is the
	// simulation, not the project.
	Vector3 viewDir;
	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, viewDir, viewOrigin);

	Mesh* hitMesh = nullptr;
	Vector3 hitMeshPos;
	float hitDistance = std::numeric_limits<float>::max();

	for (auto* m : gls.GetMeshes()) {
		if (!m->bVisible || !m->bvh || affectedShapes.count(m->shapeName) == 0)
			continue;

		Vector3 rayOrigin = m->TransformPosModelToMesh(viewOrigin);
		Vector3 rayDir = m->TransformDirModelToMesh(viewDir);

		std::vector<IntersectResult> results;
		if (!m->bvh->IntersectRay(rayOrigin, rayDir, &results))
			continue;

		for (auto& result : results) {
			const float distance = result.HitCoord.DistanceTo(rayOrigin);
			if (distance >= hitDistance)
				continue;

			hitDistance = distance;
			hitMesh = m;
			hitMeshPos = result.HitCoord;
		}
	}

	if (!hitMesh)
		return false;

	std::vector<Physics::GrabTarget> targets;
	if (!CollectPhysicsGrabTargets(hitMesh, hitMeshPos, targets))
		return false;

	if (!os->BeginPhysicsGrab(targets))
		return false;

	// The simulation pulls the grabbed patch out from under the cursor right
	// away, so the drag reads against a plane through the grabbed spot facing
	// the camera rather than against the mesh - the move brush freezes a plane
	// for the same reason.
	physicsGrabStart = hitMesh->TransformPosMeshToModel(hitMeshPos);
	physicsGrabPlaneNormal = viewDir * -1.0f;
	physicsGrabPlaneDist = physicsGrabStart.dot(physicsGrabPlaneNormal);

	ShowPhysicsGrabMarker(physicsGrabStart);
	return true;
}

bool wxGLPanel::CollectPhysicsGrabTargets(Mesh* m, const Vector3& meshPos, std::vector<Physics::GrabTarget>& outTargets) {
	AnimInfo* anim = os->project->GetWorkAnim();
	auto skinIt = anim->shapeSkinning.find(m->shapeName);
	if (skinIt == anim->shapeSkinning.end())
		return false;

	// A patch the size of the brush is taken hold of rather than a single
	// vertex, so that a grab catches a piece of cloth instead of whichever
	// bone happens to skin the one vertex under the cursor.
	const float radius = m->TransformDistModelToMesh(brushSize);
	if (!(radius > 0.0f))
		return false;

	Vector3 patchCenter = meshPos;
	std::vector<IntersectResult> results;
	m->bvh->IntersectSphere(patchCenter, radius, &results);

	// Falloff towards the rim of the patch, so the pull stays centered on the
	// cursor instead of dragging the whole bone chain the patch reaches into
	std::unordered_map<uint16_t, float> patch;
	for (auto& result : results) {
		const Triangle& tri = m->tris[result.HitFacet];
		for (const uint16_t point : {tri.p1, tri.p2, tri.p3}) {
			if (patch.count(point) != 0)
				continue;

			const float distance = m->verts[point].DistanceTo(meshPos) / radius;
			if (distance >= 1.0f)
				continue;

			patch[point] = 1.0f - distance * distance * (3.0f - 2.0f * distance);
		}
	}

	if (patch.empty())
		return false;

	// Hand the patch over to the bones skinning it: each one gets the share of
	// it its weights hold, anchored at the center of that share. Anchoring per
	// bone rather than all of them at the cursor keeps the lever arms short,
	// so pulling the hem of a skirt bends it instead of spinning its bones.
	AnimSkin& skin = skinIt->second;
	float strongest = 0.0f;

	for (auto& boneName : skin.boneNames) {
		AnimWeight& boneWeight = skin.boneWeights[boneName.second];

		Vector3 anchor;
		float share = 0.0f;

		for (auto& patchPoint : patch) {
			auto weightIt = boneWeight.weights.find(patchPoint.first);
			if (weightIt == boneWeight.weights.end())
				continue;

			const float weight = patchPoint.second * weightIt->second;
			if (!(weight > 0.0f))
				continue;

			// The simulation works in NIF global space, model space is that
			// same space in render axes and units
			anchor += Mesh::TransformPosMeshToNif(m->TransformPosMeshToModel(m->verts[patchPoint.first])) * weight;
			share += weight;
		}

		if (!(share > 0.0f))
			continue;

		outTargets.push_back({boneName.first, anchor / share, share});
		strongest = std::max(strongest, share);
	}

	if (outTargets.empty())
		return false;

	// Shares are relative to the bone holding the most of the patch: a patch
	// spread thin over many bones must not pull harder than a compact one.
	// Bones barely touching it only add drag, so they are dropped.
	constexpr float minShare = 0.05f;
	for (auto& target : outTargets)
		target.weight /= strongest;

	outTargets.erase(std::remove_if(outTargets.begin(), outTargets.end(), [minShare](const Physics::GrabTarget& target) { return target.weight < minShare; }),
					 outTargets.end());

	return !outTargets.empty();
}

void wxGLPanel::UpdatePhysicsGrab(const wxPoint& screenPos) {
	Vector3 target;
	if (!gls.CollidePlane(screenPos.x, screenPos.y, target, physicsGrabPlaneNormal, physicsGrabPlaneDist))
		return;

	os->UpdatePhysicsGrab(Mesh::TransformDiffMeshToNif(target - physicsGrabStart));
	ShowPhysicsGrabMarker(target);
}

void wxGLPanel::EndPhysicsGrab() {
	isPhysicsGrabbing = false;
	HidePhysicsGrabMarker();
	os->EndPhysicsGrab();
}

void wxGLPanel::ShowPhysicsGrabMarker(const Vector3& modelPos) {
	// Marks where the cursor is pulling to, which is not where the mesh is:
	// how far the mesh trails behind the marker shows how hard its physics
	// setup is resisting the grab. Same circle the brush cursor uses, so the
	// radius it covers reads the same way.
	Mesh* marker = gls.AddVisCircle(modelPos, physicsGrabPlaneNormal, brushSize, "physicsgrabcircle");
	if (marker)
		marker->color = Vector3(1.0f, 0.75f, 0.2f);

	gls.AddVisPoint(modelPos, "physicsgrabcenter")->color = Vector3(1.0f, 0.75f, 0.2f);
}

void wxGLPanel::HidePhysicsGrabMarker() {
	gls.DeleteOverlay("physicsgrabcircle");
	gls.DeleteOverlay("physicsgrabcenter");
}

void wxGLPanel::ShowPhysicsProbe(const Vector3& nifPos, float nifRadius) {
	// A mesh rather than an overlay: overlays are drawn after the depth buffer is
	// cleared, so the ball would float in front of the cloth it is pressed into
	// instead of sinking behind it, and how deep it has gone is the whole point.
	// It carries bPrimitive, so nothing treats it as a shape of the project.
	// Model space is NIF global space in render axes and units.
	Mesh* m = gls.AddVis3dSphere(Mesh::TransformPosNifToMesh(nifPos), Mesh::TransformDistNifToMesh(nifRadius), Vector3(1.0f, 0.75f, 0.2f), "physicsprobeball", true);
	if (!m)
		return;

	// Shaded rather than flat tinted, which the other primitives are. A ball drawn in one
	// colour is a disc: nothing in it says where its near side is, and reading how far it
	// has been pushed into the cloth is the whole reason it is on screen. The sphere is
	// built with per-vertex normals and a tangent space already, so this is all it takes.
	m->rendermode = Mesh::RenderMode::LitSolid;
}

void wxGLPanel::HidePhysicsProbe() {
	gls.DeleteMesh("physicsprobeball");
}

bool wxGLPanel::StartMoveVertex(const wxPoint& screenPos) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	mouseDownPointNormal = m->TransformDirMeshToModel(m->norms[mouseDownPoint]);
	moveVertexOperation = MoveVertexOperation::None;

	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, mouseDownViewDir, viewOrigin);
	mouseDownViewDir *= -1.0f;

	// snapDistance: shortest edge length of triangle under pointer
	const Triangle& tri = m->tris[lastHitResult.hoverTri];
	Vector3 gtp1 = m->TransformPosMeshToModel(m->verts[tri.p1]);
	Vector3 gtp2 = m->TransformPosMeshToModel(m->verts[tri.p2]);
	Vector3 gtp3 = m->TransformPosMeshToModel(m->verts[tri.p3]);
	snapDistance = gtp1.DistanceTo(gtp2);
	float elen = gtp2.DistanceTo(gtp3);
	if (snapDistance > elen)
		snapDistance = elen;
	elen = gtp3.DistanceTo(gtp1);
	if (snapDistance > elen)
		snapDistance = elen;

	UndoStateProject* usp = os->editor.undoHistory.PushState();
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = lastHitResult.hoverMeshCoord;

	if (mouseDownMirrorPoint != -1)
		usp->usss[0].pointEndState[mouseDownMirrorPoint] = usp->usss[0].pointStartState[mouseDownMirrorPoint] = m->verts[mouseDownMirrorPoint];

	if (os->editor.bEditSlider) {
		usp->sliderName = os->editor.activeSlider;
		float sliderscale = os->project->SliderValue(os->editor.activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	wxPoint p;
	gls.ProjectPointToScreen(lastHitResult.hoverRealCoord, p.x, p.y);
	mouseDownOffset = screenPos - p;

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateMoveVertex(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;
	UndoStateShape& uss = usp->usss[0];

	// Restore original position in m so CollideMeshes and IntersectSphere
	// will work.
	Vector3 mesholdpos = uss.pointStartState[mouseDownPoint];
	m->verts[mouseDownPoint] = mesholdpos;
	if (mouseDownMirrorPoint != -1)
		m->verts[mouseDownMirrorPoint] = uss.pointStartState[mouseDownMirrorPoint];
	Vector3 oldpos = m->TransformPosMeshToModel(mesholdpos);

	// Since the pointer can be offset from screenPos, calculate a point
	// to display at screenPos.  We do this by intersecting the
	// screenPos ray with the plane perpendicular to mouseDownViewDir at oldpos.
	// This will also be our default for newpos if all tool options are off.
	Vector3 pointerPoint;
	gls.CollidePlane(screenPos.x, screenPos.y, pointerPoint, mouseDownViewDir, mouseDownViewDir.dot(oldpos));

	// Determining newpos: 1. intersect screenPos-ray with plane or surface.
	Vector3 newpos = pointerPoint;
	moveVertexOperation = MoveVertexOperation::Move;
	if (toolOptionRestrictSurface) {
		Vector3 hitpt, hitnormal;
		Mesh* hitmesh = nullptr;
		bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, hitpt, hitnormal, false, &hitmesh);
		if (!hit || !hitmesh) {
			newpos = oldpos;
			moveVertexOperation = MoveVertexOperation::None;
		}
		else
			newpos = hitmesh->TransformPosMeshToModel(hitpt);
	}
	else if (toolOptionRestrictPlane) {
		gls.CollidePlane(screenPos.x, screenPos.y, newpos, mouseDownPointNormal, mouseDownPointNormal.dot(oldpos));
	}

	// 2. Project onto normal
	if (toolOptionRestrictNormal && moveVertexOperation != MoveVertexOperation::None) {
		newpos = oldpos + mouseDownPointNormal * mouseDownPointNormal.dot(newpos - oldpos);
	}

	// 3. Snap to nearest valid point
	if ((toolOptionMerge || toolOptionWeld) && moveVertexOperation != MoveVertexOperation::None) {
		// Get adjacent points (not including welded points)
		std::unordered_set<int> adjPts;
		if (m->vertEdges && m->edges) {
			for (int ei : m->vertEdges[mouseDownPoint]) {
				const Edge& edge = m->edges[ei];
				if (edge.p1 != mouseDownPoint)
					adjPts.insert(edge.p1);
				if (edge.p2 != mouseDownPoint)
					adjPts.insert(edge.p2);
			}
		}

		int closestPoint = -1;
		Mesh* closestMesh = nullptr;
		float closestDist = snapDistance;

		for (Mesh* tm : gls.GetActiveMeshes()) {
			Vector3 viewDir, viewOrigin;
			gls.GetPickRay(screenPos.x, screenPos.y, tm, viewDir, viewOrigin);

			Vector3 tmnewpos = tm->TransformPosModelToMesh(newpos);
			float tmSnapDistance = tm->TransformDistModelToMesh(snapDistance);

			std::vector<IntersectResult> iresults;
			if (tm->bvh && tm->bvh->IntersectSphere(tmnewpos, tmSnapDistance, &iresults)) {
				for (const IntersectResult& ir : iresults) {
					const Triangle& t = tm->tris[ir.HitFacet];
					for (int tvi = 0; tvi < 3; ++tvi) {
						int tp = t[tvi];
						if (tm == m && tp == mouseDownPoint)
							continue;
						if (tm == m && adjPts.count(tp) != 0)
							continue;

						// Calculate distance in plane perpendicular to view
						Vector3 diff = tm->verts[tp] - tmnewpos;
						diff -= viewDir * viewDir.dot(diff);
						float d = tm->TransformDistMeshToModel(diff.length());
						if (d >= closestDist)
							continue;

						closestPoint = tp;
						closestMesh = tm;
						closestDist = d;
					}
				}
			}
		}

		if (closestPoint != -1) {
			NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
			NiShape* s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(closestMesh->shapeName);
			bool canWeld = toolOptionWeld && s1 && s2 && os->project->PointsHaveDifferingWeightsOrDiffs(s1, mouseDownPoint, s2, closestPoint);
			bool canMerge = toolOptionMerge && m == closestMesh;
			if (canWeld || canMerge) {
				newpos = closestMesh->TransformPosMeshToModel(closestMesh->verts[closestPoint]);
				if (canWeld)
					moveVertexOperation = MoveVertexOperation::Weld;
				else if (canMerge)
					moveVertexOperation = MoveVertexOperation::Merge;
				moveVertexTarget = closestPoint;
				moveVertexWeldTargetMeshName = closestMesh->shapeName;
			}
		}
	}

	Vector3 meshnewpos = m->TransformPosModelToMesh(newpos);
	uss.pointEndState[mouseDownPoint] = meshnewpos;
	m->verts[mouseDownPoint] = meshnewpos;
	gls.SetPointCursor(newpos);
	gls.SetCenterCursor(pointerPoint);
	m->QueueUpdate(Mesh::UpdateType::Position);
	gls.ShowCursor(true);

	if (mouseDownMirrorPoint != -1) {
		Vector3 mp = newpos;
		mp.x = -mp.x;
		mp = m->TransformPosModelToMesh(mp);
		uss.pointEndState[mouseDownMirrorPoint] = mp;
		m->verts[mouseDownMirrorPoint] = mp;
		gls.ShowMirrorPointCursor(mp, m);
	}
}

void wxGLPanel::EndMoveVertex() {
	isMovingVertex = false;
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	if (moveVertexOperation == MoveVertexOperation::None) {
		CancelMoveVertex();
		return;
	}

	bool isWeld = moveVertexOperation == MoveVertexOperation::Weld;
	bool isMerge = moveVertexOperation == MoveVertexOperation::Merge;

	if (isWeld || isMerge) {
		NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		NiShape* s2 = s1;
		if (isWeld)
			s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(moveVertexWeldTargetMeshName);

		bool p1b = os->project->IsVertexOnBoundary(s1, mouseDownPoint);
		bool p2b = os->project->IsVertexOnBoundary(s2, moveVertexTarget);

		if (!p1b || !p2b) {
			int response = wxMessageBox(
				!p1b ? !p2b ? _("Neither the selected nor target vertices are on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
							: _("The selected vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
					 : _("The target vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?"),
				_("Weld/Merge Non-Boundary Vertices"),
				wxOK | wxCANCEL | wxCANCEL_DEFAULT,
				os);

			if (response == wxCANCEL) {
				CancelMoveVertex();
				return;
			}
		}

		usp->undoType = UndoType::Mesh;
		usp->sliderName.clear();
		usp->usss[0].pointStartState.clear();
		usp->usss[0].pointEndState.clear();

		if (isWeld)
			os->project->PrepareWeldVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget, s2);

		if (isMerge)
			os->project->PrepareMergeVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget);
	}

	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelMoveVertex() {
	UndoStateProject* usp = os->editor.undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	os->editor.undoHistory.PopState();
}

bool wxGLPanel::StartPickEdge() {
	if (lastHitResult.hitMeshName.empty() || (lastHitResult.hoverEdge.p1 == 0 && lastHitResult.hoverEdge.p2 == 0))
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownEdge = lastHitResult.hoverEdge;
	return true;
}

void wxGLPanel::UpdatePickEdge(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || !hitResult.hoverEdge.CompareIndices(mouseDownEdge))
		gls.HideSegCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickEdge() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || !lastHitResult.hoverEdge.CompareIndices(mouseDownEdge))
		return;

	// Clear PickEdge state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HideSegCursor();

	if (activeTool == ToolID::FlipEdge)
		ClickFlipEdge();
	if (activeTool == ToolID::SplitEdge)
		ClickSplitEdge();
}

void wxGLPanel::ClickFlipEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareFlipEdge(shape, uss, mouseDownEdge)) {
		wxMessageBox(_("The edge picked is on the surface boundary.  Pick an interior edge."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::ClickSplitEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	bool shiftDown = wxGetKeyState(WXK_SHIFT);

	constexpr uint16_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	uint32_t maxTriIndex = std::numeric_limits<uint16_t>().max();

	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	if (shape->GetNumVertices() > maxVertIndex - 2) {
		wxMessageBox(_("The shape has reached the vertex count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	if (shape->GetNumTriangles() > maxTriIndex - 2) {
		wxMessageBox(_("The shape has reached the triangle count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	std::vector<bool> pincs(shape->GetNumVertices(), false);
	pincs[mouseDownEdge.p1] = true;
	pincs[mouseDownEdge.p2] = true;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	bool noCurveOffset = shiftDown;
	if (!os->project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, noCurveOffset)) {
		wxMessageBox(_("The edge picked has multiple triangles of the same orientation.  Correct the orientations before splitting."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

std::unordered_map<std::string, std::vector<float>> wxGLPanel::StashMasks() {
	std::unordered_map<std::string, std::vector<float>> stash;
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive || !m->mask)
			continue;
		std::vector<float>& mask = stash[m->shapeName];
		mask.resize(m->nVerts);
		std::copy(m->mask.get(), m->mask.get() + m->nVerts, mask.begin());
	}
	return stash;
}

void wxGLPanel::UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) {
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive)
			continue;

		auto stit = stash.find(m->shapeName);
		if (stit == stash.end())
			continue;
		const std::vector<float>& mask = stit->second;
		// assert(mask.size() == m->nVerts);
		if (static_cast<int>(mask.size()) != m->nVerts)
			continue;
		std::copy(mask.begin(), mask.end(), m->mask.get());
		m->QueueUpdate(Mesh::UpdateType::Mask);
	}
}

void wxGLPanel::ApplyMaskChanges(const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) {
	ApplyMaskChanges(gls.GetActiveMeshes(), changesForMesh);
}

void wxGLPanel::ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& m : meshes) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (auto& change : changesForMesh(*m)) {
			uss.pointStartState[change.first].x = m->mask[change.first];
			uss.pointEndState[change.first].x = change.second;
		}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}

void wxGLPanel::MaskLess() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::GrowShrinkChanges(m, false); });
}

void wxGLPanel::MaskMore() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::GrowShrinkChanges(m, true); });
}

void wxGLPanel::InvertMask() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::InvertChanges(m); });
}

void wxGLPanel::ClearMask() {
	ApplyMaskChanges([](Mesh& m) { return MaskOps::ClearChanges(m); });
}

void wxGLPanel::InvertMaskTris(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	std::unordered_map<uint16_t, float> triMask;
	for (int t = 0; t < m->nTris; t++) {
		auto& tri = m->tris[t];
		if (mask.find(tri.p1) != mask.end() || mask.find(tri.p2) != mask.end() || mask.find(tri.p3) != mask.end()) {
			triMask.emplace(tri.p1, 1.0f);
			triMask.emplace(tri.p2, 1.0f);
			triMask.emplace(tri.p3, 1.0f);
		}
	}

	std::unordered_map<uint16_t, float> invertMask;
	for (int t = 0; t < m->nTris; t++) {
		auto& tri = m->tris[t];
		if (triMask.find(tri.p1) == triMask.end())
			invertMask.emplace(tri.p1, 1.0f);
		if (triMask.find(tri.p2) == triMask.end())
			invertMask.emplace(tri.p2, 1.0f);
		if (triMask.find(tri.p3) == triMask.end())
			invertMask.emplace(tri.p3, 1.0f);
	}

	mask = std::move(invertMask);
}

nifly::Vector3 wxGLPanel::CreateColorRamp(const float value) {
	float r;
	float g;
	float b;

	if (value <= 0.0f) {
		r = g = b = 1.0;
	}
	else if (value <= 0.25) {
		r = 0.0;
		b = 1.0;
		g = value / 0.25;
	}
	else if (value <= 0.5) {
		r = 0.0;
		g = 1.0;
		b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
	}
	else if (value <= 0.75) {
		r = (value - 0.5) / 0.25;
		g = 1.0;
		b = 0.0;
	}
	else {
		r = 1.0;
		g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
		b = 0.0;
	}

	return nifly::Vector3(r, g, b);
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

bool wxGLPanel::RestoreMode(UndoStateProject* usp) {
	bool modeChanged = false;
	UndoType undoType = usp->undoType;
	if (undoType == UndoType::VertexPosition && os->editor.activeSlider != usp->sliderName) {
		os->EnterSliderEdit(usp->sliderName);
		modeChanged = true;
	}
	if ((undoType != UndoType::VertexPosition || usp->sliderName.empty()) && os->editor.bEditSlider) {
		os->ExitSliderEdit();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && !usp->sliderName.empty() && usp->sliderscale != os->project->SliderValue(usp->sliderName)) {
		os->SetSliderValue(usp->sliderName, usp->sliderscale * 100);
		os->ApplySliders();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && usp->sliderName.empty() && !os->project->AllSlidersZero()) {
		os->ZeroSliders();
		modeChanged = true;
	}
	return modeChanged;
}

void wxGLPanel::ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender) {
	UndoType undoType = usp->undoType;
	std::unordered_map<std::string, std::vector<float>> maskStash;
	if (undoType == UndoType::Weight) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& bw : uss.boneWeights) {
				if (bw.boneName != os->GetActiveBone())
					continue;

				for (auto& wIt : bw.weights)
					m->weight[wIt.first] = bUndo ? wIt.second.startVal : wIt.second.endVal;
			}

			m->QueueUpdate(Mesh::UpdateType::Weight);
		}

		os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UndoType::Mask) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->mask[pit.first] = pit.second.x;

			m->QueueUpdate(Mesh::UpdateType::Mask);
		}
	}
	else if (undoType == UndoType::Color) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->vcolors[pit.first] = pit.second;

			m->QueueUpdate(Mesh::UpdateType::VertexColors);
		}
	}
	else if (undoType == UndoType::Alpha) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->valpha[pit.first] = pit.second.x;

			m->QueueUpdate(Mesh::UpdateType::VertexAlpha);
		}

		os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UndoType::VertexPosition) {
		auto applyUVState = [&](UndoStateShape& uss) {
			if (!usp->sliderName.empty())
				return;

			const auto& uvState = bUndo ? uss.uvStartState : uss.uvEndState;
			if (uvState.empty())
				return;

			Mesh* m = GetMesh(uss.shapeName);
			if (m && m->texcoord) {
				for (auto& uvIt : uvState) {
					if (uvIt.first >= 0 && uvIt.first < m->nVerts)
						m->texcoord[uvIt.first] = uvIt.second;
				}

				m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
			}

			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				return;

			std::vector<Vector2> uvs;
			if (!os->project->GetWorkNif()->GetUvsForShape(shape, uvs))
				return;

			for (auto& uvIt : uvState) {
				if (uvIt.first >= 0 && uvIt.first < static_cast<int>(uvs.size()))
					uvs[uvIt.first] = uvIt.second;
			}

			os->project->GetWorkNif()->SetUvsForShape(shape, uvs);
		};

		bool hasRestDiffs = false;
		for (auto& uss : usp->usss) {
			if (!uss.restDiffs.empty()) {
				hasRestDiffs = true;
				break;
			}
		}

		if (hasRestDiffs) {
			// Pose-independent undo/redo: use stored rest-space NIF diffs
			if (usp->sliderName.empty()) {
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (!shape)
						continue;

					std::vector<Vector3> restVerts;
					os->project->GetWorkNif()->GetVertsForShape(shape, restVerts);

					for (auto& rd : uss.restDiffs) {
						if (bUndo)
							restVerts[rd.first] -= rd.second;
						else
							restVerts[rd.first] += rd.second;
					}

					os->project->GetWorkNif()->SetVertsForShape(shape, restVerts);
				}
			}

			os->ActiveShapesUpdated(usp, bUndo);

			for (auto& uss : usp->usss) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
				if (!shape)
					continue;

				std::vector<Vector3> verts;
				os->project->GetLiveVerts(shape, verts);
				UpdateMeshVertices(shape->name.get(), &verts, true, true, false);

				Mesh* m = GetMesh(uss.shapeName);
				if (m)
					m->CalcWeldVerts();

				applyUVState(uss);
			}
		}
		else {
			for (auto& uss : usp->usss) {
				Mesh* m = GetMesh(uss.shapeName);
				if (!m)
					continue;

				for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
					m->verts[pit.first] = pit.second;

				m->CalcWeldVerts();
				m->SmoothNormals();
				BVHUpdateQueue.insert(m);

				m->QueueUpdate(Mesh::UpdateType::Position);

				applyUVState(uss);
			}

			os->ActiveShapesUpdated(usp, bUndo);

			if (usp->sliderName.empty()) {
				for (auto& uss : usp->usss) {
					Mesh* m = GetMesh(uss.shapeName);
					if (!m)
						continue;

					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						os->project->UpdateShapeFromMesh(shape, m);
				}

				if (os->project->bPose) {
					for (auto& uss : usp->usss) {
						auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
						if (shape) {
							std::vector<Vector3> verts;
							os->project->GetLiveVerts(shape, verts);
							UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
						}
					}
				}
			}
			else if (os->project->bPose) {
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape) {
						std::vector<Vector3> verts;
						os->project->GetLiveVerts(shape, verts);
						UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
					}
				}
			}
		}
	}
	else if (undoType == UndoType::Mesh) {
		maskStash = StashMasks();

		// Undo: restore deleted shapes before processing vertex mesh undos
		if (bUndo) {
			for (auto& ds : usp->deletedShapes)
				os->project->RestoreDeletedShape(ds);
		}

		for (auto& uss : usp->usss) {
			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			os->project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, bUndo);
		}

		// Redo: re-delete shapes after processing vertex mesh redos
		if (!bUndo) {
			for (auto& ds : usp->deletedShapes) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(ds.shapeName);
				if (shape)
					os->project->DeleteShape(shape);
			}
		}

		os->RefreshGUIFromProj(false, false);
		UnstashMasks(maskStash);
		os->ApplySliders();
	}
	else if (undoType == UndoType::ShapeDelete) {
		if (bUndo) {
			for (auto& ds : usp->deletedShapes)
				os->project->RestoreDeletedShape(ds);
		}
		else {
			for (auto& ds : usp->deletedShapes) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(ds.shapeName);
				if (shape)
					os->project->DeleteShape(shape);
			}
		}

		os->RefreshGUIFromProj(false, false);
		os->ApplySliders();
	}
	else if (undoType == UndoType::Mirror) {
		for (auto& uss : usp->usss) {
			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			os->project->GetWorkNif()->MirrorShape(shape, usp->mirrorX, usp->mirrorY, usp->mirrorZ);
			if (usp->swapBonesX)
				os->project->GetWorkAnim()->SwapBonesLR(uss.shapeName);
		}

		os->RefreshGUIFromProj(false);
	}
	else if (undoType == UndoType::NodeTransform) {
		const MatTransform& target = bUndo ? usp->nodeStartXformToParent : usp->nodeEndXformToParent;

		if (AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(usp->boneName))
			bPtr->SetTransformBoneToParent(target);

		auto* nif = os->project->GetWorkNif();
		if (nif) {
			nif->SetNodeTransformToParent(usp->boneName, target);
			if (auto* workAnim = os->project->GetWorkAnim())
				workAnim->WriteNodesToNif(nif);

			// Re-skin all shapes so poses and bind-pose display both follow
			// the restored bone transform.
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		UpdateBones();
		UpdateNodes();
		os->RefreshBoneTreeBadBoneIcons();
		os->SetPendingChanges();
	}
	else if (undoType == UndoType::BoneEdit) {
		os->project->ApplyBoneXforms(usp->boneXforms, bUndo);

		// The skin binds were recalculated, so this only changes posed meshes
		if (auto* nif = os->project->GetWorkNif()) {
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		UpdateBones();
		UpdateNodes();
		os->RefreshBoneTreeBadBoneIcons();
		os->SetPendingChanges();
	}

	if (bRender) {
		if (transformMode)
			ShowTransformTool();
		else
			Render();
	}
}

bool wxGLPanel::UndoStroke() {
	UndoStateProject* curState = os->editor.undoHistory.GetCurState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!os->editor.undoHistory.BackStepHistory())
		return false;

	ApplyUndoState(curState, true);

	return true;
}

bool wxGLPanel::RedoStroke() {
	UndoStateProject* curState = os->editor.undoHistory.GetNextState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!os->editor.undoHistory.ForwardStepHistory())
		return false;

	ApplyUndoState(curState, false);

	return true;
}

void wxGLPanel::ShowRotationCenter(bool show) {
	if (show) {
		RotationCenterMesh = gls.AddVis3dSphere(gls.camRotOffset, 0.08f, Vector3(1.0f, 0.0f, 1.0f), "RotationCenterMesh");
		RotationCenterMesh->prop.alpha = 0.25f;
		RotationCenterMesh->bVisible = true;

		RotationCenterMeshRingX = gls.AddVis3dRing(gls.camRotOffset, Vector3(1.0f, 0.0f, 0.0f), 0.25f, 0.01f, Vector3(1.0f, 0.0f, 0.0f), "RotationCenterMeshRingX");
		RotationCenterMeshRingX->prop.alpha = 0.25f;
		RotationCenterMeshRingX->bVisible = true;

		RotationCenterMeshRingY = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 1.0f, 0.0f), 0.25f, 0.01f, Vector3(0.0f, 1.0f, 0.0f), "RotationCenterMeshRingY");
		RotationCenterMeshRingY->prop.alpha = 0.25f;
		RotationCenterMeshRingY->bVisible = true;

		RotationCenterMeshRingZ = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 0.0f, 1.0f), 0.25f, 0.01f, Vector3(0.0f, 0.0f, 1.0f), "RotationCenterMeshRingZ");
		RotationCenterMeshRingZ->prop.alpha = 0.25f;
		RotationCenterMeshRingZ->bVisible = true;
	}
	else {
		if (RotationCenterMesh) {
			RotationCenterMesh->bVisible = false;
			RotationCenterMeshRingX->bVisible = false;
			RotationCenterMeshRingY->bVisible = false;
			RotationCenterMeshRingZ->bVisible = false;
		}
	}
}

void wxGLPanel::ShowTransformTool(bool show) {
	UpdateTransformCenter();

	if (show) {
		XMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		XMoveMesh->prop.alpha = 0.6f;
		YMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		YMoveMesh->prop.alpha = 0.6f;
		ZMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");
		ZMoveMesh->prop.alpha = 0.6f;

		XRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 1.25f, 0.04f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		XRotateMesh->prop.alpha = 0.6f;
		YRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 1.25f, 0.04f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		YRotateMesh->prop.alpha = 0.6f;
		ZRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 1.25f, 0.04f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");
		ZRotateMesh->prop.alpha = 0.6f;

		XScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.75f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), 0.12f, Vector3(1.0f, 0.0f, 0.0f), "XScaleMesh");
		YScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.75f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.12f, Vector3(0.0f, 1.0f, 0.0f), "YScaleMesh");
		ZScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.0f, 0.75f), Vector3(0.0f, 0.0f, 1.0f), 0.12f, Vector3(0.0f, 0.0f, 1.0f), "ZScaleMesh");
		ScaleUniformMesh = gls.AddVis3dCube(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.15f, Vector3(0.0f, 0.0f, 0.0f), "ScaleUniformMesh");

		lastCenterDistance = 0.0f;

		XMoveMesh->bVisible = true;
		YMoveMesh->bVisible = true;
		ZMoveMesh->bVisible = true;
		XRotateMesh->bVisible = true;
		YRotateMesh->bVisible = true;
		ZRotateMesh->bVisible = true;
		XScaleMesh->bVisible = true;
		YScaleMesh->bVisible = true;
		ZScaleMesh->bVisible = true;
		ScaleUniformMesh->bVisible = true;

		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}

		if (nodesMode || bonesMode) {
			// Only move and rotate gizmos are supported for bones/nodes.
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}
	}
	else {
		if (XMoveMesh) {
			XMoveMesh->bVisible = false;
			YMoveMesh->bVisible = false;
			ZMoveMesh->bVisible = false;
			XRotateMesh->bVisible = false;
			YRotateMesh->bVisible = false;
			ZRotateMesh->bVisible = false;
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}

		if (XPivotMesh && pivotMode) {
			XPivotMesh->bVisible = true;
			YPivotMesh->bVisible = true;
			ZPivotMesh->bVisible = true;
			PivotCenterMesh->bVisible = true;
		}
	}

	UpdateTransformTool();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdateTransformCenter() {
	bool centerSet = false;

	if (pivotMode) {
		xformCenter = pivotPosition;
		centerSet = true;
	}
	else if (nodesMode || bonesMode) {
		std::string activeBone = os->GetActiveBone();
		if (!activeBone.empty()) {
			auto boneMesh = gls.GetOverlay("BP_" + activeBone);
			if (boneMesh) {
				xformCenter = boneMesh->verts[0];
				centerSet = true;
			}
			else {
				auto nodeMesh = gls.GetOverlay("P_" + activeBone);
				if (nodeMesh) {
					xformCenter = nodeMesh->verts[0];
					centerSet = true;
				}
			}
		}
	}

	if (!centerSet)
		xformCenter = gls.GetActiveCenter();
}

void wxGLPanel::UpdateTransformTool() {
	if (!transformMode)
		return;

	if (!XMoveMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterDistance != 0.0f) {
		float factor = 1.0f / lastCenterDistance;
		XMoveMesh->ScaleVertices(xformCenter, factor);
		YMoveMesh->ScaleVertices(xformCenter, factor);
		ZMoveMesh->ScaleVertices(xformCenter, factor);

		XRotateMesh->ScaleVertices(xformCenter, factor);
		YRotateMesh->ScaleVertices(xformCenter, factor);
		ZRotateMesh->ScaleVertices(xformCenter, factor);

		XScaleMesh->ScaleVertices(xformCenter, factor);
		YScaleMesh->ScaleVertices(xformCenter, factor);
		ZScaleMesh->ScaleVertices(xformCenter, factor);
		ScaleUniformMesh->ScaleVertices(xformCenter, factor);
	}

	lastCenterDistance = unprojected.DistanceTo(xformCenter) / 15.0f;

	XMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ScaleUniformMesh->ScaleVertices(xformCenter, lastCenterDistance);
}

void wxGLPanel::ShowPivot(bool show) {
	if (show) {
		XPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(1.0f, 0.0f, 0.0f), "XPivotMesh");
		YPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 1.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 1.0f, 0.0f), "YPivotMesh");
		ZPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 0.0f, 1.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 0.0f, 1.0f), "ZPivotMesh");
		PivotCenterMesh = gls.AddVis3dCube(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.075f, Vector3(0.0f, 0.0f, 0.0f), "PivotCenterMesh");

		XPivotMesh->bVisible = true;
		YPivotMesh->bVisible = true;
		ZPivotMesh->bVisible = true;
		PivotCenterMesh->bVisible = true;

		lastCenterPivotDistance = 0.0f;
	}
	else {
		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}
	}

	UpdatePivot();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdatePivot() {
	if (!pivotMode)
		return;

	if (!XPivotMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterPivotDistance != 0.0f) {
		float factor = 1.0f / lastCenterPivotDistance;
		XPivotMesh->ScaleVertices(pivotPosition, factor);
		YPivotMesh->ScaleVertices(pivotPosition, factor);
		ZPivotMesh->ScaleVertices(pivotPosition, factor);
		PivotCenterMesh->ScaleVertices(pivotPosition, factor);
	}

	lastCenterPivotDistance = unprojected.DistanceTo(pivotPosition) / 15.0f;

	XPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	YPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	ZPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	PivotCenterMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
}

void wxGLPanel::ShowNodes(bool show) {
	nodesMode = show;

	for (auto& m : nodesPoints)
		m->bVisible = show;

	for (auto& m : nodesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateNodes() {
	for (auto& m : nodesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : nodesLines)
		gls.DeleteOverlay(m);

	nodesPoints.clear();
	nodesLines.clear();

	auto workNif = os->project->GetWorkNif();

	std::function<void(NiNode*, NiNode*, const Vector3&, const Vector3&)> addChildNodes =
		[&](NiNode* node, NiNode* parent, const Vector3& rootPosition, const Vector3& parentPosition) {
			MatTransform ttg = node->GetTransformToParent();
			NiNode* parentNode = parent;
			while (parentNode) {
				ttg = parentNode->GetTransformToParent().ComposeTransforms(ttg);
				parentNode = workNif->GetParentNode(parentNode);
			}

			Vector3 position = ttg.ApplyTransform(rootPosition);

			std::string nodeName = node->name.get();
			if (!nodeName.empty()) {
				Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

				auto pointMesh = gls.AddVisPoint(renderPosition, "P_" + nodeName);
				if (pointMesh) {
					pointMesh->bVisible = nodesMode;
					nodesPoints.push_back(pointMesh);
				}

				if (parent) {
					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "L_" + nodeName);
					if (lineMesh) {
						lineMesh->bVisible = nodesMode;
						nodesLines.push_back(lineMesh);
					}
				}
			}

			for (auto& child : node->childRefs) {
				auto childNode = workNif->GetHeader().GetBlock<NiNode>(child);
				if (childNode)
					addChildNodes(childNode, node, rootPosition, position);
			}
		};

	auto rootNode = workNif->GetRootNode();
	if (rootNode)
		addChildNodes(rootNode, nullptr, rootNode->transform.translation, rootNode->transform.translation);

	UpdateNodeColors();
}

void wxGLPanel::ShowBones(bool show) {
	bonesMode = show;

	if (!show && !hoverBone.empty()) {
		hoverBone.clear();
		UpdateNodeColors();
	}

	for (auto& m : bonesPoints)
		m->bVisible = show;

	for (auto& m : bonesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateBones() {
	for (auto& m : bonesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : bonesLines)
		gls.DeleteOverlay(m);

	bonesPoints.clear();
	bonesLines.clear();
	connectorBones.clear();

	auto workAnim = os->project->GetWorkAnim();
	bool isSF = os->project->GetWorkNif()->GetHeader().GetVersion().IsSF();

	std::function<bool(AnimBone*)> addChildBones = [&](AnimBone* parent) {
		bool anyBoneInSelection = false;

		for (auto& cb : parent->children) {
			bool childBonesInSelection = addChildBones(cb);

			if (!cb->boneName.empty()) {
				bool boneInSelection = false;
				for (auto& si : os->GetSelectedItems()) {
					if (workAnim->GetShapeBoneIndex(si->GetShape()->name.get(), cb->boneName) != -1) {
						boneInSelection = true;
						break;
					}
				}

				if (boneInSelection || childBonesInSelection) {
					if (!boneInSelection)
						connectorBones.insert(cb->boneName);

					const MatTransform& toGlobal = os->project->bPose ? cb->xformPoseToGlobal : cb->xformToGlobal;
					Vector3 position = toGlobal.ApplyTransform(Vector3());
					const MatTransform& parentToGlobal = os->project->bPose ? parent->xformPoseToGlobal : parent->xformToGlobal;
					Vector3 parentPosition = parentToGlobal.ApplyTransform(Vector3());

					if (isSF) {
						position *= sfHavokScale;
						parentPosition *= sfHavokScale;
					}

					bool matchesParent = position.IsNearlyEqualTo(parentPosition);

					Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

					auto pointMesh = gls.AddVisPoint(renderPosition, "BP_" + cb->boneName);
					if (pointMesh) {
						pointMesh->bVisible = bonesMode;
						bonesPoints.push_back(pointMesh);
					}

					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					if (!matchesParent) {
						auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "BL_" + cb->boneName);
						if (lineMesh) {
							lineMesh->bVisible = bonesMode;
							bonesLines.push_back(lineMesh);
						}
					}

					anyBoneInSelection = true;
				}
			}
		}

		return anyBoneInSelection;
	};

	AnimBone* rb = AnimSkeleton::getInstance().GetRootBonePtr();
	if (rb)
		addChildBones(rb);

	UpdateNodeColors();
}

void wxGLPanel::UpdateNodeColors() {
	std::string activeBone = os->GetActiveBone();

	// The active bone (whose weights are shown and edited) is red, other selected bones are pink.
	// Selected nodes and bones are drawn on top.
	for (auto& m : nodesPoints) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color = Vector3(1.0f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (os->IsBoneSelected(nodeName)) {
			m->color = Vector3(1.0f, 0.35f, 0.75f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color = Vector3(0.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : nodesLines) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color = Vector3(0.7f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (os->IsBoneSelected(nodeName)) {
			m->color = Vector3(0.7f, 0.25f, 0.53f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color = Vector3(0.0f, 0.7f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	// Hovered bones are yellow, or orange when also active, and drawn on top.
	// Connector bones are grey unless selected.
	for (auto& m : bonesPoints) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		const bool active = nodeName == activeBone;
		const bool selected = !active && os->IsBoneSelected(nodeName);
		if (!hoverBone.empty() && nodeName == hoverBone) {
			m->color = active ? Vector3(1.0f, 0.5f, 0.0f) : Vector3(1.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (active) {
			m->color = Vector3(1.0f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (selected) {
			m->color = Vector3(1.0f, 0.35f, 0.75f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (connectorBones.count(nodeName) != 0) {
			m->color = Vector3(0.4f, 0.4f, 0.4f);
			m->overlayLayer = OverlayLayer::Default;
		}
		else {
			m->color = Vector3(0.0f, 1.0f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : bonesLines) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		const bool active = nodeName == activeBone;
		const bool selected = !active && os->IsBoneSelected(nodeName);
		if (!hoverBone.empty() && nodeName == hoverBone) {
			m->color = active ? Vector3(0.9f, 0.45f, 0.0f) : Vector3(0.9f, 0.9f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (active) {
			m->color = Vector3(0.7f, 0.0f, 0.0f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (selected) {
			m->color = Vector3(0.7f, 0.25f, 0.53f);
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else if (connectorBones.count(nodeName) != 0) {
			m->color = Vector3(0.35f, 0.35f, 0.35f);
			m->overlayLayer = OverlayLayer::Default;
		}
		else {
			m->color = Vector3(0.0f, 0.7f, 0.0f);
			m->overlayLayer = OverlayLayer::Default;
		}
	}
}

void wxGLPanel::SetHoverBone(const std::string& bone) {
	if (bone == hoverBone)
		return;

	hoverBone = bone;
	UpdateNodeColors();
}

std::string wxGLPanel::PickBone(int x, int y, bool includeConnectors) {
	if (!bonesMode || !os || os->IsAnimationPlaying())
		return std::string();

	const float pointRadius = FromDIP(8);
	const float lineRadius = FromDIP(4);
	// Joints closer together on screen than this count as overlapping.
	const float tieTolerance = FromDIP(1);
	const std::string activeBone = os->GetActiveBone();
	const Vector3 cursor(static_cast<float>(x), static_cast<float>(y), 0.0f);

	// Overlapping joints, like a new bone still on its parent, prefer the
	// active bone and then the one nearest to the camera.
	Mesh* best = nullptr;
	float bestDist = 0.0f;
	float bestDepth = 0.0f;
	bool bestActive = false;

	// A skipped connector joint under the cursor also hides the lines starting
	// there, so it doesn't pick the bone below it instead.
	bool connectorJointHit = false;

	auto consider = [&](Mesh* m, float dist, float depth) {
		if (!includeConnectors && connectorBones.count(m->shapeName.substr(3)) != 0) {
			if (m->nVerts == 1)
				connectorJointHit = true;
			return;
		}

		const bool active = m->shapeName.compare(3, std::string::npos, activeBone) == 0;
		bool better = !best || dist < bestDist - tieTolerance;
		if (!better && std::fabs(dist - bestDist) <= tieTolerance) {
			if (active != bestActive)
				better = active;
			else
				better = depth < bestDepth;
		}

		if (better) {
			best = m;
			bestDist = dist;
			bestDepth = depth;
			bestActive = active;
		}
	};

	for (auto& m : bonesPoints) {
		if (!m->bVisible || m->nVerts < 1)
			continue;

		float sx, sy, depth;
		if (!gls.ProjectPointToViewport(m->verts[0], sx, sy, &depth))
			continue;

		const float dist = Vector3(sx, sy, 0.0f).DistanceTo(cursor);
		if (dist <= pointRadius)
			consider(m, dist, depth);
	}

	// Segments belong to the bone at their end, away from the parent.
	if (!best && !connectorJointHit) {
		for (auto& m : bonesLines) {
			if (!m->bVisible || m->nVerts < 2)
				continue;

			float sx0, sy0, depth0, sx1, sy1, depth1;
			if (!gls.ProjectPointToViewport(m->verts[0], sx0, sy0, &depth0) || !gls.ProjectPointToViewport(m->verts[1], sx1, sy1, &depth1))
				continue;

			const Vector3 a(sx0, sy0, 0.0f);
			const Vector3 ab = Vector3(sx1, sy1, 0.0f) - a;
			const float lenSq = ab.dot(ab);
			float t = lenSq > 0.0f ? (cursor - a).dot(ab) / lenSq : 0.0f;
			t = std::clamp(t, 0.0f, 1.0f);

			const float dist = (a + ab * t).DistanceTo(cursor);
			if (dist <= lineRadius)
				consider(m, dist, depth0 + (depth1 - depth0) * t);
		}
	}

	if (!best)
		return std::string();

	return best->shapeName.substr(3);
}

void wxGLPanel::ShowFloor(bool show) {
	floorMode = show;

	for (auto& m : floorMeshes)
		m->bVisible = show;

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateFloor() {
	for (auto& m : floorMeshes)
		gls.DeleteMesh(m);

	floorMeshes = gls.AddFloor();

	for (auto& m : floorMeshes)
		m->bVisible = floorMode;
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto& m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
			if (m) {
				m->bShowPoints = true;
				m->ComputeAvgEdgeLength();
				m->QueueUpdate(Mesh::UpdateType::Mask);
			}
		}
	}
}

void wxGLPanel::SetVertexEdit(bool on) {
	vertexEdit = on;
	ShowVertexEdit(on);
	Render();
}

void wxGLPanel::SetTransformMode(bool on) {
	transformMode = on;
	ShowTransformTool(on);
}

void wxGLPanel::SetPivotMode(bool on) {
	pivotMode = on;
	ShowPivot(on);

	if (transformMode)
		ShowTransformTool();
}

bool wxGLPanel::GetToolOptionXMirror() {
	if (activeTool == ToolID::WeightBrush)
		return toolOptionXMirrorWeight;
	return toolOptionXMirror;
}

void wxGLPanel::SetToolOptionXMirror(bool on) {
	if (activeTool == ToolID::WeightBrush)
		toolOptionXMirrorWeight = on;
	else
		toolOptionXMirror = on;
	gls.SetXMirrorCursor(on);
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

void wxGLPanel::SetBrushSize(float val) {
	val *= BrushSizeScale;
	brushSize = val;
	gls.SetCursorSize(val);
}

void wxGLPanel::ResetBrushSize() {
	brushSize = 0.45f;
	gls.SetCursorSize(brushSize);
}

float wxGLPanel::IncBrush(float step) {
	gls.SetCursorSize(gls.GetCursorSize() + step);
	brushSize += step;
	LimitBrushSize();
	return brushSize;
}

float wxGLPanel::DecBrush(float step) {
	gls.SetCursorSize(gls.GetCursorSize() - step);
	brushSize -= step;
	LimitBrushSize();
	return brushSize;
}

void wxGLPanel::LimitBrushSize() {
	if (brushSize < 0.000f) {
		gls.SetCursorSize(0.000f);
		brushSize = 0.000f;
	}
	else if (brushSize > BrushSizeScale) {
		gls.SetCursorSize(BrushSizeScale);
		brushSize = BrushSizeScale;
	}
}

float wxGLPanel::IncStr() {
	if (!activeBrush)
		return 0.0f;

	float str = activeBrush->getStrength();
	str += 0.010f;
	activeBrush->setStrength(str);
	return str;
}

float wxGLPanel::DecStr() {
	if (!activeBrush)
		return 0.0f;

	float str = activeBrush->getStrength();
	str -= 0.010f;
	activeBrush->setStrength(str);
	return str;
}

void wxGLPanel::ClearMasks() {
	for (auto& m : gls.GetMeshes())
		if (!m->bPrimitive)
			m->MaskFill(0.0f);
}

void wxGLPanel::ClearActiveMask() {
	for (auto& m : gls.GetActiveMeshes())
		m->MaskFill(0.0f);
}

void wxGLPanel::ClearColors() {
	for (auto& m : gls.GetMeshes())
		if (!m->bPrimitive)
			m->ColorFill(nifly::Vector3());
}

void wxGLPanel::ClearActiveColors() {
	for (auto& m : gls.GetActiveMeshes())
		m->ColorFill(nifly::Vector3());
}

void wxGLPanel::GetActiveMask(std::unordered_map<uint16_t, float>& mask) {
	if (gls.GetActiveMeshes().empty())
		return;

	Mesh* m = gls.GetActiveMeshes().back();

	for (int i = 0; i < m->nVerts; i++) {
		if (m->mask[i] != 0.0f)
			mask[i] = m->mask[i];
	}
}

void wxGLPanel::GetActiveUnmasked(std::unordered_map<uint16_t, float>& mask) {
	if (gls.GetActiveMeshes().empty())
		return;

	Mesh* m = gls.GetActiveMeshes().back();

	for (int i = 0; i < m->nVerts; i++)
		if (m->mask[i] == 0.0f)
			mask[i] = m->mask[i];
}

void wxGLPanel::GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++) {
		if (m->mask[i] != 0.0f)
			mask[i] = m->mask[i];
	}
}

void wxGLPanel::GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++)
		if (m->mask[i] == 0.0f)
			mask[i] = m->mask[i];
}

void wxGLPanel::SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	for (int i = 0; i < m->nVerts; i++) {
		if (mask.find(i) != mask.end())
			m->mask[i] = mask[i];
		else
			m->mask[i] = 0.0f;
	}

	m->QueueUpdate(Mesh::UpdateType::Mask);
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

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	int delt = event.GetWheelRotation();

	// Switching the edited slider and resizing the brush are both locked out
	// during playback; zooming falls through.
	const bool playing = os && os->IsAnimationPlaying();

	if (!playing && event.ControlDown()) {
		std::string sliderName = os->editor.lastActiveSlider;

		if (sliderName.empty())
			sliderName = os->project->GetSliderName(0);

		if (sliderName.empty())
			return;

		if (!os->editor.activeSlider.empty())
			os->ExitSliderEdit();

		size_t sliderCount = os->project->SliderCount();
		size_t sliderIndex = 0;
		if (os->project->SliderIndexFromName(sliderName, sliderIndex)) {
			if (delt < 0) {
				++sliderIndex;

				if (sliderIndex == sliderCount)
					sliderIndex = 0;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
			else {
				if (sliderIndex == 0)
					sliderIndex = sliderCount - 1;
				else
					--sliderIndex;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
		}
	}
	else if (!playing && wxGetKeyState(wxKeyCode('S'))) {
		wxPoint p = event.GetPosition();

		if (brushMode) {
			// Adjust brush size, in finer steps with Shift held
			float step = wxGetKeyState(WXK_SHIFT) ? 0.001f : 0.010f;
			if (delt < 0)
				DecBrush(step);
			else
				IncBrush(step);

			os->CheckBrushBounds();
			os->UpdateBrushSettings();
			gls.UpdateCursor(p.x, p.y, true);
			gls.RenderOneFrame();
		}
	}
	else {
		gls.DollyCamera(delt);
		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	if (os->IsActive())
		SetFocus();

	// Mouse motion floods the message queue, which starves both the idle events
	// and the WM_TIMER that drive playback and physics, so tick them from here
	// as well.
	os->PumpAnimationPlayback();
	os->PumpPhysics();

	bool cursorExists = false;
	int x;
	int y;
	event.GetPosition(&x, &y);

	mouseHasMovedSinceStart = true;

	ShowRotationCenter(false);

	if (mbuttonDown) {
		isMDragging = true;
		if (wxGetKeyState(wxKeyCode::WXK_SHIFT))
			gls.DollyCamera(y - lastY);
		else
			gls.PanCamera(x - lastX, y - lastY);

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (rbuttonDown) {
		// A right-click that barely moved still opens the bone context menu
		if (!isRDragging && std::abs(x - rDownPos.x) + std::abs(y - rDownPos.y) > FromDIP(3))
			isRDragging = true;

		if (wxGetKeyState(WXK_ALT)) {
			if (lastHitResult.hoverPoint != -1) {
				rotationCenterMode = RotationCenterMode::Picked;
				gls.camRotOffset = lastHitResult.hoverRealCoord;
			}
		}

		if (wxGetKeyState(WXK_SHIFT)) {
			gls.PanCamera(x - lastX, y - lastY);
		}
		else {
			float yawDegrees = gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
			os->InjectPhysicsCameraYaw(yawDegrees);
			ShowRotationCenter();
		}

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (lbuttonDown || isMovingVertex || isSlidingEdge) {
		isLDragging = true;
		if (isPhysicsGrabbing) {
			UpdatePhysicsGrab(event.GetPosition());
		}
		else if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else if (isMovingPivot) {
			UpdatePivotPosition(event.GetPosition());
		}
		else if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isSelecting) {
			SelectVertex(event.GetPosition());
		}
		else if (isPickingVertex) {
			UpdatePickVertex(event.GetPosition());
		}
		else if (isPickingEdge) {
			UpdatePickEdge(event.GetPosition());
		}
		else if (isMovingVertex) {
			UpdateMoveVertex(event.GetPosition() - mouseDownOffset);
		}
		else if (isSlidingEdge) {
			UpdateEdgeSlide(event.GetPosition());
		}
		else if (isPickingBone) {
			// Dragging off a clicked bone does nothing
		}
		else {
			if (Config.MatchValue("Input/LeftMousePan", "true")) {
				gls.PanCamera(x - lastX, y - lastY);
				UpdateTransformTool();
				UpdatePivot();
			}
		}

		gls.RenderOneFrame();
	}

	if (!rbuttonDown && !lbuttonDown && !isMovingVertex && !isSlidingEdge) {
		GLSurface::CursorHitResult hitResult{};

		// Shown bones can be hovered and clicked with any tool. The handles of the
		// transform and pivot tools take precedence where they overlap a bone.
		std::string boneUnderCursor = PickBone(x, y);
		if (!boneUnderCursor.empty() && (transformMode || pivotMode)) {
			Vector3 handleOrigin, handleNormal;
			Mesh* handleMesh = nullptr;
			if (gls.CollideOverlay(x, y, handleOrigin, handleNormal, &handleMesh) && handleMesh && handleMesh != PivotCenterMesh)
				boneUnderCursor.clear();
		}
		SetHoverBone(boneUnderCursor);

		// The brush cursor is hidden during playback: the brushes are locked out
		// anyway, and it would hit-test against the mesh BVHs that playback
		// leaves stale, reporting vertices that are not where it draws them.
		// It is also hidden over a bone, as a click there selects the bone.
		if (editMode && !(os && os->IsAnimationPlaying()) && hoverBone.empty()) {
			cursorExists = gls.UpdateCursor(x, y, true, &hitResult);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if (activeTool == ToolID::MoveVertex && hoverBone.empty()) {
			mouseDownMirrorPoint = -1;
			if (GetToolOptionXMirror()) {
				Vector3 hitPt, hitNrm;
				Mesh* mmesh = nullptr;
				int triInd = 0;
				if (gls.CollideMeshes(x, y, hitPt, hitNrm, true, &mmesh, true, &triInd) && mmesh == hitResult.hitMesh) {
					int hitPti = mmesh->tris[triInd].ClosestVertex(mmesh->verts.get(), hitPt);
					if (hitPti != hitResult.hoverPoint) {
						// Should we also check if the mirror point's location
						// is close to the mirror of the hover point's location?
						mouseDownMirrorPoint = hitPti;
						gls.ShowMirrorPointCursor(mmesh->verts[mouseDownMirrorPoint], mmesh);
					}
				}
			}
		}

		lastHitResult = hitResult;

		if ((transformMode || pivotMode) && !isTransforming && !isMovingPivot) {
			if (XMoveMesh) {
				XMoveMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YMoveMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZMoveMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XRotateMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YRotateMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZRotateMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XScaleMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YScaleMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZScaleMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				ScaleUniformMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			if (XPivotMesh) {
				XPivotMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YPivotMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZPivotMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				PivotCenterMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			Vector3 outOrigin, outNormal;
			Mesh* hitMesh = nullptr;
			if (gls.CollideOverlay(x, y, outOrigin, outNormal, &hitMesh)) {
				if (hitMesh && hitMesh != PivotCenterMesh) {
					hitMesh->color = Vector3(1.0f, 1.0f, 0.0f);
					gls.ShowCursor(false);
				}
			}
		}

		gls.RenderOneFrame();

		if (os->statusBar) {
			if (!hoverBone.empty()) {
				os->statusBar->SetStatusText(wxString::Format("Bone: %s", wxString::FromUTF8(hoverBone)), 1);
			}
			else if (cursorExists) {
				if (activeTool == ToolID::MaskBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", hitResult.hoverPoint, hitResult.hoverMask), 1);
				else if (activeTool == ToolID::WeightBrush) {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hitResult.hoverWeight, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z), 1);
				}
				else if (activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Color: %g, %g, %g, Alpha: %g",
																  hitResult.hoverPoint,
																  hitResult.hoverColor.x,
																  hitResult.hoverColor.y,
																  hitResult.hoverColor.z,
																  hitResult.hoverAlpha),
												 1);
				else {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z),
												 1);
				}
			}
			else {
				os->statusBar->SetStatusText("", 1);
			}
		}
	}

	lastX = x;
	lastY = y;
}

void wxGLPanel::OnLeftDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	lbuttonDown = true;

	// No tool may start editing while an animation plays; the click still falls
	// through to camera navigation.
	if (os && os->IsAnimationPlaying())
		return;

	// A grab feeds the running simulation and never touches the mesh data, so
	// while it is armed it takes the click ahead of the editing tools. Missing
	// the simulated mesh leaves the click to them as if it were not.
	if (os->IsPhysicsGrabEnabled()) {
		bool meshHit = StartPhysicsGrab(event.GetPosition());
		if (meshHit) {
			isPhysicsGrabbing = true;
			return;
		}
	}

	if (transformMode) {
		bool meshHit = StartTransform(event.GetPosition());
		if (meshHit) {
			isTransforming = true;
			SetHoverBone(std::string());
			return;
		}
	}

	if (pivotMode) {
		bool meshHit = StartPivotPosition(event.GetPosition());
		if (meshHit) {
			isMovingPivot = true;
			SetHoverBone(std::string());
			return;
		}
	}

	// A click on a shown bone selects it instead of using the active tool.
	// Ctrl adds it to or removes it from the selection.
	std::string pickedBone = PickBone(event.GetX(), event.GetY());
	if (!pickedBone.empty()) {
		isPickingBone = true;
		os->SelectBoneFromViewport(pickedBone, event.ControlDown());
		gls.RenderOneFrame();
		return;
	}

	if (brushMode) {
		bool meshHit = StartBrushStroke(event.GetPosition());
		if (meshHit)
			isPainting = true;
	}
	else if (activeTool == ToolID::CollapseVertex) {
		bool meshHit = StartPickVertex();
		if (meshHit)
			isPickingVertex = true;
	}
	else if (activeTool == ToolID::FlipEdge || activeTool == ToolID::SplitEdge) {
		bool meshHit = StartPickEdge();
		if (meshHit)
			isPickingEdge = true;
	}
	else if (activeTool == ToolID::MoveVertex) {
		if (GetToolOptionEdgeSlide()) {
			if (isSlidingEdge) {
				EndEdgeSlide();
				isSlidingEdge = false;
			}
			else {
				bool meshHit = StartEdgeSlide(event.GetPosition());
				if (meshHit)
					isSlidingEdge = true;
			}
		}
		else if (isMovingVertex) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else {
			bool meshHit = StartMoveVertex(event.GetPosition());
			if (meshHit)
				isMovingVertex = true;
		}
	}
	else if (vertexEdit) {
		bool meshHit = SelectVertex(event.GetPosition());
		if (meshHit)
			isSelecting = true;
	}
}

void wxGLPanel::OnMiddleDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	mbuttonDown = true;
}

void wxGLPanel::OnMiddleUp(wxMouseEvent& WXUNUSED(event)) {
	if (GetCapture() == this)
		ReleaseMouse();

	isMDragging = false;
	mbuttonDown = false;
}

void wxGLPanel::OnLeftUp(wxMouseEvent& event) {
	if (GetCapture() == this)
		ReleaseMouse();

	// OnLeftDown bails out during playback, so none of the edit states below can
	// be set; only the click-to-select path would still run, and it would pick
	// against the mesh BVHs that playback leaves stale.
	const bool playing = os && os->IsAnimationPlaying();

	if (!playing && !isLDragging && !isPainting && !isPhysicsGrabbing && !isPickingBone && activeTool == ToolID::Select) {
		int x, y;
		event.GetPosition(&x, &y);

		Mesh* m = gls.PickMesh(x, y);
		if (m)
			os->SelectShape(m->shapeName);
	}

	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		if (mouseHasMovedSinceStart) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else
			mouseHasMovedSinceStart = true;
	}

	if (isSlidingEdge) {
		if (mouseHasMovedSinceStart) {
			EndEdgeSlide();
			isSlidingEdge = false;
		}
		else
			mouseHasMovedSinceStart = true;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isPickingBone = false;
	isLDragging = false;
	lbuttonDown = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		CancelMoveVertex();
		isMovingVertex = false;
	}

	if (isSlidingEdge) {
		CancelEdgeSlide();
		isSlidingEdge = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isPickingBone = false;
	isLDragging = false;
	lbuttonDown = false;

	isMDragging = false;
	mbuttonDown = false;

	rbuttonDown = false;
	isRDragging = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnRightDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	rbuttonDown = true;
	isRDragging = false;
	rDownPos = event.GetPosition();
}

void wxGLPanel::OnRightUp(wxMouseEvent& event) {
	if (HasCapture())
		ReleaseMouse();

	const bool wasDragging = isRDragging;
	rbuttonDown = false;
	isRDragging = false;

	// Right-clicking a shown bone without turning the camera opens the bone
	// context menu for it, to add bones as its children among other things.
	if (wasDragging || lbuttonDown || !os)
		return;

	// Connector bones only get the menu to add bones below them
	const std::string bone = PickBone(event.GetX(), event.GetY(), true);
	if (bone.empty())
		return;

	// The menu acts on the clicked bone: it becomes the active one, the parent
	// of added bones. A multi-selection it is part of stays, for deleting.
	if (bone != os->GetActiveBone())
		os->SelectBoneFromViewport(bone, false, true);

	SetHoverBone(std::string());
	gls.RenderOneFrame();

	os->ShowBoneContextMenu(bone, this, event.GetPosition());
}

void wxGLPanel::OnLeaveWindow(wxMouseEvent& event) {
	event.Skip();

	if (lbuttonDown || rbuttonDown || hoverBone.empty())
		return;

	SetHoverBone(std::string());
	gls.RenderOneFrame();
}
