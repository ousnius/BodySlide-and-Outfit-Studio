/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SliderData.h"

#include "../utils/StringStuff.h"

#include <string>

inline bool SliderDataFileIsBSD(const std::string& fileName) {
	return fileName.size() > 4 && StringsEqualNInsens(fileName.c_str() + fileName.size() - 4, ".bsd", 4);
}

inline bool SliderDataFileNameIsOSD(const std::string& fileName) {
	return fileName.size() > 4 && StringsEqualNInsens(fileName.c_str() + fileName.size() - 4, ".osd", 4);
}

inline bool SplitSliderDataFileName(const std::string& fileName, std::string& outFileName, std::string& outDataName) {
	size_t split = fileName.find_last_of('/');
	if (split == std::string::npos)
		split = fileName.find_last_of('\\');
	if (split == std::string::npos)
		return false;

	outFileName = fileName.substr(0, split);
	outDataName = fileName.substr(split + 1);
	return true;
}

inline std::string SliderDataNameInFile(const DiffInfo& dataFile) {
	std::string fileName;
	std::string dataName;
	if (SplitSliderDataFileName(dataFile.fileName, fileName, dataName))
		return dataName;

	return dataFile.dataName;
}

inline std::string BuildSliderDataFileName(const DiffInfo& dataFile, const std::string& osdFileName) {
	if (osdFileName.empty() || SliderDataFileIsBSD(dataFile.fileName))
		return dataFile.fileName;

	return osdFileName + PathSepStr + SliderDataNameInFile(dataFile);
}