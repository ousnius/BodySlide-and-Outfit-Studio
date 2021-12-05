/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BuildSelection.h"
#include "../utils/PlatformUtil.h"


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

		std::string* fileName = static_cast<std::string*>(elem->GetDocument()->GetUserData());
		sourceFiles.push_back(*fileName);

		elem = elem->NextSiblingElement("OutputChoice");
	}

	return 0;
}

bool BuildSelection::HasOutputPath(const std::string& search) {
	for (auto &c : outputChoice)
		if (c.first.compare(search) == 0)
			return true;

	return false;
}

std::unordered_map<std::string, std::string> BuildSelection::GetOutputChoices() {
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

BuildSelectionFile::BuildSelectionFile(const std::string& srcFileName) {
	root = nullptr;
	error = 0;
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
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error)
		return;
#else
	fp = fopen(srcFileName.c_str(), "rb");
	if (!fp){
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
	if (error)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp){
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

// Updates or adds a build selection in the XML document
int BuildSelectionFile::Update(BuildSelection& inBuildSel) {
	BuildSelection bsFile = root;

	for (auto &choice : inBuildSel.GetOutputChoices()) {
		XMLElement* elem = nullptr;

		if (bsFile.HasOutputPath(choice.first)) {
			XMLElement* choiceElem = root->FirstChildElement("OutputChoice");
			while (choiceElem) {
				if (choiceElem->Attribute("path") == choice.first) {
					elem = choiceElem;
					break;
				}

				choiceElem = choiceElem->NextSiblingElement("OutputChoice");
			}
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

// Removes a single choice from BuildSelection 
void BuildSelectionFile::Remove(std::string& path) {
	BuildSelection bsFile = root;
	XMLElement* elem = root->FirstChildElement("OutputChoice");
	while (elem) {
		if (0 == path.compare(elem->Attribute("path"))) {
			root->DeleteChild(elem);
				return;
		}
		elem = elem->NextSiblingElement("OutputChoice");
	}
}

