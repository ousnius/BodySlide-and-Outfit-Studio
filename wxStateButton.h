#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include <wx/wx.h>
#include <wx/sizer.h>


class wxStateButton : public wxButton
{
 
    bool m_bChecked;
    wxString text;
        
  ///  static const int buttonWidth = 100;
  //  static const int buttonHeight = 20;
        
public:
    wxStateButton();
	wxStateButton(wxWindow* parent, wxWindowID,
const wxString& label = wxEmptyString,
const wxPoint& pos = wxDefaultPosition ,
const wxSize& size = wxDefaultSize,
long style = 0,
const wxValidator& validator = wxDefaultValidator,
const wxString& name = "button"); 
 //   wxStateButton(wxFrame* parent, wxString text);
        
    void paintEvent(wxPaintEvent & evt);
    void paintNow();
        
    void render(wxDC& dc);

	bool GetCheck() { return m_bChecked; }
	void SetCheck(bool newCheck = true) {m_bChecked  = newCheck; }
        
    // some useful events
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);
     
	DECLARE_DYNAMIC_CLASS(wxStateButton)
    DECLARE_EVENT_TABLE()
};
 