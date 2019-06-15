/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/SliderGroup.h"

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/filepicker.h>

class GroupManager : public wxDialog {
public:
	GroupManager(wxWindow*, std::vector<std::string>);
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
	std::map<std::string, std::vector<std::string>> groupMembers;
	std::vector<std::string> allOutfits;

	void RefreshUI(const bool = false);
	void SaveGroup();

	void DoRemoveMembers();
	void DoAddMembers();

	void OnLoadGroup(wxFileDirPickerEvent&);
	void OnSaveGroup(wxCommandEvent&);
	void OnSaveGroupAs(wxCommandEvent&);
	void OnSelectGroup(wxCommandEvent&);
	void OnDblClickMember(wxCommandEvent&);
	void OnDblClickOutfit(wxCommandEvent&);
	void OnAddGroup(wxCommandEvent&);
	void OnRemoveGroup(wxCommandEvent&);
	void OnRemoveMember(wxCommandEvent&);
	void OnAddMember(wxCommandEvent&);
	void OnCloseButton(wxCommandEvent&);
	void OnClose(wxCloseEvent&);

	wxDECLARE_EVENT_TABLE();
};
