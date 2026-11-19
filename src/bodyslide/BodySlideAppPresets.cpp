/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: presets and favorites

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"
#include "utils/GameUtil.h"

#include <algorithm>
#include <set>
#include <utility>

using namespace nifly;

bool BodySlideApp::PresetExists(const std::string& name) {
	if (name.empty())
		return false;

	std::vector<std::string> presetNames;
	sliderManager.GetPresetNames(presetNames);
	return std::find(presetNames.begin(), presetNames.end(), name) != presetNames.end();
}

std::string BodySlideApp::GetFavoriteConfigKey(const std::string& listName) const {
	return "Favorites/" + GameUtil::TargetGames[targetGame].ToStdString() + "/" + listName;
}

namespace {
constexpr char FavoriteSeparator = ';';
constexpr char FavoriteEscape = '\\';
} // namespace

std::string BodySlideApp::SerializeFavoriteNames(const std::vector<std::string>& names) const {
	std::string result;
	for (size_t i = 0; i < names.size(); i++) {
		if (i > 0)
			result.push_back(FavoriteSeparator);

		for (char c : names[i]) {
			if (c == FavoriteSeparator || c == FavoriteEscape)
				result.push_back(FavoriteEscape);

			result.push_back(c);
		}
	}

	return result;
}

std::vector<std::string> BodySlideApp::DeserializeFavoriteNames(const std::string& value) const {
	std::vector<std::string> names;
	std::string current;
	bool escaped = false;

	for (char c : value) {
		if (escaped) {
			current.push_back(c);
			escaped = false;
		}
		else if (c == FavoriteEscape)
			escaped = true;
		else if (c == FavoriteSeparator) {
			if (!current.empty())
				names.push_back(current);
			current.clear();
		}
		else
			current.push_back(c);
	}

	if (escaped)
		current.push_back(FavoriteEscape);

	if (!current.empty())
		names.push_back(current);

	return names;
}

void BodySlideApp::SetFavoriteList(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::vector<std::string>& names) {
	list.clear();
	set.clear();

	for (const auto& name : names) {
		if (!name.empty() && set.insert(name).second)
			list.push_back(name);
	}
}

bool BodySlideApp::RemoveFavoriteName(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::string& name) {
	if (set.erase(name) == 0)
		return false;

	list.erase(std::remove(list.begin(), list.end(), name), list.end());
	return true;
}

void BodySlideApp::SortFavoritesFirst(std::vector<std::string>& names, const std::unordered_set<std::string>& favorites) const {
	std::sort(names.begin(), names.end(), case_insensitive_compare());
	std::stable_partition(names.begin(), names.end(), [&favorites](const std::string& name) { return favorites.find(name) != favorites.end(); });
}

void BodySlideApp::LoadFavorites() {
	SetFavoriteList(favoriteOutfits, favoriteOutfitSet, DeserializeFavoriteNames(BodySlideConfig[GetFavoriteConfigKey("Outfits")]));
	SetFavoriteList(favoritePresets, favoritePresetSet, DeserializeFavoriteNames(BodySlideConfig[GetFavoriteConfigKey("Presets")]));
}

void BodySlideApp::SaveFavorites() {
	BodySlideConfig.SetValue(GetFavoriteConfigKey("Outfits"), SerializeFavoriteNames(favoriteOutfits));
	BodySlideConfig.SetValue(GetFavoriteConfigKey("Presets"), SerializeFavoriteNames(favoritePresets));
}

bool BodySlideApp::IsFavoriteOutfit(const std::string& name) const {
	return favoriteOutfitSet.find(name) != favoriteOutfitSet.end();
}

bool BodySlideApp::IsFavoritePreset(const std::string& name) const {
	return favoritePresetSet.find(name) != favoritePresetSet.end();
}

void BodySlideApp::SortOutfitNamesForDisplay(std::vector<std::string>& names) const {
	SortFavoritesFirst(names, favoriteOutfitSet);
}

void BodySlideApp::SortPresetNamesForDisplay(std::vector<std::string>& names) const {
	SortFavoritesFirst(names, favoritePresetSet);
}

