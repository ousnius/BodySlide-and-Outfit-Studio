/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>

class wxStateButton : public wxButton {
	bool m_bChecked = false;
	bool m_bPendingChanges = false;
	wxString text;

public:
	wxStateButton();
	wxStateButton(wxWindow* parent,
				  wxWindowID,
				  const wxString& label = wxEmptyString,
				  const wxPoint& pos = wxDefaultPosition,
				  const wxSize& size = wxDefaultSize,
				  long style = 0,
				  const wxValidator& validator = wxDefaultValidator,
				  const wxString& name = "button");

	void paintEvent(wxPaintEvent& evt);
	void paintNow();

	void render(wxDC& dc);

	bool GetCheck() { return m_bChecked; }
	void SetCheck(bool newCheck = true) { m_bChecked = newCheck; }

	bool HasPendingChanges() { return m_bPendingChanges; }
	void SetPendingChanges(bool newPending = true);

	void mouseDown(wxMouseEvent& WXUNUSED(event));
	void mouseReleased(wxMouseEvent& event);

	DECLARE_DYNAMIC_CLASS(wxStateButton)
	DECLARE_EVENT_TABLE()
};
