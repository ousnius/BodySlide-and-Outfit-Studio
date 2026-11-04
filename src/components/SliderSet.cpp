/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderSet.h"
#include "SliderDataFileUtil.h"
#include "SliderSetInheritance.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <random>

namespace {
constexpr const char* SliderSetElement = "SliderSet";
constexpr const char* SliderSetAddOnElement = "SliderSetAddOn";

bool IsSliderSetElement(const XMLElement* element) {
	return std::strcmp(element->Name(), SliderSetElement) == 0 || std::strcmp(element->Name(), SliderSetAddOnElement) == 0;
}

// Elements of a set that this version reads and writes. Others are kept as they are.
bool IsKnownSliderSetElement(const char* elementName) {
	static const char* const knownElements[] = {"ReferenceInfo",
												"BaseProject",
												"DataFolder",
												"SetFolder",
												"SourceFile",
												"SliderDataFile",
												"OutputPath",
												"OutputFile",
												"SFMorphPath",
												"SFMorphTargetShape",
												"Shape",
												"BaseShapeName",
												"Slider",
												"RemovedSlider",
												"Notes"};

	for (auto knownElement : knownElements)
		if (std::strcmp(elementName, knownElement) == 0)
			return true;

	return false;
}

// OSD data that is kept under another name than its name in the file
struct RenamedOSDData {
	std::string dataNameInFile;
	std::string dataName;
	std::string targetName;
};

void LoadRenamedOSDData(DiffDataSets& dataStorage, const std::map<std::string, std::vector<RenamedOSDData>>& renamedData) {
	for (auto& file : renamedData) {
		std::map<std::string, std::string> dataNames;
		for (auto& data : file.second)
			dataNames[data.dataNameInFile] = data.targetName;

		DiffDataSets loaded;
		loaded.LoadData({{file.first, dataNames}});

		for (auto& data : file.second)
			if (TargetDataDiffs* diff = loaded.GetDiffSet(data.dataNameInFile))
				dataStorage.LoadSet(data.dataName, data.targetName, *diff);
	}
}
} // namespace

SliderSet::SliderSet() {}

SliderSet::~SliderSet() {}

SliderSet::SliderSet(XMLElement* element) {
	LoadSliderSet(element);
}

std::vector<std::string> SliderSet::GetTargetDataFolders(const std::string& targetName) const {
	for (auto& shape : shapeAttributes) {
		if (shape.second.targetShape == targetName) {
			if (!shape.second.dataFolders.empty())
				return shape.second.dataFolders;

			break;
		}
	}

	std::vector<std::string> dataFolders;
	dataFolders.push_back(datafolder);
	return dataFolders;
}

SliderDataFileResolution SliderSet::ResolveSliderDataFile(const DiffInfo& dataFile, const std::vector<std::string>* externalDataFolders) const {
	SliderDataFileResolution result;
	if (dataFile.fileName.size() <= 4)
		return result;

	result.isBSD = SliderDataFileIsBSD(dataFile.fileName);
	if (!dataFile.bLocal && externalDataFolders)
		result.dataFolders = *externalDataFolders;
	else if (!dataFile.bLocal)
		result.dataFolders = GetTargetDataFolders(dataFile.targetName);
	else
		result.dataFolders.push_back(datafolder);

	std::string fullFilePath = baseDataPath + PathSepStr;
	std::string dataFileName = dataFile.fileName;

	if (!result.isBSD) {
		if (!SplitSliderDataFileName(dataFile.fileName, dataFileName, result.dataNameInFile))
			return result;
	}

	result.dataFileName = dataFileName;

	for (auto& dataFolder : result.dataFolders) {
		std::string filePath = dataFolder + PathSepStr + dataFileName;
		std::string candidatePath = fullFilePath + filePath;
		if (result.candidatePath.empty())
			result.candidatePath = candidatePath;

		if (PlatformUtil::FileExists(candidatePath)) {
			result.resolved = true;
			result.resolvedPath = candidatePath;
			break;
		}
	}

	return result;
}

DiffInfo* SliderSet::GetSliderDataFile(const size_t sliderIndex, const size_t dataFileIndex) {
	if (sliderIndex >= sliders.size())
		return nullptr;

	auto& slider = sliders[sliderIndex];
	if (dataFileIndex >= slider.dataFiles.size())
		return nullptr;

	return &slider.dataFiles[dataFileIndex];
}

const DiffInfo* SliderSet::GetSliderDataFile(const size_t sliderIndex, const size_t dataFileIndex) const {
	if (sliderIndex >= sliders.size())
		return nullptr;

	const auto& slider = sliders[sliderIndex];
	if (dataFileIndex >= slider.dataFiles.size())
		return nullptr;

	return &slider.dataFiles[dataFileIndex];
}

void SliderSet::SetSliderDataFileLocal(const size_t sliderIndex, const size_t dataFileIndex, const bool local) {
	DiffInfo* dataFile = GetSliderDataFile(sliderIndex, dataFileIndex);
	if (dataFile)
		dataFile->bLocal = local;
}

void SliderSet::SetSliderDataFileName(const size_t sliderIndex, const size_t dataFileIndex, const std::string& fileName) {
	DiffInfo* dataFile = GetSliderDataFile(sliderIndex, dataFileIndex);
	if (dataFile)
		dataFile->fileName = fileName;
}

