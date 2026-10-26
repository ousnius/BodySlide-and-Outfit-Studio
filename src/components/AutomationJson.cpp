/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AutomationJson.h"

#include <set>

using json = nlohmann::json;

namespace {

const char* const KeyType = "type";
const char* const KeyActive = "active";
const char* const KeyNote = "Note";
const char* const KeyTargetMeshes = "TargetMeshes";
const char* const KeyTargetRegex = "TargetRegex";
const char* const KeyTargetReference = "TargetReference";
const char* const KeyShaderProperties = "ShaderProperties";
const char* const KeyGeometryProperties = "GeometryProperties";
const char* const KeyTexturePaths = "TexturePaths";

// The struct list a step type takes, if any
const char* StructListKey(AutomationStepType type) {
	switch (type) {
		case AutomationStepType::SetShaderProperties: return KeyShaderProperties;
		case AutomationStepType::SetGeometryProperties: return KeyGeometryProperties;
		case AutomationStepType::SetTexturePaths: return KeyTexturePaths;
		default: return nullptr;
	}
}

bool ReadStringList(const json& value, std::vector<std::string>& out) {
	if (value.is_string()) {
		out = SplitCommaSeparated(value.get<std::string>());
		return true;
	}

	if (!value.is_array())
		return false;

	out.clear();
	for (auto& item : value) {
		if (!item.is_string())
			return false;
		out.push_back(item.get<std::string>());
	}
	return true;
}

std::string TypeError(const std::string& key, const char* expected) {
	return "'" + key + "' must be " + expected + ".";
}

bool ReadStructList(AutomationStepType type, const json& value, AutomationStep& step, std::string& error) {
	const std::string key = StructListKey(type);
	if (!value.is_array()) {
		error = TypeError(key, "an array of objects");
		return false;
	}

	for (auto& item : value) {
		if (!item.is_object()) {
			error = TypeError(key, "an array of objects");
			return false;
		}

		try {
			if (type == AutomationStepType::SetShaderProperties) {
				AutomationStep::ShaderProperty prop;
				prop.name = item.value("name", "");
				prop.stringValue = item.value("stringValue", "");
				prop.value1 = item.value("value1", prop.value1);
				prop.value2 = item.value("value2", prop.value2);
				prop.value3 = item.value("value3", prop.value3);
				prop.value4 = item.value("value4", prop.value4);
				step.shaderProperties.push_back(prop);
			}
			else if (type == AutomationStepType::SetGeometryProperties) {
				AutomationStep::GeometryProperty prop;
				prop.name = item.value("name", "");
				prop.enabled = item.value("enabled", prop.enabled);
				step.geometryProperties.push_back(prop);
			}
			else {
				AutomationStep::TexturePath path;
				path.index = item.value("index", path.index);
				path.name = item.value("name", "");
				path.path = item.value("path", "");
				step.texturePaths.push_back(path);
			}
		}
		catch (const json::exception& e) {
			error = "Invalid entry in '" + key + "': " + e.what();
			return false;
		}
	}

	return true;
}

json WriteStructList(const AutomationStep& step) {
	json list = json::array();
	switch (step.type) {
		case AutomationStepType::SetShaderProperties:
			for (auto& prop : step.shaderProperties)
				list.push_back({{"name", prop.name}, {"stringValue", prop.stringValue}, {"value1", prop.value1}, {"value2", prop.value2}, {"value3", prop.value3}, {"value4", prop.value4}});
			break;
		case AutomationStepType::SetGeometryProperties:
			for (auto& prop : step.geometryProperties)
				list.push_back({{"name", prop.name}, {"enabled", prop.enabled}});
			break;
		case AutomationStepType::SetTexturePaths:
			for (auto& path : step.texturePaths)
				list.push_back({{"index", path.index}, {"name", path.name}, {"path", path.path}});
			break;
		default: break;
	}
	return list;
}

const char* KindName(AutomationFieldKind kind) {
	switch (kind) {
		case AutomationFieldKind::Bool: return "bool";
		case AutomationFieldKind::Int: return "int";
		case AutomationFieldKind::Float: return "float";
		case AutomationFieldKind::String: return "string";
		case AutomationFieldKind::StringList: return "string list";
	}
	return "";
}

} // namespace

json AutomationStepToJson(const AutomationStep& step, bool includeDefaults) {
	const AutomationStepInfo& info = GetAutomationStepInfo(step.type);
	// String parameters can have defaults too, which only a default constructed step knows
	static const AutomationStep defaults;

	json out = json::object();
	out[KeyType] = info.xmlName;
	if (includeDefaults || !step.active)
		out[KeyActive] = step.active;
	if (includeDefaults || !step.note.empty())
		out[KeyNote] = step.note;
	if (includeDefaults || !step.targetMeshes.empty())
		out[KeyTargetMeshes] = step.targetMeshes;
	if (includeDefaults || step.targetRegex)
		out[KeyTargetRegex] = step.targetRegex;
	if (includeDefaults || step.targetReference)
		out[KeyTargetReference] = step.targetReference;

	for (const AutomationField& field : info.fields) {
		switch (field.kind) {
			case AutomationFieldKind::Bool: {
				bool value = step.*field.member.asBool;
				if (includeDefaults || value != field.defaultValue.asBool)
					out[field.xmlName] = value;
				break;
			}
			case AutomationFieldKind::Int: {
				int value = step.*field.member.asInt;
				if (includeDefaults || value != field.defaultValue.asInt)
					out[field.xmlName] = value;
				break;
			}
			case AutomationFieldKind::Float: {
				float value = step.*field.member.asFloat;
				if (includeDefaults || value != field.defaultValue.asFloat)
					out[field.xmlName] = value;
				break;
			}
			case AutomationFieldKind::String: {
				const std::string& value = step.*field.member.asString;
				if (includeDefaults || value != defaults.*field.member.asString)
					out[field.xmlName] = value;
				break;
			}
			case AutomationFieldKind::StringList: {
				const std::vector<std::string>& value = step.*field.member.asStringList;
				if (includeDefaults || value != defaults.*field.member.asStringList)
					out[field.xmlName] = value;
				break;
			}
		}
	}

	if (const char* listKey = StructListKey(step.type)) {
		json list = WriteStructList(step);
		if (includeDefaults || !list.empty())
			out[listKey] = list;
	}

	return out;
}

