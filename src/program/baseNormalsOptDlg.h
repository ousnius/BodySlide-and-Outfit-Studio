///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 14 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __BASENORMALSOPTDLG_H__
#define __BASENORMALSOPTDLG_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class baseNormGenDlg
///////////////////////////////////////////////////////////////////////////////
class baseNormGenDlg : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxPropertyGrid* layersProperties;
		wxPGProperty* categoryBackground;
		wxPGProperty* m_propertyGridItem2;
		wxPGProperty* m_propertyGridItem7;
		wxPGProperty* m_propertyGridItem6;
		wxPGProperty* categoryLayer1;
		wxPGProperty* m_propertyGridItem4;
		wxPGProperty* m_propertyGridItem5;
		wxPGProperty* m_propertyGridItem8;
		wxPGProperty* m_propertyGridItem9;
		wxPGProperty* m_propertyGridItem10;
		wxPGProperty* m_propertyGridItem11;
		wxButton* buttonAddLayer;
		wxButton* buttonMoveUp;
		wxButton* buttonDeleteLayer;
		wxStaticLine* m_staticline1;
		wxCheckBox* checkBackup;
		wxCheckBox* checkCompress;
		wxCheckBox* checkUseBackgroundFilename;
		wxFilePickerCtrl* outputFileName;
		wxButton* buttonPreview;
		wxButton* buttonGenerate;
		
		// Virtual event handlers, overide them in your derived class
		virtual void doPropertyChanged( wxPropertyGridEvent& event ) { event.Skip(); }
		virtual void doAddLayer( wxCommandEvent& event ) { event.Skip(); }
		virtual void doMoveUpLayer( wxCommandEvent& event ) { event.Skip(); }
		virtual void doDeleteLayer( wxCommandEvent& event ) { event.Skip(); }
		virtual void doSetOutputFileName( wxFileDirPickerEvent& event ) { event.Skip(); }
		virtual void doPreviewNormalMap( wxCommandEvent& event ) { event.Skip(); }
		virtual void doGenerateNormalMap( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		baseNormGenDlg( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Normal Map Generation Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 323,473 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSTAY_ON_TOP ); 
		~baseNormGenDlg();
	
};

#endif //__BASENORMALSOPTDLG_H__
