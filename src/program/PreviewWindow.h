/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../ui/PreviewPanel.h"
#include <wx/wx.h>

class BodySlideApp;

extern ConfigurationManager Config;

// Standalone preview window (pop-out mode).
// Hosts a PreviewPanel as its only child and delegates all preview operations to it.
class PreviewWindow : public wxFrame {
	BodySlideApp* app = nullptr;
	PreviewPanel* panel = nullptr;
	bool ownsPanel = false;

	wxDECLARE_EVENT_TABLE();

public:
	// Create a new standalone preview window with a fresh PreviewPanel
	PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app);

	// Create a standalone preview window hosting an existing PreviewPanel (reparenting)
	PreviewWindow(const wxPoint& pos, const wxSize& size, BodySlideApp* app, PreviewPanel* existingPanel);

	~PreviewWindow();

	void OnClose(wxCloseEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	PreviewPanel* GetPanel() { return panel; }

	// Release the panel without destroying it (for reparenting back)
	PreviewPanel* ReleasePanel();
};
