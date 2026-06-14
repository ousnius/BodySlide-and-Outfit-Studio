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
	std::string GetGameDataPath(int targ);

	/// Initialize archive loading (BSA/BA2 files) for the currently configured game.
	void InitArchives();

	/// Get list of archive files (BSA/BA2) for the currently configured game.
	void GetArchiveFiles(std::vector<std::string>& outList);
}
