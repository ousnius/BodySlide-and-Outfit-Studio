/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "TinyXML-2/tinyxml2.h"
#include "SliderData.h"

#include "../program/NormalGenLayers.h"

using namespace std;
using namespace tinyxml2;

class SliderSet
{
	string name;
	string baseDataPath;		// Base data path - from application configuration.
	string datafolder;			// Default data folder specified for a slider set (overridden by target data folders, usually).
	string inputfile;
	string outputpath;
	string outputfile;
	bool genWeights;			 // Generate both low and high weight meshes on output.
	map<string, string> targetshapenames;	// Target names mapped to nif file shape names.
	map<string, string> targetdatafolders;

	vector<SliderData> sliders;
	vector<NormalGenLayer> defNormalGen;

	SliderData Empty;

public:
	SliderSet();
	SliderSet(XMLElement* sliderSetSource);
	~SliderSet();

	void SetName(const string& newName) {
		name = newName;
	}
	void SetDataFolder(const string& newDataFolder) {
		datafolder = newDataFolder;
	}
	void SetInputFile(const string& newInputFile) {
		inputfile = newInputFile;
	}
	void SetOutputPath(const string& newOutputpath) {
		outputpath = newOutputpath;
	}
	void SetOutputFile(const string& newOutputFile) {
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


	void SetBaseDataPath(const string& inPath) {
		baseDataPath = inPath;
	}

	vector<NormalGenLayer>& GetNormalsGenLayers() {
		return defNormalGen;
	}

	int LoadSliderSet(XMLElement* sliderSetSource);
	void LoadSetDiffData(DiffDataSets& inDataStorage);

	// Add an empty set.
	int CreateSlider(const string& setName);

	int CopySlider(SliderData* other);

	void WriteSliderSet(XMLElement* sliderSetElement);

	void DeleteSlider(const string& setName);

	string GetName() {
		return name;
	}

	string GetInputFileName();
	string GetOutputPath() {
		return outputpath;
	}
	string GetOutputFile() {
		return outputfile;
	}
	string GetOutputFilePath();
	string GetDefaultDataFolder() {
		return datafolder;
	}
	bool GenWeights();

	// Gets the target names in the targetdatafolders map - these are the shapes with non-local or referenced data.
	// Use TargetToShape to get the NIF file shape name.
	void GetReferencedTargets(vector<string> &outTargets) {
		for (auto &tdf : targetdatafolders)
			if (tdf.second != datafolder)
				outTargets.push_back(tdf.first);
	}

	void SetReferencedData(const string& shapeName, const bool local = false) {
		string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName)
					df.bLocal = local;
	}

	void SetReferencedDataByName(const string& shapeName, const string& dataName, const bool local = false) {
		string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName && df.dataName == dataName)
					df.bLocal = local;
	}

	vector<string> GetLocalData(const string& shapeName) {
		vector<string> outDataNames;
		string targetName = ShapeToTarget(shapeName);
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == targetName && df.bLocal)
					outDataNames.push_back(df.dataName);

		return outDataNames;
	}

	string TargetToShape(const string& targetName) {
		if (targetshapenames.find(targetName) != targetshapenames.end())
			return targetshapenames[targetName];

		return "";
	}

	void ClearTargets(const string& oldTarget) {
		targetshapenames.erase(oldTarget);
		targetdatafolders.erase(oldTarget);
	}

	void Retarget(const string& oldTarget, const string& newTarget) {
		for (auto &s : sliders)
			for (auto &df : s.dataFiles)
				if (df.targetName == oldTarget)
					df.targetName = newTarget;
	}

	void AddShapeTarget(const string& shapeName, const string& targetName) {
		targetshapenames[targetName] = shapeName;
	}

	void RenameShape(const string& shapeName, const string& newShapeName) {
		for (auto& tsn : targetshapenames)
			if (tsn.second == shapeName)
				tsn.second = newShapeName;
	}

	void AddTargetDataFolder(const string& targetName, const string& datafolder) {
		targetdatafolders[targetName] = datafolder;
	}

	map<string, string>::iterator TargetShapesBegin() {
		return targetshapenames.begin();
	}
	map<string, string>::iterator TargetShapesEnd() {
		return targetshapenames.end();
	}

	string ShapeToDataName(int index, const string& shapeName) {
		for (auto &tsn : targetshapenames)
			if (tsn.second == shapeName)
				return sliders[index].TargetDataName(tsn.first);

		return "";
	}

	string ShapeToTarget(const string& shapeName) {
		for (auto &tsn : targetshapenames)
			if (tsn.second == shapeName)
				return tsn.first;

		return "";
	}

	string ShapeToDataFolder(const string& shapeName) {
		string t = ShapeToTarget(shapeName);
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

	SliderData& operator [] (const string& sliderName) {
		for (int i = 0; i < sliders.size(); i++)
			if (sliders[i].name == sliderName)
				return sliders[i];

		return Empty;			// Err... sorry... this is bad, but I really like returning references.
	}

	bool SliderExists(const string& sliderName) {
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
	XMLElement* root;
	map<string, XMLElement*> setsInFile;
	vector<string> setsOrder;
	int version;
	int error;

public:
	string fileName;
	SliderSetFile() : error(0) { }
	SliderSetFile(const string& srcFileName);
	~SliderSetFile() { }

	bool fail() {
		return error != 0;
	}
	int GetError() {
		return error;
	}

	// Loads the XML document and identifies included slider set names. On a failure, sets the internal error value.
	void Open(const string& srcFileName);

	// Creates a new empty slider set document structure, ready to add new slider sets to.
	void New(const string& newFileName);

	// Changes the internal file name. The xml file isn't saved until the save() function is used. Note the original 
	// file name is not changed. This method allows you to save a slider set as a new file without altering the original.
	void Rename(const string& newFileName);
	// Returns a list of all the slider sets found in the slider set file.
	int GetSetNames(vector<string>& outSetNames, bool append = true);
	// Returns a list of all the slider sets found in the slider set file in the order they appear.
	int GetSetNamesUnsorted(vector<string>& outSetNames, bool append = true);
	// Returns true if the set name exists.
	bool HasSet(const string& querySetName);

	void SetShapes(const string& set, vector<string>& outShapeNames);

	// Gets a single slider set from the XML document based on the name.
	int GetSet(const string& setName, SliderSet& outSliderSet);
	// Adds all of the slider sets in the file to the supplied slider set vector. Does not clear the vector before doing so.
	int GetAllSets(vector<SliderSet>& outAppendSets);
	// Gets only the output file path for the set
	void GetSetOutputFilePath(const string& setName, string& outFilePath);
	// Updates a slider set in the xml document with the provided set's information.
	// If the set does not already exist in the file (based on name) the set is added.
	int UpdateSet(SliderSet& inSliderSet);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	int Save();
};
