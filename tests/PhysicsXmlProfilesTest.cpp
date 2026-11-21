#include "physics/XmlProfiles.h"

#include <catch2/catch_test_macros.hpp>

#include <tinyxml2.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace Physics;

namespace {

std::string ReadFixture(const std::string& name) {
	std::ifstream stream(std::string(BSOS_TEST_DATA_DIR) + "/physics/" + name, std::ios::binary);
	REQUIRE(stream.good());
	std::ostringstream contents;
	contents << stream.rdbuf();
	return contents.str();
}

XmlDocument Load(const std::string& xml) {
	XmlDocument doc;
	std::string error;
	const bool loaded = doc.LoadFromString(xml, error);
	INFO(error);
	REQUIRE(loaded);
	return doc;
}

// Every bone and constraint, in reading order
std::vector<tinyxml2::XMLElement*> Elements(const XmlDocument& doc) {
	std::vector<tinyxml2::XMLElement*> elements;
	std::function<void(tinyxml2::XMLElement*)> walk = [&](tinyxml2::XMLElement* parent) {
		for (tinyxml2::XMLElement* child = parent->FirstChildElement(); child; child = child->NextSiblingElement()) {
			const ElementKind kind = KindFromName(child->Name());
			if (kind == ElementKind::Bone || kind == ElementKind::GenericConstraint || kind == ElementKind::StiffSpringConstraint
				|| kind == ElementKind::ConeTwistConstraint)
				elements.push_back(child);
			else if (kind == ElementKind::ConstraintGroup)
				walk(child);
		}
	};
	walk(doc.Root());
	return elements;
}

tinyxml2::XMLElement* Bone(const XmlDocument& doc, const std::string& name) {
	for (tinyxml2::XMLElement* element : Elements(doc))
		if (KindFromName(element->Name()) == ElementKind::Bone && name == element->Attribute("name"))
			return element;
	FAIL("no bone " + name);
	return nullptr;
}

// What the builder makes of one property of an element, worked out independently of the profile code: the element,
// or else the last default of its chain, that sets it. readBoneTemplate clears both collide lists at the first entry
// of either, so for those either one counts.
std::string Built(const XmlDocument& doc, const tinyxml2::XMLElement* element, const ChildDesc& desc) {
	const std::string name = desc.name;
	const bool collide = name == "can-collide-with-bone" || name == "no-collide-with-bone";
	const ElementDesc* bone = FindElement(ElementKind::Bone);

	auto sets = [&](const tinyxml2::XMLElement* reader) {
		if (doc.HasValue(reader, desc))
			return true;
		return collide && (doc.HasValue(reader, *bone->Child("can-collide-with-bone")) || doc.HasValue(reader, *bone->Child("no-collide-with-bone")));
	};

	std::vector<const tinyxml2::XMLElement*> readers = doc.TemplateChain(element);
	readers.push_back(element);
	for (auto it = readers.rbegin(); it != readers.rend(); ++it)
		if (sets(*it))
			return "[" + doc.ValueText(*it, desc) + "]";

	return "(schema default)";
}

// Every property of every bone and constraint, by element position and property name
std::map<std::string, std::string> BuiltAll(const XmlDocument& doc) {
	std::map<std::string, std::string> values;
	int index = 0;
	for (const tinyxml2::XMLElement* element : Elements(doc)) {
		const ElementDesc* desc = FindElement(KindFromName(element->Name()));
		for (std::size_t i = 0; i < desc->childCount; i++)
			values[std::to_string(index) + " " + XmlDocument::DisplayName(element) + " " + desc->children[i].name] = Built(doc, element, desc->children[i]);
		index++;
	}
	return values;
}

const ChildDesc& BoneProperty(const char* name) {
	const ChildDesc* desc = FindElement(ElementKind::Bone)->Child(name);
	REQUIRE(desc != nullptr);
	return *desc;
}

float Float(const XmlDocument& doc, const tinyxml2::XMLElement* element, const char* name) {
	const ValueVariant value = doc.ResolveEffective(element, BoneProperty(name)).value;
	REQUIRE(std::holds_alternative<float>(value));
	return std::get<float>(value);
}

} // namespace

