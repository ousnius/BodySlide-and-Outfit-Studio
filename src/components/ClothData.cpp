/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ClothData.h"
#include "../../lib/LZ4F/xxhash.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <unordered_set>

using namespace nifly;

namespace {
std::string Fingerprint(const BSClothExtraData& block) {
	return std::to_string(block.data.size()) + ":" + std::to_string(XXH64(block.data.data(), block.data.size(), 0));
}

std::string Lower(const std::string& text) {
	std::string lower(text);
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return lower;
}

// The Havok payload keeps its bone names as plain text, in the string table of a tagfile and inline in a packfile.
// Node and HKX spellings can differ in case, so names are compared lowercase.
std::unordered_set<std::string> Names(const BSClothExtraData& block) {
	std::unordered_set<std::string> names;
	std::string current;
	auto flush = [&]() {
		if (current.size() > 1)
			names.insert(Lower(current));
		current.clear();
	};

	const char* data = block.data.data();
	for (size_t i = 0; i < block.data.size(); ++i) {
		char c = data[i];
		if (c >= 0x20 && c <= 0x7e)
			current += c;
		else
			flush();
	}
	flush();
	return names;
}

// The bones a shape adds to the reference skeleton, lowercase. Only these tell cloth apart, as every payload names the
// whole skeleton next to the bones it drives.
std::vector<std::string> AddedBones(NifFile& nif, NiShape* shape, const ClothData::SkeletonBones& isSkeletonBone) {
	std::vector<std::string> bones;
	nif.GetShapeBoneList(shape, bones);

	std::vector<std::string> added;
	for (auto& bone : bones) {
		if (!isSkeletonBone || !isSkeletonBone(bone))
			added.push_back(Lower(bone));
	}
	return added;
}

bool Drives(const std::unordered_set<std::string>& names, const std::vector<std::string>& addedBones) {
	return std::any_of(addedBones.begin(), addedBones.end(), [&](const std::string& bone) { return names.count(bone) > 0; });
}

} // namespace

std::string ClothData::Add(const std::string& source, std::unique_ptr<BSClothExtraData> block) {
	// Reloading a file reuses the blocks it stored before. Blocks are never removed, as undo can still refer to them.
	std::string key = source;
	for (size_t suffix = 2;; ++suffix) {
		auto existing = blocks.find(key);
		if (existing == blocks.end())
			break;

		const auto& data = existing->second->data;
		if (data.size() == block->data.size() && std::equal(data.data(), data.data() + data.size(), block->data.data()))
			return key;

		key = source + " (" + std::to_string(suffix) + ")";
	}

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

void ClothData::Capture(NifFile& nif, const std::string& source, const std::vector<NiShape*>& shapes, const SkeletonBones& isSkeletonBone) {
	struct Block {
		std::string key;
		std::unordered_set<std::string> names;
	};

	std::vector<Block> captured;
	for (auto& block : nif.GetChildren<BSClothExtraData>(nullptr, true)) {
		auto key = Add(source, block->Clone());
		if (std::none_of(captured.begin(), captured.end(), [&](const Block& other) { return other.key == key; }))
			captured.push_back({key, Names(*block)});
	}

	// The payload names the whole skeleton next to the bones it drives, so only the bones a shape adds to the
	// reference skeleton tell its cloth apart. Every shape of the file is resolved, so cloth owned by a shape outside
	// the capture is not handed to another.
	std::unordered_map<NiShape*, Sources> owned;
	std::set<std::string> claimed;
	for (auto& shape : nif.GetShapes()) {
		auto added = AddedBones(nif, shape, isSkeletonBone);
		for (auto& block : captured) {
			if (Drives(block.names, added)) {
				owned[shape].push_back(block.key);
				claimed.insert(block.key);
			}
		}
	}

	// Cloth that names no added bone of any shape, such as cloth that only drives the base skeleton, stays with every
	// captured shape
	for (auto& shape : shapes) {
		if (!shape)
			continue;

		Sources sources = owned[shape];
		for (auto& block : captured) {
			if (claimed.find(block.key) == claimed.end())
				sources.push_back(block.key);
		}

		SetShapeSources(shape->name.get(), sources);
	}

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");
}

void ClothData::AddManual(const std::string& source, std::unique_ptr<BSClothExtraData> block) {
	auto key = Add(source, std::move(block));
	if (std::find(manualSources.begin(), manualSources.end(), key) == manualSources.end())
		manualSources.push_back(key);
}

ClothData::ExportChoices ClothData::GetExportChoices(NifFile& nif, const SkeletonBones& isSkeletonBone) const {
	std::optional<std::string> best;
	if (!manualSources.empty())
		best = manualSources.back();

	std::vector<std::string> added;
	for (auto& shape : nif.GetShapes()) {
		auto owned = GetShapeSources(shape->name.get());
		if (!best && !owned.empty())
			best = owned.front();

		auto bones = AddedBones(nif, shape, isSkeletonBone);
		added.insert(added.end(), bones.begin(), bones.end());
	}

	if (!best) {
		for (auto& block : blocks) {
			if (Drives(Names(*block.second), added)) {
				best = block.first;
				break;
			}
		}
	}

	// Different imports can hold the same block, which would only list the same choice twice
	ExportChoices choices;
	std::unordered_map<std::string, size_t> listed;
	for (auto& block : blocks) {
		auto entry = listed.emplace(Fingerprint(*block.second), choices.sources.size());
		if (entry.second)
			choices.sources.push_back(block.first);
		if (best && *best == block.first)
			choices.preselected = entry.first->second;
	}
	return choices;
}

void ClothData::Write(NifFile& nif, const std::string& source) const {
	auto root = nif.GetRootNode();
	if (!root)
		return;

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

	auto block = blocks.find(source);
	if (block != blocks.end())
		root->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(block->second->Clone()));
}
