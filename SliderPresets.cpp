/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "tinyxml2.h"
#include "SliderPresets.h"

#include <wx/dir.h>

using namespace tinyxml2;

void PresetCollection::Clear() {
	namedSliderPresets.clear();
}

void PresetCollection::GetPresetNames(vector<string>& outNames) {
	for (auto &it : namedSliderPresets)
		outNames.push_back(it.first);
}

void PresetCollection::SetSliderPreset(const string& set, const string& slider, float big, float small) {
	map<string, SliderPreset> newPreset;
	SliderPreset sp;
	if (namedSliderPresets.find(set) == namedSliderPresets.end()) {
		sp.big = sp.small = -10000.0f;
		if (big > -10000.0f)
			sp.big = big;
		if (small > -10000.0f)
			sp.small = small;
		newPreset[slider] = sp;
		namedSliderPresets[set] = newPreset;

	}
	else {
		if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end()) {
			sp.big = sp.small = -10000.0f;
			if (big > -10000.0f)
				sp.big = big;
			if (small > -10000.0f)
				sp.small = small;
			namedSliderPresets[set][slider] = sp;
		}
		else {
			if (big > -10000.0f) {
				namedSliderPresets[set][slider].big = big;
			}
			if (small > -10000.0f) {
				namedSliderPresets[set][slider].small = small;
			}
		}
	}
}

bool PresetCollection::GetSliderExists(const string& set, const string& slider) {
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;

	return true;
}

bool PresetCollection::GetBigPreset(const string& set, const string& slider, float& big) {
	float b;
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;
	b = namedSliderPresets[set][slider].big;
	if (b > -10000.0f) {
		big = b;
		return true;
	}
	return false;
}

bool PresetCollection::GetSmallPreset(const string& set, const string& slider, float& small) {
	float b;
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;
	b = namedSliderPresets[set][slider].small;
	if (b > -10000.0f) {
		small = b;
		return true;
	}
	return false;
}

bool PresetCollection::LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilter, bool allPresets) {
	XMLDoc doc;
	XMLElement* root;
	XMLElement* element;
	XMLElement* g;
	XMLElement* setSlider;

	string presetName, sliderName, applyTo;
	float o, b, s;

	wxArrayString files;
	wxDir::GetAllFiles(basePath, &files, "*.xml");

	for (auto &file : files) {
		if (doc.LoadFile(file) != XML_SUCCESS)
			continue;

		root = doc.FirstChildElement("SliderPresets");
		if (!root)
			continue;

		element = root->FirstChildElement("Preset");
		while (element) {
			bool skip = true;
			g = element->FirstChildElement("Group");
			while (g) {
				for (auto &filter : groupFilter) {
					if (g->Attribute("name") == filter) {
						skip = false;
						break;
					}
				}
				g = g->NextSiblingElement("Group");
			}
			if (element->Attribute("set") == sliderSet || allPresets)
				skip = false;

			if (skip) {
				element = element->NextSiblingElement("Preset");
				continue;
			}

			presetName = element->Attribute("name");
			setSlider = element->FirstChildElement("SetSlider");
			while (setSlider) {
				sliderName = setSlider->Attribute("name");
				applyTo = setSlider->Attribute("size");
				o = setSlider->FloatAttribute("value") / 100.0f;
				s = b = -10000.0f;
				if (applyTo == "small")
					s = o;
				else if (applyTo == "big")
					b = o;
				else if (applyTo == "both")
					s = b = o;

				SetSliderPreset(presetName, sliderName, b, s);
				setSlider = setSlider->NextSiblingElement("SetSlider");
			}
			element = element->NextSiblingElement("Preset");
		}
	}

	return 0;
}

int PresetCollection::SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
	if (namedSliderPresets.find(presetName) == namedSliderPresets.end())
		return -1;

	XMLElement* newElement = nullptr;
	XMLDoc outDoc;
	XMLNode* slidersNode;
	XMLElement* presetElem;
	if (outDoc.LoadFile(filePath.c_str()) == XML_SUCCESS) {
		// File exists - merge data.
		slidersNode = outDoc.FirstChildElement("SliderPresets");
		presetElem = slidersNode->FirstChildElement("Preset");
		while (presetElem) {
			// Replace preset if found in file.
			if (_stricmp(presetElem->Attribute("name"), presetName.c_str()) == 0) {
				XMLElement* tmpElem = presetElem;
				presetElem = presetElem->NextSiblingElement("Preset");
				slidersNode->DeleteChild(tmpElem);
			}
			else
				presetElem = presetElem->NextSiblingElement("Preset");
		}
	}
	else {
		newElement = outDoc.NewElement("SliderPresets");
		slidersNode = outDoc.InsertEndChild(newElement);
	}

	newElement = outDoc.NewElement("Preset");
	presetElem = slidersNode->InsertEndChild(newElement)->ToElement();
	presetElem->SetAttribute("name", presetName.c_str());
	presetElem->SetAttribute("set", sliderSetName.c_str());

	XMLElement* sliderElem;
	for (auto &group : assignGroups) {
		newElement = outDoc.NewElement("Group");
		sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
		sliderElem->SetAttribute("name", group.c_str());
	}
	for (auto &p : namedSliderPresets[presetName]) {
		if (p.second.big > -10000.0f) {
			newElement = outDoc.NewElement("SetSlider");
			sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "big");
			sliderElem->SetAttribute("value", (int)(p.second.big * 100.0f));
		}
		if (p.second.small > -10000.0f) {
			newElement = outDoc.NewElement("SetSlider");
			sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "small");
			sliderElem->SetAttribute("value", (int)(p.second.small * 100.0f));
		}
	}
	if (outDoc.SaveFile(filePath.c_str()) != XML_SUCCESS)
		return outDoc.ErrorID();

	return 0;
}