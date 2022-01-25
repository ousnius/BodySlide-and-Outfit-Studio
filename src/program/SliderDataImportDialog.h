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

	std::unordered_set<std::string> selectedShapeNames;
	std::unordered_set<std::string> selectedSliderNames;
	bool mergeSliders;
};

class SliderDataImportDialog : public wxDialog {
public:
	SliderDataImportDialog(wxWindow* parent, OutfitProject* project, ConfigurationManager& outfitStudioConfig);
	~SliderDataImportDialog();

	int ShowModal(const std::unordered_map<std::string, std::vector<std::string>>& sliderData);
	void OnImport(wxCommandEvent& event);

	SliderDataImportOptions GetOptions() { return options; }
	wxDECLARE_EVENT_TABLE();

private:
	void OnSliderListContext(wxMouseEvent& WXUNUSED(event));
	void OnSliderListContextSelect(wxCommandEvent& event);

	wxCheckListBox* shapesCheckListBox;
	wxCheckListBox* slidersCheckListBox;
	OutfitProject* project;
	ConfigurationManager& outfitStudioConfig;
	SliderDataImportOptions options;
};


