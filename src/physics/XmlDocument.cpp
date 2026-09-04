/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "XmlDocument.h"

#include "hdt/hdtStringUtils.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>

using namespace tinyxml2;

namespace Physics {
namespace XmlValue {
bool ParseFloat(const std::string& text, float& out) {
	// Mirrors hdt::convertFloat: trim, turn the first decimal comma into a
	// point, then drop a leading '+' that from_chars will not take.
	std::string s = hdt::TrimAsciiWhitespace(text);
	const std::size_t comma = s.find(',');
	if (comma != std::string::npos)
		s.replace(comma, 1, ".");

	const char* begin = s.data();
	const char* end = begin + s.size();
	if (begin != end && *begin == '+')
		++begin;

	float value{};
	const auto result = std::from_chars(begin, end, value);
	if (result.ec != std::errc() || result.ptr != end)
		return false;

	out = value;
	return true;
}

bool ParseInt(const std::string& text, int& out) {
	// Mirrors hdt::convertInt, including its radix sniffing: "0x" is
	// hexadecimal and a lone leading zero is octal.
	const std::string s = hdt::TrimAsciiWhitespace(text);
	const char* begin = s.data();
	const char* end = begin + s.size();
	if (begin != end && *begin == '+')
		++begin;

	int radix = 10;
	if (s.size() >= 2 && s.compare(0, 2, "0x") == 0) {
		radix = 16;
		begin += 2;
	}
	else if (s.size() > 1 && s[0] == '0') {
		begin += 1;
		radix = 8;
	}

	int value{};
	const auto result = std::from_chars(begin, end, value, radix);
	if (result.ec != std::errc() || result.ptr != end)
		return false;

	out = value;
	return true;
}

bool ParseBool(const std::string& text, bool& out) {
	const std::string s = hdt::TrimAsciiWhitespace(text);
	if (s == "true" || s == "1") {
		out = true;
		return true;
	}
	if (s == "false" || s == "0") {
		out = false;
		return true;
	}
	return false;
}

std::string FormatFloat(float value) {
	// Shortest form that reads back as the same float, always with a '.'
	// separator whatever the file happened to use.
	char buffer[64]{};
	const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
	if (result.ec != std::errc())
		return "0";
	return std::string(buffer, result.ptr);
}

std::string FormatInt(int value) {
	return std::to_string(value);
}

std::string FormatBool(bool value) {
	return value ? "true" : "false";
}
}

namespace {
long long NowMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Repeated edits to the same property inside this window fold into one undo
// step.
constexpr long long kCoalesceWindowMs = 500;

bool NameMatches(const XMLElement* element, const ChildDesc& desc) {
	if (!element || !element->Name())
		return false;

	if (std::strcmp(element->Name(), desc.name) == 0)
		return true;

	if (!desc.aliases)
		return false;

	const std::size_t len = std::strlen(element->Name());
	for (const char* p = desc.aliases; *p;) {
		const char* end = std::strchr(p, '|');
		const std::size_t span = end ? static_cast<std::size_t>(end - p) : std::strlen(p);
		if (span == len && std::strncmp(p, element->Name(), span) == 0)
			return true;
		if (!end)
			break;
		p = end + 1;
	}
	return false;
}

std::string ElementText(const XMLElement* element) {
	const char* text = element ? element->GetText() : nullptr;
	return text ? std::string(text) : std::string();
}

float AttrFloat(const XMLElement* element, const char* name) {
	const char* raw = element ? element->Attribute(name) : nullptr;
	float value = 0.0f;
	if (raw)
		XmlValue::ParseFloat(raw, value);
	return value;
}

void SetAttrFloat(XMLElement* element, const char* name, float value) {
	element->SetAttribute(name, XmlValue::FormatFloat(value).c_str());
}

// True when two reads of the same property mean the same thing. Compared by
// value rather than by spelling, so writing a property back unchanged is
// recognised as the no-op it is and leaves both the text and the dirty flag
// alone - the grid echoes values back constantly, and "+1.25" must not turn
// into "1.25" just because the user clicked a row.
bool ValuesEqual(const ValueVariant& a, const ValueVariant& b) {
	if (a.index() != b.index())
		return false;

	if (std::holds_alternative<std::monostate>(a))
		return true;
	if (const auto* left = std::get_if<bool>(&a))
		return *left == std::get<bool>(b);
	if (const auto* left = std::get_if<int>(&a))
		return *left == std::get<int>(b);
	if (const auto* left = std::get_if<float>(&a))
		return *left == std::get<float>(b);
	if (const auto* left = std::get_if<std::string>(&a))
		return *left == std::get<std::string>(b);
	if (const auto* left = std::get_if<nifly::Vector3>(&a)) {
		const auto& right = std::get<nifly::Vector3>(b);
		return left->x == right.x && left->y == right.y && left->z == right.z;
	}
	if (const auto* left = std::get_if<nifly::Quaternion>(&a)) {
		const auto& right = std::get<nifly::Quaternion>(b);
		return left->w == right.w && left->x == right.x && left->y == right.y && left->z == right.z;
	}
	if (const auto* left = std::get_if<AxisAngleValue>(&a)) {
		const auto& right = std::get<AxisAngleValue>(b);
		return left->axis.x == right.axis.x && left->axis.y == right.axis.y && left->axis.z == right.axis.z
			   && left->angle == right.angle;
	}
	if (const auto* left = std::get_if<LerpValue>(&a)) {
		const auto& right = std::get<LerpValue>(b);
		return left->translation == right.translation && left->rotation == right.rotation;
	}
	if (const auto* left = std::get_if<std::vector<std::string>>(&a))
		return *left == std::get<std::vector<std::string>>(b);

	return false;
}

// Position of a descriptor within its element's child list, used to keep a
// newly created child in schema order.
int DescriptorOrder(const ElementDesc& owner, const ChildDesc& desc) {
	for (std::size_t i = 0; i < owner.childCount; i++) {
		if (&owner.children[i] == &desc)
			return static_cast<int>(i);
	}
	return static_cast<int>(owner.childCount);
}
}

XmlDocument::XmlDocument()
	: doc(std::make_unique<XMLDocument>()) {
	CreateEmpty();
}

XmlDocument::~XmlDocument() = default;
XmlDocument::XmlDocument(XmlDocument&&) noexcept = default;
XmlDocument& XmlDocument::operator=(XmlDocument&&) noexcept = default;

void XmlDocument::CreateEmpty() {
	doc->Clear();
	doc->InsertEndChild(doc->NewDeclaration());
	doc->InsertEndChild(doc->NewElement("system"));
	undoStack.clear();
	redoStack.clear();
	origin = XmlOrigin::New;
	MarkClean();
}

std::unique_ptr<XMLDocument> XmlDocument::ParseSystem(const std::string& xml, std::string& outError) const {
	auto parsed = std::make_unique<XMLDocument>();
	if (parsed->Parse(xml.c_str(), xml.size()) != XML_SUCCESS) {
		outError = parsed->ErrorStr() ? parsed->ErrorStr() : "could not parse the file";
		return nullptr;
	}

	const XMLElement* root = parsed->RootElement();
	if (!root || !root->Name() || std::strcmp(root->Name(), "system") != 0) {
		outError = "the root element is not <system>, so the simulation would refuse the file";
		return nullptr;
	}

	return parsed;
}

bool XmlDocument::LoadFromString(const std::string& xml, std::string& outError) {
	auto parsed = ParseSystem(xml, outError);
	if (!parsed)
		return false;

	doc = std::move(parsed);
	undoStack.clear();
	redoStack.clear();
	MarkClean();
	return true;
}

bool XmlDocument::ReplaceContent(const std::string& xml, std::string& outError) {
	auto parsed = ParseSystem(xml, outError);
	if (!parsed)
		return false;

	doc = std::move(parsed);

	// Deliberately not MarkClean: the document is still bound to the file it
	// was bound to, and importing something else over it is precisely a
	// change that file does not have yet.
	dirty = Serialize() != cleanText;
	return true;
}

bool XmlDocument::LoadFromStream(std::istream& stream, std::string& outError) {
	std::ostringstream contents;
	contents << stream.rdbuf();
	return LoadFromString(contents.str(), outError);
}

std::string XmlDocument::Serialize() const {
	XMLPrinter printer;
	doc->Print(&printer);
	return printer.CStr() ? std::string(printer.CStr()) : std::string();
}

bool XmlDocument::SaveToFile(const std::string& path, std::string& outError) const {
	if (doc->SaveFile(path.c_str()) != XML_SUCCESS) {
		outError = doc->ErrorStr() ? doc->ErrorStr() : "could not write the file";
		return false;
	}
	return true;
}

XMLElement* XmlDocument::Root() const {
	return doc->RootElement();
}

XMLElement* XmlDocument::Resolve(const NodePath& path) const {
	XMLElement* element = doc->RootElement();
	for (std::uint16_t step : path.index) {
		if (!element)
			return nullptr;

		XMLElement* child = element->FirstChildElement();
		for (std::uint16_t i = 0; i < step && child; i++)
			child = child->NextSiblingElement();

		element = child;
	}
	return element;
}

NodePath XmlDocument::PathOf(const XMLElement* element) const {
	NodePath path;
	const XMLElement* root = doc->RootElement();

	// Indices count element children only, so comments and stray text between
	// siblings cannot shift a path.
	for (const XMLElement* node = element; node && node != root;) {
		const XMLNode* parentNode = node->Parent();
		const XMLElement* parent = parentNode ? parentNode->ToElement() : nullptr;
		if (!parent)
			return NodePath();

		std::uint16_t position = 0;
		for (const XMLElement* sibling = parent->FirstChildElement(); sibling; sibling = sibling->NextSiblingElement()) {
			if (sibling == node)
				break;
			position++;
		}

		path.index.insert(path.index.begin(), position);
		node = parent;
	}

	return path;
}

ValueVariant DefaultValue(const ChildDesc& desc) {
	if (!desc.defaultText)
		return std::monostate();

	// A list defaults to being empty; there is no one value to fall back to.
	if (desc.mult == Multiplicity::List)
		return std::monostate();

	const std::string text = desc.defaultText;

	// Whitespace separated, the way the schema table spells vector defaults
	// ("1 1 1"). Missing components stay at zero.
	auto components = [&text](std::size_t count) {
		std::vector<float> values(count, 0.0f);
		std::istringstream parts(text);
		std::string part;
		for (std::size_t i = 0; i < count && parts >> part; ++i)
			XmlValue::ParseFloat(part, values[i]);

		return values;
	};

	switch (desc.kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: {
			float value = 0.0f;
			if (!XmlValue::ParseFloat(text, value))
				return std::monostate();
			return value;
		}

		case ValueKind::Int: {
			int value = 0;
			if (!XmlValue::ParseInt(text, value))
				return std::monostate();
			return value;
		}

		case ValueKind::Bool: {
			bool value = false;
			if (!XmlValue::ParseBool(text, value))
				return std::monostate();
			return value;
		}

		case ValueKind::String:
		case ValueKind::Enum:
		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: return text;

		case ValueKind::Vec3: {
			const std::vector<float> values = components(3);
			return nifly::Vector3(values[0], values[1], values[2]);
		}

		case ValueKind::Quaternion: {
			// Same order the attributes are written in: x, y, z, w.
			const std::vector<float> values = components(4);
			return nifly::Quaternion(values[3], values[0], values[1], values[2]);
		}

		case ValueKind::AxisAngle: {
			const std::vector<float> values = components(4);
			AxisAngleValue value;
			value.axis = nifly::Vector3(values[0], values[1], values[2]);
			value.angle = values[3];
			return value;
		}

		case ValueKind::Lerp: {
			const std::vector<float> values = components(2);
			LerpValue value;
			value.translation = values[0];
			value.rotation = values[1];
			return value;
		}

		// Transform and None carry structure rather than a value, and a list
		// defaults to being empty rather than to holding something.
		case ValueKind::Transform:
		case ValueKind::None: break;
	}

	return std::monostate();
}

std::vector<XMLElement*> XmlDocument::ChildrenOfKinds(std::initializer_list<ElementKind> kinds, const XMLElement* parent) const {
	std::vector<XMLElement*> result;

	const XMLElement* owner = parent ? parent : doc->RootElement();
	if (!owner)
		return result;

	for (const XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (!child->Name())
			continue;

		const ElementKind kind = KindFromName(child->Name());
		if (std::find(kinds.begin(), kinds.end(), kind) != kinds.end())
			result.push_back(const_cast<XMLElement*>(child));
	}

	return result;
}

std::vector<XMLElement*> XmlDocument::ChildrenOfKind(ElementKind kind, const XMLElement* parent) const {
	return ChildrenOfKinds({kind}, parent);
}

std::string XmlDocument::DisplayName(const XMLElement* element) {
	if (!element)
		return std::string();

	const char* name = element->Attribute("name");
	if (name && *name)
		return name;

	// Constraints are identified by the pair of bodies they join; the XSD puts
	// no key on that pair, so this is a label, not an address.
	const char* bodyA = element->Attribute("bodyA");
	const char* bodyB = element->Attribute("bodyB");
	if ((bodyA && *bodyA) || (bodyB && *bodyB))
		return std::string(bodyA ? bodyA : "?") + " - " + (bodyB ? bodyB : "?");

	return element->Name() ? element->Name() : std::string();
}

std::string XmlFileName(const std::string& xmlPath) {
	const std::size_t slash = xmlPath.find_last_of("/\\");
	return slash == std::string::npos ? xmlPath : xmlPath.substr(slash + 1);
}

XMLElement* XmlDocument::FindChild(const XMLElement* owner, const ChildDesc& desc) const {
	if (!owner)
		return nullptr;

	for (const XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (NameMatches(child, desc))
			return const_cast<XMLElement*>(child);
	}
	return nullptr;
}

bool XmlDocument::HasValue(const XMLElement* owner, const ChildDesc& desc) const {
	return FindChild(owner, desc) != nullptr;
}

ValueVariant XmlDocument::GetValue(const XMLElement* owner, const ChildDesc& desc) const {
	if (!owner)
		return std::monostate();

	// Text valued lists come back whole; every other repeated child is
	// structure the editor walks as its own nodes.
	if (desc.mult == Multiplicity::List) {
		switch (desc.kind) {
			case ValueKind::String:
			case ValueKind::BoneRef:
			case ValueKind::NiShapeRef:
			case ValueKind::ShapeRef:
			case ValueKind::TagRef: break;
			default: return std::monostate();
		}

		std::vector<std::string> values;
		for (const XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (NameMatches(child, desc))
				values.push_back(hdt::TrimAsciiWhitespace(ElementText(child)));
		}

		if (values.empty())
			return std::monostate();
		return values;
	}

	const XMLElement* child = FindChild(owner, desc);
	if (!child)
		return std::monostate();

	switch (desc.kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: {
			float value = 0.0f;
			if (!XmlValue::ParseFloat(ElementText(child), value))
				return std::monostate();
			return value;
		}
		case ValueKind::Int: {
			int value = 0;
			if (!XmlValue::ParseInt(ElementText(child), value))
				return std::monostate();
			return value;
		}
		case ValueKind::Bool: {
			bool value = false;
			if (!XmlValue::ParseBool(ElementText(child), value))
				return std::monostate();
			return value;
		}
		case ValueKind::String:
		case ValueKind::Enum:
		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: return hdt::TrimAsciiWhitespace(ElementText(child));

		case ValueKind::Vec3:
			return nifly::Vector3(AttrFloat(child, "x"), AttrFloat(child, "y"), AttrFloat(child, "z"));

		case ValueKind::Quaternion:
			return nifly::Quaternion(AttrFloat(child, "w"), AttrFloat(child, "x"), AttrFloat(child, "y"), AttrFloat(child, "z"));

		case ValueKind::AxisAngle: {
			AxisAngleValue value;
			value.axis = nifly::Vector3(AttrFloat(child, "x"), AttrFloat(child, "y"), AttrFloat(child, "z"));
			value.angle = AttrFloat(child, "angle");
			return value;
		}

		case ValueKind::Lerp: {
			LerpValue value;
			for (const XMLElement* part = child->FirstChildElement(); part; part = part->NextSiblingElement()) {
				if (!part->Name())
					continue;
				if (std::strcmp(part->Name(), "translationLerp") == 0)
					XmlValue::ParseFloat(ElementText(part), value.translation);
				else if (std::strcmp(part->Name(), "rotationLerp") == 0)
					XmlValue::ParseFloat(ElementText(part), value.rotation);
			}
			return value;
		}

		// Transform keeps whichever of basis / basis-axis-angle / origin the
		// author wrote, so it is edited through those children rather than
		// flattened into a matrix and back.
		case ValueKind::Transform:
		case ValueKind::None: return std::monostate();
	}

	return std::monostate();
}

ValueVariant XmlDocument::GetAttr(const XMLElement* owner, const AttrDesc& desc) const {
	const char* raw = owner ? owner->Attribute(desc.name) : nullptr;
	if (!raw)
		return std::monostate();

	switch (desc.kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: {
			float value = 0.0f;
			if (!XmlValue::ParseFloat(raw, value))
				return std::monostate();
			return value;
		}
		case ValueKind::Int: {
			int value = 0;
			if (!XmlValue::ParseInt(raw, value))
				return std::monostate();
			return value;
		}
		case ValueKind::Bool: {
			bool value = false;
			if (!XmlValue::ParseBool(raw, value))
				return std::monostate();
			return value;
		}
		default: return std::string(raw);
	}
}

XMLElement* XmlDocument::EnsureChild(XMLElement* owner, const ChildDesc& desc) {
	if (XMLElement* existing = FindChild(owner, desc))
		return existing;

	const ElementDesc* ownerDesc = FindElement(KindFromName(owner->Name()));
	XMLElement* created = doc->NewElement(desc.name);

	if (!ownerDesc) {
		owner->InsertEndChild(created);
		return created;
	}

	// Put it where the schema lists it, so a file the editor has touched still
	// reads in a predictable order. Nothing already present ever moves.
	const int order = DescriptorOrder(*ownerDesc, desc);
	XMLElement* before = nullptr;
	for (XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
		const ChildDesc* childDesc = ownerDesc->Child(child->Name());
		if (childDesc && DescriptorOrder(*ownerDesc, *childDesc) > order) {
			before = child;
			break;
		}
	}

	if (!before) {
		owner->InsertEndChild(created);
		return created;
	}

	XMLNode* previous = before->PreviousSiblingElement();
	if (previous)
		owner->InsertAfterChild(previous, created);
	else
		owner->InsertFirstChild(created);

	return created;
}

bool XmlDocument::SetValue(XMLElement* owner, const ChildDesc& desc, const ValueVariant& value) {
	if (!owner)
		return false;

	if (std::holds_alternative<std::monostate>(value)) {
		ClearValue(owner, desc);
		return true;
	}

	// Nothing to do, and nothing to dirty, when the file already says this.
	if (ValuesEqual(GetValue(owner, desc), value))
		return true;

	if (desc.mult == Multiplicity::List) {
		const auto* values = std::get_if<std::vector<std::string>>(&value);
		if (!values)
			return false;

		// Rewrite the run in place: reuse the elements that are already there
		// so the first entry keeps its position among its siblings.
		std::vector<XMLElement*> existing;
		for (XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (NameMatches(child, desc))
				existing.push_back(child);
		}

		for (std::size_t i = 0; i < values->size(); i++) {
			if (i < existing.size())
				existing[i]->SetText((*values)[i].c_str());
			else {
				XMLElement* created = doc->NewElement(desc.name);
				created->SetText((*values)[i].c_str());
				if (!existing.empty())
					owner->InsertAfterChild(existing.back(), created);
				else
					owner->InsertEndChild(created);
				existing.push_back(created);
			}
		}

		for (std::size_t i = values->size(); i < existing.size(); i++)
			owner->DeleteChild(existing[i]);

		dirty = true;
		return true;
	}

	XMLElement* child = EnsureChild(owner, desc);

	switch (desc.kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: {
			const auto* number = std::get_if<float>(&value);
			if (!number)
				return false;
			child->SetText(XmlValue::FormatFloat(*number).c_str());
			break;
		}
		case ValueKind::Int: {
			const auto* number = std::get_if<int>(&value);
			if (!number)
				return false;
			child->SetText(XmlValue::FormatInt(*number).c_str());
			break;
		}
		case ValueKind::Bool: {
			const auto* flag = std::get_if<bool>(&value);
			if (!flag)
				return false;
			child->SetText(XmlValue::FormatBool(*flag).c_str());
			break;
		}
		case ValueKind::String:
		case ValueKind::Enum:
		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: {
			const auto* text = std::get_if<std::string>(&value);
			if (!text)
				return false;
			child->SetText(text->c_str());
			break;
		}
		case ValueKind::Vec3: {
			const auto* vector = std::get_if<nifly::Vector3>(&value);
			if (!vector)
				return false;
			SetAttrFloat(child, "x", vector->x);
			SetAttrFloat(child, "y", vector->y);
			SetAttrFloat(child, "z", vector->z);
			break;
		}
		case ValueKind::Quaternion: {
			const auto* quaternion = std::get_if<nifly::Quaternion>(&value);
			if (!quaternion)
				return false;
			SetAttrFloat(child, "x", quaternion->x);
			SetAttrFloat(child, "y", quaternion->y);
			SetAttrFloat(child, "z", quaternion->z);
			SetAttrFloat(child, "w", quaternion->w);
			break;
		}
		case ValueKind::AxisAngle: {
			const auto* axisAngle = std::get_if<AxisAngleValue>(&value);
			if (!axisAngle)
				return false;
			SetAttrFloat(child, "x", axisAngle->axis.x);
			SetAttrFloat(child, "y", axisAngle->axis.y);
			SetAttrFloat(child, "z", axisAngle->axis.z);
			SetAttrFloat(child, "angle", axisAngle->angle);
			break;
		}
		case ValueKind::Lerp: {
			const auto* lerp = std::get_if<LerpValue>(&value);
			if (!lerp)
				return false;

			XMLElement* translation = child->FirstChildElement("translationLerp");
			if (!translation) {
				translation = doc->NewElement("translationLerp");
				child->InsertEndChild(translation);
			}
			translation->SetText(XmlValue::FormatFloat(lerp->translation).c_str());

			XMLElement* rotation = child->FirstChildElement("rotationLerp");
			if (!rotation) {
				rotation = doc->NewElement("rotationLerp");
				child->InsertEndChild(rotation);
			}
			rotation->SetText(XmlValue::FormatFloat(lerp->rotation).c_str());
			break;
		}
		case ValueKind::Transform:
		case ValueKind::None: return false;
	}

	dirty = true;
	return true;
}

bool XmlDocument::SetAttr(XMLElement* owner, const AttrDesc& desc, const ValueVariant& value) {
	if (!owner)
		return false;

	if (std::holds_alternative<std::monostate>(value)) {
		if (!owner->Attribute(desc.name))
			return true;
		owner->DeleteAttribute(desc.name);
		dirty = true;
		return true;
	}

	if (ValuesEqual(GetAttr(owner, desc), value))
		return true;

	if (const auto* text = std::get_if<std::string>(&value))
		owner->SetAttribute(desc.name, text->c_str());
	else if (const auto* number = std::get_if<float>(&value))
		owner->SetAttribute(desc.name, XmlValue::FormatFloat(*number).c_str());
	else if (const auto* integer = std::get_if<int>(&value))
		owner->SetAttribute(desc.name, XmlValue::FormatInt(*integer).c_str());
	else if (const auto* flag = std::get_if<bool>(&value))
		owner->SetAttribute(desc.name, XmlValue::FormatBool(*flag).c_str());
	else
		return false;

	dirty = true;
	return true;
}

void XmlDocument::ClearValue(XMLElement* owner, const ChildDesc& desc) {
	if (!owner)
		return;

	std::vector<XMLElement*> doomed;
	for (XMLElement* child = owner->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (NameMatches(child, desc))
			doomed.push_back(child);
	}

	for (XMLElement* child : doomed)
		owner->DeleteChild(child);

	if (!doomed.empty())
		dirty = true;
}

XMLElement* XmlDocument::InsertElement(ElementKind kind, XMLElement* parent, XMLElement* after) {
	const ElementDesc* desc = FindElement(kind);
	if (!desc || !parent)
		return nullptr;

	XMLElement* created = doc->NewElement(desc->name);
	for (std::size_t i = 0; i < desc->attrCount; i++) {
		if (desc->attrs[i].required)
			created->SetAttribute(desc->attrs[i].name, "");
	}

	if (after)
		parent->InsertAfterChild(after, created);
	else
		parent->InsertEndChild(created);

	dirty = true;
	return created;
}

void XmlDocument::RemoveElement(XMLElement* element) {
	if (!element)
		return;

	XMLNode* parent = element->Parent();
	if (!parent)
		return;

	parent->DeleteChild(element);
	dirty = true;
}

void XmlDocument::PushSnapshot(std::deque<Snapshot>& stack, const std::string& label, const NodePath& key, const void* desc) {
	Snapshot snapshot;
	snapshot.text = Serialize();
	snapshot.label = label;
	snapshot.coalesceKey = key;
	snapshot.coalesceDesc = desc;
	snapshot.stamp = NowMs();

	stack.push_back(std::move(snapshot));
	while (stack.size() > maxUndoDepth)
		stack.pop_front();
}

void XmlDocument::BeginEdit(const std::string& label, const NodePath& coalesceKey, const void* coalesceDesc) {
	// Dragging a spin control fires a change per pixel; folding those into the
	// step that started them keeps undo usable.
	if (coalesceDesc && !undoStack.empty()) {
		const Snapshot& last = undoStack.back();
		if (last.coalesceDesc == coalesceDesc && last.coalesceKey == coalesceKey && NowMs() - last.stamp < kCoalesceWindowMs) {
			redoStack.clear();
			return;
		}
	}

	PushSnapshot(undoStack, label, coalesceKey, coalesceDesc);
	redoStack.clear();
}

bool XmlDocument::RestoreFrom(std::deque<Snapshot>& from, std::deque<Snapshot>& to) {
	if (from.empty())
		return false;

	Snapshot snapshot = std::move(from.back());
	from.pop_back();

	// Record where we are now, so the move can be taken back again.
	PushSnapshot(to, snapshot.label, snapshot.coalesceKey, nullptr);

	auto parsed = std::make_unique<XMLDocument>();
	if (parsed->Parse(snapshot.text.c_str(), snapshot.text.size()) != XML_SUCCESS) {
		to.pop_back();
		return false;
	}

	doc = std::move(parsed);

	// Undo can walk a document back to exactly the text it was loaded from,
	// and when it does there is nothing left to save.
	dirty = Serialize() != cleanText;
	return true;
}

void XmlDocument::MarkClean() {
	cleanText = Serialize();
	dirty = false;
}

bool XmlDocument::Undo() {
	return RestoreFrom(undoStack, redoStack);
}

bool XmlDocument::Redo() {
	return RestoreFrom(redoStack, undoStack);
}

const std::string& XmlDocument::UndoLabel() const {
	static const std::string none;
	return undoStack.empty() ? none : undoStack.back().label;
}

const std::string& XmlDocument::RedoLabel() const {
	static const std::string none;
	return redoStack.empty() ? none : redoStack.back().label;
}


namespace {
// Tag and bone names are IDStr at runtime, which compares without regard to
// case; so must every list membership test the editor makes about them.
bool IEquals(const std::string& a, const std::string& b) {
	if (a.size() != b.size())
		return false;

	for (std::size_t i = 0; i < a.size(); i++) {
		if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			return false;
	}
	return true;
}

bool Contains(const std::vector<std::string>& list, const std::string& value) {
	return std::any_of(list.begin(), list.end(), [&value](const std::string& item) { return IEquals(item, value); });
}

std::string AttrText(const XMLElement* element, const char* name) {
	const char* raw = element ? element->Attribute(name) : nullptr;
	return raw ? std::string(raw) : std::string();
}

bool ListedInEnum(const EnumValues& values, const std::string& text) {
	for (std::size_t i = 0; i < values.count; i++) {
		if (values.values[i] && text == values.values[i])
			return true;
	}
	return false;
}

// "0 .. 1", or one sided when only one end of the range is documented.
std::string RangeText(const ChildDesc& desc) {
	const bool low = desc.rangeMin != -kUnbounded;
	const bool high = desc.rangeMax != kUnbounded;

	if (low && high)
		return XmlValue::FormatFloat(desc.rangeMin) + " .. " + XmlValue::FormatFloat(desc.rangeMax);
	if (low)
		return XmlValue::FormatFloat(desc.rangeMin) + " or more";
	if (high)
		return XmlValue::FormatFloat(desc.rangeMax) + " or less";

	return std::string();
}
}

EffectiveValue XmlDocument::ResolveEffective(const XMLElement* element, const ChildDesc& desc) const {
	EffectiveValue result;

	if (!element) {
		result.value = DefaultValue(desc);
		return result;
	}

	if (HasValue(element, desc)) {
		result.value = GetValue(element, desc);
		result.source = element;
		result.explicitHere = true;
		return result;
	}

	const ElementKind kind = KindFromName(element->Name());
	const ElementKind defaultKind = DefaultKindOf(kind);
	if (defaultKind == ElementKind::Unknown || !CascadeApplies(kind)) {
		result.value = DefaultValue(desc);
		return result;
	}

	// Every *-default of the matching kind the parser would have read before it
	// reached this element, in reading order. Constraint groups are descended
	// into: a default declared inside one is not scoped to it, because the
	// builder puts it in the same template map as all the others.
	std::vector<const XMLElement*> preceding;
	bool reached = false;
	std::function<void(const XMLElement*)> walk = [&](const XMLElement* parent) {
		for (const XMLElement* child = parent->FirstChildElement(); child && !reached; child = child->NextSiblingElement()) {
			if (child == element) {
				reached = true;
				return;
			}

			const ElementKind childKind = KindFromName(child->Name());
			if (childKind == defaultKind)
				preceding.push_back(child);
			else if (childKind == ElementKind::ConstraintGroup)
				walk(child);
		}
	};

	if (const XMLElement* root = doc->RootElement())
		walk(root);

	// A template name resolves to the chain of defaults that built it, as it
	// stood when it was declared: @extends composes, the ambient set lives
	// under the empty name, and a name that was never declared falls back to
	// the ambient set rather than to nothing, exactly as getBoneTemplate does.
	std::map<std::string, std::vector<const XMLElement*>> chains;
	for (const XMLElement* declared : preceding) {
		const auto base = chains.find(AttrText(declared, "extends"));
		std::vector<const XMLElement*> chain = base != chains.end() ? base->second : chains[std::string()];
		chain.push_back(declared);
		chains[AttrText(declared, "name")] = std::move(chain);
	}

	const std::string selected = AttrText(element, IsDefaultKind(kind) ? "extends" : "template");
	const auto found = chains.find(selected);
	const std::vector<const XMLElement*>& chain = found != chains.end() ? found->second : chains[std::string()];

	// Last one wins: each default in the chain was applied on top of the ones
	// before it.
	for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
		if (!HasValue(*it, desc))
			continue;

		result.value = GetValue(*it, desc);
		result.source = *it;

		// The common case is an unnamed default, which can only be pointed at
		// by where it sits among the others of its kind.
		const std::string name = AttrText(*it, "name");
		const std::string tag = (*it)->Name() ? (*it)->Name() : "";
		if (!name.empty())
			result.sourceLabel = tag + " \"" + name + "\"";
		else {
			const auto position = std::find(preceding.begin(), preceding.end(), *it);
			result.sourceLabel = tag + " #" + std::to_string(std::distance(preceding.begin(), position) + 1);
		}

		return result;
	}

