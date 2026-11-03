/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SliderSet.h"

#include <atomic>
#include <set>
#include <string>
#include <thread>
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
	std::vector<std::string> filterNames; // Lower case names the sets were filtered by, empty if every set was loaded

	// Calls 'use' with each set other than 'exceptFile'/'exceptSet' and the name of each of its data in the .osd file
	template<typename Use>
	void ForDataInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet, Use use) const;

public:
	SliderSetIndex() {}
	explicit SliderSetIndex(const std::string& inProjectPath);
	// Only the sets of the project files that mention one of the file names, which are all that
	// DataUsedInFile and SetsUsingNif can find for those files. Much faster than loading every set.
	SliderSetIndex(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames);

	// With 'mentionedFileNames', only the project files whose text has one of them are loaded.
	// Setting 'cancel' stops loading, which leaves the index incomplete.
	void Load(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames = {}, const std::atomic<bool>* cancel = nullptr);

	const std::string& GetProjectPath() const { return projectPath; }
	const std::vector<SliderSetIndexEntry>& Entries() const { return entries; }

	// True if every set that mentions the file names was loaded, so the queries for them are complete
	bool Covers(const std::vector<std::string>& fileNames) const;

	// First set with the name, which is the one BodySlide lists.
	const SliderSetIndexEntry* Find(const std::string& setName) const;

	// Names of the data in an .osd file that sets other than 'exceptFile'/'exceptSet' use.
	std::set<std::string> DataUsedInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const;

	// Sets other than 'exceptFile'/'exceptSet' that have data in the .osd file.
	std::vector<const SliderSetIndexEntry*> SetsUsingDataFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const;

	// Sets other than 'exceptFile'/'exceptSet' that use the NIF file as their source file.
	std::vector<const SliderSetIndexEntry*> SetsUsingNif(const std::string& nifFilePath, const std::string& exceptFile, const std::string& exceptSet) const;

	// True if both paths name the same existing file.
	static bool SameFile(const std::string& fileName1, const std::string& fileName2);
};

// Loads an index of the sets that mention some file names on a thread of its own
class SliderSetIndexTask {
	std::vector<std::string> fileNames;
	SliderSetIndex index;
	std::atomic<bool> cancel{false};
	std::atomic<bool> done{false};
	std::thread thread;

public:
	SliderSetIndexTask(const std::string& projectPath, const std::vector<std::string>& inFileNames);
	~SliderSetIndexTask();

	SliderSetIndexTask(const SliderSetIndexTask&) = delete;
	SliderSetIndexTask& operator=(const SliderSetIndexTask&) = delete;

	const std::vector<std::string>& FileNames() const { return fileNames; }

	// True once the index is complete
	bool IsDone() const { return done; }

	// Only to be used once IsDone() is true
	const SliderSetIndex& Index() const { return index; }

	// Stops loading and waits for the thread, the index is only complete if it was done before
	void Cancel();
};
