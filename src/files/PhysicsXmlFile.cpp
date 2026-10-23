/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsXmlFile.h"

#include "../utils/StringStuff.h"

#include <tinyxml2.h>

#include <algorithm>
#include <charconv>
#include <iterator>
#include <map>

using namespace tinyxml2;

namespace {
// SMP reads numbers with a decimal point or comma
bool ReadFloat(const char* text, float& value) {
	if (!text)
		return false;

	std::string s = TrimString(text);
	std::replace(s.begin(), s.end(), ',', '.');
	auto result = std::from_chars(s.data(), s.data() + s.size(), value);
	return result.ec == std::errc();
}

std::string Attribute(const XMLElement* element, const char* name) {
	const char* value = element->Attribute(name);
	return value ? value : "";
}
} // namespace

bool PhysicsXmlFile::Shape::Declares(const std::string& bone) const {
	return std::any_of(declaredBones.begin(), declaredBones.end(), [&](const std::string& b) { return StringsEqualInsens(b.c_str(), bone.c_str()); });
}

bool PhysicsXmlFile::Load(std::istream& stream) {
	std::string xml{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
	return Parse(xml);
}

bool PhysicsXmlFile::Parse(const std::string& xml) {
	error.clear();
	bones.clear();
	shapes.clear();

	XMLDocument doc;
	if (doc.Parse(xml.data(), xml.size()) != XML_SUCCESS) {
		error = doc.ErrorStr() ? doc.ErrorStr() : "The XML can't be parsed.";
		return false;
	}

	const XMLElement* system = doc.FirstChildElement("system");
	if (!system) {
		error = "The root element isn't <system>.";
		return false;
	}

	// Mass of each bone template by name. The unnamed one is the default that undeclared bones get, and an
	// unnamed <bone-default> changes it from that line on.
	std::map<std::string, float, case_insensitive_compare> templateMass{{"", 0.0f}};

	for (const XMLElement* element = system->FirstChildElement(); element; element = element->NextSiblingElement()) {
		const std::string tag = element->Name();
		if (tag == "bone-default") {
			auto base = templateMass.find(Attribute(element, "extends"));
			float mass = base != templateMass.end() ? base->second : templateMass[""];
			if (const XMLElement* massElement = element->FirstChildElement("mass"))
				ReadFloat(massElement->GetText(), mass);
			templateMass[Attribute(element, "name")] = mass;
		}
		else if (tag == "bone") {
			// SMP skips a bone that's declared twice
			const std::string name = Attribute(element, "name");
			if (!name.empty() && std::none_of(bones.begin(), bones.end(), [&](const std::string& b) { return StringsEqualInsens(b.c_str(), name.c_str()); }))
				bones.push_back(name);
		}
		else if (tag == "per-vertex-shape" || tag == "per-triangle-shape") {
			Shape shape;
			shape.name = Attribute(element, "name");
			shape.declaredBones = bones;
			shape.implicitMass = templateMass[""];
			shapes.push_back(std::move(shape));
		}
	}
	return true;
}

const PhysicsXmlFile::Shape* PhysicsXmlFile::FindShape(const std::string& name) const {
	auto it = std::find_if(shapes.begin(), shapes.end(), [&](const Shape& s) { return StringsEqualInsens(s.name.c_str(), name.c_str()); });
	return it != shapes.end() ? &*it : nullptr;
}
