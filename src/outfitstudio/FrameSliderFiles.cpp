/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: slider presets and importing and exporting slider data

#include "OutfitStudioFrame.h"
#include "OutfitStudioApp.h"
#include "ShapeTreeItems.h"
#include "utils/ConfigurationManager.h"
#include "components/SliderGroup.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "files/TriFile.h"
#include "files/SFMorphFile.h"
#include "ui/PresetSaveDialog.h"
#include "ui/outfitstudio/SliderDataImportDialog.h"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <sstream>

using namespace nifly;

void OutfitStudioFrame::OnLoadPreset(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	PresetCollection presets;
	std::vector<std::string> names;
	wxChoice* presetChoice;

	std::string choice;
	bool hi = true;

	CloseBrushSettings();

	presets.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", choice, names, true);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto& n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		// Without generated weights only the high weight values are used, as in BodySlide
		if (!project->mGenWeights) {
			XRCCTRL(dlg, "weightHi", wxRadioButton)->SetValue(true);
			XRCCTRL(dlg, "weightLo", wxRadioButton)->Enable(false);
		}

		dlg.SetSize(dlg.FromDIP(wxSize(325, 175)));
		dlg.SetSizeHints(dlg.FromDIP(wxSize(325, 175)), wxSize(-1, -1));
		dlg.CenterOnParent();

		if (dlg.ShowModal() != wxID_OK)
			return;

		choice = presetChoice->GetStringSelection();
		if (XRCCTRL(dlg, "weightLo", wxRadioButton)->GetValue())
			hi = false;

		if (choice == "Zero All") {
			ZeroPresetSliders();
			if (presetOff)
				presetOff->SetValue(true);
			wxLogMessage("Sliders were reset to zero.");
			return;
		}

		ApplyPresetValues(presets, choice, hi);

		// The dropdown's presets may be older than the ones the dialog just loaded
		RefreshQuickPresets();
		SyncQuickPreset(choice, hi, true);
	}
}

void OutfitStudioFrame::ApplyPresetValues(PresetCollection& presets, const std::string& presetName, bool hi) {
	wxLogMessage("Applying preset '%s' with option '%s'.", presetName, hi ? "High" : "Low");

	for (auto& value : ProjectOperations(*project, editor).GetPresetValues(presets, presetName, hi)) {
		// The slider in edit mode keeps its value
		if (editor.bEditSlider && value.first == editor.activeSlider)
			continue;

		SetSliderValue(value.first, value.second * 100.0f);
	}

	ApplySliders();
}

void OutfitStudioFrame::ZeroPresetSliders() {
	if (!editor.bEditSlider) {
		ZeroSliders();
		return;
	}

	// ProjectOperations::ZeroSliders would zero the slider in edit mode as well
	for (size_t i = 0; i < project->SliderCount(); i++) {
		if (project->SliderClamp(i))
			continue;

		if (project->GetSliderName(i) == editor.activeSlider)
			continue;

		SetSliderValue(i, 0);
	}

	ApplySliders();
}

void OutfitStudioFrame::RefreshQuickPresets() {
	if (!presetQuick)
		return;

	std::string selected;
	if (presetQuick->GetSelection() != wxNOT_FOUND)
		selected = presetQuick->GetString(presetQuick->GetSelection()).ToUTF8();

	quickPresets.Clear();

	std::vector<std::string> names;
	quickPresets.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", "", names, true);
	quickPresets.GetPresetNames(names);
	std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) {
		return wxString::FromUTF8(a).CmpNoCase(wxString::FromUTF8(b)) < 0;
	});

	presetQuick->Clear();

	int selection = wxNOT_FOUND;
	for (auto& n : names) {
		int item = presetQuick->AppendItem(wxString::FromUTF8(n));
		if (n == selected)
			selection = item;
	}

	presetQuick->SetSelection(selection);
}

void OutfitStudioFrame::SyncQuickPreset(const std::string& presetName, bool hi, bool on) {
	if (!presetQuick)
		return;

	int item = presetQuick->FindString(wxString::FromUTF8(presetName), true);
	presetQuick->SetSelection(item);

	// Without generated weights there's no weight to choose, keep the one chosen for projects that have them
	if (project->mGenWeights)
		presetLastHi = hi;
	SetQuickPresetOn(on);
}

