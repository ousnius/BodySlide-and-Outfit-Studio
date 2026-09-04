#include "../src/physics/XmlDocument.h"
#include "../src/physics/XmlSchema.h"

#include <catch2/catch_test_macros.hpp>

#include <tinyxml2.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace Physics;

namespace {
#ifndef BSOS_TEST_DATA_DIR
#define BSOS_TEST_DATA_DIR "tests/data"
#endif

std::string DataPath(const std::string& name) {
	return std::string(BSOS_TEST_DATA_DIR) + "/physics/" + name;
}

std::string ReadFile(const std::string& path) {
	std::ifstream stream(path, std::ios::binary);
	REQUIRE(stream.good());
	std::ostringstream contents;
	contents << stream.rdbuf();
	return contents.str();
}

XmlDocument LoadFixture(const std::string& name) {
	XmlDocument doc;
	std::string error;
	const bool loaded = doc.LoadFromString(ReadFile(DataPath(name)), error);
	INFO(error);
	REQUIRE(loaded);
	return doc;
}

// Structural comparison: names, attributes in order, text and comments. Used
// instead of comparing bytes because tinyxml2's printer reflows whitespace, so
// a saved file is semantically but not textually identical to its input.
void RequireSameTree(const tinyxml2::XMLNode* a, const tinyxml2::XMLNode* b) {
	REQUIRE((a == nullptr) == (b == nullptr));
	if (!a)
		return;

	const tinyxml2::XMLElement* elementA = a->ToElement();
	const tinyxml2::XMLElement* elementB = b->ToElement();
	REQUIRE((elementA == nullptr) == (elementB == nullptr));

	if (elementA) {
		REQUIRE(std::string(elementA->Name()) == std::string(elementB->Name()));

		const tinyxml2::XMLAttribute* attrA = elementA->FirstAttribute();
		const tinyxml2::XMLAttribute* attrB = elementB->FirstAttribute();
		for (; attrA && attrB; attrA = attrA->Next(), attrB = attrB->Next()) {
			REQUIRE(std::string(attrA->Name()) == std::string(attrB->Name()));
			REQUIRE(std::string(attrA->Value()) == std::string(attrB->Value()));
		}
		REQUIRE(attrA == nullptr);
		REQUIRE(attrB == nullptr);
	}

	const tinyxml2::XMLComment* commentA = a->ToComment();
	const tinyxml2::XMLComment* commentB = b->ToComment();
	REQUIRE((commentA == nullptr) == (commentB == nullptr));
	if (commentA)
		REQUIRE(std::string(commentA->Value()) == std::string(commentB->Value()));

	const tinyxml2::XMLText* textA = a->ToText();
	const tinyxml2::XMLText* textB = b->ToText();
	REQUIRE((textA == nullptr) == (textB == nullptr));
	if (textA)
		REQUIRE(std::string(textA->Value()) == std::string(textB->Value()));

	const tinyxml2::XMLNode* childA = a->FirstChild();
	const tinyxml2::XMLNode* childB = b->FirstChild();
	for (; childA && childB; childA = childA->NextSibling(), childB = childB->NextSibling())
		RequireSameTree(childA, childB);

	REQUIRE(childA == nullptr);
	REQUIRE(childB == nullptr);
}

const ChildDesc& Child(ElementKind kind, const char* name) {
	const ElementDesc* desc = FindElement(kind);
	REQUIRE(desc != nullptr);
	const ChildDesc* child = desc->Child(name);
	REQUIRE(child != nullptr);
	return *child;
}
}

TEST_CASE("Physics XML values convert exactly as the simulation converts them", "[PhysicsXml]") {
	// Parity with the converters in src/physics/hdt/XmlReader.cpp: the editor
	// has to read a file the same way SystemBuilder will.
	SECTION("floats") {
		float value = 0.0f;
		REQUIRE(XmlValue::ParseFloat("0,5", value));
		REQUIRE(value == 0.5f);
		REQUIRE(XmlValue::ParseFloat("+1.25", value));
		REQUIRE(value == 1.25f);
		REQUIRE(XmlValue::ParseFloat("  -2.5\t", value));
		REQUIRE(value == -2.5f);
		REQUIRE_FALSE(XmlValue::ParseFloat("1.0f", value));
		REQUIRE_FALSE(XmlValue::ParseFloat("", value));
	}

	SECTION("integers keep the radix sniffing") {
		int value = 0;
		REQUIRE(XmlValue::ParseInt("0x10", value));
		REQUIRE(value == 16);
		REQUIRE(XmlValue::ParseInt("010", value));
		REQUIRE(value == 8);
		REQUIRE(XmlValue::ParseInt("+12", value));
		REQUIRE(value == 12);
		REQUIRE_FALSE(XmlValue::ParseInt("12abc", value));
	}

	SECTION("booleans") {
		bool value = false;
		REQUIRE(XmlValue::ParseBool("true", value));
		REQUIRE(value);
		REQUIRE(XmlValue::ParseBool("1", value));
		REQUIRE(value);
		REQUIRE(XmlValue::ParseBool("0", value));
		REQUIRE_FALSE(value);
		REQUIRE_FALSE(XmlValue::ParseBool("yes", value));
	}

	SECTION("writing always uses a point") {
		REQUIRE(XmlValue::FormatFloat(0.5f).find(',') == std::string::npos);
		REQUIRE(XmlValue::FormatBool(true) == "true");
	}
}

