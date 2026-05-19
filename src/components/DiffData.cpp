/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "DiffData.h"
#include "../utils/PlatformUtil.h"
#include "../utils/ParallelFor.h"
#include "../utils/StringStuff.h"
#include "NifUtil.hpp"
#include "UndoState.h"

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

using namespace nifly;

OSDataFile::OSDataFile() {
	version = 1;
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

	uint32_t header = 0;
	file.read((char*)&header, 4);
	if (header != "OSD\0"_mci)
		return false;

	file.read((char*)&version, 4);

	uint32_t dataCount = 0;
	file.read((char*)&dataCount, 4);
	dataDiffs.reserve(dataCount);

	uint8_t nameLength = 0;
	std::string dataName;
	uint16_t diffSize = 0;
	for (uint32_t i = 0; i < dataCount; ++i) {
		file.read((char*)&nameLength, 1);
		dataName.resize(nameLength, ' ');
		file.read((char*)&dataName.front(), nameLength);

		TargetDataDiffs diffs;
		file.read((char*)&diffSize, 2);
		diffs.reserve(diffSize);

		std::vector<DiffStruct> diffData(diffSize);
		file.read((char*)diffData.data(), diffSize * sizeof(DiffStruct));

		for (int j = 0; j < diffSize; ++j) {
			auto& diffEntry = diffData[j];
			diffEntry.diff.clampEpsilon();
			diffs[diffEntry.index] = std::move(diffEntry.diff);
		}

		dataDiffs.emplace(dataName, std::make_unique<TargetDataDiffs>(std::move(diffs)));
	}

	return true;
}

bool OSDataFile::Write(const std::string& fileName) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	if (!file)
		return false;

	uint32_t header = "OSD\0"_mci;
	file.write((char*)&header, 4);
	file.write((char*)&version, 4);

	uint32_t dataCount = static_cast<uint32_t>(dataDiffs.size());
	file.write((char*)&dataCount, 4);

	uint8_t nameLength;
	uint16_t diffSize;
	for (auto& diffs : dataDiffs) {
		nameLength = static_cast<uint8_t>(diffs.first.length());
		file.write((char*)&nameLength, 1);
		file.write(diffs.first.c_str(), nameLength);

		diffSize = static_cast<uint16_t>(diffs.second->size());

		std::vector<DiffStruct> diffData(diffSize);
		diffData.resize(diffSize);

		size_t i = 0;

		for (auto& diff : *diffs.second) {
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

TargetData& OSDataFile::GetDataDiffs() {
	return dataDiffs;
}

std::unique_ptr<TargetDataDiffs>* OSDataFile::GetDataDiff(const std::string& dataName) {
	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		return &it->second;

	return nullptr;
}

void OSDataFile::SetDataDiff(const std::string& dataName, const TargetDataDiffs& inDataDiff) {
	dataDiffs[dataName] = std::make_unique<TargetDataDiffs>(inDataDiff);
}


void DiffDataSets::MoveToSet(const std::string& name, const std::string& target, std::unique_ptr<TargetDataDiffs>& inDiffData) {
	namedSet[name] = std::move(inDiffData);
	dataTargets[name] = target;
}

void DiffDataSets::LoadSet(const std::string& name, const std::string& target, const TargetDataDiffs& inDiffData) {
	namedSet[name] = std::make_unique<TargetDataDiffs>(inDiffData);
	dataTargets[name] = target;
}

int DiffDataSets::LoadSet(const std::string& name, const std::string& target, const std::string& fromFile) {
	std::fstream inFile;
	PlatformUtil::OpenFileStream(inFile, fromFile, std::ios::in | std::ios::binary);

	if (!inFile)
		return 1;

	uint32_t sz;
	inFile.read((char*)&sz, 4);

	auto data = std::make_unique<TargetDataDiffs>();
	data->reserve(sz);

	uint32_t idx;
	Vector3 v;
	for (uint32_t i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(uint32_t));
		inFile.read((char*)&v, sizeof(Vector3));
		v.clampEpsilon();
		data->emplace(static_cast<uint16_t>(idx), v);
	}

	inFile.close();

	namedSet[name] = std::move(data);
	dataTargets[name] = target;

	return 0;
}

bool DiffDataSets::LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
	std::vector<std::pair<std::string, const std::map<std::string, std::string>*>> osdEntries;
	osdEntries.reserve(osdNames.size());
	for (auto& osd : osdNames)
		osdEntries.emplace_back(osd.first, &osd.second);

	std::vector<std::unique_ptr<OSDataFile>> loaded(osdEntries.size());
	ParallelForDynamic(osdEntries.size(), 1, 1, [&](size_t startIndex, size_t endIndex) {
		for (size_t entryIndex = startIndex; entryIndex < endIndex; entryIndex++) {
			auto osdFile = std::make_unique<OSDataFile>();
			if (osdFile->Read(osdEntries[entryIndex].first))
				loaded[entryIndex] = std::move(osdFile);
		}
	});

	for (size_t entryIndex = 0; entryIndex < osdEntries.size(); entryIndex++) {
		auto& osdFile = loaded[entryIndex];
		if (!osdFile)
			continue;

		auto& osd = osdEntries[entryIndex];
		for (auto& dataNames : *osd.second) {
			auto diff = osdFile->GetDataDiff(dataNames.first);
			if (diff)
				MoveToSet(dataNames.first, dataNames.second, *diff);
		}
	}
	return true;
}

