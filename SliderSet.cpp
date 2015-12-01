/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderSet.h"

SliderSet::SliderSet() {
	genWeights = true;
}

SliderSet::SliderSet(XMLElement* element) {
	LoadSliderSet(element);
}

SliderSet::~SliderSet() {
}

void SliderSet::DeleteSlider(const string& setName) {
	for (int i = 0; i < sliders.size(); i++) {
		if (sliders[i].name == setName) {
			sliders.erase(sliders.begin() + i);
			return;
		}
	}
}

int SliderSet::CreateSlider(const string& setName) {
	sliders.emplace_back(setName);
	return sliders.size() - 1;
}

int SliderSet::CopySlider(SliderData* other) {
	sliders.emplace_back(other->name);
	SliderData* ms = &sliders.back();
	ms->bClamp = other->bClamp;
	ms->bHidden = other->bHidden;
	ms->bInvert = other->bInvert;
	ms->bZap = other->bZap;
	ms->defBigValue = other->defBigValue;
	ms->defSmallValue = other->defSmallValue;
	ms->dataFiles = other->dataFiles;
	return sliders.size() - 1;
}

int SliderSet::LoadSliderSet(XMLElement* element) {
	name = element->Attribute("name");
	XMLElement* tmpElement;
	tmpElement = element->FirstChildElement("SetFolder");
	if (tmpElement)
		datafolder = tmpElement->GetText();
	tmpElement = element->FirstChildElement("SourceFile");
	if (tmpElement)
		inputfile = tmpElement->GetText();
	tmpElement = element->FirstChildElement("OutputPath");
	if (tmpElement)
		outputpath = tmpElement->GetText();

	genWeights = true;
	tmpElement = element->FirstChildElement("OutputFile");
	if (tmpElement) {
		outputfile = tmpElement->GetText();
		if (tmpElement->Attribute("GenWeights")) {
			string gw = tmpElement->Attribute("GenWeights");
			if (_strnicmp(gw.c_str(), "false", 5) == 0) {
				genWeights = false;
			}
		}
	}

	XMLElement* shapeName = element->FirstChildElement("BaseShapeName");
	while (shapeName) {
		if (shapeName->Attribute("DataFolder"))
			targetdatafolders[shapeName->Attribute("target")] = shapeName->Attribute("DataFolder");
		else
			targetdatafolders[shapeName->Attribute("target")] = datafolder;

		targetshapenames[shapeName->Attribute("target")] = shapeName->GetText();
		shapeName = shapeName->NextSiblingElement("BaseShapeName");
	}

	XMLElement* sliderEntry = element->FirstChildElement("Slider");
	SliderData tmpSlider;
	while (sliderEntry) {
		tmpSlider.Clear();
		if (tmpSlider.LoadSliderData(sliderEntry) == 0)
			sliders.push_back(tmpSlider);

		sliderEntry = sliderEntry->NextSiblingElement("Slider");
	}
	return 0;
}

void SliderSet::LoadSetDiffData(DiffDataSets& inDataStorage) {
	string fullfilepath;
	DiffDataFile f;
	for (int i = 0; i < sliders.size(); i++) {
		for (int j = 0; j < sliders[i].dataFiles.size(); j++) {
			f = sliders[i].dataFiles[j];
			fullfilepath = baseDataPath + "\\";
			if (f.bLocal)
				fullfilepath += datafolder + "\\";
			else
				fullfilepath += targetdatafolders[f.targetName] + "\\";

			fullfilepath += f.fileName;
			inDataStorage.LoadSet(f.dataName, f.targetName, fullfilepath);
		}
	}
}

