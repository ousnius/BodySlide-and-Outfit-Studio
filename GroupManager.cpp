#include "GroupManager.h"

BEGIN_EVENT_TABLE (GroupManager, wxDialog)
	EVT_FILEPICKER_CHANGED(XRCID("fpGroupXML"), GroupManager::OnLoadGroup)
	EVT_LISTBOX(XRCID("listGroups"), GroupManager::OnSelectGroup)
	EVT_BUTTON(XRCID("btAddGroup"), GroupManager::OnAddGroup)
END_EVENT_TABLE();

GroupManager::GroupManager(wxWindow* parent, vector<string> outfits) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->LoadDialog(this, parent, "dlgGroupManager");

	SetSize(700, 430);
	SetDoubleBuffered(true);
	CenterOnParent();

	listGroups = (wxListBox*)FindWindowByName("listGroups", this);
	groupName = (wxTextCtrl*)FindWindowByName("groupName", this);
	btAddGroup = (wxButton*)FindWindowByName("btAddGroup", this);
	btSave = (wxButton*)FindWindowByName("btSave", this);
	btSaveAs = (wxButton*)FindWindowByName("btSaveAs", this);
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
	wxString name = groupName->GetValue();
	if (name.empty())
		return;

	listGroups->Append(name);
}
