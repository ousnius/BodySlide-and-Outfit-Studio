/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLDialog.h"

wxBEGIN_EVENT_TABLE(GLCanvas, wxGLCanvas)
	EVT_PAINT(GLCanvas::OnPaint)
	EVT_MOTION(GLCanvas::OnMotion)
	EVT_MOUSEWHEEL(GLCanvas::OnMouseWheel)
	EVT_KEY_UP(GLCanvas::OnKeyUp)
	EVT_SIZE(GLCanvas::OnResized)
wxEND_EVENT_TABLE()

GLCanvas::GLCanvas(GLDialog* parent, const wxGLAttributes& attribs)
	: wxGLCanvas(parent, attribs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
	, parent(parent) {}

void GLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		parent->OnShown();
	}
	parent->Render();
}

void GLCanvas::OnMotion(wxMouseEvent& event) {
	if (parent->IsActive())
		SetFocus();

	auto delta = event.GetPosition() - lastMousePosition;

	if (event.LeftIsDown()) {
		parent->LeftDrag(delta.x, delta.y);
	}
	else if (event.MiddleIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			parent->MouseWheel(delta.y);
		else
			parent->LeftDrag(delta.x, delta.y);
	}
	else if (event.RightIsDown()) {
		if (wxGetKeyState(WXK_SHIFT))
			parent->LeftDrag(delta.x, delta.y);
		else
			parent->RightDrag(delta.x, delta.y);
	}
	else
		parent->TrackMouse(event.GetX(), event.GetY());

	lastMousePosition = event.GetPosition();
}

void GLCanvas::OnMouseWheel(wxMouseEvent& event) {
	parent->MouseWheel(event.GetWheelRotation());
}

void GLCanvas::OnKeyUp(wxKeyEvent& event) {
	int key = event.GetKeyCode();
	switch (key) {
		case 'T': parent->ToggleTextures(); break;
		case 'W': parent->ToggleWireframe(); break;
		case 'L': parent->ToggleLighting(); break;
		case 'G': parent->ToggleFloor(); break;
	}
}

void GLCanvas::OnResized(wxSizeEvent& event) {
	parent->Resized(event.GetSize().GetWidth(), event.GetSize().GetHeight());
}
