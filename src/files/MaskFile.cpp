/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MaskFile.h"

#include "../utils/PlatformUtil.h"

#include <tinyxml2.h>

using namespace tinyxml2;

static void LoadMaskDataElement(XMLElement* maskDataElem, MaskEntry& entry) {
	const char* nameAttr = maskDataElem->Attribute("name");
	entry.name = nameAttr ? nameAttr : "";

	XMLElement* shapeElem = maskDataElem->FirstChildElement("Shape");
	while (shapeElem) {
		MaskShapeData shapeData;

		const char* sn = shapeElem->Attribute("name");
		if (sn)
			shapeData.name = sn;

		shapeData.vertexCount = shapeElem->IntAttribute("vertexCount", 0);

		XMLElement* vertElem = shapeElem->FirstChildElement("V");
		while (vertElem) {
			int idx = vertElem->IntAttribute("i", -1);
			float val = vertElem->FloatAttribute("m", 0.0f);
			if (idx >= 0)
				shapeData.mask[static_cast<uint16_t>(idx)] = val;
			vertElem = vertElem->NextSiblingElement("V");
		}

		entry.shapes.push_back(std::move(shapeData));
		shapeElem = shapeElem->NextSiblingElement("Shape");
	}
}

int MaskFile::Load(const std::string& fileName) {
	XMLDocument doc;
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	int err = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (err || !fp)
		return 1;
#else
	fp = fopen(fileName.c_str(), "rb");
	if (!fp)
		return 1;
#endif

	XMLError error = doc.LoadFile(fp);
	fclose(fp);

	if (error != XML_SUCCESS)
		return 2;

	entries.clear();

	XMLElement* root = doc.FirstChildElement("MaskFile");
	if (!root)
		return 3;

	version = root->IntAttribute("version", 1);

	XMLElement* maskDataElem = root->FirstChildElement("MaskData");
	while (maskDataElem) {
		MaskEntry entry;
		LoadMaskDataElement(maskDataElem, entry);
		entries.push_back(std::move(entry));
		maskDataElem = maskDataElem->NextSiblingElement("MaskData");
	}

	return 0;
}

int MaskFile::Save(const std::string& fileName) const {
	XMLDocument doc;
	doc.InsertFirstChild(doc.NewDeclaration());

	XMLElement* root = doc.NewElement("MaskFile");
	root->SetAttribute("version", version);
	doc.InsertEndChild(root);

	for (const auto& entry : entries) {
		XMLElement* maskDataElem = doc.NewElement("MaskData");
		maskDataElem->SetAttribute("name", entry.name.c_str());
		root->InsertEndChild(maskDataElem);

		for (const auto& shapeData : entry.shapes) {
			XMLElement* shapeElem = doc.NewElement("Shape");
			shapeElem->SetAttribute("name", shapeData.name.c_str());
			shapeElem->SetAttribute("vertexCount", shapeData.vertexCount);
			maskDataElem->InsertEndChild(shapeElem);

			for (const auto& [vertIndex, value] : shapeData.mask) {
				XMLElement* vertElem = doc.NewElement("V");
				vertElem->SetAttribute("i", vertIndex);
				vertElem->SetAttribute("m", std::to_string(value).c_str());
				shapeElem->InsertEndChild(vertElem);
			}
		}
	}

	FILE* fp = nullptr;
#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	int err = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (err || !fp)
		return 1;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp)
		return 1;
#endif

	XMLError error = doc.SaveFile(fp);
	fclose(fp);

	return (error != XML_SUCCESS) ? 2 : 0;
}

std::string MaskFile::GetFirstName() const {
	if (entries.empty())
		return {};
	return entries.front().name;
}

const MaskEntry* MaskFile::FindEntry(const std::string& entryName) const {
	for (const auto& entry : entries) {
		if (entry.name == entryName)
			return &entry;
	}
	return nullptr;
}

void MaskEntry::SetFromMaskData(const std::map<std::string, std::unordered_map<uint16_t, float>>& maskData,
	const std::map<std::string, int>& vertexCounts) {
	shapes.clear();
	for (const auto& [shapeName, mask] : maskData) {
		MaskShapeData shapeData;
		shapeData.name = shapeName;
		shapeData.mask = mask;

		auto it = vertexCounts.find(shapeName);
		if (it != vertexCounts.end())
			shapeData.vertexCount = it->second;

		shapes.push_back(std::move(shapeData));
	}
}

std::map<std::string, std::unordered_map<uint16_t, float>> MaskEntry::ToMaskData() const {
	std::map<std::string, std::unordered_map<uint16_t, float>> result;
	for (const auto& shapeData : shapes)
		result[shapeData.name] = shapeData.mask;
	return result;
}

const MaskShapeData* MaskEntry::FindMatchingMask(const std::string& shapeName, int vertexCount) const {
	// First try: match by shape name
	for (const auto& sd : shapes) {
		if (sd.name == shapeName)
			return &sd;
	}

	// Fallback: match by vertex count
	for (const auto& sd : shapes) {
		if (sd.vertexCount > 0 && sd.vertexCount == vertexCount)
			return &sd;
	}

	return nullptr;
}