	result.value = DefaultValue(desc);
	return result;
}

std::vector<std::string> XmlDocument::CollectTags() const {
	std::vector<std::string> tags;

	for (const XMLElement* shape : ChildrenOfKinds({ElementKind::PerVertexShape,
												   ElementKind::PerTriangleShape,
												   ElementKind::PerVertexShapeDefault,
												   ElementKind::PerTriangleShapeDefault})) {
		for (const XMLElement* child = shape->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (!child->Name() || std::strcmp(child->Name(), "tag") != 0)
				continue;

			const std::string tag = hdt::TrimAsciiWhitespace(ElementText(child));
			if (!tag.empty() && !Contains(tags, tag))
				tags.push_back(tag);
		}
	}

	return tags;
}

std::vector<ShapeFacts> CollectShapeFacts(const XmlDocument& doc) {
	std::vector<ShapeFacts> facts;

	auto readList = [&doc](const XMLElement* shape, const ElementDesc& desc, const char* name) {
		std::vector<std::string> values;

		const ChildDesc* child = desc.Child(name);
		if (!child)
			return values;

		const ValueVariant value = doc.GetValue(shape, *child);
		if (const auto* list = std::get_if<std::vector<std::string>>(&value))
			values = *list;

		return values;
	};

	for (const XMLElement* shape : doc.ChildrenOfKinds({ElementKind::PerVertexShape, ElementKind::PerTriangleShape})) {
		const ElementDesc* desc = FindElement(shape->Name());
		if (!desc)
			continue;

		ShapeFacts entry;
		entry.xmlPath = doc.XmlPath();
		entry.name = AttrText(shape, "name");
		entry.kind = KindFromName(shape->Name());
		entry.node = doc.PathOf(shape);
		entry.tags = readList(shape, *desc, "tag");
		entry.canCollideWithTags = readList(shape, *desc, "can-collide-with-tag");
		entry.noCollideWithTags = readList(shape, *desc, "no-collide-with-tag");

		if (const ChildDesc* shared = desc->Child("shared")) {
			const EffectiveValue effective = doc.ResolveEffective(shape, *shared);
			const auto* text = std::get_if<std::string>(&effective.value);
			if (text && !text->empty())
				entry.shared = *text;
		}

		facts.push_back(std::move(entry));
	}

	return facts;
}