bool SliderSet::TargetHasExternalData(const std::string& targetName) const {
	for (auto& slider : sliders)
		for (auto& dataFile : slider.dataFiles)
			if (dataFile.targetName == targetName && !dataFile.bLocal)
				return true;

	return false;
}

bool SliderSet::ClearLocalOnlyDataFolders() {
	bool changed = false;
	for (auto& shape : shapeAttributes) {
		if (shape.second.dataFolders.empty())
			continue;

		if (!TargetHasExternalData(shape.second.targetShape)) {
			shape.second.dataFolders.clear();
			changed = true;
		}
	}

	return changed;
}


size_t SliderSet::CloneSlider(const std::string& sliderName, const std::string& cloneName) {
	// Find slider
	auto sliderIt = std::find_if(sliders.begin(), sliders.end(), [&sliderName](const SliderData& s) {
		return s.name == sliderName;
	});

	if (sliderIt == sliders.end())
		return static_cast<size_t>(-1);

	// Clone slider
	auto& clonedSlider = sliders.emplace_back(*sliderIt);
	clonedSlider.name = cloneName;
	clonedSlider.curValue = 0.0f;
	clonedSlider.zapToggles.clear();
	clonedSlider.ForgetSliders([&cloneName](const std::string& slider) { return slider == cloneName; });
	clonedSlider.ClearInherited();

	return sliders.size() - 1;
}

void SliderSet::DeleteSlider(const std::string& sliderName) {
	// Delete toggles from other sliders
	for (auto& slider : sliders)
		slider.ForgetSliders([&sliderName](const std::string& toggled) { return toggled == sliderName; });

	// Find and delete slider
	for (size_t i = 0; i < sliders.size(); i++) {
		if (sliders[i].name == sliderName) {
			sliders.erase(sliders.begin() + i);
			return;
		}
	}
}

size_t SliderSet::CreateSlider(const std::string& sliderName) {
	sliders.emplace_back(sliderName);
	return sliders.size() - 1;
}

size_t SliderSet::CopySlider(SliderData* other) {
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
	ms->bToggle = other->bToggle;
	ms->toggleValue = other->toggleValue;
	ms->toggleSliders = other->toggleSliders;
	ms->bInherited = other->bInherited;
	ms->addOnName = other->addOnName;
	ms->addOnID = other->addOnID;
	ms->bAutoConformed = other->bAutoConformed;
	ms->stackedFrom = other->stackedFrom;
	ms->dataFiles = other->dataFiles;
	return sliders.size() - 1;
}

