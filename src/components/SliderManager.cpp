/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderManager.h"

#include <unordered_map>

Slider Slider::FromData(const SliderData& data) {
	Slider s;
	s.name = data.name;
	s.invert = data.bInvert;
	s.zap = data.bZap;
	s.clamp = data.bClamp;
	s.uv = data.bUV;
	s.toggle = data.bToggle && !data.bZap;
	s.toggleValue = data.toggleValue / 100.0f;
	for (auto& target : data.toggleSliders)
		s.toggleSliders.emplace_back(target.slider, target.value / 100.0f);

	s.zapToggles = data.zapToggles;
	for (auto& df : data.dataFiles)
		s.linkedDataSets.push_back(df.dataName);

	return s;
}

std::vector<float> ToggledSliderValues(const std::vector<Slider>& sliders, std::vector<int>* forcedBy) {
	std::vector<float> values(sliders.size());
	if (forcedBy)
		forcedBy->assign(sliders.size(), -1);

	bool anyTicked = false;
	for (size_t i = 0; i < sliders.size(); i++) {
		auto& slider = sliders[i];
		if (slider.toggle) {
			bool ticked = slider.value > 0.0f;
			values[i] = ticked ? slider.toggleValue : 0.0f;
			anyTicked |= ticked && !slider.toggleSliders.empty();
		}
		else
			values[i] = slider.value;
	}

	if (!anyTicked)
		return values;

	std::unordered_map<std::string, size_t> indices;
	for (size_t i = 0; i < sliders.size(); i++)
		if (!sliders[i].toggle && !sliders[i].clamp)
			indices.emplace(sliders[i].name, i);

	for (size_t i = 0; i < sliders.size(); i++) {
		auto& slider = sliders[i];
		if (!slider.toggle || slider.value <= 0.0f)
			continue;

		for (auto& target : slider.toggleSliders) {
			auto index = indices.find(target.first);
			if (index == indices.end())
				continue;

			values[index->second] = target.second;
			if (forcedBy)
				(*forcedBy)[index->second] = static_cast<int>(i);
		}
	}

	return values;
}

SliderManager::SliderManager() {
	mSliderCount = 0;
	bNeedReload = true;
}

SliderManager::~SliderManager() {}

