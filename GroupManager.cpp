#include "GroupManager.h"

BEGIN_EVENT_TABLE (GroupManager, wxDialog)
	EVT_FILEPICKER_CHANGED(XRCID("fpGroupXML"), GroupManager::OnLoadGroup)
	EVT_LISTBOX(XRCID("listGroups"), GroupManager::OnSelectGroup)
	EVT_BUTTON(XRCID("btAddGroup"), GroupManager::OnAddGroup)
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

	listGroups = (wxListBox*)FindWindowByName("listGroups", this);
	groupName = (wxTextCtrl*)FindWindowByName("groupName", this);
	btAddGroup = (wxButton*)FindWindowByName("btAddGroup", this);
	btSave = (wxButton*)FindWindowByName("btSave", this);
	btSaveAs = (wxButton*)FindWindowByName("btSaveAs", this);
	btRemoveMember = (wxButton*)FindWindowByName("btRemoveMember", this);
	btAddMember = (wxButton*)FindWindowByName("btAddMember", this);
	listMembers = (wxListBox*)FindWindowByName("listMembers", this);
	listOutfits = (wxListBox*)FindWindowByName("listOutfits", this);

	allOutfits = outfits;
	RefreshLists();
}

GroupManager::~GroupManager() {
}

void GroupManager::RefreshLists(const bool& clearGroups) {
	listMembers->Clear();
	listOutfits->Clear();

	// Get group selection if existing
	string selectedGroup = listGroups->GetStringSelection().ToStdString();

	if (clearGroups) {
		// Add groups to list
		listGroups->Clear();
		for (auto group : groupMembers)
			listGroups->Append(group.first);
	}

	// Add members of selected group to list
	if (!selectedGroup.empty())
		for (auto member : groupMembers[selectedGroup])
			listMembers->Append(member);

	// Add outfits that are no members to list
	for (auto outfit : allOutfits)
		if (listMembers->FindString(outfit) == wxNOT_FOUND)
			listOutfits->Append(outfit);
}

void GroupManager::OnLoadGroup(wxFileDirPickerEvent& event) {
	// Clear and load group file
	currentGroupFile.Clear();
	currentGroupFile.Open(event.GetPath().ToStdString());
	if (currentGroupFile.fail())
		return;

	groupMembers.clear();
	btAddGroup->Enable();
	btSave->Enable();

	// Fill group member map
	vector<string> groupNames;
	currentGroupFile.GetGroupNames(groupNames, true, true);
	for (auto grp : groupNames) {
		SliderSetGroup group;
		if (currentGroupFile.GetGroup(grp, group))
			return;

		vector<string> members;
		group.GetMembers(members);
		groupMembers[grp] = members;
	}

	RefreshLists(true);
}

void GroupManager::OnSaveGroup(wxCommandEvent& WXUNUSED(event)) {
	for (auto grp : groupMembers) {
		SliderSetGroup group;
		group.SetName(grp.first);
		group.AddMembers(grp.second);
		currentGroupFile.UpdateGroup(group);
	}
	currentGroupFile.Save();
}

void GroupManager::OnSaveGroupAs(wxCommandEvent& WXUNUSED(event)) {

}

void GroupManager::OnSelectGroup(wxCommandEvent& WXUNUSED(event)) {
	RefreshLists();
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

	selections.Clear();
	RefreshLists();
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

	selections.Clear();
	RefreshLists();
}