TEST_CASE("Physics XML documents survive a round trip", "[PhysicsXml]") {
	const std::string original = ReadFile(DataPath("roundtrip.xml"));

	XmlDocument doc;
	std::string error;
	REQUIRE(doc.LoadFromString(original, error));

	const std::string written = doc.Serialize();

	tinyxml2::XMLDocument before;
	REQUIRE(before.Parse(original.c_str(), original.size()) == tinyxml2::XML_SUCCESS);
	tinyxml2::XMLDocument after;
	REQUIRE(after.Parse(written.c_str(), written.size()) == tinyxml2::XML_SUCCESS);

	RequireSameTree(&before, &after);

	SECTION("comments, unknown elements and unknown attributes are all kept") {
		REQUIRE(written.find("A leading comment") != std::string::npos);
		REQUIRE(written.find("a comment nested inside an element") != std::string::npos);
		REQUIRE(written.find("unknownAttribute=\"kept verbatim\"") != std::string::npos);
		REQUIRE(written.find("<somethingUnknown") != std::string::npos);
	}

	SECTION("serializing is idempotent") {
		XmlDocument again;
		REQUIRE(again.LoadFromString(written, error));
		REQUIRE(again.Serialize() == written);
	}

	SECTION("only <system> is accepted as a root") {
		XmlDocument wrong;
		REQUIRE_FALSE(wrong.LoadFromString("<patterns/>", error));
		REQUIRE_FALSE(error.empty());
	}
}

TEST_CASE("Writing a value back unchanged is a no-op", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");
	const std::string before = doc.Serialize();

	tinyxml2::XMLElement* bone = doc.Root()->FirstChildElement("bone");
	REQUIRE(bone != nullptr);

	// The file spells this "+1.25". Writing 1.25f back must not renormalize the
	// text or mark the document dirty - the grid echoes values back constantly.
	REQUIRE(doc.SetValue(bone, Child(ElementKind::Bone, "mass"), 1.25f));
	REQUIRE(doc.Serialize() == before);
	REQUIRE_FALSE(doc.IsDirty());

	REQUIRE(doc.SetValue(bone, Child(ElementKind::Bone, "mass"), 2.0f));
	REQUIRE(doc.IsDirty());
	REQUIRE(doc.Serialize() != before);
}

TEST_CASE("Physics XML values read and write through the schema", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");
	tinyxml2::XMLElement* bone = doc.Root()->FirstChildElement("bone");

	SECTION("an absent child is not the same as a zero") {
		const ValueVariant absent = doc.GetValue(bone, Child(ElementKind::Bone, "restitution"));
		REQUIRE(std::holds_alternative<std::monostate>(absent));
	}

	SECTION("a new child lands where the schema lists it") {
		// restitution comes before margin-multiplier in the schema, and both
		// come after the mass and friction the file already has.
		REQUIRE(doc.SetValue(bone, Child(ElementKind::Bone, "margin-multiplier"), 2.0f));
		REQUIRE(doc.SetValue(bone, Child(ElementKind::Bone, "restitution"), 0.4f));

		std::vector<std::string> order;
		for (const tinyxml2::XMLElement* child = bone->FirstChildElement(); child; child = child->NextSiblingElement())
			order.push_back(child->Name());

		const std::vector<std::string> expected{"mass", "friction", "restitution", "margin-multiplier"};
		REQUIRE(order == expected);
	}

	SECTION("clearing a value hands it back to the cascade") {
		doc.ClearValue(bone, Child(ElementKind::Bone, "friction"));
		REQUIRE(std::holds_alternative<std::monostate>(doc.GetValue(bone, Child(ElementKind::Bone, "friction"))));
	}

	SECTION("text lists come back whole and are rewritten in place") {
		tinyxml2::XMLElement* shape = doc.Root()->FirstChildElement("per-triangle-shape");
		const ChildDesc& tag = Child(ElementKind::PerTriangleShape, "tag");

		const ValueVariant tags = doc.GetValue(shape, tag);
		REQUIRE(std::holds_alternative<std::vector<std::string>>(tags));
		const std::vector<std::string> both{"body", "torso"};
		REQUIRE(std::get<std::vector<std::string>>(tags) == both);

		const std::vector<std::string> justBody{"body"};
		REQUIRE(doc.SetValue(shape, tag, justBody));
		REQUIRE(std::get<std::vector<std::string>>(doc.GetValue(shape, tag)) == justBody);

		const std::vector<std::string> three{"body", "torso", "chest"};
		REQUIRE(doc.SetValue(shape, tag, three));
		REQUIRE(std::get<std::vector<std::string>>(doc.GetValue(shape, tag)).size() == 3);
	}

	SECTION("vectors read from attributes") {
		tinyxml2::XMLElement* constraint = doc.Root()->FirstChildElement("generic-constraint");
		const ValueVariant limit = doc.GetValue(constraint, Child(ElementKind::GenericConstraint, "linearUpperLimit"));
		REQUIRE(std::holds_alternative<nifly::Vector3>(limit));
		REQUIRE(std::get<nifly::Vector3>(limit).x == 1.0f);
	}
}

TEST_CASE("Cone twist limit aliases resolve to one descriptor", "[PhysicsXml]") {
	const ElementDesc* desc = FindElement(ElementKind::ConeTwistConstraint);
	REQUIRE(desc != nullptr);

	REQUIRE(desc->Child("swingSpan1") == desc->Child("coneLimit"));
	REQUIRE(desc->Child("swingSpan1") == desc->Child("limitZ"));
	REQUIRE(desc->Child("swingSpan2") == desc->Child("planeLimit"));
	REQUIRE(desc->Child("swingSpan2") == desc->Child("limitY"));
	REQUIRE(desc->Child("twistSpan") == desc->Child("twistLimit"));
	REQUIRE(desc->Child("twistSpan") == desc->Child("limitX"));

	// Reading has to follow whichever spelling the file used.
	XmlDocument doc = LoadFixture("roundtrip.xml");
	tinyxml2::XMLElement* cone = doc.Root()->FirstChildElement("conetwist-constraint");
	REQUIRE(cone != nullptr);

	const ValueVariant swing = doc.GetValue(cone, Child(ElementKind::ConeTwistConstraint, "swingSpan1"));
	REQUIRE(std::holds_alternative<float>(swing));
	REQUIRE(std::get<float>(swing) == 0.5f);
}

