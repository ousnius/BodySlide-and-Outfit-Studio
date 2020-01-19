/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderSet.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"


SliderSet::SliderSet() {
}

SliderSet::~SliderSet() {
}

SliderSet::SliderSet(XMLElement* element) {
	LoadSliderSet(element);
}


void SliderSet::DeleteSlider(const std::string& setName) {
	// Delete toggles from other sliders
	for (auto &slider : sliders) {
		slider.zapToggles.erase(
			std::remove(slider.zapToggles.begin(), slider.zapToggles.end(), setName),
			slider.zapToggles.end());
	}

	// Find and delete slider
	for (int i = 0; i < sliders.size(); i++) {
		if (sliders[i].name == setName) {
			sliders.erase(sliders.begin() + i);
			return;
		}
	}
}

int SliderSet::CreateSlider(const std::string& setName) {
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
	ms->bUV = other->bUV;
	ms->defBigValue = other->defBigValue;
	ms->defSmallValue = other->defSmallValue;
	ms->zapToggles = other->zapToggles;
	ms->dataFiles = other->dataFiles;
	return sliders.size() - 1;
}

int SliderSet::LoadSliderSet(XMLElement* element) {
	XMLElement *root = element->Parent()->ToElement();
	int version = root->IntAttribute("version");

	std::string shapeStr = version >= 1 ? "Shape" : "BaseShapeName";
	std::string dataFolderStr = version >= 1 ? "DataFolder" : "SetFolder";

	name = element->Attribute("name");

	XMLElement* tmpElement = element->FirstChildElement(dataFolderStr.c_str());
	if (tmpElement) {
		tmpElement->SetName("DataFolder");
		datafolder = ToOSSlashes(tmpElement->GetText());
	}

	tmpElement = element->FirstChildElement("SourceFile");
	if (tmpElement)
		inputfile = tmpElement->GetText();

	tmpElement = element->FirstChildElement("OutputPath");
	if (tmpElement)
		outputpath = ToOSSlashes(tmpElement->GetText());

	genWeights = true;
	tmpElement = element->FirstChildElement("OutputFile");
	if (tmpElement) {
		outputfile = tmpElement->GetText();
		genWeights = tmpElement->BoolAttribute("GenWeights", true);
	}

	XMLElement* shapeName = element->FirstChildElement(shapeStr.c_str());
	while (shapeName) {
		shapeName->SetName("Shape");

		auto& shape = shapeAttributes[shapeName->GetText()];
		if (shapeName->Attribute("DataFolder"))
			shape.dataFolder = ToOSSlashes(shapeName->Attribute("DataFolder"));
		else
			shape.dataFolder = datafolder;

		if (shapeName->Attribute("target"))
			shape.targetShape = shapeName->Attribute("target");

		shape.smoothSeamNormals = shapeName->BoolAttribute("SmoothSeamNormals", true);
		shape.lockNormals = shapeName->BoolAttribute("LockNormals", false);

		shapeName = shapeName->NextSiblingElement(shapeStr.c_str());
	}

	XMLElement* sliderEntry = element->FirstChildElement("Slider");
	while (sliderEntry) {
		SliderData tmpSlider;
		if (tmpSlider.LoadSliderData(sliderEntry, genWeights) == 0) {
			// Check if slider already exists
			for (auto &s : sliders) {
				if (s.name == tmpSlider.name) {
					// Merge data of existing sliders
					for (auto &df : tmpSlider.dataFiles)
						s.AddDataFile(df.targetName, df.dataName, df.fileName, df.bLocal);

					break;
				}
			}

			if (!SliderExists(tmpSlider.name))
				sliders.push_back(std::move(tmpSlider));
		}

		sliderEntry = sliderEntry->NextSiblingElement("Slider");
	}

	// A slider set/project can optionally specify a default normals generation configuration.  If present,
	// it activates the normals generation functionality in the preview window. Note customized normals generation settings 
	// are saved in separate xml files -- the project-homed ones are default settings only.
	XMLElement* normalsGeneration = element->FirstChildElement("NormalsGeneration");
	if (normalsGeneration) {
		// Only load default settings if none are currently specified
		if (defNormalGen.size() == 0)
			NormalGenLayer::LoadFromXML(normalsGeneration, defNormalGen);
	}
	else
		defNormalGen.clear();

	return 0;
}

