/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/AutomationJson.h"
#include "../program/AutomationRunner.h"
#include "../program/OutfitStudio.h"
#include "../utils/ProjectUtil.h"

#include <wx/dir.h>
#include <wx/filename.h>

using nlohmann::json;

namespace {

std::string AutomationsFolder() {
	return ProjectUtil::GetProjectPath() + "/Automations";
}

std::map<std::string, std::string> StringMap(const json& args, const char* name) {
	std::map<std::string, std::string> map;
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return map;
	if (!value->is_object())
		McpArgs::Wrong(name, "an object of names to strings");
	for (auto& item : value->items()) {
		if (!item.value().is_string())
			McpArgs::Wrong(name, "an object of names to strings");
		map[item.key()] = item.value().get<std::string>();
	}
	return map;
}

// Script name relative to the Automations folder, without extension, as the automation dialog shows it
std::string ScriptPath(const std::string& name) {
	if (name.find("..") != std::string::npos)
		throw McpToolError("Script names can't leave the Automations folder.");
	return AutomationsFolder() + "/" + name + ".xml";
}

AutomationScript LoadScript(const std::string& name) {
	std::string path = ScriptPath(name);
	if (!wxFileName::FileExists(wxString::FromUTF8(path)))
		throw McpToolError("There is no automation script '" + name + "'. list_automation_scripts lists them.");

	AutomationScript script;
	if (script.Load(path) != 0)
		throw McpToolError("The automation script '" + name + "' couldn't be read.");
	return script;
}

std::vector<AutomationStep> StepsFromJson(const json& list) {
	if (!list.is_array())
		throw McpToolError("Argument 'steps' has to be an array of step objects.");

	std::vector<AutomationStep> steps;
	for (size_t i = 0; i < list.size(); i++) {
		AutomationStep step;
		std::string error;
		if (!AutomationStepFromJson(list[i], step, error))
			throw McpToolError("Step " + std::to_string(i) + ": " + error);
		steps.push_back(step);
	}
	return steps;
}

McpToolResult StepsResult(const json& steps, bool failed) {
	McpToolResult result;
	result.data["steps"] = steps;
	result.isError = failed;
	if (failed)
		result.data["error"] = "A step failed, see 'steps' and 'log'.";
	return result;
}

} // namespace

json McpRunSteps(OutfitStudioMcp& mcp,
				 const std::vector<AutomationStep>& steps,
				 const std::map<std::string, std::string>& variables,
				 bool stopOnError,
				 bool& failed,
				 const ObjImportOptions* objOptions) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	failed = false;

	// Placeholders are filled in up front, like the automation dialog does
	AutomationScript execScript;
	for (auto& step : steps)
		execScript.AddStep(step);
	if (!variables.empty())
		execScript.SubstitutePlaceholders(variables);

	AutomationRunner runner(frame);
	// Imports take their default options instead of asking
	runner.SetHeadless(true);
	if (objOptions)
		runner.SetObjImportOptions(*objOptions);
	runner.BeginRun(variables);

	json results = json::array();
	for (size_t i = 0; i < execScript.GetSteps().size(); i++) {
		const AutomationStep& step = execScript.GetSteps()[i];
		json entry = {{"index", i}, {"type", AutomationStepTypeToString(step.type)}};

		if (!step.active) {
			entry["skipped"] = true;
			results.push_back(entry);
			continue;
		}

		if (failed && stopOnError) {
			entry["skipped"] = true;
			results.push_back(entry);
			continue;
		}

		int code = runner.RunStep(step);
		entry["ok"] = code == 0;
		if (code != 0) {
			entry["code"] = code;
			failed = true;
		}
		results.push_back(entry);
	}

	return results;
}

void McpRunStep(OutfitStudioMcp& mcp, const json& stepJson, const ObjImportOptions* objOptions) {
	AutomationStep step;
	std::string error;
	if (!AutomationStepFromJson(stepJson, step, error))
		throw McpToolError(error);

	bool failed = false;
	McpRunSteps(mcp, {step}, {}, true, failed, objOptions);
	if (failed)
		throw McpToolError("The " + stepJson.value("type", std::string()) + " step failed, see 'log'.");
}

std::string McpFilePath(const std::string& path) {
	return AutomationRunner::MakeAbsoluteToProject(wxString::FromUTF8(path)).ToUTF8().data();
}

void McpCheckOverwrite(const std::string& path, const json& args) {
	if (wxFileName::FileExists(wxString::FromUTF8(path)) && !McpArgs::Bool(args, "overwrite", false))
		throw McpToolError("The file '" + path + "' exists. Pass overwrite: true to replace it.");
}

