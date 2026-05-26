/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct UndoStateVertexSliderDiff;

struct TargetDataDiff {
	uint16_t index = 0;
	nifly::Vector3 diff;
};

typedef std::unordered_map<uint16_t, nifly::Vector3> TargetDataDiffs;
typedef std::vector<TargetDataDiff> TargetDataDiffList;
typedef std::unordered_map<std::string, std::unique_ptr<TargetDataDiffs>> TargetData;
typedef std::unordered_map<std::string, std::unique_ptr<TargetDataDiffList>> TargetDataDiffLists;

class OSDataFile {
	uint32_t version;
	TargetData dataDiffs;
	TargetDataDiffLists dataDiffLists;

	TargetDataDiffs* MaterializeDataDiff(const std::string& dataName);
	void MaterializeDataDiffs();

public:
	OSDataFile();
	~OSDataFile();

	bool Read(const std::string& fileName, const std::map<std::string, std::string>* dataNames = nullptr);
	bool Write(const std::string& fileName);

	TargetData& GetDataDiffs();
	std::unique_ptr<TargetDataDiffs>* GetDataDiff(const std::string& dataName);
	void SetDataDiff(const std::string& dataName, const TargetDataDiffs& inDataDiff);
};

class DiffDataSets {
	struct DataSet {
		std::string target;
		std::unique_ptr<TargetDataDiffs> diffs;
		std::unique_ptr<TargetDataDiffList> linearDiffs;
	};

	std::unordered_map<std::string, DataSet> namedSet;

	TargetDataDiffs* MaterializeSet(DataSet& dataSet);
	TargetDataDiffs* MaterializeSet(const std::string& name);

public:
	inline bool TargetMatch(const std::string& set, const std::string& target);
	void MoveToSet(const std::string& name, const std::string& target, std::unique_ptr<TargetDataDiffs>& inDiffData);
	void LoadSet(const std::string& name, const std::string& target, const TargetDataDiffs& inDiffData);
	int LoadSet(const std::string& name, const std::string& target, const std::string& fromFile);
	int SaveSet(const std::string& name, const std::string& target, const std::string& toFile);
	bool LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames);
	bool SaveData(const std::map<std::string, std::map<std::string, std::string>>& osdNames);

	void RenameSet(const std::string& oldName, const std::string& newName);
	void RenameDataTarget(const std::string& oldTarget, const std::string& newTarget);
	void CopySet(const std::string& oldName, const std::string& newName, const std::string& newTargetName);
	std::string GetDataTargetName(const std::string& targetName, const std::string& dataNameSuffix);

	void AddEmptySet(const std::string& name, const std::string& target);
	void UpdateDiff(const std::string& name, const std::string& target, uint16_t index, const nifly::Vector3& newdiff);
	void SumDiff(const std::string& name, const std::string& target, uint16_t index, const nifly::Vector3& newdiff);
	void ScaleDiff(const std::string& name, const std::string& target, float scalevalue);
	void OffsetDiff(const std::string& name, const std::string& target, const nifly::Vector3& offset);
	bool ApplyDiff(const std::string& set, const std::string& target, float percent, std::vector<nifly::Vector3>* inOutResult);
	bool ApplyUVDiff(const std::string& set, const std::string& target, float percent, std::vector<nifly::Vector2>* inOutResult);
	bool ApplyClamp(const std::string& set, const std::string& target, std::vector<nifly::Vector3>* inOutResult);
	TargetDataDiffs* GetDiffSet(const std::string& targetDataName);
	void GetDiffIndices(const std::string& set, const std::string& target, std::vector<uint16_t>& outIndices, float threshold = 0.0f);

	// indices must be in ascending order.
	void DeleteVerts(const std::string& target, const std::vector<uint16_t>& indices);
	// indices must be in ascending order.
	void InsertVertexIndices(const std::string& target, const std::vector<uint16_t>& indices);
	void ClearSet(const std::string& name);
	void EmptySet(const std::string& set, const std::string& target);


	void ZeroVertDiff(const std::string& set, int vertCount, float* vColorMask);

	// Zeroes diffs for the specified verts (or all verts in set if vertSet is null), with an optional mask value. A partially masked vertex will have its diff brought closer to 0,
	// a fully masked vertex will have its diff remain the same and a fully unmasked vert will have its diff erased.
	void ZeroVertDiff(const std::string& set, const std::string& target, std::vector<uint16_t>* vertSet, std::unordered_map<uint16_t, float>* mask);

	void Clear() {
		namedSet.clear();
	}
};

// Set == slider name, target == shape name.
bool DiffDataSets::TargetMatch(const std::string& set, const std::string& target) {
	auto it = namedSet.find(set);
	if (it != namedSet.end())
		return it->second.target == target;

	return false;
}
