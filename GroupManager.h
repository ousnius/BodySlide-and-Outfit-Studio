#pragma once
#include "stdafx.h"
#include "wx/wx.h"
#include "wx/image.h"
#include "wx/xrc/xmlres.h"
#include <string>

#include "SliderGroup.h"

using namespace std;

class GroupManager :
	public wxDialog
{

	wxCheckListBox* lstGroups;
	wxCheckListBox* lstOutfits;
	wxListBox* lstGroupMembers;

	string FilterString;

public:
	GroupManager( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Filter Group and Outfits"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 696,434 ), long style = wxDEFAULT_DIALOG_STYLE );
	~GroupManager(void);

	void OnAddGroup (wxCommandEvent& event) ;
	void OnAddOutfitToGroup(wxCommandEvent& event);
	void OnRemoveOutfit(wxCommandEvent& event) ;

    DECLARE_EVENT_TABLE()
};

