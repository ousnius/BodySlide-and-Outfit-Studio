#include "../src/files/PhysicsXmlFile.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Physics XML shapes know the bones declared before them", "[PhysicsXmlFile]") {
	const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<system>
	<bone-default>
		<mass>0</mass>
	</bone-default>
	<bone name="NPC Spine2 [Spn2]"/>
	<per-triangle-shape name="VirtualBody">
		<margin>1</margin>
	</per-triangle-shape>
	<bone-default>
		<mass>1,5</mass>
	</bone-default>
	<bone name="Cape 01"/>
	<bone name="cape 01"/>
	<per-vertex-shape name="Cape">
		<margin>1</margin>
	</per-vertex-shape>
	<bone name="Cape 02"/>
</system>)";

	PhysicsXmlFile file;
	REQUIRE(file.Parse(xml));
	CHECK(file.Bones() == std::vector<std::string>{"NPC Spine2 [Spn2]", "Cape 01", "Cape 02"});

	const PhysicsXmlFile::Shape* body = file.FindShape("virtualbody");
	REQUIRE(body);
	CHECK(body->implicitMass == 0.0f);
	CHECK(body->Declares("NPC Spine2 [Spn2]"));
	CHECK_FALSE(body->Declares("Cape 01"));

	// A bone declared after the shape is created by it with the default template, not declared
	const PhysicsXmlFile::Shape* cape = file.FindShape("Cape");
	REQUIRE(cape);
	CHECK(cape->implicitMass == Catch::Approx(1.5f));
	CHECK(cape->Declares("CAPE 01"));
	CHECK_FALSE(cape->Declares("Cape 02"));
	CHECK_FALSE(cape->Declares("NPC Pelvis [Pelv]"));

	CHECK_FALSE(file.FindShape("Boots"));
}

TEST_CASE("Named physics bone templates don't change the default", "[PhysicsXmlFile]") {
	const std::string xml = R"(<system>
	<bone-default name="heavy">
		<mass>2</mass>
	</bone-default>
	<bone-default name="heavier" extends="heavy"/>
	<per-vertex-shape name="Skirt"/>
	<bone-default extends="heavier"/>
	<per-vertex-shape name="Hood"/>
</system>)";

	PhysicsXmlFile file;
	REQUIRE(file.Parse(xml));
	REQUIRE(file.FindShape("Skirt"));
	CHECK(file.FindShape("Skirt")->implicitMass == 0.0f);
	REQUIRE(file.FindShape("Hood"));
	CHECK(file.FindShape("Hood")->implicitMass == 2.0f);
}

TEST_CASE("Broken physics XML is reported", "[PhysicsXmlFile]") {
	PhysicsXmlFile file;
	CHECK_FALSE(file.Parse("<system><bone name=\"a\"></system>"));
	CHECK_FALSE(file.Error().empty());
	CHECK_FALSE(file.Parse("<physics/>"));
	CHECK(file.Shapes().empty());
}
