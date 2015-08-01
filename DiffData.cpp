#include "DiffData.h"

#include <algorithm>

int DiffDataSets::LoadSet(const string& name, const string& target, unordered_map<ushort, Vector3>& inDiffData) {
	if (namedSet.find(name) != namedSet.end())
		namedSet.erase(name);

	namedSet[name] = inDiffData;
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::LoadSet(const string& name, const string& target, const string& fromFile) {
	fstream inFile(fromFile.c_str(), ios_base::in | ios_base::binary);
	if (!inFile.is_open())
		return 1;

	int sz;
	inFile.read((char*)&sz, 4);

	unordered_map<ushort, Vector3> data(sz);
	int idx;
	Vector3 v;
	for (int i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(int));
		inFile.read((char*)&v, sizeof(Vector3));
		data.emplace(idx, v);
	}
	inFile.close();
	if (namedSet.find(name) != namedSet.end())
		namedSet.erase(name);

	namedSet.emplace(name, move(data));
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::SaveSet(const string& name, const string& target, const string& toFile) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return 1;

	fstream outFile(toFile.c_str(), ios_base::out | ios_base::binary);
	if (!outFile.is_open())
		return 2;

	int sz = data->size();
	outFile.write((char*)&sz, sizeof(int));
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		outFile.write((char*)&resultIt->first, sizeof(int));
		outFile.write((char*)&resultIt->second, sizeof(Vector3));
	}
	return 0;
}

void DiffDataSets::RenameSet(const string& oldName, const string& newName) {
	if (namedSet.find(oldName) != namedSet.end()) {
		namedSet.emplace(newName, namedSet[oldName]);
		namedSet.erase(oldName);
		dataTargets[newName] = dataTargets[oldName];
		dataTargets.erase(oldName);
	}
}

void DiffDataSets::DeepRename(const string& oldName, const string& newName) {
	vector<string> oldTargets;
	vector<string> newTargets;
	string newDT = "";
	for (auto& dt : dataTargets) {
		if (dt.second == oldName && dt.first.length() >= oldName.length()) {
			oldTargets.push_back(dt.first);
			newDT = dt.first.substr(oldName.length());
			newDT = newName + newDT;
			newTargets.push_back(newDT);
			dt.second = newName;
		}
	}
	for (int i = 0; i < oldTargets.size(); i++) {
		string ot = oldTargets[i];
		string nt = newTargets[i];
		if (dataTargets.find(ot) != dataTargets.end()) {
			dataTargets[nt] = dataTargets[ot];
			dataTargets.erase(ot);
		}
		if (namedSet.find(ot) != namedSet.end()) {
			namedSet[nt] = move(namedSet[ot]);
			namedSet.erase(ot);
		}
	}
}

void DiffDataSets::AddEmptySet(const string &name, const string &target) {
	if (namedSet.find(name) == namedSet.end()) {
		unordered_map<ushort, Vector3> data;
		namedSet[name] = data;
		dataTargets[name] = target;
	}
}

void DiffDataSets::UpdateDiff(const string& name, const string& target, ushort index, Vector3 &newdiff) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const string& name, const string& target, ushort index, Vector3 &newdiff) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	Vector3 v = (*data)[index];
	v += newdiff;
	(*data)[index] = v;
}

void DiffDataSets::ScaleDiff(const string& name, const string& target, float scalevalue) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];

	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second *= scalevalue;
}

void DiffDataSets::OffsetDiff(const string& name, const string& target, Vector3 &offset) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second += offset;
}

void DiffDataSets::ApplyUVDiff(const string& set, const string& target, float percent, vector<Vector2>* inOutResult) {
	if (percent == 0)
		return;

	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue; // prevent crashes.

		(*inOutResult)[resultIt->first].u += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].v += resultIt->second.y * percent;
	}
}

void DiffDataSets::ApplyDiff(const string& set, const string& target, float percent, vector<Vector3>* inOutResult) {
	if (percent == 0)
		return;

	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue; // prevent crashes.

		(*inOutResult)[resultIt->first].x += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].y += resultIt->second.y * percent;
		(*inOutResult)[resultIt->first].z += resultIt->second.z * percent;
	}
}

void DiffDataSets::ApplyClamp(const string& set, const string& target, vector<Vector3>* inOutResult) {
	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue; // prevent crashes.

		(*inOutResult)[resultIt->first].x = resultIt->second.x;
		(*inOutResult)[resultIt->first].y = resultIt->second.y;
		(*inOutResult)[resultIt->first].z = resultIt->second.z;
	}
}

unordered_map<ushort, Vector3>* DiffDataSets::GetDiffSet(const string& targetDataName) {
	if (namedSet.find(targetDataName) == namedSet.end())
		return nullptr;

	return &namedSet[targetDataName];
}

void DiffDataSets::GetDiffIndices(const string& set, const string& target, vector<ushort>& outIndices, float threshold) {
	if (!TargetMatch(set, target))
		return;

	unordered_map<ushort, Vector3>* data = &namedSet[set];
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (fabs(resultIt->second.x) > threshold ||
			fabs(resultIt->second.y) > threshold ||
			fabs(resultIt->second.z) > threshold) {
			outIndices.push_back(resultIt->first);
		}
	}
	std::sort(outIndices.begin(), outIndices.end());
	std::unique(outIndices.begin(), outIndices.end());
}

void DiffDataSets::ClearSet(const string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);
}
