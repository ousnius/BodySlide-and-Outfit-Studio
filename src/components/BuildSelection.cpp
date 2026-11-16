/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BuildSelection.h"
#include "../utils/PlatformUtil.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <wx/filename.h>


std::string BuildRecord::NormalizeFolder(const std::string& folder) {
	if (folder.empty())
		return folder;

	wxFileName dir = wxFileName::DirName(wxString::FromUTF8(folder));
	dir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).ToUTF8().data();
}

bool BuildRecord::SameFolder(const std::string& a, const std::string& b) {
#ifdef _WINDOWS
	return wxString::FromUTF8(a).CmpNoCase(wxString::FromUTF8(b)) == 0;
#else
	return a == b;
#endif
}

void BuildRecord::AddPresetChange(const std::string& name, bool isSmall, bool zap, float value, float presetValue) {
	if (zap) {
		// Zaps only have the one value
		bool zapped = value > 0.0f;
		if (!isSmall && zapped != (presetValue > 0.0f))
			zaps.push_back({name, zapped});
		return;
	}

	// Compared in whole percent like the slider values of presets and the UI
	int percent = (int)std::lround(value * 100.0f);
	if (percent != (int)std::lround(presetValue * 100.0f))
		sliders.push_back({name, isSmall, percent});
}

int BuildSelection::LoadBuildSelection(XMLElement* srcElement) {
	if (srcElement == nullptr)
		return 1;

	XMLElement* elem = srcElement->FirstChildElement("OutputChoice");
	while (elem) {
		if (!elem->Attribute("path")) {
			elem = elem->NextSiblingElement("OutputChoice");
			continue;
		}

		std::string sName = elem->Attribute("path");
		if (elem->Attribute("choice"))
			outputChoice[sName] = elem->Attribute("choice");
		else
			outputChoice[sName].clear();

		elem = elem->NextSiblingElement("OutputChoice");
	}

	elem = srcElement->FirstChildElement("ZapChoice");
	while (elem) {
		if (!elem->Attribute("project") || !elem->Attribute("zap") || !elem->Attribute("choice")) {
			elem = elem->NextSiblingElement("ZapChoice");
			continue;
		}

		std::string project = elem->Attribute("project");
		std::string zap = elem->Attribute("zap");
		bool choice = elem->BoolAttribute("choice");

		zapChoice[{project, zap}] = choice;

		elem = elem->NextSiblingElement("ZapChoice");
	}

	elem = srcElement->FirstChildElement("Built");
	while (elem) {
		const char* path = elem->Attribute("path");
		const char* set = elem->Attribute("set");
		if (path && set) {
			BuildRecord record;
			record.path = path;
			record.set = set;

			const char* attr = elem->Attribute("folder");
			if (attr)
				record.dataFolder = attr;

			attr = elem->Attribute("preset");
			if (attr)
				record.preset = attr;

			attr = elem->Attribute("time");
			if (attr)
				record.time = attr;

			for (XMLElement* child = elem->FirstChildElement("SetSlider"); child; child = child->NextSiblingElement("SetSlider")) {
				const char* name = child->Attribute("name");
				const char* size = child->Attribute("size");
				if (!name || !size)
					continue;

				BuildRecord::SliderValue slider;
				slider.name = name;
				slider.isSmall = std::strcmp(size, "small") == 0;
				slider.value = child->IntAttribute("value");
				record.sliders.push_back(std::move(slider));
			}

			for (XMLElement* child = elem->FirstChildElement("Zap"); child; child = child->NextSiblingElement("Zap")) {
				const char* name = child->Attribute("name");
				if (name)
					record.zaps.push_back({name, child->BoolAttribute("zapped")});
			}

			buildRecords[record.path].push_back(std::move(record));
		}

		elem = elem->NextSiblingElement("Built");
	}

	return 0;
}

const BuildRecord* BuildSelection::GetBuildRecord(const std::string& outputPath, const std::string& dataFolder) const {
	auto it = buildRecords.find(outputPath);
	if (it == buildRecords.end())
		return nullptr;

	std::string folder = BuildRecord::NormalizeFolder(dataFolder);
	for (auto& record : it->second)
		if (BuildRecord::SameFolder(record.dataFolder, folder))
			return &record;

	return nullptr;
}

bool BuildSelection::HasOutputPath(const std::string& search) {
	for (auto& c : outputChoice)
		if (c.first.compare(search) == 0)
			return true;

	return false;
}