TEST_CASE("Node paths survive the reparse that undo performs", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");

	tinyxml2::XMLElement* shape = doc.Root()->FirstChildElement("per-triangle-shape");
	const NodePath path = doc.PathOf(shape);
	REQUIRE(doc.Resolve(path) == shape);

	const std::string before = doc.Serialize();

	doc.BeginEdit("set margin");
	REQUIRE(doc.SetValue(shape, Child(ElementKind::PerTriangleShape, "margin"), 0.25f));
	REQUIRE(doc.Serialize() != before);

	REQUIRE(doc.Undo());
	REQUIRE(doc.Serialize() == before);

	// Every pointer into the old document is gone, but the path still finds it.
	tinyxml2::XMLElement* again = doc.Resolve(path);
	REQUIRE(again != nullptr);
	REQUIRE(std::string(again->Name()) == "per-triangle-shape");
	REQUIRE(std::string(again->Attribute("name")) == "BodyShape");

	REQUIRE(doc.Redo());
	REQUIRE(std::get<float>(doc.GetValue(doc.Resolve(path), Child(ElementKind::PerTriangleShape, "margin"))) == 0.25f);
}

TEST_CASE("Undo returns a document to exactly where it started", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");
	const std::string original = doc.Serialize();

	const struct {
		const char* label;
		ElementKind kind;
		const char* element;
		const char* property;
		float value;
	} edits[] = {
		{"mass", ElementKind::Bone, "bone", "mass", 3.0f},
		{"friction", ElementKind::Bone, "bone", "friction", 0.1f},
		{"restitution", ElementKind::Bone, "bone", "restitution", 0.9f},
		{"margin", ElementKind::PerTriangleShape, "per-triangle-shape", "margin", 0.75f},
		{"penetration", ElementKind::PerTriangleShape, "per-triangle-shape", "penetration", 2.0f},
	};

	for (const auto& edit : edits) {
		tinyxml2::XMLElement* target = doc.Root()->FirstChildElement(edit.element);
		REQUIRE(target != nullptr);
		// A distinct descriptor per step, so nothing coalesces.
		doc.BeginEdit(edit.label);
		REQUIRE(doc.SetValue(target, Child(edit.kind, edit.property), edit.value));
	}

	REQUIRE(doc.Serialize() != original);

	for (std::size_t i = 0; i < sizeof(edits) / sizeof(edits[0]); i++)
		REQUIRE(doc.Undo());

	REQUIRE_FALSE(doc.CanUndo());
	REQUIRE(doc.Serialize() == original);
}

TEST_CASE("Adding and removing elements leaves the rest of the document alone", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");

	std::vector<std::string> before;
	for (const tinyxml2::XMLElement* child = doc.Root()->FirstChildElement(); child; child = child->NextSiblingElement())
		before.push_back(child->Name());

	tinyxml2::XMLElement* firstBone = doc.Root()->FirstChildElement("bone");
	tinyxml2::XMLElement* added = doc.InsertElement(ElementKind::Bone, doc.Root(), firstBone);
	REQUIRE(added != nullptr);
	// Required attributes are created so the element is at least addressable.
	REQUIRE(added->Attribute("name") != nullptr);

	std::vector<std::string> after;
	for (const tinyxml2::XMLElement* child = doc.Root()->FirstChildElement(); child; child = child->NextSiblingElement())
		after.push_back(child->Name());

	REQUIRE(after.size() == before.size() + 1);

	doc.RemoveElement(added);

	std::vector<std::string> restored;
	for (const tinyxml2::XMLElement* child = doc.Root()->FirstChildElement(); child; child = child->NextSiblingElement())
		restored.push_back(child->Name());

	REQUIRE(restored == before);
}

TEST_CASE("A document reports what it declares", "[PhysicsXml]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");

	SECTION("children come back in document order, without descending") {
		const std::vector<tinyxml2::XMLElement*> bones = doc.ChildrenOfKind(ElementKind::Bone);
		std::vector<std::string> names;
		for (const tinyxml2::XMLElement* bone : bones)
			names.push_back(bone->Attribute("name"));

		const std::vector<std::string> expected{"NPC L Breast01", "NPC L Breast02", "NPC R Breast01"};
		REQUIRE(names == expected);

		// The one inside the constraint-group is the group's child, not the
		// system's, and the builder treats it the same way.
		REQUIRE(doc.ChildrenOfKind(ElementKind::GenericConstraint).size() == 1);

		tinyxml2::XMLElement* group = doc.Root()->FirstChildElement("constraint-group");
		REQUIRE(group != nullptr);
		REQUIRE(doc.ChildrenOfKind(ElementKind::GenericConstraint, group).size() == 1);
	}

	SECTION("several kinds merge without losing document order") {
		const std::vector<tinyxml2::XMLElement*> shapes
			= doc.ChildrenOfKinds({ElementKind::PerVertexShape, ElementKind::PerTriangleShape});

		std::vector<std::string> names;
		for (const tinyxml2::XMLElement* shape : shapes)
			names.push_back(shape->Attribute("name"));

		// per-triangle-shape comes first in the file, per-vertex-shape second
		const std::vector<std::string> expected{"BodyShape", "HairShape"};
		REQUIRE(names == expected);
	}

	SECTION("elements have a name to show") {
		REQUIRE(XmlDocument::DisplayName(doc.Root()->FirstChildElement("bone")) == "NPC L Breast01");

		// A constraint has no name of its own; the pair of bodies is the label
		REQUIRE(XmlDocument::DisplayName(doc.Root()->FirstChildElement("generic-constraint")) == "NPC L Breast01 - NPC L Breast02");

		// An unnamed default falls back to what kind of element it is
		REQUIRE(XmlDocument::DisplayName(doc.Root()->FirstChildElement("bone-default")) == "bone-default");
		REQUIRE(XmlDocument::DisplayName(nullptr).empty());
	}
}