void BodySlideApp::ToggleFavoriteOutfit(const std::string& name) {
	if (name.empty() || !OutfitExists(name))
		return;

	if (!RemoveFavoriteName(favoriteOutfits, favoriteOutfitSet, name)) {
		favoriteOutfitSet.insert(name);
		favoriteOutfits.push_back(name);
	}

	SortOutfitNamesForDisplay(filteredOutfits);

	SaveFavorites();
}

void BodySlideApp::ToggleFavoritePreset(const std::string& name) {
	if (name.empty() || !PresetExists(name))
		return;

	if (!RemoveFavoriteName(favoritePresets, favoritePresetSet, name)) {
		favoritePresetSet.insert(name);
		favoritePresets.push_back(name);
	}

	SaveFavorites();
}

void BodySlideApp::RemoveFavoriteOutfit(const std::string& name) {
	if (RemoveFavoriteName(favoriteOutfits, favoriteOutfitSet, name))
		SaveFavorites();
}

void BodySlideApp::RemoveFavoritePreset(const std::string& name) {
	if (RemoveFavoriteName(favoritePresets, favoritePresetSet, name))
		SaveFavorites();
}

void BodySlideApp::ActivatePreset(const std::string& presetName, const bool updatePreview) {
	wxLogMessage("Applying preset '%s' to sliders.", presetName);

	BodySlideConfig.SetValue("SelectedPreset", presetName);
	std::vector<bool> zappedBefore = ZappedSliders();
	sliderManager.InitializeSliders(presetName);

	bool zapChanged = false;
	Slider* sliderSmall = nullptr;
	Slider* sliderBig = nullptr;
	for (size_t i = 0; i < sliderManager.slidersBig.size(); i++) {
		sliderSmall = &sliderManager.slidersSmall[i];
		sliderBig = &sliderManager.slidersBig[i];

		if (sliderBig->zap && !sliderBig->uv && !zapChanged) {
			auto* sd = sliderView->GetSliderDisplay(sliderBig->name);
			if (sd) {
				float zapValueUI = sd->zapCheckHi->IsChecked() ? 0.01f : 0.0f;
				if (sliderBig->value != zapValueUI)
					zapChanged = true;
			}
		}

		sliderView->SetSliderPosition(sliderSmall->name.c_str(), sliderSmall->value, SLIDER_LO);
		sliderView->SetSliderPosition(sliderBig->name.c_str(), sliderBig->value, SLIDER_HI);
	}

	sliderView->SetPresetChanged(false);
	UpdateBuildRecordLabel();

	if (UpdateZapChoices())
		zapChanged = true;

	// Zaps that toggles hold change the mesh as well
	UpdateToggledSliders();
	if (ZappedSliders() != zappedBefore)
		zapChanged = true;

	if (preview && updatePreview)
		zapChanged ? RebuildPreviewMeshes() : UpdatePreview();
}

void BodySlideApp::DeletePreset(const std::string& presetName) {
	std::string outputFile = sliderManager.GetPresetFileNames(presetName);
	if (outputFile.empty())
		return;

	int select = wxNOT_FOUND;
	if (sliderView->presetChoice)
		select = sliderView->presetChoice->GetSelection();

	wxLogMessage("Deleting preset '%s'...", presetName);
	if (!sliderManager.DeletePreset(outputFile, presetName)) {
		RemoveFavoritePreset(presetName);
		LoadPresets("");
		PopulatePresetList(presetName);

		if (sliderView->presetChoice) {
			int count = sliderView->presetChoice->GetCount();
			if (count > select)
				sliderView->presetChoice->Select(select);
			else if (count > select - 1)
				sliderView->presetChoice->Select(select - 1);

			ActivatePreset(sliderView->GetSelectedPresetName());
		}
	}
	else
		wxLogMessage("Failed to delete preset!");
}

