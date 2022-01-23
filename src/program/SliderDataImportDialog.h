/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "ShapeProperties.h"

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>


class OutfitProject;
class ConfigurationManager;

class SliderDataImportOptions {
public:
	SliderDataImportOptions()
		: mergeSliders(false) {}

	std::unordered_set<std::string> selectedSliderNames;
	bool mergeSliders;
};

class SliderDataImportDialog : public wxDialog {
public:
	SliderDataImportDialog(wxWindow* parent, OutfitProject* project, ConfigurationManager& outfitStudioConfig);
	~SliderDataImportDialog();

	int ShowModal(const std::vector<std::string>& sliderNames);
	void OnImport(wxCommandEvent& event);

	SliderDataImportOptions GetOptions() { return options; }
	wxDECLARE_EVENT_TABLE();

private:
	void OnSliderListContext(wxMouseEvent& WXUNUSED(event));
	void OnSliderListContextSelect(wxCommandEvent& event);

	wxCheckListBox* checkListBox;
	OutfitProject* project;
	ConfigurationManager& outfitStudioConfig;
	SliderDataImportOptions options;
};