TEST_CASE("Every schema default parses as the kind it belongs to", "[PhysicsXml][schema]") {
	// The grid shows the default whenever a file leaves a property out, so a
	// default that does not parse would silently become a blank or a zero.
	for (const ElementDesc* element : AllElements()) {
		for (std::size_t i = 0; i < element->childCount; ++i) {
			const ChildDesc& child = element->children[i];
			if (!child.defaultText || child.mult == Multiplicity::List)
				continue;

			INFO(std::string(element->name) + "/" + child.name + " = " + child.defaultText);
			const ValueVariant value = DefaultValue(child);
			REQUIRE_FALSE(std::holds_alternative<std::monostate>(value));

			switch (child.kind) {
				case ValueKind::Float:
				case ValueKind::Factor:
				case ValueKind::PosFloat: REQUIRE(std::holds_alternative<float>(value)); break;
				case ValueKind::Int: REQUIRE(std::holds_alternative<int>(value)); break;
				case ValueKind::Bool: REQUIRE(std::holds_alternative<bool>(value)); break;
				case ValueKind::Vec3: REQUIRE(std::holds_alternative<nifly::Vector3>(value)); break;
				case ValueKind::Quaternion: REQUIRE(std::holds_alternative<nifly::Quaternion>(value)); break;
				case ValueKind::AxisAngle: REQUIRE(std::holds_alternative<AxisAngleValue>(value)); break;
				case ValueKind::Lerp: REQUIRE(std::holds_alternative<LerpValue>(value)); break;
				default: REQUIRE(std::holds_alternative<std::string>(value)); break;
			}
		}
	}

	SECTION("a vector default is read component by component") {
		const ChildDesc* lowerLimit = FindElement(ElementKind::GenericConstraint)->Child("angularLowerLimit");
		REQUIRE(lowerLimit != nullptr);

		const ValueVariant value = DefaultValue(*lowerLimit);
		REQUIRE(std::holds_alternative<nifly::Vector3>(value));
		REQUIRE(std::get<nifly::Vector3>(value).x == 1.0f);
		REQUIRE(std::get<nifly::Vector3>(value).z == 1.0f);
	}

	SECTION("a property with no default has none, rather than a zero") {
		const ChildDesc* tag = FindElement(ElementKind::PerVertexShape)->Child("tag");
		REQUIRE(tag != nullptr);
		REQUIRE(std::holds_alternative<std::monostate>(DefaultValue(*tag)));
	}
}

TEST_CASE("The schema table covers the whole hdtSMP64 schema", "[PhysicsXml][schema]") {
	// Ground truth is the XSD shipped with FSMP, checked in beside this test.
	// When FSMP publishes a new one, replace the file and this test says what
	// the table has to gain.
	const std::string schemaText = ReadFile(DataPath("hdtSMP64.xsd"));

	tinyxml2::XMLDocument xsd;
	REQUIRE(xsd.Parse(schemaText.c_str(), schemaText.size()) == tinyxml2::XML_SUCCESS);

	// Element names the table deliberately does not describe as elements of
	// their own: they are values of a parent element rather than editable
	// nodes, or they are XSD plumbing.
	const std::set<std::string> valueElements = {
		"basis", "basis-axis-angle", "origin", "translationLerp", "rotationLerp", "transform", "point", "halfExtend",
		"margin", "radius", "height",
	};

	std::set<std::string> declared;
	std::vector<const tinyxml2::XMLElement*> stack{xsd.RootElement()};
	while (!stack.empty()) {
		const tinyxml2::XMLElement* node = stack.back();
		stack.pop_back();
		if (!node)
			continue;

		if (std::strcmp(node->Name(), "xsd:element") == 0) {
			if (const char* name = node->Attribute("name"))
				declared.insert(name);
		}

		for (const tinyxml2::XMLElement* child = node->FirstChildElement(); child; child = child->NextSiblingElement())
			stack.push_back(child);
	}

	REQUIRE_FALSE(declared.empty());

	for (const std::string& name : declared) {
		if (valueElements.count(name))
			continue;

		INFO("the schema table has no entry for <" << name << ">, and no parent describes it as a value");

		bool covered = FindElement(name.c_str()) != nullptr;
		if (!covered) {
			for (const ElementDesc* element : AllElements()) {
				if (element->Child(name.c_str())) {
					covered = true;
					break;
				}
			}
		}
		REQUIRE(covered);
	}
}

TEST_CASE("Every schema descriptor names something the schema declares", "[PhysicsXml][schema]") {
	// The other direction, so the table cannot grow entries the format does not
	// have. The exceptions are the port's own extensions.
	const std::set<std::string> portExtensions = {"collision-filter", "priority"};

	const std::string schemaText = ReadFile(DataPath("hdtSMP64.xsd"));
	REQUIRE(schemaText.find("hdtSMP64") != std::string::npos);

	for (const ElementDesc* element : AllElements()) {
		if (!portExtensions.count(element->name)) {
			INFO("<" << element->name << "> is in the table but not in the schema");
			REQUIRE(schemaText.find(std::string("name=\"") + element->name + "\"") != std::string::npos);
		}

		for (std::size_t i = 0; i < element->childCount; i++) {
			const ChildDesc& child = element->children[i];
			if (portExtensions.count(child.name))
				continue;

			INFO("<" << element->name << "> lists a child <" << child.name << "> the schema does not declare");
			REQUIRE(schemaText.find(std::string("name=\"") + child.name + "\"") != std::string::npos);
		}
	}
}

