/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PresetSaveDialog.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/ProjectUtil.h"

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
	SetSize(FromDIP(wxSize(460, 340)));
	SetSizeHints(FromDIP(wxSize(460, 340)), FromDIP(wxSize(460, -1)));
	CenterOnParent();

	wxSearchCtrl* search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(200, -1)), wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->SetDescriptiveText(_("Group Filter"));
	search->SetToolTip(_("Filter list by group name"));

	xrc->AttachUnknownControl("spFilter", search, this);
	auto groupHelp = (wxStaticText*)FindWindowByName("spGroupHelp", this);
	if (groupHelp)
		groupHelp->Wrap(FromDIP(430));

	wxCheckListBox* chkbox = XRCCTRL((*this), "spGroupDisplay", wxCheckListBox);
	chkbox->SetDoubleBuffered(true);
}

PresetSaveDialog::~PresetSaveDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SavePreset.xrc");
}

void PresetSaveDialog::SetExistingPreset(const std::string& presetName, const std::string& presetFileName, const std::vector<std::string>& groups) {
	editExistingPreset = true;
	existingPresetFileName = presetFileName;
	selectedGroups.clear();
	selectedGroups.insert(groups.begin(), groups.end());

	SetTitle(_("Edit preset groups..."));

	if (auto presetNameLabel = XRCCTRL((*this), "spPresetNameLabel", wxStaticText))
		presetNameLabel->SetLabel(_("Preset name:"));

	if (auto groupLabel = XRCCTRL((*this), "spGroupSelectLabel", wxStaticText))
		groupLabel->SetLabel(_("Select groups to assign to this preset:"));

	if (auto presetNameCtrl = XRCCTRL((*this), "spPresetName", wxTextCtrl)) {
		presetNameCtrl->ChangeValue(wxString::FromUTF8(presetName));
		presetNameCtrl->SetEditable(false);
	}

	Layout();
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

		for (auto& group : allGroupNames) {
			wxString groupStr = wxString::FromUTF8(group);
			if (groupStr.Lower().Contains(filterStr))
				filteredGroups.push_back(groupStr.ToUTF8().data());
		}
	}

	for (auto& g : filteredGroups) {
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
	outGroups.assign(selectedGroups.begin(), selectedGroups.end());

	if (editExistingPreset) {
		outFileName = existingPresetFileName;
		EndModal(wxID_OK);
		return;
	}

	std::string presetFile = outPresetName + ".xml";

	wxFileDialog savePresetDialog(this,
								  "Choose a preset file",
								  wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderPresets",
								  wxString::FromUTF8(presetFile),
								  "Preset Files (*.xml)|*.xml",
								  wxFD_SAVE);
	if (savePresetDialog.ShowModal() == wxID_OK) {
		outFileName = savePresetDialog.GetPath().ToUTF8();
		EndModal(wxID_OK);
	}
}

void PresetSaveDialog::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	return;
}
