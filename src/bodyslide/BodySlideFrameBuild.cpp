/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The BodySlide window: building outfits and batch builds

#include "BodySlideFrame.h"
#include "BodySlideApp.h"

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <algorithm>
#include <set>
#include <utility>

using namespace nifly;

void BodySlideFrame::OnBuildBodies(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	bool tri = false;
	bool forceNormals = false;

	auto cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	auto cbForceBodyNormals = (wxCheckBox*)FindWindowByName("cbForceBodyNormals");
	if (cbForceBodyNormals)
		forceNormals = cbForceBodyNormals->IsChecked();

	if (wxGetKeyState(WXK_CONTROL))
		app->BuildBodies(true, false, tri, forceNormals);
	else if (wxGetKeyState(WXK_ALT))
		app->BuildBodies(false, true, tri, forceNormals);
	else
		app->BuildBodies(false, false, tri, forceNormals);
}

void BodySlideFrame::OnBatchBuild(wxCommandEvent& WXUNUSED(event)) {
	if (OutfitIsEmpty())
		return;

	bool custpath = false;
	bool clean = false;
	if (wxGetKeyState(WXK_CONTROL))
		custpath = true;
	else if (wxGetKeyState(WXK_ALT))
		clean = true;

	if (app->clippingFixStrength > 0.0f) {
		int answer = wxMessageBox(
			_("Fix Clipping is enabled for this batch build.\n\n"
			  "Use this carefully: applying clipping fixes to many outfits at once can create unwelcome side effects on some meshes.\n\n"
			  "Consider building outfits one-by-one and checking each result in Preview.\n\n"
			  "Do you want to continue with batch build?"),
			_("Warning"),
			wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
			this);

		if (answer != wxYES)
			return;
	}

	wxArrayString oChoices;
	std::vector<std::string> outfitChoices;
	std::vector<std::string> toBuild;

	bool tri = false;
	bool forceNormals = false;

	auto cbMorphs = (wxCheckBox*)FindWindowByName("cbMorphs");
	if (cbMorphs)
		tri = cbMorphs->IsChecked();

	auto cbForceBodyNormals = (wxCheckBox*)FindWindowByName("cbForceBodyNormals");
	if (cbForceBodyNormals)
		forceNormals = cbForceBodyNormals->IsChecked();

	app->GetFilteredOutfits(outfitChoices);

	std::map<std::string, uint32_t> outfitIndices;

	uint32_t idx = 0;
	for (auto& o : outfitChoices) {
		oChoices.Add(wxString::FromUTF8(o));
		outfitIndices[o] = idx;
		idx++;
	}

	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* batchBuildChooser = rsrc->LoadDialog(this, "dlgBatchBuild");
	if (!batchBuildChooser)
		return;

	batchBuildChooser->SetSize(batchBuildChooser->FromDIP(wxSize(800, 450)));
	batchBuildChooser->SetSizeHints(batchBuildChooser->FromDIP(wxSize(600, 300)));
	batchBuildChooser->CenterOnParent();

	// Load BuildSelection file
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	app->GetBuildSelection(buildSelFile, buildSelection);

	// The first column only holds the check boxes
	enum { ColCheck, ColOutfit, ColBuilt, ColModified, ColPreset, ColBuiltInstead };
	batchBuildList = XRCCTRL((*batchBuildChooser), "batchBuildList", wxListCtrl);
	batchBuildList->EnableCheckBoxes();
	batchBuildList->AppendColumn(wxEmptyString);
	batchBuildList->AppendColumn(_("Outfit"));
	batchBuildList->AppendColumn(_("Built"), wxLIST_FORMAT_CENTRE);
	batchBuildList->AppendColumn(_("Modified"), wxLIST_FORMAT_CENTRE);
	batchBuildList->AppendColumn(_("Preset"));
	batchBuildList->AppendColumn(_("Built Instead"));
	batchBuildList->Bind(wxEVT_CONTEXT_MENU, &BodySlideFrame::OnBatchBuildContext, this);
	batchBuildList->Bind(wxEVT_KEY_DOWN, &BodySlideFrame::OnBatchBuildKey, this);
	batchBuildList->Bind(wxEVT_CHAR, &BodySlideFrame::OnBatchBuildKey, this);

	for (uint32_t i = 0; i < oChoices.size(); i++) {
		batchBuildList->InsertItem(i, wxEmptyString);
		batchBuildList->SetItem(i, ColOutfit, oChoices[i]);
	}

	// Show what was last built to the output of each outfit
	const wxString checkMark(wxUniChar(0x2713));
	std::string dataFolder = app->GetOutputDataPath();
	std::string activePreset = BodySlideConfig["SelectedPreset"];
	batchBuildStates.assign(outfitChoices.size(), BatchBuildState::NotBuilt);
	if (!dataFolder.empty()) {
		for (auto& outFile : app->outFileCount) {
			const BuildRecord* record = buildSelection.GetBuildRecord(outFile.first, dataFolder);
			if (!record)
				continue;

			for (auto& outfit : outFile.second) {
				auto outfitIndex = outfitIndices.find(outfit);
				if (outfitIndex == outfitIndices.end())
					continue;

				uint32_t i = outfitIndex->second;
				if (record->set != outfit) {
					batchBuildStates[i] = BatchBuildState::OtherOutfitBuilt;
					batchBuildList->SetItem(i, ColBuiltInstead, wxString::FromUTF8(record->set));
					continue;
				}

				batchBuildStates[i] = record->preset == activePreset ? BatchBuildState::BuiltWithPreset : BatchBuildState::BuiltWithOtherPreset;
				batchBuildList->SetItem(i, ColBuilt, checkMark);
				batchBuildList->SetItem(i, ColPreset, wxString::FromUTF8(record->preset));
				if (record->IsModified())
					batchBuildList->SetItem(i, ColModified, checkMark);
			}
		}
	}

	for (uint32_t i = 0; i < oChoices.size(); i++)
		batchBuildList->CheckItem(i, true);

	for (auto& outFile : app->outFileCount) {
		if (outFile.second.size() > 1) {
			std::vector<std::string> outfitsInBuild;
			for (auto& outfit : outFile.second) {
				// Only if it's going to be batch built
				if (std::find(outfitChoices.begin(), outfitChoices.end(), outfit) != outfitChoices.end())
					outfitsInBuild.push_back(outfit);
			}

			// Same file would not be written more than once
			if (outfitsInBuild.size() <= 1)
				continue;

			// The saved choice, otherwise the outfit last built to the output
			std::string outputChoice = app->GetDefaultOutputChoice(buildSelection, outFile.first, dataFolder);
			if (!outputChoice.empty()) {
				for (auto& outfit : outfitsInBuild) {
					if (outfit != outputChoice) {
						// Uncheck choice by default
						batchBuildList->CheckItem(outfitIndices[outfit], false);
					}
				}
			}
		}
	}

	batchBuildDefaults.resize(oChoices.size());
	for (uint32_t i = 0; i < oChoices.size(); i++)
		batchBuildDefaults[i] = batchBuildList->IsItemChecked(i);

	// Columns as wide as their header or content, whichever is wider
	batchBuildList->SetColumnWidth(ColCheck, batchBuildList->FromDIP(28));
	for (int col = ColOutfit; col <= ColBuiltInstead; col++) {
		batchBuildList->SetColumnWidth(col, wxLIST_AUTOSIZE);
		int contentWidth = batchBuildList->GetColumnWidth(col);
		batchBuildList->SetColumnWidth(col, wxLIST_AUTOSIZE_USEHEADER);
		if (contentWidth > batchBuildList->GetColumnWidth(col))
			batchBuildList->SetColumnWidth(col, contentWidth);
	}

	if (batchBuildList->GetItemCount() > 0)
		batchBuildList->SetItemState(0, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

	int batchBuildResult = batchBuildChooser->ShowModal();
	batchBuildDefaults.clear();
	batchBuildStates.clear();

	if (batchBuildResult == wxID_OK) {
		toBuild.clear();
		for (int i = 0; i < batchBuildList->GetItemCount(); i++)
			if (batchBuildList->IsItemChecked(i))
				toBuild.push_back(outfitChoices[i]);

		batchBuildList = nullptr;
		delete batchBuildChooser;
	}
	else {
		batchBuildList = nullptr;
		delete batchBuildChooser;
		return;
	}

	std::map<std::string, std::string> failedOutfits;
	int ret;
	if (custpath) {
		std::string path = wxDirSelector(_("Choose a folder to contain the saved files")).ToStdString();
		if (path.empty())
			return;

		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, forceNormals, path + PathSepStr);
	}
	else if (clean)
		ret = app->BuildListBodies(toBuild, failedOutfits, true, tri, forceNormals);
	else
		ret = app->BuildListBodies(toBuild, failedOutfits, false, tri, forceNormals);

	wxLog::FlushActive();

	if (ret == 0) {
		wxLogMessage("All sets processed successfully!");
		wxMessageBox(_("All sets processed successfully!"), _("Complete"), wxICON_INFORMATION);
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto& e : failedOutfits) {
			wxString ename = wxString::FromUTF8(e.first);
			wxLogError("Failed to build '%s': %s", ename, e.second);
			errlist.Add(ename + ":" + e.second);
		}

		wxSingleChoiceDialog errdisplay(this, _("The following sets failed"), _("Failed"), errlist, (void**)0, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
		errdisplay.ShowModal();
	}
}

