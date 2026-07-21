/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

class ConfigurationManager;

class AddProjectOptions {
public:
	AddProjectOptions()
		: sliderDataLocal(true)
		, appendNewSliders(true)
		, setAsReference(true) {}

	bool sliderDataLocal;
	bool appendNewSliders;
	bool setAsReference;
};


class AddProjectDialog : public wxDialog {
public:
	AddProjectDialog(wxWindow* parent, ConfigurationManager& outfitStudioConfig);
	~AddProjectDialog();

	AddProjectOptions GetOptions() { return options; }
	void OnAdd(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();

private:
	ConfigurationManager& outfitStudioConfig;
	AddProjectOptions options;
};