void SliderSet::LoadSetDiffData(DiffDataSets& inDataStorage, const std::string& forShape) {
	std::map<std::string, std::map<std::string, std::string>> osdNames;

	for (auto &slider : sliders) {
		for (auto &ddf : slider.dataFiles) {
			if (ddf.fileName.size() <= 4)
				continue;

			std::string shapeName = TargetToShape(ddf.targetName);
			if (!forShape.empty() && shapeName != forShape)
				continue;

			std::string fullFilePath = baseDataPath + PathSepStr;
			if (ddf.bLocal)
				fullFilePath += datafolder + PathSepStr;
			else
				fullFilePath += ShapeToDataFolder(shapeName) + PathSepStr;

			fullFilePath += ddf.fileName;

			// BSD format
			if (ddf.fileName.compare(ddf.fileName.size() - 4, ddf.fileName.size(), ".bsd") == 0) {
				inDataStorage.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
			}
			// OSD format
			else {
				// Split file name to get file and data name in it
				int split = fullFilePath.find_last_of('/');
				if (split < 0)
					split = fullFilePath.find_last_of('\\');
				if (split < 0)
					return;

				std::string dataName = fullFilePath.substr(split + 1);
				std::string fileName = fullFilePath.substr(0, split);

				// Cache data locations
				osdNames[fileName][dataName] = ddf.targetName;
			}
		}
	}

	// Load from cached data locations at once
	inDataStorage.LoadData(osdNames);
}

void SliderSet::Merge(SliderSet& mergeSet, DiffDataSets& inDataStorage, DiffDataSets& baseDiffData, const std::string& baseShape) {
	std::map<std::string, std::map<std::string, std::string>> osdNames;
	std::map<std::string, std::map<std::string, std::string>> osdNamesBase;

	auto addSlider = [&](DiffInfo& ddf) {
		if (ddf.fileName.size() <= 4)
			return;

		std::string shapeName = mergeSet.TargetToShape(ddf.targetName);
		std::string fullFilePath = mergeSet.baseDataPath + PathSepStr;
		if (ddf.bLocal)
			fullFilePath += mergeSet.datafolder + PathSepStr;
		else
			fullFilePath += mergeSet.ShapeToDataFolder(shapeName) + PathSepStr;

		fullFilePath += ddf.fileName;

		// BSD format
		if (ddf.fileName.compare(ddf.fileName.size() - 4, ddf.fileName.size(), ".bsd") == 0) {
			if (shapeName != baseShape)
				inDataStorage.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
			else
				baseDiffData.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
		}
		// OSD format
		else {
			// Split file name to get file and data name in it
			int split = fullFilePath.find_last_of('/');
			if (split < 0)
				split = fullFilePath.find_last_of('\\');
			if (split < 0)
				return;

			std::string dataName = fullFilePath.substr(split + 1);
			std::string fileName = fullFilePath.substr(0, split);

			// Cache data locations
			if (shapeName != baseShape)
				osdNames[fileName][dataName] = ddf.targetName;
			else
				osdNamesBase[fileName][dataName] = ddf.targetName;
		}

		// Make new data local
		ddf.bLocal = true;
	};

	for (auto &s : mergeSet.sliders) {
		auto sliderIt = std::find_if(sliders.begin(), sliders.end(), [&s](const SliderData& rs) {
			return rs.name == s.name;
		});

		if (sliderIt != sliders.end()) {
			// Copy missing data of existing slider
			for (auto &sd : s.dataFiles) {
				auto sliderDataIt = std::find_if(sliderIt->dataFiles.begin(), sliderIt->dataFiles.end(), [&](const DiffInfo& rd) {
					std::string shapeName = TargetToShape(rd.targetName);
					std::string shapeNameMerge = mergeSet.TargetToShape(sd.targetName);
					return rd.targetName == sd.targetName && rd.dataName == sd.dataName && shapeName == shapeNameMerge;
				});

				if (sliderDataIt == sliderIt->dataFiles.end()) {
					int df = sliderIt->AddDataFile(sd.targetName, sd.dataName, sd.fileName, sd.bLocal);
					addSlider(sliderIt->dataFiles[df]);
				}
			}
		}
		else {
			// Copy new slider to the set
			sliders.push_back(s);
			for (auto &ddf : sliders.back().dataFiles)
				addSlider(ddf);
		}
	}

	for (auto &s : mergeSet.shapeAttributes) {
		// Copy new shapes to the set
		if (shapeAttributes.find(s.first) == shapeAttributes.end()) {
			s.second.dataFolder.clear();
			shapeAttributes[s.first] = s.second;
		}
	}

	// Load from cached data locations at once
	inDataStorage.LoadData(osdNames);
	baseDiffData.LoadData(osdNamesBase);
}

