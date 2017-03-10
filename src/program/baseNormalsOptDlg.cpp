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
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	topSizer->Add( m_staticText1, 0, wxLEFT|wxTOP, 5 );
	
	layersProperties = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER);
	layersProperties->SetExtraStyle( wxPG_EX_ENABLE_TLP_TRACKING|wxPG_EX_HELP_AS_TOOLTIPS ); 
	categoryBackground = layersProperties->Append( new wxPropertyCategory( _("Background"), _("Background") ) ); 
	m_propertyGridItem2 = layersProperties->Append( new wxImageFileProperty( _("Background File"), _("Background File") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem2, _("Filename source for this layer.  ") );
	m_propertyGridItem7 = layersProperties->Append( new wxColourProperty( _("Color"), _("Color") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem7, _("Solid background color (If File is not set)") );
	m_propertyGridItem6 = layersProperties->Append( new wxEnumProperty( _("Resolution"), _("Resolution") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem6, _("Output texture dimesions.  By default all images will be caled to fit this size.") );
	categoryLayer1 = layersProperties->Append( new wxPropertyCategory( _("Layer 1"), _("Layer 1") ) ); 
	m_propertyGridItem4 = layersProperties->Append( new wxFileProperty( _("File"), _("File") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem4, _("A file containing normals data to combine.  Note this file should fit the mesh uvs") );
	m_propertyGridItem5 = layersProperties->Append( new wxBoolProperty( _("Is Tangent Space?"), _("Is Tangent Space?") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem5, _("True if the normals data in the layer file is in tangent space, false if they are in model space (msn). ") );
	m_propertyGridItem8 = layersProperties->Append( new wxImageFileProperty( _("Mask"), _("Mask") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem8, _("A greyscale image used to mask updates to destination image") );
	m_propertyGridItem9 = layersProperties->Append( new wxUIntProperty( _("X Offset"), _("X Offset") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem9, _("Offset to apply to image position ") );
	m_propertyGridItem10 = layersProperties->Append( new wxUIntProperty( _("Y Offset"), _("Y Offset") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem10, _("Y Offset to apply to image position ") );
	m_propertyGridItem11 = layersProperties->Append( new wxBoolProperty( _("Scale"), _("Scale") ) );
	layersProperties->SetPropertyHelpString( m_propertyGridItem11, _("if true, scale image to match background resolution.") );
	topSizer->Add( layersProperties, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	buttonAddLayer = new wxButton( this, wxID_ANY, _("Add Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( buttonAddLayer, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	buttonMoveUp = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( buttonMoveUp, 0, wxBOTTOM, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	buttonDeleteLayer = new wxButton( this, wxID_ANY, _("Delete Layer"), wxDefaultPosition, wxDefaultSize, 0 );
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
	sbSizer1->Add( checkUseBackgroundFilename, 0, wxALL, 5 );
	
	outputFileName = new wxFilePickerCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxT("Output File"), _("Select a file"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE );
	outputFileName->Enable( false );
	
	sbSizer1->Add( outputFileName, 0, wxEXPAND|wxLEFT, 20 );
	
	
	topSizer->Add( sbSizer1, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	buttonPreview = new wxButton( this, wxID_ANY, _("Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( buttonPreview, 0, wxALL, 5 );
	
	
	bSizer5->Add( 0, 0, 1, wxEXPAND, 5 );
	
	buttonGenerate = new wxButton( this, wxID_ANY, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonGenerate->SetToolTip( _("Generate and save the normal map") );
	
	bSizer5->Add( buttonGenerate, 0, wxALL, 5 );
	
	
	topSizer->Add( bSizer5, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( topSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	layersProperties->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( baseNormGenDlg::doPropertyChanged ), NULL, this );
	buttonAddLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doAddLayer ), NULL, this );
	buttonMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doMoveUpLayer ), NULL, this );
	buttonDeleteLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doDeleteLayer ), NULL, this );
	outputFileName->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( baseNormGenDlg::doSetOutputFileName ), NULL, this );
	buttonPreview->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doPreviewNormalMap ), NULL, this );
	buttonGenerate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doGenerateNormalMap ), NULL, this );
}

baseNormGenDlg::~baseNormGenDlg()
{
	// Disconnect Events
	layersProperties->Disconnect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( baseNormGenDlg::doPropertyChanged ), NULL, this );
	buttonAddLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doAddLayer ), NULL, this );
	buttonMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doMoveUpLayer ), NULL, this );
	buttonDeleteLayer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doDeleteLayer ), NULL, this );
	outputFileName->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( baseNormGenDlg::doSetOutputFileName ), NULL, this );
	buttonPreview->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doPreviewNormalMap ), NULL, this );
	buttonGenerate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( baseNormGenDlg::doGenerateNormalMap ), NULL, this );
	
}
