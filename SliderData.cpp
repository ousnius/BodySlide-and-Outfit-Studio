#include "SliderData.h"
#include "XmlFinder.h"
#include <sstream>

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

SliderData::SliderData(XMLElement* element) {
	LoadSliderData(element);
}

int SliderData::LoadSliderData(XMLElement* element, set<string>* exclude_targets) {
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
		if (exclude_targets) {
			if (exclude_targets->find(tmpDataFile.targetName) != exclude_targets->end()) {
				datafile = datafile->NextSiblingElement("datafile");
				continue;
			}
		}

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

	float reqval;
	string reqslider;
	XMLElement* requirement = element->FirstChildElement("require");
	while (requirement) {
		reqval = requirement->FloatAttribute("value");
		reqslider = requirement->Attribute("slidername");
		requirements[reqslider] = reqval;
		requirement = requirement->NextSiblingElement("require");
	}
	return 0;
}

SliderData::~SliderData() {
}

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

int SliderSet::LoadSliderSet(XMLElement* element, uint flags) {
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
	set<string> exclude_targets;
	XMLElement* shapeName = element->FirstChildElement("BaseShapeName");
	while (shapeName) {
		if (shapeName->Attribute("DataFolder")) {
			if ((flags & LOADSS_REFERENCE) == 0) {
				exclude_targets.insert(shapeName->Attribute("target"));
				shapeName = shapeName->NextSiblingElement("BaseShapeName");
				continue;
			}
			targetdatafolders[shapeName->Attribute("target")] = shapeName->Attribute("DataFolder");
			targetshapenames[shapeName->Attribute("target")] = shapeName->GetText();
		}
		else {
			if ((flags & LOADSS_DIRECT) == 0) {
				exclude_targets.insert(shapeName->Attribute("target"));
				shapeName = shapeName->NextSiblingElement("BaseShapeName");
				continue;
			}
			targetdatafolders[shapeName->Attribute("target")] = datafolder;
			targetshapenames[shapeName->Attribute("target")] = shapeName->GetText();
		}


		if (shapeName->Attribute("voffset")) {
			vector<string> parts;
			string stroffset = shapeName->Attribute("voffset");
			stringstream ss(stroffset);
			string item;
			Vector3 v;
			while (std::getline(ss, item, ' ')) {
				parts.push_back(item);
			}
			if (parts.size() == 3) {
				v.x = (float)atof(parts[0].c_str());
				v.y = (float)atof(parts[1].c_str());
				v.z = (float)atof(parts[2].c_str());
			}
			targetoffsets[shapeName->Attribute("target")] = v;
		}

		shapeName = shapeName->NextSiblingElement("BaseShapeName");
	}

	XMLElement* sliderEntry = element->FirstChildElement("Slider");
	SliderData tmpSlider;
	while (sliderEntry) {
		tmpSlider.Clear();
		if (tmpSlider.LoadSliderData(sliderEntry, &exclude_targets) == 0) {
			sliders.push_back(tmpSlider);
		}
		else if ((flags & LOADSS_ADDEXCLUDED) != 0) {
			tmpSlider.ClearDataFiles();
			sliders.push_back(tmpSlider);
		}

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
	char buf[256];

	for (auto tsn : targetshapenames) {
		newElement = sliderSetElement->GetDocument()->NewElement("BaseShapeName");
		baseShapeElement = sliderSetElement->InsertEndChild(newElement)->ToElement();
		baseShapeElement->SetAttribute("target", tsn.first.c_str());
		if (targetdatafolders.find(tsn.first) != targetdatafolders.end()) {
			baseShapeElement->SetAttribute("DataFolder", targetdatafolders[tsn.first].c_str());
		}
		auto o = targetoffsets.find(tsn.first);
		if (o != targetoffsets.end()) {
			_snprintf_s(buf, 256, 256, "%.5f %.5f %.5f", o->second.x, o->second.y, o->second.z);
			baseShapeElement->SetAttribute("voffset", buf);
		}

		newText = sliderSetElement->GetDocument()->NewText(tsn.second.c_str());
		baseShapeElement->InsertEndChild(newText);
	}

	for (auto slider : sliders) {
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
		for (auto df : slider.dataFiles) {
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
	XMLElement* setElement;
	string setname;
	fileName = srcFileName;
	error = doc.LoadFile(srcFileName.c_str());
	if (error)
		return;

	root = doc.FirstChildElement("SliderSetInfo");
	if (!root) {
		error = 100;
		return;
	}

	setElement = root->FirstChildElement("SliderSet");
	while (setElement) {
		setname = setElement->Attribute("name");
		setsInFile[setname] = setElement;
		setsOrder.push_back(setname);
		setElement = setElement->NextSiblingElement("SliderSet");
	}
}

void SliderSetFile::New(const string& newFileName) {
	error = 0;
	fileName = newFileName;
	doc.Clear();

	XMLElement* rootElement = doc.NewElement("SliderSetInfo");
	doc.InsertEndChild(rootElement);
	root = doc.FirstChildElement("SliderSetInfo");
}

int SliderSetFile::GetSetNames(vector<string> &outSetNames, bool append) {
	if (!append)
		outSetNames.clear();

	for (auto xmlit = setsInFile.begin(); xmlit != setsInFile.end(); ++xmlit)
		outSetNames.push_back(xmlit->first);

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

int SliderSetFile::GetSet(const string &setName, SliderSet &outSliderSet, uint flags) {
	XMLElement* setPtr;
	if (!HasSet(setName))
		return 1;

	setPtr = setsInFile[setName];

	int ret;
	ret = outSliderSet.LoadSliderSet(setPtr, flags);

	return ret;
}

int SliderSetFile::GetAllSets(vector<SliderSet> &outAppendSets) {
	int err;
	SliderSet tmpSet;
	for (auto xmlit = setsInFile.begin(); xmlit != setsInFile.end(); ++xmlit) {
		err = tmpSet.LoadSliderSet(xmlit->second);
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

void PresetCollection::Clear() {
	namedSliderPresets.clear();
}

void PresetCollection::GetPresetNames(vector<string>& outNames) {
	for (auto it = namedSliderPresets.begin(); it != namedSliderPresets.end(); ++it)
		outNames.push_back(it->first);
}

void PresetCollection::SetSliderPreset(const string& set, const string& slider, float big, float small) {
	map<string, SliderPreset> newPreset;
	SliderPreset sp;
	if (namedSliderPresets.find(set) == namedSliderPresets.end()) {
		sp.big = sp.small = -10000.0f;
		if (big > -10000.0f)
			sp.big = big;
		if (small > -10000.0f)
			sp.small = small;
		newPreset[slider] = sp;
		namedSliderPresets[set] = newPreset;

	}
	else {
		if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end()) {
			sp.big = sp.small = -10000.0f;
			if (big > -10000.0f)
				sp.big = big;
			if (small > -10000.0f)
				sp.small = small;
			namedSliderPresets[set][slider] = sp;
		}
		else {
			if (big > -10000.0f) {
				namedSliderPresets[set][slider].big = big;
			}
			if (small > -10000.0f) {
				namedSliderPresets[set][slider].small = small;
			}
		}
	}
}

bool PresetCollection::GetSliderExists(const string& set, const string& slider) {
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;

	return true;
}

bool PresetCollection::GetBigPreset(const string& set, const string& slider, float& big) {
	float b;
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;
	b = namedSliderPresets[set][slider].big;
	if (b > -10000.0f) {
		big = b;
		return true;
	}
	return false;
}

bool PresetCollection::GetSmallPreset(const string& set, const string& slider, float& small) {
	float b;
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;
	b = namedSliderPresets[set][slider].small;
	if (b > -10000.0f) {
		small = b;
		return true;
	}
	return false;
}

bool PresetCollection::LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilter, bool allPresets) {
	XMLDoc doc;
	XMLElement* root;
	XMLElement* element;
	XMLElement* g;
	XMLElement* setSlider;

	string presetName, sliderName, applyTo;
	float o, b, s;

	XmlFinder finder(basePath);
	while (!finder.atEnd()) {
		string filename = finder.next();
		if (doc.LoadFile(filename.c_str()) != XML_SUCCESS)
			continue;

		root = doc.FirstChildElement("SliderPresets");
		if (!root)
			continue;

		element = root->FirstChildElement("Preset");
		while (element) {
			bool skip = true;
			g = element->FirstChildElement("Group");
			while (g) {
				for (int i = 0; i < groupFilter.size(); i++) {
					if (g->Attribute("name") == groupFilter[i]) {
						skip = false;
						break;
					}
				}
				g = g->NextSiblingElement("Group");
			}
			if (element->Attribute("set") == sliderSet || allPresets)
				skip = false;

			if (skip) {
				element = element->NextSiblingElement("Preset");
				continue;
			}

			presetName = element->Attribute("name");
			setSlider = element->FirstChildElement("SetSlider");
			while (setSlider) {
				sliderName = setSlider->Attribute("name");
				applyTo = setSlider->Attribute("size");
				o = setSlider->FloatAttribute("value") / 100.0f;
				s = b = -10000.0f;
				if (applyTo == "small")
					s = o;
				else if (applyTo == "big")
					b = o;
				else if (applyTo == "both")
					s = b = o;

				SetSliderPreset(presetName, sliderName, b, s);
				setSlider = setSlider->NextSiblingElement("SetSlider");
			}
			element = element->NextSiblingElement("Preset");
		}
	}
	return !finder.hadError();
}

int PresetCollection::SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
	if (namedSliderPresets.find(presetName) == namedSliderPresets.end())
		return -1;

	XMLElement* newElement = nullptr;
	XMLDoc outDoc;
	XMLNode* slidersNode;
	XMLElement* presetElem;
	if (outDoc.LoadFile(filePath.c_str()) == XML_SUCCESS) {
		// File exists - merge data.
		slidersNode = outDoc.FirstChildElement("SliderPresets");
		presetElem = slidersNode->FirstChildElement("Preset");
		while (presetElem) {
			// Replace preset if found in file.
			if (_stricmp(presetElem->Attribute("name"), presetName.c_str()) == 0) {
				XMLElement* tmpElem = presetElem;
				presetElem = presetElem->NextSiblingElement("Preset");
				slidersNode->DeleteChild(tmpElem);
			}
			else
				presetElem = presetElem->NextSiblingElement("Preset");
		}
	}
	else {
		newElement = outDoc.NewElement("SliderPresets");
		slidersNode = outDoc.InsertEndChild(newElement);
	}
	
	newElement = outDoc.NewElement("Preset");
	presetElem = slidersNode->InsertEndChild(newElement)->ToElement();
	presetElem->SetAttribute("name", presetName.c_str());
	presetElem->SetAttribute("set", sliderSetName.c_str());

	XMLElement* sliderElem;
	for (int i = 0; i < assignGroups.size(); i++){
		newElement = outDoc.NewElement("Group");
		sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
		sliderElem->SetAttribute("name", assignGroups[i].c_str());
	}
	for (auto p : namedSliderPresets[presetName]) {
		if (p.second.big > -10000.0f) {
			newElement = outDoc.NewElement("SetSlider");
			sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "big");
			sliderElem->SetAttribute("value", (int)(p.second.big * 100.0f));
		}
		if (p.second.small > -10000.0f) {
			newElement = outDoc.NewElement("SetSlider");
			sliderElem = presetElem->InsertEndChild(newElement)->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "small");
			sliderElem->SetAttribute("value", (int)(p.second.small * 100.0f));
		}
	}
	if (outDoc.SaveFile(filePath.c_str()) != XML_SUCCESS)
		return outDoc.ErrorID();

	return 0;
}
