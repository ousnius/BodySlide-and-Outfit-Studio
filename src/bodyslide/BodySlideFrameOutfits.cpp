/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The BodySlide window: the outfit and preset lists, groups and favorites

#include "BodySlideFrame.h"
#include "BodySlideApp.h"
#include "ui/GroupManager.h"
#include "ui/PresetSaveDialog.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <set>
#include <utility>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/wx.h>

using namespace nifly;

void BodySlideFrame::ClearPresetList() {
	presetChoiceNames.clear();
	if (presetChoice)
		presetChoice->Clear();

	UpdateFavoriteButtons();
}

void BodySlideFrame::ClearOutfitList() {
	outfitChoiceNames.clear();
	if (outfitChoice)
		outfitChoice->Clear();

	ShowOutfitBaseProject(nullptr);
	ShowOutfitAddOns(nullptr);
	UpdateFavoriteButtons();
}

void BodySlideFrame::SetPresetChanged(bool changed) {
	if (btnSavePreset)
		btnSavePreset->Enable(changed);
}

wxString BodySlideFrame::OutfitChoiceSuffix(const std::string& name) const {
	bool addOn = false;
	if (app->GetOutfitBaseProject(name, &addOn).empty())
		return wxString();

	return addOn ? _(" (add-on)") : _(" (variant)");
}

void BodySlideFrame::ShowOutfitBaseProject(const SliderSet* set) {
	if (!lblOutfitBase)
		return;

	wxString text;
	wxColour colour;
	wxString tip;
	if (set && set->HasBaseProject()) {
		wxString baseName = wxString::FromUTF8(set->GetBaseProjectName());
		if (set->GetResolveStatus().baseMissing) {
			text = wxString::Format(set->IsAddOn() ? _("Add-on of %s (not found)") : _("Variant of %s (not found)"), baseName);
			colour = wxColour(255, 110, 110);
			tip = _("The base project wasn't found, so this project is built as it was last saved.");
		}
		else {
			text = wxString::Format(set->IsAddOn() ? _("Add-on of %s") : _("Variant of %s"), baseName);
			colour = set->IsAddOn() ? wxColour(255, 190, 90) : wxColour(120, 190, 255);
			tip = set->IsAddOn() ? _("Adds sliders to the base project and builds its shapes.") : _("Has its own shapes and uses the sliders of the base project.");
		}

		if (!set->GetBaseProjectFile().empty())
			tip += "\n" + wxString::FromUTF8(set->GetBaseProjectFile());
	}

	bool show = !text.empty();
	if (show == lblOutfitBase->IsShown() && text == lblOutfitBase->GetLabel())
		return;

	lblOutfitBase->SetLabel(text);
	lblOutfitBase->SetToolTip(tip);
	if (show)
		lblOutfitBase->SetForegroundColour(colour);

	lblOutfitBase->Show(show);
	Layout();
}

void BodySlideFrame::ShowOutfitAddOns(const BodySlideApp::ProjectAddOns* addOns) {
	if (!lblOutfitAddOns)
		return;

	wxString text;
	wxString tip;
	wxColour colour(120, 190, 255);
	if (addOns) {
		std::vector<std::string> stacked;
		for (auto* list : {&addOns->stacked, &addOns->baseStacked})
			for (auto& addOn : *list)
				if (std::find(stacked.begin(), stacked.end(), addOn.setName) == stacked.end())
					stacked.push_back(addOn.setName);

		auto listNames = [](const std::vector<std::string>& names) {
			wxString list;
			for (auto& name : names)
				list += "\n  " + wxString::FromUTF8(name);
			return list;
		};

		const size_t count = stacked.size() + addOns->conformed.size();
		if (count > 0)
			text = wxString::Format(wxPLURAL("+%zu add-on", "+%zu add-ons", count), count);

		if (!stacked.empty())
			tip += _("Sliders from add-ons:") + listNames(stacked);

		if (!addOns->conformed.empty()) {
			if (!tip.empty())
				tip += "\n\n";
			tip += wxString::Format(_("Conformed automatically from add-ons of '%s':"), wxString::FromUTF8(addOns->conformBody)) + listNames(addOns->conformed);
			colour = wxColour(255, 190, 90);
		}

		if (!addOns->missing.empty()) {
			if (!tip.empty())
				tip += "\n\n";
			tip += _("Not installed, so their sliders are left out:") + listNames(addOns->missing);

			wxString missingText = wxString::Format(wxPLURAL("%zu add-on not installed", "%zu add-ons not installed", addOns->missing.size()), addOns->missing.size());
			text = text.empty() ? missingText : text + ", " + missingText;
			colour = wxColour(255, 110, 110);
		}
	}

	bool show = !text.empty();
	if (show == lblOutfitAddOns->IsShown() && text == lblOutfitAddOns->GetLabel() && tip == lblOutfitAddOns->GetToolTipText())
		return;

	lblOutfitAddOns->SetLabel(text);
	lblOutfitAddOns->SetToolTip(tip);
	if (show)
		lblOutfitAddOns->SetForegroundColour(colour);

	lblOutfitAddOns->Show(show);
	Layout();
}

