#include "../src/components/AutomationJson.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>

using json = nlohmann::json;

TEST_CASE("Every step type survives a JSON round trip", "[AutomationJson]") {
	for (const auto& info : GetAutomationStepTypes()) {
		CAPTURE(info.xmlName);

		AutomationStep step;
		std::string error;
		REQUIRE(AutomationStepFromJson({{"type", info.xmlName}}, step, error));
		CHECK(step.type == info.type);

		json full = AutomationStepToJson(step, true);
		AutomationStep again;
		REQUIRE(AutomationStepFromJson(full, again, error));
		CHECK(AutomationStepToJson(again, true) == full);

		// Default parameters are left out unless asked for
		CHECK(AutomationStepToJson(step) == json{{"type", info.xmlName}});
	}
}

TEST_CASE("JSON steps match the XML format", "[AutomationJson]") {
	json in = {
		{"type", "ConformSliders"},
		{"Note", "conform the skirt"},
		{"TargetMeshes", {"skirt", "belt"}},
		{"ProximityRadius", 7.5},
		{"MaxResults", 3},
		{"SliderNames", "Breasts, Butt"},
	};

	AutomationStep step;
	std::string error;
	REQUIRE(AutomationStepFromJson(in, step, error));
	CHECK(step.targetMeshes == std::vector<std::string>{"skirt", "belt"});
	CHECK(step.conformSliderNames == std::vector<std::string>{"Breasts", "Butt"});

	auto path = std::filesystem::temp_directory_path() / "bsos_automation_json_test.xml";
	AutomationScript script;
	script.AddStep(step);
	REQUIRE(script.Save(path.string()) == 0);

	AutomationScript loaded;
	REQUIRE(loaded.Load(path.string()) == 0);
	std::remove(path.string().c_str());

	REQUIRE(loaded.GetSteps().size() == 1);
	CHECK(AutomationStepToJson(loaded.GetSteps()[0]) == AutomationStepToJson(step));
}

TEST_CASE("Invalid JSON steps are rejected with a message", "[AutomationJson]") {
	AutomationStep step;
	std::string error;

	CHECK_FALSE(AutomationStepFromJson({{"type", "NoSuchStep"}}, step, error));
	CHECK(error.find("NoSuchStep") != std::string::npos);

	CHECK_FALSE(AutomationStepFromJson({{"type", "ConformSliders"}, {"Radius", 5}}, step, error));
	CHECK(error.find("Radius") != std::string::npos);

	CHECK_FALSE(AutomationStepFromJson({{"type", "ConformSliders"}, {"MaxResults", "many"}}, step, error));
	CHECK(error.find("MaxResults") != std::string::npos);

	CHECK_FALSE(AutomationStepFromJson(json::array(), step, error));
}

TEST_CASE("Struct list parameters round trip", "[AutomationJson]") {
	json in = {
		{"type", "SetTexturePaths"},
		{"TexturePaths", {{{"index", 1}, {"name", "Normal"}, {"path", "textures\\a_n.dds"}}}},
	};

	AutomationStep step;
	std::string error;
	REQUIRE(AutomationStepFromJson(in, step, error));
	REQUIRE(step.texturePaths.size() == 1);
	CHECK(step.texturePaths[0].path == "textures\\a_n.dds");
	CHECK(AutomationStepToJson(step)["TexturePaths"] == in["TexturePaths"]);
	CHECK(ResolveAutomationTexturePathIndex(step.texturePaths[0]) == 1);
	CHECK(AutomationTexturePathIndexForName("Env Mask") == 5);
}
