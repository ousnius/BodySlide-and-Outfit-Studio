/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "ProjectOperations.h"
#include "../files/MaterialFile.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

// Bringing shapes of another game's NIF or BodySlide set into the project, for the import_from_game tool and the
// ImportFromGame automation step
namespace GameImport {

struct Options {
	// NIF file, or slider set file (.osp) of another BodySlide folder, as an absolute path
	std::string file;
	// Set of the slider set file, needed when it has more than one
	std::string set;
	// Only these shapes, all when empty
	std::vector<std::string> shapes;
	std::vector<std::string> skipShapes;
	// Slider data to take over from a set: "local", "all", "zaps" or "none"
	std::string sliders = "local";
	// Absolute data folders of the source game or mod to find material files in
	std::vector<std::string> dataFolders;
};

struct Report {
	std::string sourceGame;
	std::string sourceFile;
	// Per shape: name, source, vertices, triangles, sourceSpace, translationToGlobal, vertexColors, sourceBones,
	// warning, and either material (file, target with applied and notes, or error) or textures
	nlohmann::json shapes = nlohmann::json::array();
	std::vector<std::string> slidersWithData;
	std::vector<std::string> slidersAdded;
};

std::string GameName(const nifly::NiVersion& version);

// A material path of a shader, found in one of the data folders, with or without the leading "materials"
std::string FindMaterialFile(const std::string& materialPath, const std::vector<std::string>& dataFolders);

// Specular power (Skyrim glossiness) for a Fallout 4 smoothness
float GlossinessFromSmoothness(float smoothness);

// Sets up a Skyrim lighting shader from a Fallout 4 material. Returns what was taken over ("applied") and what has
// no place in the target shader ("notes").
nlohmann::json ApplyMaterial(nifly::NifFile& nif, nifly::NiShape* shape, MaterialFile& mat);

// Imports the shapes in global space with their UVs, normals, vertex colors, shader and (from a set) slider data.
// Bone weights aren't kept, since the skeletons differ. The caller refreshes the window.
OpResult Import(OutfitProject& project, const Options& options, Report& outReport);

} // namespace GameImport
