/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "DiffData.h"

#include <algorithm>
#include <fstream>

OSDataFile::OSDataFile() {
	header = 'OSD\0';
	version = 1;
	dataCount = 0;
}

OSDataFile::~OSDataFile() {
}

bool OSDataFile::Read(const std::string& fileName) {
	std::ifstream file(fileName, std::ios_base::binary);
	if (!file)
		return false;

	file.read((char*)&header, 4);
	if (header != 'OSD\0')
		return false;

	file.read((char*)&version, 4);
	file.read((char*)&dataCount, 4);

	byte nameLength;
	std::string dataName;
	ushort diffSize;
	for (int i = 0; i < dataCount; ++i) {
		file.read((char*)&nameLength, 1);
		dataName.resize(nameLength, ' ');
		file.read((char*)&dataName.front(), nameLength);

		ushort index;
		Vector3 diff;
		std::unordered_map<ushort, Vector3> diffs;

		file.read((char*)&diffSize, 2);
		diffs.reserve(diffSize);
		for (int j = 0; j < diffSize; ++j) {
			file.read((char*)&index, 2);
			file.read((char*)&diff, sizeof(Vector3));
			diff.clampEpsilon();
			diffs.emplace(index, diff);
		}

		dataDiffs[dataName] = move(diffs);
	}

	return true;
}

bool OSDataFile::Write(const std::string& fileName) {
	std::ofstream file(fileName, std::ios_base::binary);
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

std::map<std::string, std::unordered_map<ushort, Vector3>> OSDataFile::GetDataDiffs() {
	return dataDiffs;
}

std::unordered_map<ushort, Vector3>* OSDataFile::GetDataDiff(const std::string& dataName) {
	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		return &dataDiffs[dataName];

	return nullptr;
}

void OSDataFile::SetDataDiff(const std::string& dataName, std::unordered_map<ushort, Vector3>& inDataDiff) {
	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		dataDiffs.erase(dataName);

	dataDiffs[dataName] = inDataDiff;
	dataCount++;
}


int DiffDataSets::LoadSet(const std::string& name, const std::string& target, std::unordered_map<ushort, Vector3>& inDiffData) {
	if (namedSet.find(name) != namedSet.end())
		namedSet.erase(name);

	namedSet[name] = inDiffData;
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::LoadSet(const std::string& name, const std::string& target, const std::string& fromFile) {
	std::ifstream inFile(fromFile, std::ios_base::binary);
	if (!inFile)
		return 1;

	int sz;
	inFile.read((char*)&sz, 4);

	std::unordered_map<ushort, Vector3> data;
	data.reserve(sz);

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

bool DiffDataSets::LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
	for (auto &osd : osdNames) {
		OSDataFile osdFile;
		if (!osdFile.Read(osd.first))
			return false;

		for (auto &dataNames : osd.second) {
			auto diff = osdFile.GetDataDiff(dataNames.first);
			if (diff)
				LoadSet(dataNames.first, dataNames.second, *diff);
		}
	}

	return true;
}

int DiffDataSets::SaveSet(const std::string& name, const std::string& target, const std::string& toFile) {
	std::unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return 2;

	std::ofstream outFile(toFile, std::ios_base::binary);
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

bool DiffDataSets::SaveData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
	for (auto &osd : osdNames) {
		OSDataFile osdFile;
		for (auto &dataNames : osd.second) {
			std::unordered_map<ushort, Vector3>* data = &namedSet[dataNames.first];
			if (!TargetMatch(dataNames.first, dataNames.second))
				continue;

			osdFile.SetDataDiff(dataNames.first, *data);
		}

		if (!osdFile.Write(osd.first))
			return false;
	}

	return true;
}

void DiffDataSets::RenameSet(const std::string& oldName, const std::string& newName) {
	if (namedSet.find(oldName) != namedSet.end()) {
		namedSet.emplace(newName, namedSet[oldName]);
		namedSet.erase(oldName);
		dataTargets[newName] = dataTargets[oldName];
		dataTargets.erase(oldName);
	}
}

void DiffDataSets::DeepRename(const std::string& oldName, const std::string& newName) {
	std::vector<std::string> oldTargets;
	std::vector<std::string> newTargets;
	std::string newDT = "";
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
		std::string ot = oldTargets[i];
		std::string nt = newTargets[i];
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

void DiffDataSets::AddEmptySet(const std::string& name, const std::string& target) {
	if (namedSet.find(name) == namedSet.end()) {
		std::unordered_map<ushort, Vector3> data;
		namedSet[name] = data;
		dataTargets[name] = target;
	}
}

void DiffDataSets::UpdateDiff(const std::string& name, const std::string& target, ushort index, Vector3 &newdiff) {
	std::unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const std::string& name, const std::string& target, ushort index, Vector3 &newdiff) {
	std::unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	Vector3 v = (*data)[index];
	v += newdiff;
	(*data)[index] = v;
}

void DiffDataSets::ScaleDiff(const std::string& name, const std::string& target, float scalevalue) {
	std::unordered_map<ushort, Vector3>* data = &namedSet[name];

	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second *= scalevalue;
}

void DiffDataSets::OffsetDiff(const std::string& name, const std::string& target, Vector3 &offset) {
	std::unordered_map<ushort, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second += offset;
}

void DiffDataSets::ApplyUVDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector2>* inOutResult) {
	if (percent == 0)
		return;

	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	std::unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].u += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].v += resultIt->second.y * percent;
	}
}

