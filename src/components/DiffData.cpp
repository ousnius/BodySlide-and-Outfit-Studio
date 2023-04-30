/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "DiffData.h"
#include "../utils/PlatformUtil.h"
#include "NifUtil.hpp"
#include "UndoState.h"

#include <algorithm>
#include <fstream>

#ifdef WIN64
#include <concurrent_unordered_map.h>
#include <ppl.h>
#else
#undef _PPL_H
#endif

using namespace nifly;

OSDataFile::OSDataFile() {
	header = "OSD\0"_mci;
	version = 1;
	dataCount = 0;
}

OSDataFile::~OSDataFile() {}

#pragma pack(push, 1)
struct DiffStruct {
	uint16_t index = 0;
	Vector3 diff;
};
#pragma pack(pop)

bool OSDataFile::Read(const std::string& fileName) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	if (!file)
		return false;

	file.read((char*)&header, 4);
	if (header != "OSD\0"_mci)
		return false;

	file.read((char*)&version, 4);
	file.read((char*)&dataCount, 4);
	dataDiffs.reserve(dataCount);

	uint8_t nameLength;
	std::string dataName;
	uint16_t diffSize;
	for (uint32_t i = 0; i < dataCount; ++i) {
		file.read((char*)&nameLength, 1);
		dataName.resize(nameLength, ' ');
		file.read((char*)&dataName.front(), nameLength);

		std::unordered_map<uint16_t, Vector3> diffs;
		file.read((char*)&diffSize, 2);
		diffs.reserve(diffSize);

		std::vector<DiffStruct> diffData(diffSize);
		file.read((char*)diffData.data(), diffSize * sizeof(DiffStruct));

		for (int j = 0; j < diffSize; ++j) {
			auto& diffEntry = diffData[j];
			diffEntry.diff.clampEpsilon();
			diffs.emplace(diffEntry.index, std::move(diffEntry.diff));
		}

		dataDiffs.emplace(dataName, std::move(diffs));
	}

	return true;
}

bool OSDataFile::Write(const std::string& fileName) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	if (!file)
		return false;

	file.write((char*)&header, 4);
	file.write((char*)&version, 4);
	file.write((char*)&dataCount, 4);

	uint8_t nameLength;
	uint16_t diffSize;
	for (auto& diffs : dataDiffs) {
		nameLength = static_cast<uint8_t>(diffs.first.length());
		file.write((char*)&nameLength, 1);
		file.write(diffs.first.c_str(), nameLength);

		diffSize = static_cast<uint16_t>(diffs.second.size());

		std::vector<DiffStruct> diffData(diffSize);
		diffData.resize(diffSize);

		size_t i = 0;

		for (auto& diff : diffs.second) {
			if (i >= diffSize)
				break;

			diffData[i].index = diff.first;
			diffData[i].diff = diff.second;
			++i;
		}

		file.write((char*)&diffSize, 2);
		file.write((char*)diffData.data(), diffSize * sizeof(DiffStruct));
	}

	return true;
}

std::unordered_map<std::string, std::unordered_map<uint16_t, Vector3>> OSDataFile::GetDataDiffs() {
	return dataDiffs;
}

std::unordered_map<uint16_t, Vector3>* OSDataFile::GetDataDiff(const std::string& dataName) {
	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		return &it->second;

	return nullptr;
}

void OSDataFile::SetDataDiff(const std::string& dataName, std::unordered_map<uint16_t, Vector3>& inDataDiff) {
	dataDiffs[dataName] = inDataDiff;
	dataCount++;
}


void DiffDataSets::MoveToSet(const std::string& name, const std::string& target, std::unordered_map<uint16_t, Vector3>& inDiffData) {
	namedSet[name] = std::move(inDiffData);
	dataTargets[name] = target;
}

void DiffDataSets::LoadSet(const std::string& name, const std::string& target, const std::unordered_map<uint16_t, Vector3>& inDiffData) {
	namedSet[name] = inDiffData;
	dataTargets[name] = target;
}

