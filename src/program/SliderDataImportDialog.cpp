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

	shapesCheckListBox = XRCCTRL(*this, "sliderShapesImportList", wxCheckListBox);
	slidersCheckListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	slidersCheckListBox->Bind(wxEVT_RIGHT_UP, &SliderDataImportDialog::OnSliderListContext, this);

	SetDoubleBuffered(true);
	CenterOnParent();
}

SliderDataImportDialog::~SliderDataImportDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SliderDataImport.xrc");
}

int SliderDataImportDialog::ShowModal(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& sliderData) {

	std::unordered_set<std::string> addedSliders;
	for (auto& shapeSliders : sliderData) {
		auto shapeName = project->TargetToShape(shapeSliders.first);
		shapesCheckListBox->Append(shapeName, new ShapeSliderData(shapeName, shapeSliders.first, shapeSliders.second));

		for (auto& sliderName : shapeSliders.second) {
			if (addedSliders.find(sliderName.first) != addedSliders.end())
				continue;

			slidersCheckListBox->Append(sliderName.first, new SliderNameData(sliderName.first, sliderName.second));
			addedSliders.emplace(sliderName.first);
		}
	}

	for (uint32_t i = 0; i < shapesCheckListBox->GetCount(); i++)
		shapesCheckListBox->Check(i);

	for (uint32_t i = 0; i < slidersCheckListBox->GetCount(); i++)
		slidersCheckListBox->Check(i);

	return wxDialog::ShowModal();
}

void SliderDataImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {

	options.mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "chkImportMergeSliders");

	wxArrayInt checked;
	slidersCheckListBox->GetCheckedItems(checked);
	std::unordered_set<std::string> selectedSliders;
	for (const auto& i : checked) {
		const auto data = dynamic_cast<SliderNameData*>(slidersCheckListBox->GetClientObject(i));
		selectedSliders.emplace(data->displayName);
	}

	shapesCheckListBox->GetCheckedItems(checked);
	for (const auto& i : checked) {
		const auto data = dynamic_cast<ShapeSliderData*>(shapesCheckListBox->GetClientObject(i));
		for (auto sliderNames : data->sliderNames) {
			if (selectedSliders.find(sliderNames.first) == selectedSliders.end())
				continue;

			options.selectedShapesToSliders[data->targetShape].emplace(sliderNames.second, sliderNames.first);
		}
	}

	EndModal(wxID_OK);
}

void SliderDataImportDialog::OnSliderListContext(wxMouseEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("sliderDataContext");
	if (menu) {
		menu->Bind(wxEVT_MENU, &SliderDataImportDialog::OnSliderListContextSelect, this);
		PopupMenu(menu);
		delete menu;
	}
}

void SliderDataImportDialog::OnSliderListContextSelect(wxCommandEvent& event) {
	if (event.GetId() == XRCID("sliderDataContextNone")) {
		for (uint32_t i = 0; i < slidersCheckListBox->GetCount(); i++)
			slidersCheckListBox->Check(i, false);
	}
	else if (event.GetId() == XRCID("sliderDataContextAll")) {
		for (uint32_t i = 0; i < slidersCheckListBox->GetCount(); i++)
			slidersCheckListBox->Check(i);
	}
	else if (event.GetId() == XRCID("sliderDataContextInvert")) {
		for (uint32_t i = 0; i < slidersCheckListBox->GetCount(); i++)
			slidersCheckListBox->Check(i, !slidersCheckListBox->IsChecked(i));
	}
}