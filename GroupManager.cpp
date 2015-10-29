/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "GroupManager.h"

BEGIN_EVENT_TABLE (GroupManager, wxDialog)
	EVT_FILEPICKER_CHANGED(XRCID("fpGroupXML"), GroupManager::OnLoadGroup)
	EVT_LISTBOX(XRCID("listGroups"), GroupManager::OnSelectGroup)
	EVT_BUTTON(XRCID("btAddGroup"), GroupManager::OnAddGroup)
	EVT_BUTTON(XRCID("btRemoveGroup"), GroupManager::OnRemoveGroup)
	EVT_BUTTON(XRCID("btSave"), GroupManager::OnSaveGroup)
	EVT_BUTTON(XRCID("btSaveAs"), GroupManager::OnSaveGroupAs)
	EVT_BUTTON(XRCID("btRemoveMember"), GroupManager::OnRemoveMember)
	EVT_BUTTON(XRCID("btAddMember"), GroupManager::OnAddMember)
END_EVENT_TABLE();

GroupManager::GroupManager(wxWindow* parent, vector<string> outfits) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->LoadDialog(this, parent, "dlgGroupManager");

	SetSize(800, 500);
	SetDoubleBuffered(true);
	CenterOnParent();

	XRCCTRL(*this, "fpGroupXML", wxFilePickerCtrl)->SetInitialDirectory("SliderGroups");
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

	allOutfits = outfits;
	RefreshUI();
}

GroupManager::~GroupManager() {
}

void GroupManager::RefreshUI(const bool& clearGroups) {
	listMembers->Clear();
	listOutfits->Clear();

	btSave->Enable(dirty & !fileName.empty());

	string selectedGroup;
	if (clearGroups) {
		// Add groups to list
		listGroups->Clear();
		for (auto &group : groupMembers)
			listGroups->Append(group.first);
	}
	else {
		// Get group selection if existing
		selectedGroup = listGroups->GetStringSelection().ToStdString();
	}

	// Add members of selected group to list
	if (!selectedGroup.empty())
		for (auto &member : groupMembers[selectedGroup])
			listMembers->Append(member);

	// Add outfits that are no members to list
	for (auto &outfit : allOutfits)
		if (listMembers->FindString(outfit) == wxNOT_FOUND)
			listOutfits->Append(outfit);
}

void GroupManager::OnLoadGroup(wxFileDirPickerEvent& event) {
	// Load group file
	SliderSetGroupFile groupFile(event.GetPath().ToStdString());
	if (groupFile.fail())
		return;

	groupMembers.clear();
	btRemoveGroup->Enable();

	// Fill group member map
	vector<string> groupNames;
	groupFile.GetGroupNames(groupNames, true, true);
	for (auto &grp : groupNames) {
		SliderSetGroup group;
		if (groupFile.GetGroup(grp, group))
			return;

		vector<string> members;
		group.GetMembers(members);
		groupMembers[grp] = members;
	}

	wxFileName fileInfo(event.GetPath());
	fileName = fileInfo.GetFullPath();
	RefreshUI(true);
}

void GroupManager::OnSaveGroup(wxCommandEvent& WXUNUSED(event)) {
	SliderSetGroupFile groupFile;
	groupFile.New(fileName.ToStdString());
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

void GroupManager::OnSaveGroupAs(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog file(this, "Saving group XML file...", "SliderGroups", fileName, "Group Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (file.ShowModal() != wxID_OK)
		return;

	SliderSetGroupFile groupFile;
	groupFile.New(file.GetPath().ToStdString());
	for (auto &grp : groupMembers) {
		SliderSetGroup group;
		group.SetName(grp.first);
		group.AddMembers(grp.second);
		groupFile.UpdateGroup(group);
	}

	groupFile.Save();
	fileName = file.GetPath();

	dirty = false;
	btSave->Enable(dirty & !fileName.empty());
}

void GroupManager::OnSelectGroup(wxCommandEvent& WXUNUSED(event)) {
	RefreshUI();
}

void GroupManager::OnAddGroup(wxCommandEvent& WXUNUSED(event)) {
	wxString name = groupName->GetValue().Trim();
	if (name.empty())
		return;

	groupName->Clear();

	// Already exists
	auto it = groupMembers.find(name.ToStdString());
	if (it != groupMembers.end())
		return;

	// Create empty group entry
	groupMembers[name.ToStdString()] = vector<string>();
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
	groupMembers.erase(group.ToStdString());

	dirty = true;
	RefreshUI(true);
}

void GroupManager::OnRemoveMember(wxCommandEvent& WXUNUSED(event)) {
	wxArrayInt selections;
	listMembers->GetSelections(selections);

	// Find and remove member from selected group
	string selectedGroup = listGroups->GetStringSelection().ToStdString();
	if (!selectedGroup.empty()) {
		for (int i = 0; i < selections.size(); i++) {
			string member = listMembers->GetString(selections[i]);
			auto it = find(groupMembers[selectedGroup].begin(), groupMembers[selectedGroup].end(), member);
			if (it != groupMembers[selectedGroup].end())
				groupMembers[selectedGroup].erase(it);
		}
	}

	dirty = true;
	selections.Clear();
	RefreshUI();
}

void GroupManager::OnAddMember(wxCommandEvent& WXUNUSED(event)) {
	wxArrayInt selections;
	listOutfits->GetSelections(selections);

	// Add member to selected group
	string selectedGroup = listGroups->GetStringSelection().ToStdString();
	if (!selectedGroup.empty()) {
		for (int i = 0; i < selections.size(); i++) {
			string member = listOutfits->GetString(selections[i]);
			groupMembers[selectedGroup].push_back(member);
		}
	}

	dirty = true;
	selections.Clear();
	RefreshUI();
}
