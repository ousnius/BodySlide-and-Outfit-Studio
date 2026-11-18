#include "../src/utils/ConfigurationManager.h"
#include "../src/utils/ProjectUtil.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include <string>

extern ConfigurationManager Config;

namespace {
// Temporary folder with an app dir, a project dir and a game data dir, restoring the config afterwards
struct PathScope {
	std::filesystem::path root;
	std::string previousProjectPath;
	std::string previousAppDir;
	std::string previousGameDataPath;

	PathScope() {
		std::random_device random;
		root = std::filesystem::temp_directory_path() / ("BSOSTests-ProjectUtil-" + std::to_string(random()));
		std::filesystem::create_directories(root / "App");
		std::filesystem::create_directories(root / "Projects");
		std::filesystem::create_directories(root / "Data");

		previousProjectPath = Config["ProjectPath"];
		previousAppDir = Config["AppDir"];
		previousGameDataPath = Config["GameDataPath"];
		Config.SetValue("ProjectPath", "");
		Config.SetValue("AppDir", Path("App"));
		Config.SetValue("GameDataPath", Path("Data"));
	}

	~PathScope() {
		Config.SetValue("ProjectPath", previousProjectPath);
		Config.SetValue("AppDir", previousAppDir);
		Config.SetValue("GameDataPath", previousGameDataPath);
		std::error_code error;
		std::filesystem::remove_all(root, error);
	}

	std::string Path(const std::string& sub) const { return (root / sub).u8string(); }
};
} // namespace

TEST_CASE("The configured project path wins over the app dir", "[ProjectUtil]") {
	PathScope scope;
	std::filesystem::create_directories(scope.root / "App" / "SliderSets");

	Config.SetValue("ProjectPath", scope.Path("Projects"));
	CHECK(ProjectUtil::GetProjectPath() == scope.Path("Projects"));
}

TEST_CASE("The app dir with project files is used without a configured project path", "[ProjectUtil]") {
	PathScope scope;
	std::filesystem::create_directories(scope.root / "App" / "SliderSets");
	std::filesystem::create_directories(scope.root / "Data" / "Tools" / "BodySlide");

	CHECK(ProjectUtil::GetProjectPath() == scope.Path("App"));

	// A configured path that doesn't exist falls back the same way
	Config.SetValue("ProjectPath", scope.Path("Missing"));
	CHECK(ProjectUtil::GetProjectPath() == scope.Path("App"));
}

TEST_CASE("The BodySlide folder in the game data path is the fallback", "[ProjectUtil]") {
	PathScope scope;
	std::filesystem::create_directories(scope.root / "Data" / "Tools" / "BodySlide");

	auto path = std::filesystem::path(ProjectUtil::GetProjectPath());
	CHECK(path == scope.root / "Data" / "Tools" / "BodySlide");

	std::filesystem::remove_all(scope.root / "Data" / "Tools");
	CHECK(ProjectUtil::GetProjectPath() == scope.Path("App"));
}