TEST_CASE("Defaults read as profiles", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(ReadFixture("defaults.xml"));

	const std::vector<tinyxml2::XMLElement*> profiles = Profiles(doc);
	REQUIRE(profiles.size() == 9);

	tinyxml2::XMLElement* first = profiles[0];
	tinyxml2::XMLElement* soft = profiles[2];
	tinyxml2::XMLElement* second = profiles[4];
	REQUIRE(doc.ProfileLabel(first) == "Default");
	REQUIRE(doc.ProfileLabel(soft) == "soft");
	REQUIRE(doc.ProfileLabel(second) == "Default 2");

	SECTION("an element's profile is the last default of its chain") {
		REQUIRE(ProfileOf(doc, Bone(doc, "A")) == first);
		REQUIRE(ProfileOf(doc, Bone(doc, "B")) == soft);
		REQUIRE(ProfileOf(doc, Bone(doc, "D")) == second);
		// A template naming nothing falls back to the ambient set
		REQUIRE(ProfileOf(doc, Bone(doc, "E")) == second);
	}

	SECTION("template names match without regard to case, like IDStr") {
		REQUIRE(ProfileOf(doc, Bone(doc, "C")) == profiles[3]);
		REQUIRE(Float(doc, Bone(doc, "C"), "linearDamping") == 0.9f);
	}

	SECTION("a named default is a snapshot of what it extends") {
		// base was redefined after soft extended it, which soft doesn't see
		REQUIRE(Float(doc, Bone(doc, "B"), "linearDamping") == 0.5f);
	}

	SECTION("every default starts from the ambient set, so the first one reaches every bone") {
		REQUIRE(ProfileUsers(doc, first).size() == 5);
		REQUIRE(ProfileUsers(doc, soft).size() == 1);
		REQUIRE(ProfileUsers(doc, second).size() == 2);
	}

	SECTION("a collide list replaces both lists a bone inherits") {
		// base's list replaces the first default's
		const EffectiveValue list = doc.ResolveChain(doc.TemplateChain(Bone(doc, "B")), BoneProperty("can-collide-with-bone"));
		REQUIRE(std::get<std::vector<std::string>>(list.value) == std::vector<std::string>{"NPC Spine2 [Spn2]"});

		// E's own replaces what it inherits
		const EffectiveValue own = doc.ResolveEffective(Bone(doc, "E"), BoneProperty("can-collide-with-bone"));
		REQUIRE(own.explicitHere);
		REQUIRE(std::get<std::vector<std::string>>(own.value) == std::vector<std::string>{"E1"});
	}

	SECTION("unnamed defaults with others of their kind are positional, named ones never") {
		REQUIRE(IsPositional(doc, first));
		REQUIRE(IsPositional(doc, second));
		REQUIRE_FALSE(IsPositional(doc, soft));
		REQUIRE(NeedsTidy(doc));
	}
}

TEST_CASE("Tidying keeps every value", "[PhysicsXml][profiles]") {
	for (const char* fixture : {"defaults.xml", "roundtrip.xml"}) {
		INFO(fixture);
		XmlDocument doc = Load(ReadFixture(fixture));
		const std::map<std::string, std::string> before = BuiltAll(doc);
		const std::string original = doc.Serialize();

		const TidyResult result = TidyDefaults(doc);
		INFO(result.report);
		REQUIRE(result.ok);
		REQUIRE(BuiltAll(doc) == before);
		REQUIRE_FALSE(NeedsTidy(doc));

		for (tinyxml2::XMLElement* profile : Profiles(doc)) {
			REQUIRE_FALSE(IsPositional(doc, profile));
			REQUIRE(profile->Parent() == doc.Root());
			REQUIRE(profile->Attribute("extends") == nullptr);
		}

		// As one undo step
		REQUIRE(doc.Undo());
		REQUIRE(doc.Serialize() == original);
	}
}

TEST_CASE("After tidying, a bone reads the same wherever it goes", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(ReadFixture("defaults.xml"));
	REQUIRE(TidyDefaults(doc).ok);

	// After the profiles, at the end, and inside a constraint group
	std::vector<tinyxml2::XMLElement*> added;
	added.push_back(doc.InsertElementBefore(ElementKind::Bone, Bone(doc, "A")));
	added.push_back(doc.InsertElement(ElementKind::Bone, doc.Root(), nullptr));
	added.push_back(doc.InsertElement(ElementKind::Bone, doc.ChildrenOfKind(ElementKind::ConstraintGroup).front(), nullptr));

	std::vector<std::string> mass;
	for (tinyxml2::XMLElement* bone : added)
		mass.push_back(Built(doc, bone, BoneProperty("mass")) + Built(doc, bone, BoneProperty("can-collide-with-bone")));

	REQUIRE(mass[0] == mass[1]);
	REQUIRE(mass[1] == mass[2]);
}

TEST_CASE("Tidying keeps the profiles an author named", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(ReadFixture("defaults.xml"));
	REQUIRE(TidyDefaults(doc).ok);

	std::vector<std::string> names;
	for (tinyxml2::XMLElement* profile : Profiles(doc))
		names.push_back(doc.ProfileLabel(profile));

	auto has = [&names](const std::string& name) { return std::find(names.begin(), names.end(), name) != names.end(); };
	REQUIRE(has("soft"));
	REQUIRE(has("stiff"));
	// Nothing used it, but someone wrote it for a reason
	REQUIRE(has("unused"));

	REQUIRE(std::string(Bone(doc, "B")->Attribute("template")) == "soft");
}

