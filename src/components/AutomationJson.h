/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Automation.h"

#include <nlohmann/json.hpp>

#include <string>

// Automation steps as JSON objects, with the same names as the XML format:
//   {"type": "ConformSliders", "active": true, "Note": "...", "TargetMeshes": ["top1"], "TargetRegex": false,
//    "ProximityRadius": 10.0, "SliderNames": ["Breasts"], ...}
// String lists are arrays (a comma separated string is accepted too). The struct lists of a few
// step types use "ShaderProperties", "GeometryProperties" and "TexturePaths" arrays of objects.

// Parameters that have their default value are left out unless includeDefaults is set.
nlohmann::json AutomationStepToJson(const AutomationStep& step, bool includeDefaults = false);

// Fails with a message for an unknown step type, a key the step type doesn't have,
// or a value of the wrong type. Missing keys keep their defaults.
bool AutomationStepFromJson(const nlohmann::json& json, AutomationStep& outStep, std::string& error);

// Describes the parameters of a step type: name, value type and default value.
nlohmann::json AutomationStepSchemaJson(AutomationStepType type);
