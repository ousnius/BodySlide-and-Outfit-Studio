/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderManager.h"

SliderManager::SliderManager() {
	mSliderCount = 0;
	bNeedReload = true;
}

SliderManager::~SliderManager() {
}

void SliderManager::AddSlidersInSet(SliderSet& inSet, bool hideAll) {
	int sz = inSet.size();

	for (int i = 0; i < sz; i++) {
		if (inSet[i].bHidden || hideAll) {
			AddHiddenSlider(inSet[i].name, inSet[i].bInvert, inSet[i].bZap, inSet[i].bUV);
		}
		else {
			if (inSet[i].bZap)
				AddZapSlider(inSet[i].name);
			else if (inSet[i].bUV)
				AddUVSlider(inSet[i].name, inSet[i].bInvert);
			else
				AddSlider(inSet[i].name, inSet[i].bInvert);
		}

		SetSliderDefaults(inSet[i].name, inSet[i].defBigValue / 100.0f, inSet[i].defSmallValue / 100.0f);
		if (inSet[i].bClamp)
			SetClampSlider(inSet[i].name);

		for (int j = 0; j < inSet[i].dataFiles.size(); j++)
			AddSliderLink(inSet[i].name, inSet[i].dataFiles[j].dataName);
	}
}

void SliderManager::AddSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.value = 0.0f;
	s.defValue = 0.0f;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = false;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddUVSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = true;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddZapSlider(const string& name, const string& dataSetName) {
	Slider s;
	s.name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.value = 0;
	s.defValue = 0;
	s.invert = false;
	s.zap = true;
	s.clamp = false;
	s.uv = false;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddHiddenSlider(const string& name, bool invert, bool isZap, bool isUv, const string& dataSetName) {
	Slider s;
	s.name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = isZap;
	s.clamp = false;
	s.uv = isUv;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
}

void SliderManager::SetSliderDefaults(const string& slider, float bigVal, float smallVal) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].defValue = bigVal;

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].defValue = smallVal;
}

void SliderManager::SetClampSlider(const string& slider) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].clamp = true;

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].clamp = true;
}

void SliderManager::AddSliderLink(const string& slider, const string& dataSetName) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].linkedDataSets.push_back(dataSetName);

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].linkedDataSets.push_back(dataSetName);
}

float SliderManager::GetSlider(const string& slider, bool isSmall) {
	if (!isSmall) {
		for (int i = 0; i < slidersBig.size(); i++)
			if (slidersBig[i].name == slider)
				return slidersBig[i].value;
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++)
			if (slidersSmall[i].name == slider)
				return slidersSmall[i].value;
	}
	return 0.0f;
}

void SliderManager::SetSlider(const string& slider, bool isSmall, float val) {
	if (!isSmall) {
		for (int i = 0; i < slidersBig.size(); i++) {
			if (slidersBig[i].name == slider) {
				if (slidersBig[i].zap) {
					FlagReload(true);
					if (val > 0.0f)
						val = 1.0f;
				}
				slidersBig[i].value = val;
				return;
			}
		}
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++) {
			if (slidersSmall[i].name == slider) {
				if (slidersSmall[i].zap) {
					FlagReload(true);
					if (val > 0.0f)
						val = 1.0f;
				}
				slidersSmall[i].value = val;
				return;
			}
		}
	}
}

void SliderManager::SetChanged(const string& slider, bool isSmall) {
	if (!isSmall) {
		for (int i = 0; i < slidersBig.size(); i++) {
			if (slidersBig[i].name == slider) {
				slidersBig[i].changed = true;
				return;
			}
		}
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++) {
			if (slidersSmall[i].name == slider) {
				slidersSmall[i].changed = true;
				return;
			}
		}
	}
}

float SliderManager::GetBigPresetValue(const string& presetName, const string& sliderName, float defVal) {
	float ps;
	if (!presetCollection.GetBigPreset(presetName, sliderName, ps))
		ps = defVal;

	return ps;
}

float SliderManager::GetSmallPresetValue(const string& presetName, const string& sliderName, float defVal){
	float ps;
	if (!presetCollection.GetSmallPreset(presetName, sliderName, ps))
		ps = defVal;

	return ps;
}

void SliderManager::InitializeSliders(const string& presetName) {
	float ps;
	for (int i = 0; i < slidersBig.size(); i++) {
		if (!presetCollection.GetBigPreset(presetName, slidersBig[i].name, ps))
			ps = slidersBig[i].defValue;

		slidersBig[i].value = ps;
	}
	for (int i = 0; i < slidersSmall.size(); i++) {
		if (!presetCollection.GetSmallPreset(presetName, slidersSmall[i].name, ps))
			ps = slidersSmall[i].defValue;

		slidersSmall[i].value = ps;
	}
}

bool SliderManager::SliderHasChanged(const string& slider, bool getBig) {
	if (getBig) {
		for (int i = 0; i < slidersBig.size(); i++)
			if (slidersBig[i].name == slider)
				return(slidersBig[i].defValue != slidersBig[i].value || slidersBig[i].changed);
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++)
			if (slidersSmall[i].name == slider)
				return(slidersSmall[i].defValue != slidersSmall[i].value || slidersSmall[i].changed);
	}
	return false;
}

float SliderManager::SliderValue(const string& slider, bool getBig) {
	if (getBig) {
		for (int i = 0; i < slidersBig.size(); i++)
			if (slidersBig[i].name == slider)
				return(slidersBig[i].value);
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++)
			if (slidersSmall[i].name == slider)
				return(slidersSmall[i].value);
	}
	return 0.0f;
}

void SliderManager::GetSmallSliderList(vector<string>& names) {
	for (int i = 0; i < slidersSmall.size(); i++)
		names.push_back(slidersSmall[i].name);
}

void SliderManager::GetBigSliderList(vector<string>& names) {
	for (int i = 0; i < slidersBig.size(); i++)
		names.push_back(slidersBig[i].name);
}
