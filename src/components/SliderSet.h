/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../TinyXML-2/tinyxml2.h"
#include "SliderData.h"
#include "../components/NormalGenLayers.h"

using namespace tinyxml2;

class SliderSet
{
	std::string name;
	std::string baseDataPath;								// Base data path - from application configuration.
	std::string datafolder;									// Default data folder specified for a slider set (overridden by target data folders, usually).
	std::string inputfile;
	std::string outputpath;
	std::string outputfile;
	bool genWeights = false;								// Generate both low and high weight meshes on output.
	std::map<std::string, std::string> targetshapenames;	// Target names mapped to nif file shape names.
	std::map<std::string, std::string> targetdatafolders;

	std::vector<SliderData> sliders;
	std::vector<NormalGenLayer> defNormalGen;

	SliderData Empty;

public:
	SliderSet();
	SliderSet(XMLElement* sliderSetSource);
	~SliderSet();

	void SetName(const std::string& newName) {
		name = newName;
	}
	void SetDataFolder(const std::string& newDataFolder) {
		datafolder = newDataFolder;
	}
	void SetInputFile(const std::string& newInputFile) {
		inputfile = newInputFile;
	}
	void SetOutputPath(const std::string& newOutputpath) {
		outputpath = newOutputpath;
	}
	void SetOutputFile(const std::string& newOutputFile) {
		outputfile = newOutputFile;
	}
	void SetGenWeights(bool inGenWeights) {
		genWeights = inGenWeights;
	}

	int GetTargetShapeCount() {
		return targetshapenames.size();
	}

	void Clear() {
		targetshapenames.clear();
		targetdatafolders.clear();
		sliders.clear();
	}


	void SetBaseDataPath(const std::string& inPath) {
		baseDataPath = inPath;
	}

	std::vector<NormalGenLayer>& GetNormalsGenLayers() {
		return defNormalGen;
	}

	int LoadSliderSet(XMLElement* sliderSetSource);
	void LoadSetDiffData(DiffDataSets& inDataStorage);

	// Add an empty set.
	int CreateSlider(const std::string& setName);

	int CopySlider(SliderData* other);

	void WriteSliderSet(XMLElement* sliderSetElement);

	void DeleteSlider(const std::string& setName);

	std::string GetName() {
		return name;
	}

	std::string GetInputFileName();
	std::string GetOutputPath() {
		return outputpath;
	}
	std::string GetOutputFile() {
		return outputfile;
	}
	std::string GetOutputFilePath();
	std::string GetDefaultDataFolder() {
		return datafolder;
	}
	bool GenWeights();

	// Gets the target names in the targetdatafolders map - these are the shapes with non-local or referenced data.
	// Use TargetToShape to get the NIF file shape name.
	void GetReferencedTargets(std::vector<std::string> &outTargets) {
		for (auto &tdf : targetdatafolders)
			if (tdf.second != datafolder)
				outTargets.push_back(tdf.first);
	}