bool OutfitStudioFrame::UpdateQuickPresetWeightVisibility() {
	if (!presetWeightLo || !presetWeightHi)
		return false;

	// Without generated weights there's only one set of values, the high weight one, as in BodySlide
	const bool genWeights = project && project->mGenWeights;
	if (presetWeightLo->IsShown() == genWeights)
		return false;

	bool lowToHigh = false;
	if (!genWeights && presetWeightLo->GetValue()) {
		presetWeightHi->SetValue(true);
		lowToHigh = true;
	}

	presetWeightLo->Show(genWeights);
	presetWeightHi->SetLabel(genWeights ? _("High") : _("Preset"));
	presetWeightHi->SetToolTip(genWeights ? _("Preview the high weight values of the preset.") : _("Preview the preset."));
	presetWeightLo->GetParent()->Layout();
	return lowToHigh;
}

bool OutfitStudioFrame::IsQuickPresetOn() const {
	return !presetOff || !presetOff->GetValue();
}

void OutfitStudioFrame::SetQuickPresetOn(bool on) {
	if (!presetWeightLo || !presetWeightHi || !presetOff)
		return;

	if (!on)
		presetOff->SetValue(true);
	else if (presetLastHi || !presetWeightLo->IsShown())
		presetWeightHi->SetValue(true);
	else
		presetWeightLo->SetValue(true);
}

void OutfitStudioFrame::ApplyQuickPreset() {
	if (!presetQuick || presetQuick->GetSelection() == wxNOT_FOUND)
		return;

	if (!IsQuickPresetOn()) {
		ZeroPresetSliders();
		return;
	}

	// Without generated weights only the high weight values are used
	bool hi = !project->mGenWeights || !presetWeightLo || !presetWeightLo->GetValue();
	std::string presetName{presetQuick->GetString(presetQuick->GetSelection()).ToUTF8()};
	ApplyPresetValues(quickPresets, presetName, hi);
}

void OutfitStudioFrame::OnQuickPreset(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	// Picking a preset shows it, with the weight it had last
	SetQuickPresetOn(true);
	ApplyQuickPreset();
}

void OutfitStudioFrame::OnQuickPresetDropdown(wxCommandEvent& event) {
	// Presets saved in BodySlide or elsewhere in the meantime show up as well
	RefreshQuickPresets();
	event.Skip();
}

void OutfitStudioFrame::OnQuickPresetState(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	// Turning the preset back on (Ctrl+P) uses the weight that was chosen last
	if (presetWeightLo && presetWeightLo->GetValue())
		presetLastHi = false;
	else if (presetWeightHi && presetWeightHi->GetValue())
		presetLastHi = true;

	ApplyQuickPreset();
}

void OutfitStudioFrame::OnQuickPresetToggleAccel(wxCommandEvent& WXUNUSED(event)) {
	if (!presetQuick || presetQuick->GetSelection() == wxNOT_FOUND) {
		wxLogMessage("No preset is selected to turn on or off.");
		statusBar->SetStatusText(_("Select a preset above the sliders first to turn it on or off with Ctrl+P."));
		return;
	}

	CloseBrushSettings();
	SetQuickPresetOn(!IsQuickPresetOn());
	ApplyQuickPreset();
}

void OutfitStudioFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> sliders;
	project->GetSliderList(sliders);
	if (sliders.empty()) {
		wxMessageBox(_("There are no sliders loaded!"), _("Error"), wxICON_ERROR, this);
		return;
	}

	CloseBrushSettings();

	SliderSetGroupCollection groupCollection;
	groupCollection.LoadGroups(ProjectUtil::GetProjectPath() + "/SliderGroups");

	std::set<std::string> allGroups;
	groupCollection.GetAllGroups(allGroups);

	PresetSaveDialog psd(this);
	psd.allGroupNames.assign(allGroups.begin(), allGroups.end());
	psd.FilterGroups();

	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	std::string fileName = psd.outFileName;
	std::string presetName = psd.outPresetName;

	std::vector<std::string> groups;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	bool addedSlider = false;
	PresetCollection presets;
	for (auto& s : sliders) {
		size_t index = 0;
		if (!project->SliderIndexFromName(s, index))
			continue;

		if (project->SliderZap(index) || project->SliderHidden(index))
			continue;

		float value = project->SliderValue(index);
		if (project->SliderInvert(index))
			value = 1.0f - value;

		if (project->SliderDefault(index, true) == value)
			continue;

		presets.SetSliderPreset(presetName, s, value, -10000.0f);

		if (!addedSlider)
			addedSlider = true;
	}

	if (!addedSlider) {
		wxLogMessage("No changes were made to the sliders, so no preset was saved!");
		wxMessageBox(_("No changes were made to the sliders, so no preset was saved!"), _("Info"), wxICON_INFORMATION, this);
		return;
	}

	int error = presets.SavePreset(fileName, presetName, "OutfitStudioFrame", groups);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to save preset (%d)!"), error), _("Error"), wxICON_ERROR, this);
		return;
	}

	RefreshQuickPresets();
}

