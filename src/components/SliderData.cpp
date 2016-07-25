/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderData.h"

#include <wx/dir.h>

SliderData::SliderData(const string& inName) {
	name = inName;
	bClamp = false;
	bHidden = false;
	bZap = false;
	bInvert = false;
	bShow = false;
	bUV = false;

	curValue = 0;
	defBigValue = 100;
	defSmallValue = 0;
}

SliderData::~SliderData() {
}

int SliderData::LoadSliderData(XMLElement* element, bool genWeights) {
	//Outfit Studio state values, not saved in XML.
	curValue = 0;
	bShow = false;
	int numData = 0;

	XMLElement *root = element->Parent()->Parent()->ToElement();
	int version = root->IntAttribute("version");

	string dataFileStr = version >= 1 ? "Data" : "datafile";

	name = element->Attribute("name");
	if (element->Attribute("invert"))
		bInvert = (_strnicmp(element->Attribute("invert"), "true", 4) == 0);
	else
		bInvert = false;

	if (element->Attribute("uv"))
		bUV = (_strnicmp(element->Attribute("uv"), "true", 4) == 0);
	else
		bUV = false;

	if (genWeights) {
		float b = element->FloatAttribute("big");
		float s = element->FloatAttribute("small");
		defBigValue = b;
		defSmallValue = s;
	}
	else {
		float defValue = element->FloatAttribute("default");
		defBigValue = defValue;
		defSmallValue = defValue;
	}

	if (element->Attribute("hidden"))
		bHidden = (_strnicmp(element->Attribute("hidden"), "true", 4) == 0);
	else
		bHidden = false;

	if (element->Attribute("zap"))
		bZap = (_strnicmp(element->Attribute("zap"), "true", 4) == 0);
	else
		bZap = false;

	if (element->Attribute("clamp"))
		bClamp = (_strnicmp(element->Attribute("clamp"), "true", 4) == 0);
	else
		bClamp = false;

	DiffInfo tmpDataFile;
	XMLElement* datafile = element->FirstChildElement(dataFileStr.c_str());
	while (datafile) {
		datafile->SetName("Data");
		tmpDataFile.targetName = datafile->Attribute("target");

		if (datafile->Attribute("local"))
			tmpDataFile.bLocal = (_strnicmp(datafile->Attribute("local"), "true", 4) == 0);
		else
			tmpDataFile.bLocal = false;

		if (datafile->Attribute("name"))
			tmpDataFile.dataName = datafile->Attribute("name");
		else
			tmpDataFile.dataName = name;

		tmpDataFile.fileName = datafile->GetText();

		dataFiles.push_back(tmpDataFile);

		datafile = datafile->NextSiblingElement(dataFileStr.c_str());
		numData++;
	}
	if (numData == 0)
		return 1;

	return 0;
}
