#pragma once

#include "tinyxml2.h"
#include "DiffData.h"

#include <vector>
#include <map>

using namespace std;
using namespace tinyxml2;

struct DiffDataFile {
	bool bLocal;			// Local files use the slider set's directory for path info. Otherwise, it uses the target's data path.
	string dataName;		// Alias for the data.
	string targetName;		// Shape affected by the data.
	string fileName;		// File name not including path.
	DiffDataFile(bool l = false, const string& dn = "", const string& tn = "", const string& fn = "")
		: bLocal(l), dataName(dn), targetName(tn), fileName(fn) {
	}
};

class TextureVariant {
public:
	// int is the texture channel to change, string is the texture file name.
	// If there is no '\' in the texture file name, it assumes no change in texture path.
	unordered_map<int, string> tOverride;
};

class TextureVariantCollection {
public:
	// Texture variants organized by shape name.
	unordered_map<string, TextureVariant> shapeTexVariants;
};

class SliderData
{
public:
	string name;
	bool bHidden;
	bool bInvert;
	bool bZap;
	bool bClamp;
	bool bUV;			// UV Sliders!? Wat!
	float defSmallValue;
	float defBigValue;

	// Outfit studio values
	float curValue;		// Current slider value.
	bool bShow;			// On to enable this slider when deforming verts.

	vector<DiffDataFile> dataFiles;
	map<string, float> requirements;

	SliderData(const string& inName = "");
	SliderData(XMLElement* element);
	~SliderData();

	// Gets the slider's data record name for the specified target.
	string TargetDataName(const string& targetName) {
		for (auto &df : dataFiles)
			if (df.targetName == targetName)
				return df.dataName;

		return "";
	}
	string DataFileName(const string& targetDataName) {
		for (auto &df : dataFiles)
			if (df.dataName == targetDataName)
				return df.fileName;

		return "";
	}

	// Creates a data file record and returns the record index.  
	int AddDataFile(const string& shapeTarget, const string& dataAlias, const string& fileName, bool localData = true) {
		dataFiles.emplace_back(localData, dataAlias, shapeTarget, fileName);
		return dataFiles.size() - 1;
	}

	bool IsLocalData(const string& dataAlias) {
		for (auto &df : dataFiles)
			if (df.dataName == dataAlias)
				return df.bLocal;

		return false;
	}

	void SetLocalData(const string& dataAlias) {
		for (auto &df : dataFiles)
			if (df.dataName == dataAlias)
				df.bLocal = true;
	}

	void Clear() {
		dataFiles.clear();
		requirements.clear();
	}
	void ClearDataFiles() {
		dataFiles.clear();
	}
	int LoadSliderData(XMLElement* srcdata, set<string>* exclude_targets = nullptr);
};


#define LOADSS_REFERENCE 1		// Excludes targets from slider set that are referenced.
#define LOADSS_DIRECT    2		// Excludes targets from slider set that are referenced.
#define LOADSS_ADDEXCLUDED 4	// Creates an empty slider entry when an excluded target doesn't have a data file for a particular slider.

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
	map<string, Vector3> targetoffsets;		// Display offset for bodyslide (emulates bone offsets).

	vector<SliderData> sliders;

	// Texture variants by variant name.
	unordered_map <string, TextureVariantCollection> texVariants;

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

	int LoadSliderSet(XMLElement* sliderSetSource, uint flags = LOADSS_REFERENCE | LOADSS_DIRECT);
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

	string TargetToShape(const string& targetName) {
		if (targetshapenames.find(targetName) != targetshapenames.end())
			return targetshapenames[targetName];

		return "";
	}

	// Adds a shape target and also ensures that any diff data in the set knows about the shape target.
	void LinkShapeTarget(const string& shapeName, const string& targetName) {
		targetshapenames[targetName] = shapeName;
		for (auto &&s : sliders)
			for (auto &df : s.dataFiles)
				df.targetName = targetName;
	}

	void ClearTargets(const string& oldTarget) {
		targetshapenames.erase(oldTarget);
		targetdatafolders.erase(oldTarget);
		targetoffsets.erase(oldTarget);
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

	void AddTargetVirtualOffset(const string& targetName, const Vector3& offset) {
		targetoffsets[targetName] = offset;
	}

	Vector3 GetTargetVirtualOffset(const string& targetName) {
		if (targetoffsets.find(targetName) != targetoffsets.end())
			return targetoffsets[targetName];

		else return Vector3(0.0f, 0.0f, 0.0f);
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
	XMLDoc doc;
	XMLElement* root;
	map<string, XMLElement*> setsInFile;
	vector<string> setsOrder;
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
	void SetTargets(const string& set, vector<string>& outTargetNames);

	// Adds all of the slider sets in the file to the supplied slider set vector. Does not clear the vector before doing so.
	int GetAllSets(vector<SliderSet>& outAppendSets);
	// Gets a single slider set from the XML document based on the name.
	int GetSet(const string& setName, SliderSet& outSliderSet, uint flags = LOADSS_DIRECT | LOADSS_REFERENCE);
	// Updates a slider set in the xml document with the provided set's information.
	// If the set does not already exist in the file (based on name) the set is added.
	int UpdateSet(SliderSet& inSliderSet);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	int Save();
};


#undef small

class SliderPreset {
public:
	float big;
	float small;
};

class PresetCollection {
	map<string, map<string, SliderPreset>> namedSliderPresets;
public:

	void Clear();

	void GetPresetNames(vector<string>& outNames);
	void SetSliderPreset(const string& set, const string& slider, float big = -10000.0f, float small = -10000.0f);
	bool GetSliderExists(const string& set, const string& slider);
	bool GetBigPreset(const string& set, const string& slider, float& big);
	bool GetSmallPreset(const string& set, const string& slider, float& small);
	bool LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilter, bool allPresets = false);

	int SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups);
};