void SliderSet::WriteSliderSet(XMLElement* sliderSetElement) {
	sliderSetElement->DeleteChildren();
	sliderSetElement->SetAttribute("name", name.c_str());

	XMLElement* newElement = sliderSetElement->GetDocument()->NewElement("DataFolder");
	std::string datafolder_bs = ToBackslashes(datafolder);
	XMLText* newText = sliderSetElement->GetDocument()->NewText(datafolder_bs.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("SourceFile");
	newText = sliderSetElement->GetDocument()->NewText(inputfile.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("OutputPath");
	std::string outputpath_bs = ToBackslashes(outputpath);
	newText = sliderSetElement->GetDocument()->NewText(outputpath_bs.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("OutputFile");
	XMLElement* outputFileElement = sliderSetElement->InsertEndChild(newElement)->ToElement();

	outputFileElement->SetAttribute("GenWeights", genWeights);

	newText = sliderSetElement->GetDocument()->NewText(outputfile.c_str());
	outputFileElement->InsertEndChild(newText);

	XMLElement* baseShapeElement;
	XMLElement* sliderElement;
	XMLElement* dataFileElement;

	for (auto &s : shapeAttributes) {
		newElement = sliderSetElement->GetDocument()->NewElement("Shape");
		baseShapeElement = sliderSetElement->InsertEndChild(newElement)->ToElement();

		if (!s.second.targetShape.empty())
			baseShapeElement->SetAttribute("target", s.second.targetShape.c_str());

		if (!s.second.dataFolder.empty()) {
			std::string dataFolder_bs = ToBackslashes(s.second.dataFolder);
			baseShapeElement->SetAttribute("DataFolder", dataFolder_bs.c_str());
		}

		if (!s.second.smoothSeamNormals)
			baseShapeElement->SetAttribute("SmoothSeamNormals", s.second.smoothSeamNormals);

		if (s.second.lockNormals)
			baseShapeElement->SetAttribute("LockNormals", s.second.lockNormals);

		newText = sliderSetElement->GetDocument()->NewText(s.first.c_str());
		baseShapeElement->InsertEndChild(newText);
	}

	for (auto &slider : sliders) {
		if (slider.dataFiles.size() == 0)
			continue;

		newElement = sliderSetElement->GetDocument()->NewElement("Slider");
		sliderElement = sliderSetElement->InsertEndChild(newElement)->ToElement();
		sliderElement->SetAttribute("name", slider.name.c_str());
		sliderElement->SetAttribute("invert", slider.bInvert);

		if (genWeights) {
			sliderElement->SetAttribute("small", (int)slider.defSmallValue);
			sliderElement->SetAttribute("big", (int)slider.defBigValue);
		}
		else
			sliderElement->SetAttribute("default", (int)slider.defBigValue);

		if (slider.bHidden)
			sliderElement->SetAttribute("hidden", true);
		if (slider.bClamp)
			sliderElement->SetAttribute("clamp", true);

		if (slider.bZap) {
			sliderElement->SetAttribute("zap", true);

			std::string zapToggles;
			for (auto &toggle : slider.zapToggles) {
				zapToggles.append(toggle);
				zapToggles.append(";");
			}

			if (!zapToggles.empty())
				sliderElement->SetAttribute("zaptoggles", zapToggles.c_str());
		}

		if (slider.bUV)
			sliderElement->SetAttribute("uv", true);

		for (auto &df : slider.dataFiles) {
			newElement = sliderSetElement->GetDocument()->NewElement("Data");
			dataFileElement = sliderElement->InsertEndChild(newElement)->ToElement();
			dataFileElement->SetAttribute("name", df.dataName.c_str());
			dataFileElement->SetAttribute("target", df.targetName.c_str());
			if (df.bLocal)
				dataFileElement->SetAttribute("local", true);

			std::string fileName_bs = ToBackslashes(df.fileName);
			newText = sliderSetElement->GetDocument()->NewText(fileName_bs.c_str());
			dataFileElement->InsertEndChild(newText);
		}
	}
}

std::string SliderSet::GetInputFileName() {
	std::string o;
	o = baseDataPath + PathSepStr;
	o += datafolder + PathSepStr;
	o += inputfile;
	return o;
}

std::string SliderSet::GetOutputFilePath() {
	return outputpath + PathSepStr + outputfile;
}

bool SliderSet::GenWeights() {
	return genWeights;
}

SliderSetFile::SliderSetFile(const std::string& srcFileName) :error(0) {
	Open(srcFileName);
}

void SliderSetFile::Open(const std::string& srcFileName) {
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error)
		return;
#else
	fp = fopen(srcFileName.c_str(), "rb");
	if (!fp) {
		error = errno;
		return;
	}
#endif

	error = doc.LoadFile(fp);
	fclose(fp);

	if (error)
		return;

	root = doc.FirstChildElement("SliderSetInfo");
	if (!root) {
		error = 100;
		return;
	}

	version = root->IntAttribute("version");

	XMLElement* setElement;
	std::string setname;
	setElement = root->FirstChildElement("SliderSet");
	while (setElement) {
		setname = setElement->Attribute("name");
		setsInFile[setname] = setElement;
		setsOrder.push_back(setname);
		setElement = setElement->NextSiblingElement("SliderSet");
	}
}

void SliderSetFile::New(const std::string& newFileName) {
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

int SliderSetFile::GetSetNames(std::vector<std::string> &outSetNames, bool append) {
	if (!append)
		outSetNames.clear();

	for (auto &xmlit : setsInFile)
		outSetNames.push_back(xmlit.first);

	return outSetNames.size();
}

int SliderSetFile::GetSetNamesUnsorted(std::vector<std::string> &outSetNames, bool append) {
	if (!append) {
		outSetNames.clear();
		outSetNames.assign(setsOrder.begin(), setsOrder.end());
	}
	else
		outSetNames.insert(outSetNames.end(), setsOrder.begin(), setsOrder.end());

	return outSetNames.size();
}

bool SliderSetFile::HasSet(const std::string& querySetName) {
	return (setsInFile.find(querySetName) != setsInFile.end());
}

void SliderSetFile::SetShapes(const std::string& set, std::vector<std::string>& outShapeNames) {
	if (!HasSet(set))
		return;

	XMLElement* setElement = setsInFile[set];
	XMLElement* shapeElement = setElement->FirstChildElement("Shape");
	while (shapeElement) {
		outShapeNames.push_back(shapeElement->GetText());
		shapeElement = shapeElement->NextSiblingElement("Shape");
	}
}

int SliderSetFile::GetSet(const std::string& setName, SliderSet &outSliderSet) {
	XMLElement* setPtr;
	if (!HasSet(setName))
		return 1;

	setPtr = setsInFile[setName];

	int ret;
	ret = outSliderSet.LoadSliderSet(setPtr);

	return ret;
}

int SliderSetFile::GetAllSets(std::vector<SliderSet> &outAppendSets) {
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

void SliderSetFile::GetSetOutputFilePath(const std::string& setName, std::string& outFilePath) {
	outFilePath.clear();

	if (!HasSet(setName))
		return;

	XMLElement* setPtr = setsInFile[setName];
	XMLElement* tmpElement = setPtr->FirstChildElement("OutputPath");
	if (tmpElement)
		outFilePath += ToOSSlashes(tmpElement->GetText());

	tmpElement = setPtr->FirstChildElement("OutputFile");
	if (tmpElement) {
		outFilePath += PathSepStr;
		outFilePath += tmpElement->GetText();
	}
}

int SliderSetFile::UpdateSet(SliderSet &inSliderSet) {
	XMLElement* setPtr;
	std::string setName;
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

int SliderSetFile::DeleteSet(const std::string& setName) {
	if (!HasSet(setName))
		return 1;

	XMLElement* setPtr = setsInFile[setName];
	setsInFile.erase(setName);
	doc.DeleteNode(setPtr);

	return 0;
}

bool SliderSetFile::Save() {
	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (error)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp) {
		error = errno;
		return false;
	}
#endif

	doc.SetBOM(true);

	const tinyxml2::XMLNode* firstChild = doc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		doc.InsertFirstChild(doc.NewDeclaration());

	error = doc.SaveFile(fp);
	fclose(fp);
	if (error)
		return false;

	return true;
}
