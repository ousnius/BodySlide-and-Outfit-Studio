/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PreviewWindow.h"
#include "BodySlideApp.h"

extern ConfigurationManager Config;
extern ConfigurationManager BodySlideConfig;

wxBEGIN_EVENT_TABLE(PreviewWindow, wxFrame)
	EVT_CLOSE(PreviewWindow::OnClose)
	EVT_MOVE_END(PreviewWindow::OnMoveWindow)
	EVT_SIZE(PreviewWindow::OnSetSize)
wxEND_EVENT_TABLE()

PreviewWindow::PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app)
	: wxFrame(nullptr, wxID_ANY, _("Preview"), pos, size)
	, app(app)
	, ownsPanel(true) {
	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/BodySlide.png", wxBITMAP_TYPE_PNG));

	panel = new PreviewPanel(this, app);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(panel, 1, wxEXPAND);
	SetSizer(sizer);
	Show();
}

PreviewWindow::PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app, PreviewPanel* existingPanel)
	: wxFrame(nullptr, wxID_ANY, _("Preview"), pos, size)
	, app(app)
	, panel(existingPanel)
	, ownsPanel(false) {
	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/BodySlide.png", wxBITMAP_TYPE_PNG));

	panel->Reparent(this);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(panel, 1, wxEXPAND);
	SetSizer(sizer);
	panel->Show();
	Show();
}

PreviewWindow::~PreviewWindow() {}

PreviewPanel* PreviewWindow::ReleasePanel() {
	PreviewPanel* p = panel;
	panel = nullptr;
	ownsPanel = false;
	return p;
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

void PreviewWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
	if (ownsPanel) {
		// Standalone mode: clean up and destroy
		if (panel)
			panel->Cleanup();
		Destroy();
		app->PreviewClosed();
	}
	else {
		// Pop-out mode: dock the panel back into the main frame
		app->DockPreview();
	}
	wxLogMessage("Preview window closed.");
}
