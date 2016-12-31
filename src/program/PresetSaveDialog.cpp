/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "PresetSaveDialog.h"

wxBEGIN_EVENT_TABLE(PresetSaveDialog, wxDialog) 
	EVT_TEXT_ENTER(XRCID("spFilter"), PresetSaveDialog::FilterChanged)
	EVT_TEXT(XRCID("spFilter"), PresetSaveDialog::FilterChanged)
	EVT_CHECKLISTBOX(XRCID("spGroupDisplay"), PresetSaveDialog::CheckGroup)
	EVT_BUTTON(wxID_SAVE, PresetSaveDialog::OnSave)
wxEND_EVENT_TABLE()

PresetSaveDialog::PresetSaveDialog(wxWindow* parent) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load("res\\xrc\\SavePreset.xrc");
	xrc->LoadDialog(this, parent, "dlgSavePreset");

	SetDoubleBuffered(true);
	SetSize(460, 300);
	SetSizeHints(wxSize(460, 300), wxSize(460, -1));
	CenterOnParent();

	wxSearchCtrl* search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->SetDescriptiveText("Group Filter");
	search->SetToolTip("Filter list by group name");

	xrc->AttachUnknownControl("spFilter", search, this);
	wxCheckListBox* chkbox = XRCCTRL((*this), "spGroupDisplay", wxCheckListBox);
	chkbox->SetDoubleBuffered(true);
}
	
PresetSaveDialog::~PresetSaveDialog() {
	wxXmlResource::Get()->Unload("res\\xrc\\SavePreset.xrc");
}

void PresetSaveDialog::FilterGroups(const string& filter) {
	wxCheckListBox* chkbox = XRCCTRL((*this), "spGroupDisplay", wxCheckListBox);
	wxArrayString strings;
	filteredGroups.clear();
	chkbox->Clear();
	if (filter.empty()) {
		filteredGroups.assign(allGroupNames.begin(), allGroupNames.end());
	}
	else {
		try {
			regex re(filter, regex_constants::icase);
			for (auto &s : allGroupNames) {
				if (regex_search(s, re))
					filteredGroups.push_back(s);
			}
		}
		catch (...) {}

	}
	for (auto &g : filteredGroups) {
		int i = chkbox->Append(g);
		if (selectedGroups.find(g) != selectedGroups.end()) {
			chkbox->Check(i);
		}
	}
}

void PresetSaveDialog::FilterChanged(wxCommandEvent& event) {
	string filter = event.GetString();
	FilterGroups(filter);
}

void PresetSaveDialog::CheckGroup(wxCommandEvent& event) {
	string name;
	wxCheckListBox* chk = (wxCheckListBox*)event.GetEventObject();
	int item = event.GetInt();
	if (chk->IsChecked(item)) {
		name = event.GetString();
		selectedGroups.insert(name);
	}
	else {
		name = event.GetString();
		selectedGroups.erase(name);
	}
}

void PresetSaveDialog::OnSave(wxCommandEvent& WXUNUSED(event)) {
	outPresetName = XRCCTRL((*this), "spPresetName", wxTextCtrl)->GetValue();
	string presetFile = outPresetName + ".xml";

	wxFileDialog savePresetDialog(this, "Choose a preset file", "SliderPresets", presetFile, "Preset Files (*.xml)|*.xml", wxFD_SAVE);
	if (savePresetDialog.ShowModal() == wxID_OK) {
		outFileName = savePresetDialog.GetPath();
		outGroups.assign(selectedGroups.begin(), selectedGroups.end());
		wxDialog::Close();
	}
}

void PresetSaveDialog::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	return;
}