TEST_CASE("The schema table records what this preview actually simulates", "[PhysicsXml][schema]") {
	// These are the gaps between the format and the ported parser. They are
	// recorded in one place so the editor can tell the user, and pinned here so
	// nobody silently flips one without also fixing SystemBuilder.
	const auto support = [](ElementKind kind, const char* name) {
		return Child(kind, name).preview;
	};

	REQUIRE(support(ElementKind::GenericConstraint, "linearSpringFrequency") == PreviewSupport::NotParsed);
	REQUIRE(support(ElementKind::GenericConstraint, "angularSpringFrequency") == PreviewSupport::NotParsed);
	REQUIRE(support(ElementKind::GenericConstraint, "linearNonHookeanStiffness") == PreviewSupport::ParsedNotApplied);
	REQUIRE(support(ElementKind::GenericConstraint, "angularNonHookeanDamping") == PreviewSupport::ParsedNotApplied);
	REQUIRE(support(ElementKind::PerVertexShape, "disable-tag") == PreviewSupport::ParsedNotApplied);
	REQUIRE(support(ElementKind::PerVertexShape, "priority") == PreviewSupport::NotParsed);
	REQUIRE(support(ElementKind::Bone, "can-collide-with-bone") == PreviewSupport::ParsedNotApplied);
	REQUIRE(support(ElementKind::Bone, "collision-filter") == PreviewSupport::ParsedNotApplied);

	// ... while the per-shape collide lists are the ones that do work.
	REQUIRE(support(ElementKind::PerVertexShape, "can-collide-with-bone") == PreviewSupport::Simulated);
}

TEST_CASE("The schema table records what can be changed without a rebuild", "[PhysicsXml][schema]") {
	const auto patch = [](ElementKind kind, const char* name) {
		return Child(kind, name).patch;
	};

	SECTION("scalars that live in a member the solver reads every frame") {
		REQUIRE(patch(ElementKind::Bone, "friction") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::Bone, "linearDamping") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::PerTriangleShape, "margin") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::PerTriangleShape, "penetration") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::GenericConstraint, "linearStiffness") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::ConeTwistConstraint, "swingSpan1") == PatchKind::Hot);
	}

	SECTION("things baked in at build time") {
		// SystemBuilder multiplies these into the distance the two bones
		// happened to be apart when the constraint was constructed, and does
		// not keep that distance, so re-applying them live would compound.
		REQUIRE(patch(ElementKind::StiffSpringConstraint, "minDistanceFactor") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::StiffSpringConstraint, "maxDistanceFactor") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::StiffSpringConstraint, "equilibrium") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::StiffSpringConstraint, "stiffness") == PatchKind::Hot);
		REQUIRE(patch(ElementKind::StiffSpringConstraint, "damping") == PatchKind::Hot);

		// Constraint frames are resolved against the build time bone
		// transforms, and weight thresholds are baked into the colliders.
		REQUIRE(patch(ElementKind::GenericConstraint, "frameInA") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::GenericConstraint, "useLinearReferenceFrameA") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::PerVertexShape, "weight-threshold") == PatchKind::Structural);
		REQUIRE(patch(ElementKind::Bone, "centerOfMassTransform") == PatchKind::Structural);
	}
}

TEST_CASE("Schema element kinds map back and forth", "[PhysicsXml][schema]") {
	REQUIRE(KindFromName("bone") == ElementKind::Bone);
	REQUIRE(KindFromName("not-an-element") == ElementKind::Unknown);
	REQUIRE(std::string(NameFromKind(ElementKind::PerTriangleShape)) == "per-triangle-shape");

	REQUIRE(IsDefaultKind(ElementKind::BoneDefault));
	REQUIRE_FALSE(IsDefaultKind(ElementKind::Bone));
	REQUIRE(InstanceKindOf(ElementKind::BoneDefault) == ElementKind::Bone);
	REQUIRE(DefaultKindOf(ElementKind::GenericConstraint) == ElementKind::GenericConstraintDefault);

	REQUIRE(IsSystemChildKind(ElementKind::ConstraintGroup));
	REQUIRE_FALSE(IsSystemChildKind(ElementKind::ShapeChild));

	// An instance and its template describe the same properties, which is what
	// lets the cascade resolve one against the other.
	const ElementDesc* bone = FindElement(ElementKind::Bone);
	const ElementDesc* boneDefault = FindElement(ElementKind::BoneDefault);
	REQUIRE(bone->childCount == boneDefault->childCount);
	REQUIRE(bone->children == boneDefault->children);
}

