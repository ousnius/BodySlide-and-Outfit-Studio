///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 14 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "baseNormalsOptDlg.h"

///////////////////////////////////////////////////////////////////////////

baseNormGenDlg::baseNormGenDlg( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer4->Add( m_staticText1, 0, wxLEFT|wxTOP, 5 );
	
	
	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButton1 = new wxBitmapButton( this, wxID_ANY, wxBitmap( wxT("res/images/save.png"), wxBITMAP_TYPE_ANY ), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxNO_BORDER );
	m_bpButton1->SetToolTip( _("Load or save preset layer settings") );
	
	bSizer4->Add( m_bpButton1, 0, wxALIGN_BOTTOM|wxRIGHT, 5 );
	
	
	topSizer->Add( bSizer4, 0, wxEXPAND, 5 );
	
	layersProperties = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER);
	layersProperties->SetExtraStyle( wxPG_EX_ENABLE_TLP_TRACKING|wxPG_EX_HELP_AS_TOOLTIPS ); 
	topSizer->Add( layersProperties, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	buttonAddLayer = new wxButton( this, wxID_ANY, _("Add Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonAddLayer->SetToolTip( _("Add a new layer after the current one in the layer list") );
	
	bSizer3->Add( buttonAddLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	buttonMoveUp = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonMoveUp->SetToolTip( _("Move selected layer up one position") );
	
	bSizer3->Add( buttonMoveUp, 0, wxBOTTOM, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	buttonDeleteLayer = new wxButton( this, wxID_ANY, _("Delete Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonDeleteLayer->SetToolTip( _("Delete the selected layer") );
	
	bSizer3->Add( buttonDeleteLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	topSizer->Add( bSizer3, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	topSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	checkBackup = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Backup destination"), wxDefaultPosition, wxDefaultSize, 0 );
	checkBackup->SetValue(true); 
	checkBackup->SetToolTip( _("Save a copy of an existing normal map if one already exists.  File is saved in the original directory.") );
	
	sbSizer1->Add( checkBackup, 0, wxALL, 5 );
	
	checkCompress = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Compress output "), wxDefaultPosition, wxDefaultSize, 0 );
	checkCompress->SetToolTip( _("Compress output file using BC7 compression.  This can make saving the file take a VERY long time!  ") );
	
	sbSizer1->Add( checkCompress, 0, wxALL, 5 );
	
	checkUseBackgroundFilename = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Use Background Layer file as output"), wxDefaultPosition, wxDefaultSize, 0 );
	checkUseBackgroundFilename->SetValue(true); 
	checkUseBackgroundFilename->SetToolTip( _("use the file name specified in the background layer to save the normal map") );
	
	sbSizer1->Add( checkUseBackgroundFilename, 0, wxALL, 5 );
	
	outputFileName = new wxFilePickerCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxT("Output File"), _("Select a file"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	outputFileName->Enable( false );
	outputFileName->SetToolTip( _("Location to save normal map") );
	
	sbSizer1->Add( outputFileName, 0, wxEXPAND|wxLEFT, 20 );
	
	
	topSizer->Add( sbSizer1, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	buttonPreview = new wxButton( this, wxID_ANY, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonPreview->SetToolTip( _("Display current settings on mesh in preview window") );
	
	bSizer5->Add( buttonPreview, 0, wxALL, 5 );
	
	
	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );
	
	buttonGenerate = new wxButton( this, wxID_ANY, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonGenerate->SetToolTip( _("Generate and save the normal map") );
	
	bSizer5->Add( buttonGenerate, 0, wxALL, 5 );
	
	
	topSizer->Add( bSizer5, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( topSizer );
	this->Layout();
	presetContext = new wxMenu();
	wxMenuItem* ctxLoadPreset;
	ctxLoadPreset = new wxMenuItem( presetContext, wxID_ANY, wxString( _("Load Preset...") ) , wxEmptyString, wxITEM_NORMAL );
	presetContext->Append( ctxLoadPreset );
	
	wxMenuItem* ctxSavePreset;
	ctxSavePreset = new wxMenuItem( presetContext, wxID_ANY, wxString( _("Save Preset...") ) , wxEmptyString, wxITEM_NORMAL );
	presetContext->Append( ctxSavePreset );
	
	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( baseNormGenDlg::baseNormGenDlgOnContextMenu ), NULL, this ); 
	
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_bpButton1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doShowPresetContext ), NULL, this );
	layersProperties->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( baseNormGenDlg::doPropertyChanged ), NULL, this );
	buttonAddLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doAddLayer ), NULL, this );
	buttonMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doMoveUpLayer ), NULL, this );
	buttonDeleteLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doDeleteLayer ), NULL, this );
	checkUseBackgroundFilename->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( baseNormGenDlg::OnUseBackgroundLayerCheck ), NULL, this );
	outputFileName->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( baseNormGenDlg::doSetOutputFileName ), NULL, this );
	buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doPreviewNormalMap ), NULL, this );
	buttonGenerate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doGenerateNormalMap ), NULL, this );
	this->Connect( ctxLoadPreset->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( baseNormGenDlg::doLoadPreset ) );
	this->Connect( ctxSavePreset->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( baseNormGenDlg::doSavePreset ) );
}

baseNormGenDlg::~baseNormGenDlg()
{
	// Disconnect Events
	m_bpButton1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doShowPresetContext ), NULL, this );
	layersProperties->Disconnect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( baseNormGenDlg::doPropertyChanged ), NULL, this );
	buttonAddLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doAddLayer ), NULL, this );
	buttonMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doMoveUpLayer ), NULL, this );
	buttonDeleteLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doDeleteLayer ), NULL, this );
	checkUseBackgroundFilename->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( baseNormGenDlg::OnUseBackgroundLayerCheck ), NULL, this );
	outputFileName->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( baseNormGenDlg::doSetOutputFileName ), NULL, this );
	buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doPreviewNormalMap ), NULL, this );
	buttonGenerate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doGenerateNormalMap ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( baseNormGenDlg::doLoadPreset ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( baseNormGenDlg::doSavePreset ) );
	
	delete presetContext; 
}
