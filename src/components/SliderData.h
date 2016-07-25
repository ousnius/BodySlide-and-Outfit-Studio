/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "TinyXML-2/tinyxml2.h"
#include "DiffData.h"

using namespace std;
using namespace tinyxml2;

struct DiffInfo {
	bool bLocal;			// Local files use the slider set's directory for path info. Otherwise, it uses the target's data path.
	string dataName;		// Alias for the data.
	string targetName;		// Shape affected by the data.
	string fileName;		// File name not including path.
	DiffInfo(bool l = false, const string& dn = "", const string& tn = "", const string& fn = "")
		: bLocal(l), dataName(dn), targetName(tn), fileName(fn) {
	}
};

class SliderData {
public:
	string name;
	bool bHidden;
	bool bInvert;
	bool bZap;
	bool bClamp;
	bool bUV;			// UV Sliders!? Wat!
	float defSmallValue;
	float defBigValue;

	// Outfit Studio values
	float curValue;		// Current slider value.
	bool bShow;			// On to enable this slider when deforming verts.

	vector<DiffInfo> dataFiles;

	SliderData(const string& inName = "");
	SliderData(XMLElement* element);
	~SliderData();

	// Gets the slider's data record name for the specified target.
	string TargetDataName(const string& targetName) {
		for (auto &df : dataFiles)
			if (df.targetName == targetName)
				return df.dataName;

		return "";
	}
	string DataFileName(const string& targetDataName) {
		for (auto &df : dataFiles)
			if (df.dataName == targetDataName)
				return df.fileName;

		return "";
	}

	// Creates a data file record and returns the record index.  
	int AddDataFile(const string& shapeTarget, const string& dataAlias, const string& fileName, bool localData = true) {
		dataFiles.emplace_back(localData, dataAlias, shapeTarget, fileName);
		return dataFiles.size() - 1;
	}

	bool IsLocalData(const string& dataAlias) {
		for (auto &df : dataFiles)
			if (df.dataName == dataAlias)
				return df.bLocal;

		return false;
	}

	void SetLocalData(const string& dataAlias) {
		for (auto &df : dataFiles)
			if (df.dataName == dataAlias)
				df.bLocal = true;
	}

	void Clear() {
		dataFiles.clear();
	}

	int LoadSliderData(XMLElement* srcdata, bool genWeights);
};
