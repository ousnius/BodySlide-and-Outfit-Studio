/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectUtil.h"
#include "ConfigurationManager.h"
#include "StringStuff.h"

#include <wx/dir.h>

extern ConfigurationManager Config;

std::string ProjectUtil::GetProjectPath() {
	std::string projectPath = Config["ProjectPath"];
	std::string appDir = Config["AppDir"];
	std::string gameDataPath = Config["GameDataPath"];

	// Build list of paths to check in order of preference
	std::vector<std::string> pathsToCheck;

	// First priority: configured ProjectPath (if set)
	if (!projectPath.empty()) {
		pathsToCheck.push_back(projectPath);
	}

	// Fallback paths in order of preference
	pathsToCheck.push_back(appDir + PathSepStr + "SliderSets");
	pathsToCheck.push_back(gameDataPath + PathSepStr + "CalienteTools" + PathSepStr + "BodySlide");
	pathsToCheck.push_back(gameDataPath + PathSepStr + "Tools" + PathSepStr + "BodySlide");

	// Return first existing path
	for (const auto& path : pathsToCheck) {
		if (wxDir::Exists(path)) {
			return path;
		}
	}

	// If no path exists, return projectPath if configured, otherwise AppDir
	return !projectPath.empty() ? projectPath : appDir;
}
