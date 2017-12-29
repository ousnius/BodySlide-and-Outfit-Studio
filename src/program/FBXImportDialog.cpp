/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "FBXImportDialog.h"

wxBEGIN_EVENT_TABLE(FBXImportDialog, wxDialog)
	EVT_BUTTON(wxID_OK, FBXImportDialog::OnImport)
wxEND_EVENT_TABLE()

FBXImportDialog::FBXImportDialog(wxWindow* parent) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load("res\\xrc\\FBXImport.xrc");
	xrc->LoadDialog(this, parent, "dlgFBXImport");

	SetDoubleBuffered(true);
	SetSize(300, 125);
	SetSizeHints(wxSize(300, 125), wxSize(300, 125));
	CenterOnParent();

	cbInvertU = XRCCTRL((*this), "cbInvertU", wxCheckBox);
	cbInvertV = XRCCTRL((*this), "cbInvertV", wxCheckBox);
}

FBXImportDialog::~FBXImportDialog() {
	wxXmlResource::Get()->Unload("res\\xrc\\FBXImport.xrc");
}

void FBXImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {
	options.InvertU = cbInvertU->IsChecked();
	options.InvertV = cbInvertV->IsChecked();

	EndModal(wxID_OK);
}