std::string SliderSet::GenerateProjectID() {
	static std::mt19937_64 random{std::random_device{}() ^ static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())};
	uint64_t high = random();
	uint64_t low = random();

	// Version 4 (random) and variant 1 as in RFC 4122
	high = (high & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
	low = (low & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;

	char id[37];
	snprintf(id,
			 sizeof(id),
			 "%08x-%04x-%04x-%04x-%012llx",
			 static_cast<uint32_t>(high >> 32),
			 static_cast<uint32_t>((high >> 16) & 0xFFFF),
			 static_cast<uint32_t>(high & 0xFFFF),
			 static_cast<uint32_t>(low >> 48),
			 static_cast<unsigned long long>(low & 0xFFFFFFFFFFFFull));
	return id;
}

void SliderSet::ClearBaseProject() {
	baseProjectFile.clear();
	baseProjectName.clear();
	baseProjectID.clear();
	addOn = false;
	sliderDataFile.clear();
	removedSliders.clear();
	for (auto& slider : sliders)
		slider.ClearInherited();
}

int SliderSet::LoadSliderSet(XMLElement* element, bool appendNewSliders, const std::string& onlyShapeName) {
	XMLElement* root = element->Parent()->ToElement();
	int version = root->IntAttribute("version");

	std::string shapeStr = version >= 1 ? "Shape" : "BaseShapeName";
	std::string dataFolderStr = version >= 1 ? "DataFolder" : "SetFolder";

	name = AttributeText(element, "name");

	XMLElement* tmpElement = element->FirstChildElement(dataFolderStr.c_str());
	if (tmpElement) {
		tmpElement->SetName("DataFolder");
		datafolder = ToOSSlashes(ElementText(tmpElement));
	}

	tmpElement = element->FirstChildElement("SourceFile");
	if (tmpElement)
		inputfile = ElementText(tmpElement);

	tmpElement = element->FirstChildElement("OutputPath");
	if (tmpElement)
		outputpath = ToOSSlashes(ElementText(tmpElement));

	genWeights = true;
	preventMorphFile = false;
	keepZappedShapes = false;

	tmpElement = element->FirstChildElement("OutputFile");
	if (tmpElement) {
		outputfile = ElementText(tmpElement);
		genWeights = tmpElement->BoolAttribute("GenWeights", true);
		preventMorphFile = tmpElement->BoolAttribute("PreventMorphFile");
		keepZappedShapes = tmpElement->BoolAttribute("KeepZappedShapes");
	}

	tmpElement = element->FirstChildElement("SFMorphPath");
	if (tmpElement && tmpElement->GetText())
		sfMorphPath = ToOSSlashes(tmpElement->GetText());
	else
		sfMorphPath.clear();

	tmpElement = element->FirstChildElement("SFMorphTargetShape");
	if (tmpElement && tmpElement->GetText())
		sfMorphTargetShape = tmpElement->GetText();
	else
		sfMorphTargetShape.clear();

	XMLElement* shapeName = element->FirstChildElement(shapeStr.c_str());
	while (shapeName) {
		shapeName->SetName("Shape");

		auto shapeText = shapeName->GetText();
		if (shapeText && (onlyShapeName.empty() || onlyShapeName == shapeText)) {
			auto& shape = shapeAttributes[shapeText];
			if (shapeName->Attribute("DataFolder")) {
				std::string dataFolderAttr = ToOSSlashes(shapeName->Attribute("DataFolder"));
				shape.dataFolders = SplitString(dataFolderAttr, ';');
			}
			else if (shape.dataFolders.empty())
				shape.dataFolders.push_back(datafolder);

			if (shapeName->Attribute("target"))
				shape.targetShape = shapeName->Attribute("target");
			else if (shape.targetShape.empty())
				shape.targetShape = shapeText;

			shape.smoothSeamNormals = shapeName->BoolAttribute("SmoothSeamNormals", true);
			shape.smoothSeamNormalsAngle = shapeName->FloatAttribute("SmoothSeamNormalsAngle", SliderSetShape::SliderSetDefaultSmoothAngle);
			shape.lockNormals = shapeName->BoolAttribute("LockNormals", false);
			shape.vertexCount = onlyShapeName.empty() ? shapeName->UnsignedAttribute("vertexCount") : 0;
		}

		shapeName = shapeName->NextSiblingElement(shapeStr.c_str());
	}

	std::string onlyTarget;
	if (!onlyShapeName.empty()) {
		onlyTarget = ShapeToTarget(onlyShapeName);
		if (onlyTarget.empty())
			onlyTarget = onlyShapeName;
	}

	XMLElement* sliderEntry = element->FirstChildElement("Slider");
	while (sliderEntry) {
		SliderData tmpSlider;
		if (tmpSlider.LoadSliderData(sliderEntry, genWeights) == 0) {
			// What the set inherits doesn't carry over to the set the shape is added to
			if (!onlyShapeName.empty())
				tmpSlider.ClearInherited();

			// Data of other targets would duplicate the entries of existing shapes with the same target
			if (!onlyTarget.empty()) {
				tmpSlider.dataFiles.erase(std::remove_if(tmpSlider.dataFiles.begin(),
														 tmpSlider.dataFiles.end(),
														 [&onlyTarget](const DiffInfo& df) { return df.targetName != onlyTarget; }),
										  tmpSlider.dataFiles.end());
			}

			// Check if slider already exists
			for (auto& s : sliders) {
				if (s.name == tmpSlider.name) {
					// Replace stale data of the target that is being loaded
					if (!onlyTarget.empty()) {
						s.dataFiles.erase(std::remove_if(s.dataFiles.begin(), s.dataFiles.end(), [&onlyTarget](const DiffInfo& df) { return df.targetName == onlyTarget; }),
										  s.dataFiles.end());
					}

					// Merge data of existing sliders
					for (auto& df : tmpSlider.dataFiles)
						s.AddDataFile(df.targetName, df.dataName, df.fileName, df.bLocal, df.bInherited);

					break;
				}
			}

			if (appendNewSliders && !SliderExists(tmpSlider.name))
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

	tmpElement = element->FirstChildElement("Notes");
	if (tmpElement && tmpElement->GetText())
		notes = tmpElement->GetText();
	else
		notes.clear();

	tmpElement = element->FirstChildElement("ReferenceInfo");
	if (tmpElement) {
		if (tmpElement->Attribute("ProjectFile"))
			refProjectFile = ToOSSlashes(tmpElement->Attribute("ProjectFile"));
		if (tmpElement->Attribute("ProjectName"))
			refProjectName = tmpElement->Attribute("ProjectName");
		if (tmpElement->Attribute("ShapeName"))
			refShapeName = tmpElement->Attribute("ShapeName");
	}
	else {
		refProjectFile.clear();
		refProjectName.clear();
		refShapeName.clear();
	}

	// Loading only one shape adds slider data, the set keeps its own base project
	if (onlyShapeName.empty()) {
		projectID = AttributeText(element, "id");
		addOn = std::strcmp(element->Name(), SliderSetAddOnElement) == 0;

		tmpElement = element->FirstChildElement("BaseProject");
		baseProjectFile = tmpElement ? ToOSSlashes(AttributeText(tmpElement, "ProjectFile")) : "";
		baseProjectName = tmpElement ? AttributeText(tmpElement, "ProjectName") : "";
		baseProjectID = tmpElement ? AttributeText(tmpElement, "ProjectID") : "";

		tmpElement = element->FirstChildElement("SliderDataFile");
		sliderDataFile = tmpElement ? ElementText(tmpElement) : "";

		removedSliders.clear();
		for (tmpElement = element->FirstChildElement("RemovedSlider"); tmpElement; tmpElement = tmpElement->NextSiblingElement("RemovedSlider"))
			AddRemovedSlider(AttributeText(tmpElement, "name"));

		unknownElements.clear();
		for (tmpElement = element->FirstChildElement(); tmpElement; tmpElement = tmpElement->NextSiblingElement()) {
			if (IsKnownSliderSetElement(tmpElement->Name()))
				continue;

			XMLPrinter printer(nullptr, true);
			tmpElement->Accept(&printer);
			unknownElements.push_back(printer.CStr());
		}

		resolveStatus = SliderSetResolveStatus();
	}

	return 0;
}

void SliderSet::LoadSetDiffData(DiffDataSets& inDataStorage, const std::string& forShape) {
	std::map<std::string, std::map<std::string, std::string>> osdNames;

	for (auto& slider : sliders) {
		for (auto& ddf : slider.dataFiles) {
			if (ddf.bGenerated) {
				if (!generatedData || (!forShape.empty() && TargetToShape(ddf.targetName) != forShape))
					continue;

				auto generated = generatedData->data.find(ddf.dataName);
				if (generated != generatedData->data.end())
					inDataStorage.LoadSet(ddf.dataName, generated->second.targetName, generated->second.diffs);
				continue;
			}

			if (ddf.fileName.size() <= 4)
				continue;

			std::string shapeName = TargetToShape(ddf.targetName);
			if (!forShape.empty() && shapeName != forShape)
				continue;

			bool isBSDFile = ddf.fileName.compare(ddf.fileName.size() - 4, ddf.fileName.size(), ".bsd") == 0;
			std::string fullFilePath = baseDataPath + PathSepStr;
			std::string dataName;

			std::vector<std::string> dataFolders;
			if (!ddf.bLocal)
				dataFolders = GetShapeDataFolders(shapeName);
			else
				dataFolders.push_back(datafolder);

			for (auto& df : dataFolders) {
				if (isBSDFile) {
					std::string filePath = df + PathSepStr + ddf.fileName;

					// Use data folder that contains the external file
					if (PlatformUtil::FileExists(fullFilePath + filePath)) {
						fullFilePath += filePath;
						break;
					}
				}
				else {
					// Split file name to get file and data name in it
					size_t split = ddf.fileName.find_last_of('/');
					if (split == std::string::npos)
						split = ddf.fileName.find_last_of('\\');
					if (split == std::string::npos)
						continue;

					std::string fileName = ddf.fileName.substr(0, split);
					dataName = ddf.fileName.substr(split + 1);

					std::string filePath = df + PathSepStr + fileName;

					// Use data folder that contains the external file
					if (PlatformUtil::FileExists(fullFilePath + filePath)) {
						fullFilePath += filePath;
						break;
					}
				}
			}

			// BSD format
			if (isBSDFile) {
				inDataStorage.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
			}
			// OSD format
			else {
				// Cache data locations
				osdNames[fullFilePath][dataName] = ddf.targetName;
			}
		}
	}

	// Load from cached data locations at once
	inDataStorage.LoadData(osdNames);
}

void SliderSet::Merge(
	SliderSet& mergeSet, DiffDataSets& inDataStorage, DiffDataSets& baseDiffData, const std::string& baseShape, const bool newDataLocal, const bool appendNewSliders) {
	std::map<std::string, std::map<std::string, std::string>> osdNames;
	std::map<std::string, std::map<std::string, std::string>> osdNamesBase;
	std::map<std::string, std::vector<RenamedOSDData>> osdRenamed;
	std::map<std::string, std::vector<RenamedOSDData>> osdRenamedBase;

	auto addSlider = [&](DiffInfo& ddf) {
		if (ddf.fileName.size() <= 4)
			return;

		std::string shapeName = mergeSet.TargetToShape(ddf.targetName);
		bool isBSDFile = ddf.fileName.compare(ddf.fileName.size() - 4, ddf.fileName.size(), ".bsd") == 0;

		std::string fullFilePath = mergeSet.baseDataPath + PathSepStr;
		std::string dataName;

		std::vector<std::string> dataFolders;
		if (!ddf.bLocal)
			dataFolders = mergeSet.GetShapeDataFolders(shapeName);
		else
			dataFolders.push_back(mergeSet.datafolder);

		for (auto& df : dataFolders) {
			if (isBSDFile) {
				std::string filePath = df + PathSepStr + ddf.fileName;

				// Use data folder that contains the external file
				if (PlatformUtil::FileExists(fullFilePath + filePath)) {
					fullFilePath += filePath;
					break;
				}
			}
			else {
				// Split file name to get file and data name in it
				size_t split = ddf.fileName.find_last_of('/');
				if (split == std::string::npos)
					split = ddf.fileName.find_last_of('\\');
				if (split == std::string::npos)
					continue;

				std::string fileName = ddf.fileName.substr(0, split);
				dataName = ddf.fileName.substr(split + 1);

				std::string filePath = df + PathSepStr + fileName;

				// Use data folder that contains the external file
				if (PlatformUtil::FileExists(fullFilePath + filePath)) {
					fullFilePath += filePath;
					break;
				}
			}
		}

		// BSD format
		if (isBSDFile) {
			if (shapeName != baseShape)
				inDataStorage.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
			else
				baseDiffData.LoadSet(ddf.dataName, ddf.targetName, fullFilePath);
		}
		// OSD format
		else if (dataName != ddf.dataName) {
			// Data of renamed shapes is stored under its new name
			auto& renamed = shapeName != baseShape ? osdRenamed : osdRenamedBase;
			renamed[fullFilePath].push_back({dataName, ddf.dataName, ddf.targetName});
		}
		else {
			// Cache data locations
			if (shapeName != baseShape)
				osdNames[fullFilePath][dataName] = ddf.targetName;
			else
				osdNamesBase[fullFilePath][dataName] = ddf.targetName;
		}

		// Make new data local or not
		ddf.bLocal = newDataLocal;
	};

	for (auto& s : mergeSet.sliders) {
		auto sliderIt = std::find_if(sliders.begin(), sliders.end(), [&s](const SliderData& rs) { return rs.name == s.name; });

		if (sliderIt != sliders.end()) {
			// Copy missing data of existing slider
			for (auto& sd : s.dataFiles) {
				auto sliderDataIt = std::find_if(sliderIt->dataFiles.begin(), sliderIt->dataFiles.end(), [&](const DiffInfo& rd) {
					std::string shapeName = TargetToShape(rd.targetName);
					std::string shapeNameMerge = mergeSet.TargetToShape(sd.targetName);
					return rd.targetName == sd.targetName && rd.dataName == sd.dataName && shapeName == shapeNameMerge;
				});

				if (sliderDataIt == sliderIt->dataFiles.end()) {
					size_t df = sliderIt->AddDataFile(sd.targetName, sd.dataName, sd.fileName, sd.bLocal);
					addSlider(sliderIt->dataFiles[df]);
				}
			}
		}
		else if (appendNewSliders) {
			// Copy new slider to the set, it's this project's own now
			sliders.push_back(s);
			sliders.back().ClearInherited();
			for (auto& ddf : sliders.back().dataFiles)
				addSlider(ddf);
		}
	}

	for (auto& s : mergeSet.shapeAttributes) {
		// Copy new shapes to the set
		if (newDataLocal)
			s.second.dataFolders.clear();

		shapeAttributes[s.first] = s.second;
	}

	// Load from cached data locations at once
	inDataStorage.LoadData(osdNames);
	baseDiffData.LoadData(osdNamesBase);
	LoadRenamedOSDData(inDataStorage, osdRenamed);
	LoadRenamedOSDData(baseDiffData, osdRenamedBase);
}

void SliderSet::WriteSliderSet(XMLElement* sliderSetElement) {
	sliderSetElement->DeleteChildren();
	sliderSetElement->SetName(addOn ? SliderSetAddOnElement : SliderSetElement);
	sliderSetElement->SetAttribute("name", name.c_str());

	// Older versions keep the attributes of the element when they save the set
	if (!projectID.empty())
		sliderSetElement->SetAttribute("id", projectID.c_str());
	else
		sliderSetElement->DeleteAttribute("id");

	if (!refProjectFile.empty() || !refProjectName.empty() || !refShapeName.empty()) {
		XMLElement* newElement = sliderSetElement->GetDocument()->NewElement("ReferenceInfo");
		XMLElement* refElement = sliderSetElement->InsertEndChild(newElement)->ToElement();
		if (!refProjectFile.empty()) {
			std::string projectFile_bs = ToBackslashes(refProjectFile);
			refElement->SetAttribute("ProjectFile", projectFile_bs.c_str());
		}
		if (!refProjectName.empty())
			refElement->SetAttribute("ProjectName", refProjectName.c_str());
		if (!refShapeName.empty())
			refElement->SetAttribute("ShapeName", refShapeName.c_str());
	}

	if (HasBaseProject()) {
		XMLElement* baseElement = sliderSetElement->InsertNewChildElement("BaseProject");
		if (!baseProjectFile.empty()) {
			std::string projectFile_bs = ToBackslashes(baseProjectFile);
			baseElement->SetAttribute("ProjectFile", projectFile_bs.c_str());
		}
		baseElement->SetAttribute("ProjectName", baseProjectName.c_str());
		if (!baseProjectID.empty())
			baseElement->SetAttribute("ProjectID", baseProjectID.c_str());
	}

	XMLElement* newElement = sliderSetElement->GetDocument()->NewElement("DataFolder");
	std::string datafolder_bs = ToBackslashes(datafolder);
	XMLText* newText = sliderSetElement->GetDocument()->NewText(datafolder_bs.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("SourceFile");
	newText = sliderSetElement->GetDocument()->NewText(inputfile.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	if (addOn && !sliderDataFile.empty())
		sliderSetElement->InsertNewChildElement("SliderDataFile")->SetText(sliderDataFile.c_str());

	newElement = sliderSetElement->GetDocument()->NewElement("OutputPath");
	std::string outputpath_bs = ToBackslashes(outputpath);
	newText = sliderSetElement->GetDocument()->NewText(outputpath_bs.c_str());
	sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);

	newElement = sliderSetElement->GetDocument()->NewElement("OutputFile");
	XMLElement* outputFileElement = sliderSetElement->InsertEndChild(newElement)->ToElement();

	outputFileElement->SetAttribute("GenWeights", genWeights);
	outputFileElement->SetAttribute("PreventMorphFile", preventMorphFile);
	outputFileElement->SetAttribute("KeepZappedShapes", keepZappedShapes);

	newText = sliderSetElement->GetDocument()->NewText(outputfile.c_str());
	outputFileElement->InsertEndChild(newText);

	if (!sfMorphPath.empty()) {
		newElement = sliderSetElement->GetDocument()->NewElement("SFMorphPath");
		std::string sfMorphPath_bs = ToBackslashes(sfMorphPath);
		newText = sliderSetElement->GetDocument()->NewText(sfMorphPath_bs.c_str());
		sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);
	}

	if (!sfMorphTargetShape.empty()) {
		newElement = sliderSetElement->GetDocument()->NewElement("SFMorphTargetShape");
		newText = sliderSetElement->GetDocument()->NewText(sfMorphTargetShape.c_str());
		sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);
	}

	XMLElement* baseShapeElement;
	XMLElement* sliderElement;
	XMLElement* dataFileElement;

	for (auto& s : shapeAttributes) {
		newElement = sliderSetElement->GetDocument()->NewElement("Shape");
		baseShapeElement = sliderSetElement->InsertEndChild(newElement)->ToElement();

		if (!s.second.targetShape.empty())
			baseShapeElement->SetAttribute("target", s.second.targetShape.c_str());

		if (!s.second.dataFolders.empty()) {
			std::string dataFolder_bs = ToBackslashes(JoinStrings(s.second.dataFolders, ";"));
			baseShapeElement->SetAttribute("DataFolder", dataFolder_bs.c_str());
		}

		if (!s.second.smoothSeamNormals)
			baseShapeElement->SetAttribute("SmoothSeamNormals", s.second.smoothSeamNormals);

		if (s.second.smoothSeamNormals && s.second.smoothSeamNormalsAngle != SliderSetShape::SliderSetDefaultSmoothAngle)
			baseShapeElement->SetAttribute("SmoothSeamNormalsAngle", s.second.smoothSeamNormalsAngle);

		if (s.second.lockNormals)
			baseShapeElement->SetAttribute("LockNormals", s.second.lockNormals);

		if (addOn && s.second.vertexCount > 0)
			baseShapeElement->SetAttribute("vertexCount", s.second.vertexCount);

		newText = sliderSetElement->GetDocument()->NewText(s.first.c_str());
		baseShapeElement->InsertEndChild(newText);
	}

	for (auto& slider : sliders) {
		// Generated data is made anew each time the set is loaded. Toggles need no data of their own.
		if (std::all_of(slider.dataFiles.begin(), slider.dataFiles.end(), [](const DiffInfo& df) { return df.bGenerated; })
			&& (!slider.dataFiles.empty() || !slider.KeptWithoutData()))
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
			for (auto& toggle : slider.zapToggles) {
				zapToggles.append(toggle);
				zapToggles.append(";");
			}

			if (!zapToggles.empty())
				sliderElement->SetAttribute("zaptoggles", zapToggles.c_str());
		}

		if (slider.bUV)
			sliderElement->SetAttribute("uv", true);

		bool isToggle = slider.bToggle && !slider.bZap;
		if (isToggle) {
			sliderElement->SetAttribute("toggle", true);
			sliderElement->SetAttribute("togglevalue", slider.toggleValue);
		}

		if (slider.bInherited && HasBaseProject())
			sliderElement->SetAttribute("inherited", true);

		if (!slider.addOnName.empty())
			sliderElement->SetAttribute("addon", slider.addOnName.c_str());
		if (!slider.addOnID.empty())
			sliderElement->SetAttribute("addonid", slider.addOnID.c_str());

		for (auto& df : slider.dataFiles) {
			if (df.bGenerated)
				continue;

			newElement = sliderSetElement->GetDocument()->NewElement("Data");
			dataFileElement = sliderElement->InsertEndChild(newElement)->ToElement();
			dataFileElement->SetAttribute("name", df.dataName.c_str());
			dataFileElement->SetAttribute("target", df.targetName.c_str());
			if (df.bLocal)
				dataFileElement->SetAttribute("local", true);
			if (df.bInherited && HasBaseProject())
				dataFileElement->SetAttribute("inherited", true);

			std::string fileName_bs = ToBackslashes(df.fileName);
			newText = sliderSetElement->GetDocument()->NewText(fileName_bs.c_str());
			dataFileElement->InsertEndChild(newText);
		}

		// The sliders that a toggle sets while ticked
		if (isToggle) {
			for (auto& target : slider.toggleSliders) {
				XMLElement* setsElement = sliderElement->InsertNewChildElement("Sets");
				setsElement->SetAttribute("slider", target.slider.c_str());
				setsElement->SetAttribute("value", target.value);
			}
		}
	}

	if (HasBaseProject())
		for (auto& removedSlider : removedSliders)
			sliderSetElement->InsertNewChildElement("RemovedSlider")->SetAttribute("name", removedSlider.c_str());

	if (!notes.empty()) {
		newElement = sliderSetElement->GetDocument()->NewElement("Notes");
		newText = sliderSetElement->GetDocument()->NewText(notes.c_str());
		newText->SetCData(true);
		sliderSetElement->InsertEndChild(newElement)->ToElement()->InsertEndChild(newText);
	}

	// Elements of newer versions stay as they were
	for (auto& unknownElement : unknownElements) {
		XMLDocument elementDoc;
		if (elementDoc.Parse(unknownElement.c_str()) != XML_SUCCESS || !elementDoc.RootElement())
			continue;

		sliderSetElement->InsertEndChild(elementDoc.RootElement()->DeepClone(sliderSetElement->GetDocument()));
	}
}

std::string SliderSet::GetInputFileName() const {
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

bool SliderSet::PreventMorphFile() {
	return preventMorphFile;
}

bool SliderSet::KeepZappedShapes() {
	return keepZappedShapes;
}

SliderSetFile::SliderSetFile(const std::string& srcFileName)
	: error(0) {
	Open(srcFileName);
}

void SliderSetFile::Open(const std::string& srcFileName) {
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error || !fp)
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

	// Bring old element names up to date before anything reads the document. The parsing
	// functions rename them in place as well, which would make a second read of the same
	// document find nothing (the version attribute stays at the old value there).
	if (version < 1)
		UpgradeLegacyFormat();

	// Slider add-ons are a different element, so older versions don't list them
	for (XMLElement* setElement = root->FirstChildElement(); setElement; setElement = setElement->NextSiblingElement()) {
		if (!IsSliderSetElement(setElement))
			continue;

		std::string setname = AttributeText(setElement, "name");
		setsInFile[setname] = setElement;
		setsOrder.push_back(setname);
	}
}

void SliderSetFile::UpgradeLegacyFormat() {
	auto renameElements = [](XMLElement* parent, const char* oldName, const char* newName) {
		XMLElement* element = parent->FirstChildElement(oldName);
		while (element) {
			XMLElement* nextElement = element->NextSiblingElement(oldName);
			element->SetName(newName);
			element = nextElement;
		}
	};

	XMLElement* setElement = root->FirstChildElement("SliderSet");
	while (setElement) {
		renameElements(setElement, "SetFolder", "DataFolder");
		renameElements(setElement, "BaseShapeName", "Shape");

		XMLElement* sliderEntry = setElement->FirstChildElement("Slider");
		while (sliderEntry) {
			renameElements(sliderEntry, "datafile", "Data");
			sliderEntry = sliderEntry->NextSiblingElement("Slider");
		}

		setElement = setElement->NextSiblingElement("SliderSet");
	}

	version = 1;
	root->SetAttribute("version", version);
}

void SliderSetFile::New(const std::string& newFileName) {
	version = 1; // Saving raises it when the file needs version 2
	error = 0;
	fileName = newFileName;
	doc.Clear();

	XMLElement* rootElement = doc.NewElement("SliderSetInfo");
	if (version > 0)
		rootElement->SetAttribute("version", version);

	doc.InsertEndChild(rootElement);
	root = doc.FirstChildElement("SliderSetInfo");
}

int SliderSetFile::GetSetNames(std::vector<std::string>& outSetNames, bool append) {
	if (!append)
		outSetNames.clear();

	for (auto& xmlit : setsInFile)
		outSetNames.push_back(xmlit.first);

	return static_cast<int>(outSetNames.size());
}

int SliderSetFile::GetSetNamesUnsorted(std::vector<std::string>& outSetNames, bool append) {
	if (!append) {
		outSetNames.clear();
		outSetNames.assign(setsOrder.begin(), setsOrder.end());
	}
	else
		outSetNames.insert(outSetNames.end(), setsOrder.begin(), setsOrder.end());

	return static_cast<int>(outSetNames.size());
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
		outShapeNames.push_back(ElementText(shapeElement));
		shapeElement = shapeElement->NextSiblingElement("Shape");
	}
}