void DiffDataSets::ApplyDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector3>* inOutResult) {
	if (percent == 0)
		return;

	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	std::unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].x += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].y += resultIt->second.y * percent;
		(*inOutResult)[resultIt->first].z += resultIt->second.z * percent;
	}
}

void DiffDataSets::ApplyClamp(const std::string& set, const std::string& target, std::vector<Vector3>* inOutResult) {
	if (!TargetMatch(set, target))
		return;

	int maxidx = (*inOutResult).size();
	std::unordered_map<ushort, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].x = resultIt->second.x;
		(*inOutResult)[resultIt->first].y = resultIt->second.y;
		(*inOutResult)[resultIt->first].z = resultIt->second.z;
	}
}

std::unordered_map<ushort, Vector3>* DiffDataSets::GetDiffSet(const std::string& targetDataName) {
	if (namedSet.find(targetDataName) == namedSet.end())
		return nullptr;

	return &namedSet[targetDataName];
}

void DiffDataSets::GetDiffIndices(const std::string& set, const std::string& target, std::vector<ushort>& outIndices, float threshold) {
	if (!TargetMatch(set, target))
		return;

	std::unordered_map<ushort, Vector3>* data = &namedSet[set];
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (fabs(resultIt->second.x) > threshold ||
			fabs(resultIt->second.y) > threshold ||
			fabs(resultIt->second.z) > threshold) {
			outIndices.push_back(resultIt->first);
		}
	}

	std::sort(outIndices.begin(), outIndices.end());
	outIndices.erase(std::unique(outIndices.begin(), outIndices.end()), outIndices.end());
}

void DiffDataSets::DeleteVerts(const std::string& target, const std::vector<ushort>& indices) {
	if (indices.empty())
		return;

	ushort highestRemoved = indices.back();
	std::vector<int> indexCollapse(highestRemoved + 1, 0);

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < indices.size() && indices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;						// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	for (auto &data : namedSet) {
		if (TargetMatch(data.first, target)) {
			std::unordered_map<ushort, Vector3> indexCopy;
			for (auto &d : data.second) {
				if (d.first > highestRemoved)
					indexCopy.emplace(d.first - remCount, d.second);
				else if (indexCollapse[d.first] != -1)
					indexCopy.emplace(d.first - indexCollapse[d.first], d.second);
			}

			data.second.clear();
			data.second.reserve(indexCopy.size());
			for (auto &copy : indexCopy)
				data.second[copy.first] = std::move(copy.second);
		}
	}
}

void DiffDataSets::ClearSet(const std::string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);
}