void BodySlideApp::PopulatePresetList(const std::string& select) {
	std::string myselect = BodySlideConfig["SelectedPreset"];
	if (!select.empty())
		myselect = select;

	std::vector<std::string> presets;

	wxArrayString items;
	sliderManager.GetPresetNames(presets);

	std::vector<std::string> filteredPresets = ApplyPresetFilter(presets);
	items.reserve(filteredPresets.size());
	for (size_t i = 0; i < filteredPresets.size(); i++)
		items.Add(wxString::FromUTF8(filteredPresets[i]));

	sliderView->PopulatePresetList(items, wxString::FromUTF8(myselect));
}

void BodySlideApp::LoadCmdPresetFile() {
	if (cmdPresetFile.empty())
		return;

	std::vector<std::string> noGroupFilter;
	std::vector<std::string> loadedPresets;

	// Loaded before the preset folder so that the file given on the command line wins on name conflicts.
	if (!sliderManager.LoadPresetFile(cmdPresetFile, "", noGroupFilter, true, &loadedPresets)) {
		wxLogError("Failed to load preset file '%s' from the command line.", cmdPresetFile);
		cmdPresetFile.clear();
		cmdPresetPending = false;
		return;
	}

	// The presets of the file are loaded again whenever the preset collection was cleared, the name is only resolved once.
	if (cmdPresetResolved)
		return;

	cmdPresetResolved = true;

	if (!cmdPreset.empty()) {
		if (std::find(loadedPresets.begin(), loadedPresets.end(), cmdPreset) == loadedPresets.end())
			wxLogWarning("Preset '%s' was not found in preset file '%s' from the command line.", cmdPreset, cmdPresetFile);

		return;
	}

	if (loadedPresets.empty()) {
		wxLogWarning("No presets found in preset file '%s' from the command line.", cmdPresetFile);
		cmdPresetPending = false;
		return;
	}

	// Without an explicit name, the first preset of the file is used.
	cmdPreset = loadedPresets.front();
	wxLogMessage("Using preset '%s' of preset file '%s' from the command line.", cmdPreset, cmdPresetFile);
}

void BodySlideApp::LoadPresets(const std::string& sliderSet) {
	std::string outfit = sliderSet;
	if (sliderSet.empty())
		outfit = BodySlideConfig["SelectedOutfit"];

	wxLogMessage("Loading assigned presets...");

	LoadCmdPresetFile();

	std::vector<std::string> groups_and_aliases;
	for (auto& g : presetGroups) {
		groups_and_aliases.push_back(g);
		for (auto& ag : this->groupAlias)
			if (ag.second == g)
				groups_and_aliases.push_back(ag.first);
	}

	sliderManager.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", outfit, groups_and_aliases, groups_and_aliases.empty());
}

void BodySlideApp::GetPresetNames(std::vector<std::string>& outNames) {
	sliderManager.GetPresetNames(outNames);
}

std::string BodySlideApp::GetPresetFileName(const std::string& presetName) {
	return sliderManager.GetPresetFileNames(presetName);
}

void BodySlideApp::GetPresetGroups(const std::string& presetName, std::vector<std::string>& outGroups) {
	sliderManager.GetPresetGroups(presetName, outGroups);
}

void BodySlideApp::InitializeSliders(const std::string& presetName) {
	sliderManager.InitializeSliders(presetName);
}

void BodySlideApp::RefreshPresetsForCurrentOutfit() {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	std::string presetName = BodySlideConfig["SelectedPreset"];

	sliderManager.ClearPresets();
	SetPresetGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList(presetName);
}

int BodySlideApp::UpdateSliderPositions(const std::string& presetName) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	std::vector<std::string> groups;

	std::string outputFile = sliderManager.GetPresetFileNames(presetName);
	if (outputFile.empty())
		return -2;

	sliderManager.GetPresetGroups(presetName, groups);

	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

int BodySlideApp::SaveSliderPositions(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	return sliderManager.SavePreset(outputFile, presetName, outfitName, groups);
}

int BodySlideApp::SavePresetGroups(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups) {
	std::string outfitName = BodySlideConfig["SelectedOutfit"];
	return sliderManager.SavePresetGroups(outputFile, presetName, outfitName, groups);
}