int SliderSetFile::GetSet(const std::string& setName, SliderSet& outSliderSet, bool appendNewSliders, const std::string& onlyShapeName, bool resolveBase) {
	XMLElement* setPtr;
	if (!HasSet(setName))
		return 1;

	setPtr = setsInFile[setName];

	int ret;
	if (!resolveBase || !setPtr->FirstChildElement("BaseProject")) {
		ret = outSliderSet.LoadSliderSet(setPtr, appendNewSliders, onlyShapeName);
	}
	else {
		SliderSet resolvedSet;
		SliderSetResolver resolver(SliderSetResolver::ProjectPathOfFile(fileName));
		resolver.Resolve(*this, setName, resolvedSet);

		// The merged set is read like a saved one, so that loading into a set with sliders works the same
		XMLDocument resolvedDoc;
		XMLElement* resolvedRoot = resolvedDoc.NewElement("SliderSetInfo");
		resolvedRoot->SetAttribute("version", CurrentVersion);
		resolvedDoc.InsertEndChild(resolvedRoot);

		XMLElement* resolvedElement = resolvedRoot->InsertNewChildElement(SliderSetElement);
		resolvedSet.WriteSliderSet(resolvedElement);

		ret = outSliderSet.LoadSliderSet(resolvedElement, appendNewSliders, onlyShapeName);
		if (onlyShapeName.empty())
			outSliderSet.SetResolveStatus(resolvedSet.GetResolveStatus());
	}

	if (onlyShapeName.empty() && version > CurrentVersion) {
		SliderSetResolveStatus status = outSliderSet.GetResolveStatus();
		status.newerVersion = true;
		outSliderSet.SetResolveStatus(status);
	}

	return ret;
}

