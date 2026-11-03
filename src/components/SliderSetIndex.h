/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SliderSet.h"

#include <set>
#include <string>
#include <vector>

struct SliderSetIndexEntry {
	std::string fileName; // Full path of the project file
	std::string setName;
	SliderSet set;		  // As stored in the file, without anything from a base project
};

// All slider sets in the "SliderSets" folder of a project path and its subfolders, in the
// order BodySlide loads them.
class SliderSetIndex {
	std::string projectPath;
	std::vector<SliderSetIndexEntry> entries;

public:
	SliderSetIndex() {}
	explicit SliderSetIndex(const std::string& inProjectPath);
	// Only the sets of the project files that mention one of the file names, which are all that
	// DataUsedInFile and SetsUsingNif can find for those files. Much faster than loading every set.
	SliderSetIndex(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames);

	// With 'mentionedFileNames', only the project files whose text has one of them are loaded
	void Load(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames = {});

	const std::string& GetProjectPath() const { return projectPath; }
	const std::vector<SliderSetIndexEntry>& Entries() const { return entries; }

	// First set with the name, which is the one BodySlide lists.
	const SliderSetIndexEntry* Find(const std::string& setName) const;

	// Names of the data in an .osd file that sets other than 'exceptFile'/'exceptSet' use.
	std::set<std::string> DataUsedInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const;

	// Sets other than 'exceptFile'/'exceptSet' that use the NIF file as their source file.
	std::vector<const SliderSetIndexEntry*> SetsUsingNif(const std::string& nifFilePath, const std::string& exceptFile, const std::string& exceptSet) const;

	// True if both paths name the same existing file.
	static bool SameFile(const std::string& fileName1, const std::string& fileName2);
};
