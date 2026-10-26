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
// The folders loose assets are looked up in, in order: the game data folder, then
// the extra data folders of the target game (GameUtil::GetExtraDataFolders).
std::vector<std::string> DataFolders();
// The same with another folder in place of the game data folder
std::vector<std::string> DataFolders(const std::string& baseDataPath);

// The full path of a data folder relative asset ("textures/..."): in the first data
// folder that has it as a loose file, or else under the game data folder, where
// the texture loader turns it back into the archive path.
std::string FindAssetPath(const std::string& relPath);
// The same with another folder in place of the game data folder
std::string FindAssetPath(const std::string& relPath, const std::string& baseDataPath);

// Opens a file at an absolute or working-directory relative path.
std::unique_ptr<std::istream> OpenLoose(const std::string& fullPath);

// Opens a data folder relative path ("meshes/actors/...") in one of the loaded
// archives. A non-empty "archivePath" only reads from the archive at that path.
std::unique_ptr<std::istream> OpenArchive(const std::string& relPath, const std::string& archivePath = {});

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