bool SliderSetFile::IsAddOn(const std::string& setName) {
	auto setIt = setsInFile.find(setName);
	return setIt != setsInFile.end() && std::strcmp(setIt->second->Name(), SliderSetAddOnElement) == 0;
}

std::string SliderSetFile::GetSetBaseProjectName(const std::string& setName) {
	auto setIt = setsInFile.find(setName);
	if (setIt == setsInFile.end())
		return "";

	XMLElement* baseElement = setIt->second->FirstChildElement("BaseProject");
	if (!baseElement)
		return "";

	return AttributeText(baseElement, "ProjectName");
}

std::string SliderSetFile::GetSetBaseProjectID(const std::string& setName) {
	auto setIt = setsInFile.find(setName);
	if (setIt == setsInFile.end())
		return "";

	XMLElement* baseElement = setIt->second->FirstChildElement("BaseProject");
	return baseElement ? AttributeText(baseElement, "ProjectID") : "";
}

std::string SliderSetFile::GetSetProjectID(const std::string& setName) {
	auto setIt = setsInFile.find(setName);
	return setIt != setsInFile.end() ? AttributeText(setIt->second, "id") : "";
}

void SliderSetFile::GetSetSourceFile(const std::string& setName, std::string& outDataFolder, std::string& outSourceFile) {
	outDataFolder.clear();
	outSourceFile.clear();

	auto setIt = setsInFile.find(setName);
	if (setIt == setsInFile.end())
		return;

	if (XMLElement* element = setIt->second->FirstChildElement("DataFolder"))
		outDataFolder = ToOSSlashes(ElementText(element));
	if (XMLElement* element = setIt->second->FirstChildElement("SourceFile"))
		outSourceFile = ElementText(element);
}

