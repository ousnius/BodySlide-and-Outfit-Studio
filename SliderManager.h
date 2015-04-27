#pragma once

#include "stdafx.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include "NifFile.h"
#include "DiffData.h"
#include "SliderData.h"

using namespace std;

class SliderNotifyTrigger {
public:
	float triggerValue;
	unsigned short triggerType;
	string targetSlider;
};

class Slider {
public:
	string Name;
	bool invert;
	bool zap;
	bool clamp;
	bool uv;
	bool changed;
	float defValue;
	float Value;
	vector<string> linkedDataSets;
	vector<SliderNotifyTrigger> triggers;

	float Get() {
		return Value;
	}
	void Set(float val) {
		Value = val;
	}
};

class SliderManager {
	int mSliderCount;
	PresetCollection presetCollection;
	bool bNeedReload;

public:
	vector<Slider> SlidersBig;
	vector<Slider> SlidersSmall;

	map<int, int> sliderEdits;  // ControlID -> slider index.


	SliderManager();
	~SliderManager();

	void ClearSliders() {
		SlidersBig.clear();
		SlidersSmall.clear();
		mSliderCount = 0;
	}

	bool LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilters) {
		return presetCollection.LoadPresets(basePath, sliderSet, groupFilters);
	}

	int SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
		int index = 0;
		for (auto s : SlidersBig) {
			if (SliderHasChanged(s.Name, true))
				presetCollection.SetSliderPreset(presetName, s.Name, s.Value);
			if (SliderHasChanged(s.Name, false))
				presetCollection.SetSliderPreset(presetName, SlidersSmall[index].Name, -10000.0f, SlidersSmall[index].Value);
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
	void AddSliderTrigger(const string& slider, const string& targetSlider, float triggerVal, unsigned int triggerType);

	float GetSlider(const string& slider, bool isSmall);
	void SetSlider(const string& slider, bool isSmall, float val);
	void SetChanged(const string& slider, bool isSmall);

	void InitializeSliders(const string& presetName = "");

	bool SliderHasChanged(const string& slider, bool getBig);
	float SliderValue(const string& slider, bool getBig);

	void FlagReload(bool needReload) {
		bNeedReload = needReload;
	}
	bool NeedReload() { return bNeedReload; }

	void GetSmallSliderList(vector<string>& names);
	void GetBigSliderList(vector<string>& names);
	int VisibleSliderCount() { return mSliderCount; }
};