void AddAutomationTools(OutfitStudioMcp& mcp) {
	McpTool runSteps;
	runSteps.name = "run_steps";
	runSteps.description =
		"Runs automation steps on the project, the same way an automation script does, and refreshes the window. "
		"Covers everything automation can do: loading references and projects, importing, conforming, bone "
		"weights, bones, geometry, transforms, partitions, masks, sliders, shader properties, saving and exporting. "
		"Each step is an object with \"type\" and the step's parameters by name, as get_step_schema describes "
		"them, e.g. {\"type\": \"ConformSliders\", \"TargetMeshes\": [\"Top\"]}. Every step also takes "
		"\"TargetMeshes\" (shape names, empty for the selected shapes or all, depending on the step), "
		"\"TargetRegex\" (names are regular expressions) and \"active\". Values can use {variables}.";
	runSteps.inputSchema = McpSchema(
		{
			{"steps", {{"type", "array"}, {"items", {{"type", "object"}}}, {"description", "Steps to run in order"}}},
			{"variables", {{"type", "object"}, {"additionalProperties", {{"type", "string"}}}, {"description", "Values for {name} placeholders in the steps"}}},
			{"stopOnError", McpProp("boolean", "Skip the remaining steps after a failed one. Default true.")},
		},
		{"steps"});
	runSteps.handler = [&mcp](const json& args) {
		std::vector<AutomationStep> steps = StepsFromJson(args.at("steps"));
		auto variables = StringMap(args, "variables");

		bool failed = false;
		json results = McpRunSteps(mcp, steps, variables, McpArgs::Bool(args, "stopOnError", true), failed);
		return StepsResult(results, failed);
	};
	mcp.AddTool(std::move(runSteps));

	McpTool schema;
	schema.name = "get_step_schema";
	schema.description =
		"Describes automation step types for run_steps: their parameters with type and default value. "
		"Without types, lists all step types with their category and name.";
	schema.inputSchema = McpSchema({{"types", McpStringList("Step types to describe, e.g. [\"ConformSliders\", \"SaveProject\"]")}});
	schema.readOnly = true;
	schema.handler = [](const json& args) {
		McpToolResult result;
		std::vector<std::string> types = McpArgs::Strings(args, "types");
		if (types.empty()) {
			json list = json::array();
			for (auto& info : GetAutomationStepTypes())
				list.push_back({{"type", info.xmlName}, {"category", info.category}, {"name", info.displayName}});
			result.data["stepTypes"] = list;
			return result;
		}

		json list = json::array();
		for (auto& typeName : types) {
			AutomationStepType type = AutomationStepTypeFromString(typeName);
			if (AutomationStepTypeToString(type) != typeName)
				throw McpToolError("Unknown step type '" + typeName + "'. Call get_step_schema without types to list them.");
			list.push_back(AutomationStepSchemaJson(type));
		}
		result.data["stepTypes"] = list;
		return result;
	};
	mcp.AddTool(std::move(schema));

	McpTool listScripts;
	listScripts.name = "list_automation_scripts";
	listScripts.description = "Lists the automation scripts in the Automations folder by name, relative to it and without extension.";
	listScripts.readOnly = true;
	listScripts.handler = [](const json&) {
		wxArrayString files;
		wxString folder = wxString::FromUTF8(AutomationsFolder());
		if (wxDir::Exists(folder))
			wxDir::GetAllFiles(folder, &files, "*.xml");

		json names = json::array();
		for (auto& file : files) {
			wxFileName fileName(file);
			fileName.MakeRelativeTo(folder);
			fileName.ClearExt();
			names.push_back(fileName.GetFullPath(wxPATH_UNIX).ToUTF8().data());
		}

		McpToolResult result;
		result.data["scripts"] = names;
		return result;
	};
	mcp.AddTool(std::move(listScripts));

	McpTool getScript;
	getScript.name = "get_automation_script";
	getScript.description = "Returns the steps and variables of an automation script as run_steps takes them, to read, change or run.";
	getScript.inputSchema = McpSchema({{"name", McpProp("string", "Script name as list_automation_scripts gives it")}}, {"name"});
	getScript.readOnly = true;
	getScript.handler = [](const json& args) {
		AutomationScript script = LoadScript(McpArgs::String(args, "name"));

		json steps = json::array();
		for (auto& step : script.GetSteps())
			steps.push_back(AutomationStepToJson(step));

		McpToolResult result;
		result.data["steps"] = steps;
		result.data["variables"] = script.GetVariables();
		result.data["batch"] = script.GetBatchMode() != AutomationBatchMode::None;
		return result;
	};
	mcp.AddTool(std::move(getScript));

	McpTool runScript;
	runScript.name = "run_automation_script";
	runScript.description =
		"Runs the active steps of an automation script on the open project. Batch scripts, which run once per "
		"input file or slider set, aren't run here: get their steps with get_automation_script and run them "
		"with run_steps for each input.";
	runScript.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "Script name as list_automation_scripts gives it")},
			{"variables", {{"type", "object"}, {"additionalProperties", {{"type", "string"}}}, {"description", "Values that replace the script's own variables"}}},
		},
		{"name"});
	runScript.handler = [&mcp](const json& args) {
		AutomationScript script = LoadScript(McpArgs::String(args, "name"));
		if (script.GetBatchMode() != AutomationBatchMode::None)
			throw McpToolError("This is a batch script. Get its steps with get_automation_script and run them with run_steps for each input.");

		auto variables = script.GetVariables();
		for (auto& value : StringMap(args, "variables"))
			variables[value.first] = value.second;

		bool failed = false;
		json results = McpRunSteps(mcp, script.GetSteps(), variables, true, failed);
		return StepsResult(results, failed);
	};
	mcp.AddTool(std::move(runScript));
}
