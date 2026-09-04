/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <istream>
#include <memory>
#include <string>

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
//
// "outSourcePath" reports where the contents actually came from, so an editor
// can write them back there: the full path of the loose file, or the data
// folder relative path inside the archive when "outFromArchive" comes back
// true - which is not a writable location, so saving has to go elsewhere.
std::unique_ptr<std::istream> OpenPhysicsXml(const std::string& xmlPath,
											 const std::string& nifFilePath,
											 std::string* outSourcePath = nullptr,
											 bool* outFromArchive = nullptr);
}
