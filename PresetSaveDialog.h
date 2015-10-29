/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "stdafx.h"

#include <wx/xrc/xmlres.h>
#include <wx/srchctrl.h>
#include <regex>
#include <set>
#include <vector>

using namespace std;

class PresetSaveDialog : public wxDialog {
public:
	vector<string> allGroupNames;
	vector<string> filteredGroups;
	set<string> selectedGroups;
	string outFileName;
	string outPresetName;
	vector<string> outGroups;
	PresetSaveDialog(wxWindow* parent);
	~PresetSaveDialog();

	void FilterGroups(const string& filter = "");

	void OnEraseBackground(wxEraseEvent &event);
	void FilterChanged(wxCommandEvent& event);
	void CheckGroup(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);

	DECLARE_EVENT_TABLE();
};