void SliderSet::WriteSliderSet(XMLElement* sliderSetElement) {
	sliderSetElement->DeleteChildren();
	sliderSetElement->SetAttribute("name", name.c_str());

	XMLElement* newElement = sliderSetElement->GetDocument()->NewElement("SetFolder");
	XMLText* newText = sliderSetElement->GetDocument()->NewText(datafolder.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("SourceFile");
	newText = sliderSetElement->GetDocument()->NewText(inputfile.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("OutputPath");
	newText = sliderSetElement->GetDocument()->NewText(outputpath.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("OutputFile");
	XMLElement* outputFileElement = sliderSetElement->InsertEndChild(newElement)->ToElement();

	if (!genWeights)
		outputFileElement->SetAttribute("GenWeights", "false");

	newText = sliderSetElement->GetDocument()->NewText(outputfile.c_str());
	outputFileElement->InsertEndChild(newText);

	XMLElement* baseShapeElement;
	XMLElement* sliderElement;
	XMLElement* dataFileElement;

	for (auto &tsn : targetshapenames) {
		newElement = sliderSetElement->GetDocument()->NewElement("BaseShapeName");
		baseShapeElement = sliderSetElement->InsertEndChild(newElement)->ToElement();
		baseShapeElement->SetAttribute("target", tsn.first.c_str());
		if (targetdatafolders.find(tsn.first) != targetdatafolders.end())
			baseShapeElement->SetAttribute("DataFolder", targetdatafolders[tsn.first].c_str());

		newText = sliderSetElement->GetDocument()->NewText(tsn.second.c_str());
		baseShapeElement->InsertEndChild(newText);
	}

	for (auto &slider : sliders) {
		if (slider.dataFiles.size() == 0)
			continue;

		newElement = sliderSetElement->GetDocument()->NewElement("Slider");
		sliderElement = sliderSetElement->InsertEndChild(newElement)->ToElement();
		sliderElement->SetAttribute("name", slider.name.c_str());
		sliderElement->SetAttribute("invert", (slider.bInvert) ? "true" : "false");
		sliderElement->SetAttribute("small", (int)slider.defSmallValue);
		sliderElement->SetAttribute("big", (int)slider.defBigValue);
		if (slider.bHidden)
			sliderElement->SetAttribute("hidden", "true");
		if (slider.bClamp)
			sliderElement->SetAttribute("clamp", "true");
		if (slider.bZap)
			sliderElement->SetAttribute("zap", "true");
		if (slider.bUV)
			sliderElement->SetAttribute("uv", "true");
		for (auto &df : slider.dataFiles) {
			newElement = sliderSetElement->GetDocument()->NewElement("datafile");
			dataFileElement = sliderElement->InsertEndChild(newElement)->ToElement();
			dataFileElement->SetAttribute("name", df.dataName.c_str());
			dataFileElement->SetAttribute("target", df.targetName.c_str());
			if (df.bLocal) {
				dataFileElement->SetAttribute("local", "true");
			}

			// Hack! Force these clamp bsd's to use the new ones that don't include the CBBE offset.
			// this is here in order to allow older slider sets to be opened by Outfit Studio and saved without
			// telling the user about the offset. (OS clears out the offset naturally when loading the files, so everything lines up
			// except for these old seam clamps.)
			string fn = df.fileName;
			if (fn == "LockSeamLo.bsd")
				fn = "LockSeamLo2.bsd";
			if (fn == "LockSeamHi.bsd")
				fn = "LockSeamHi2.bsd";
			if (fn == "NH_LockSeamLo.bsd")
				fn = "NH_LockSeamLo2.bsd";
			if (fn == "NH_LockSeamHi.bsd")
				fn = "NH_LockSeamHi2.bsd";

			newText = sliderSetElement->GetDocument()->NewText(fn.c_str());
			dataFileElement->InsertEndChild(newText);
		}
	}
}

string SliderSet::GetInputFileName() {
	string o;
	o = baseDataPath + "\\";
	o += datafolder + "\\";
	o += inputfile;
	return o;
}

string SliderSet::GetOutputFilePath() {
	return outputpath + "\\" + outputfile;
}

bool SliderSet::GenWeights() {
	return genWeights;
}

SliderSetFile::SliderSetFile(const string& srcFileName) :error(0) {
	Open(srcFileName);
}

void SliderSetFile::Open(const string& srcFileName) {
	fileName = srcFileName;
	error = doc.LoadFile(srcFileName.c_str());
	if (error)
		return;

	root = doc.FirstChildElement("SliderSetInfo");
	if (!root) {
		error = 100;
		return;
	}

	version = root->IntAttribute("Version");

	XMLElement* setElement;
	string setname;
	setElement = root->FirstChildElement("SliderSet");
	while (setElement) {
		setname = setElement->Attribute("name");
		setsInFile[setname] = setElement;
		setsOrder.push_back(setname);
		setElement = setElement->NextSiblingElement("SliderSet");
	}
}

void SliderSetFile::New(const string& newFileName) {
	version = 1; // Most recent
	error = 0;
	fileName = newFileName;
	doc.Clear();

	XMLElement* rootElement = doc.NewElement("SliderSetInfo");
	if (version > 0)
		rootElement->SetAttribute("Version", version);

	doc.InsertEndChild(rootElement);
	root = doc.FirstChildElement("SliderSetInfo");
}

int SliderSetFile::GetSetNames(vector<string> &outSetNames, bool append) {
	if (!append)
		outSetNames.clear();

	for (auto &xmlit : setsInFile)
		outSetNames.push_back(xmlit.first);

	return outSetNames.size();
}

int SliderSetFile::GetSetNamesUnsorted(vector<string> &outSetNames, bool append) {
	if (!append) {
		outSetNames.clear();
		outSetNames.assign(setsOrder.begin(), setsOrder.end());
	}
	else
		outSetNames.insert(outSetNames.end(), setsOrder.begin(), setsOrder.end());

	return outSetNames.size();
}

bool SliderSetFile::HasSet(const string &querySetName) {
	return (setsInFile.find(querySetName) != setsInFile.end());
}

void SliderSetFile::SetShapes(const string& set, vector<string>& outShapeNames) {
	if (!HasSet(set))
		return;

	XMLElement* setElement = setsInFile[set];
	XMLElement* shapeElement = setElement->FirstChildElement("BaseShapeName");
	while (shapeElement) {
		outShapeNames.push_back(shapeElement->GetText());
		shapeElement = shapeElement->NextSiblingElement("BaseShapeName");
	}
}

void SliderSetFile::SetTargets(const string& set, vector<string>& outTargetNames) {
	if (!HasSet(set))
		return;

	XMLElement* setElement = setsInFile[set];
	XMLElement* shapeElement = setElement->FirstChildElement("BaseShapeName");
	while (shapeElement) {
		outTargetNames.push_back(shapeElement->Attribute("target"));
		shapeElement = shapeElement->NextSiblingElement("BaseShapeName");
	}
}

int SliderSetFile::GetSet(const string &setName, SliderSet &outSliderSet) {
	XMLElement* setPtr;
	if (!HasSet(setName))
		return 1;

	setPtr = setsInFile[setName];

	int ret;
	ret = outSliderSet.LoadSliderSet(setPtr);

	return ret;
}

int SliderSetFile::GetAllSets(vector<SliderSet> &outAppendSets) {
	int err;
	SliderSet tmpSet;
	for (auto &xmlit : setsInFile) {
		err = tmpSet.LoadSliderSet(xmlit.second);
		if (err)
			return err;
		outAppendSets.push_back(tmpSet);
	}
	return 0;
}

int SliderSetFile::UpdateSet(SliderSet &inSliderSet) {
	XMLElement* setPtr;
	string setName;
	setName = inSliderSet.GetName();
	if (HasSet(setName)) {
		setPtr = setsInFile[setName];
	}
	else {
		XMLElement* tmpElement = doc.NewElement("SliderSet");
		tmpElement->SetAttribute("name", setName.c_str());
		setPtr = root->InsertEndChild(tmpElement)->ToElement();
	}
	inSliderSet.WriteSliderSet(setPtr);

	return 0;
}

int SliderSetFile::Save() {
	return doc.SaveFile(fileName.c_str()) == XML_SUCCESS;
}
