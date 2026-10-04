/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace nifly {
class NifFile;
}

// Opening game asset files the way the game's resource system does: loose files
// under the configured data folder first, then the loaded BSA/BA2 archives.
// Shared by BodySlide and Outfit Studio.
namespace GameDataStream {
// Opens a file at an absolute or working-directory relative path.
std::unique_ptr<std::istream> OpenLoose(const std::string& fullPath);

// Opens a data folder relative path ("meshes/actors/...") in one of the loaded
// archives.
std::unique_ptr<std::istream> OpenArchive(const std::string& relPath);

// Resolves a physics XML path as referenced by a "HDT Skinned Mesh Physics
// Object" NiStringExtraData, e.g.
// "SKSE\Plugins\hdtSkinnedMeshConfigs\outfit.xml". Looks for a loose file under
// the game data folder, then relative to "nifFilePath" (the NIF the link came
// from, for mods previewed straight out of their own folder), then in the
// archives. Returns nullptr when it cannot be found.
std::unique_ptr<std::istream> OpenPhysicsXml(const std::string& xmlPath, const std::string& nifFilePath);

// Opens the .mesh file a Starfield shape keeps its geometry in, by the path
// the shape references (usually "<hash>\<hash>", relative to "geometries\").
// Looks for a loose file under the data folder, then beside the meshes folder
// (or in the folder) of "nifFilePath", then in the archives. Returns nullptr
// when it cannot be found.
std::unique_ptr<std::istream> OpenExternalGeometry(const std::string& meshPath, const std::string& dataPath, const std::string& nifFilePath);

// Loads the external geometry of every shape of a Starfield NIF, which only
// references its .mesh files until then. Returns the paths of the meshes that
// could not be found, whose shapes are left without geometry.
std::vector<std::string> LoadExternalGeometry(nifly::NifFile& nif, const std::string& dataPath, const std::string& nifFilePath);
}
