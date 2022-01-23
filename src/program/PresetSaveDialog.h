/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <set>
#include <wx/srchctrl.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

class PresetSaveDialog : public wxDialog {
public:
	std::vector<std::string> allGroupNames;
	std::vector<std::string> filteredGroups;
	std::set<std::string> selectedGroups;
	std::string outFileName;
	std::string outPresetName;
	std::vector<std::string> outGroups;

	PresetSaveDialog(wxWindow* parent);
	~PresetSaveDialog();

	std::string GetProjectPath() const;

	void FilterGroups(const std::string& filter = "");

	void OnEraseBackground(wxEraseEvent& event);
	void FilterChanged(wxCommandEvent& event);
	void CheckGroup(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};
