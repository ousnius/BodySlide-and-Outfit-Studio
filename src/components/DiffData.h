/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../NIF/utils/Object3d.h"

#include <map>
#include <unordered_map>

struct UndoStateVertexSliderDiff;

class OSDataFile {
	uint header;
	uint version;
	uint dataCount;
	std::unordered_map<std::string, std::unordered_map<ushort, Vector3>> dataDiffs;

public:
	OSDataFile();
	~OSDataFile();

	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);

	std::unordered_map<std::string, std::unordered_map<ushort, Vector3>> GetDataDiffs();
	std::unordered_map<ushort, Vector3>* GetDataDiff(const std::string& dataName);
	void SetDataDiff(const std::string& dataName, std::unordered_map<ushort, Vector3>& inDataDiff);
};

class DiffDataSets {
	std::unordered_map<std::string, std::unordered_map<ushort, Vector3>> namedSet;
	std::map<std::string, std::string> dataTargets;

public:
	inline bool TargetMatch(const std::string& set, const std::string& target);
	int MoveToSet(const std::string& name, const std::string& target, std::unordered_map<ushort, Vector3>& inDiffData);
	int LoadSet(const std::string& name, const std::string& target, const std::unordered_map<ushort, Vector3>& inDiffData);
	int LoadSet(const std::string& name, const std::string& target, const std::string& fromFile);
	int SaveSet(const std::string& name, const std::string& target, const std::string& toFile);
	bool LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames);
	bool SaveData(const std::map<std::string, std::map<std::string, std::string>>& osdNames);
	void RenameSet(const std::string& oldName, const std::string& newName);
	void DeepRename(const std::string& oldName, const std::string& newName);
	void DeepCopy(const std::string& srcName, const std::string& destName);
	void AddEmptySet(const std::string& name, const std::string& target);
	void UpdateDiff(const std::string& name, const std::string& target, ushort index, Vector3& newdiff);
	void SumDiff(const std::string& name, const std::string& target, ushort index, Vector3& newdiff);
	void ScaleDiff(const std::string& name, const std::string& target, float scalevalue);
	void OffsetDiff(const std::string& name, const std::string& target, Vector3 &offset);
	bool ApplyDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector3>* inOutResult);
	bool ApplyUVDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector2>* inOutResult);
	bool ApplyClamp(const std::string& set, const std::string& target, std::vector<Vector3>* inOutResult);
	std::unordered_map<ushort, Vector3>* GetDiffSet(const std::string& targetDataName);
	void GetDiffIndices(const std::string& set, const std::string& target, std::vector<ushort>& outIndices, float threshold = 0.0f);

	// indices must be in ascending order.
	void DeleteVerts(const std::string& target, const std::vector<ushort>& indices);
	// indices must be in ascending order.
	void InsertVertexIndices(const std::string& target, const std::vector<ushort>& indices);
	void GetVertexDiffs(const std::string& target, int vertIndex, std::vector<UndoStateVertexSliderDiff> &diffs);
	void SetVertexDiffs(const std::string& target, int vertIndex, const std::vector<UndoStateVertexSliderDiff> &diffs);

	void ApplyTransformToDiffSet(const std::string &setName, const MatTransform &t);

	void ClearSet(const std::string& name);
	void EmptySet(const std::string& set, const std::string& target) {
		if (!TargetMatch(set, target))
			return;

		namedSet[set].clear();
	}


	void ZeroVertDiff(const std::string& set, Vector3* vColorMask) {
		for (auto &ns : namedSet[set]) {
			float f = vColorMask[ns.first].x;
			if (f == 1.0f)
				continue;
			else if (f == 0.0f)
				ns.second *= 0.0f;
			else
				ns.second *= f;
		}
	}

	// Zeroes diffs for the specified verts (or all verts in set if vertSet is null), with an optional mask value. A partially masked vertex will have its diff brought closer to 0,
	// a fully masked vertex will have its diff remain the same and a fully unmasked vert will have its diff erased.
	void ZeroVertDiff(const std::string& set, const std::string& target, std::vector<ushort>* vertSet, std::unordered_map<ushort, float>* mask) {
		if (!TargetMatch(set, target))
			return;

		std::vector<ushort> v;
		if (vertSet) {
			v = (*vertSet);
		}
		else {
			for (auto &diff : namedSet[set])
				v.push_back(diff.first);
		}

		for (auto &i : v) {
			auto d = namedSet[set].find(i);
			if (d == namedSet[set].end())
				continue;

			float f = 0.0f;
			if (mask) {
				auto m = mask->find(i);
				if (m != mask->end())
					f = m->second;
			}

			if (f == 1.0f)
				continue;

			if (f == 0.0f) {
				namedSet[set].erase(i);
				continue;
			}
			namedSet[set][i] *= f;
		}
	}

	void Clear() {
		namedSet.clear();
		dataTargets.clear();
	}
};

// Set == slider name, target == shape name.
bool DiffDataSets::TargetMatch(const std::string& set, const std::string& target) {
	auto it = dataTargets.find(set);
	if (it != dataTargets.end())
		return it->second == target;

	return false;
}
