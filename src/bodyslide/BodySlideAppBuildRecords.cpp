/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: build selections, conflicting outputs and the records of past builds

#include "BodySlideApp.h"
#include "BodySlideFrame.h"

#include <algorithm>
#include <set>
#include <utility>
#include <wx/wx.h>

using namespace nifly;

void BodySlideApp::GetBuildSelection(BuildSelectionFile& file, BuildSelection& buildSel) {
	const std::string buildSelFileName = Config["AppDir"] + PathSepStr + "BuildSelection.xml";

	file.Open(buildSelFileName);

	if (file.GetError())
		file.New(buildSelFileName);

	file.Get(buildSel);
}

std::string BodySlideApp::GetDefaultOutputChoice(BuildSelection& buildSelection, const std::string& outputPath, const std::string& dataFolder) {
	auto outFile = outFileCount.find(outputPath);
	if (outFile == outFileCount.end())
		return {};

	auto writesOutput = [&](const std::string& outfit) {
		return !outfit.empty() && std::find(outFile->second.cbegin(), outFile->second.cend(), outfit) != outFile->second.cend();
	};

	// Ignore saved choices and builds of outfits that no longer exist
	std::string choice = buildSelection.GetOutputChoice(outputPath);
	if (writesOutput(choice))
		return choice;

	const BuildRecord* record = buildSelection.GetBuildRecord(outputPath, dataFolder);
	if (record && writesOutput(record->set))
		return record->set;

	return {};
}

// Local time with its offset to UTC, e.g. 2026-11-16T21:15:50+01:00
static std::string BuildRecordTimeNow() {
	wxDateTime now = wxDateTime::Now();
	wxString offset = now.Format("%z");
	if (offset.length() == 5)
		offset.insert(3, ":");

	return (now.Format("%Y-%m-%dT%H:%M:%S") + offset).ToUTF8().data();
}

// Changed sliders and zaps of a build record for a tooltip, shortened when there are many
static wxString BuildRecordChanges(const BuildRecord& record) {
	const size_t maxShown = 12;
	wxString changes;

	if (!record.sliders.empty()) {
		// Low and high weight values of a slider on one line
		std::vector<std::string> names;
		std::map<std::string, std::pair<wxString, wxString>> values;
		for (auto& slider : record.sliders) {
			auto& value = values[slider.name];
			if (value.first.empty() && value.second.empty())
				names.push_back(slider.name);

			(slider.isSmall ? value.first : value.second) = wxString::Format("%d%%", slider.value);
		}

		changes += "\n" + _("Changed sliders:");
		for (size_t i = 0; i < names.size() && i < maxShown; i++) {
			auto& value = values[names[i]];
			wxString line = "\n  " + wxString::FromUTF8(names[i]) + ":";
			if (!value.first.empty())
				line += " " + wxString::Format(_("low %s"), value.first);
			if (!value.second.empty())
				line += " " + wxString::Format(_("high %s"), value.second);
			changes += line;
		}

		if (names.size() > maxShown)
			changes += "\n  " + wxString::Format(_("and %d more"), (int)(names.size() - maxShown));
	}

	if (!record.zaps.empty()) {
		changes += "\n" + _("Changed zaps:");
		for (size_t i = 0; i < record.zaps.size() && i < maxShown; i++) {
			auto& zap = record.zaps[i];
			changes += "\n  " + wxString::FromUTF8(zap.name) + ": " + (zap.zapped ? _("on") : _("off"));
		}

		if (record.zaps.size() > maxShown)
			changes += "\n  " + wxString::Format(_("and %d more"), (int)(record.zaps.size() - maxShown));
	}

	return changes;
}

BuildRecord BodySlideApp::ActiveSetBuildRecord() {
	auto& activeSet = GetActiveSet();
	std::string preset = BodySlideConfig["SelectedPreset"];

	BuildRecord record;
	record.path = activeSet.GetOutputFilePath();
	record.set = activeSet.GetName();

	// Zaps that aren't in their default state flip the defaults of the zaps they toggle, like batch builds do
	std::unordered_set<std::string> flippedDefaults;
	for (auto& slider : sliderManager.slidersBig) {
		if (!slider.zap || slider.uv || (slider.value > 0.0f) == (slider.defValue > 0.0f))
			continue;

		for (auto& zapToggle : slider.zapToggles)
			if (!flippedDefaults.insert(zapToggle).second)
				flippedDefaults.erase(zapToggle);
	}

	auto defaultValue = [&](const Slider& slider) {
		return flippedDefaults.count(slider.name) ? 1.0f - slider.defValue : slider.defValue;
	};

	// The values set in the UI, before toggles hold other sliders
	for (auto& slider : sliderManager.slidersBig)
		if (!slider.clamp)
			record.AddPresetChange(slider.name, false, slider.zap, slider.value, sliderManager.GetBigPresetValue(preset, slider.name, defaultValue(slider)));

	if (activeSet.GenWeights())
		for (auto& slider : sliderManager.slidersSmall)
			if (!slider.clamp && !slider.zap)
				record.AddPresetChange(slider.name, true, false, slider.value, sliderManager.GetSmallPresetValue(preset, slider.name, defaultValue(slider)));

	return record;
}