bool SliderSetFile::SetMentions(const std::string& setName, const std::vector<std::string>& lowerNames) {
	auto setIt = setsInFile.find(setName);
	if (setIt == setsInFile.end())
		return false;

	auto mentions = [&lowerNames](const char* text) {
		if (!text)
			return false;

		std::string lowerText = ToLower(text);
		for (auto& name : lowerNames)
			if (lowerText.find(name) != std::string::npos)
				return true;

		return false;
	};

	XMLElement* setElement = setIt->second;
	for (XMLElement* source = setElement->FirstChildElement("SourceFile"); source; source = source->NextSiblingElement("SourceFile"))
		if (mentions(source->GetText()))
			return true;

	for (XMLElement* slider = setElement->FirstChildElement("Slider"); slider; slider = slider->NextSiblingElement("Slider"))
		for (XMLElement* data = slider->FirstChildElement("Data"); data; data = data->NextSiblingElement("Data"))
			if (mentions(data->GetText()))
				return true;

	return false;
}

std::string SliderSetFile::FindSetByID(const std::string& projectID) {
	if (projectID.empty())
		return "";

	for (auto& setName : setsOrder) {
		auto setIt = setsInFile.find(setName);
		if (setIt != setsInFile.end() && AttributeText(setIt->second, "id") == projectID)
			return setName;
	}

	return "";
}