bool AutomationStepFromJson(const json& in, AutomationStep& outStep, std::string& error) {
	if (!in.is_object()) {
		error = "A step must be a JSON object.";
		return false;
	}

	auto typeIt = in.find(KeyType);
	if (typeIt == in.end() || !typeIt->is_string()) {
		error = "A step needs a 'type' string.";
		return false;
	}

	const std::string typeName = typeIt->get<std::string>();
	const AutomationStepInfo* info = nullptr;
	for (const auto& candidate : GetAutomationStepTypes()) {
		if (typeName == candidate.xmlName) {
			info = &candidate;
			break;
		}
	}

	if (!info) {
		error = "Unknown step type '" + typeName + "'.";
		return false;
	}

	AutomationStep step;
	step.type = info->type;

	// Parameters missing from the JSON get their defaults, the same as when loading XML
	for (const AutomationField& field : info->fields) {
		switch (field.kind) {
			case AutomationFieldKind::Bool: step.*field.member.asBool = field.defaultValue.asBool; break;
			case AutomationFieldKind::Int: step.*field.member.asInt = field.defaultValue.asInt; break;
			case AutomationFieldKind::Float: step.*field.member.asFloat = field.defaultValue.asFloat; break;
			default: break;
		}
	}

	const char* listKey = StructListKey(info->type);

	for (auto it = in.begin(); it != in.end(); ++it) {
		const std::string& key = it.key();
		const json& value = it.value();

		if (key == KeyType)
			continue;

		if (key == KeyActive) {
			if (!value.is_boolean()) {
				error = TypeError(key, "true or false");
				return false;
			}
			step.active = value.get<bool>();
			continue;
		}

		if (key == KeyNote) {
			if (!value.is_string()) {
				error = TypeError(key, "a string");
				return false;
			}
			step.note = value.get<std::string>();
			continue;
		}

		if (key == KeyTargetMeshes) {
			if (!ReadStringList(value, step.targetMeshes)) {
				error = TypeError(key, "an array of strings");
				return false;
			}
			continue;
		}

		if (key == KeyTargetRegex) {
			if (!value.is_boolean()) {
				error = TypeError(key, "true or false");
				return false;
			}
			step.targetRegex = value.get<bool>();
			continue;
		}

		if (key == KeyTargetReference) {
			if (!value.is_boolean()) {
				error = TypeError(key, "true or false");
				return false;
			}
			step.targetReference = value.get<bool>();
			continue;
		}

		if (listKey && key == listKey) {
			if (!ReadStructList(info->type, value, step, error))
				return false;
			continue;
		}

		const AutomationField* field = nullptr;
		for (const AutomationField& candidate : info->fields) {
			if (key == candidate.xmlName) {
				field = &candidate;
				break;
			}
		}

		if (!field) {
			error = "Step type '" + typeName + "' has no parameter '" + key + "'.";
			return false;
		}

		switch (field->kind) {
			case AutomationFieldKind::Bool:
				if (!value.is_boolean()) {
					error = TypeError(key, "true or false");
					return false;
				}
				step.*field->member.asBool = value.get<bool>();
				break;
			case AutomationFieldKind::Int:
				if (!value.is_number_integer()) {
					error = TypeError(key, "an integer");
					return false;
				}
				step.*field->member.asInt = value.get<int>();
				break;
			case AutomationFieldKind::Float:
				if (!value.is_number()) {
					error = TypeError(key, "a number");
					return false;
				}
				step.*field->member.asFloat = value.get<float>();
				break;
			case AutomationFieldKind::String:
				if (!value.is_string()) {
					error = TypeError(key, "a string");
					return false;
				}
				step.*field->member.asString = value.get<std::string>();
				break;
			case AutomationFieldKind::StringList:
				if (!ReadStringList(value, step.*field->member.asStringList)) {
					error = TypeError(key, "an array of strings");
					return false;
				}
				break;
		}
	}

	outStep = std::move(step);
	return true;
}

json AutomationStepSchemaJson(AutomationStepType type) {
	const AutomationStepInfo& info = GetAutomationStepInfo(type);

	json params = json::array();
	for (const AutomationField& field : info.fields) {
		json param = {{"name", field.xmlName}, {"type", KindName(field.kind)}};
		switch (field.kind) {
			case AutomationFieldKind::Bool: param["default"] = field.defaultValue.asBool; break;
			case AutomationFieldKind::Int: param["default"] = field.defaultValue.asInt; break;
			case AutomationFieldKind::Float: param["default"] = field.defaultValue.asFloat; break;
			default: break;
		}
		params.push_back(param);
	}

	json schema = {{"type", info.xmlName}, {"category", info.category}, {"name", info.displayName}, {"parameters", params}};
	if (const char* listKey = StructListKey(type))
		schema["structList"] = listKey;

	return schema;
}
