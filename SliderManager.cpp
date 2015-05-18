#include "SliderManager.h"
#include <sstream>
#include <algorithm>


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
			AddHiddenSlider(inSet[i].Name, inSet[i].bInvert, inSet[i].bZap, inSet[i].bUV);
		}
		else {
			if (inSet[i].bZap)
				AddZapSlider(inSet[i].Name);
			else if (inSet[i].bUV)
				AddUVSlider(inSet[i].Name, inSet[i].bInvert);
			else
				AddSlider(inSet[i].Name, inSet[i].bInvert);
		}

		SetSliderDefaults(inSet[i].Name, inSet[i].defBigValue / 100.0f, inSet[i].defSmallValue / 100.0f);
		if (inSet[i].bClamp)
			SetClampSlider(inSet[i].Name);

		for (int j = 0; j < inSet[i].dataFiles.size(); j++)
			AddSliderLink(inSet[i].Name, inSet[i].dataFiles[j].dataName);

		for (auto reqIter = inSet[i].requirements.begin(); reqIter != inSet[i].requirements.end(); ++reqIter)
			AddSliderTrigger(reqIter->first, inSet[i].Name, reqIter->second, 0);
	}
}

void SliderManager::AddSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = false;
	s.changed = false;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddUVSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = true;
	s.changed = false;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddZapSlider(const string& name, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.Value = 0;
	s.defValue = 0;
	s.invert = false;
	s.zap = true;
	s.clamp = false;
	s.uv = false;
	s.changed = false;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddHiddenSlider(const string& name, bool invert, bool isZap, bool isUv, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);

	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = isZap;
	s.clamp = false;
	s.uv = isUv;
	s.changed = false;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
}

void SliderManager::SetSliderDefaults(const string& slider, float bigVal, float smallVal) {
	for (int i = 0; i < SlidersBig.size(); i++)
		if (SlidersBig[i].Name == slider)
			SlidersBig[i].defValue = bigVal;

	for (int i = 0; i < SlidersSmall.size(); i++)
		if (SlidersSmall[i].Name == slider)
			SlidersSmall[i].defValue = smallVal;
}

void SliderManager::SetClampSlider(const string& slider) {
	for (int i = 0; i < SlidersBig.size(); i++)
		if (SlidersBig[i].Name == slider)
			SlidersBig[i].clamp = true;

	for (int i = 0; i < SlidersSmall.size(); i++)
		if (SlidersSmall[i].Name == slider)
			SlidersSmall[i].clamp = true;
}

void SliderManager::AddSliderLink(const string& slider, const string& dataSetName) {
	for (int i = 0; i < SlidersBig.size(); i++)
		if (SlidersBig[i].Name == slider)
			SlidersBig[i].linkedDataSets.push_back(dataSetName);

	for (int i = 0; i < SlidersSmall.size(); i++)
		if (SlidersSmall[i].Name == slider)
			SlidersSmall[i].linkedDataSets.push_back(dataSetName);
}

void SliderManager::AddSliderTrigger(const string& slider, const string& targetSlider, float triggerVal, uint triggerType) {
	int targetIdx = -1;
	int sliderIdx = -1;
	SliderNotifyTrigger trigger;
	trigger.triggerValue = triggerVal;
	trigger.triggerType = triggerType;
	for (int i = 0; i < SlidersBig.size(); i++) {
		if (SlidersBig[i].Name == slider)
			sliderIdx = i;
		if (SlidersBig[i].Name == targetSlider)
			targetIdx = i;
	}
	if (sliderIdx >= 0 && targetIdx >= 0) {
		trigger.targetSlider = SlidersBig[targetIdx].Name;
		SlidersBig[sliderIdx].triggers.push_back(trigger);
	}
	sliderIdx = -1; targetIdx = -1;
	for (int i = 0; i < SlidersSmall.size(); i++) {
		if (SlidersSmall[i].Name == slider)
			sliderIdx = i;
		if (SlidersSmall[i].Name == targetSlider)
			targetIdx = i;
	}
	if (sliderIdx >= 0 && targetIdx >= 0){
		trigger.targetSlider = SlidersSmall[targetIdx].Name;
		SlidersSmall[sliderIdx].triggers.push_back(trigger);
	}
}

