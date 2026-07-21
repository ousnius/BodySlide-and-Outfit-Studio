/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AddProjectDialog.h"

#include "../utils/ConfigurationManager.h"
#include "../utils/ConfigDialogUtil.h"

extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(AddProjectDialog, wxDialog)
	EVT_BUTTON(wxID_OK, AddProjectDialog::OnAdd)
wxEND_EVENT_TABLE()

AddProjectDialog::AddProjectDialog(wxWindow* parent, ConfigurationManager& outfitStudioConfig)
	: outfitStudioConfig(outfitStudioConfig) {

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/AddProject.xrc");
	xrc->LoadDialog(this, parent, "dlgAddProject");
	ConfigDialogUtil::LoadDialogCheckBox(outfitStudioConfig, *this, "AddProject", "chkSliderDataLocal");
	ConfigDialogUtil::LoadDialogCheckBox(outfitStudioConfig, *this, "AddProject", "chkAppendNewSliders");
	ConfigDialogUtil::LoadDialogCheckBox(outfitStudioConfig, *this, "AddProject", "chkSetAsReference");

	Fit();
	SetDoubleBuffered(true);
	CenterOnParent();
}

AddProjectDialog::~AddProjectDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/AddProject.xrc");
}

void AddProjectDialog::OnAdd(wxCommandEvent& WXUNUSED(event)) {
	options.sliderDataLocal = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "AddProject", "chkSliderDataLocal");
	options.appendNewSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "AddProject", "chkAppendNewSliders");
	options.setAsReference = ConfigDialogUtil::SetBoolFromDialogCheckbox(outfitStudioConfig, *this, "AddProject", "chkSetAsReference");
	EndModal(wxID_OK);
}