bool BuildSelection::HasZapChoice(const std::string& project, const std::string& zap) {
	for (auto& c : zapChoice)
		if (c.first.first.compare(project) == 0 && c.first.second.compare(zap) == 0)
			return true;

	return false;
}

std::map<std::string, std::string> BuildSelection::GetOutputChoices() {
	return outputChoice;
}

std::string BuildSelection::GetOutputChoice(const std::string& outputPath) {
	if (!HasOutputPath(outputPath))
		return "";

	return outputChoice[outputPath];
}

void BuildSelection::SetOutputChoice(const std::string& outputPath, const std::string& choice) {
	outputChoice[outputPath] = choice;
}

void BuildSelection::RemoveOutputChoice(const std::string& outputPath) {
	outputChoice.erase(outputPath);
}

std::map<ZapChoiceKey, bool> BuildSelection::GetZapChoices() {
	return zapChoice;
}

bool BuildSelection::GetZapChoice(const std::string& project, const std::string& zap) {
	if (!HasZapChoice(project, zap))
		return false;

	return zapChoice[{project, zap}];
}

void BuildSelection::SetZapChoice(const std::string& project, const std::string& zap, bool choice) {
	zapChoice[{project, zap}] = choice;
}

void BuildSelection::RemoveZapChoice(const std::string& project, const std::string& zap) {
	zapChoice.erase({project, zap});
}

BuildSelectionFile::BuildSelectionFile(const std::string& srcFileName) {
	Open(srcFileName);
}

// Clears all data of the file
void BuildSelectionFile::Clear() {
	root = nullptr;
	doc.Clear();
	error = 0;
}

// Creates a new empty document structure
void BuildSelectionFile::New(const std::string& newFileName) {
	if (root)
		return;

	Clear();

	XMLElement* newElement = doc.NewElement("BuildSelection");
	root = doc.InsertEndChild(newElement)->ToElement();

	fileName = newFileName;
	doc.SetUserData(&fileName);

	error = 0;
}

void BuildSelectionFile::Open(const std::string& srcFileName) {
	root = nullptr;
	error = 0;
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error || !fp)
		return;
#else
	fp = fopen(srcFileName.c_str(), "rb");
	if (!fp) {
		error = errno;
		return;
	}
#endif

	error = doc.LoadFile(fp);
	fclose(fp);

	if (error)
		return;

	doc.SetUserData(&fileName);
	root = doc.FirstChildElement("BuildSelection");
	if (!root) {
		error = 2;
		return;
	}

	error = 0;
}

void BuildSelectionFile::Rename(const std::string& newFileName) {
	fileName = newFileName;
}

bool BuildSelectionFile::Save() {
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (error || !fp)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp) {
		error = errno;
		return false;
	}
#endif

	doc.SetBOM(true);

	const tinyxml2::XMLNode* firstChild = doc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		doc.InsertFirstChild(doc.NewDeclaration());

	error = doc.SaveFile(fp);
	fclose(fp);
	if (error)
		return false;

	return true;
}

// Get the build selection in the file
void BuildSelectionFile::Get(BuildSelection& outBuildSel) {
	outBuildSel = root;
}

// Updates or adds output choices in the XML document
int BuildSelectionFile::UpdateOutputChoices(BuildSelection& inBuildSel) {
	for (auto& choice : inBuildSel.GetOutputChoices()) {
		XMLElement* elem = nullptr;

		XMLElement* choiceElem = root->FirstChildElement("OutputChoice");
		while (choiceElem) {
			const char* attrPath = choiceElem->Attribute("path");
			if (attrPath && choice.first.compare(attrPath) == 0) {
				elem = choiceElem;
				break;
			}

			choiceElem = choiceElem->NextSiblingElement("OutputChoice");
		}

		if (!elem) {
			XMLElement* newElement = doc.NewElement("OutputChoice");
			elem = root->InsertEndChild(newElement)->ToElement();
		}

		if (elem) {
			elem->SetAttribute("path", choice.first.c_str());
			elem->SetAttribute("choice", choice.second.c_str());
		}
	}

	return 0;
}

