/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderGroup.h"
#include "../utils/PlatformUtil.h"

int SliderSetGroupCollection::LoadGroups(const std::string& basePath) {
	groups.clear();

	wxArrayString files;
	wxDir::GetAllFiles(basePath, &files, "*.xml");

	for (auto &file : files) {
		SliderSetGroupFile groupFile(file.ToUTF8().data());
		std::vector<std::string> groupNames;
		groupFile.GetGroupNames(groupNames);
		for (auto &group : groupNames) {
			SliderSetGroup ssg;
			groupFile.GetGroup(group, ssg);
			if (groups.find(group) != groups.end())
				groups[group].MergeMembers(ssg);
			else
				groups[group] = std::move(ssg);
		}
	}

	return 0;
}

int SliderSetGroupCollection::GetAllGroups(std::set<std::string>& outGroups) {
	outGroups.clear();
	for (auto &g : groups)
		outGroups.insert(g.first);

	return outGroups.size();
}

int SliderSetGroupCollection::GetOutfitGroups(const std::string& outfitName, std::vector<std::string>& outGroups) {
	outGroups.clear();
	for (auto &g : groups)
		if (g.second.HasMember(outfitName))
			outGroups.push_back(g.first);

	return outGroups.size();
}

int SliderSetGroupCollection::GetGroupMembers(const std::string& groupName, std::vector<std::string>& outMembers) {
	auto git = groups.find(groupName);
	if (git == groups.end())
		return 0;

	return git->second.GetMembers(outMembers);
}

int SliderSetGroupCollection::GetGroupMembers(const std::string& groupName, std::unordered_set<std::string>& outMembers) {
	auto git = groups.find(groupName);
	if (git == groups.end())
		return 0;

	return git->second.GetMembers(outMembers);
}

// Combine the source groups members into this one's list. Also merges the source file list.
void SliderSetGroup::MergeMembers(const SliderSetGroup& sourceGroup) {
	for (size_t i = 0; i < sourceGroup.members.size(); i++) {
		members.push_back(sourceGroup.members[i]);
		sourceFiles.push_back(sourceGroup.sourceFiles[i]);
	}
}

int SliderSetGroup::LoadGroup(XMLElement* srcGroupElement) {
	if (srcGroupElement == nullptr)
		return 1;

	name = srcGroupElement->Attribute("name");
	XMLElement* member = srcGroupElement->FirstChildElement("Member");
	while (member) {
		std::string mName = member->Attribute("name");
		members.push_back(mName);
		
		std::string* fileName = static_cast<std::string*>(member->GetDocument()->GetUserData());
		sourceFiles.push_back(*fileName);
		member = member->NextSiblingElement("Member");
	}
	return 0;
}

bool SliderSetGroup::HasMember(const std::string& search) {
	for (auto &m : members)
		if (m.compare(search) == 0)
			return true;

	return false;
}

int SliderSetGroup::GetMembers(std::vector<std::string>& outMembers) {
	outMembers.insert(outMembers.end(), members.begin(), members.end());
	return outMembers.size();
}

int SliderSetGroup::GetMembers(std::unordered_set<std::string>& outMembers) {
	outMembers.insert(members.begin(), members.end());
	return outMembers.size();
}

int SliderSetGroup::AddMembers(const std::vector<std::string>& inMembers) {
	members.insert(members.end(), inMembers.begin(), inMembers.end());
	return members.size();
}

void SliderSetGroup::WriteGroup(XMLElement* groupElement, bool append) {
	if (!append)
		groupElement->DeleteChildren();
	
	for (auto &member : members) {
		XMLElement* newElement = groupElement->GetDocument()->NewElement("Member");
		XMLElement* element = groupElement->InsertEndChild(newElement)->ToElement();
		element->SetAttribute("name", member.c_str());
	}
}

SliderSetGroupFile::SliderSetGroupFile(const std::string& srcFileName) {
	root = nullptr;
	error = 0;
	Open(srcFileName);
}

// Loads the XML document and identifies included group names. On a failure, sets the internal error value.
void SliderSetGroupFile::Open(const std::string& srcFileName) {
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error)
		return;
