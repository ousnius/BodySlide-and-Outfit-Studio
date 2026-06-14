/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "Anim.h"
#include "../utils/PlatformUtil.h"

#include <wx/filename.h>

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
		poseBoneData.scale = boneElement->FloatAttribute("scale", 1.0f);
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
		if (bone.scale != 1.0f)
			newElement->SetAttribute("scale", bone.scale);
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

PoseData* PoseDataCollection::AddPose(PoseData pose) {
	poseData.push_back(std::move(pose));
	return &poseData.back();
}

PoseFileFormat PoseDataCollection::GetPoseFileFormat(const std::string& filePath) {
	wxFileName fn(wxString::FromUTF8(filePath.c_str()));
	wxString ext = fn.GetExt().Lower();
	if (ext == "hkx")
		return PoseFileFormat::Hkx;
	if (ext == "json")
		return PoseFileFormat::Json;
	if (ext == "yaml" || ext == "yml")
		return PoseFileFormat::Yaml;
	return PoseFileFormat::Unknown;
}

std::string PoseDataCollection::SanitizeFileStem(const std::string& name) {
	wxString wxName = wxString::FromUTF8(name.c_str());
	wxName.Trim(true).Trim(false);
	if (wxName.empty() || wxName == "<New>")
		wxName = "pose";

	static const char* invalidChars = "<>:\"/\\|?*";
	for (const char* ch = invalidChars; *ch; ++ch)
		wxName.Replace(wxString::Format("%c", *ch), "_");

	while (!wxName.empty() && (wxName.Last() == '.' || wxName.Last() == ' '))
		wxName.RemoveLast();

	if (wxName.empty())
		wxName = "pose";

	return std::string(wxName.ToUTF8().data());
}

void PoseDataCollection::CaptureCurrentPose(const std::string& poseName, bool absoluteLocal, PoseData& outPose) {
	outPose.name = poseName;
	outPose.absoluteLocal = absoluteLocal;
	outPose.boneData.clear();

	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const auto& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		PoseBoneData poseBoneData{};
		poseBoneData.name = bone->boneName;

		if (absoluteLocal) {
			nifly::MatTransform poseDelta;
			poseDelta.translation = bone->poseTranVec;
			poseDelta.rotation = nifly::RotVecToMat(bone->poseRotVec);
			poseDelta.scale = (bone->poseScale != 0.0f) ? bone->poseScale : 1.0f;

			nifly::MatTransform localTransform = bone->xformToParent.ComposeTransforms(poseDelta);
			poseBoneData.rotation = nifly::RotMatToVec(localTransform.rotation);
			poseBoneData.translation = localTransform.translation;
			poseBoneData.scale = (localTransform.scale != 0.0f) ? localTransform.scale : 1.0f;
		}
		else {
			poseBoneData.rotation = bone->poseRotVec;
			poseBoneData.translation = bone->poseTranVec;
			poseBoneData.scale = (bone->poseScale != 0.0f) ? bone->poseScale : 1.0f;
		}

		outPose.boneData.push_back(std::move(poseBoneData));
	}
}

void PoseData::ApplyToSkeleton() const {
	using namespace nifly;

	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const auto& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		auto it = std::find_if(boneData.begin(), boneData.end(),
			[&boneName](const PoseBoneData& bd) { return bd.name == boneName; });

		if (it != boneData.end()) {
			if (absoluteLocal) {
				MatTransform frameLocal;
				frameLocal.translation = it->translation;
				frameLocal.rotation = RotVecToMat(it->rotation);
				frameLocal.scale = it->scale;

				MatTransform delta = bone->xformToParent.InverseTransform().ComposeTransforms(frameLocal);
				bone->poseRotVec = RotMatToVec(delta.rotation);
				bone->poseTranVec = delta.translation;
				bone->poseScale = (delta.scale != 0.0f) ? delta.scale : 1.0f;
			}
			else {
				bone->poseRotVec = it->rotation;
				bone->poseTranVec = it->translation;
				bone->poseScale = it->scale;
			}
		}
		else {
			bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
		}

		bone->UpdatePoseTransform();
	}
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
