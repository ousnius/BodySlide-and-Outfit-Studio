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

static bool ReadOSDData(const std::string& fileName, const std::map<std::string, std::string>* dataNames, TargetDataDiffLists& outDataDiffs, uint32_t* outVersion = nullptr);

bool OSDataFile::Read(const std::string& fileName, const std::map<std::string, std::string>* dataNames) {
	dataDiffs.clear();
	dataDiffLists.clear();
	return ReadOSDData(fileName, dataNames, dataDiffLists, &version);
}

bool OSDataFile::Write(const std::string& fileName) {
	MaterializeDataDiffs();

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
	MaterializeDataDiffs();
	return dataDiffs;
}

std::unique_ptr<TargetDataDiffs>* OSDataFile::GetDataDiff(const std::string& dataName) {
	MaterializeDataDiff(dataName);

	auto it = dataDiffs.find(dataName);
	if (it != dataDiffs.end())
		return &it->second;

	return nullptr;
}

void OSDataFile::SetDataDiff(const std::string& dataName, const TargetDataDiffs& inDataDiff) {
	dataDiffLists.erase(dataName);
	dataDiffs[dataName] = std::make_unique<TargetDataDiffs>(inDataDiff);
}


TargetDataDiffs* OSDataFile::MaterializeDataDiff(const std::string& dataName) {
	auto dataDiff = dataDiffs.find(dataName);
	if (dataDiff != dataDiffs.end())
		return dataDiff->second.get();

	auto dataDiffList = dataDiffLists.find(dataName);
	if (dataDiffList == dataDiffLists.end())
		return nullptr;

	auto diffs = std::make_unique<TargetDataDiffs>();
	diffs->reserve(dataDiffList->second->size());
	for (auto& diff : *dataDiffList->second)
		diffs->insert_or_assign(diff.index, diff.diff);

	auto result = diffs.get();
	dataDiffs[dataName] = std::move(diffs);
	dataDiffLists.erase(dataDiffList);
	return result;
}


void OSDataFile::MaterializeDataDiffs() {
	while (!dataDiffLists.empty()) {
		std::string dataName = dataDiffLists.begin()->first;
		MaterializeDataDiff(dataName);
	}
}

static bool ReadOSDData(const std::string& fileName, const std::map<std::string, std::string>* dataNames, TargetDataDiffLists& outDataDiffs, uint32_t* outVersion) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	if (!file)
		return false;

	uint32_t header = 0;
	file.read((char*)&header, 4);
	if (header != "OSD\0"_mci)
		return false;

	uint32_t version = 1;
	file.read((char*)&version, 4);
	if (outVersion)
		*outVersion = version;

	uint32_t dataCount = 0;
	file.read((char*)&dataCount, 4);
	outDataDiffs.reserve(dataNames ? std::min<size_t>(dataCount, dataNames->size()) : dataCount);

	uint8_t nameLength = 0;
	std::string dataName;
	uint16_t diffSize = 0;
	for (uint32_t i = 0; i < dataCount; ++i) {
		file.read((char*)&nameLength, 1);
		dataName.resize(nameLength, ' ');
		file.read((char*)&dataName.front(), nameLength);

		file.read((char*)&diffSize, 2);
		if (dataNames && dataNames->find(dataName) == dataNames->end()) {
			file.seekg(static_cast<std::streamoff>(diffSize) * sizeof(DiffStruct), std::ios::cur);
			continue;
		}

		std::vector<DiffStruct> diffData(diffSize);
		file.read((char*)diffData.data(), diffSize * sizeof(DiffStruct));

		auto diffs = std::make_unique<TargetDataDiffList>();
		diffs->reserve(diffSize);
		for (auto& diffEntry : diffData) {
			diffEntry.diff.clampEpsilon();
			diffs->push_back(TargetDataDiff{diffEntry.index, std::move(diffEntry.diff)});
		}

		outDataDiffs.emplace(dataName, std::move(diffs));
	}

	return true;
}


TargetDataDiffs* DiffDataSets::MaterializeSet(DataSet& dataSet) {
	if (dataSet.diffs)
		return dataSet.diffs.get();

	if (!dataSet.linearDiffs)
		return nullptr;

	auto diffs = std::make_unique<TargetDataDiffs>();
	diffs->reserve(dataSet.linearDiffs->size());
	for (auto& diff : *dataSet.linearDiffs)
		diffs->insert_or_assign(diff.index, diff.diff);

	dataSet.linearDiffs.reset();
	dataSet.diffs = std::move(diffs);
	return dataSet.diffs.get();
}


