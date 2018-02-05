/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "../TinyXML-2/tinyxml2.h"
#include "SliderPresets.h"
#include "../utils/PlatformUtil.h"

#include <wx/dir.h>

using namespace tinyxml2;

void PresetCollection::Clear() {
	namedSliderPresets.clear();
	presetFileNames.clear();
	presetGroups.clear();
}

void PresetCollection::ClearSlider(const std::string& presetName, const std::string& sliderName, const bool big) {
	if (namedSliderPresets.find(presetName) != namedSliderPresets.end()) {
		if (big)
			namedSliderPresets[presetName][sliderName].big = -10000.0f;
		else
			namedSliderPresets[presetName][sliderName].small = -10000.0f;
	}
}

void PresetCollection::GetPresetNames(std::vector<std::string>& outNames) {
	for (auto &it : namedSliderPresets)
		outNames.push_back(it.first);
}

void PresetCollection::AddEmptyPreset(const std::string& set) {
	namedSliderPresets.try_emplace(set);
}

void PresetCollection::SetSliderPreset(const std::string& set, const std::string& slider, float big, float small) {
	std::map<std::string, SliderPreset> newPreset;
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
			if (big > -10000.0f)
				namedSliderPresets[set][slider].big = big;
			if (small > -10000.0f)
				namedSliderPresets[set][slider].small = small;
		}
	}
}

bool PresetCollection::GetSliderExists(const std::string& set, const std::string& slider) {
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;

	return true;
}

bool PresetCollection::GetBigPreset(const std::string& set, const std::string& slider, float& big) {
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

bool PresetCollection::GetSmallPreset(const std::string& set, const std::string& slider, float& small) {
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

std::string PresetCollection::GetPresetFileName(const std::string& set) {
	auto result = presetFileNames.find(set);
	if (result != presetFileNames.end())
		return result->second;

	return "";
}

void PresetCollection::GetPresetGroups(const std::string& set, std::vector<std::string>& outGroups) {
	outGroups.clear();

	auto result = presetGroups.find(set);
	if (result != presetGroups.end())
		outGroups = result->second;
}

bool PresetCollection::LoadPresets(const std::string& basePath, const std::string& sliderSet, std::vector<std::string>& groupFilter, bool allPresets) {
	XMLDocument doc;
	XMLElement* root;
	XMLElement* element;
	XMLElement* g;
	XMLElement* setSlider;

	std::string presetName, sliderName, applyTo;
	float o, b, s;

	wxArrayString files;
	wxDir::GetAllFiles(basePath, &files, "*.xml");

	for (auto &file : files) {
		FILE* fp = nullptr;

#ifdef _WINDOWS
		std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(file.ToUTF8().data());
		int ret = _wfopen_s(&fp, winFileName.c_str(), L"rb");
		if (ret)
			continue;
#else
		fp = fopen(file.ToUTF8().data(), "rb");
		if (!fp)
			continue;
#endif

		ret = doc.LoadFile(fp);
		fclose(fp);

		if (ret)
			continue;

		root = doc.FirstChildElement("SliderPresets");
		if (!root)
			continue;

		element = root->FirstChildElement("Preset");
		while (element) {
			bool skip = true;
			std::vector<std::string> groups;

			g = element->FirstChildElement("Group");
			while (g) {
				std::string groupName = g->Attribute("name");
				groups.push_back(groupName);

				for (auto &filter : groupFilter) {
					if (groupName == filter) {
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
			if (presetFileNames.find(presetName) != presetFileNames.end()) {
				element = element->NextSiblingElement("Preset");
				continue;
			}

			presetFileNames[presetName] = file.ToUTF8();
			presetGroups[presetName] = groups;

			AddEmptyPreset(presetName);

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

int PresetCollection::SavePreset(const std::string& filePath, const std::string& presetName, const std::string& sliderSetName, std::vector<std::string>& assignGroups) {
	XMLElement* newElement = nullptr;
	XMLDocument outDoc;
	XMLNode* slidersNode = nullptr;
	XMLElement* presetElem = nullptr;

	bool loaded = false;
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
	int ret = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (ret == 0)
		loaded = true;
#else
	fp = fopen(filePath.c_str(), "rb");
	if (fp)
		loaded = true;
#endif

	if (loaded) {
		ret = outDoc.LoadFile(fp);
		fclose(fp);
		fp = nullptr;

		if (ret != 0)
			loaded = false;
	}

	if (loaded) {
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

#ifdef _WINDOWS
	winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
	ret = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (ret)
		return 1;
#else
	fp = fopen(filePath.c_str(), "w");
	if (!fp)
		return 1;
#endif

	outDoc.SetBOM(true);

	const tinyxml2::XMLNode* firstChild = outDoc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		outDoc.InsertFirstChild(outDoc.NewDeclaration());

	int retSaveDoc = outDoc.SaveFile(fp);
	fclose(fp);
	if (retSaveDoc)
		return retSaveDoc;

	return 0;
}

int PresetCollection::DeletePreset(const std::string& filePath, const std::string& presetName) {
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
	int ret = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (ret)
		return -1;
#else
	fp = fopen(filePath.c_str(), "rb");
	if (!fp)
		return -1;
#endif

	XMLDocument doc;
	ret = doc.LoadFile(fp);
	fclose(fp);
	fp = nullptr;

	if (ret)
		return -1;

	XMLNode* slidersNode = doc.FirstChildElement("SliderPresets");
	if (slidersNode) {
		XMLElement* presetElem = slidersNode->FirstChildElement("Preset");
		while (presetElem) {
			if (_stricmp(presetElem->Attribute("name"), presetName.c_str()) == 0) {
				slidersNode->DeleteChild(presetElem);
				break;
			}
			else
				presetElem = presetElem->NextSiblingElement("Preset");
		}
	}

#ifdef _WINDOWS
	winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
	ret = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (ret)
		return -1;
#else
	fp = fopen(filePath.c_str(), "w");
	if (!fp)
		return -1;
#endif

	doc.SetBOM(true);

	const tinyxml2::XMLNode* firstChild = doc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		doc.InsertFirstChild(doc.NewDeclaration());

	int retSaveDoc = doc.SaveFile(fp);
	fclose(fp);
	if (retSaveDoc)
		return retSaveDoc;

	namedSliderPresets.erase(presetName);
	presetFileNames.erase(presetName);
	presetGroups.erase(presetName);

	return 0;
}