TEST_CASE("The ambient default cascade resolves the way the builder reads it", "[PhysicsXml][cascade]") {
	const std::string xml = R"(<system>
	<bone-default><mass>1</mass><friction>0.25</friction></bone-default>
	<bone name="A"/>
	<bone-default><mass>2</mass></bone-default>
	<bone name="B"/>
	<bone-default name="soft"><mass>5</mass></bone-default>
	<bone name="C" template="soft"/>
	<bone name="D" template="nothing-declares-this"/>
	<bone name="E"><mass>9</mass></bone>
</system>)";

	XmlDocument doc;
	std::string error;
	INFO(error);
	REQUIRE(doc.LoadFromString(xml, error));

	const ElementDesc* boneDesc = FindElement("bone");
	REQUIRE(boneDesc != nullptr);
	const ChildDesc* mass = boneDesc->Child("mass");
	const ChildDesc* friction = boneDesc->Child("friction");
	REQUIRE(mass != nullptr);
	REQUIRE(friction != nullptr);

	std::map<std::string, tinyxml2::XMLElement*> bones;
	for (tinyxml2::XMLElement* bone : doc.ChildrenOfKind(ElementKind::Bone))
		bones[bone->Attribute("name")] = bone;
	REQUIRE(bones.size() == 5);

	auto massOf = [&](const std::string& name) {
		const EffectiveValue effective = doc.ResolveEffective(bones[name], *mass);
		REQUIRE(std::holds_alternative<float>(effective.value));
		return std::get<float>(effective.value);
	};

	SECTION("an unnamed default applies to everything after it, and only after it") {
		REQUIRE(massOf("A") == 1.0f);
		REQUIRE(massOf("B") == 2.0f);
	}

	SECTION("a named default is selected by template and starts from the ambient set") {
		REQUIRE(massOf("C") == 5.0f);

		// friction was never mentioned by "soft", so it still comes from the
		// first unnamed default, which is what the template was built on top of.
		const EffectiveValue inherited = doc.ResolveEffective(bones["C"], *friction);
		REQUIRE(std::get<float>(inherited.value) == 0.25f);
	}

	SECTION("a template name nothing declares falls back to the ambient set") {
		// getBoneTemplate hands out m_boneTemplates[""] rather than failing, so
		// a typo in a template name is silent at runtime.
		REQUIRE(massOf("D") == 2.0f);
	}

	SECTION("a value on the element itself wins and is reported as its own") {
		const EffectiveValue effective = doc.ResolveEffective(bones["E"], *mass);
		REQUIRE(std::get<float>(effective.value) == 9.0f);
		REQUIRE(effective.explicitHere);
		REQUIRE(effective.source == bones["E"]);
	}

	SECTION("an inherited value says where it came from") {
		const EffectiveValue effective = doc.ResolveEffective(bones["B"], *mass);
		REQUIRE_FALSE(effective.explicitHere);
		REQUIRE(effective.sourceLabel == "bone-default #2");

		const EffectiveValue named = doc.ResolveEffective(bones["C"], *mass);
		REQUIRE(named.sourceLabel == "bone-default \"soft\"");
	}

	SECTION("a property nothing supplies falls through to the schema default") {
		const ChildDesc* restitution = boneDesc->Child("restitution");
		REQUIRE(restitution != nullptr);

		const EffectiveValue effective = doc.ResolveEffective(bones["A"], *restitution);
		REQUIRE(effective.source == nullptr);
		REQUIRE(std::get<float>(effective.value) == std::get<float>(DefaultValue(*restitution)));
	}
}

TEST_CASE("A constraint group does not scope the defaults inside it", "[PhysicsXml][cascade]") {
	const std::string xml = R"(<system>
	<generic-constraint-default><linearStiffness x="1" y="1" z="1"/></generic-constraint-default>
	<constraint-group>
		<generic-constraint-default><linearStiffness x="7" y="7" z="7"/></generic-constraint-default>
		<generic-constraint bodyA="a" bodyB="b"/>
	</constraint-group>
	<generic-constraint bodyA="c" bodyB="d"/>
</system>)";

	XmlDocument doc;
	std::string error;
	INFO(error);
	REQUIRE(doc.LoadFromString(xml, error));

	const ElementDesc* desc = FindElement("generic-constraint");
	REQUIRE(desc != nullptr);
	const ChildDesc* stiffness = desc->Child("linearStiffness");
	REQUIRE(stiffness != nullptr);

	const std::vector<tinyxml2::XMLElement*> groups = doc.ChildrenOfKind(ElementKind::ConstraintGroup);
	REQUIRE(groups.size() == 1);

	const std::vector<tinyxml2::XMLElement*> inside = doc.ChildrenOfKind(ElementKind::GenericConstraint, groups[0]);
	const std::vector<tinyxml2::XMLElement*> outside = doc.ChildrenOfKind(ElementKind::GenericConstraint);
	REQUIRE(inside.size() == 1);
	REQUIRE(outside.size() == 1);

	auto stiffnessOf = [&](tinyxml2::XMLElement* constraint) {
		const EffectiveValue effective = doc.ResolveEffective(constraint, *stiffness);
		REQUIRE(std::holds_alternative<nifly::Vector3>(effective.value));
		return std::get<nifly::Vector3>(effective.value).x;
	};

	REQUIRE(stiffnessOf(inside[0]) == 7.0f);

	// The one after the group gets the group's default too: the builder writes
	// every *-default into the same template map, whatever it is nested in.
	REQUIRE(stiffnessOf(outside[0]) == 7.0f);
}

TEST_CASE("Shape defaults do not cascade, because this preview never reads them", "[PhysicsXml][cascade]") {
	const std::string xml = R"(<system>
	<per-vertex-shape-default><margin>4</margin></per-vertex-shape-default>
	<per-vertex-shape name="Body"/>
</system>)";

	XmlDocument doc;
	std::string error;
	INFO(error);
	REQUIRE(doc.LoadFromString(xml, error));

	const ElementDesc* desc = FindElement("per-vertex-shape");
	REQUIRE(desc != nullptr);
	const ChildDesc* margin = desc->Child("margin");
	REQUIRE(margin != nullptr);

	const std::vector<tinyxml2::XMLElement*> shapes = doc.ChildrenOfKind(ElementKind::PerVertexShape);
	REQUIRE(shapes.size() == 1);

	// SystemBuilder's parse loop has no case for per-vertex-shape-default: it
	// warns about it and skips it, so the shape is built with the schema
	// default and the editor has to say the same.
	REQUIRE_FALSE(CascadeApplies(ElementKind::PerVertexShape));

	const EffectiveValue effective = doc.ResolveEffective(shapes[0], *margin);
	REQUIRE(effective.source == nullptr);
	REQUIRE(std::get<float>(effective.value) == std::get<float>(DefaultValue(*margin)));
}

