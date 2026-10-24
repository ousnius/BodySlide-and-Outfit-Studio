/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../program/ProjectOperations.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class OutfitStudioMcp;

// A compact picture of a project, kept with each snapshot to tell what changed since
struct McpProjectDigest {
	struct Slider {
		uint64_t hash = 0;
		size_t moved = 0;
		float maxDelta = 0.0f;
	};

	struct Shape {
		size_t vertices = 0;
		size_t triangles = 0;
		std::vector<nifly::Vector3> positions;
		uint64_t triangleHash = 0;
		uint64_t uvHash = 0;
		size_t masked = 0;
		// Hash of the weights of each bone
		std::map<std::string, uint64_t> bones;
		// Morphs of the sliders with data on the shape
		std::map<std::string, Slider> sliders;
	};

	std::map<std::string, Shape> shapes;
	std::vector<std::string> sliders;
	std::string reference;
};

McpProjectDigest McpDigest(OutfitStudioMcp& mcp);

// What changed from one digest to the other, at most limit sliders per shape
nlohmann::json McpDigestDiff(const McpProjectDigest& before, const McpProjectDigest& after, size_t limit);

// Named snapshots of the project for the session of the window, and the automatic ones taken before tools
// that can't be undone
class McpSnapshotStore {
public:
	struct Entry {
		std::string name;
		std::unique_ptr<EditSnapshot> snapshot;
		McpProjectDigest digest;
		bool automatic = false;
		std::string tool; // The tool an automatic snapshot was taken before
	};

	static constexpr size_t MaxNamed = 10;
	static constexpr size_t MaxAutomatic = 3;

	// Snapshots belong to one project, they are dropped when another one is opened
	void Track(const OutfitProject* project);

	Entry* Find(const std::string& name);

	// Replaces a snapshot of the same name. Throws when there are too many.
	void Save(OutfitStudioMcp& mcp, const std::string& name);

	// Keeps the newest few automatic snapshots, and the one named keep. Returns the name of the new one.
	std::string SaveAutomatic(OutfitStudioMcp& mcp, const std::string& tool, const std::string& keep = "");

	bool Remove(const std::string& name);

	// Names of all snapshots, oldest first
	nlohmann::json Names() const;
	// The automatic snapshots, with the tool each was taken before
	nlohmann::json Automatic() const;

private:
	std::vector<Entry> entries;
	uint64_t projectId = 0;
	size_t automaticCount = 0;
};
