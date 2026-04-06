/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include <algorithm>
#include <map>
#include <regex>
#include <string>
#include <vector>

enum class AutomationStepType {
	AddCustomBone,
	CopyBoneWeights,
	DeleteBones,
	EditBone,
	RemoveSkinning,
	ExportFile,
	SaveProject,
	ImportFile,
	ImportSliderData,
	AddProject,
	ClearProject,
	ClearReference,
	LoadReference,
	SetBaseShape,
	SetReferenceShape,
	ApplyPose,
	DeleteShape,
	DuplicateShape,
	InvertUVs,
	MirrorShape,
	RefineMesh,
	RenameShape,
	ResetTransforms,
	TransformShape,
	ConformSliders,
	DeleteSlider,
	SetSliderValues,
	SetSliderProperties,
	LoadMask,
	RemoveUnusedNodes
};

constexpr int AutomationStepTypeCount = 30;
static_assert(static_cast<int>(AutomationStepType::RemoveUnusedNodes) + 1 == AutomationStepTypeCount,
	"AutomationStepTypeCount must match the number of enum values");

std::string AutomationStepTypeToString(AutomationStepType type);
AutomationStepType AutomationStepTypeFromString(const std::string& str);

inline std::string JoinStrings(const std::vector<std::string>& vec, const std::string& sep) {
	std::string result;
	for (size_t i = 0; i < vec.size(); i++) {
		if (i > 0)
			result += sep;
		result += vec[i];
	}
	return result;
}

inline std::vector<std::string> SplitCommaSeparated(const std::string& str) {
	std::vector<std::string> result;
	if (str.empty())
		return result;

	size_t start = 0;
	size_t end = str.find(',');
	while (end != std::string::npos) {
		std::string token = str.substr(start, end - start);
		size_t first = token.find_first_not_of(" \t");
		size_t last = token.find_last_not_of(" \t");
		if (first != std::string::npos)
			result.push_back(token.substr(first, last - first + 1));
		start = end + 1;
		end = str.find(',', start);
	}
	std::string token = str.substr(start);
	size_t first = token.find_first_not_of(" \t");
	size_t last = token.find_last_not_of(" \t");
	if (first != std::string::npos)
		result.push_back(token.substr(first, last - first + 1));

	return result;
}

inline bool MatchesFilter(const std::string& name, const std::string& filter, bool useRegex, const std::regex& filterRegex) {
	if (filter.empty())
		return true;

	if (useRegex)
		return std::regex_search(name, filterRegex);

	// Substring match (case-insensitive)
	std::string nameLower = name;
	std::string filterLower = filter;
	std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
	std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
	return nameLower.find(filterLower) != std::string::npos;
}

enum class AutomationBatchMode {
	None,
	FolderScan,
	SliderSets
};

std::string AutomationBatchModeToString(AutomationBatchMode mode);
AutomationBatchMode AutomationBatchModeFromString(const std::string& str);

struct AutomationStep {
	bool active = true;
	std::string note;
	AutomationStepType type = AutomationStepType::LoadReference;
	std::vector<std::string> targetMeshes;
	bool targetRegex = false;

	// LoadReference / AddProject params
	std::string refSourceFile;
	std::string refSet;
	std::string refShape;
	bool refLoadAll = true;
	bool refMergeSliders = true;
	bool refMergeZaps = true;
	bool refAppendNewSliders = true;

	// SetSliderValues params
	std::vector<std::string> setSliderNames;
	float setSliderValue = 1.0f;

	// ConformSliders params
	float conformProximityRadius = 10.0f;
	int conformMaxResults = 10;
	bool conformNoSqueeze = false;
	bool conformSolidMode = false;
	bool conformAxisX = true;
	bool conformAxisY = true;
	bool conformAxisZ = true;
	std::vector<std::string> conformSliderNames;

	// CopyBoneWeights params
	float weightProximityRadius = 10.0f;
	int weightMaxResults = 10;
	std::vector<std::string> weightBoneList;

	// ImportSliderData params
	std::string sliderDataFile;
	bool sliderDataFromFolder = false;
	bool sliderMerge = false;
	std::vector<std::string> sliderNames;

	// ImportFile params
	std::string importFilePath;
	bool importFromFolder = false;

	// TransformShape params
	float moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f;
	float rotateX = 0.0f, rotateY = 0.0f, rotateZ = 0.0f;
	float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
	float inflateX = 0.0f, inflateY = 0.0f, inflateZ = 0.0f;

	// InvertUVs params
	bool invertU = false;
	bool invertV = false;

	// DeleteBones params
	std::vector<std::string> deleteBoneNames;
	bool deleteBoneFromProject = true;

	// AddCustomBone params
	std::string addBoneName;
	std::string addBoneParent;
	float addBoneTransX = 0.0f, addBoneTransY = 0.0f, addBoneTransZ = 0.0f;
	float addBoneRotX = 0.0f, addBoneRotY = 0.0f, addBoneRotZ = 0.0f;

	// EditBone params
	std::string editBoneName;
	std::string editBoneParent;
	float editBoneTransX = 0.0f, editBoneTransY = 0.0f, editBoneTransZ = 0.0f;
	float editBoneRotX = 0.0f, editBoneRotY = 0.0f, editBoneRotZ = 0.0f;

