#pragma once

#include "stdafx.h"
#include "SliderGroup.h"

#include <wx/filepicker.h>
#include <wx/xrc/xmlres.h>

using namespace std;

class GroupManager : public wxDialog {
private:
	wxListBox* listGroups;
	wxTextCtrl* groupName;
	wxButton* btAddGroup;
	wxButton* btSave;
	wxButton* btSaveAs;
	wxListBox* listMembers;
	wxListBox* listOutfits;

	SliderSetGroupFile currentGroupFile;
	map<string, string> groupMembers;

public:
	GroupManager(wxWindow* parent);
	~GroupManager();

	void ClearUI();
	void OnLoadGroup(wxFileDirPickerEvent& event);
	void OnSelectGroup(wxCommandEvent& event);
	void OnAddGroup(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