bool BodySlideFrame::SelectChoiceName(wxFavoriteChoice* choice, const std::vector<std::string>& names, const std::string& selectItem) const {
	for (size_t i = 0; i < names.size(); i++) {
		if (names[i] == selectItem) {
			choice->SetSelection(i);
			return true;
		}
	}

	return false;
}

void BodySlideFrame::SetFavoriteButtonBitmap(wxButton* button, bool favorite) const {
	if (!button)
		return;

	wxString iconPath = wxString::FromUTF8(Config["AppDir"]) + wxString::FromUTF8(favorite ? FavoriteStarIcon : FavoriteStarEmptyIcon);
	wxBitmap bitmap(iconPath, wxBITMAP_TYPE_PNG);
	if (bitmap.IsOk()) {
		button->SetLabel("");
		button->SetBitmap(bitmap);
	}
}

void BodySlideFrame::PopulateOutfitList(const wxArrayString& items, const wxString& selectItem) {
	if (!outfitChoice)
		return;

	outfitChoiceNames.clear();

	for (size_t i = 0; i < items.GetCount(); i++) {
		std::string rawName = items[i].ToUTF8().data();
		outfitChoiceNames.push_back(rawName);
	}

	RebuildOutfitChoice(selectItem.ToUTF8().data());
}

void BodySlideFrame::RebuildOutfitChoice(const std::string& selectItem) {
	if (!outfitChoice)
		return;

	std::string selectedName = selectItem;
	if (selectedName.empty())
		selectedName = GetSelectedOutfitName();

	app->SortOutfitNamesForDisplay(outfitChoiceNames);

	populatingChoices = true;
	wxEventBlocker blocker(outfitChoice, wxEVT_COMBOBOX);
	outfitChoice->Freeze();
	outfitChoice->Clear();

	for (const auto& rawName : outfitChoiceNames)
		outfitChoice->AppendItem(wxString::FromUTF8(rawName), app->IsFavoriteOutfit(rawName), OutfitChoiceSuffix(rawName));

	if (!SelectChoiceName(outfitChoice, outfitChoiceNames, selectedName)) {
		int i = wxNOT_FOUND;
		wxString missingItem = wxString::FromUTF8(selectedName);
		if (missingItem.empty()) {
			outfitChoiceNames.push_back("");
			i = outfitChoice->AppendItem("");
		}
		else if (!missingItem.StartsWith("[")) {
			outfitChoiceNames.push_back(selectedName);
			i = outfitChoice->AppendItem("[" + missingItem + "]", false, OutfitChoiceSuffix(selectedName));
		}
		else {
			outfitChoiceNames.push_back(selectedName);
			i = outfitChoice->AppendItem(missingItem);
		}

		outfitChoice->SetSelection(i);
	}

	outfitChoice->Thaw();
	populatingChoices = false;
	UpdateFavoriteButtons();
}

void BodySlideFrame::PopulatePresetList(const wxArrayString& items, const wxString& selectItem) {
	if (!presetChoice)
		return;

	presetChoiceNames.clear();

	for (size_t i = 0; i < items.GetCount(); i++) {
		std::string rawName = items[i].ToUTF8().data();
		presetChoiceNames.push_back(rawName);
	}

	RebuildPresetChoice(selectItem.ToUTF8().data());
}

