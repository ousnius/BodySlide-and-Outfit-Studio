/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLDialog.h"
#include "../utils/ConfigurationManager.h"

using namespace nifly;

extern ConfigurationManager Config;


wxBEGIN_EVENT_TABLE(GLDialog, wxDialog)
	EVT_CLOSE(GLDialog::OnClose)
wxEND_EVENT_TABLE()

GLDialog::GLDialog() {
	CenterOnParent();
}

GLCanvas* GLDialog::CreateCanvas() {
	canvas = new GLCanvas(this, GLSurface::GetGLAttribs());
	context = std::make_unique<wxGLContext>(canvas, nullptr, &GLSurface::GetGLContextAttribs());
	return canvas;
}

void GLDialog::OnShown() {
	if (!context->IsOK()) {
		Destroy();
		canvas = nullptr;
		wxLogError("Preview failed: OpenGL context is not OK.");
		wxMessageBox(_("Preview failed: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR);
		return;
	}

	gls.Initialize(canvas, context.get());

	auto size = canvas->GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight(), 65.0);

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

	// Turn texture rendering off initially
	gls.ToggleTextures();

	floorMeshes = gls.AddFloor();
}

void GLDialog::OnClose(wxCloseEvent& WXUNUSED(event)) {
	Cleanup();
	Destroy();
}

void GLDialog::LeftDrag(int dX, int dY) {
	gls.PanCamera(dX, dY);
	gls.RenderOneFrame();
}

void GLDialog::RightDrag(int dX, int dY) {
	gls.TurnTableCamera(dX);
	gls.PitchCamera(dY);
	gls.RenderOneFrame();
}

void GLDialog::MouseWheel(int dW) {
	gls.DollyCamera(dW);
	gls.RenderOneFrame();
}

void GLDialog::TrackMouse(int X, int Y) {
	gls.UpdateCursor(X, Y);
	gls.RenderOneFrame();
}

void GLDialog::ToggleTextures() {
	gls.ToggleTextures();
	gls.RenderOneFrame();
}

void GLDialog::ToggleWireframe() {
	gls.ToggleWireframe();
	gls.RenderOneFrame();
}

void GLDialog::ToggleLighting() {
	gls.ToggleLighting();
	gls.RenderOneFrame();
}

void GLDialog::ToggleFloor() {
	for (auto& m : floorMeshes)
		m->bVisible = !m->bVisible;

	gls.RenderOneFrame();
}

void GLDialog::Render() {
	gls.RenderOneFrame();
}

void GLDialog::Resized(uint32_t w, uint32_t h) {
	gls.SetSize(w, h);
}

void GLDialog::Cleanup() {
	// Set current context for resource deletion
	if (canvas && context)
		canvas->SetCurrent(*context);

	gls.Cleanup();
	shapeMaterials.clear();
	gls.RenderOneFrame();
}
