#include "../src/components/StarfieldSpace.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <utility>

using Catch::Approx;
using nifly::MatTransform;
using nifly::Matrix3;
using nifly::Vector3;

namespace {
MatTransform SkinTransformInMeters() {
	MatTransform xform;
	xform.translation = Vector3(0.012f, -0.034f, 1.21f);
	xform.rotation = Matrix3::MakeRotation(0.25f, -0.1f, 0.4f);
	xform.scale = 1.5f;
	return xform;
}

void RequireSameTranslation(const MatTransform& a, const MatTransform& b) {
	REQUIRE(a.translation.x == Approx(b.translation.x));
	REQUIRE(a.translation.y == Approx(b.translation.y));
	REQUIRE(a.translation.z == Approx(b.translation.z));
}

void RequireSameRotationAndScale(const MatTransform& a, const MatTransform& b) {
	REQUIRE(a.scale == Approx(b.scale));
	for (int row = 0; row < 3; row++)
		for (int col = 0; col < 3; col++)
			REQUIRE(a.rotation[row][col] == Approx(b.rotation[row][col]));
}
} // namespace

TEST_CASE("Skinned Starfield shapes are placed in havokScale space for conforming", "[starfield][conform]") {
	const MatTransform skin = SkinTransformInMeters();
	MatTransform expected = skin;
	expected.translation *= sfHavokScale;

	const MatTransform conform = ShapeToGlobalForConform(skin, true, true);
	RequireSameTranslation(conform, expected);
	RequireSameRotationAndScale(conform, skin);
}

TEST_CASE("Conform space matches the editor's scaled global-to-shape transform", "[starfield][conform]") {
	// wxGLPanel::AddMeshFromNif scales the translation of the inverse transform instead.
	// Both have to describe the same placement or the outfit and reference drift apart.
	const MatTransform skin = SkinTransformInMeters();
	MatTransform editorGlobalToShape = skin.InverseTransform();
	editorGlobalToShape.translation *= sfHavokScale;

	const MatTransform conformGlobalToShape = ShapeToGlobalForConform(skin, true, true).InverseTransform();
	RequireSameTranslation(conformGlobalToShape, editorGlobalToShape);
	RequireSameRotationAndScale(conformGlobalToShape, editorGlobalToShape);
}

TEST_CASE("Unskinned or non-Starfield shapes keep their transform for conforming", "[starfield][conform]") {
	const MatTransform skin = SkinTransformInMeters();

	for (const auto& [isSkinned, isStarfield] : {std::pair{false, true}, std::pair{true, false}, std::pair{false, false}}) {
		const MatTransform conform = ShapeToGlobalForConform(skin, isSkinned, isStarfield);
		RequireSameTranslation(conform, skin);
		RequireSameRotationAndScale(conform, skin);
	}
}