	void SetReferencedData(const std::string& shapeName, const bool local = false) {
		std::string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName)
					df.bLocal = local;
	}

	void SetReferencedDataByName(const std::string& shapeName, const std::string& dataName, const bool local = false) {
		std::string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName && df.dataName == dataName)
					df.bLocal = local;
	}

	std::vector<std::string> GetLocalData(const std::string& shapeName) {
		std::vector<std::string> outDataNames;
		std::string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName && df.bLocal)
					outDataNames.push_back(df.dataName);

		return outDataNames;
	}

	std::string TargetToShape(const std::string& targetName) {
		if (targetshapenames.find(targetName) != targetshapenames.end())
			return targetshapenames[targetName];

		return "";
	}

	void ClearTargets(const std::string& oldTarget) {
		targetshapenames.erase(oldTarget);
		targetdatafolders.erase(oldTarget);
	}

	void Retarget(const std::string& oldTarget, const std::string& newTarget) {
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == oldTarget)
					df.targetName = newTarget;
	}

	void AddShapeTarget(const std::string& shapeName, const std::string& targetName) {
		targetshapenames[targetName] = shapeName;
	}

	void RenameShape(const std::string& shapeName, const std::string& newShapeName) {
		std::string target = ShapeToTarget(shapeName);
		if (!target.empty()) {
			if (targetshapenames.find(target) != targetshapenames.end()) {
				targetshapenames[newShapeName] = newShapeName;
				targetshapenames.erase(target);
			}

			if (targetdatafolders.find(target) != targetdatafolders.end()) {
				targetdatafolders[newShapeName] = targetdatafolders[target];
				targetdatafolders.erase(target);
			}
		}

		for (auto& slider : sliders)
			slider.RenameTarget(target, newShapeName);
	}

	void AddTargetDataFolder(const std::string& targetName, const std::string& dataFolder) {
		targetdatafolders[targetName] = dataFolder;
	}

	std::map<std::string, std::string>::iterator TargetShapesBegin() {
		return targetshapenames.begin();
	}
	std::map<std::string, std::string>::iterator TargetShapesEnd() {
		return targetshapenames.end();
	}

	std::string ShapeToDataName(int index, const std::string& shapeName) {
		for (auto &tsn : targetshapenames)
			if (tsn.second == shapeName)
				return sliders[index].TargetDataName(tsn.first);

		return "";
	}

	std::string ShapeToTarget(const std::string& shapeName) {
		for (auto &tsn : targetshapenames)
			if (tsn.second == shapeName)
				return tsn.first;

		return "";
	}

	std::string ShapeToDataFolder(const std::string& shapeName) {
		std::string t = ShapeToTarget(shapeName);
		if (targetdatafolders.find(t) != targetdatafolders.end())
			return targetdatafolders[t];
		else
			return datafolder;
	}

	size_t size() {
		return sliders.size();
	}

	SliderData& operator [] (int idx) {
		return sliders[idx];
	}

	SliderData& operator [] (const std::string& sliderName) {
		for (int i = 0; i < sliders.size(); i++)
			if (sliders[i].name == sliderName)
				return sliders[i];

		return Empty;			// Err... sorry... this is bad, but I really like returning references.
	}

	bool SliderExists(const std::string& sliderName) {
		for (int i = 0; i < sliders.size(); i++)
			if (sliders[i].name == sliderName)
				return true;

		return false;
	}
};

/* Does not manage set information, just provides an interface for loading/saving to specfic set files.
information is maintained in the tinyxml document object, and sets can be retrieved and added/updated to that
document while the slidersetfile object exists.
*/
class SliderSetFile {
	XMLDocument doc;
	XMLElement* root = nullptr;
	std::map<std::string, XMLElement*> setsInFile;
	std::vector<std::string> setsOrder;
	int version = 1;
	int error = 0;

public:
	std::string fileName;

	SliderSetFile() {}
	SliderSetFile(const std::string& srcFileName);

	bool fail() {
		return error != 0;
	}
	int GetError() {
		return error;
	}

	// Loads the XML document and identifies included slider set names. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Creates a new empty slider set document structure, ready to add new slider sets to.
	void New(const std::string& newFileName);

	// Returns a list of all the slider sets found in the slider set file.
	int GetSetNames(std::vector<std::string>& outSetNames, bool append = true);
	// Returns a list of all the slider sets found in the slider set file in the order they appear.
	int GetSetNamesUnsorted(std::vector<std::string>& outSetNames, bool append = true);
	// Returns true if the set name exists.
	bool HasSet(const std::string& querySetName);

	void SetShapes(const std::string& set, std::vector<std::string>& outShapeNames);

	// Gets a single slider set from the XML document based on the name.
	int GetSet(const std::string& setName, SliderSet& outSliderSet);
	// Adds all of the slider sets in the file to the supplied slider set vector. Does not clear the vector before doing so.
	int GetAllSets(std::vector<SliderSet>& outAppendSets);
	// Gets only the output file path for the set
	void GetSetOutputFilePath(const std::string& setName, std::string& outFilePath);
	// Updates a slider set in the xml document with the provided set's information.
	// If the set does not already exist in the file (based on name) the set is added.
	int UpdateSet(SliderSet& inSliderSet);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	int Save();
};