TEST_CASE("Validation reports what the schema and the port each care about", "[PhysicsXml][validate]") {
	const std::string xml = R"(<system>
	<bone name="NPC L Breast"><mass>1</mass><gravity-factor>4</gravity-factor></bone>
	<bone name="NPC L Breast"><mass>1</mass></bone>
	<per-vertex-shape name="Body"><shared>sideways</shared></per-vertex-shape>
	<per-triangle-shape name="Body">
		<can-collide-with-tag>hair</can-collide-with-tag>
		<no-collide-with-tag>cloth</no-collide-with-tag>
	</per-triangle-shape>
	<generic-constraint bodyA="NPC L Breast" template="never-declared"/>
	<shape type="sphere"><radius>3</radius></shape>
	<somethingUnknown/>
</system>)";

	XmlDocument doc;
	std::string error;
	INFO(error);
	REQUIRE(doc.LoadFromString(xml, error));

	const std::vector<Diagnostic> diagnostics = doc.Validate();

	auto reported = [&diagnostics](const std::string& fragment) {
		return std::any_of(diagnostics.begin(), diagnostics.end(), [&fragment](const Diagnostic& diagnostic) {
			return diagnostic.message.find(fragment) != std::string::npos;
		});
	};

	auto severityOf = [&diagnostics](const std::string& fragment) {
		for (const Diagnostic& diagnostic : diagnostics) {
			if (diagnostic.message.find(fragment) != std::string::npos)
				return diagnostic.severity;
		}
		return Diagnostic::Severity::Info;
	};

	SECTION("the identity constraints the XSD declares") {
		REQUIRE(reported("is already declared"));
		REQUIRE(severityOf("a <bone> named") == Diagnostic::Severity::Error);
	}

	SECTION("a required attribute that is missing") {
		REQUIRE(reported("the required attribute \"bodyB\" is missing"));
	}

	SECTION("a value outside the range the schema documents") {
		REQUIRE(reported("outside the documented range 0 .. 1"));
	}

	SECTION("a value the enumeration does not list") {
		REQUIRE(reported("which the schema does not list"));
	}

	SECTION("a template name nothing declares, which the builder swallows") {
		REQUIRE(reported("names no generic-constraint-default declared before this point"));
	}

	SECTION("a blacklist a whitelist has already made pointless") {
		REQUIRE(reported("so the blacklist does nothing"));
	}

	SECTION("the nameless top level shape that aborts the whole file") {
		REQUIRE(reported("aborts the whole file"));
		REQUIRE(severityOf("aborts the whole file") == Diagnostic::Severity::Error);
	}

	SECTION("an element this preview does not understand, as a note rather than a fault") {
		REQUIRE(reported("not part of the hdtSMP64 schema"));
		REQUIRE(severityOf("not part of the hdtSMP64 schema") == Diagnostic::Severity::Info);
	}

	SECTION("names from outside the file are only checked when they are known") {
		ValidationContext context;
		context.skeletonBoneNames = {"NPC R Breast"};

		const std::vector<Diagnostic> withSkeleton = doc.Validate(context);
		REQUIRE(std::any_of(withSkeleton.begin(), withSkeleton.end(), [](const Diagnostic& diagnostic) {
			return diagnostic.message.find("is not a node of the loaded skeleton") != std::string::npos;
		}));

		// Without a skeleton the same document must not complain about names it
		// has no way to check.
		REQUIRE_FALSE(reported("is not a node of the loaded skeleton"));
	}
}

TEST_CASE("Collision filtering answers with the rule the simulation uses", "[PhysicsXml][collision]") {
	const std::string xml = R"(<system>
	<per-vertex-shape name="Body">
		<tag>body</tag>
		<no-collide-with-tag>vagina</no-collide-with-tag>
	</per-vertex-shape>
	<per-triangle-shape name="Hair">
		<tag>hair</tag>
		<can-collide-with-tag>body</can-collide-with-tag>
	</per-triangle-shape>
	<per-vertex-shape name="Vagina">
		<tag>vagina</tag>
	</per-vertex-shape>
	<per-vertex-shape name="Hidden">
		<tag>hidden</tag>
		<shared>external</shared>
	</per-vertex-shape>
</system>)";

	XmlDocument doc;
	std::string error;
	INFO(error);
	REQUIRE(doc.LoadFromString(xml, error));
	doc.SetSource("body.xml", "", XmlOrigin::New);

	const std::vector<ShapeFacts> facts = CollectShapeFacts(doc);
	REQUIRE(facts.size() == 4);

	auto find = [&facts](const std::string& name) {
		const auto it = std::find_if(facts.begin(), facts.end(), [&name](const ShapeFacts& entry) { return entry.name == name; });
		REQUIRE(it != facts.end());
		return *it;
	};

	const ShapeFacts body = find("Body");
	const ShapeFacts hair = find("Hair");
	const ShapeFacts vagina = find("Vagina");
	const ShapeFacts hidden = find("Hidden");

	SECTION("a whitelist decides on its own and the pair still needs both sides to agree") {
		REQUIRE(EvaluateCollision(hair, body).collides);
		REQUIRE(EvaluateCollision(body, hair).collides);

		// Hair whitelists "body" and nothing else, so the vagina is turned away
		// by a list it does not own.
		REQUIRE_FALSE(EvaluateCollision(hair, vagina).collides);
		REQUIRE_FALSE(EvaluateCollision(vagina, hair).collides);
	}

	SECTION("a blacklist is consulted in both directions") {
		const CollisionResult result = EvaluateCollision(vagina, body);
		REQUIRE_FALSE(result.collides);
		REQUIRE(result.reason.find("blacklists") != std::string::npos);

		// Only the body carries the blacklist, and the answer is the same
		// whichever way round the pair is asked.
		REQUIRE_FALSE(EvaluateCollision(body, vagina).collides);
	}

	SECTION("shared=external never collides with anything in a single actor preview") {
		REQUIRE_FALSE(EvaluateCollision(hidden, body).collides);
		REQUIRE_FALSE(EvaluateCollision(body, hidden).collides);
		REQUIRE(EvaluateCollision(body, hidden).reason.find("external") != std::string::npos);
	}

	SECTION("shared=private keeps a shape to the file it was declared in") {
		ShapeFacts elsewhere = body;
		elsewhere.xmlPath = "outfit.xml";
		elsewhere.name = "Cloth";
		elsewhere.tags = {"cloth"};
		elsewhere.noCollideWithTags.clear();

		ShapeFacts privateBody = body;
		privateBody.shared = "private";

		REQUIRE(EvaluateCollision(privateBody, elsewhere).collides == false);

		elsewhere.xmlPath = privateBody.xmlPath;
		REQUIRE(EvaluateCollision(privateBody, elsewhere).collides);
	}

	SECTION("tags match without regard to case, as IDStr does") {
		ShapeFacts shouting = vagina;
		shouting.tags = {"VAGINA"};

		REQUIRE_FALSE(EvaluateCollision(body, shouting).collides);
	}
}

