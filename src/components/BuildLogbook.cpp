/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BuildLogbook.h"
#include "../utils/PlatformUtil.h"

std::string BuildLogbookEntry::GetSummary() {
	std::string summary = "Outfit: " + set + "\nPreset: " + preset + "\nSlider modifications: " + (sliders.size() > 0 ? "Yes" : "No");
	return summary;
}

int BuildLogbook::LoadBuildLogbook(XMLElement* srcElement) {
	if (srcElement == nullptr)
		return 1;

	XMLElement* elem = srcElement->FirstChildElement("Mesh");
	while (elem) {
		if (!elem->Attribute("path")) {
			elem = elem->NextSiblingElement("Mesh");
			continue;
		}

		BuildLogbookEntry entry;
		entry.path = elem->Attribute("path");
		
		if (elem->Attribute("set"))
			entry.set = elem->Attribute("set");
			
		if (elem->Attribute("preset"))
			entry.preset = elem->Attribute("preset");

		XMLElement* sliderElem = elem->FirstChildElement("SetSlider");
		while (sliderElem) {
			if (sliderElem->Attribute("name") && sliderElem->Attribute("size") && sliderElem->Attribute("value")) {
				BuildLogbookEntry::SliderValue val;
				val.name = sliderElem->Attribute("name");
				val.size = sliderElem->Attribute("size");
				val.value = sliderElem->FloatAttribute("value") / 100.0f;
				entry.sliders.push_back(val);
			}
			sliderElem = sliderElem->NextSiblingElement("SetSlider");
		}

		entries[entry.path] = entry;
		elem = elem->NextSiblingElement("Mesh");
	}

	return 0;
}

void BuildLogbook::AddEntry(const BuildLogbookEntry& entry) {
	entries[entry.path] = entry;
}

void BuildLogbook::RemoveEntry(const std::string& path) {
	entries.erase(path);
}

bool BuildLogbook::HasEntry(const std::string& path) {
	return entries.find(path) != entries.end();
}

BuildLogbookEntry BuildLogbook::GetEntry(const std::string& path) {
	if (HasEntry(path))
		return entries[path];
	return BuildLogbookEntry();
}

const std::map<std::string, BuildLogbookEntry>& BuildLogbook::GetEntries() const {
	return entries;
}

BuildLogbookFile::BuildLogbookFile(const std::string& srcFileName) {
	Open(srcFileName);
}

void BuildLogbookFile::Clear() {
	root = nullptr;
	doc.Clear();
	error = 0;
}

void BuildLogbookFile::New(const std::string& newFileName) {
	if (root)
		return;

	Clear();

	XMLElement* newElement = doc.NewElement("BuildLogbook");
	root = doc.InsertEndChild(newElement)->ToElement();

	fileName = newFileName;
	doc.SetUserData(&fileName);

	error = 0;
}

void BuildLogbookFile::Open(const std::string& srcFileName) {
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
	root = doc.FirstChildElement("BuildLogbook");
	if (!root) {
		error = 2;
		return;
	}

	error = 0;
}

void BuildLogbookFile::Rename(const std::string& newFileName) {
	fileName = newFileName;
}

bool BuildLogbookFile::Save() {
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

void BuildLogbookFile::Get(BuildLogbook& outLogbook) {
	outLogbook = root;
}

int BuildLogbookFile::UpdateEntries(const BuildLogbook& inLogbook) {
	for (const auto& pair : inLogbook.GetEntries()) {
		const BuildLogbookEntry& entry = pair.second;
		XMLElement* elem = nullptr;

		XMLElement* meshElem = root->FirstChildElement("Mesh");
		while (meshElem) {
			const char* attrPath = meshElem->Attribute("path");
			if (attrPath && entry.path.compare(attrPath) == 0) {
				elem = meshElem;
				break;
			}
			meshElem = meshElem->NextSiblingElement("Mesh");
		}

		if (!elem) {
			XMLElement* newElement = doc.NewElement("Mesh");
			elem = root->InsertEndChild(newElement)->ToElement();
		}

		if (elem) {
			elem->SetAttribute("path", entry.path.c_str());
			if (!entry.set.empty())
				elem->SetAttribute("set", entry.set.c_str());
			if (!entry.preset.empty())
				elem->SetAttribute("preset", entry.preset.c_str());
				
			elem->DeleteChildren(); // Clear existing sliders to update them
			
			for (const auto& slider : entry.sliders) {
				XMLElement* sliderElem = doc.NewElement("SetSlider");
				sliderElem->SetAttribute("name", slider.name.c_str());
				sliderElem->SetAttribute("size", slider.size.c_str());
				sliderElem->SetAttribute("value", (int)(slider.value * 100.0f));
				elem->InsertEndChild(sliderElem);
			}
		}
	}

	return 0;
}

void BuildLogbookFile::RemoveEntry(const std::string& path) {
	XMLElement* elem = root->FirstChildElement("Mesh");
	while (elem) {
		if (path.compare(elem->Attribute("path")) == 0) {
			root->DeleteChild(elem);
			return;
		}
		elem = elem->NextSiblingElement("Mesh");
	}
}
