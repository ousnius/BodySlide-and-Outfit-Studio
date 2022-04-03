/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>

class GLDialog;

class GLCanvas : public wxGLCanvas {
	GLDialog* parent = nullptr;
	bool firstPaint = true;
	wxPoint lastMousePosition;

public:
	GLCanvas(GLDialog* parent, const wxGLAttributes& attribs);

	void OnPaint(wxPaintEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnResized(wxSizeEvent& event);

	wxDECLARE_EVENT_TABLE();
};