int SliderSetFile::GetAllSets(std::vector<SliderSet>& outAppendSets) {
	int err;
	SliderSet tmpSet;
	for (auto& xmlit : setsInFile) {
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
		outFilePath += ToOSSlashes(ElementText(tmpElement));

	tmpElement = setPtr->FirstChildElement("OutputFile");
	if (tmpElement) {
		outFilePath += PathSepStr;
		outFilePath += ElementText(tmpElement);
	}
}

int SliderSetFile::UpdateSet(SliderSet& inSliderSet) {
	XMLElement* setPtr;
	std::string setName;
	setName = inSliderSet.GetName();
	if (HasSet(setName)) {
		setPtr = setsInFile[setName];
	}
	else {
		XMLElement* tmpElement = doc.NewElement(SliderSetElement);
		tmpElement->SetAttribute("name", setName.c_str());
		setPtr = root->InsertEndChild(tmpElement)->ToElement();
		setsInFile[setName] = setPtr;
		setsOrder.push_back(setName);
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
	if (error || !fp)
		return false;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp) {
		error = errno;
		return false;
	}
#endif

	doc.SetBOM(true);

	// Only files that use what version 2 added get it
	if (version < 2) {
		for (XMLElement* setElement = root->FirstChildElement(); setElement; setElement = setElement->NextSiblingElement()) {
			if (!IsSliderSetElement(setElement))
				continue;

			if (std::strcmp(setElement->Name(), SliderSetAddOnElement) == 0 || setElement->FirstChildElement("BaseProject") || setElement->Attribute("id")) {
				version = 2;
				root->SetAttribute("version", version);
				break;
			}
		}
	}

	const tinyxml2::XMLNode* firstChild = doc.FirstChild();
	if (!firstChild || !firstChild->ToDeclaration())
		doc.InsertFirstChild(doc.NewDeclaration());

	error = doc.SaveFile(fp);
	fclose(fp);
	if (error)
		return false;

	return true;
}
