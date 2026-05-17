/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct MaskShapeData {
	std::string name;
	int vertexCount = 0;
	std::unordered_map<uint16_t, float> mask;
};

struct MaskEntry {
	std::string name;
	std::vector<MaskShapeData> shapes;

	void SetFromMaskData(const std::map<std::string, std::unordered_map<uint16_t, float>>& maskData,
		const std::map<std::string, int>& vertexCounts);

	std::map<std::string, std::unordered_map<uint16_t, float>> ToMaskData() const;

	// Find a mask matching by shape name, or fall back to vertex count.
	const MaskShapeData* FindMatchingMask(const std::string& shapeName, int vertexCount) const;
};

class MaskFile {
	int version = 1;
	std::vector<MaskEntry> entries;

public:
	int Load(const std::string& fileName);
	int Save(const std::string& fileName) const;

	int GetVersion() const { return version; }

	const std::vector<MaskEntry>& GetEntries() const { return entries; }
	std::vector<MaskEntry>& GetEntries() { return entries; }

	// Get the name of the first entry, or empty if no entries.
	std::string GetFirstName() const;

	// Find an entry by name. Returns nullptr if not found.
	const MaskEntry* FindEntry(const std::string& entryName) const;
};
