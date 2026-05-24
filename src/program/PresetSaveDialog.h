/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <set>
#include <string>
#include <vector>
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
	void SetExistingPreset(const std::string& presetName, const std::string& presetFileName, const std::vector<std::string>& groups);

	void FilterGroups(const std::string& filter = "");

	void OnEraseBackground(wxEraseEvent& event);
	void FilterChanged(wxCommandEvent& event);
	void CheckGroup(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();

private:
	bool editExistingPreset = false;
	std::string existingPresetFileName;
};