TEST_CASE("A document that is edited back to where it started is not dirty", "[PhysicsXml][dirty]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");
	REQUIRE_FALSE(doc.IsDirty());

	const ElementDesc* boneDesc = FindElement("bone");
	REQUIRE(boneDesc != nullptr);
	const ChildDesc* mass = boneDesc->Child("mass");
	REQUIRE(mass != nullptr);

	const std::vector<tinyxml2::XMLElement*> bones = doc.ChildrenOfKind(ElementKind::Bone);
	REQUIRE_FALSE(bones.empty());

	SECTION("undoing the only change clears the marker") {
		doc.BeginEdit("mass");
		REQUIRE(doc.SetValue(bones[0], *mass, 9.25f));
		REQUIRE(doc.IsDirty());

		REQUIRE(doc.Undo());
		REQUIRE_FALSE(doc.IsDirty());

		// And redoing it brings the marker back, rather than leaving a changed
		// document looking saved.
		REQUIRE(doc.Redo());
		REQUIRE(doc.IsDirty());
	}

	SECTION("undoing one of two changes does not") {
		doc.BeginEdit("mass");
		REQUIRE(doc.SetValue(bones[0], *mass, 9.25f));
		doc.BeginEdit("friction", NodePath(), boneDesc->Child("friction"));
		REQUIRE(doc.SetValue(bones[0], *boneDesc->Child("friction"), 0.75f));

		REQUIRE(doc.Undo());
		REQUIRE(doc.IsDirty());
	}
}

TEST_CASE("Saving a document is what makes it clean", "[PhysicsXml][dirty]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");

	const ElementDesc* boneDesc = FindElement("bone");
	const ChildDesc* mass = boneDesc ? boneDesc->Child("mass") : nullptr;
	REQUIRE(mass != nullptr);

	const std::vector<tinyxml2::XMLElement*> bones = doc.ChildrenOfKind(ElementKind::Bone);
	REQUIRE_FALSE(bones.empty());

	doc.BeginEdit("mass");
	REQUIRE(doc.SetValue(bones[0], *mass, 3.5f));
	REQUIRE(doc.IsDirty());

	const std::filesystem::path written = std::filesystem::temp_directory_path() / "bsos-physics-save-test.xml";
	std::filesystem::remove(written);

	std::string error;
	INFO(error);
	REQUIRE(doc.SaveToFile(written.string(), error));
	doc.MarkClean();
	REQUIRE_FALSE(doc.IsDirty());

	SECTION("what was written reads back as the same document") {
		XmlDocument reloaded;
		REQUIRE(reloaded.LoadFromString(ReadFile(written.string()), error));
		REQUIRE(reloaded.Serialize() == doc.Serialize());

		const std::vector<tinyxml2::XMLElement*> reloadedBones = reloaded.ChildrenOfKind(ElementKind::Bone);
		REQUIRE_FALSE(reloadedBones.empty());
		REQUIRE(std::get<float>(reloaded.GetValue(reloadedBones[0], *mass)) == 3.5f);
	}

	SECTION("undoing past the save makes it dirty again") {
		// The save is what the document is now compared against, so going back
		// to the text it was loaded from is a change like any other.
		REQUIRE(doc.Undo());
		REQUIRE(doc.IsDirty());
	}

	std::filesystem::remove(written);
}

TEST_CASE("Importing over a document is an edit to it, not a new document", "[PhysicsXml][dirty]") {
	XmlDocument doc = LoadFixture("roundtrip.xml");
	const std::string before = doc.Serialize();

	std::string error;
	INFO(error);
	REQUIRE(doc.ReplaceContent("<system><bone name=\"Imported\"/></system>", error));

	SECTION("the new content is in place and counts as unsaved") {
		REQUIRE(doc.IsDirty());
		REQUIRE(doc.ChildrenOfKind(ElementKind::Bone).size() == 1);
		REQUIRE(XmlDocument::DisplayName(doc.ChildrenOfKind(ElementKind::Bone)[0]) == "Imported");
	}

	SECTION("the binding to the file it stands for is untouched") {
		doc.SetSource("hair.xml", "C:/mods/hair.xml", XmlOrigin::Loose);
		REQUIRE(doc.ReplaceContent("<system/>", error));
		REQUIRE(doc.XmlPath() == "hair.xml");
		REQUIRE(doc.SourcePath() == "C:/mods/hair.xml");
		REQUIRE(doc.Origin() == XmlOrigin::Loose);
	}

	SECTION("a file that will not parse changes nothing") {
		REQUIRE_FALSE(doc.ReplaceContent("<system><unclosed></system>", error));
		REQUIRE_FALSE(error.empty());
		REQUIRE(doc.ChildrenOfKind(ElementKind::Bone).size() == 1);

		// Nor does one this editor would refuse to open on its own.
		REQUIRE_FALSE(doc.ReplaceContent("<patterns/>", error));
		REQUIRE(doc.ChildrenOfKind(ElementKind::Bone).size() == 1);
	}

	SECTION("it can be taken back like anything else") {
		XmlDocument again = LoadFixture("roundtrip.xml");
		again.BeginEdit("import");
		REQUIRE(again.ReplaceContent("<system/>", error));
		REQUIRE(again.CanUndo());
		REQUIRE(again.Undo());
		REQUIRE(again.Serialize() == before);
		REQUIRE_FALSE(again.IsDirty());
	}
}
