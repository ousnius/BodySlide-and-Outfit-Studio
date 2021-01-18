/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GroupManager.h"
#include "../utils/ConfigurationManager.h"

#include <wx/srchctrl.h>
#include <regex>

extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(GroupManager, wxDialog)
	EVT_FILEPICKER_CHANGED(XRCID("fpGroupXML"), GroupManager::OnLoadGroup)
	EVT_LISTBOX(XRCID("listGroups"), GroupManager::OnSelectGroup)
	EVT_LISTBOX_DCLICK(XRCID("listGroups"), GroupManager::OnDblClickGroup)
	EVT_LISTBOX_DCLICK(XRCID("listMembers"), GroupManager::OnDblClickMember)
	EVT_LISTBOX_DCLICK(XRCID("listOutfits"), GroupManager::OnDblClickOutfit)
	EVT_BUTTON(XRCID("btAddGroup"), GroupManager::OnAddGroup)
	EVT_BUTTON(XRCID("btRemoveGroup"), GroupManager::OnRemoveGroup)
	EVT_BUTTON(XRCID("btSave"), GroupManager::OnSaveGroup)
	EVT_BUTTON(XRCID("btSaveAs"), GroupManager::OnSaveGroupAs)
	EVT_BUTTON(XRCID("btRemoveMember"), GroupManager::OnRemoveMember)
	EVT_BUTTON(XRCID("btAddMember"), GroupManager::OnAddMember)
	EVT_TEXT_ENTER(XRCID("outfitFilter"), GroupManager::OnFilterChanged)
	EVT_TEXT(XRCID("outfitFilter"), GroupManager::OnFilterChanged)
	EVT_BUTTON(wxID_OK, GroupManager::OnCloseButton)
	EVT_CLOSE(GroupManager::OnClose)
wxEND_EVENT_TABLE()

GroupManager::GroupManager(wxWindow* parent, std::vector<std::string> outfits)
	: allOutfits(std::move(outfits)) {
	wxXmlResource *xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/GroupManager.xrc");
	xrc->LoadDialog(this, parent, "dlgGroupManager");

	SetSize(800, 500);
	SetDoubleBuffered(true);
	CenterOnParent();

	XRCCTRL(*this, "fpGroupXML", wxFilePickerCtrl)->SetInitialDirectory(wxString::FromUTF8(GetProjectPath()) + "/SliderGroups");
	listGroups = XRCCTRL(*this, "listGroups", wxListBox);
	groupName = XRCCTRL(*this, "groupName", wxTextCtrl);
	btAddGroup = XRCCTRL(*this, "btAddGroup", wxButton);
	btRemoveGroup = XRCCTRL(*this, "btRemoveGroup", wxButton);
	btSave = XRCCTRL(*this, "btSave", wxButton);
	btSaveAs = XRCCTRL(*this, "btSaveAs", wxButton);
	btRemoveMember = XRCCTRL(*this, "btRemoveMember", wxButton);
	btAddMember = XRCCTRL(*this, "btAddMember", wxButton);
	listMembers = XRCCTRL(*this, "listMembers", wxListBox);
	listOutfits = XRCCTRL(*this, "listOutfits", wxListBox);

	auto search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
	search->ShowSearchButton(true);
	search->SetDescriptiveText("Outfit Filter");
	search->SetToolTip("Filter outfit list by name");

	xrc->AttachUnknownControl("outfitFilter", search, this);

	RefreshUI();
}

GroupManager::~GroupManager() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/GroupManager.xrc");
}

std::string GroupManager::GetProjectPath() const {
	std::string res = Config["ProjectPath"];
	return res.empty() ? Config["AppDir"] : res;
}

