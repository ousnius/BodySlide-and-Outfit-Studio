#include "wxStateButton.h"

IMPLEMENT_DYNAMIC_CLASS(wxStateButton, wxButton)
 
BEGIN_EVENT_TABLE(wxStateButton, wxButton)
	EVT_LEFT_DOWN(wxStateButton::mouseDown)
	EVT_LEFT_UP(wxStateButton::mouseReleased)
    EVT_PAINT(wxStateButton::paintEvent)
END_EVENT_TABLE()

wxStateButton::wxStateButton() : wxButton() {
	m_bChecked = false;
}

wxStateButton::wxStateButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos,
	const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: wxButton(parent, id, label, pos, size, style, validator, name) { }

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */
void wxStateButton::paintEvent(wxPaintEvent& WXUNUSED(evt)) {
	// Depending on your system you may need to look at double-buffered dcs
	wxPaintDC dc(this);
	render(dc);
}
 
/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxStateButton::paintNow() {
	// Depending on your system you may need to look at double-buffered dcs
	wxClientDC dc(this);
	render(dc);
}
 
/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxStateButton::render(wxDC& dc) {
	int w;
	int h;
	dc.GetSize(&w, &h);
	if (m_bChecked) {
		dc.SetBrush(*wxGREY_BRUSH);
		dc.SetTextForeground(wxColor(255, 255, 255));
	}
	else {
		dc.SetBrush(wxBrush(wxColor(64, 64, 64)));
		dc.SetTextForeground(*wxLIGHT_GREY);
	}
	dc.SetPen(*wxGREY_PEN);

	wxRect r = GetClientRect();
	dc.DrawRectangle(0, 0, w, h);
	dc.SetFont(wxSystemSettings::GetFont(wxSystemFont::wxSYS_DEFAULT_GUI_FONT));
	dc.DrawLabel(GetLabel(), r, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
}
 
void wxStateButton::mouseDown(wxMouseEvent& WXUNUSED(event)) {
	m_bChecked = true;
	wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	evt.SetEventObject(this);
	ProcessEvent(evt);
}

void wxStateButton::mouseReleased(wxMouseEvent& event) {
	event.Skip();
}
