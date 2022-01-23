/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderDataImportDialog.h"

#include "OutfitProject.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/ConfigDialogUtil.h"

extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(SliderDataImportDialog, wxDialog)
	EVT_BUTTON(wxID_OK, SliderDataImportDialog::OnImport)
wxEND_EVENT_TABLE()

SliderDataImportDialog::SliderDataImportDialog(wxWindow* parent, OutfitProject* project, ConfigurationManager& outfitStudioConfig)
	: project(project), outfitStudioConfig(outfitStudioConfig) {

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SliderDataImport.xrc");
	xrc->LoadDialog(this, parent, "dlgSliderDataImport");
	ConfigDialogUtil::LoadDialogCheckBox(outfitStudioConfig, *this, "chkImportMergeSliders");

	SetDoubleBuffered(true);
	CenterOnParent();
}

SliderDataImportDialog::~SliderDataImportDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SliderDataImport.xrc");
}

int SliderDataImportDialog::ShowModal(const std::vector<std::string>& sliderNames) {

	wxArrayString filteredSliderNames;
	for (auto& sliderName : sliderNames) {
		std::string bestShapeName;
		nifly::NiShape* bestShape;
		for (auto& shape : project->GetWorkNif()->GetShapes()) {
			std::string shapeName = shape->name.get();
			// Diff name is supposed to begin with matching shape name
			if (sliderName.substr(0, shapeName.size()) != shapeName)
				continue;
			if (shapeName.length() > bestShapeName.length()) {
				bestShapeName = shapeName;
				bestShape = shape;
			}
		}
		if (bestShapeName.length() == 0)
			continue;
		auto newName = sliderName.substr(bestShapeName.length(), sliderName.length() - bestShapeName.length() + 1);
		options.selectedSliderNames[newName] = std::make_tuple(sliderName, bestShape);
		filteredSliderNames.push_back(newName);
	}

	const auto checkListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	checkListBox->Append(filteredSliderNames);

	for (uint32_t i = 0; i < filteredSliderNames.size(); i++) {
		checkListBox->Check(i);
	}

	return wxDialog::ShowModal();
}

void SliderDataImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {

	options.mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "chkImportMergeSliders");

	std::unordered_map<std::string, std::tuple<std::string, nifly::NiShape*>> copySelectedSliderNames = options.selectedSliderNames;
	options.selectedSliderNames.clear();

	wxArrayInt checked;
	const auto checkListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	checkListBox->GetCheckedItems(checked);
	for (auto& i : checked) {
		auto key = checkListBox->GetString(i).ToStdString();
		options.selectedSliderNames.emplace(key, std::move(copySelectedSliderNames[key]));
	}

	EndModal(wxID_OK);
}