	// ApplyPose params
	std::string poseName;

	// RenameShape params
	std::string renameOldName;
	std::string renameNewName;

	// DeleteSlider params
	std::string deleteSliderName;
	bool deleteSliderRegex = false;

	// SetReferenceShape params
	std::string setRefShapeName;

	// SaveProject params
	std::string saveName;
	std::string saveOutputFileName;
	std::string saveOutputDataPath;
	std::string saveSliderSetFile;
	std::string saveShapeDataFolder;
	std::string saveShapeDataFile;
	bool saveGenWeights = true;
	bool saveAutoCopyRef = true;
	bool saveCopyRefFromProject = false; // Copy reference based on loaded project
	std::string saveCopyRefShapeName; // Only treat this shape name as reference
	bool saveUseOriginal = false; // Use original project from batch mode
	std::string saveReplaceFrom; // Batch: replace this word in original fields
	std::string saveReplaceTo;   // Batch: replace with this word
	std::string saveSuffix;      // Batch: append suffix to original fields

	// DuplicateShape params
	std::string dupNewName;

	// MirrorShape params
	bool mirrorX = true;
	bool mirrorY = false;
	bool mirrorZ = false;
	bool mirrorSwapBonesX = false;

	// ExportFile params
	std::string exportFilePath;
	bool exportWithRef = true;
	bool exportUseOriginalPath = false; // Use original file path from batch mode
	std::string exportPrefix;
	std::string exportSuffix;

	// SetSliderProperties params
	std::vector<std::string> sliderPropNames;
	int sliderPropZap = -1;       // -1 = no change, 0 = false, 1 = true
	int sliderPropHidden = -1;    // -1 = no change, 0 = false, 1 = true
	int sliderPropDefaultLo = -1; // -1 = no change, 0-100 = set value
	int sliderPropDefaultHi = -1; // -1 = no change, 0-100 = set value

	// LoadMask params
	std::string loadMaskFile;
	std::string loadMaskName;
};

class AutomationScript {
	std::vector<AutomationStep> steps;
	std::map<std::string, std::string> variables;

	// Batch settings
	AutomationBatchMode batchMode = AutomationBatchMode::None;
	std::string batchFolder;
	std::string batchExtension;
	bool batchSubdirectories = false;
	std::string batchFileFilter;
	bool batchFileFilterRegex = false;

	// SliderSet batch settings
	std::string batchSliderSetFilter;
	bool batchSliderSetFilterRegex = false;

public:
	AutomationScript() {}

	int Load(const std::string& fileName);
	int Save(const std::string& fileName);

	std::vector<AutomationStep>& GetSteps() { return steps; }
	const std::vector<AutomationStep>& GetSteps() const { return steps; }

	std::map<std::string, std::string>& GetVariables() { return variables; }
	const std::map<std::string, std::string>& GetVariables() const { return variables; }

	AutomationBatchMode GetBatchMode() const { return batchMode; }
	void SetBatchMode(AutomationBatchMode mode) { batchMode = mode; }

	const std::string& GetBatchFolder() const { return batchFolder; }
	void SetBatchFolder(const std::string& folder) { batchFolder = folder; }

	const std::string& GetBatchExtension() const { return batchExtension; }
	void SetBatchExtension(const std::string& ext) { batchExtension = ext; }

	bool GetBatchSubdirectories() const { return batchSubdirectories; }
	void SetBatchSubdirectories(bool v) { batchSubdirectories = v; }

	const std::string& GetBatchFileFilter() const { return batchFileFilter; }
	void SetBatchFileFilter(const std::string& f) { batchFileFilter = f; }

	bool GetBatchFileFilterRegex() const { return batchFileFilterRegex; }
	void SetBatchFileFilterRegex(bool v) { batchFileFilterRegex = v; }

	const std::string& GetBatchSliderSetFilter() const { return batchSliderSetFilter; }
	void SetBatchSliderSetFilter(const std::string& f) { batchSliderSetFilter = f; }

	bool GetBatchSliderSetFilterRegex() const { return batchSliderSetFilterRegex; }
	void SetBatchSliderSetFilterRegex(bool v) { batchSliderSetFilterRegex = v; }

	void AddStep(const AutomationStep& step) { steps.push_back(step); }

	void InsertStep(size_t index, const AutomationStep& step) {
		if (index <= steps.size())
			steps.insert(steps.begin() + index, step);
		else
			steps.push_back(step);
	}

	void RemoveStep(size_t index) {
		if (index < steps.size())
			steps.erase(steps.begin() + index);
	}

	void MoveStepUp(size_t index) {
		if (index > 0 && index < steps.size())
			std::swap(steps[index], steps[index - 1]);
	}

	void MoveStepDown(size_t index) {
		if (index + 1 < steps.size())
			std::swap(steps[index], steps[index + 1]);
	}

	void Clear() { steps.clear(); }

	// Substitute placeholders like {{KEY}} in all string fields
	void SubstitutePlaceholders(const std::map<std::string, std::string>& vars);
};