TargetDataDiffs* DiffDataSets::MaterializeSet(const std::string& name) {
	auto dataSet = namedSet.find(name);
	if (dataSet == namedSet.end())
		return nullptr;

	return MaterializeSet(dataSet->second);
}


void DiffDataSets::MoveToSet(const std::string& name, const std::string& target, std::unique_ptr<TargetDataDiffs>& inDiffData) {
	auto& dataSet = namedSet[name];
	dataSet.target = target;
	dataSet.diffs = std::move(inDiffData);
	dataSet.linearDiffs.reset();
}

void DiffDataSets::LoadSet(const std::string& name, const std::string& target, const TargetDataDiffs& inDiffData) {
	auto& dataSet = namedSet[name];
	dataSet.target = target;
	dataSet.diffs = std::make_unique<TargetDataDiffs>(inDiffData);
	dataSet.linearDiffs.reset();
}

int DiffDataSets::LoadSet(const std::string& name, const std::string& target, const std::string& fromFile) {
	std::fstream inFile;
	PlatformUtil::OpenFileStream(inFile, fromFile, std::ios::in | std::ios::binary);

	if (!inFile)
		return 1;

	uint32_t sz;
	inFile.read((char*)&sz, 4);

	auto data = std::make_unique<TargetDataDiffList>();
	data->reserve(sz);

	uint32_t idx;
	Vector3 v;
	for (uint32_t i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(uint32_t));
		inFile.read((char*)&v, sizeof(Vector3));
		v.clampEpsilon();
		data->push_back(TargetDataDiff{static_cast<uint16_t>(idx), v});
	}

	inFile.close();

	auto& dataSet = namedSet[name];
	dataSet.target = target;
	dataSet.diffs.reset();
	dataSet.linearDiffs = std::move(data);

	return 0;
}

bool DiffDataSets::LoadData(const std::map<std::string, std::map<std::string, std::string>>& osdNames) {
	std::vector<std::pair<std::string, const std::map<std::string, std::string>*>> osdEntries;
	osdEntries.reserve(osdNames.size());
	size_t dataNameCount = 0;
	for (auto& osd : osdNames) {
		osdEntries.emplace_back(osd.first, &osd.second);
		dataNameCount += osd.second.size();
	}
	namedSet.reserve(namedSet.size() + dataNameCount);

	std::vector<std::unique_ptr<TargetDataDiffLists>> loaded(osdEntries.size());
	ParallelForDynamic(osdEntries.size(), 1, 1, [&](size_t startIndex, size_t endIndex) {
		for (size_t entryIndex = startIndex; entryIndex < endIndex; entryIndex++) {
			auto osdData = std::make_unique<TargetDataDiffLists>();
			if (ReadOSDData(osdEntries[entryIndex].first, osdEntries[entryIndex].second, *osdData))
				loaded[entryIndex] = std::move(osdData);
		}
	});

	for (size_t entryIndex = 0; entryIndex < osdEntries.size(); entryIndex++) {
		auto& osdData = loaded[entryIndex];
		if (!osdData)
			continue;

		auto& osd = osdEntries[entryIndex];
		for (auto& dataNames : *osd.second) {
			auto diff = osdData->find(dataNames.first);
			if (diff != osdData->end()) {
				auto& dataSet = namedSet[dataNames.first];
				dataSet.target = dataNames.second;
				dataSet.diffs.reset();
				dataSet.linearDiffs = std::move(diff->second);
			}
		}
	}
	return true;
}

int DiffDataSets::SaveSet(const std::string& name, const std::string& target, const std::string& toFile) {
	if (!TargetMatch(name, target))
		return 2;

	auto data = MaterializeSet(name);
	if (!data)
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
			if (!TargetMatch(dataNames.first, dataNames.second))
				continue;

			auto data = MaterializeSet(dataNames.first);
			if (!data)
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
	}
}

void DiffDataSets::RenameDataTarget(const std::string& oldTarget, const std::string& newTarget) {
	for (auto& dataSet : namedSet) {
		if (dataSet.second.target == oldTarget)
			dataSet.second.target = newTarget;
	}
}

