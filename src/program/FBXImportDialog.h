/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "FBXImportOptions.h"

using namespace std;

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
