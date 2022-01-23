/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../utils/PlatformUtil.h"

bool PoseData::LoadElement(XMLElement* srcElement) {
	if (srcElement == nullptr)
		return false;

	name = srcElement->Attribute("name");

	XMLElement* boneElement = srcElement->FirstChildElement("Bone");
	while (boneElement) {
		PoseBoneData poseBoneData{};
		poseBoneData.name = boneElement->Attribute("name");
		poseBoneData.rotation.x = boneElement->FloatAttribute("rotX");
		poseBoneData.rotation.y = boneElement->FloatAttribute("rotY");
		poseBoneData.rotation.z = boneElement->FloatAttribute("rotZ");
		poseBoneData.translation.x = boneElement->FloatAttribute("transX");
		poseBoneData.translation.y = boneElement->FloatAttribute("transY");
		poseBoneData.translation.z = boneElement->FloatAttribute("transZ");
		boneData.push_back(poseBoneData);

		boneElement = boneElement->NextSiblingElement("Bone");
	}

	return true;
}

void PoseData::WriteElement(XMLElement* element, bool append) const {
	if (!append)
		element->DeleteChildren();

	for (auto& bone : boneData) {
		XMLElement* newElement = element->GetDocument()->NewElement("Bone");
		newElement = element->InsertEndChild(newElement)->ToElement();
		newElement->SetAttribute("name", bone.name.c_str());
		newElement->SetAttribute("rotX", bone.rotation.x);
		newElement->SetAttribute("rotY", bone.rotation.y);
		newElement->SetAttribute("rotZ", bone.rotation.z);
		newElement->SetAttribute("transX", bone.translation.x);
		newElement->SetAttribute("transY", bone.translation.y);
		newElement->SetAttribute("transZ", bone.translation.z);
	}
}

int PoseDataCollection::LoadData(const std::string& basePath) {
	poseData.clear();

	wxArrayString files;
	wxDir::GetAllFiles(basePath, &files, "*.xml");

	for (auto& file : files) {
		PoseDataFile poseDataFile(file.ToUTF8().data());

		std::vector<PoseData> poseDataEntries;
		poseDataFile.GetData(poseDataEntries);

		for (auto& pd : poseDataEntries)
			poseData.push_back(pd);
	}

	return 0;
}


PoseDataFile::PoseDataFile(const std::string& srcFileName) {
	root = nullptr;
	error = 0;
	Open(srcFileName);
}

void PoseDataFile::Open(const std::string& srcFileName) {
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error)
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
	root = doc.FirstChildElement("PoseData");
	if (!root) {
		error = 2;
		return;
	}

	error = 0;
}

void PoseDataFile::New(const std::string& newFileName) {
	if (root)
		return;

	Clear();

	XMLElement* newElement = doc.NewElement("PoseData");
	root = doc.InsertEndChild(newElement)->ToElement();

	fileName = newFileName;
	doc.SetUserData(&fileName);

	error = 0;
}

void PoseDataFile::Clear() {
	doc.Clear();
	root = nullptr;
	error = 0;
}

void PoseDataFile::Rename(const std::string& newFileName) {
	fileName = newFileName;
}

int PoseDataFile::SetData(const std::vector<PoseData>& data) {
	root->DeleteChildren();

	for (auto& pd : data) {
		XMLElement* newElement = doc.NewElement("Pose");
		XMLElement* element = root->InsertEndChild(newElement)->ToElement();
		element->SetAttribute("name", pd.name.c_str());
		pd.WriteElement(element);
	}

	return 0;
}

bool PoseDataFile::Save() {
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (error)
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

int PoseDataFile::GetData(std::vector<PoseData>& outData) {
	XMLElement* poseElement = root->FirstChildElement("Pose");
	while (poseElement) {
		PoseData poseData{};
		poseData.LoadElement(poseElement);
		outData.push_back(poseData);

		poseElement = poseElement->NextSiblingElement("Pose");
	}

	return 0;
}
