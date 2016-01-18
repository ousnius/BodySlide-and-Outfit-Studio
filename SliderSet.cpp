/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "SliderSet.h"

SliderSet::SliderSet() {
}

SliderSet::~SliderSet() {
}

SliderSet::SliderSet(XMLElement* element) {
	LoadSliderSet(element);
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
	XMLElement *root = element->Parent()->ToElement();
	int version = root->IntAttribute("version");

	string shapeStr = version >= 1 ? "Shape" : "BaseShapeName";
	string dataFolderStr = version >= 1 ? "DataFolder" : "SetFolder";

	name = element->Attribute("name");

	XMLElement* tmpElement = element->FirstChildElement(dataFolderStr.c_str());
	if (tmpElement) {
		tmpElement->SetName("DataFolder");
		datafolder = tmpElement->GetText();
	}

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

	XMLElement* shapeName = element->FirstChildElement(shapeStr.c_str());
	while (shapeName) {
		shapeName->SetName("Shape");
		if (shapeName->Attribute("DataFolder"))
			targetdatafolders[shapeName->Attribute("target")] = shapeName->Attribute("DataFolder");
		else
			targetdatafolders[shapeName->Attribute("target")] = datafolder;

		targetshapenames[shapeName->Attribute("target")] = shapeName->GetText();
		shapeName = shapeName->NextSiblingElement(shapeStr.c_str());
	}

	SliderData tmpSlider;
	XMLElement* sliderEntry = element->FirstChildElement("Slider");
	while (sliderEntry) {
		tmpSlider.Clear();
		if (tmpSlider.LoadSliderData(sliderEntry, genWeights) == 0)
			sliders.push_back(tmpSlider);

		sliderEntry = sliderEntry->NextSiblingElement("Slider");
	}
	return 0;
}

void SliderSet::LoadSetDiffData(DiffDataSets& inDataStorage) {
	map<string, map<string, string>> osdNames;

	for (auto &slider : sliders) {
		for (auto &ddf : slider.dataFiles) {
			string fullFilePath = baseDataPath + "\\";
			if (ddf.bLocal)
				fullFilePath += datafolder + "\\";
			else
				fullFilePath += targetdatafolders[ddf.targetName] + "\\";

			fullFilePath += ddf.fileName;

			// BSD format
			if (ddf.fileName.compare(ddf.fileName.size() - 4, ddf.fileName.size(), ".bsd") == 0) {
				inDataStorage.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
			}
			// OSD format
			else {
				// Split file name to get file and data name in it
				int split = fullFilePath.find_last_of('\\');
				if (split < 0)
					continue;

				string dataName = fullFilePath.substr(split + 1);
				string fileName = fullFilePath.substr(0, split);

				// Cache data locations
				osdNames[fileName][dataName] = ddf.targetName;
			}
		}
	}

	// Load from cached data locations at once
	inDataStorage.LoadData(osdNames);
}

void SliderSet::WriteSliderSet(XMLElement* sliderSetElement) {
	sliderSetElement->DeleteChildren();
	sliderSetElement->SetAttribute("name", name.c_str());

	XMLElement* newElement = sliderSetElement->GetDocument()->NewElement("DataFolder");
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

	outputFileElement->SetAttribute("GenWeights", genWeights ? "true" : "false");

	newText = sliderSetElement->GetDocument()->NewText(outputfile.c_str());
	outputFileElement->InsertEndChild(newText);

	XMLElement* baseShapeElement;
	XMLElement* sliderElement;
	XMLElement* dataFileElement;

	for (auto &tsn : targetshapenames) {
		newElement = sliderSetElement->GetDocument()->NewElement("Shape");
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
		sliderElement->SetAttribute("invert", slider.bInvert ? "true" : "false");

		if (genWeights) {
			sliderElement->SetAttribute("small", (int)slider.defSmallValue);
			sliderElement->SetAttribute("big", (int)slider.defBigValue);
		}
		else
			sliderElement->SetAttribute("default", (int)slider.defBigValue);

		if (slider.bHidden)
			sliderElement->SetAttribute("hidden", "true");
		if (slider.bClamp)
			sliderElement->SetAttribute("clamp", "true");
		if (slider.bZap)
			sliderElement->SetAttribute("zap", "true");
		if (slider.bUV)
			sliderElement->SetAttribute("uv", "true");
		for (auto &df : slider.dataFiles) {
			newElement = sliderSetElement->GetDocument()->NewElement("Data");
			dataFileElement = sliderElement->InsertEndChild(newElement)->ToElement();
			dataFileElement->SetAttribute("name", df.dataName.c_str());
			dataFileElement->SetAttribute("target", df.targetName.c_str());
			if (df.bLocal)
				dataFileElement->SetAttribute("local", "true");

			newText = sliderSetElement->GetDocument()->NewText(df.fileName.c_str());
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

	version = root->IntAttribute("version");

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
		rootElement->SetAttribute("version", version);

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
	XMLElement* shapeElement = setElement->FirstChildElement("Shape");
	while (shapeElement) {
		outShapeNames.push_back(shapeElement->GetText());
		shapeElement = shapeElement->NextSiblingElement("Shape");
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

void SliderSetFile::GetSetOutputFilePath(const string &setName, string &outFilePath) {
	outFilePath.clear();

	if (!HasSet(setName))
		return;

	XMLElement* setPtr = setsInFile[setName];
	XMLElement* tmpElement = setPtr->FirstChildElement("OutputPath");
	if (tmpElement)
		outFilePath += tmpElement->GetText();

	tmpElement = setPtr->FirstChildElement("OutputFile");
	if (tmpElement) {
		outFilePath += "\\";
		outFilePath += tmpElement->GetText();
	}
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
