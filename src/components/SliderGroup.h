/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../TinyXML-2/tinyxml2.h"

#include <wx/dir.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>

using namespace tinyxml2;

class SliderSetGroup {
	std::string name;
	std::vector<std::string> members;
	std::vector<std::string> sourceFiles;
	bool isValid;

public:
	SliderSetGroup() :isValid(false) { }
	SliderSetGroup(XMLElement * srcGroupElement) {
		if (LoadGroup(srcGroupElement))
			isValid = false;
		isValid = true;
	}

	std::string GetName() {
		return name;
	}
	void SetName(const std::string& inName) {
		name = inName;
	}

	bool HasMember(const std::string& search);
	int GetMembers(std::vector<std::string>& outMembers);
	int AppendMembers(std::vector<std::string>& outMembers);
	int GetMembers(std::unordered_set<std::string>& outMembers);
	int AppendMembers(std::unordered_set<std::string>& outMembers);
	int AddMembers(const std::vector<std::string>& inMembers);

	// Combine the source groups members into this one's list. Also merges the source file list.
	void MergeMembers(const SliderSetGroup& sourceGroup);

	int LoadGroup(XMLElement* srcGroupElement);
	void WriteGroup(XMLElement* groupElement, bool append = false);
};


class SliderSetGroupCollection {
	std::map<std::string, SliderSetGroup> groups;

public:
	// Loads all groups in the specified folder.
	int LoadGroups(const std::string& basePath);

	int GetAllGroups(std::set<std::string>& outGroups);
	int GetOutfitGroups(const std::string& outfitName, std::vector<std::string>& outGroups);

	int GetGroupMembers(const std::string& groupName, std::vector<std::string>& outMembers, bool append = true);
	int GetGroupMembers(const std::string& groupName, std::unordered_set<std::string>& outMembers, bool append = true);
};


class SliderSetGroupFile {
	XMLDocument doc;
	XMLElement* root;
	std::map<std::string, XMLElement*> groupsInFile;
	int error;

public:
	std::string fileName;
	SliderSetGroupFile() :error(0), root(nullptr) { }
	SliderSetGroupFile(const std::string& srcFileName);
	~SliderSetGroupFile() { }

	bool fail() {
		return error != 0;
	}
	int GetError() {
		return error;
	}

	// Loads the XML document and identifies included group names. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Creates a new empty group document structure, ready to add new groups and members to.
	void New(const std::string& newFileName);

	// Clears all data of the file.
	void Clear();

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save a group as a new file without altering the original.
	void Rename(const std::string& newFileName);

	// Returns a list of all the groups found in the file.
	int GetGroupNames(std::vector<std::string>& outGroupNames, bool append = true, bool unique = false);

	// Returns true if the group name exists in the file.
	bool HasGroup(const std::string& queryGroupName);

	// Adds all of the groups in the file to the supplied groups vector. Does not clear the vector before doing so.
	int GetAllGroups(std::vector<SliderSetGroup>& outAppendGroups);

	// Gets a single group from the XML document based on the name.
	int GetGroup(const std::string& groupName, SliderSetGroup& outSliderSetGroup);

	// Updates a slider set group in the xml document with the provided information.
	// If the group does not already exist in the file (based on name) the group is added.
	int UpdateGroup(SliderSetGroup& inGroup);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	int Save();
};
