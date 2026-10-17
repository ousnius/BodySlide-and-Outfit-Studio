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
			names.insert(current);
		current.clear();
	};

	const char* data = block.data.data();
	for (size_t i = 0; i < block.data.size(); ++i) {
		char c = data[i];
		if (c >= 0x20 && c <= 0x7e)
			current += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		else
			flush();
	}
	flush();
	return names;
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

void ClothData::Capture(NifFile& nif, const std::string& source, const std::vector<NiShape*>& shapes, const SkeletonBones& isSkeletonBone) {
	struct Block {
		std::string key;
		std::unordered_set<std::string> names;
	};

	std::vector<Block> captured;
	for (auto& block : nif.GetChildren<BSClothExtraData>(nullptr, true))
		captured.push_back({Add(source, block->Clone()), Names(*block)});

	// The payload names the whole skeleton next to the bones it drives, so only the bones a shape adds to the
	// reference skeleton tell its cloth apart. Every shape of the file is resolved, so cloth owned by a shape outside
	// the capture is not handed to another.
	std::unordered_map<NiShape*, Sources> owned;
	std::set<std::string> claimed;
	for (auto& shape : nif.GetShapes()) {
		std::vector<std::string> bones;
		nif.GetShapeBoneList(shape, bones);

		std::vector<std::string> added;
		for (auto& bone : bones) {
			if (!isSkeletonBone || !isSkeletonBone(bone))
				added.push_back(Lower(bone));
		}

		for (auto& block : captured) {
			bool drives = std::any_of(added.begin(), added.end(), [&](const std::string& bone) { return block.names.count(bone) > 0; });
			if (drives) {
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

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

	// Several shapes can share a source and different imports can hold the same block, so each payload is written once
	std::set<std::string> written;
	for (auto& source : sources) {
		auto block = blocks.find(source);
		if (block != blocks.end() && written.insert(Fingerprint(*block->second)).second)
			root->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(block->second->Clone()));
	}
}