void OutfitStudioFrame::OnSliderImportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .nif file for slider calculation"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), editor.activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from NIF file '%s'...", editor.activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromNIF(editor.activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("No mesh found in the .nif file that matches currently selected shape!");
		wxMessageBox(_("No mesh found in the .nif file that matches currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), editor.activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", editor.activeSlider, activeItem->GetShape()->name.get(), fn);
	project->SetSliderFromBSD(editor.activeSlider, activeItem->GetShape(), fn.ToUTF8().data());

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .obj file for slider calculation"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), editor.activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", editor.activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromOBJ(editor.activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing morphs from OSD file '%s'...", fn);

	OSDataFile osd;
	if (!osd.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import OSD file '%s'!", fn);
		wxMessageBox(_("Failed to import OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;
	auto& diffs = osd.GetDataDiffs();
	const auto& shapes = project->GetWorkNif()->GetShapes();
	for (auto& diff : diffs) {
		std::string bestTargetName;
		for (auto& shape : shapes) {
			std::string shapeName = shape->name.get();
			std::string targetName = project->ShapeToTarget(shapeName);

			// Diff name is supposed to begin with matching shape name
			if (diff.first.substr(0, targetName.size()) != targetName)
				continue;

			if (shapeName.length() > bestTargetName.length())
				bestTargetName = targetName;
		}

		if (bestTargetName.length() == 0)
			continue;

		// Find slider name from data name
		auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diff.first);
		if (sliderName.empty())
			sliderName = diff.first.substr(bestTargetName.length(), diff.first.length() - bestTargetName.length() + 1);

		shapeToSliders[bestTargetName].emplace(sliderName, diff.first);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("OSD Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		editor.activeSlider.clear();
		editor.lastActiveSlider.clear();
	}

	std::unordered_set<NiShape*> addedShapes;

	for (auto& shape : shapes) {
		// check if the shape is selected
		auto selectedSliders = options.selectedShapesToSliders.find(shape->name.get());
		if (selectedSliders == options.selectedShapesToSliders.end())
			continue;

		addedShapes.emplace(shape);

		for (auto& diff : diffs) {
			auto& sliderNameToDisplayName = selectedSliders->second;

			// check the diff is selected for the specific shape
			auto sliderName = selectedSliders->second.find(diff.first);
			if (sliderName == sliderNameToDisplayName.end())
				continue;

			if (!project->ValidSlider(sliderName->second)) {
				project->AddEmptySlider(sliderName->second);
				createSliderGUI(sliderName->second, sliderScroll, sliderScroll->GetSizer());
			}

			if (!ConfirmSliderDataLocalForEdit(shape, sliderName->second))
				continue;

			project->SetSliderFromDiff(sliderName->second, shape, *diff.second);
		}
	}

	wxString addedDiffs;
	for (auto& addedShape : addedShapes) {
		addedDiffs += addedShape->name.get() + "\n";
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedDiffs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedDiffs), _("OSD Import"));
}

void OutfitStudioFrame::OnSliderImportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing morphs from TRI file '%s'...", fn);

	TriFile tri;
	if (!tri.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to load TRI file '%s'!", fn);
		wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;
	auto morphs = tri.GetMorphs();
	for (auto& morph : morphs) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(morph.first);
		if (!shape)
			continue;

		for (auto& morphData : morph.second)
			shapeToSliders[shape->name.get()].emplace(morphData->name, morphData->name);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("TRI Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		editor.activeSlider.clear();
		editor.lastActiveSlider.clear();
	}

	wxString addedMorphs;
	std::unordered_set<NiShape*> addedShapes;

	for (auto& morph : morphs) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(morph.first);
		if (!shape)
			continue;

		// check if the shape is selected
		auto selectedSliders = options.selectedShapesToSliders.find(shape->name.get());
		if (selectedSliders == options.selectedShapesToSliders.end())
			continue;

		addedMorphs += morph.first + "\n";
		for (auto& morphData : morph.second) {
			// check the morph is selected for the specific shape
			if (selectedSliders->second.find(morphData->name) == selectedSliders->second.end())
				continue;

			if (!project->ValidSlider(morphData->name)) {
				project->AddEmptySlider(morphData->name);
				createSliderGUI(morphData->name, sliderScroll, sliderScroll->GetSizer());
			}

			std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			if (!ConfirmSliderDataLocalForEdit(shape, morphData->name))
				continue;

			project->SetSliderFromDiff(morphData->name, shape, diff);

			if (morphData->type == MORPHTYPE_UV) {
				size_t sliderIndex = 0;
				if (!project->SliderIndexFromName(morphData->name, sliderIndex))
					continue;

				project->SetSliderUV(sliderIndex, true);
			}
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedMorphs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedMorphs), _("TRI Import"));
}

void OutfitStudioFrame::OnSliderImportMorphsSF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Import Starfield morph.dat file"), wxEmptyString, wxEmptyString, ".dat", "*.dat", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing morphs from Starfield morph file '%s'...", fn);

	SFMorphFile morphFile;
	if (!morphFile.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import Starfield morph file '%s'!", fn);
		wxMessageBox(_("Failed to import Starfield morph file!"), _("Error"), wxICON_ERROR);
		return;
	}

	if (!morphFile.FileToCacheData()) {
		wxLogError("Failed to import Starfield morph file '%s'!", fn);
		wxMessageBox(_("Failed to import Starfield morph file!"), _("Error"), wxICON_ERROR);
		return;
	}

	morphFile.UpdateCachedMorphData();
	
	auto shape = activeItem->GetShape();

	std::string shapeName = shape->name.get();
	std::string targetName = project->ShapeToTarget(shapeName);

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;

	for (auto& morphName : morphFile.GetMorphNames()) {
		// Find slider name from morph name
		auto sliderName = project->activeSet.SliderFromDataName(shapeName, morphName);
		if (sliderName.empty())
			sliderName = morphName;

		shapeToSliders[shapeName].emplace(sliderName, morphName);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("Starfield Morph Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		editor.activeSlider.clear();
		editor.lastActiveSlider.clear();
	}

	// Check if the shape is selected
	auto selectedSliders = options.selectedShapesToSliders.find(shapeName);
	if (selectedSliders == options.selectedShapesToSliders.end())
		return;

	for (auto& morphName : morphFile.GetMorphNames()) {
		auto& sliderNameToDisplayName = selectedSliders->second;

		// check the diff is selected for the specific shape
		auto sliderName = selectedSliders->second.find(morphName);
		if (sliderName == sliderNameToDisplayName.end())
			continue;

		if (!project->ValidSlider(sliderName->second)) {
			project->AddEmptySlider(sliderName->second);
			createSliderGUI(sliderName->second, sliderScroll, sliderScroll->GetSizer());
		}

		auto& morphIndex = morphFile.morphNamesCacheMap[morphName];
		auto& morphOffsets = morphFile.morphOffsetsCache[morphIndex];
		if (!ConfirmSliderDataLocalForEdit(shape, sliderName->second))
			continue;

		project->SetSliderFromDiff(sliderName->second, shape, morphOffsets);
	}

	wxString addedDiffs = shapeName + "\n";

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedDiffs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedDiffs), _("Starfield Morph Import"));
}

