/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <vector>
#include <array>
#include <wx/string.h>

namespace GameUtil {
	/// Shared array mapping target game indices to game names
	extern const std::array<wxString, 10> TargetGames;

	/// Get the game data path for the specified target game.
	/// Checks config "GameDataPaths/<gamename>" first, then falls back to Windows registry on Windows.
	wxString GetGameDataPath(int targ);

	/// Get the game data path from the Windows registry using config "GameRegKey/<gamename>" and "GameRegVal/<gamename>".
	/// Returns an empty string if not found or not on Windows.
	wxString GetRegistryDataPath(int targ);

	/// Initialize archive loading (BSA/BA2 files) for the currently configured game.
	void InitArchives();

	/// Get list of archive files (BSA/BA2) for the currently configured game: the activated ones of the game data
	/// path, then all of the extra data folders.
	void GetArchiveFiles(std::vector<std::string>& outList);

	/// Folders with assets besides the game data path, laid out like it (textures, materials, meshes, archives),
	/// e.g. a mod that is being converted: config "ExtraDataFolders/<gamename>", separated by semicolons. Assets
	/// are looked up in them after the game data path. Each ends with a path separator.
	std::vector<std::string> GetExtraDataFolders(int targ);

	/// Sets the extra data folders of the game in the config, without saving it or reloading the archives.
	void SetExtraDataFolders(int targ, const std::vector<std::string>& folders);
}
