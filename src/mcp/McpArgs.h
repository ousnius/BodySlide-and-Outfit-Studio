/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "McpProtocol.h"

#include <map>
#include <string>
#include <vector>

// Reading tool arguments. Missing required or mistyped arguments throw an McpToolError that
// names the argument, so the agent can correct the call.
namespace McpArgs {

inline const nlohmann::json* Find(const nlohmann::json& args, const char* name) {
	auto it = args.find(name);
	if (it == args.end() || it->is_null())
		return nullptr;
	return &*it;
}

[[noreturn]] inline void Wrong(const char* name, const char* expected) {
	throw McpToolError(std::string("Argument '") + name + "' has to be " + expected + ".");
}

inline bool Has(const nlohmann::json& args, const char* name) {
	return Find(args, name) != nullptr;
}

inline std::string String(const nlohmann::json& args, const char* name) {
	const nlohmann::json* value = Find(args, name);
	if (!value)
		throw McpToolError(std::string("Argument '") + name + "' is missing.");
	if (!value->is_string())
		Wrong(name, "a string");
	return value->get<std::string>();
}

inline std::string String(const nlohmann::json& args, const char* name, const std::string& def) {
	return Has(args, name) ? String(args, name) : def;
}

inline bool Bool(const nlohmann::json& args, const char* name, bool def) {
	const nlohmann::json* value = Find(args, name);
	if (!value)
		return def;
	if (!value->is_boolean())
		Wrong(name, "true or false");
	return value->get<bool>();
}

inline double Number(const nlohmann::json& args, const char* name) {
	const nlohmann::json* value = Find(args, name);
	if (!value)
		throw McpToolError(std::string("Argument '") + name + "' is missing.");
	if (!value->is_number())
		Wrong(name, "a number");
	return value->get<double>();
}

inline float Float(const nlohmann::json& args, const char* name, float def) {
	return Has(args, name) ? static_cast<float>(Number(args, name)) : def;
}

inline int Int(const nlohmann::json& args, const char* name, int def) {
	const nlohmann::json* value = Find(args, name);
	if (!value)
		return def;
	if (!value->is_number_integer())
		Wrong(name, "a whole number");
	return value->get<int>();
}

inline std::vector<std::string> Strings(const nlohmann::json& args, const char* name) {
	std::vector<std::string> list;
	const nlohmann::json* value = Find(args, name);
	if (!value)
		return list;
	// A client with an older schema of the tool may send the list as text, or a single name
	nlohmann::json parsed;
	if (value->is_string()) {
		parsed = nlohmann::json::parse(value->get<std::string>(), nullptr, false);
		if (!parsed.is_array())
			parsed = nlohmann::json::array({*value});
		value = &parsed;
	}
	if (!value->is_array())
		Wrong(name, "an array of strings");
	for (auto& item : *value) {
		if (!item.is_string())
			Wrong(name, "an array of strings");
		list.push_back(item.get<std::string>());
	}
	return list;
}

inline std::vector<int> Ints(const nlohmann::json& args, const char* name) {
	std::vector<int> list;
	const nlohmann::json* value = Find(args, name);
	if (!value)
		return list;
	if (!value->is_array())
		Wrong(name, "an array of whole numbers");
	for (auto& item : *value) {
		if (!item.is_number_integer())
			Wrong(name, "an array of whole numbers");
		list.push_back(item.get<int>());
	}
	return list;
}

// An object of names to numbers, like slider values
inline std::map<std::string, float> FloatMap(const nlohmann::json& args, const char* name) {
	std::map<std::string, float> map;
	const nlohmann::json* value = Find(args, name);
	if (!value)
		return map;
	if (!value->is_object())
		Wrong(name, "an object of names to numbers");
	for (auto& item : value->items()) {
		if (!item.value().is_number())
			Wrong(name, "an object of names to numbers");
		map[item.key()] = item.value().get<float>();
	}
	return map;
}

} // namespace McpArgs

// Builds the input schema of a tool: an object with the given properties
inline nlohmann::json McpSchema(nlohmann::json properties = nlohmann::json::object(), std::vector<std::string> required = {}) {
	nlohmann::json schema = {{"type", "object"}, {"properties", std::move(properties)}};
	if (!required.empty())
		schema["required"] = required;
	return schema;
}

// Shorthands for property schemas
inline nlohmann::json McpProp(const char* type, const std::string& description) {
	return {{"type", type}, {"description", description}};
}

inline nlohmann::json McpStringList(const std::string& description) {
	return {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", description}};
}

inline nlohmann::json McpIntList(const std::string& description) {
	return {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", description}};
}

inline nlohmann::json McpNumberMap(const std::string& description) {
	return {{"type", "object"}, {"additionalProperties", {{"type", "number"}}}, {"description", description}};
}
