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

	checkListBox = XRCCTRL(*this, "sliderImportList", wxCheckListBox);
	checkListBox->Bind(wxEVT_RIGHT_UP, &SliderDataImportDialog::OnSliderListContext, this);

	SetDoubleBuffered(true);
	CenterOnParent();
}

SliderDataImportDialog::~SliderDataImportDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/SliderDataImport.xrc");
}

int SliderDataImportDialog::ShowModal(const std::vector<std::string>& sliderNames) {
	for (auto& sliderName : sliderNames)
		checkListBox->Append(sliderName);

	for (uint32_t i = 0; i < sliderNames.size(); i++)
		checkListBox->Check(i);

	return wxDialog::ShowModal();
}

void SliderDataImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {

	options.mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "chkImportMergeSliders");

	wxArrayInt checked;
	checkListBox->GetCheckedItems(checked);
	for (const auto& i : checked) {
		auto key = checkListBox->GetString(i).ToStdString();
		options.selectedSliderNames.emplace(key);
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
		for (uint32_t i = 0; i < checkListBox->GetCount(); i++)
			checkListBox->Check(i, false);
	}
	else if (event.GetId() == XRCID("sliderDataContextAll")) {
		for (uint32_t i = 0; i < checkListBox->GetCount(); i++)
			checkListBox->Check(i);
	}
	else if (event.GetId() == XRCID("sliderDataContextInvert")) {
		for (uint32_t i = 0; i < checkListBox->GetCount(); i++)
			checkListBox->Check(i, !checkListBox->IsChecked(i));
	}
}