void OutfitStudioFrame::OnSliderImportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .fbx file for slider calculation"), wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), editor.activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", editor.activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromFBX(editor.activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

// One format the data of the slider in edit mode exports to
struct SliderDataExportFormat {
	const char* name;		  // For the log, like NIF
	const char* extension;	  // Without the dot
	wxString directoryTitle;  // For several shapes, which export into a folder
	wxString fileTitle;		  // For one shape
	wxString failedMessage;	  // Shown if exporting one shape failed, empty to not check
	std::function<int(NiShape*, const std::string&)> save;
};

void OutfitStudioFrame::ExportActiveSliderData(const SliderDataExportFormat& format) {
	if (!ShapeSelectionCheck())
		return;

	if (!editor.bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(format.directoryTitle, wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto& i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + i->GetShape()->name.get() + "_" + editor.activeSlider + "." + format.extension;
			wxLogMessage("Exporting %s slider data of '%s' for shape '%s' to '%s'...", format.name, editor.activeSlider, i->GetShape()->name.get(), targetFile);
			format.save(i->GetShape(), targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(format.fileTitle, wxEmptyString, wxEmptyString, wxString(".") + format.extension, wxString("*.") + format.extension, wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting %s slider data of '%s' for shape '%s' to '%s'...", format.name, editor.activeSlider, activeItem->GetShape()->name.get(), fn);
		if (format.save(activeItem->GetShape(), fn.ToUTF8().data()) && !format.failedMessage.IsEmpty()) {
			wxLogError("Failed to export %s file '%s'!", format.name, fn);
			wxMessageBox(format.failedMessage, _("Error"), wxICON_ERROR);
		}
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportNIF(wxCommandEvent& WXUNUSED(event)) {
	ExportActiveSliderData({"NIF",
							"nif",
							_("Export .nif slider data to directory"),
							_("Export .nif slider data"),
							_("Failed to export NIF file!"),
							[this](NiShape* shape, const std::string& file) { return project->SaveSliderNIF(editor.activeSlider, shape, file); }});
}

void OutfitStudioFrame::OnSliderExportBSD(wxCommandEvent& WXUNUSED(event)) {
	ExportActiveSliderData({"BSD",
							"bsd",
							_("Export .bsd slider data to directory"),
							_("Export .bsd slider data"),
							wxEmptyString,
							[this](NiShape* shape, const std::string& file) { return project->SaveSliderBSD(editor.activeSlider, shape, file); }});
}

void OutfitStudioFrame::OnSliderExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	ExportActiveSliderData({"OBJ",
							"obj",
							_("Export .obj slider data to directory"),
							_("Export .obj slider data"),
							_("Failed to export OBJ file!"),
							[this](NiShape* shape, const std::string& file) { return project->SaveSliderOBJ(editor.activeSlider, shape, file); }});
}

void OutfitStudioFrame::OnSliderExportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Export .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting OSD file to '%s'...", fn);
	if (!project->SaveSliderData(fn.ToUTF8().data())) {
		wxLogError("Failed to export OSD file to '%s'!", fn);
		wxMessageBox(_("Failed to export OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnSliderExportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Export .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting TRI morphs to '%s'...", fn);
	if (!project->WriteMorphTRI(fn.ToUTF8().data())) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnSliderExportMorphsSF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Export Starfield morph.dat"), wxEmptyString, wxEmptyString, ".dat", "*.dat", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting Starfield morph.dat to '%s'...", fn);
	if (!project->WriteSFMorphs(activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Failed to export Starfield morph.dat file to '%s'!", fn);
		wxMessageBox(_("Failed to export Starfield morph.dat file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnSliderExportToOBJs(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString dir = wxDirSelector(_("Export .obj slider data to directory"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
	if (dir.IsEmpty())
		return;

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);

	wxLogMessage("Exporting sliders to OBJ files in '%s'...", dir);
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		for (auto& slider : sliderList) {
			if (!project->SliderShow(slider))
				continue;

			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + shape->name.get() + "_" + slider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", slider, shape->name.get(), targetFile);
			if (project->SaveSliderOBJ(slider, shape, targetFile, true))
				wxLogError("Failed to export OBJ file '%s'!", targetFile);
		}
	}
}
