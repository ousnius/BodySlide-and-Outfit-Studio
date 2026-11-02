/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderSetIndex.h"
#include "SliderDataFileUtil.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include <filesystem>
#include <map>
#include <wx/dir.h>

SliderSetIndex::SliderSetIndex(const std::string& inProjectPath) {
	Load(inProjectPath);
}

void SliderSetIndex::Load(const std::string& inProjectPath) {
	projectPath = inProjectPath;
	entries.clear();

	wxString sliderSetsPath = wxString::FromUTF8(projectPath + PathSepStr + "SliderSets");
	if (!wxDir::Exists(sliderSetsPath))
		return;

	wxArrayString files;
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.osp");
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.xml");

	const std::string shapeDataPath = projectPath + PathSepStr + "ShapeData";

	for (auto& file : files) {
		std::string fileName{file.ToUTF8()};

		SliderSetFile sliderSetFile(fileName);
		if (sliderSetFile.fail())
			continue;

		std::vector<std::string> setNames;
		sliderSetFile.GetSetNamesUnsorted(setNames, false);

		for (auto& setName : setNames) {
			SliderSetIndexEntry entry;
			if (sliderSetFile.GetSet(setName, entry.set, true, "", false) != 0)
				continue;

			entry.fileName = fileName;
			entry.setName = setName;
			entry.set.SetBaseDataPath(shapeDataPath);
			entries.push_back(std::move(entry));
		}
	}
}

const SliderSetIndexEntry* SliderSetIndex::Find(const std::string& setName) const {
	for (auto& entry : entries)
		if (entry.setName == setName)
			return &entry;

	return nullptr;
}

std::set<std::string> SliderSetIndex::DataUsedInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const {
	std::set<std::string> dataNames;

	std::string osdName = std::filesystem::u8path(osdFilePath).filename().u8string();

	// Many sets name the same files, so paths are compared as text and each one is looked up once
	auto normalize = [](const std::string& path) {
		std::string normalized = std::filesystem::u8path(path).lexically_normal().u8string();
#ifdef _WINDOWS
		normalized = ToLower(normalized);
#endif
		return normalized;
	};

	std::map<std::string, bool> existingFiles;
	auto fileExists = [&existingFiles](const std::string& path) {
		auto existing = existingFiles.find(path);
		if (existing != existingFiles.end())
			return existing->second;

		return existingFiles[path] = PlatformUtil::FileExists(path);
	};

	const std::string osdPath = normalize(osdFilePath);

	for (auto& entry : entries) {
		if (entry.setName == exceptSet && SameFile(entry.fileName, exceptFile))
			continue;

		const SliderSet& set = entry.set;
		for (size_t i = 0; i < set.size(); i++) {
			for (auto& dataFile : set[i].dataFiles) {
				if (SliderDataFileIsBSD(dataFile.fileName))
					continue;

				std::string dataOSDFileName;
				std::string dataNameInFile;
				if (!SplitSliderDataFileName(dataFile.fileName, dataOSDFileName, dataNameInFile))
					continue;

				// Only resolve the paths of data in a file of the same name
				if (!StringsEqualInsens(dataOSDFileName.c_str(), osdName.c_str()))
					continue;

				// The data is in the first of its data folders that has the file, like ResolveSliderDataFile finds it
				std::vector<std::string> dataFolders;
				if (dataFile.bLocal)
					dataFolders.push_back(set.GetDefaultDataFolder());
				else
					dataFolders = set.GetTargetDataFolders(dataFile.targetName);

				for (auto& dataFolder : dataFolders) {
					std::string candidatePath = normalize(set.GetBaseDataPath() + PathSepStr + dataFolder + PathSepStr + dataOSDFileName);
					if (candidatePath == osdPath) {
						dataNames.insert(dataNameInFile);
						break;
					}

					if (fileExists(candidatePath))
						break;
				}
			}
		}
	}

	return dataNames;
}

std::vector<const SliderSetIndexEntry*> SliderSetIndex::SetsUsingNif(const std::string& nifFilePath, const std::string& exceptFile, const std::string& exceptSet) const {
	std::vector<const SliderSetIndexEntry*> sets;

	std::string nifName = std::filesystem::u8path(nifFilePath).filename().u8string();

	for (auto& entry : entries) {
		if (entry.setName == exceptSet && SameFile(entry.fileName, exceptFile))
			continue;

		const SliderSet& set = entry.set;
		if (!StringsEqualInsens(set.GetInputFile().c_str(), nifName.c_str()))
			continue;

		if (SameFile(set.GetInputFileName(), nifFilePath))
			sets.push_back(&entry);
	}

	return sets;
}

bool SliderSetIndex::SameFile(const std::string& fileName1, const std::string& fileName2) {
	std::error_code error;
	return std::filesystem::equivalent(std::filesystem::u8path(fileName1), std::filesystem::u8path(fileName2), error);
}
