/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "SliderSet.h"
#include "SliderPresets.h"

using namespace std;

class Slider {
public:
	string name;
	bool invert;
	bool zap;
	bool clamp;
	bool uv;
	bool changed;
	float defValue;
	float value;
	vector<string> linkedDataSets;
};

class SliderManager {
	int mSliderCount;
	PresetCollection presetCollection;
	bool bNeedReload;

public:
	vector<Slider> slidersBig;
	vector<Slider> slidersSmall;

	SliderManager();
	~SliderManager();

	void ClearSliders() {
		slidersBig.clear();
		slidersSmall.clear();
		mSliderCount = 0;
	}

	bool LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilters) {
		return presetCollection.LoadPresets(basePath, sliderSet, groupFilters);
	}

	int SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
		int index = 0;
		for (auto &s : slidersBig) {
			if (SliderHasChanged(s.name, true))
				presetCollection.SetSliderPreset(presetName, s.name, s.value);
			if (SliderHasChanged(s.name, false))
				presetCollection.SetSliderPreset(presetName, slidersSmall[index].name, -10000.0f, slidersSmall[index].value);
			index++;
		}
		return presetCollection.SavePreset(filePath, presetName, sliderSetName, assignGroups);
	}

	void SetSliderPreset(const string& presetName, const string& slider, float big = -1, float little = -1) {
		presetCollection.SetSliderPreset(presetName, slider, big, little);
	}

	float GetBigPresetValue(const string& presetName, const string& sliderName, float defVal = 0.0f);
	float GetSmallPresetValue(const string& presetName, const string& sliderName, float defVal = 0.0f);

	void GetPresetNames(vector<string>& outNames) {
		presetCollection.GetPresetNames(outNames);
	}

	string GetPresetFileNames(const string& set) {
		return presetCollection.GetPresetFileName(set);
	}

	void GetPresetGroups(const string& set, vector<string>& outGroups) {
		presetCollection.GetPresetGroups(set, outGroups);
	}

	void ClearPresets() {
		presetCollection.Clear();
	}

	void AddSlidersInSet(SliderSet& inSet, bool hideAll = false);

	void AddSlider(const string& name, bool invert = false, const string& dataSetName = "");
	void AddHiddenSlider(const string& name, bool invert = false, bool isZap = false, bool isUV = false, const string& dataSetName = "");
	void AddZapSlider(const string& name, const string& dataSetName = "");
	void AddUVSlider(const string& name, bool invert = false, const string& dataSetName = "");
	void SetSliderDefaults(const string& slider, float bigVal, float smallVal);
	void SetClampSlider(const string& slider);
	void AddSliderLink(const string& slider, const string& dataSetName);

	float GetSlider(const string& slider, bool isSmall);
	void SetSlider(const string& slider, bool isSmall, float val);
	void SetChanged(const string& slider, bool isSmall);

	void InitializeSliders(const string& presetName = "");

	bool SliderHasChanged(const string& slider, bool getBig);
	float SliderValue(const string& slider, bool getBig);

	void FlagReload(bool needReload) {
		bNeedReload = needReload;
	}

	bool NeedReload() {
		return bNeedReload;
	}

	void GetSmallSliderList(vector<string>& names);
	void GetBigSliderList(vector<string>& names);
	int VisibleSliderCount() { return mSliderCount; }
};