void SliderManager::AddSlidersInSet(SliderSet& inSet) {
	int sz = static_cast<int>(inSet.size());

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
		if (inSet[i].bToggle && !inSet[i].bZap)
			SetToggleSlider(inSet[i].name, inSet[i].toggleValue, inSet[i].toggleSliders);

		for (size_t j = 0; j < inSet[i].dataFiles.size(); j++)
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

void SliderManager::SetSliderDefaults(const std::string& sliderName, float bigVal, float smallVal) {
	for (auto& slider : slidersBig)
		if (slider.name == sliderName)
			slider.defValue = bigVal;

	for (auto& slider : slidersSmall)
		if (slider.name == sliderName)
			slider.defValue = smallVal;
}

void SliderManager::SetClampSlider(const std::string& sliderName) {
	for (auto& slider : slidersBig)
		if (slider.name == sliderName)
			slider.clamp = true;

	for (auto& slider : slidersSmall)
		if (slider.name == sliderName)
			slider.clamp = true;
}

void SliderManager::SetToggleSlider(const std::string& sliderName, float toggleValue, const std::vector<ToggleTarget>& toggleSliders) {
	auto setToggle = [&](Slider& slider) {
		slider.toggle = true;
		slider.toggleValue = toggleValue / 100.0f;
		slider.toggleSliders.clear();
		for (auto& target : toggleSliders)
			slider.toggleSliders.emplace_back(target.slider, target.value / 100.0f);
	};

	for (auto& slider : slidersBig)
		if (slider.name == sliderName)
			setToggle(slider);

	for (auto& slider : slidersSmall)
		if (slider.name == sliderName)
			setToggle(slider);
}

void SliderManager::AddSliderLink(const std::string& sliderName, const std::string& dataSetName) {
	for (auto& slider : slidersBig)
		if (slider.name == sliderName)
			slider.linkedDataSets.push_back(dataSetName);

	for (auto& slider : slidersSmall)
		if (slider.name == sliderName)
			slider.linkedDataSets.push_back(dataSetName);
}

float SliderManager::GetSlider(const std::string& sliderName, bool isSmall) {
	if (!isSmall) {
		for (auto& slider : slidersBig)
			if (slider.name == sliderName)
				return slider.value;
	}
	else {
		for (auto& slider : slidersSmall)
			if (slider.name == sliderName)
				return slider.value;
	}
	return 0.0f;
}

std::vector<std::string> SliderManager::GetSliderZapToggles(const std::string& sliderName) {
	for (auto& slider : slidersBig)
		if (slider.name == sliderName)
			return slider.zapToggles;

	return std::vector<std::string>();
}

void SliderManager::SetSlider(const std::string& sliderName, bool isSmall, float val) {
	if (!isSmall) {
		for (auto& slider : slidersBig) {
			if (slider.name == sliderName) {
				if (slider.zap) {
					FlagReload(true);
					if (val > 0.0f)
						val = 1.0f;
				}
				else if (slider.toggle)
					val = val > 0.0f ? 1.0f : 0.0f;
				slider.value = val;
				return;
			}
		}
	}
	else {
		for (auto& slider : slidersSmall) {
			if (slider.name == sliderName) {
				if (slider.zap) {
					FlagReload(true);
					if (val > 0.0f)
						val = 1.0f;
				}
				else if (slider.toggle)
					val = val > 0.0f ? 1.0f : 0.0f;
				slider.value = val;
				return;
			}
		}
	}
}

void SliderManager::SetChanged(const std::string& sliderName, bool isSmall) {
	if (!isSmall) {
		for (size_t i = 0; i < slidersBig.size(); i++) {
			auto& sl = slidersBig[i];
			if (sl.name == sliderName) {
				sl.changed = true;

				if ((sl.zap || sl.toggle) && slidersSmall.size() > i) {
					auto& sls = slidersSmall[i];
					sls.changed = true;
				}

				return;
			}
		}
	}
	else {
		for (size_t i = 0; i < slidersSmall.size(); i++) {
			auto& sl = slidersSmall[i];
			if (sl.name == sliderName) {
				sl.changed = true;

				if ((sl.zap || sl.toggle) && slidersBig.size() > i) {
					auto& slb = slidersBig[i];
					slb.changed = true;
				}

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

float SliderManager::GetSmallPresetValue(const std::string& presetName, const std::string& sliderName, float defVal) {
	float ps;
	if (!presetCollection.GetSmallPreset(presetName, sliderName, ps))
		ps = defVal;

	return ps;
}

void SliderManager::InitializeSliders(const std::string& presetName) {
	float ps = 0.0f;

	for (auto& slider : slidersBig) {
		if (!presetCollection.GetBigPreset(presetName, slider.name, ps)) {
			ps = slider.defValue;
			slider.changed = false;
		}

		slider.value = ps;
	}

	for (auto& slider : slidersSmall) {
		if (!presetCollection.GetSmallPreset(presetName, slider.name, ps)) {
			ps = slider.defValue;
			slider.changed = false;
		}

		slider.value = ps;
	}
}

bool SliderManager::SliderHasChanged(const std::string& sliderName, bool getBig) {
	if (getBig) {
		for (auto& slider : slidersBig)
			if (slider.name == sliderName)
				return (slider.defValue != slider.value || (slider.changed && (slider.zap || slider.toggle)));
	}
	else {
		for (auto& slider : slidersSmall)
			if (slider.name == sliderName)
				return (slider.defValue != slider.value || (slider.changed && (slider.zap || slider.toggle)));
	}
	return false;
}

float SliderManager::SliderValue(const std::string& sliderName, bool getBig) {
	if (getBig) {
		for (auto& slider : slidersBig)
			if (slider.name == sliderName)
				return slider.value;
	}
	else {
		for (auto& slider : slidersSmall)
			if (slider.name == sliderName)
				return slider.value;
	}
	return 0.0f;
}

void SliderManager::GetSmallSliderList(std::vector<std::string>& names) {
	for (auto& slider : slidersSmall)
		names.push_back(slider.name);
}

void SliderManager::GetBigSliderList(std::vector<std::string>& names) {
	for (auto& slider : slidersBig)
		names.push_back(slider.name);
}