float SliderManager::GetSlider(const string& slider, bool isSmall) {
	if (!isSmall) {
		for (int i = 0; i < SlidersBig.size(); i++)
			if (SlidersBig[i].Name == slider)
				return SlidersBig[i].Get();
	}
	else {
		for (int i = 0; i < SlidersSmall.size(); i++)
			if (SlidersSmall[i].Name == slider)
				return SlidersSmall[i].Get();
	}
	return 0.0f;
}

void SliderManager::SetSlider(const string& slider, bool isSmall, float val) {
	if (!isSmall) {
		for (int i = 0; i < SlidersBig.size(); i++) {
			if (SlidersBig[i].Name == slider) {
				if (SlidersBig[i].zap) {
					FlagReload(true);
					if (val > 0)
						val = 1.0;
				}
				SlidersBig[i].Set(val);
				return;
			}
		}
	}
	else {
		for (int i = 0; i < SlidersSmall.size(); i++) {
			if (SlidersSmall[i].Name == slider) {
				if (SlidersSmall[i].zap) {
					FlagReload(true);
					if (val > 0)
						val = 1.0;
				}
				SlidersSmall[i].Set(val);
				return;
			}
		}
	}
}

void SliderManager::SetChanged(const string& slider, bool isSmall) {
	if (!isSmall) {
		for (int i = 0; i < SlidersBig.size(); i++) {
			if (SlidersBig[i].Name == slider) {
				SlidersBig[i].changed = true;
				return;
			}
		}
	}
	else {
		for (int i = 0; i < SlidersSmall.size(); i++) {
			if (SlidersSmall[i].Name == slider) {
				SlidersSmall[i].changed = true;
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
	for (int i = 0; i < SlidersBig.size(); i++) {
		if (!presetCollection.GetBigPreset(presetName, SlidersBig[i].Name, ps))
			ps = SlidersBig[i].defValue;

		SlidersBig[i].Set(ps);
	}
	for (int i = 0; i < SlidersSmall.size(); i++) {
		if (!presetCollection.GetSmallPreset(presetName, SlidersSmall[i].Name, ps))
			ps = SlidersSmall[i].defValue;

		SlidersSmall[i].Set(ps);
	}
}

bool SliderManager::SliderHasChanged(const string& slider, bool getBig) {
	if (getBig) {
		for (int i = 0; i < SlidersBig.size(); i++)
			if (SlidersBig[i].Name == slider)
				return(SlidersBig[i].defValue != SlidersBig[i].Value || SlidersBig[i].changed);
	}
	else {
		for (int i = 0; i < SlidersSmall.size(); i++)
			if (SlidersSmall[i].Name == slider)
				return(SlidersSmall[i].defValue != SlidersSmall[i].Value || SlidersBig[i].changed);
	}
	return false;
}

float SliderManager::SliderValue(const string& slider, bool getBig) {
	if (getBig) {
		for (int i = 0; i < SlidersBig.size(); i++)
			if (SlidersBig[i].Name == slider)
				return(SlidersBig[i].Value);
	}
	else {
		for (int i = 0; i < SlidersSmall.size(); i++)
			if (SlidersSmall[i].Name == slider)
				return(SlidersSmall[i].Value);
	}
	return 0.0f;
}

void SliderManager::GetSmallSliderList(vector<string>& names) {
	for (int i = 0; i < SlidersSmall.size(); i++)
		names.push_back(SlidersSmall[i].Name);
}

void SliderManager::GetBigSliderList(vector<string>& names) {
	for (int i = 0; i < SlidersBig.size(); i++)
		names.push_back(SlidersBig[i].Name);
}
