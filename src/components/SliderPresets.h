/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <map>
#include <vector>

class SliderPreset {
public:
	float big;
	float small;
};

class PresetCollection {
	std::map<std::string, std::map<std::string, SliderPreset>> namedSliderPresets;
	std::map<std::string, std::string> presetFileNames;
	std::map<std::string, std::vector<std::string>> presetGroups;

public:
	void Clear();
	void ClearSlider(const std::string& presetName, const std::string& sliderName, const bool big = true);

	void GetPresetNames(std::vector<std::string>& outNames);
	void SetSliderPreset(const std::string& set, const std::string& slider, float big = -10000.0f, float small = -10000.0f);
	bool GetSliderExists(const std::string& set, const std::string& slider);
	bool GetBigPreset(const std::string& set, const std::string& slider, float& big);
	bool GetSmallPreset(const std::string& set, const std::string& slider, float& small);

	std::string GetPresetFileName(const std::string& set);
	void GetPresetGroups(const std::string& set, std::vector<std::string>& outGroups);

	bool LoadPresets(const std::string& basePath, const std::string& sliderSet, std::vector<std::string>& groupFilter, bool allPresets = false);
	int SavePreset(const std::string& filePath, const std::string& presetName, const std::string& sliderSetName, std::vector<std::string>& assignGroups);
	int DeletePreset(const std::string& filePath, const std::string& presetName);
};
