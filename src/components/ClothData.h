/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "NifFile.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Keeps the BSClothExtraData blocks of every loaded NIF with the shapes that came from it
class ClothData {
public:
	using Sources = std::vector<std::string>;
	using Blocks = std::map<std::string, std::unique_ptr<nifly::BSClothExtraData>>;
	using ShapeSources = std::unordered_map<std::string, Sources>;

	// Tells whether a bone belongs to the reference skeleton rather than to a loaded outfit
	using SkeletonBones = std::function<bool(const std::string&)>;

private:
	Blocks blocks;
	ShapeSources shapeSources;
	Sources manualSources;

	std::string Add(const std::string& source, std::unique_ptr<nifly::BSClothExtraData> block);

public:
	const Blocks& GetBlocks() const { return blocks; }
	const Sources& GetManualSources() const { return manualSources; }

	Sources GetShapeSources(const std::string& shape) const;
	void SetShapeSources(const std::string& shape, const Sources& sources);
	void RemoveShape(const std::string& shape) { shapeSources.erase(shape); }
	void RenameShape(const std::string& before, const std::string& after);

	// The cloth of every shape, for project snapshots. Blocks are never removed, so restored sources stay valid.
	const ShapeSources& GetAllShapeSources() const { return shapeSources; }
	void SetAllShapeSources(const ShapeSources& sources) { shapeSources = sources; }

	// Takes the cloth blocks out of the NIF and assigns them to the given shapes.
	// Cloth drives the bones an outfit adds to the skeleton, so a block belongs to the shapes of the file skinned to an
	// added bone it names. A block that names no such bone is kept with every given shape rather than dropped.
	// The payload also names rig bones the cloth only collides with, so a shape skinned to such a bone outside the
	// reference skeleton, like a physics body under cloth built for that body's skeleton, claims the cloth as well.
	void Capture(nifly::NifFile& nif,
				 const std::string& source,
				 const std::vector<nifly::NiShape*>& shapes,
				 const SkeletonBones& isSkeletonBone = {});

	// Forgets all blocks, e.g. when the project is cleared for a new one
	void Clear() {
		blocks.clear();
		shapeSources.clear();
		manualSources.clear();
	}

	// Standalone HKX import, preselected for exports over the cloth of the loaded files
	void AddManual(const std::string& source, std::unique_ptr<nifly::BSClothExtraData> block);

	struct ExportChoices {
		// Each distinct payload once, by the first source that holds it
		Sources sources;
		// Index into sources, unset when no cloth fits the exported shapes
		std::optional<size_t> preselected;
	};

	// The blocks an export can write and the default for the shapes left in the NIF. A NIF holds a single cloth block,
	// so the default is the latest standalone HKX import, else the cloth of the exported shapes, else cloth driving an
	// added bone they are skinned to. The last one carries body physics over to an outfit that was given the body's
	// physics bones by a weight copy from the reference.
	ExportChoices GetExportChoices(nifly::NifFile& nif, const SkeletonBones& isSkeletonBone = {}) const;

	// Replaces any cloth in the NIF with the block of one source, or with none for an empty name
	void Write(nifly::NifFile& nif, const std::string& source) const;
};