void BodySlideFrame::OnBatchBuildContext(wxContextMenuEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("batchBuildContext");
	if (menu) {
		menu->Bind(wxEVT_MENU, &BodySlideFrame::OnBatchBuildSelect, this);
		batchBuildList->PopupMenu(menu);
		delete menu;
	}
}

void BodySlideFrame::OnBatchBuildKey(wxKeyEvent& event) {
	if (event.GetKeyCode() != WXK_SPACE || event.HasAnyModifiers()) {
		event.Skip();
		return;
	}

	// The list only toggles the focused row itself, so space is handled here for both key down and char
	if (event.GetEventType() != wxEVT_KEY_DOWN)
		return;

	// All selected rows take the opposite state of the focused one
	long focused = batchBuildList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
	long first = batchBuildList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (focused < 0 || batchBuildList->GetItemState(focused, wxLIST_STATE_SELECTED) == 0)
		focused = first;
	if (focused < 0)
		return;

	bool check = !batchBuildList->IsItemChecked(focused);
	for (long i = first; i >= 0; i = batchBuildList->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
		batchBuildList->CheckItem(i, check);
}

void BodySlideFrame::OnBatchBuildSelect(wxCommandEvent& event) {
	int count = batchBuildList->GetItemCount();
	if (event.GetId() == XRCID("batchBuildNone")) {
		for (int i = 0; i < count; i++)
			batchBuildList->CheckItem(i, false);
	}
	else if (event.GetId() == XRCID("batchBuildAll")) {
		for (int i = 0; i < count; i++)
			batchBuildList->CheckItem(i, true);
	}
	else if (event.GetId() == XRCID("batchBuildInvert")) {
		for (int i = 0; i < count; i++)
			batchBuildList->CheckItem(i, !batchBuildList->IsItemChecked(i));
	}
	else if (event.GetId() == XRCID("batchBuildBuilt")) {
		// What's in the output folder now, to rebuild it
		for (int i = 0; i < count && i < (int)batchBuildStates.size(); i++) {
			auto state = batchBuildStates[i];
			batchBuildList->CheckItem(i, state == BatchBuildState::BuiltWithPreset || state == BatchBuildState::BuiltWithOtherPreset);
		}
	}
	else if (event.GetId() == XRCID("batchBuildNotBuilt")) {
		// What a batch build would build by default, without the outputs already built with this preset
		for (int i = 0; i < count && i < (int)batchBuildStates.size() && i < (int)batchBuildDefaults.size(); i++)
			batchBuildList->CheckItem(i, batchBuildDefaults[i] && batchBuildStates[i] != BatchBuildState::BuiltWithPreset);
	}
}
