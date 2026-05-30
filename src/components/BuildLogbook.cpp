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
				val.value = sliderElem->FloatAttribute("value") / 100.0f; // Scale percentage value back to 0-1 range
				entry.sliders.push_back(val);
			}
			sliderElem = sliderElem->NextSiblingElement("SetSlider");
		}

		entries[entry.path] = entry; // Store entry mapped by its output path
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

bool BuildLogbook::LoadFromFile(const std::string& path) {
	fileName = path;
	entries.clear();

	XMLDocument doc;
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(path);
	if (_wfopen_s(&fp, winFileName.c_str(), L"rb") != 0 || !fp)
		return false;
#else
	fp = fopen(path.c_str(), "rb");
	if (!fp)
		return false;
#endif

	if (doc.LoadFile(fp) != 0) {
		fclose(fp);
		return false;
	}
	fclose(fp);

	XMLElement* root = doc.FirstChildElement("BuildLogbook");
	if (!root)
		return false;

	LoadBuildLogbook(root);
	return true;
}

bool BuildLogbook::SaveToFile() {
	if (fileName.empty()) return false;

	XMLDocument doc;
	XMLElement* root = nullptr;
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	if (_wfopen_s(&fp, winFileName.c_str(), L"rb") == 0 && fp) {
		doc.LoadFile(fp);
		fclose(fp);
		root = doc.FirstChildElement("BuildLogbook");
	}
#else
	fp = fopen(fileName.c_str(), "rb");
	if (fp) {
		doc.LoadFile(fp);
		fclose(fp);
		root = doc.FirstChildElement("BuildLogbook");
	}
#endif

	if (!root) {
		doc.Clear();
		XMLElement* newElement = doc.NewElement("BuildLogbook");
		root = doc.InsertEndChild(newElement)->ToElement();
	}

	// Remove Mesh elements that are no longer in our entries map
	XMLElement* meshElem = root->FirstChildElement("Mesh");
	while (meshElem) {
		XMLElement* nextElem = meshElem->NextSiblingElement("Mesh");
		const char* attrPath = meshElem->Attribute("path");
		if (attrPath && !HasEntry(attrPath)) {
			root->DeleteChild(meshElem);
		}
		meshElem = nextElem;
	}

	// Update existing elements and add new ones
	for (const auto& pair : GetEntries()) {
		const BuildLogbookEntry& entry = pair.second;
		XMLElement* elem = nullptr;

		// Try to find an existing <Mesh> element with the matching path
		meshElem = root->FirstChildElement("Mesh");
		while (meshElem) {
			const char* attrPath = meshElem->Attribute("path");
			if (attrPath && entry.path.compare(attrPath) == 0) {
				elem = meshElem;
				break;
			}
			meshElem = meshElem->NextSiblingElement("Mesh");
		}

		// If no matching <Mesh> element was found, create a new one and append it to the root
		if (!elem) {
			XMLElement* newElement = doc.NewElement("Mesh");
			elem = root->InsertEndChild(newElement)->ToElement();
		}

		if (elem) {
			// Update the attributes of the <Mesh> element
			elem->SetAttribute("path", entry.path.c_str());
			if (!entry.set.empty())
				elem->SetAttribute("set", entry.set.c_str());
			if (!entry.preset.empty())
				elem->SetAttribute("preset", entry.preset.c_str());
				
			elem->DeleteChildren(); // Clear existing sliders to update them
			
			// Recreate the <SetSlider> child elements based on the current data
			for (const auto& slider : entry.sliders) {
				XMLElement* sliderElem = doc.NewElement("SetSlider");
				sliderElem->SetAttribute("name", slider.name.c_str());
				sliderElem->SetAttribute("size", slider.size.c_str());
				sliderElem->SetAttribute("value", (int)(slider.value * 100.0f)); // Convert 0-1 scale back to percentage for XML
				elem->InsertEndChild(sliderElem);
			}
		}
	}

#ifdef _WINDOWS
	if (_wfopen_s(&fp, winFileName.c_str(), L"w") != 0 || !fp)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp)
		return false;
#endif

	// Enable Byte Order Mark for UTF-8 when saving
	doc.SetBOM(true);

	// Ensure there is an XML declaration
	const tinyxml2::XMLNode* firstChild = doc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		doc.InsertFirstChild(doc.NewDeclaration());

	// Save the XML document to the file pointer
	int error = doc.SaveFile(fp);
	fclose(fp);

	return error == 0;
}
