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

class ShapeSliderData : public wxClientData {
public:
	ShapeSliderData(const std::string& name, const std::string& originalName, const std::unordered_map<std::string, std::string>& sliderNames)
	:  shapeName(originalName), targetShape(name), sliderNames(sliderNames) {}
	std::string shapeName;
	std::string targetShape;
	std::unordered_map<std::string, std::string> sliderNames;
};

class SliderNameData : public wxClientData {
public:
	SliderNameData(const std::string& displayName, const std::string& originalName)
		: displayName(displayName), originalName(originalName){}
	std::string displayName;
	std::string originalName;
};

class SliderDataImportOptions {
public:
	SliderDataImportOptions()
		: mergeSliders(false) {}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> selectedShapesToSliders;
	bool mergeSliders;
};


class SliderDataImportDialog : public wxDialog {
public:
	SliderDataImportDialog(wxWindow* parent, OutfitProject* project, ConfigurationManager& outfitStudioConfig);
	~SliderDataImportDialog();

	int ShowModal(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& sliderData);
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


