/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <map>
#include <vector>

using namespace std;

class SliderPreset {
public:
	float big;
	float small;
};

class PresetCollection {
	map<string, map<string, SliderPreset>> namedSliderPresets;
public:

	void Clear();

	void GetPresetNames(vector<string>& outNames);
	void SetSliderPreset(const string& set, const string& slider, float big = -10000.0f, float small = -10000.0f);
	bool GetSliderExists(const string& set, const string& slider);
	bool GetBigPreset(const string& set, const string& slider, float& big);
	bool GetSmallPreset(const string& set, const string& slider, float& small);
	bool LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilter, bool allPresets = false);

	int SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups);
};
