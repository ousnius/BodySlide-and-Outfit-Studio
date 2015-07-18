#pragma once

#include "stdafx.h"
#include "SliderGroup.h"

#include <wx/xrc/xmlres.h>
#include <wx/filepicker.h>

using namespace std;

class GroupManager : public wxDialog {
public:
	GroupManager(wxWindow*, vector<string>);
	~GroupManager();

private:
	wxListBox* listGroups;
	wxTextCtrl* groupName;
	wxButton* btAddGroup;
	wxButton* btRemoveGroup;
	wxButton* btSave;
	wxButton* btSaveAs;
	wxButton* btRemoveMember;
	wxButton* btAddMember;
	wxListBox* listMembers;
	wxListBox* listOutfits;

	bool dirty = false;
	wxString fileName;
	SliderSetGroupFile currentGroupFile;
	map<string, vector<string>> groupMembers;
	vector<string> allOutfits;

	void RefreshUI(const bool& = false);
	void OnLoadGroup(wxFileDirPickerEvent&);
	void OnSaveGroup(wxCommandEvent&);
	void OnSaveGroupAs(wxCommandEvent&);
	void OnSelectGroup(wxCommandEvent&);
	void OnAddGroup(wxCommandEvent&);
	void OnRemoveGroup(wxCommandEvent&);
	void OnRemoveMember(wxCommandEvent&);
	void OnAddMember(wxCommandEvent&);

	DECLARE_EVENT_TABLE()
};