void BodySlideFrame::RebuildPresetChoice(const std::string& selectItem) {
	if (!presetChoice)
		return;

	std::string selectedName = selectItem;
	if (selectedName.empty())
		selectedName = GetSelectedPresetName();

	app->SortPresetNamesForDisplay(presetChoiceNames);

	populatingChoices = true;
	wxEventBlocker blocker(presetChoice, wxEVT_COMBOBOX);
	presetChoice->Freeze();
	presetChoice->Clear();

	for (const auto& rawName : presetChoiceNames)
		presetChoice->AppendItem(wxString::FromUTF8(rawName), app->IsFavoritePreset(rawName));

	if (!SelectChoiceName(presetChoice, presetChoiceNames, selectedName)) {
		int i = wxNOT_FOUND;
		wxString missingItem = wxString::FromUTF8(selectedName);
		if (missingItem.empty()) {
			presetChoiceNames.push_back("");
			i = presetChoice->AppendItem("");
		}
		else if (!missingItem.StartsWith("[")) {
			presetChoiceNames.push_back(selectedName);
			i = presetChoice->AppendItem("[" + missingItem + "]");
		}
		else {
			presetChoiceNames.push_back(selectedName);
			i = presetChoice->AppendItem(missingItem);
		}

		presetChoice->SetSelection(i);
	}

	presetChoice->Thaw();
	populatingChoices = false;
	UpdateFavoriteButtons();
}

std::string BodySlideFrame::GetSelectedOutfitName() const {
	if (!outfitChoice)
		return "";

	int selection = outfitChoice->GetSelection();
	if (selection < 0 || selection >= static_cast<int>(outfitChoiceNames.size()))
		return "";

	return outfitChoiceNames[selection];
}

std::string BodySlideFrame::GetSelectedPresetName() const {
	if (!presetChoice)
		return "";

	int selection = presetChoice->GetSelection();
	if (selection < 0 || selection >= static_cast<int>(presetChoiceNames.size()))
		return "";

	return presetChoiceNames[selection];
}

void BodySlideFrame::UpdateFavoriteButtons() {
	if (btnFavoriteOutfit) {
		std::string outfitName = GetSelectedOutfitName();
		bool canFavorite = !outfitName.empty() && app->OutfitExists(outfitName);
		bool isFavorite = canFavorite && app->IsFavoriteOutfit(outfitName);
		btnFavoriteOutfit->Enable(canFavorite);
		SetFavoriteButtonBitmap(btnFavoriteOutfit, isFavorite);
		btnFavoriteOutfit->SetToolTip(isFavorite ? _("Remove this outfit/body from favorites") : _("Favorite this outfit/body"));
	}

	if (btnFavoritePreset) {
		std::string presetName = GetSelectedPresetName();
		bool canFavorite = !presetName.empty() && app->PresetExists(presetName);
		bool isFavorite = canFavorite && app->IsFavoritePreset(presetName);
		btnFavoritePreset->Enable(canFavorite);
		SetFavoriteButtonBitmap(btnFavoritePreset, isFavorite);
		btnFavoritePreset->SetToolTip(isFavorite ? _("Remove this preset from favorites") : _("Favorite this preset"));
	}
}

void BodySlideFrame::OnSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnOutfitSearchChange(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnPresetFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	app->PopulatePresetList("");
}

