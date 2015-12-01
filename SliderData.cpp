/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
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

SliderData::SliderData(XMLElement* element) {
	LoadSliderData(element);
}

int SliderData::LoadSliderData(XMLElement* element) {
	//Outfit Studio state values, not saved in XML.
	curValue = 0;
	bShow = false;
	int numDatafiles = 0;

	name = element->Attribute("name");
	if (element->Attribute("invert"))
		bInvert = (_strnicmp(element->Attribute("invert"), "true", 4) == 0);
	else
		bInvert = false;

	if (element->Attribute("uv"))
		bUV = (_strnicmp(element->Attribute("uv"), "true", 4) == 0);
	else
		bUV = false;

	float b = element->FloatAttribute("big");
	float s = element->FloatAttribute("small");
	defBigValue = b;
	defSmallValue = s;

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

	DiffDataFile tmpDataFile;
	XMLElement* datafile = element->FirstChildElement("datafile");
	while (datafile) {
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

		datafile = datafile->NextSiblingElement("datafile");
		numDatafiles++;
	}
	if (numDatafiles == 0)
		return 1;

	return 0;
}
