/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PresetSaveDialog.h"
#include "../utils/ConfigurationManager.h"

#include <regex>

extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(PresetSaveDialog, wxDialog) 
	EVT_TEXT_ENTER(XRCID("spFilter"), PresetSaveDialog::FilterChanged)
	EVT_TEXT(XRCID("spFilter"), PresetSaveDialog::FilterChanged)
	EVT_CHECKLISTBOX(XRCID("spGroupDisplay"), PresetSaveDialog::CheckGroup)
	EVT_BUTTON(wxID_SAVE, PresetSaveDialog::OnSave)
wxEND_EVENT_TABLE()

PresetSaveDialog::PresetSaveDialog(wxWindow* parent) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SavePreset.xrc");
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
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SavePreset.xrc");
}

std::string PresetSaveDialog::GetProjectPath() const {
	std::string res = Config["ProjectPath"];
	return res.empty() ? Config["AppDir"] : res;
}

void PresetSaveDialog::FilterGroups(const std::string& filter) {
	wxCheckListBox* chkbox = XRCCTRL((*this), "spGroupDisplay", wxCheckListBox);
	chkbox->Clear();
	filteredGroups.clear();

	if (filter.empty()) {
		filteredGroups.assign(allGroupNames.begin(), allGroupNames.end());
	}
	else {
		wxString filterStr = wxString::FromUTF8(filter);
		filterStr.MakeLower();

		for (auto &group : allGroupNames) {
			wxString groupStr = wxString::FromUTF8(group);
			if (groupStr.Lower().Contains(filterStr))
				filteredGroups.push_back(groupStr.ToUTF8().data());
		}
	}

	for (auto &g : filteredGroups) {
		int i = chkbox->Append(g);
		if (selectedGroups.find(g) != selectedGroups.end())
			chkbox->Check(i);
	}
}

void PresetSaveDialog::FilterChanged(wxCommandEvent& event) {
	std::string filter{event.GetString().ToUTF8()};
	FilterGroups(filter);
}

void PresetSaveDialog::CheckGroup(wxCommandEvent& event) {
	std::string name;
	wxCheckListBox* chk = (wxCheckListBox*)event.GetEventObject();
	int item = event.GetInt();
	if (chk->IsChecked(item)) {
		name = event.GetString().ToUTF8();
		selectedGroups.insert(name);
	}
	else {
		name = event.GetString().ToUTF8();
		selectedGroups.erase(name);
	}
}

void PresetSaveDialog::OnSave(wxCommandEvent& WXUNUSED(event)) {
	outPresetName = XRCCTRL((*this), "spPresetName", wxTextCtrl)->GetValue().ToUTF8();
	std::string presetFile = outPresetName + ".xml";

	wxFileDialog savePresetDialog(this, "Choose a preset file", wxString::FromUTF8(GetProjectPath()) + "/SliderPresets", wxString::FromUTF8(presetFile), "Preset Files (*.xml)|*.xml", wxFD_SAVE);
	if (savePresetDialog.ShowModal() == wxID_OK) {
		outFileName = savePresetDialog.GetPath().ToUTF8();
		outGroups.assign(selectedGroups.begin(), selectedGroups.end());
		wxDialog::Close();
	}
}

void PresetSaveDialog::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	return;
}