void BodySlideFrame::OnChooseGroups(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> groupNames;
	std::unordered_set<std::string> curGroupNames;
	app->GetAllGroupNames(groupNames);

	wxString srch = search->GetValue();
	wxStringTokenizer tokenizer(srch, ",;");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim();
		token.Trim(false);
		curGroupNames.insert(token.ToUTF8().data());
	}

	wxArrayString grpChoices;
	wxArrayInt grpSelections;
	int idx = 0;
	for (auto& g : groupNames) {
		grpChoices.Add(wxString::FromUTF8(g));
		if (curGroupNames.find(g) != curGroupNames.end())
			grpSelections.Add(idx);

		idx++;
	}

	grpChoices.Add("Unassigned");

	// Cast and use wxAnyChoiceDialog "Create" function so we can set the wxLB_EXTENDED flag for the internal wxCheckListBox
	wxMultiChoiceDialog chooser{};
	wxAnyChoiceDialog* anyChooser = dynamic_cast<wxAnyChoiceDialog*>(&chooser);
	anyChooser->Create(this, _("Choose groups to filter outfit list"), _("Choose Groups"), grpChoices, wxCHOICEDLG_STYLE, wxDefaultPosition, wxLB_ALWAYS_SB | wxLB_EXTENDED);
	chooser.SetSelections(grpSelections);

	wxString filter;
	if (chooser.ShowModal() == wxID_OK) {
		wxArrayInt sel = chooser.GetSelections();
		for (size_t i = 0; i < sel.size(); i++) {
			if (i > 0)
				filter += ", ";
			filter += grpChoices[sel[i]];
		}
		search->ChangeValue(filter);
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnBrowseOutfitFolder(wxCommandEvent& WXUNUSED(event)) {
	if (!app->HasActiveProject())
		return;

	auto& activeSet = app->GetActiveSet();

	wxFileName folderPath(wxString::FromUTF8(activeSet.GetInputFileName()));
	if (folderPath.IsRelative())
		folderPath.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	folderPath.SetFullName("");

	if (!folderPath.FileExists() && folderPath.DirExists())
		wxLaunchDefaultApplication(folderPath.GetPath());
}

void BodySlideFrame::OnSaveGroups(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	wxFileDialog saveGroupDialog(this,
								 _("Choose or create group file"),
								 wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderGroups",
								 wxEmptyString,
								 "Group Files (*.xml)|*.xml",
								 wxFD_SAVE);
	if (saveGroupDialog.ShowModal() == wxID_CANCEL)
		return;

	wxString fName = saveGroupDialog.GetPath();
	wxString gName;
	int ret = 0;

	do {
		gName = wxGetTextFromUser(_("What would you like the new group to be called?"), _("New Group Name"));
		if (gName.IsEmpty())
			return;

		ret = app->SaveGroupList(fName.ToUTF8().data(), gName.ToUTF8().data());
	} while (ret == 2);

	if (ret == 0) {
		app->LoadAllGroups();
		app->RefreshPresetsForCurrentOutfit();
		search->ChangeValue(gName);
		outfitsearch->ChangeValue("");
		app->PopulateOutfitList("");
	}
}

void BodySlideFrame::OnRefreshGroups(wxCommandEvent& WXUNUSED(event)) {
	app->LoadAllGroups();
	app->RefreshPresetsForCurrentOutfit();
}

void BodySlideFrame::OnRefreshOutfits(wxCommandEvent& WXUNUSED(event)) {
	app->RefreshOutfitList();

	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	app->ActivateOutfit(outfitName);
}

void BodySlideFrame::OnRegexOutfits(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnFilterHasZaps(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateFilterData();
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnFilterOutputWinners(wxCommandEvent& WXUNUSED(event)) {
	app->PopulateFilterData();
	app->PopulateOutfitList("");
}

void BodySlideFrame::OnChooseOutfit(wxCommandEvent& WXUNUSED(event)) {
	if (populatingChoices)
		return;

	std::string sstr = GetSelectedOutfitName();
	if (sstr.empty())
		return;

	app->ActivateOutfit(sstr);
	UpdateFavoriteButtons();
}

void BodySlideFrame::OnChoosePreset(wxCommandEvent& WXUNUSED(event)) {
	if (populatingChoices)
		return;

	std::string sstr = GetSelectedPresetName();

	app->ActivatePreset(sstr);
	UpdateFavoriteButtons();
}

void BodySlideFrame::OnFavoriteOutfit(wxCommandEvent& WXUNUSED(event)) {
	std::string outfitName = GetSelectedOutfitName();
	if (outfitName.empty() || !app->OutfitExists(outfitName))
		return;

	app->ToggleFavoriteOutfit(outfitName);
	RebuildOutfitChoice(outfitName);
}

void BodySlideFrame::OnFavoritePreset(wxCommandEvent& WXUNUSED(event)) {
	std::string presetName = GetSelectedPresetName();
	if (presetName.empty() || !app->PresetExists(presetName))
		return;

	app->ToggleFavoritePreset(presetName);
	RebuildPresetChoice(presetName);
}

void BodySlideFrame::OnDeleteProject(wxCommandEvent& WXUNUSED(event)) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];

	wxString msg = _("Do you really wish to delete the selected project?");
	std::vector<std::string> dependents = app->GetDerivedProjects(outfitName);
	if (!dependents.empty())
		msg += "\n\n"
			   + wxString::Format(_("These projects are variants or add-ons of it: %s. They keep the sliders they were last saved with, but get no more changes from it."),
								  wxString::FromUTF8(JoinStrings(dependents, ", ")));

	int res = wxMessageBox(msg, _("Delete Project"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	app->DeleteOutfit(outfitName);
}

void BodySlideFrame::OnDeletePreset(wxCommandEvent& WXUNUSED(event)) {
	int res = wxMessageBox(_("Do you really wish to delete the selected preset?"), _("Delete Preset"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	app->DeletePreset(presetName);
}

void BodySlideFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	if (!btnSavePreset->IsEnabled())
		return;

	if (OutfitIsEmpty())
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	if (presetName.empty())
		return;

	int error = app->UpdateSliderPositions(presetName);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to save preset (%d)!"), error), _("Error"));
	}

	SetPresetChanged(false);
	app->LoadPresets("");
	app->PopulatePresetList(presetName);
}

void BodySlideFrame::OnEditPreset(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	std::string presetName = BodySlideConfig["SelectedPreset"];
	if (presetName.empty())
		return;

	std::string presetFileName = app->GetPresetFileName(presetName);
	if (presetFileName.empty()) {
		wxLogError("Failed to find preset file for '%s'!", presetName);
		wxMessageBox(wxString::Format(_("Failed to find preset file for '%s'!"), presetName), _("Error"), wxICON_ERROR, this);
		return;
	}

	std::vector<std::string> groups;
	app->GetPresetGroups(presetName, groups);

	PresetSaveDialog psd(this);
	app->GetAllGroupNames(psd.allGroupNames);
	for (auto& group : groups) {
		if (std::find(psd.allGroupNames.begin(), psd.allGroupNames.end(), group) == psd.allGroupNames.end())
			psd.allGroupNames.push_back(group);
	}
	psd.SetExistingPreset(presetName, presetFileName, groups);
	psd.FilterGroups();
	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	int error = app->SavePresetGroups(psd.outFileName, presetName, groups);
	if (error) {
		wxLogError("Failed to save preset '%s' (%d)!", presetName, error);
		wxMessageBox(wxString::Format(_("Failed to save preset '%s' (%d)!"), presetName, error), _("Error"), wxICON_ERROR, this);
	}

	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	BodySlideConfig.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnSavePresetAs(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	std::vector<std::string> groups;
	PresetSaveDialog psd(this);

	app->GetAllGroupNames(psd.allGroupNames);
	psd.FilterGroups();
	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	std::string fname = psd.outFileName;
	std::string presetName = psd.outPresetName;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	int error = app->SaveSliderPositions(fname, presetName, groups);
	if (error) {
		wxLogError("Failed to save preset as '%s' (%d)!", fname, error);
		wxMessageBox(wxString::Format(_("Failed to save preset as '%s' (%d)!"), fname, error), "Error");
	}

	SetPresetChanged(false);
	app->LoadPresets("");
	app->PopulatePresetList(presetName);
	BodySlideConfig.SetValue("SelectedPreset", presetName);
}

void BodySlideFrame::OnGroupManager(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> outfits;
	app->GetOutfits(outfits);

	GroupManager gm(this, outfits);
	gm.ShowModal();
	app->LoadAllGroups();
	app->RefreshPresetsForCurrentOutfit();
}

void BodySlideFrame::OnConflictPopup(wxMouseEvent& WXUNUSED(event)) {
	std::vector<std::string> conflictingOutfits = app->GetConflictingOutfits();
	std::string currentOutfitName = BodySlideConfig["SelectedOutfit"];

	int id = fileCollisionMenu->GetMenuItemCount();
	while (id--)
		fileCollisionMenu->Delete(id);

	for (id = 0; id < static_cast<int>(conflictingOutfits.size()); id++) {
		std::string& outfitName = conflictingOutfits[id];
		wxMenuItem* outfitItem = fileCollisionMenu->AppendRadioItem(id, wxString::FromUTF8(outfitName));

		if (outfitName == currentOutfitName)
			outfitItem->Check();
	}

	id = GetPopupMenuSelectionFromUser(*fileCollisionMenu, wxDefaultPosition);
	if (id < 0)
		return;

	std::string& selectedOutfitName = conflictingOutfits[id];
	if (selectedOutfitName != currentOutfitName)
		app->ActivateOutfit(selectedOutfitName);
}

void BodySlideFrame::OnOutfitChoiceSelect(wxCommandEvent& WXUNUSED(event)) {
	app->SetDefaultBuildSelection();
}
