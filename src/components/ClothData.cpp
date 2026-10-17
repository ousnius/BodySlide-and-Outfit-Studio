/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ClothData.h"
#include "../../lib/LZ4F/xxhash.h"

#include <set>

using namespace nifly;

namespace {
// Shape extra data listing the fingerprints of the cloth blocks a shape owns, so ownership survives saving and reopening
constexpr const char* ownershipName = "BSOS_ClothOwnership_v1";

std::string Fingerprint(const BSClothExtraData& block) {
	return std::to_string(block.data.size()) + ":" + std::to_string(XXH64(block.data.data(), block.data.size(), 0));
}

const NiStringsExtraData* Ownership(NifFile& nif, const NiRef& ref) {
	auto data = nif.GetHeader().GetBlock<NiStringsExtraData>(ref);
	return data && data->name.get() == ownershipName ? data : nullptr;
}

void RemoveOwnership(NifFile& nif) {
	std::set<uint32_t> ids;
	for (auto& shape : nif.GetShapes()) {
		for (auto& ref : shape->extraDataRefs) {
			if (Ownership(nif, ref))
				ids.insert(ref.index);
		}
	}

	// Deleting a block renumbers the ones after it
	for (auto it = ids.rbegin(); it != ids.rend(); ++it)
		nif.GetHeader().DeleteBlock(*it);
}
} // namespace

std::string ClothData::Add(const std::string& source, std::unique_ptr<BSClothExtraData> block) {
	std::string key = source;
	for (size_t suffix = 2; blocks.find(key) != blocks.end(); ++suffix)
		key = source + " (" + std::to_string(suffix) + ")";

	blocks.emplace(key, std::move(block));
	return key;
}

ClothData::Sources ClothData::GetShapeSources(const std::string& shape) const {
	auto it = shapeSources.find(shape);
	return it != shapeSources.end() ? it->second : Sources{};
}

void ClothData::SetShapeSources(const std::string& shape, const Sources& sources) {
	if (sources.empty())
		shapeSources.erase(shape);
	else
		shapeSources[shape] = sources;
}

void ClothData::RenameShape(const std::string& before, const std::string& after) {
	if (before == after)
		return;

	auto sources = GetShapeSources(before);
	RemoveShape(before);
	SetShapeSources(after, sources);
}

void ClothData::Capture(NifFile& nif, const std::string& source, const std::vector<NiShape*>& shapes) {
	Sources sources;
	std::unordered_map<std::string, Sources> payloadSources;
	for (auto& block : nif.GetChildren<BSClothExtraData>(nullptr, true)) {
		auto key = Add(source, block->Clone());
		sources.push_back(key);
		payloadSources[Fingerprint(*block)].push_back(key);
	}

	for (auto& shape : shapes) {
		if (!shape)
			continue;

		// Without ownership records, all cloth of the file belongs to every shape
		bool hasOwnership = false;
		Sources owned;
		for (auto& ref : shape->extraDataRefs) {
			auto data = Ownership(nif, ref);
			if (!data)
				continue;

			hasOwnership = true;
			for (auto fingerprint = data->stringsData.cbegin(); fingerprint != data->stringsData.cend(); ++fingerprint) {
				auto it = payloadSources.find(fingerprint->get());
				if (it != payloadSources.end())
					owned.insert(owned.end(), it->second.begin(), it->second.end());
			}
		}

		SetShapeSources(shape->name.get(), hasOwnership ? owned : sources);
	}

	RemoveOwnership(nif);
	nif.GetHeader().DeleteBlockByType("BSClothExtraData");
}

void ClothData::AddManual(const std::string& source, std::unique_ptr<BSClothExtraData> block) {
	manualSources.push_back(Add(source, std::move(block)));
}

void ClothData::Apply(NifFile& nif, const Sources& additional) const {
	auto root = nif.GetRootNode();
	if (!root)
		return;

	Sources sources;
	for (auto& shape : nif.GetShapes()) {
		auto owned = GetShapeSources(shape->name.get());
		sources.insert(sources.end(), owned.begin(), owned.end());
	}
	sources.insert(sources.end(), additional.begin(), additional.end());

	RemoveOwnership(nif);
	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

	// Several shapes can share a source and different imports can hold the same block, so each payload is written once
	std::unordered_map<std::string, std::string> fingerprints;
	std::set<std::string> written;
	for (auto& source : sources) {
		auto block = blocks.find(source);
		if (block == blocks.end() || fingerprints.find(source) != fingerprints.end())
			continue;

		auto fingerprint = Fingerprint(*block->second);
		fingerprints[source] = fingerprint;
		if (written.insert(fingerprint).second)
			root->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(block->second->Clone()));
	}

	if (written.empty())
		return;

	for (auto& shape : nif.GetShapes()) {
		auto owned = GetShapeSources(shape->name.get());
		owned.insert(owned.end(), additional.begin(), additional.end());

		std::set<std::string> shapeFingerprints;
		for (auto& source : owned) {
			auto it = fingerprints.find(source);
			if (it != fingerprints.end())
				shapeFingerprints.insert(it->second);
		}

		auto ownership = std::make_unique<NiStringsExtraData>();
		ownership->name.get() = ownershipName;
		for (auto& fingerprint : shapeFingerprints) {
			NiString value(fingerprint);
			ownership->stringsData.push_back(value);
		}

		shape->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(ownership)));
	}
}