std::vector<Diagnostic> XmlDocument::Validate(const ValidationContext& context) const {
	std::vector<Diagnostic> diagnostics;

	const XMLElement* root = doc->RootElement();
	if (!root)
		return diagnostics;

	auto report = [this, &diagnostics](Diagnostic::Severity severity, const XMLElement* element, std::string message) {
		Diagnostic diagnostic;
		diagnostic.severity = severity;
		diagnostic.node = PathOf(element);
		diagnostic.message = std::move(message);

		const std::string tag = element && element->Name() ? element->Name() : "";
		const std::string display = DisplayName(element);
		diagnostic.elementLabel = display == tag ? tag : tag + " \"" + display + "\"";

		diagnostics.push_back(std::move(diagnostic));
	};

	// Names already seen, so the second one of each is the one reported. The
	// two skinned shape kinds share a namespace: both end up as bodies keyed by
	// the NiShape they are built from.
	std::vector<std::string> boneNames;
	std::vector<std::string> shapeNames;
	std::map<ElementKind, std::vector<std::string>> templateNames;

	// Document order is the parser's order, so walking it front to back is also
	// what decides whether a template was declared before it was used.
	const ElementDesc* rootDesc = FindElement(root->Name());

	std::function<void(const XMLElement*, const ElementDesc*)> visit = [&](const XMLElement* element, const ElementDesc* ownerDesc) {
		const ElementKind kind = KindFromName(element->Name());
		const ElementDesc* desc = FindElement(element->Name());
		const XMLNode* parentNode = element->Parent();
		const XMLElement* parent = parentNode ? parentNode->ToElement() : nullptr;

		if (!desc) {
			// A value element carries no description of its own, and the element
			// holding it has already vouched for it. Only something nothing at
			// all accounts for is worth reporting - and there is nothing inside
			// a value to walk into either.
			if (!ownerDesc || !ownerDesc->Child(element->Name())) {
				report(Diagnostic::Severity::Info,
					   element,
					   "not part of the hdtSMP64 schema. It is kept in the file, but this preview warns about it and skips it.");
			}

			return;
		}

		{
			for (std::size_t i = 0; i < desc->attrCount; i++) {
				const AttrDesc& attr = desc->attrs[i];
				const char* raw = element->Attribute(attr.name);

				if (!raw) {
					if (attr.required)
						report(Diagnostic::Severity::Error, element, std::string("the required attribute \"") + attr.name + "\" is missing");
					continue;
				}

				if (attr.kind == ValueKind::Enum && attr.enumValues && !ListedInEnum(*attr.enumValues, raw))
					report(Diagnostic::Severity::Warning, element, std::string("\"") + raw + "\" is not one of the values the schema lists for " + attr.name);
			}

			for (std::size_t i = 0; i < desc->childCount; i++) {
				const ChildDesc& child = desc->children[i];
				if (child.mult == Multiplicity::List || !HasValue(element, child))
					continue;

				const ValueVariant value = GetValue(element, child);

				switch (child.kind) {
					case ValueKind::Float:
					case ValueKind::Factor:
					case ValueKind::PosFloat: {
						const float* number = std::get_if<float>(&value);
						if (!number) {
							report(Diagnostic::Severity::Error, element, std::string("<") + child.name + "> does not read as a number");
							break;
						}

						if (*number < child.rangeMin || *number > child.rangeMax) {
							report(Diagnostic::Severity::Warning,
								   element,
								   std::string("<") + child.name + "> is " + XmlValue::FormatFloat(*number) + ", outside the documented range " + RangeText(child));
						}
						break;
					}

					case ValueKind::Int:
						if (!std::get_if<int>(&value))
							report(Diagnostic::Severity::Error, element, std::string("<") + child.name + "> does not read as a whole number");
						break;

					case ValueKind::Bool:
						if (!std::get_if<bool>(&value))
							report(Diagnostic::Severity::Error, element, std::string("<") + child.name + "> is neither true nor false");
						break;

					case ValueKind::Enum: {
						const std::string* text = std::get_if<std::string>(&value);
						if (text && child.enumValues && !ListedInEnum(*child.enumValues, *text)) {
							report(Diagnostic::Severity::Warning,
								   element,
								   std::string("<") + child.name + "> is \"" + *text + "\", which the schema does not list; the preview falls back to its default");
						}
						break;
					}

					default: break;
				}
			}
		}

		// A template that names nothing declared before it does not fail: the
		// builder hands out the ambient default instead, silently, which is
		// exactly the kind of thing an author never notices.
		if (desc && CascadeApplies(kind)) {
			const bool isTemplate = IsDefaultKind(kind);
			const char* attrName = isTemplate ? "extends" : "template";
			const std::string selected = AttrText(element, attrName);
			const std::vector<std::string>& declared = templateNames[DefaultKindOf(kind)];

			if (!selected.empty() && !Contains(declared, selected)) {
				report(Diagnostic::Severity::Warning,
					   element,
					   std::string(attrName) + "=\"" + selected + "\" names no " + NameFromKind(DefaultKindOf(kind))
						   + " declared before this point, so the ambient defaults are used instead");
			}

			if (isTemplate) {
				const std::string name = AttrText(element, "name");
				if (!name.empty()) {
					std::vector<std::string>& known = templateNames[kind];
					if (Contains(known, name)) {
						report(Diagnostic::Severity::Warning,
							   element,
							   "a template named \"" + name + "\" was already declared; this one replaces it for everything that follows");
					}
					known.push_back(name);
				}
			}
		}

		switch (kind) {
			case ElementKind::Bone: {
				const std::string name = AttrText(element, "name");
				if (name.empty())
					break;

				if (Contains(boneNames, name))
					report(Diagnostic::Severity::Error, element, "a <bone> named \"" + name + "\" is already declared; the name has to be unique");
				else
					boneNames.push_back(name);

				if (!context.skeletonBoneNames.empty() && !Contains(context.skeletonBoneNames, name))
					report(Diagnostic::Severity::Warning, element, "\"" + name + "\" is not a node of the loaded skeleton");
				break;
			}

			case ElementKind::PerVertexShape:
			case ElementKind::PerTriangleShape: {
				const std::string name = AttrText(element, "name");
				if (!name.empty()) {
					if (Contains(shapeNames, name))
						report(Diagnostic::Severity::Error, element, "a skinned shape for \"" + name + "\" is already declared; the name has to be unique");
					else
						shapeNames.push_back(name);

					if (!context.nifShapeNames.empty() && !Contains(context.nifShapeNames, name))
						report(Diagnostic::Severity::Warning, element, "no shape named \"" + name + "\" is loaded, so nothing is built from it");
				}

				const ElementDesc* shapeDesc = FindElement(element->Name());
				const ChildDesc* whitelist = shapeDesc ? shapeDesc->Child("can-collide-with-tag") : nullptr;
				const ChildDesc* blacklist = shapeDesc ? shapeDesc->Child("no-collide-with-tag") : nullptr;
				if (whitelist && blacklist && HasValue(element, *whitelist) && HasValue(element, *blacklist)) {
					report(Diagnostic::Severity::Warning,
						   element,
						   "can-collide-with-tag and no-collide-with-tag are both set; a whitelist decides on its own, so the blacklist does nothing");
				}
				break;
			}

			case ElementKind::GenericConstraint:
			case ElementKind::StiffSpringConstraint:
			case ElementKind::ConeTwistConstraint: {
				for (const char* side : {"bodyA", "bodyB"}) {
					const std::string body = AttrText(element, side);
					if (body.empty() || context.skeletonBoneNames.empty() || Contains(context.skeletonBoneNames, body))
						continue;

					report(Diagnostic::Severity::Warning,
						   element,
						   std::string(side) + "=\"" + body + "\" is not a node of the loaded skeleton, so the constraint is dropped");
				}
				break;
			}

			case ElementKind::Shape:
				// The reader takes a top level shape's name without a fallback,
				// and the exception it throws is caught around the whole file:
				// one nameless <shape> and nothing at all is simulated.
				if (parent == root && AttrText(element, "name").empty()) {
					report(Diagnostic::Severity::Error,
						   element,
						   "a top level <shape> without a name aborts the whole file in this preview, and nothing in it is simulated");
				}
				break;

			default: break;
		}

		for (const XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
			visit(child, desc);
	};

	for (const XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement())
		visit(child, rootDesc);

	return diagnostics;
}

CollisionResult EvaluateCollision(const ShapeFacts& a, const ShapeFacts& b) {
	CollisionResult result;

	// PreviewBody::canCollideWith, which runs before the tag lists are looked
	// at. The preview is one actor on one skeleton, so "internal" behaves
	// exactly like "public" here and "external" can never match anything.
	auto sharingAllows = [](const ShapeFacts& one, const ShapeFacts& other, std::string& why) {
		if (IEquals(one.shared, "external")) {
			why = one.name + " is shared=\"external\", which never collides with anything in a single actor preview";
			return false;
		}

		if (IEquals(one.shared, "private") && one.xmlPath != other.xmlPath) {
			why = one.name + " is shared=\"private\", so it only meets shapes from its own file";
			return false;
		}

		return true;
	};

	std::string sharingWhy;
	if (!sharingAllows(a, b, sharingWhy) || !sharingAllows(b, a, sharingWhy)) {
		result.reason = sharingWhy;
		return result;
	}

	// SkinnedMeshBody::canCollideWith: a non-empty whitelist decides on its own
	// and the blacklist stops being consulted at all.
	auto tagsAllow = [](const ShapeFacts& one, const ShapeFacts& other, std::string& why) {
		if (!one.canCollideWithTags.empty()) {
			for (const std::string& tag : other.tags) {
				if (Contains(one.canCollideWithTags, tag)) {
					why = one.name + " whitelists \"" + tag + "\"";
					return true;
				}
			}

			why = one.name + " has a whitelist and " + other.name + " carries none of the tags in it";
			return false;
		}

		for (const std::string& tag : other.tags) {
			if (Contains(one.noCollideWithTags, tag)) {
				why = one.name + " blacklists \"" + tag + "\"";
				return false;
			}
		}

		why.clear();
		return true;
	};

	// Both directions, because the dispatcher asks both bodies and needs two
	// yeses. A shape can therefore be turned away by a list it does not own.
	std::string whyA;
	std::string whyB;
	if (!tagsAllow(a, b, whyA)) {
		result.reason = whyA;
		return result;
	}
	if (!tagsAllow(b, a, whyB)) {
		result.reason = whyB;
		return result;
	}

	result.collides = true;
	if (!whyA.empty() && !whyB.empty())
		result.reason = whyA + "; " + whyB;
	else if (!whyA.empty())
		result.reason = whyA;
	else if (!whyB.empty())
		result.reason = whyB;
	else
		result.reason = "no list on either shape turns the other away";

	return result;
}

void XmlDocument::SetSource(const std::string& inXmlPath, const std::string& inSourcePath, XmlOrigin inOrigin) {
	xmlPath = inXmlPath;
	sourcePath = inSourcePath;
	origin = inOrigin;
}
}
