#include "SliderData.h"
#include "XmlFinder.h"
#include <sstream>

SliderData::SliderData(const string& inName) {
	Name = inName;
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

SliderData::SliderData(TiXmlElement* e) {
	LoadSliderData(e);
}

int SliderData::LoadSliderData(TiXmlElement* e, set<string>* exclude_targets) {
	//Outfit Studio state values, not saved in xml.
	curValue = 0;
	bShow = false;
	int n_datafiles = 0;

	Name = e->Attribute("name");
	if (e->Attribute("invert"))
		bInvert = (_strnicmp(e->Attribute("invert"), "true", 4) == 0);
	else
		bInvert = false;

	if (e->Attribute("uv"))
		bUV = (_strnicmp(e->Attribute("uv"), "true", 4) == 0);
	else
		bUV = false;

	double b, s;
	e->Attribute("big", &b);
	e->Attribute("small", &s);
	defBigValue = (float)b;
	defSmallValue = (float)s;

	if (e->Attribute("hidden")) {
		bHidden = (_strnicmp(e->Attribute("hidden"), "true", 4) == 0);
	}
	else
		bHidden = false;

	if (e->Attribute("zap")) {
		bZap = (_strnicmp(e->Attribute("zap"), "true", 4) == 0);
	}
	else
		bZap = false;

	if (e->Attribute("clamp")) {
		bClamp = (_strnicmp(e->Attribute("clamp"), "true", 4) == 0);
	}
	else
		bClamp = false;

	DiffDataFile tmpDataFile;
	TiXmlElement* datafile = e->FirstChildElement("datafile");
	while (datafile) {
		tmpDataFile.targetName = datafile->Attribute("target");
		if (exclude_targets) {
			if (exclude_targets->find(tmpDataFile.targetName) != exclude_targets->end()) {
				datafile = datafile->NextSiblingElement("datafile");
				continue;
			}

		}

		if (datafile->Attribute("local")) {
			tmpDataFile.bLocal = (_strnicmp(datafile->Attribute("local"), "true", 4) == 0);
		}
		else
			tmpDataFile.bLocal = false;

		if (datafile->Attribute("name")) {
			tmpDataFile.dataName = datafile->Attribute("name");
		}
		else
			tmpDataFile.dataName = Name;

		tmpDataFile.fileName = datafile->GetText();

		dataFiles.push_back(tmpDataFile);

		datafile = datafile->NextSiblingElement("datafile");
		n_datafiles++;
	}
	if (n_datafiles == 0)
		return 1;

	double reqval;
	string reqslider;
	TiXmlElement* requirement = e->FirstChildElement("require");
	while (requirement) {
		requirement->Attribute("value", &reqval);
		reqslider = requirement->Attribute("slidername");
		requirements[reqslider] = (float)reqval;
		requirement = requirement->NextSiblingElement("require");
	}
	return 0;
}

SliderData::~SliderData() {
}

SliderSet::SliderSet() {
	genWeights = true;
}

SliderSet::SliderSet(TiXmlElement* e) {
	LoadSliderSet(e);
}

SliderSet::~SliderSet(){
}

void SliderSet::DeleteSlider(const string& setName) {
	for (int i = 0; i < sliders.size(); i++) {
		if (sliders[i].Name == setName) {
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
	sliders.emplace_back(other->Name);
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

int SliderSet::LoadSliderSet(TiXmlElement* e, unsigned int flags) {
	Name = e->Attribute("name");
	TiXmlElement* te;
	te = e->FirstChildElement("SetFolder");
	if (te)
		datafolder = te->GetText();
	te = e->FirstChildElement("SourceFile");
	if (te)
		inputfile = te->GetText();
	te = e->FirstChildElement("OutputPath");
	if (te)
		outputpath = te->GetText();

	genWeights = true;
	te = e->FirstChildElement("OutputFile");
	if (te) {
		outputfile = te->GetText();
		if (te->Attribute("GenWeights")) {
			string gw = te->Attribute("GenWeights");
			if (_strnicmp(gw.c_str(), "false", 5) == 0) {
				genWeights = false;
			}
		}
	}
	set <string> exclude_targets;
	TiXmlElement* shapename = e->FirstChildElement("BaseShapeName");
	while (shapename) {
		if (shapename->Attribute("DataFolder")) {
			if ((flags & LOADSS_REFERENCE) == 0) {
				exclude_targets.insert(shapename->Attribute("target"));
				shapename = shapename->NextSiblingElement("BaseShapeName");
				continue;
			}
			targetdatafolders[shapename->Attribute("target")] = shapename->Attribute("DataFolder");
			targetshapenames[shapename->Attribute("target")] = shapename->GetText();
		}
		else {
			if ((flags & LOADSS_DIRECT) == 0) {
				exclude_targets.insert(shapename->Attribute("target"));
				shapename = shapename->NextSiblingElement("BaseShapeName");
				continue;
			}
			targetdatafolders[shapename->Attribute("target")] = datafolder;
			targetshapenames[shapename->Attribute("target")] = shapename->GetText();
		}


		if (shapename->Attribute("voffset")) {
			vector<string> parts;
			string stroffset = shapename->Attribute("voffset");
			stringstream ss(stroffset);
			string item;
			vec3 v;
			while (std::getline(ss, item, ' ')) {
				parts.push_back(item);
			}
			if (parts.size() == 3) {
				v.x = (float)atof(parts[0].c_str());
				v.y = (float)atof(parts[1].c_str());
				v.z = (float)atof(parts[2].c_str());
			}
			targetoffsets[shapename->Attribute("target")] = v;
		}

		shapename = shapename->NextSiblingElement("BaseShapeName");
	}

	TiXmlElement* sliderEntry = e->FirstChildElement("Slider");
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

void SliderSet::WriteSliderSet(TiXmlElement* SliderSetElement) {
	SliderSetElement->Clear();
	SliderSetElement->SetAttribute("name", Name.c_str());

	SliderSetElement->InsertEndChild(TiXmlElement("SetFolder"))->ToElement()->InsertEndChild(TiXmlText(datafolder.c_str()));
	SliderSetElement->InsertEndChild(TiXmlElement("SourceFile"))->ToElement()->InsertEndChild(TiXmlText(inputfile.c_str()));
	SliderSetElement->InsertEndChild(TiXmlElement("OutputPath"))->ToElement()->InsertEndChild(TiXmlText(outputpath.c_str()));

	TiXmlElement* of = SliderSetElement->InsertEndChild(TiXmlElement("OutputFile"))->ToElement();
	if (!genWeights)
		of->SetAttribute("GenWeights", "false");
	of->InsertEndChild(TiXmlText(outputfile.c_str()));

	TiXmlElement* baseshapeElem;
	TiXmlElement* sliderElem;
	TiXmlElement* datafileElem;
	char buf[256];

	for (auto tsn : targetshapenames) {
		baseshapeElem = SliderSetElement->InsertEndChild(TiXmlElement("BaseShapeName"))->ToElement();
		baseshapeElem->SetAttribute("target", tsn.first.c_str());
		if (targetdatafolders.find(tsn.first) != targetdatafolders.end()) {
			baseshapeElem->SetAttribute("DataFolder", targetdatafolders[tsn.first].c_str());
		}
		auto o = targetoffsets.find(tsn.first);
		if (o != targetoffsets.end()) {
			_snprintf_s(buf, 256, 256, "%.5f %.5f %.5f", o->second.x, o->second.y, o->second.z);
			baseshapeElem->SetAttribute("voffset", buf);
		}

		baseshapeElem->InsertEndChild(TiXmlText(tsn.second.c_str()));
	}

	for (auto slider : sliders) {
		if (slider.dataFiles.size() == 0)
			continue;

		sliderElem = SliderSetElement->InsertEndChild(TiXmlElement("Slider"))->ToElement();
		sliderElem->SetAttribute("name", slider.Name.c_str());
		sliderElem->SetAttribute("invert", (slider.bInvert) ? "true" : "false");
		sliderElem->SetAttribute("small", (int)slider.defSmallValue);
		sliderElem->SetAttribute("big", (int)slider.defBigValue);
		if (slider.bHidden)
			sliderElem->SetAttribute("hidden", "true");
		if (slider.bClamp)
			sliderElem->SetAttribute("clamp", "true");
		if (slider.bZap)
			sliderElem->SetAttribute("zap", "true");
		if (slider.bUV)
			sliderElem->SetAttribute("uv", "true");
		for (auto df : slider.dataFiles) {
			datafileElem = sliderElem->InsertEndChild(TiXmlElement("datafile"))->ToElement();
			datafileElem->SetAttribute("name", df.dataName.c_str());
			datafileElem->SetAttribute("target", df.targetName.c_str());
			if (df.bLocal) {
				datafileElem->SetAttribute("local", "true");
			}

			// Hack! Force these clamp bsd's to use the new ones that don't include the CBBE offset.
			// this is here in order to allow older slider sets to be opened by Outfit Studio and saved without
			// telling the user about the offset. (OS clears out the offset naturally when loading the files, so everything lines up
			// except for these old seam clamps.)
			string fn = df.fileName;
			if (fn == "LockSeamLo.bsd") fn = "LockSeamLo2.bsd";
			if (fn == "LockSeamHi.bsd") fn = "LockSeamHi2.bsd";
			if (fn == "NH_LockSeamLo.bsd") fn = "NH_LockSeamLo2.bsd";
			if (fn == "NH_LockSeamHi.bsd") fn = "NH_LockSeamHi2.bsd";
			datafileElem->InsertEndChild(TiXmlText(fn.c_str()));
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
	TiXmlElement* setElement;
	string setname;
	fileName = srcFileName;
	if (!doc.LoadFile(srcFileName.c_str())) {
		error = doc.ErrorId();
		return;
	}

	root = doc.FirstChildElement("SliderSetInfo");
	if (!root) {
		error = 100;
		return;
	}

	setElement = root->FirstChildElement("SliderSet");
	while (setElement) {
		setname = setElement->Attribute("name");
		SetsInFile[setname] = setElement;
		SetsOrder.push_back(setname);
		setElement = setElement->NextSiblingElement("SliderSet");
	}
}

void SliderSetFile::New(const string& newFileName) {
	error = 0;
	fileName = newFileName;
	doc.Clear();
	doc.ClearError();
	TiXmlElement rootelem("SliderSetInfo");
	doc.InsertEndChild(rootelem);
	root = doc.FirstChildElement("SliderSetInfo");
}

int SliderSetFile::GetSetNames(vector<string> &outSetNames, bool append) {
	if (!append)
		outSetNames.clear();
	map<string, TiXmlElement*>::iterator xmlit;
	for (xmlit = SetsInFile.begin(); xmlit != SetsInFile.end(); ++xmlit) {
		outSetNames.push_back(xmlit->first);
	}

	return outSetNames.size();
}

int SliderSetFile::GetSetNamesUnsorted(vector<string> &outSetNames, bool append) {
	if (!append) {
		outSetNames.clear();
		outSetNames.assign(SetsOrder.begin(), SetsOrder.end());
	}
	else
		outSetNames.insert(outSetNames.end(), SetsOrder.begin(), SetsOrder.end());

	return outSetNames.size();
}

bool SliderSetFile::HasSet(const string &querySetName) {
	return (SetsInFile.find(querySetName) != SetsInFile.end());
}

void SliderSetFile::SetShapes(const string& set, vector<string>& outShapeNames) {
	if (!HasSet(set))
		return;

	TiXmlElement* setElement = SetsInFile[set];
	TiXmlElement* shapeElement = setElement->FirstChildElement("BaseShapeName");
	while (shapeElement) {
		outShapeNames.push_back(shapeElement->GetText());
		shapeElement = shapeElement->NextSiblingElement("BaseShapeName");
	}
}

void SliderSetFile::SetTargets(const string& set, vector<string>& outTargetNames) {
	if (!HasSet(set))
		return;

	TiXmlElement* setElement = SetsInFile[set];
	TiXmlElement* shapeElement = setElement->FirstChildElement("BaseShapeName");
	while (shapeElement) {
		outTargetNames.push_back(shapeElement->Attribute("target"));
		shapeElement = shapeElement->NextSiblingElement("BaseShapeName");
	}
}

int SliderSetFile::GetSet(const string &setName, SliderSet &outSliderSet, unsigned int flags) {
	TiXmlElement* setPtr;
	if (!HasSet(setName))
		return 1;

	setPtr = SetsInFile[setName];

	int ret;
	ret = outSliderSet.LoadSliderSet(setPtr, flags);

	return ret;
}

int SliderSetFile::GetAllSets(vector<SliderSet> &outAppendSets) {
	int err;
	SliderSet tmpSet;
	map<string, TiXmlElement*>::iterator xmlit;
	for (xmlit = SetsInFile.begin(); xmlit != SetsInFile.end(); ++xmlit) {
		err = tmpSet.LoadSliderSet(xmlit->second);
		if (err)
			return err;
		outAppendSets.push_back(tmpSet);
	}
	return 0;
}

int SliderSetFile::UpdateSet(SliderSet &inSliderSet) {
	TiXmlElement* setPtr;
	string setName;
	setName = inSliderSet.GetName();
	if (HasSet(setName)) {
		setPtr = SetsInFile[setName];
	}
	else {
		TiXmlElement tmpElement("SliderSet");
		tmpElement.SetAttribute("name", setName.c_str());
		setPtr = root->InsertEndChild(tmpElement)->ToElement();
	}
	inSliderSet.WriteSliderSet(setPtr);

	return 0;
}

int SliderSetFile::Save() {
	return doc.SaveFile(fileName.c_str());
}

void PresetCollection::Clear() {
	namedSliderPresets.clear();
}

void PresetCollection::GetPresetNames(vector<string>& outNames) {
	map<string, map<string, SliderPreset>>::iterator it;
	for (it = namedSliderPresets.begin(); it != namedSliderPresets.end(); ++it) {
		outNames.push_back(it->first);
	}
}

void PresetCollection::SetSliderPreset(const string& set, const string& slider, float big, float small) {
	map<string, SliderPreset> newPreset;
	SliderPreset sp;
	if (namedSliderPresets.find(set) == namedSliderPresets.end()) {
		sp.big = sp.small = -10000.0f;
		if (big > -10000)
			sp.big = big;
		if (small > -10000)
			sp.small = small;
		newPreset[slider] = sp;
		namedSliderPresets[set] = newPreset;

	}
	else {
		if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end()) {
			sp.big = sp.small = -10000.0f;
			if (big > -10000)
				sp.big = big;
			if (small > -10000)
				sp.small = small;
			namedSliderPresets[set][slider] = sp;
		}
		else {
			if (big > -10000) {
				namedSliderPresets[set][slider].big = big;
			}
			if (small > -10000) {
				namedSliderPresets[set][slider].small = small;
			}
		}
	}
}

bool PresetCollection::GetBigPreset(const string& set, const string& slider, float& big) {
	float b;
	if (namedSliderPresets.find(set) == namedSliderPresets.end())
		return false;
	if (namedSliderPresets[set].find(slider) == namedSliderPresets[set].end())
		return false;
	b = namedSliderPresets[set][slider].big;
	if (b > -10000) {
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
	if (b > -10000) {
		small = b;
		return true;
	}
	return false;
}

bool PresetCollection::LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilter) {
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* e;
	TiXmlElement* g;
	TiXmlElement* setslider;

	string presetName, sliderName, applyTo;
	double o, b, s;

	XmlFinder finder(basePath);
	while (!finder.atEnd()) {
		string filename = finder.next();
		if (!doc.LoadFile(filename.c_str()))
			continue;
		root = doc.FirstChildElement("SliderPresets");
		if (!root)
			continue;
		e = root->FirstChildElement("Preset");
		while (e) {
			bool skip = true;
			g = e->FirstChildElement("Group");
			while (g) {
				for (int i = 0; i < groupFilter.size(); i++) {
					if (g->Attribute("name") == groupFilter[i]) {
						skip = false;
						break;
					}
				}
				g = g->NextSiblingElement("Group");
			}
			if (e->Attribute("set") == sliderSet)
				skip = false;

			if (skip) {
				e = e->NextSiblingElement("Preset");
				continue;
			}

			presetName = e->Attribute("name");
			setslider = e->FirstChildElement("SetSlider");
			while (setslider) {
				sliderName = setslider->Attribute("name");
				applyTo = setslider->Attribute("size");
				setslider->Attribute("value", &o);
				o = o / 100.0;
				s = b = -10000.0;
				if (applyTo == "small")
					s = o;
				else if (applyTo == "big")
					b = o;
				else if (applyTo == "both")
					s = b = o;
				SetSliderPreset(presetName, sliderName, b, s);
				setslider = setslider->NextSiblingElement("SetSlider");
			}
			e = e->NextSiblingElement("Preset");
		}
	}
	return !finder.hadError();
}

int PresetCollection::SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
	if (namedSliderPresets.find(presetName) == namedSliderPresets.end())
		return -1;

	TiXmlDocument outDoc;
	TiXmlNode* slidersNode;
	TiXmlElement* presetElem;
	if (outDoc.LoadFile(filePath.c_str())) {
		// File exists - merge data.
		slidersNode = outDoc.FirstChild("SliderPresets");
		presetElem = (TiXmlElement*)slidersNode->FirstChildElement("Preset");
		while (presetElem) {
			// Replace preset if found in file.
			if (_stricmp(presetElem->Attribute("name"), presetName.c_str()) == 0) {
				TiXmlElement* tmpElem = presetElem;
				presetElem = presetElem->NextSiblingElement("Preset");
				slidersNode->RemoveChild(tmpElem);
			}
			else
				presetElem = presetElem->NextSiblingElement("Preset");
		}
	}
	else
		slidersNode = outDoc.InsertEndChild(TiXmlElement("SliderPresets"));

	presetElem = slidersNode->InsertEndChild(TiXmlElement("Preset"))->ToElement();
	presetElem->SetAttribute("name", presetName.c_str());
	presetElem->SetAttribute("set", sliderSetName.c_str());

	TiXmlElement* sliderElem;
	for (int i = 0; i < assignGroups.size(); i++){
		sliderElem = presetElem->InsertEndChild(TiXmlElement("Group"))->ToElement();
		sliderElem->SetAttribute("name", assignGroups[i].c_str());
	}
	for (auto p : namedSliderPresets[presetName]) {
		if (p.second.big > -10000.0f) {
			sliderElem = presetElem->InsertEndChild(TiXmlElement("SetSlider"))->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "big");
			sliderElem->SetAttribute("value", p.second.big * 100);
		}
		if (p.second.small > -10000.0f) {
			sliderElem = presetElem->InsertEndChild(TiXmlElement("SetSlider"))->ToElement();
			sliderElem->SetAttribute("name", p.first.c_str());
			sliderElem->SetAttribute("size", "small");
			sliderElem->SetAttribute("value", p.second.small * 100);
		}
	}
	if (!outDoc.SaveFile())
		return outDoc.ErrorId();

	return 0;
}