int DiffDataSets::SaveSet(const std::string& name, const std::string& target, const std::string& toFile) {
	auto& data = namedSet[name];
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

			osdFile.SetDataDiff(dataNames.first, *data);
		}

		if (!osdFile.Write(osd.first))
			return false;
	}

	return true;
}

void DiffDataSets::RenameSet(const std::string& oldName, const std::string& newName) {
	if (namedSet.find(oldName) != namedSet.end()) {
		namedSet.insert(std::make_pair(newName, std::move(namedSet[oldName])));
		namedSet.erase(oldName);
		dataTargets[newName] = dataTargets[oldName];
		dataTargets.erase(oldName);
	}
}

void DiffDataSets::RenameDataTarget(const std::string& oldTarget, const std::string& newTarget) {
	for (auto& dt : dataTargets) {
		if (dt.second == oldTarget)
			dt.second = newTarget;
	}
}

void DiffDataSets::CopySet(const std::string& oldName, const std::string& newName, const std::string& newTargetName) {
	auto namedSetIt = namedSet.find(oldName);
	if (namedSetIt != namedSet.end()) {
		namedSet[newName] = std::make_unique<TargetDataDiffs>(*namedSetIt->second);
		dataTargets[newName] = newTargetName;
	}
}

std::string DiffDataSets::GetDataTargetName(const std::string& targetName, const std::string& dataNameSuffix) {
	for (auto& dt : dataTargets) {
		if (dt.second == targetName && StringEndsWith(dt.first, dataNameSuffix))
			return dt.first;
	}
	return "";
}

void DiffDataSets::AddEmptySet(const std::string& name, const std::string& target) {
	if (namedSet.find(name) == namedSet.end()) {
		namedSet[name] = std::make_unique<TargetDataDiffs>();
		dataTargets[name] = target;
	}
}

void DiffDataSets::UpdateDiff(const std::string& name, const std::string& target, uint16_t index, const Vector3& newdiff) {
	auto& data = namedSet[name];
	if (!TargetMatch(name, target))
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const std::string& name, const std::string& target, uint16_t index, const Vector3& newdiff) {
	auto& data = namedSet[name];
	if (!TargetMatch(name, target))
		return;

	Vector3 v = (*data)[index];
	v += newdiff;

	if (v.IsZero(true))
		data->erase(index);
	else
		(*data)[index] = v;
}

void DiffDataSets::ScaleDiff(const std::string& name, const std::string& target, float scalevalue) {
	auto& data = namedSet[name];

	if (!TargetMatch(name, target))
		return;

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt)
		resultIt->second *= scalevalue;
}

void DiffDataSets::OffsetDiff(const std::string& name, const std::string& target, const Vector3& offset) {
	auto& data = namedSet[name];
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
	auto& data = namedSet[set];

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
	auto& data = namedSet[set];

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
	auto& data = namedSet[set];

	for (auto resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		if (resultIt->first >= maxidx)
			continue;

		(*inOutResult)[resultIt->first].x = resultIt->second.x;
		(*inOutResult)[resultIt->first].y = resultIt->second.y;
		(*inOutResult)[resultIt->first].z = resultIt->second.z;
	}

	return true;
}

TargetDataDiffs* DiffDataSets::GetDiffSet(const std::string& targetDataName) {
	if (namedSet.find(targetDataName) == namedSet.end())
		return nullptr;

	return namedSet[targetDataName].get();
}

void DiffDataSets::GetDiffIndices(const std::string& set, const std::string& target, std::vector<uint16_t>& outIndices, float threshold) {
	if (!TargetMatch(set, target))
		return;

	auto& data = namedSet[set];
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
			ApplyIndexMapToMapKeys(*data.second, indexCollapse, -static_cast<int>(indices.size()));
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

		ApplyIndexMapToMapKeys(*data.second, indexExpand, static_cast<int>(indices.size()));
	}
}

void DiffDataSets::ClearSet(const std::string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);
}
