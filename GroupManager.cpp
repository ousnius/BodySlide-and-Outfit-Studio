#include "GroupManager.h"

BEGIN_EVENT_TABLE (GroupManager, wxDialog)
	EVT_BUTTON(XRCID("btnAddGroup"), GroupManager::OnAddGroup)
	EVT_BUTTON(XRCID("btnAddOutfitToGroup"), GroupManager::OnAddOutfitToGroup)
	EVT_BUTTON(XRCID("btnRemoveOutfit"), GroupManager::OnRemoveOutfit)
END_EVENT_TABLE();

GroupManager::GroupManager(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->Load("res\\GroupManager.xrc");
}

GroupManager::~GroupManager(void) {
}

void GroupManager::OnAddGroup(wxCommandEvent& WXUNUSED(event)) {

}

void GroupManager::OnAddOutfitToGroup(wxCommandEvent& WXUNUSED(event)) {

}

void GroupManager::OnRemoveOutfit(wxCommandEvent& WXUNUSED(event)) {

}