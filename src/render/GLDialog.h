/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

#include "GLCanvas.h"
#include "GLSurface.h"

class GLDialog : public wxDialog {
public:
	GLDialog();

	GLCanvas* CreateCanvas();
	virtual void OnShown();
	void OnClose(wxCloseEvent& event);

	GLCanvas* GetCanvas() { return canvas; }
	wxGLContext* GetContext() { return context.get(); }
	GLSurface& GetSurface() { return gls; }

	void LeftDrag(int dX, int dY);
	void RightDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);

	void ToggleTextures();
	void ToggleWireframe();
	void ToggleLighting();
	void ToggleFloor();

	void Resized(uint32_t w, uint32_t h);

	void Render();
	void Cleanup();

	wxDECLARE_EVENT_TABLE();

private:
	GLCanvas* canvas = nullptr;
	std::unique_ptr<wxGLContext> context;
	GLSurface gls;

	std::unordered_map<std::string, GLMaterial*> shapeMaterials;
	std::vector<Mesh*> floorMeshes;
};