void DiffDataSets::CopySet(const std::string& oldName, const std::string& newName, const std::string& newTargetName) {
	auto namedSetIt = namedSet.find(oldName);
	if (namedSetIt != namedSet.end()) {
		auto& newDataSet = namedSet[newName];
		newDataSet.target = newTargetName;
		if (namedSetIt->second.diffs) {
			newDataSet.diffs = std::make_unique<TargetDataDiffs>(*namedSetIt->second.diffs);
			newDataSet.linearDiffs.reset();
		}
		else if (namedSetIt->second.linearDiffs) {
			newDataSet.diffs.reset();
			newDataSet.linearDiffs = std::make_unique<TargetDataDiffList>(*namedSetIt->second.linearDiffs);
		}
		else {
			newDataSet.diffs.reset();
			newDataSet.linearDiffs.reset();
		}
	}
}

std::string DiffDataSets::GetDataTargetName(const std::string& targetName, const std::string& dataNameSuffix) {
	for (auto& dataSet : namedSet) {
		if (dataSet.second.target == targetName && StringEndsWith(dataSet.first, dataNameSuffix))
			return dataSet.first;
	}
	return "";
}

void DiffDataSets::AddEmptySet(const std::string& name, const std::string& target) {
	if (namedSet.find(name) == namedSet.end()) {
		auto& dataSet = namedSet[name];
		dataSet.target = target;
		dataSet.diffs = std::make_unique<TargetDataDiffs>();
		dataSet.linearDiffs.reset();
	}
}

void DiffDataSets::UpdateDiff(const std::string& name, const std::string& target, uint16_t index, const Vector3& newdiff) {
	if (!TargetMatch(name, target))
		return;

	auto data = MaterializeSet(name);
	if (!data)
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const std::string& name, const std::string& target, uint16_t index, const Vector3& newdiff) {
	if (!TargetMatch(name, target))
		return;

	auto data = MaterializeSet(name);
	if (!data)
		return;

	auto diff = data->find(index);
	Vector3 v;
	if (diff != data->end())
		v = diff->second;
	v += newdiff;

	if (v.IsZero(true))
		data->erase(index);
	else
		data->insert_or_assign(index, v);
}

void DiffDataSets::ScaleDiff(const std::string& name, const std::string& target, float scalevalue) {
	auto dataSet = namedSet.find(name);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return;

	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs)
			diff.diff *= scalevalue;
	}
	else if (dataSet->second.diffs) {
		for (auto& diff : *dataSet->second.diffs)
			diff.second *= scalevalue;
	}
}

void DiffDataSets::OffsetDiff(const std::string& name, const std::string& target, const Vector3& offset) {
	auto dataSet = namedSet.find(name);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return;

	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs)
			diff.diff += offset;
	}
	else if (dataSet->second.diffs) {
		for (auto& diff : *dataSet->second.diffs)
			diff.second += offset;
	}
}

bool DiffDataSets::ApplyUVDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector2>* inOutResult) {
	if (percent == 0.0f)
		return false;

	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs) {
			if (diff.index >= maxidx)
				continue;

			(*inOutResult)[diff.index].u += diff.diff.x * percent;
			(*inOutResult)[diff.index].v += diff.diff.y * percent;
		}
	}
	else if (dataSet->second.diffs) {
		for (auto resultIt = dataSet->second.diffs->begin(); resultIt != dataSet->second.diffs->end(); ++resultIt) {
			if (resultIt->first >= maxidx)
				continue;

			(*inOutResult)[resultIt->first].u += resultIt->second.x * percent;
			(*inOutResult)[resultIt->first].v += resultIt->second.y * percent;
		}
	}

	return true;
}

bool DiffDataSets::ApplyDiff(const std::string& set, const std::string& target, float percent, std::vector<Vector3>* inOutResult) {
	if (percent == 0.0f)
		return false;

	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs) {
			if (diff.index >= maxidx)
				continue;

			(*inOutResult)[diff.index].x += diff.diff.x * percent;
			(*inOutResult)[diff.index].y += diff.diff.y * percent;
			(*inOutResult)[diff.index].z += diff.diff.z * percent;
		}
	}
	else if (dataSet->second.diffs) {
		for (auto resultIt = dataSet->second.diffs->begin(); resultIt != dataSet->second.diffs->end(); ++resultIt) {
			if (resultIt->first >= maxidx)
				continue;

			(*inOutResult)[resultIt->first].x += resultIt->second.x * percent;
			(*inOutResult)[resultIt->first].y += resultIt->second.y * percent;
			(*inOutResult)[resultIt->first].z += resultIt->second.z * percent;
		}
	}

	return true;
}

