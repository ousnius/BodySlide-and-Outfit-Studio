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

	const auto checkListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	for (auto& sliderName : sliderNames)
		checkListBox->Append(sliderName);

	for (uint32_t i = 0; i < sliderNames.size(); i++)
		checkListBox->Check(i);

	return wxDialog::ShowModal();
}

void SliderDataImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {

	options.mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "chkImportMergeSliders");

	wxArrayInt checked;
	const auto checkListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	checkListBox->GetCheckedItems(checked);
	for (const auto& i : checked) {
		auto key = checkListBox->GetString(i).ToStdString();
		options.selectedSliderNames.emplace(key);
	}

	EndModal(wxID_OK);
}
