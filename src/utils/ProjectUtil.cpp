/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectUtil.h"
#include "ConfigurationManager.h"
#include "StringStuff.h"

#include <wx/dir.h>

std::string ProjectUtil::GetProjectPath() {
	std::string projectPath = Config["ProjectPath"];
	std::string appDir = Config["AppDir"];
	std::string gameDataPath = Config["GameDataPath"];

	// First priority: configured ProjectPath (if set)
	if (!projectPath.empty() && wxDir::Exists(projectPath)) {
		return projectPath;
	}

	// Then the app dir, if it has project files
	if (wxDir::Exists(appDir + PathSepStr + "SliderSets")) {
		return appDir;
	}

	// Fallback paths in order of preference
	std::vector<std::string> pathsToCheck;
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
