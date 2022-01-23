/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/SliderGroup.h"

#include <wx/filepicker.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

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

	std::string GetProjectPath() const;

	void RefreshUI(const bool = false);
	bool ChooseFile();
	void SaveGroup();

	void DoRenameGroup();
	void DoRemoveMembers();
	void DoAddMembers();
	void DoFilterOutfits(const std::string& filter = "");

	void OnLoadGroup(wxFileDirPickerEvent&);
	void OnSaveGroup(wxCommandEvent&);
	void OnSaveGroupAs(wxCommandEvent&);
	void OnSelectGroup(wxCommandEvent&);
	void OnDblClickGroup(wxCommandEvent&);
	void OnDblClickMember(wxCommandEvent&);
	void OnDblClickOutfit(wxCommandEvent&);
	void OnAddGroup(wxCommandEvent&);
	void OnRemoveGroup(wxCommandEvent&);
	void OnRemoveMember(wxCommandEvent&);
	void OnAddMember(wxCommandEvent&);
	void OnFilterChanged(wxCommandEvent&);
	void OnCloseButton(wxCommandEvent&);
	void OnClose(wxCloseEvent&);

	wxDECLARE_EVENT_TABLE();
};