int DiffDataSets::LoadSet(const std::string& name, const std::string& target, const std::string& fromFile) {
	std::fstream inFile;
	PlatformUtil::OpenFileStream(inFile, fromFile, std::ios::in | std::ios::binary);

	if (!inFile)
		return 1;

	uint32_t sz;
	inFile.read((char*)&sz, 4);

	std::unordered_map<uint16_t, Vector3> data;
	data.reserve(sz);

	uint32_t idx;
	Vector3 v;
	for (uint32_t i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(uint32_t));
		inFile.read((char*)&v, sizeof(Vector3));
		v.clampEpsilon();
		data.emplace(static_cast<uint16_t>(idx), v);
	}

	inFile.close();

	namedSet[name] = std::move(data);
	dataTargets[name] = target;

	return 0;
}

bool DiffDataSets::LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
#ifdef _PPL_H
	Concurrency::concurrent_unordered_map<std::string, OSDataFile> loaded;
	Concurrency::parallel_for_each(osdNames.begin(), osdNames.end(), [&](auto& osd) {
		OSDataFile osdFile;
		if (!osdFile.Read(osd.first))
			return;
		loaded[osd.first] = std::move(osdFile);
	});
#endif
	for (auto& osd : osdNames) {
#ifdef _PPL_H
		auto kvp = loaded.find(osd.first);
		if (kvp == loaded.end())
			continue;
		auto& osdFile = kvp->second;
#else
		OSDataFile osdFile;
		if (!osdFile.Read(osd.first))
			continue;
#endif
		for (auto& dataNames : osd.second) {
			auto diff = osdFile.GetDataDiff(dataNames.first);
			if (diff)
				MoveToSet(dataNames.first, dataNames.second, *diff);
		}
	}
	return true;
}

int DiffDataSets::SaveSet(const std::string& name, const std::string& target, const std::string& toFile) {
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return 2;

	std::fstream outFile;
	PlatformUtil::OpenFileStream(outFile, toFile, std::ios::out | std::ios::binary);

	if (!outFile)
		return 1;

	uint32_t sz = static_cast<uint32_t>(data->size());
	outFile.write((char*)&sz, sizeof(uint32_t));

	uint32_t idx = 0;
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		idx = static_cast<uint32_t>(resultIt->first);
		outFile.write((char*)&idx, sizeof(uint32_t));
		outFile.write((char*)&resultIt->second, sizeof(Vector3));
	}
	return 0;
}

bool DiffDataSets::SaveData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
	for (auto& osd : osdNames) {
		OSDataFile osdFile;
		for (auto& dataNames : osd.second) {
			auto& data = namedSet[dataNames.first];
			if (!TargetMatch(dataNames.first, dataNames.second))
				continue;

			osdFile.SetDataDiff(dataNames.first, data);
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
	for (size_t i = 0; i < oldTargets.size(); i++) {
		std::string ot = oldTargets[i];
		std::string nt = newTargets[i];
		if (dataTargets.find(ot) != dataTargets.end()) {
			dataTargets[nt] = dataTargets[ot];
			dataTargets.erase(ot);
		}
		if (namedSet.find(ot) != namedSet.end()) {
			namedSet[nt] = std::move(namedSet[ot]);
			namedSet.erase(ot);
		}
	}
}

void DiffDataSets::DeepCopy(const std::string& srcName, const std::string& destName) {
	std::vector<std::string> oldTargets;
	std::vector<std::string> newTargets;
	std::string newDT = "";

	for (auto& dt : dataTargets) {
		if (dt.second == srcName && dt.first.length() >= srcName.length()) {
			oldTargets.push_back(dt.first);
			newDT = dt.first.substr(srcName.length());
			newDT = destName + newDT;
			newTargets.push_back(newDT);
		}
	}

	for (size_t i = 0; i < oldTargets.size(); i++) {
		std::string ot = oldTargets[i];
		std::string nt = newTargets[i];
		dataTargets[nt] = destName;

		if (namedSet.find(ot) != namedSet.end())
			namedSet[nt] = namedSet[ot];
	}
}

void DiffDataSets::AddEmptySet(const std::string& name, const std::string& target) {
	if (namedSet.find(name) == namedSet.end()) {
		std::unordered_map<uint16_t, Vector3> data;
		namedSet[name] = data;
		dataTargets[name] = target;
	}
}

void DiffDataSets::UpdateDiff(const std::string& name, const std::string& target, uint16_t index, Vector3& newdiff) {
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const std::string& name, const std::string& target, uint16_t index, Vector3& newdiff) {
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	Vector3 v = (*data)[index];
	v += newdiff;
	(*data)[index] = v;
}

void DiffDataSets::ScaleDiff(const std::string& name, const std::string& target, float scalevalue) {
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[name];

	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second *= scalevalue;
}

void DiffDataSets::OffsetDiff(const std::string& name, const std::string& target, Vector3& offset) {
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[name];
	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second += offset;
}

bool DiffDataSets::ApplyUVDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector2>* inOutResult) {
	if (percent == 0.0f)
		return false;

	if (!TargetMatch(set, target))
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].u += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].v += resultIt->second.y * percent;
	}

	return true;
}