bool DiffDataSets::ApplyClamp(const std::string& set, const std::string& target, std::vector<Vector3>* inOutResult) {
	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return false;

	uint16_t maxidx = static_cast<uint16_t>(inOutResult->size());
	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs) {
			if (diff.index >= maxidx)
				continue;

			(*inOutResult)[diff.index].x = diff.diff.x;
			(*inOutResult)[diff.index].y = diff.diff.y;
			(*inOutResult)[diff.index].z = diff.diff.z;
		}
	}
	else if (dataSet->second.diffs) {
		for (auto resultIt = dataSet->second.diffs->begin(); resultIt != dataSet->second.diffs->end(); ++resultIt) {
			if (resultIt->first >= maxidx)
				continue;

			(*inOutResult)[resultIt->first].x = resultIt->second.x;
			(*inOutResult)[resultIt->first].y = resultIt->second.y;
			(*inOutResult)[resultIt->first].z = resultIt->second.z;
		}
	}

	return true;
}

TargetDataDiffs* DiffDataSets::GetDiffSet(const std::string& targetDataName) {
	return MaterializeSet(targetDataName);
}

void DiffDataSets::GetDiffIndices(const std::string& set, const std::string& target, std::vector<uint16_t>& outIndices, float threshold) {
	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return;

	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs) {
			if (fabs(diff.diff.x) > threshold || fabs(diff.diff.y) > threshold || fabs(diff.diff.z) > threshold)
				outIndices.push_back(diff.index);
		}
	}
	else if (dataSet->second.diffs) {
		for (auto resultIt = dataSet->second.diffs->begin(); resultIt != dataSet->second.diffs->end(); ++resultIt) {
			if (fabs(resultIt->second.x) > threshold || fabs(resultIt->second.y) > threshold || fabs(resultIt->second.z) > threshold)
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
		if (data.second.target == target) {
			auto diffSet = MaterializeSet(data.second);
			if (diffSet)
				ApplyIndexMapToMapKeys(*diffSet, indexCollapse, -static_cast<int>(indices.size()));
		}
	}
}

void DiffDataSets::InsertVertexIndices(const std::string& target, const std::vector<uint16_t>& indices) {
	if (indices.empty())
		return;

	int highestAdded = indices.back();
	std::vector<int> indexExpand = GenerateIndexExpandMap(indices, highestAdded + 1);

	for (auto& data : namedSet) {
		if (data.second.target != target)
			continue;

		auto diffSet = MaterializeSet(data.second);
		if (diffSet)
			ApplyIndexMapToMapKeys(*diffSet, indexExpand, static_cast<int>(indices.size()));
	}
}

void DiffDataSets::ClearSet(const std::string& name) {
	namedSet.erase(name);
}

void DiffDataSets::EmptySet(const std::string& set, const std::string& target) {
	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end() || dataSet->second.target != target)
		return;

	if (dataSet->second.linearDiffs)
		dataSet->second.linearDiffs->clear();
	if (dataSet->second.diffs)
		dataSet->second.diffs->clear();
}

void DiffDataSets::ZeroVertDiff(const std::string& set, int vertCount, float* vColorMask) {
	auto dataSet = namedSet.find(set);
	if (dataSet == namedSet.end())
		return;

	if (dataSet->second.linearDiffs) {
		for (auto& diff : *dataSet->second.linearDiffs) {
			if (diff.index < vertCount) {
				float f = vColorMask[diff.index];
				if (f == 1.0f)
					continue;
				else if (f == 0.0f)
					diff.diff *= 0.0f;
				else
					diff.diff *= f;
			}
			else
				diff.diff *= 0.0f;
		}
	}
	else if (dataSet->second.diffs) {
		for (auto& diff : *dataSet->second.diffs) {
			if (diff.first < vertCount) {
				float f = vColorMask[diff.first];
				if (f == 1.0f)
					continue;
				else if (f == 0.0f)
					diff.second *= 0.0f;
				else
					diff.second *= f;
			}
			else
				diff.second *= 0.0f;
		}
	}
}

void DiffDataSets::ZeroVertDiff(const std::string& set, const std::string& target, std::vector<uint16_t>* vertSet, std::unordered_map<uint16_t, float>* mask) {
	if (!TargetMatch(set, target))
		return;

	auto data = MaterializeSet(set);
	if (!data)
		return;

	std::vector<uint16_t> v;
	if (vertSet) {
		v = (*vertSet);
	}
	else {
		v.reserve(data->size());
		for (auto& diff : *data)
			v.push_back(diff.first);
	}

	for (auto& i : v) {
		auto d = data->find(i);
		if (d == data->end())
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
			data->erase(i);
			continue;
		}
		d->second *= f;
	}
}
