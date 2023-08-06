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

	return 0;
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
		return "";

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
	BuildSelection bsFile = root;

	for (auto& choice : inBuildSel.GetOutputChoices()) {
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

// Updates or adds zap choices in the XML document
int BuildSelectionFile::UpdateZapChoices(BuildSelection& inBuildSel) {
	BuildSelection bsFile = root;

	for (auto& choice : inBuildSel.GetZapChoices()) {
		XMLElement* elem = nullptr;

		if (bsFile.HasZapChoice(choice.first.first, choice.first.second)) {
			XMLElement* choiceElem = root->FirstChildElement("ZapChoice");
			while (choiceElem) {
				if (choiceElem->Attribute("project") == choice.first.first && choiceElem->Attribute("zap") == choice.first.second) {
					elem = choiceElem;
					break;
				}

				choiceElem = choiceElem->NextSiblingElement("ZapChoice");
			}
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