bool DiffDataSets::ApplyDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector3>* inOutResult) {
	if (percent == 0.0f)
		return false;

	if (!TargetMatch(set, target))
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].x += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].y += resultIt->second.y * percent;
		(*inOutResult)[resultIt->first].z += resultIt->second.z * percent;
	}

	return true;
}

bool DiffDataSets::ApplyClamp(const std::string& set, const std::string& target, std::vector<Vector3>* inOutResult) {
	if (!TargetMatch(set, target))
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	std::unordered_map<uint16_t, Vector3>* data = &namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].x = resultIt->second.x;
		(*inOutResult)[resultIt->first].y = resultIt->second.y;
		(*inOutResult)[resultIt->first].z = resultIt->second.z;
	}

	return true;
}

std::unordered_map<uint16_t, Vector3>* DiffDataSets::GetDiffSet(const std::string& targetDataName) {
	if (namedSet.find(targetDataName) == namedSet.end())
		return nullptr;

	return &namedSet[targetDataName];
}

void DiffDataSets::GetDiffIndices(const std::string& set, const std::string& target, std::vector<uint16_t>& outIndices, float threshold) {
	if (!TargetMatch(set, target))
		return;

	std::unordered_map<uint16_t, Vector3>* data = &namedSet[set];
	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (fabs(resultIt->second.x) > threshold || fabs(resultIt->second.y) > threshold || fabs(resultIt->second.z) > threshold) {
			outIndices.push_back(resultIt->first);
		}
	}

	std::sort(outIndices.begin(), outIndices.end());
	outIndices.erase(std::unique(outIndices.begin(), outIndices.end()), outIndices.end());
}

void DiffDataSets::DeleteVerts(const std::string& target, const std::vector<uint16_t>& indices) {
	if (indices.empty())
		return;

	uint16_t highestRemoved = indices.back();
	std::vector<int> indexCollapse = GenerateIndexCollapseMap(indices, highestRemoved + 1);

	for (auto& data : namedSet) {
		if (TargetMatch(data.first, target))
			ApplyIndexMapToMapKeys(data.second, indexCollapse, -static_cast<int>(indices.size()));
	}
}

void DiffDataSets::InsertVertexIndices(const std::string& target, const std::vector<uint16_t>& indices) {
	if (indices.empty())
		return;

	int highestAdded = indices.back();
	std::vector<int> indexExpand = GenerateIndexExpandMap(indices, highestAdded + 1);

	for (auto& data : namedSet) {
		if (!TargetMatch(data.first, target))
			continue;

		ApplyIndexMapToMapKeys(data.second, indexExpand, static_cast<int>(indices.size()));
	}
}

void DiffDataSets::ClearSet(const std::string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);
}
