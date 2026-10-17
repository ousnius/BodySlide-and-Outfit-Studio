/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "NifFile.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Keeps the BSClothExtraData blocks of every loaded NIF with the shapes that came from it
class ClothData {
public:
	using Sources = std::vector<std::string>;
	using Blocks = std::map<std::string, std::unique_ptr<nifly::BSClothExtraData>>;

	// Tells whether a bone belongs to the reference skeleton rather than to a loaded outfit
	using SkeletonBones = std::function<bool(const std::string&)>;

private:
	Blocks blocks;
	std::unordered_map<std::string, Sources> shapeSources;
	Sources manualSources;

	std::string Add(const std::string& source, std::unique_ptr<nifly::BSClothExtraData> block);

public:
	const Blocks& GetBlocks() const { return blocks; }
	const Sources& GetManualSources() const { return manualSources; }

	Sources GetShapeSources(const std::string& shape) const;
	void SetShapeSources(const std::string& shape, const Sources& sources);
	void RemoveShape(const std::string& shape) { shapeSources.erase(shape); }
	void RenameShape(const std::string& before, const std::string& after);

	// Takes the cloth blocks out of the NIF and assigns them to the given shapes.
	// Cloth drives the bones an outfit adds to the skeleton, so a block belongs to the shapes of the file skinned to an
	// added bone it names. A block that names no such bone is kept with every given shape rather than dropped.
	void Capture(nifly::NifFile& nif,
				 const std::string& source,
				 const std::vector<nifly::NiShape*>& shapes,
				 const SkeletonBones& isSkeletonBone = {});

	// Standalone HKX import that is only written to the output on request
	void AddManual(const std::string& source, std::unique_ptr<nifly::BSClothExtraData> block);

	// Writes the cloth of the shapes left in the NIF plus any additional sources
	void Apply(nifly::NifFile& nif, const Sources& additional = {}) const;
};
