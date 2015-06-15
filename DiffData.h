#pragma once

#include "NifFile.h"

#include <map>
#include <unordered_map>
#include <vector>

using namespace std;

class DiffDataSets {
	map<string, unordered_map<ushort, Vector3>> namedSet;
	map<string, string> dataTargets;

public:
	inline bool TargetMatch(const string& set, const string& target);
	int LoadSet(const string& name, const string& target, unordered_map<ushort, Vector3>& inDiffData);
	int LoadSet(const string& name, const string& target, const string& fromFile);
	int SaveSet(const string& name, const string& target, const string& toFile);
	void RenameSet(const string& oldName, const string& newName);
	void DeepRename(const string& oldName, const string& newName);
	void AddEmptySet(const string& name, const string& target);
	void UpdateDiff(const string& name, const string& target, ushort index, Vector3& newdiff);
	void SumDiff(const string& name, const string& target, ushort index, Vector3& newdiff);
	void ScaleDiff(const string& name, const string& target, float scalevalue);
	void OffsetDiff(const string& name, const string& target, Vector3 &offset);
	void ApplyDiff(const string& set, const string& target, float percent, vector<Vector3>* inOutResult);
	void ApplyUVDiff(const string& set, const string& target, float percent, vector<Vector2>* inOutResult);
	void ApplyClamp(const string& set, const string& target, vector<Vector3>* inOutResult);
	unordered_map<ushort, Vector3>* GetDiffSet(const string& targetDataName);
	void GetDiffIndices(const string& set, const string& target, vector<ushort>& outIndices, float threshold = 0.0f);

	void ClearSet(const string& name);
	void EmptySet(const string& set, const string& target) {
		if (!TargetMatch(set, target))
			return;

		namedSet[set].clear();
	}


	void ZeroVertDiff(const string& set, Vector3* vColorMask) {
		for (auto ns : namedSet[set]) {
			float f = vColorMask[ns.first].x;
			if (f == 1.0f)
				continue;
			else if (f == 0.0f)
				namedSet[set][ns.first] *= 0.0f;
			else
				namedSet[set][ns.first] *= f;
		}
	}

	// Zeroes diffs for the specified verts (or all verts in set if vertSet is null), with an optional mask value. A partially masked vertex will have its diff brought closer to 0,
	// a fully masked vertex will have its diff remain the same and a fully unmasked vert will have its diff erased.
	void ZeroVertDiff(const string& set, const string& target, vector<ushort>* vertSet, unordered_map<ushort, float>* mask) {
		if (!TargetMatch(set, target))
			return;

		vector<ushort> v;
		if (vertSet) {
			v = (*vertSet);
		}
		else {
			for (auto diff : namedSet[set])
				v.push_back(diff.first);
		}

		for (auto i : v) {
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
bool DiffDataSets::TargetMatch(const string& set, const string& target) {
	auto it = dataTargets.find(set);
	if (it != dataTargets.end())
		return it->second == target;

	return false;
}