TEST_CASE("Tidying refuses what only declaring bones could keep", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(R"(<system>
	<bone-default><mass>1</mass></bone-default>
	<per-vertex-shape name="Body"/>
	<bone-default><mass>2</mass></bone-default>
	<per-vertex-shape name="Outfit"/>
</system>)");
	const std::string original = doc.Serialize();

	const TidyResult result = TidyDefaults(doc);
	REQUIRE_FALSE(result.ok);
	REQUIRE(result.report.find("Outfit") != std::string::npos);
	REQUIRE(doc.Serialize() == original);
	REQUIRE_FALSE(doc.CanUndo());
}

TEST_CASE("The two collide lists of a bone go together", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(R"(<system>
	<bone-default><can-collide-with-bone>Head</can-collide-with-bone></bone-default>
	<bone name="A"><no-collide-with-bone>Spine</no-collide-with-bone></bone>
	<bone name="B"/>
</system>)");
	tinyxml2::XMLElement* a = Bone(doc, "A");
	const ChildDesc& can = BoneProperty("can-collide-with-bone");
	const ChildDesc& no = BoneProperty("no-collide-with-bone");

	SECTION("the other list clears an inherited one") {
		const EffectiveValue cleared = doc.ResolveEffective(a, can);
		REQUIRE(cleared.explicitHere);
		REQUIRE(std::get<std::vector<std::string>>(cleared.value).empty());
		REQUIRE(HasOwnValue(doc, a, can));
	}

	SECTION("applying one to the profile moves both") {
		const std::string b = Built(doc, Bone(doc, "B"), can) + Built(doc, Bone(doc, "B"), no);
		REQUIRE(ApplyToProfile(doc, a, can));
		REQUIRE_FALSE(HasOwnValue(doc, a, no));
		REQUIRE(Built(doc, a, can) + Built(doc, a, no) == "[][<no-collide-with-bone>Spine</no-collide-with-bone>]");
		REQUIRE(Built(doc, Bone(doc, "B"), can) + Built(doc, Bone(doc, "B"), no) != b);
	}

	SECTION("resetting one resets both") {
		ResetToProfile(doc, a, can);
		REQUIRE(Built(doc, a, can) == "[<can-collide-with-bone>Head</can-collide-with-bone>]");
		REQUIRE_FALSE(doc.HasValue(a, no));
	}

	SECTION("tidying keeps them") {
		const std::map<std::string, std::string> before = BuiltAll(doc);
		REQUIRE(TidyDefaults(doc).ok);
		REQUIRE(BuiltAll(doc) == before);
	}
}

TEST_CASE("Values move between an element and its profile", "[PhysicsXml][profiles]") {
	XmlDocument doc = Load(ReadFixture("defaults.xml"));
	const std::string original = doc.Serialize();

	SECTION("applying a value to the profile reaches its users and keeps the element's value") {
		tinyxml2::XMLElement* b = Bone(doc, "B");
		REQUIRE(ApplyToProfile(doc, b, BoneProperty("angularDamping")));
		REQUIRE_FALSE(doc.HasValue(b, BoneProperty("angularDamping")));
		REQUIRE(Float(doc, b, "angularDamping") == 0.3f);
		REQUIRE(doc.HasValue(ProfileOf(doc, b), BoneProperty("angularDamping")));

		REQUIRE(doc.Undo());
		REQUIRE(doc.Serialize() == original);
	}

	SECTION("a collide list applied to the profile replaces the profile's") {
		REQUIRE(ApplyToProfile(doc, Bone(doc, "E"), BoneProperty("can-collide-with-bone")));
		const std::string expected = Built(doc, Bone(doc, "E"), BoneProperty("can-collide-with-bone"));
		REQUIRE(expected == "[<can-collide-with-bone>E1</can-collide-with-bone>]");
		// D uses the same profile
		REQUIRE(Built(doc, Bone(doc, "D"), BoneProperty("can-collide-with-bone")) == expected);
	}

	SECTION("a new profile takes the element's values without changing them") {
		const std::map<std::string, std::string> before = BuiltAll(doc);
		tinyxml2::XMLElement* b = Bone(doc, "B");

		REQUIRE(CreateProfileFrom(doc, b, "SOFT") == nullptr);

		tinyxml2::XMLElement* created = CreateProfileFrom(doc, b, "ear tip");
		REQUIRE(created != nullptr);
		REQUIRE(ProfileOf(doc, b) == created);
		REQUIRE(std::string(created->Attribute("extends")) == "soft");
		REQUIRE_FALSE(doc.HasValue(b, BoneProperty("angularDamping")));
		REQUIRE(BuiltAll(doc) == before);
	}
}
