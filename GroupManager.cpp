#include "GroupManager.h"

BEGIN_EVENT_TABLE (GroupManager, wxDialog)
	EVT_FILEPICKER_CHANGED(XRCID("fpGroupXML"), GroupManager::OnLoadGroup)
	EVT_LISTBOX(XRCID("listGroups"), GroupManager::OnSelectGroup)
	EVT_BUTTON(XRCID("btAddGroup"), GroupManager::OnAddGroup)
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
	for (auto outfit : allOutfits)
		listOutfits->Append(outfit);
}

GroupManager::~GroupManager() {
}

void GroupManager::ResetUI() {
	listGroups->Clear();
	groupName->Clear();
	listMembers->Clear();
	listOutfits->Clear();

	for (auto outfit : allOutfits)
		listOutfits->Append(outfit);
}

void GroupManager::OnLoadGroup(wxFileDirPickerEvent& event) {
	currentGroupFile.Clear();
	currentGroupFile.Open(event.GetPath().ToStdString());
	if (currentGroupFile.fail())
		return;

	ResetUI();
	btAddGroup->Enable();
	btSave->Enable();

	vector<string> groupNames;
	currentGroupFile.GetGroupNames(groupNames, true, true);
	for (auto grp : groupNames)
		listGroups->Append(grp);
}

void GroupManager::OnSelectGroup(wxCommandEvent& event) {
	listMembers->Clear();

	SliderSetGroup group;
	if (currentGroupFile.GetGroup(event.GetString().ToStdString(), group))
		return;

	vector<string> members;
	group.GetMembers(members);
	for (auto mbr : members)
		listMembers->Append(mbr);
}

void GroupManager::OnAddGroup(wxCommandEvent& WXUNUSED(event)) {
	wxString name = groupName->GetValue().Trim();
	if (name.empty())
		return;

	listGroups->Append(name);
}

void GroupManager::OnRemoveMember(wxCommandEvent& WXUNUSED(event)) {
	wxArrayInt selections;
	listMembers->GetSelections(selections);

	for (auto id : selections) {
		listMembers->Delete(id);
		wxString member = listMembers->GetString(id);
		// todo
	}
}

void GroupManager::OnAddMember(wxCommandEvent& WXUNUSED(event)) {

}
