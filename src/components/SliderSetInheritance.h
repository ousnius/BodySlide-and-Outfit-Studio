/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SliderSet.h"
#include "SliderSetIndex.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Loads slider sets together with the sliders of their base projects, through any number of
// variants and add-ons of variants.
class SliderSetResolver {
public:
	// Base projects can name each other, but not deeper than this
	static constexpr size_t MaxDepth = 8;

	explicit SliderSetResolver(const std::string& inProjectPath);

	// The project path whose "SliderSets" folder holds the file, or the configured project path.
	static std::string ProjectPathOfFile(const std::string& fileName);

	// Loads a set of the file with everything that it inherits. Returns non-zero if the set doesn't exist.
	int Resolve(SliderSetFile& file, const std::string& setName, SliderSet& outSet);

	// The file holding the base project of a set, or nullptr if it can't be found.
	// 'fromFile' is the file of the set, which often holds its base project too.
	SliderSetFile* FindBaseProject(const SliderSet& set, SliderSetFile& fromFile, std::string& outSetName);

private:
	std::string projectPath;
	std::map<std::string, std::unique_ptr<SliderSetFile>> openFiles;
	std::unique_ptr<SliderSetIndex> index;
	std::vector<std::pair<std::string, std::string>> resolving; // File and set name of the sets being resolved

	SliderSetFile* OpenFile(const std::string& fileName);
	const SliderSetIndex& Index();
	int Resolve(SliderSetFile& file, const std::string& setName, SliderSet& outSet, size_t depth);
};