void GroupManager::RefreshUI(const bool clearGroups) {
	listMembers->Clear();

	btSave->Enable(dirty & !fileName.empty());

	std::string selectedGroup;
	if (clearGroups) {
		// Add groups to list
		listGroups->Clear();
		for (auto &group : groupMembers)
			listGroups->Append(wxString::FromUTF8(group.first));
	}
	else {
		// Get group selection if existing
		selectedGroup = listGroups->GetStringSelection().ToUTF8();
	}

	// Add members of selected group to list
	bool groupSelected = !selectedGroup.empty();
	if (groupSelected)
		for (auto &member : groupMembers[selectedGroup])
			listMembers->Append(wxString::FromUTF8(member));

	auto outfitFilter = XRCCTRL(*this, "outfitFilter", wxSearchCtrl);
	std::string filter{ outfitFilter->GetValue().ToUTF8() };
	DoFilterOutfits(filter);

	listMembers->Enable(groupSelected);
	listOutfits->Enable(groupSelected);
	outfitFilter->Enable(groupSelected);

	btRemoveMember->Enable(groupSelected);
	btAddMember->Enable(groupSelected);
}

bool GroupManager::ChooseFile() {
	wxFileDialog file(this, "Saving group XML file...", wxString::FromUTF8(GetProjectPath()) + "/SliderGroups", fileName, "Group Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (file.ShowModal() != wxID_OK)
		return false;

	fileName = file.GetPath();
	return true;
}

void GroupManager::SaveGroup() {
	SliderSetGroupFile groupFile;
	groupFile.New(fileName.ToUTF8().data());
	for (auto &grp : groupMembers) {
		SliderSetGroup group;
		group.SetName(grp.first);
		group.AddMembers(grp.second);
		groupFile.UpdateGroup(group);
	}
	groupFile.Save();

	dirty = false;
	btSave->Enable(dirty & !fileName.empty());
}

void GroupManager::DoRenameGroup() {
	// Rename the selected group
	std::string selectedGroup{ listGroups->GetStringSelection().ToUTF8() };
	if (!selectedGroup.empty()) {
		std::string newGroupName;

		do {
			std::string result{ wxGetTextFromUser(_("Please enter a new unique name for the group."), _("Rename Group")).ToUTF8() };
			if (result.empty())
				return;

			newGroupName = std::move(result);
		} while (groupMembers.find(newGroupName) != groupMembers.end());

		auto it = groupMembers.find(selectedGroup);
		if (it != groupMembers.end()) {
			std::swap(groupMembers[newGroupName], it->second);
			groupMembers.erase(it);
		}

		listGroups->SetString(listGroups->GetSelection(), wxString::FromUTF8(newGroupName));

		dirty = true;
		RefreshUI();
	}
}

void GroupManager::DoRemoveMembers() {
	wxArrayInt selections;
	listMembers->GetSelections(selections);

	// Find and remove member from selected group
	std::string selectedGroup{listGroups->GetStringSelection().ToUTF8()};
	if (!selectedGroup.empty()) {
		for (auto &sel : selections) {
			std::string member{listMembers->GetString(sel).ToUTF8()};
			auto it = find(groupMembers[selectedGroup].begin(), groupMembers[selectedGroup].end(), member);
			if (it != groupMembers[selectedGroup].end())
				groupMembers[selectedGroup].erase(it);
		}
	}

	dirty = true;
	selections.Clear();
	RefreshUI();
}

void GroupManager::DoAddMembers() {
	wxArrayInt selections;
	listOutfits->GetSelections(selections);

	// Add member to selected group
	std::string selectedGroup{listGroups->GetStringSelection().ToUTF8()};
	if (!selectedGroup.empty()) {
		for (auto &sel : selections) {
			std::string member{listOutfits->GetString(sel).ToUTF8()};
			groupMembers[selectedGroup].push_back(member);
		}
	}

	dirty = true;
	selections.Clear();
	RefreshUI();
}

void GroupManager::DoFilterOutfits(const std::string& filter) {
	std::regex re(filter, std::regex_constants::icase);

	listOutfits->Clear();

	// Add outfits that are no members to list
	for (auto &outfit : allOutfits) {
		if (listMembers->FindString(outfit) == wxNOT_FOUND) {
			try {
				// Filter outfit
				if (std::regex_search(outfit, re))
					listOutfits->Append(wxString::FromUTF8(outfit));
			}
			catch (std::regex_error&) {
				listOutfits->Append(wxString::FromUTF8(outfit));
			}
		}
	}
}

void GroupManager::OnLoadGroup(wxFileDirPickerEvent& event) {
	// Load group file
	SliderSetGroupFile groupFile(event.GetPath().ToUTF8().data());
	if (groupFile.fail())
		return;

	groupMembers.clear();
	btRemoveGroup->Enable();

	// Fill group member map
	std::vector<std::string> groupNames;
	groupFile.GetGroupNames(groupNames, true, true);
	for (auto &grp : groupNames) {
		SliderSetGroup group;
		if (groupFile.GetGroup(grp, group))
			return;

		std::vector<std::string> members;
		group.GetMembers(members);
		groupMembers[grp] = members;
	}

	wxFileName fileInfo(event.GetPath());
	fileName = fileInfo.GetFullPath();
	RefreshUI(true);
}

void GroupManager::OnSaveGroup(wxCommandEvent& WXUNUSED(event)) {
	SaveGroup();
}

void GroupManager::OnSaveGroupAs(wxCommandEvent& WXUNUSED(event)) {
	if (ChooseFile())
		SaveGroup();
}

void GroupManager::OnSelectGroup(wxCommandEvent& WXUNUSED(event)) {
	RefreshUI();
}

void GroupManager::OnDblClickGroup(wxCommandEvent& WXUNUSED(event)) {
	DoRenameGroup();
}

void GroupManager::OnDblClickMember(wxCommandEvent& WXUNUSED(event)) {
	DoRemoveMembers();
}

void GroupManager::OnDblClickOutfit(wxCommandEvent& WXUNUSED(event)) {
	DoAddMembers();
}

void GroupManager::OnAddGroup(wxCommandEvent& WXUNUSED(event)) {
	wxString name = groupName->GetValue().Trim();
	if (name.empty())
		return;

	groupName->Clear();

	// Already exists
	auto it = groupMembers.find(name.ToUTF8().data());
	if (it != groupMembers.end())
		return;

	// Create empty group entry
	groupMembers[name.ToUTF8().data()] = std::vector<std::string>();
	listGroups->Append(name);

	dirty = true;
	btSave->Enable(dirty & !fileName.empty());
}

void GroupManager::OnRemoveGroup(wxCommandEvent& WXUNUSED(event)) {
	int id = listGroups->GetSelection();
	if (id == wxNOT_FOUND)
		return;

	// Remove empty group entry
	wxString group = listGroups->GetString(id);
	groupMembers.erase(group.ToUTF8().data());

	dirty = true;
	RefreshUI(true);
}

void GroupManager::OnRemoveMember(wxCommandEvent& WXUNUSED(event)) {
	DoRemoveMembers();
}

void GroupManager::OnAddMember(wxCommandEvent& WXUNUSED(event)) {
	DoAddMembers();
}

void GroupManager::OnFilterChanged(wxCommandEvent& event) {
	std::string filter{ event.GetString().ToUTF8() };
	DoFilterOutfits(filter);
}

void GroupManager::OnCloseButton(wxCommandEvent& WXUNUSED(event)) {
	Close();
}

void GroupManager::OnClose(wxCloseEvent& event) {
	if (dirty) {
		int ret = wxMessageBox(_("Save changes to group file?"), _("Save Changes"), wxYES_NO | wxCANCEL | wxICON_WARNING, this);
		if (ret == wxCANCEL) {
			event.Veto();
			return;
		}
		else if (ret == wxYES) {
			if (fileName.empty()) {
				if (!ChooseFile()) {
					event.Veto();
					return;
				}
			}

			SaveGroup();
		}
	}

	EndModal(wxID_OK);
}
