#include "wxStateButton.h"

IMPLEMENT_DYNAMIC_CLASS(wxStateButton, wxButton)
 
BEGIN_EVENT_TABLE(wxStateButton, wxButton)
    //EVT_MOTION(wxStateButton::mouseMoved)
	EVT_LEFT_DOWN(wxStateButton::mouseDown)
	EVT_LEFT_UP(wxStateButton::mouseReleased)
	//EVT_RIGHT_DOWN(wxStateButton::rightClick)
	//EVT_LEAVE_WINDOW(wxStateButton::mouseLeftWindow)
	//EVT_KEY_DOWN(wxStateButton::keyPressed)
	//EVT_KEY_UP(wxStateButton::keyReleased)
	//EVT_MOUSEWHEEL(wxStateButton::mouseWheelMoved)

    // catch paint events
    EVT_PAINT(wxStateButton::paintEvent)
END_EVENT_TABLE()

wxStateButton::wxStateButton() : wxButton() {
	//SetMinSize( wxSize(buttonWidth, buttonHeight) );
	//this->text = "My State";
	m_bChecked = false;
}

wxStateButton::wxStateButton(wxWindow* parent, wxWindowID id, const wxString& label,
							 const wxPoint& pos, const wxSize& size, long style,
							 const wxValidator& validator, const wxString& name)
							 :wxButton(parent, id, label, pos, size, style, validator, name) { } 

/* wxStateButton::wxStateButton(wxFrame* parent, wxString text) :
wxButton(parent) {
    SetMinSize( wxSize(buttonWidth, buttonHeight) );
    this->text = text;
    m_bChecked = false;
}
 */
/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */
 
void wxStateButton::paintEvent(wxPaintEvent& WXUNUSED(evt)) {
    // depending on your system you may need to look at double-buffered dcs
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
    // depending on your system you may need to look at double-buffered dcs
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
	dc.GetSize(&w,&h);
    if (m_bChecked) {
        dc.SetBrush(*wxGREY_BRUSH);
		dc.SetTextForeground(wxColor(255, 255, 255));
	} else {
        dc.SetBrush(wxBrush(wxColor(64, 64, 64))); 
		dc.SetTextForeground(*wxLIGHT_GREY);
	}
	dc.SetPen(*wxGREY_PEN);

	wxRect r = GetClientRect();
    dc.DrawRectangle(0, 0, w, h);
	dc.SetFont(wxSystemSettings::GetFont(wxSystemFont::wxSYS_DEFAULT_GUI_FONT));
	dc.DrawLabel(GetLabel(), r, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
}
 
void wxStateButton::mouseDown(wxMouseEvent& event) {
	//if (m_bChecked) {
		//return;
	//}
    m_bChecked = true;
	wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	evt.SetEventObject(this);
	ProcessCommand(evt);
	//paintNow();
	//event.Skip();
}

void wxStateButton::mouseReleased(wxMouseEvent& event) {
	//m_bChecked = false;
	//paintNow();
	//wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
	//GetEventHandler()->AddPendingEvent(evt);
	event.Skip();
	//wxMessageBox(wxT("You pressed a custom button"));
}
void wxStateButton::mouseLeftWindow(wxMouseEvent& event) {}
void wxStateButton::mouseMoved(wxMouseEvent& event) {}
void wxStateButton::mouseWheelMoved(wxMouseEvent& event) {}
void wxStateButton::rightClick(wxMouseEvent& event) {}
void wxStateButton::keyPressed(wxKeyEvent& event) {}
void wxStateButton::keyReleased(wxKeyEvent& event) {}