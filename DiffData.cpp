/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "DiffData.h"

#include <algorithm>

OSDataFile::OSDataFile() {
	header = 'OSD\0';
	version = 1;
	dataCount = 0;
}

OSDataFile::~OSDataFile() {
}

bool OSDataFile::Read(const string& fileName) {
	ifstream file(fileName, ios_base::binary);
	if (!file)
		return false;

	file.read((char*)&header, 4);
	if (header != 'OSD\0')
		return false;

	file.read((char*)&version, 4);
	file.read((char*)&dataCount, 4);

	byte nameLength;
	string dataName;
	ushort diffSize;
	for (int i = 0; i < dataCount; ++i) {
		file.read((char*)&nameLength, 1);
		dataName.resize(nameLength, ' ');
		file.read((char*)&dataName.front(), nameLength);

		ushort index;
		Vector3 diff;
		unordered_map<ushort, Vector3> diffs;
		file.read((char*)&diffSize, 2);
		for (int j = 0; j < diffSize; ++j) {
			file.read((char*)&index, 2);
			file.read((char*)&diff, sizeof(Vector3));
			diff.clampEpsilon();
			diffs[index] = diff;
		}

		dataDiffs[dataName] = diffs;
	}

	return true;
}

bool OSDataFile::Write(const string& fileName) {
	ofstream file(fileName, ios_base::binary);
	if (!file)
		return false;

	file.write((char*)&header, 4);
	file.write((char*)&version, 4);
	file.write((char*)&dataCount, 4);

	byte nameLength;
	ushort diffSize;
	for (auto &diffs : dataDiffs) {
		nameLength = diffs.first.length();
		file.write((char*)&nameLength, 1);
		file.write(diffs.first.c_str(), nameLength);

		diffSize = diffs.second.size();
		file.write((char*)&diffSize, 2);
		for (auto &diff : diffs.second) {
			file.write((char*)&diff.first, 2);
			file.write((char*)&diff.second, sizeof(Vector3));
		}
	}

	return true;
}

map<string, unordered_map<ushort, Vector3>> OSDataFile::GetDataDiffs() {
	return dataDiffs;
}

void OSDataFile::GetDataDiff(const string& dataName, unordered_map<ushort, Vector3>& outDataDiff) {
	outDataDiff.clear();

	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		outDataDiff = dataDiffs[dataName];
}

void OSDataFile::SetDataDiff(const string& dataName, unordered_map<ushort, Vector3>& inDataDiff) {
	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		dataDiffs.erase(dataName);

	dataDiffs[dataName] = inDataDiff;
	dataCount++;
}


int DiffDataSets::LoadSet(const string& name, const string& target, unordered_map<ushort, Vector3>& inDiffData) {
	if (namedSet.find(name) != namedSet.end())
		namedSet.erase(name);

	namedSet[name] = inDiffData;
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::LoadSet(const string& name, const string& target, const string& fromFile) {
	ifstream inFile(fromFile, ios_base::binary);
	if (!inFile)
		return 1;

	int sz;
	inFile.read((char*)&sz, 4);

	unordered_map<ushort, Vector3> data(sz);
	int idx;
	Vector3 v;
	for (int i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(int));
		inFile.read((char*)&v, sizeof(Vector3));
		v.clampEpsilon();
		data.emplace(idx, v);
	}
	inFile.close();
	if (namedSet.find(name) != namedSet.end())
		namedSet.erase(name);

	namedSet.emplace(name, move(data));
	dataTargets[name] = target;

	return 0;
}

bool DiffDataSets::LoadData(const map<string, map<string, string>>& osdNames) {
	for (auto &osd : osdNames) {
		OSDataFile osdFile;
		if (!osdFile.Read(osd.first))
			return false;

		for (auto &dataNames : osd.second) {
			unordered_map<ushort, Vector3> diff;
			osdFile.GetDataDiff(dataNames.first, diff);
			LoadSet(dataNames.first, dataNames.second, diff);
		}
	}

	return true;
}

int DiffDataSets::SaveSet(const string& name, const string& target, const string& toFile) {
	unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return 2;

	ofstream outFile(toFile, ios_base::binary);
	if (!outFile)
		return 1;

	int sz = data->size();
	outFile.write((char*)&sz, sizeof(int));
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		outFile.write((char*)&resultIt->first, sizeof(int));
		outFile.write((char*)&resultIt->second, sizeof(Vector3));
	}
	return 0;
}

bool DiffDataSets::SaveData(const map<string, map<string, string>>& osdNames) {
	for (auto &osd : osdNames) {
		OSDataFile osdFile;
		for (auto &dataNames : osd.second) {
			unordered_map<ushort, Vector3>* data = &namedSet[dataNames.first];
			if (!TargetMatch(dataNames.first, dataNames.second))
				continue;

			osdFile.SetDataDiff(dataNames.first, *data);
		}

		if (!osdFile.Write(osd.first))
			return false;
	}

	return true;
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
			continue;

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
			continue;

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
			continue;

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

	sort(outIndices.begin(), outIndices.end());
	outIndices.erase(unique(outIndices.begin(), outIndices.end()), outIndices.end());
}

void DiffDataSets::ClearSet(const string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);
}