// Updates or adds zap choices in the XML document
int BuildSelectionFile::UpdateZapChoices(BuildSelection& inBuildSel) {
	for (auto& choice : inBuildSel.GetZapChoices()) {
		XMLElement* elem = nullptr;

		XMLElement* choiceElem = root->FirstChildElement("ZapChoice");
		while (choiceElem) {
			const char* attrProject = choiceElem->Attribute("project");
			const char* attrZap = choiceElem->Attribute("zap");
			if (attrProject && attrZap && choice.first.first.compare(attrProject) == 0 && choice.first.second.compare(attrZap) == 0) {
				elem = choiceElem;
				break;
			}

			choiceElem = choiceElem->NextSiblingElement("ZapChoice");
		}

		if (!elem) {
			XMLElement* newElement = doc.NewElement("ZapChoice");
			elem = root->InsertEndChild(newElement)->ToElement();
		}

		if (elem) {
			elem->SetAttribute("project", choice.first.first.c_str());
			elem->SetAttribute("zap", choice.first.second.c_str());
			elem->SetAttribute("choice", choice.second);
		}
	}

	return 0;
}

// Removes a single choice from BuildSelection
void BuildSelectionFile::RemoveOutputChoice(const std::string& path) {
	XMLElement* elem = root->FirstChildElement("OutputChoice");
	while (elem) {
		if (path.compare(elem->Attribute("path")) == 0) {
			root->DeleteChild(elem);
			return;
		}
		elem = elem->NextSiblingElement("OutputChoice");
	}
}

// Removes a single choice from BuildSelection
void BuildSelectionFile::RemoveZapChoice(const std::string& project, const std::string& zap) {
	XMLElement* elem = root->FirstChildElement("ZapChoice");
	while (elem) {
		if (project.compare(elem->Attribute("project")) == 0 && zap.compare(elem->Attribute("zap")) == 0) {
			root->DeleteChild(elem);
			return;
		}
		elem = elem->NextSiblingElement("ZapChoice");
	}
}

// Elements of the build records by output path
static std::unordered_map<std::string, std::vector<XMLElement*>> BuildRecordElements(XMLElement* root) {
	std::unordered_map<std::string, std::vector<XMLElement*>> elements;
	for (XMLElement* elem = root->FirstChildElement("Built"); elem; elem = elem->NextSiblingElement("Built")) {
		const char* path = elem->Attribute("path");
		if (path)
			elements[path].push_back(elem);
	}
	return elements;
}

static XMLElement* FindBuildRecordElement(std::vector<XMLElement*>& elements, const std::string& folder) {
	for (auto elem : elements) {
		const char* attrFolder = elem->Attribute("folder");
		if (BuildRecord::SameFolder(attrFolder ? attrFolder : "", folder))
			return elem;
	}
	return nullptr;
}

void BuildSelectionFile::SetBuildRecords(const std::vector<BuildRecord>& records) {
	if (!root || records.empty())
		return;

	auto elements = BuildRecordElements(root);

	for (auto& record : records) {
		std::string folder = BuildRecord::NormalizeFolder(record.dataFolder);
		auto& pathElements = elements[record.path];

		XMLElement* elem = FindBuildRecordElement(pathElements, folder);
		if (!elem) {
			elem = root->InsertEndChild(doc.NewElement("Built"))->ToElement();
			pathElements.push_back(elem);
		}

		elem->SetAttribute("path", record.path.c_str());
		elem->SetAttribute("folder", folder.c_str());
		elem->SetAttribute("set", record.set.c_str());
		elem->SetAttribute("preset", record.preset.c_str());
		elem->SetAttribute("time", record.time.c_str());

		// Same elements as in preset files, so they can be copied into one
		elem->DeleteChildren();
		for (auto& slider : record.sliders) {
			XMLElement* sliderElem = elem->InsertEndChild(doc.NewElement("SetSlider"))->ToElement();
			sliderElem->SetAttribute("name", slider.name.c_str());
			sliderElem->SetAttribute("size", slider.isSmall ? "small" : "big");
			sliderElem->SetAttribute("value", slider.value);
		}

		for (auto& zap : record.zaps) {
			XMLElement* zapElem = elem->InsertEndChild(doc.NewElement("Zap"))->ToElement();
			zapElem->SetAttribute("name", zap.name.c_str());
			zapElem->SetAttribute("zapped", zap.zapped);
		}
	}
}

void BuildSelectionFile::RemoveBuildRecords(const std::vector<std::string>& paths, const std::string& dataFolder) {
	if (!root || paths.empty())
		return;

	auto elements = BuildRecordElements(root);
	std::string folder = BuildRecord::NormalizeFolder(dataFolder);

	for (auto& path : paths) {
		auto it = elements.find(path);
		if (it == elements.end())
			continue;

		XMLElement* elem = FindBuildRecordElement(it->second, folder);
		if (elem) {
			it->second.erase(std::find(it->second.begin(), it->second.end(), elem));
			root->DeleteChild(elem);
		}
	}
}
