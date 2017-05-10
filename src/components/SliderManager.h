/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "SliderSet.h"
#include "SliderPresets.h"

class Slider {
public:
	std::string name;
	bool invert;
	bool zap;
	bool clamp;
	bool uv;
	bool changed;
	float defValue;
	float value;
	std::vector<std::string> zapToggles;
	std::vector<std::string> linkedDataSets;
};

class SliderManager {
	int mSliderCount;
	PresetCollection presetCollection;
	bool bNeedReload;

public:
	std::vector<Slider> slidersBig;
	std::vector<Slider> slidersSmall;

	SliderManager();
	~SliderManager();

	void ClearSliders() {
		slidersBig.clear();
		slidersSmall.clear();
		mSliderCount = 0;
	}

	bool LoadPresets(const std::string& basePath, const std::string& sliderSet, std::vector<std::string>& groupFilters, const bool allPresets = false) {
		return presetCollection.LoadPresets(basePath, sliderSet, groupFilters, allPresets);
	}

	int SavePreset(const std::string& filePath, const std::string& presetName, const std::string& sliderSetName, std::vector<std::string>& assignGroups) {
		int index = 0;
		for (auto &s : slidersBig) {
			if (SliderHasChanged(s.name, true))
				presetCollection.SetSliderPreset(presetName, s.name, s.value);
			else
				presetCollection.ClearSlider(presetName, s.name, true);

			if (SliderHasChanged(s.name, false))
				presetCollection.SetSliderPreset(presetName, slidersSmall[index].name, -10000.0f, slidersSmall[index].value);
			else
				presetCollection.ClearSlider(presetName, slidersSmall[index].name, false);

			index++;
		}
		return presetCollection.SavePreset(filePath, presetName, sliderSetName, assignGroups);
	}

	void SetSliderPreset(const std::string& presetName, const std::string& slider, float big = -1, float little = -1) {
		presetCollection.SetSliderPreset(presetName, slider, big, little);
	}

	float GetBigPresetValue(const std::string& presetName, const std::string& sliderName, float defVal = 0.0f);
	float GetSmallPresetValue(const std::string& presetName, const std::string& sliderName, float defVal = 0.0f);

	void GetPresetNames(std::vector<std::string>& outNames) {
		presetCollection.GetPresetNames(outNames);
	}

	std::string GetPresetFileNames(const std::string& set) {
		return presetCollection.GetPresetFileName(set);
	}

	void GetPresetGroups(const std::string& set, std::vector<std::string>& outGroups) {
		presetCollection.GetPresetGroups(set, outGroups);
	}

	void ClearPresets() {
		presetCollection.Clear();
	}

	void AddSlidersInSet(SliderSet& inSet);

	void AddSlider(const std::string& name, bool invert = false, const std::string& dataSetName = "");
	void AddHiddenSlider(const std::string& name, bool invert = false, bool isZap = false, bool isUV = false, const std::string& dataSetName = "");
	void AddZapSlider(const std::string& name, const std::vector<std::string>& zapToggles, const std::string& dataSetName = "");
	void AddUVSlider(const std::string& name, bool invert = false, bool isZap = false, const std::string& dataSetName = "");
	void SetSliderDefaults(const std::string& slider, float bigVal, float smallVal);
	void SetClampSlider(const std::string& slider);
	void AddSliderLink(const std::string& slider, const std::string& dataSetName);

	float GetSlider(const std::string& slider, bool isSmall);
	std::vector<std::string> GetSliderZapToggles(const std::string& slider);
	void SetSlider(const std::string& slider, bool isSmall, float val);
	void SetChanged(const std::string& slider, bool isSmall);

	void InitializeSliders(const std::string& presetName = "");

	bool SliderHasChanged(const std::string& slider, bool getBig);
	float SliderValue(const std::string& slider, bool getBig);

	void FlagReload(bool needReload) {
		bNeedReload = needReload;
	}

	bool NeedReload() {
		return bNeedReload;
	}

	void GetSmallSliderList(std::vector<std::string>& names);
	void GetBigSliderList(std::vector<std::string>& names);
	int VisibleSliderCount() { return mSliderCount; }
};
