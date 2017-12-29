/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "FBXImportOptions.h"

class FBXImportDialog : public wxDialog {
public:
	FBXImportDialog(wxWindow* parent);
	~FBXImportDialog();

	void OnImport(wxCommandEvent& event);
	FBXImportOptions GetOptions() { return options; }

	wxDECLARE_EVENT_TABLE();

private:
	FBXImportOptions options;
	wxCheckBox* cbInvertU;
	wxCheckBox* cbInvertV;
};
