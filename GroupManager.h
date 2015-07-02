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
	wxButton* btSave;
	wxButton* btSaveAs;
	wxButton* btRemoveMember;
	wxButton* btAddMember;
	wxListBox* listMembers;
	wxListBox* listOutfits;

	SliderSetGroupFile currentGroupFile;
	map<string, string> groupMembers;
	vector<string> allOutfits;

	bool mouseDownOutfits = false;
	wxPoint mouseDownPos;

	void ResetUI();
	void OnLoadGroup(wxFileDirPickerEvent&);
	void OnSelectGroup(wxCommandEvent&);
	void OnAddGroup(wxCommandEvent&);
	void OnRemoveMember(wxCommandEvent&);
	void OnAddMember(wxCommandEvent&);

	DECLARE_EVENT_TABLE()
};