void BodySlideApp::RecordBuilds(std::vector<BuildRecord>& builtRecords, const std::vector<std::string>& removedOutputs, const std::string& dataFolder) {
	if (builtRecords.empty() && removedOutputs.empty())
		return;

	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	std::string preset = BodySlideConfig["SelectedPreset"];
	std::string time = BuildRecordTimeNow();

	for (auto& record : builtRecords) {
		record.dataFolder = dataFolder;
		record.preset = preset;
		record.time = time;
	}

	buildSelFile.SetBuildRecords(builtRecords);
	buildSelFile.RemoveBuildRecords(removedOutputs, dataFolder);

	if (!buildSelFile.Save())
		wxLogWarning("Failed to save the build records to '%s'.", buildSelFile.fileName);

	if (!projects.empty())
		UpdateBuildRecordLabel();
}

void BodySlideApp::UpdateBuildRecordLabel() {
	if (projects.empty() || !sliderView)
		return;

	auto buildInfo = (wxStaticText*)sliderView->FindWindowByName("buildInfo");
	if (!buildInfo)
		return;

	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	auto& activeSet = GetActiveSet();
	std::string dataFolder = GetOutputDataPath();
	const BuildRecord* record = dataFolder.empty() ? nullptr : buildSelection.GetBuildRecord(activeSet.GetOutputFilePath(), dataFolder);

	if (!record) {
		buildInfo->SetLabel(_("Not built"));
		buildInfo->SetToolTip(_("No build of this output to the output folder was recorded."));
		buildInfo->SetForegroundColour(wxColour("#C8C8C8"));
	}
	else {
		wxString set = wxString::FromUTF8(record->set);
		wxString preset = wxString::FromUTF8(record->preset);

		if (record->set != activeSet.GetName()) {
			buildInfo->SetLabel(wxString::Format(_("Built from '%s'"), set));
			buildInfo->SetForegroundColour(wxColour("#FFD769"));
		}
		else if (record->preset != BodySlideConfig["SelectedPreset"]) {
			if (record->IsModified())
				buildInfo->SetLabel(wxString::Format(_("Built with '%s' (modified)"), preset));
			else
				buildInfo->SetLabel(wxString::Format(_("Built with '%s'"), preset));
			buildInfo->SetForegroundColour(wxColour("#FFD769"));
		}
		else {
			if (record->IsModified())
				buildInfo->SetLabel(_("Built with this preset (modified)"));
			else
				buildInfo->SetLabel(_("Built with this preset"));
			buildInfo->SetForegroundColour(wxColour("#00FFFF"));
		}

		wxString time = wxString::FromUTF8(record->time);
		time.Replace("T", " ");

		wxString tip = wxString::Format(_("Last build of this output:\nOutfit: %s\nPreset: %s\nTime: %s\nFolder: %s"),
										set,
										preset,
										time,
										wxString::FromUTF8(record->dataFolder));
		tip += BuildRecordChanges(*record);
		buildInfo->SetToolTip(tip);
	}

	buildInfo->Show();

	if (sliderView->leftPanel)
		sliderView->leftPanel->Layout();
}

void BodySlideApp::UpdateConflictManager() {
	if (projects.empty())
		return;

	// Populate Conflict UI
	auto& activeSet = GetActiveSet();
	auto conflictCheckBox = (wxCheckBox*)sliderView->FindWindowByName("cbIsOutfitChoice");
	auto conflictLabel = (wxStaticText*)sliderView->FindWindowByName("conflictLabel");
	auto conflictInfo = (wxStaticText*)sliderView->FindWindowByName("conflictInfo");

	auto outputFilePath = activeSet.GetOutputFilePath();
	auto& col = outFileCount[outputFilePath];

	bool isOutputChoice = true;
	std::string textColourName = "#C8C8C8";

	if (1 < col.size()) {
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		std::string outputChoice = buildSelection.GetOutputChoice(outputFilePath);
		isOutputChoice = outputChoice == activeSet.GetName();
		textColourName = isOutputChoice ? "#00FFFF" : "#FFD769";
	}

	conflictCheckBox->SetValue(isOutputChoice);
	conflictCheckBox->Show();

	if (activeSet.GenWeights())
		conflictLabel->SetLabel(wxString::FromUTF8(outputFilePath) + "_0.nif (and _1.nif)");
	else
		conflictLabel->SetLabel(wxString::FromUTF8(outputFilePath) + ".nif");

	conflictLabel->SetForegroundColour(wxColour(textColourName));
	conflictLabel->Show();

	if (1 < col.size())
		conflictInfo->Show();
	else
		conflictInfo->Hide();

	UpdateBuildRecordLabel();

	if (sliderView->leftPanel)
		sliderView->leftPanel->Layout();
}

void BodySlideApp::SetDefaultBuildSelection() {
	if (projects.empty())
		return;

	auto& activeSet = GetActiveSet();
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	auto outputFilePath = activeSet.GetOutputFilePath();
	std::string choiceName = activeSet.GetName();

	bool willSet = 0 != choiceName.compare(buildSelection.GetOutputChoice(outputFilePath));
	if (willSet) {
		buildSelection.SetOutputChoice(outputFilePath, activeSet.GetName());
		buildSelFile.UpdateOutputChoices(buildSelection);
	}
	else {
		buildSelection.SetOutputChoice(outputFilePath, "");
		buildSelFile.RemoveOutputChoice(outputFilePath);
	}

	buildSelFile.Save();

	UpdateConflictManager();
	sliderView->Refresh();
}
