/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GameUtil.h"
#include "ConfigurationManager.h"
#include "StringStuff.h"
#include "../../lib/FSEngine/FSManager.h"

#include <wx/dir.h>
#include <wx/filename.h>
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

	if (!Config[cust].IsEmpty())
		dataPath = Config[cust];
	else
		dataPath = GetRegistryDataPath(targ);

	return dataPath;
}

wxString GameUtil::GetRegistryDataPath(int targ) {
#if defined(_WINDOWS) && wxUSE_REGKEY
	wxString gamestr = GameUtil::TargetGames[targ];
	wxString gameKey = Config["GameRegKey/" + gamestr];
	wxString gameVal = Config["GameRegVal/" + gamestr];
	if (gameKey.empty())
		return wxEmptyString;

	// Bethesda keys are in the 32-bit view, Steam uninstall keys (Starfield) in the 64-bit view
	for (auto viewMode : {wxRegKey::WOW64ViewMode_32, wxRegKey::WOW64ViewMode_64}) {
		wxRegKey key(wxRegKey::HKLM, gameKey, viewMode);
		wxString installPath;
		if (key.Exists() && key.HasValue(gameVal) && key.QueryValue(gameVal, installPath) && !installPath.empty()) {
			if (!installPath.EndsWith("\\") && !installPath.EndsWith("/"))
				installPath.Append(PathSepChar);

			installPath.Append("Data").Append(PathSepChar);
			return installPath;
		}
	}
#else
	(void)targ;
#endif
	return wxEmptyString;
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

	// Extra data folders have no plugin load order, so all of their archives load
	for (auto& folder : GetExtraDataFolders(targ)) {
		if (!wxDirExists(wxString::FromUTF8(folder)))
			continue;

		wxArrayString extraFiles;
		wxDir::GetAllFiles(wxString::FromUTF8(folder), &extraFiles, "*.ba2", wxDIR_FILES);
		wxDir::GetAllFiles(wxString::FromUTF8(folder), &extraFiles, "*.bsa", wxDIR_FILES);
		for (auto& f : extraFiles)
			outList.push_back(f.ToUTF8().data());
	}
}

std::vector<std::string> GameUtil::GetExtraDataFolders(int targ) {
	std::vector<std::string> folders;
	wxStringTokenizer tokenizer(Config["ExtraDataFolders/" + TargetGames[targ]], ";");
	while (tokenizer.HasMoreTokens()) {
		wxString folder = tokenizer.GetNextToken().Trim(false).Trim();
		if (folder.empty())
			continue;
		if (!folder.EndsWith("/") && !folder.EndsWith("\\"))
			folder += wxFileName::GetPathSeparator();
		folders.push_back(folder.ToUTF8().data());
	}
	return folders;
}

void GameUtil::SetExtraDataFolders(int targ, const std::vector<std::string>& folders) {
	std::string value;
	for (auto& folder : folders) {
		if (folder.empty())
			continue;
		if (!value.empty())
			value += ";";
		value += folder;
	}
	Config.SetValue("ExtraDataFolders/" + TargetGames[targ].ToStdString(), value);
}
