/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderManager.h"

SliderManager::SliderManager() {
	mSliderCount = 0;
	bNeedReload = true;
}

SliderManager::~SliderManager() {
}

void SliderManager::AddSlidersInSet(SliderSet& inSet) {
	int sz = inSet.size();

	for (int i = 0; i < sz; i++) {
		if (inSet[i].bHidden) {
			AddHiddenSlider(inSet[i].name, inSet[i].bInvert, inSet[i].bZap, inSet[i].bUV);
		}
		else {
			if (inSet[i].bZap && inSet[i].bUV)
				AddUVSlider(inSet[i].name, inSet[i].bInvert, true);
			else if (inSet[i].bZap)
				AddZapSlider(inSet[i].name, inSet[i].zapToggles);
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

void SliderManager::AddSlider(const std::string& name, bool invert, const std::string& dataSetName) {
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

void SliderManager::AddUVSlider(const std::string& name, bool invert, bool isZap, const std::string& dataSetName) {
	Slider s;
	s.name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = isZap;
	s.clamp = false;
	s.uv = true;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddZapSlider(const std::string& name, const std::vector<std::string>& zapToggles, const std::string& dataSetName) {
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
	s.zapToggles = zapToggles;
	s.changed = false;

	slidersSmall.push_back(s);
	slidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddHiddenSlider(const std::string& name, bool invert, bool isZap, bool isUv, const std::string& dataSetName) {
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

void SliderManager::SetSliderDefaults(const std::string& slider, float bigVal, float smallVal) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].defValue = bigVal;

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].defValue = smallVal;
}

void SliderManager::SetClampSlider(const std::string& slider) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].clamp = true;

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].clamp = true;
}

void SliderManager::AddSliderLink(const std::string& slider, const std::string& dataSetName) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			slidersBig[i].linkedDataSets.push_back(dataSetName);

	for (int i = 0; i < slidersSmall.size(); i++)
		if (slidersSmall[i].name == slider)
			slidersSmall[i].linkedDataSets.push_back(dataSetName);
}

float SliderManager::GetSlider(const std::string& slider, bool isSmall) {
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

std::vector<std::string> SliderManager::GetSliderZapToggles(const std::string& slider) {
	for (int i = 0; i < slidersBig.size(); i++)
		if (slidersBig[i].name == slider)
			return slidersBig[i].zapToggles;

	return std::vector<std::string>();
}

void SliderManager::SetSlider(const std::string& slider, bool isSmall, float val) {
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

void SliderManager::SetChanged(const std::string& slider, bool isSmall) {
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

float SliderManager::GetBigPresetValue(const std::string& presetName, const std::string& sliderName, float defVal) {
	float ps;
	if (!presetCollection.GetBigPreset(presetName, sliderName, ps))
		ps = defVal;

	return ps;
}

float SliderManager::GetSmallPresetValue(const std::string& presetName, const std::string& sliderName, float defVal){
	float ps;
	if (!presetCollection.GetSmallPreset(presetName, sliderName, ps))
		ps = defVal;

	return ps;
}

void SliderManager::InitializeSliders(const std::string& presetName) {
	float ps;
	for (int i = 0; i < slidersBig.size(); i++) {
		if (!presetCollection.GetBigPreset(presetName, slidersBig[i].name, ps)) {
			ps = slidersBig[i].defValue;
			slidersBig[i].changed = false;
		}

		slidersBig[i].value = ps;
	}
	for (int i = 0; i < slidersSmall.size(); i++) {
		if (!presetCollection.GetSmallPreset(presetName, slidersSmall[i].name, ps)) {
			ps = slidersSmall[i].defValue;
			slidersSmall[i].changed = false;
		}

		slidersSmall[i].value = ps;
	}
}

bool SliderManager::SliderHasChanged(const std::string& slider, bool getBig) {
	if (getBig) {
		for (int i = 0; i < slidersBig.size(); i++)
			if (slidersBig[i].name == slider)
				return (slidersBig[i].defValue != slidersBig[i].value || (slidersBig[i].changed && slidersBig[i].zap));
	}
	else {
		for (int i = 0; i < slidersSmall.size(); i++)
			if (slidersSmall[i].name == slider)
				return (slidersSmall[i].defValue != slidersSmall[i].value || (slidersSmall[i].changed && slidersSmall[i].zap));
	}
	return false;
}

float SliderManager::SliderValue(const std::string& slider, bool getBig) {
	if (getBig) {
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

void SliderManager::GetSmallSliderList(std::vector<std::string>& names) {
	for (int i = 0; i < slidersSmall.size(); i++)
		names.push_back(slidersSmall[i].name);
}

void SliderManager::GetBigSliderList(std::vector<std::string>& names) {
	for (int i = 0; i < slidersBig.size(); i++)
		names.push_back(slidersBig[i].name);
}
