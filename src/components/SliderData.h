/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../TinyXML-2/tinyxml2.h"
#include "DiffData.h"

using namespace tinyxml2;

struct DiffInfo {
	bool bLocal;			// Local files use the slider set's directory for path info. Otherwise, it uses the target's data path.
	std::string dataName;	// Alias for the data.
	std::string targetName; // Shape affected by the data.
	std::string fileName;	// File name not including path.
	DiffInfo(bool l = false, const std::string& dn = "", const std::string& tn = "", const std::string& fn = "")
		: bLocal(l)
		, dataName(dn)
		, targetName(tn)
		, fileName(fn) {}
};

class SliderData {
public:
	std::string name;
	bool bHidden = false;
	bool bInvert = false;
	bool bZap = false;
	bool bClamp = false;
	bool bUV = false;
	float defSmallValue = 0.0f;
	float defBigValue = 0.0f;
	std::vector<std::string> zapToggles;

	float curValue = 0.0f;
	bool bShow = false;

	std::vector<DiffInfo> dataFiles;

	SliderData(std::string inName = "");
	~SliderData();

	// Gets the slider's data record name for the specified target.
	std::string TargetDataName(const std::string& targetName) {
		for (auto& df : dataFiles)
			if (df.targetName == targetName)
				return df.dataName;

		return "";
	}

	std::string DataFileName(const std::string& targetDataName) {
		for (auto& df : dataFiles)
			if (df.dataName == targetDataName)
				return df.fileName;

		return "";
	}

	void RenameTarget(const std::string& oldTarget, const std::string& newTarget) {
		for (auto& df : dataFiles) {
			if (df.targetName == oldTarget) {
				df.targetName = newTarget;

				if (df.dataName.length() >= oldTarget.length()) {
					std::string dtname = df.dataName.substr(oldTarget.length());
					df.dataName = newTarget + dtname;
				}
			}
		}
	}

	void RenameData(const std::string& oldDT, const std::string& newDT) {
		for (auto& df : dataFiles) {
			if (df.dataName == oldDT)
				df.dataName = newDT;
		}
	}

	// Creates a data file record and returns the record index.
	size_t AddDataFile(const std::string& shapeTarget, const std::string& dataAlias, const std::string& fileName, bool localData = true) {
		dataFiles.emplace_back(localData, dataAlias, shapeTarget, fileName);
		return dataFiles.size() - 1;
	}

	bool IsLocalData(const std::string& dataAlias) {
		for (auto& df : dataFiles)
			if (df.dataName == dataAlias)
				return df.bLocal;

		return false;
	}

	void SetLocalData(const std::string& dataAlias) {
		for (auto& df : dataFiles)
			if (df.dataName == dataAlias)
				df.bLocal = true;
	}

	void Clear() { dataFiles.clear(); }

	int LoadSliderData(XMLElement* srcdata, bool genWeights);
};