#else
	fp = fopen(srcFileName.c_str(), "rb");
	if (!fp)
		return;
#endif

	error = doc.LoadFile(fp);
	fclose(fp);

	if (error)
		return;

	doc.SetUserData(&fileName);
	root = doc.FirstChildElement("SliderGroups");
	if (!root) {
		error = 2;
		return;
	}

	XMLElement* element = root->FirstChildElement("Group");
	while (element) {
		groupsInFile[element->Attribute("name")] = element;
		element = element->NextSiblingElement("Group");
	}
	if (groupsInFile.empty()) {
		error = 3;
		return;
	}

	error = 0;
}

// Creates a new empty group document structure, ready to add new groups and members to.
void SliderSetGroupFile::New(const std::string& newFileName) {
	if (root)
		return;

	Clear();

	XMLElement* newElement = doc.NewElement("SliderGroups");
	root = doc.InsertEndChild(newElement)->ToElement();

	fileName = newFileName;
	doc.SetUserData(&fileName);

	error = 0;
}

// Clears all data of the file.
void SliderSetGroupFile::Clear() {
	root = nullptr;
	doc.Clear();
	groupsInFile.clear();
	error = 0;
}

// Changes the internal file name. The XML file isn't saved until the save() function is used.
// Note the original file name is not changed. This method allows you to save a group as a new file without altering the original.
void SliderSetGroupFile::Rename(const std::string& newFileName) {
	fileName = newFileName;
}

// Returns a list of all the groups found in the file.
int SliderSetGroupFile::GetGroupNames(std::vector<std::string>& outGroupNames, bool append, bool unique) {
	if (!append)
		outGroupNames.clear();

	std::unordered_set<std::string> existingNames;
	if (unique)
		existingNames.insert(outGroupNames.begin(), outGroupNames.end());

	for (auto &gn : groupsInFile) {
		if (unique && existingNames.find(gn.first) != existingNames.end())
			continue;
		else if (unique)
			existingNames.insert(gn.first);

		outGroupNames.push_back(gn.first);
	}
	return 0;
}

// Returns true if the group name exists in the file.
bool SliderSetGroupFile::HasGroup(const std::string& queryGroupName) {
	if (groupsInFile.find(queryGroupName) != groupsInFile.end())
		return true;

	return false;
}
	
// Adds all of the groups in the file to the supplied groups vector. Does not clear the vector before doing so.
int SliderSetGroupFile::GetAllGroups(std::vector<SliderSetGroup>& outAppendGroups) {
	int count = 0;
	bool add = true;
	for (auto &g : groupsInFile) {
		add = true;
		for (auto& og : outAppendGroups) {
			if (og.GetName() == g.first) {
				og.LoadGroup(g.second);
				add = false;
				break;
			}
		}
		if (!add)
			continue;

		outAppendGroups.emplace_back(g.second);
		count++;
	}
	return count;
}

// Gets a single group from the XML document based on the name.
int SliderSetGroupFile::GetGroup(const std::string& groupName, SliderSetGroup& outSliderSetGroup) {
	if (groupsInFile.find(groupName) != groupsInFile.end())
		outSliderSetGroup.LoadGroup(groupsInFile[groupName]);
	else
		return 1;

	return 0;
}

// Updates a slider set group in the XML document with the provided information.
// If the group does not already exist in the file (based on name) the group is added.
int SliderSetGroupFile::UpdateGroup(SliderSetGroup& inGroup) {
	if (groupsInFile.find(inGroup.GetName()) != groupsInFile.end()) {
		inGroup.WriteGroup(groupsInFile[inGroup.GetName()]);
	}
	else {
		XMLElement* newElement = doc.NewElement("Group");
		XMLElement* element = root->InsertEndChild(newElement)->ToElement();
		element->SetAttribute("name", inGroup.GetName().c_str());
		inGroup.WriteGroup(element);
		groupsInFile[inGroup.GetName()] = element;
	}
	return 0;
}

// Writes the XML file using the internal fileName (use Rename() to change the name).
bool SliderSetGroupFile::Save() {
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (error)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp)
		return false;
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
