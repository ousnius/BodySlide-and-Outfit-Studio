/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GameUtil.h"
#include "ConfigurationManager.h"
#include "StringStuff.h"
#include "../../lib/FSEngine/FSManager.h"

#include <wx/dir.h>
#include <wx/tokenzr.h>

#ifdef _WINDOWS
	#include <wx/msw/registry.h>
#endif

extern ConfigurationManager Config;

// TargetGames mapping - must match enum in BodySlideApp.h and OutfitStudio.h
// enum TargetGame { FO3, FONV, SKYRIM, FO4, SKYRIMSE, FO4VR, SKYRIMVR, FO76, OB, SF };
namespace GameUtil {
const std::array<wxString, 10> TargetGames = {
	"Fallout3", "FalloutNewVegas", "Skyrim", "Fallout4", "SkyrimSpecialEdition",
	"Fallout4VR", "SkyrimVR", "Fallout76", "Oblivion", "Starfield"
};
} // namespace GameUtil

wxString GameUtil::GetGameDataPath(int targ) {
	wxString dataPath;
	wxString gamestr = GameUtil::TargetGames[targ];
	wxString cust = "GameDataPaths/" + gamestr;

	if (!Config[cust].IsEmpty()) {
		dataPath = Config[cust];
	}
#ifdef _WINDOWS
	else {
		wxString gkey = "GameRegKey/" + gamestr;
		wxString gval = "GameRegVal/" + gamestr;
		std::string gameKey = Config[gkey].ToStdString();
		// Try to read from registry if available
		#ifdef wxUSE_REGKEY
			wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
			if (!gameKey.empty() && key.Exists()) {
				if (key.HasValues() && key.QueryValue(Config[gval], dataPath)) {
					dataPath.Append("Data").Append(PathSepChar);
				}
			}
		#endif
	}
#endif
	return dataPath;
}

void GameUtil::InitArchives() {
	// Auto-detect archives
	FSManager::del();

	std::vector<std::string> fileList;
	GetArchiveFiles(fileList);

	FSManager::addArchives(fileList);
}

void GameUtil::GetArchiveFiles(std::vector<std::string>& outList) {
	int targ = Config.GetIntValue("TargetGame");
	std::string cp = "GameDataFiles/" + GameUtil::TargetGames[targ].ToStdString();
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false);
		val = val.Trim().MakeLower();
		fsearch[val] = true;
	}

	wxString dataDir = Config["GameDataPath"];
	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& f : files) {
		f = f.AfterLast('/').AfterLast('\\');
		if (fsearch.find(f.Lower()) == fsearch.end())
			outList.push_back((dataDir + f).ToUTF8().data());
	}
}
