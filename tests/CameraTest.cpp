#include "render/GLSurface.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <vector>

using namespace nifly;

namespace {

// A turned and pitched camera with every offset set, so no term of the view matrix drops out
void SetTurnedCamera(GLSurface& gls, bool perspective) {
	gls.perspective = perspective;
	gls.camPos = Vector3(0.4f, -3.0f, -12.0f);
	gls.camOffset = Vector3(0.3f, -0.2f, 0.5f);
	gls.camRot = Vector3(25.0f, 70.0f, 0.0f);
	gls.camRotOffset = Vector3(-0.5f, 2.0f, 0.25f);
}

glm::vec3 ToView(const glm::mat4& view, const Vector3& p) {
	return glm::vec3(view * glm::vec4(p.x, p.y, p.z, 1.0f));
}

// Where the model points end up on screen
glm::vec3 ToClip(const GLSurface& gls, const Vector3& p) {
	glm::mat4 projection, view;
	gls.ComputeCameraMatrices(1.5f, projection, view);
	glm::vec4 clip = projection * view * glm::vec4(p.x, p.y, p.z, 1.0f);
	return glm::vec3(clip) / clip.w;
}

const Vector3 modelPoints[] = {Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 6.0f, -0.5f), Vector3(-2.0f, 1.0f, 1.5f), Vector3(0.5f, 11.0f, 0.2f)};

} // namespace

TEST_CASE("Moving the rotation center keeps the perspective view", "[Camera]") {
	GLSurface gls;
	SetTurnedCamera(gls, true);

	glm::mat4 projection, viewBefore, viewAfter;
	gls.ComputeCameraMatrices(1.5f, projection, viewBefore);

	const Vector3 center(1.5f, 9.0f, -1.0f);
	gls.SetRotationCenter(center);
	gls.ComputeCameraMatrices(1.5f, projection, viewAfter);

	for (auto& p : modelPoints) {
		glm::vec3 before = ToView(viewBefore, p);
		glm::vec3 after = ToView(viewAfter, p);
		CHECK(after.x == Catch::Approx(before.x).margin(1e-4));
		CHECK(after.y == Catch::Approx(before.y).margin(1e-4));
		CHECK(after.z == Catch::Approx(before.z).margin(1e-4));
	}

	// The camera now turns around the center, so it stays where it is in the view
	glm::vec3 centerBefore = ToView(viewAfter, center);
	gls.camRot = Vector3(-40.0f, 200.0f, 0.0f);
	gls.ComputeCameraMatrices(1.5f, projection, viewAfter);
	glm::vec3 centerAfter = ToView(viewAfter, center);
	CHECK(centerAfter.x == Catch::Approx(centerBefore.x).margin(1e-4));
	CHECK(centerAfter.y == Catch::Approx(centerBefore.y).margin(1e-4));
	CHECK(centerAfter.z == Catch::Approx(centerBefore.z).margin(1e-4));
}

TEST_CASE("Resetting the rotation center keeps the view and turns around the offset target", "[Camera]") {
	GLSurface gls;
	SetTurnedCamera(gls, true);

	glm::mat4 projection, viewBefore, viewAfter;
	gls.ComputeCameraMatrices(1.5f, projection, viewBefore);

	gls.ResetRotationCenter();
	CHECK(gls.camRotOffset.IsZero(true));

	gls.ComputeCameraMatrices(1.5f, projection, viewAfter);
	for (auto& p : modelPoints) {
		glm::vec3 before = ToView(viewBefore, p);
		glm::vec3 after = ToView(viewAfter, p);
		CHECK(after.x == Catch::Approx(before.x).margin(1e-4));
		CHECK(after.y == Catch::Approx(before.y).margin(1e-4));
		CHECK(after.z == Catch::Approx(before.z).margin(1e-4));
	}
}

TEST_CASE("Moving the rotation center keeps the orthographic view and zoom", "[Camera]") {
	GLSurface gls;
	SetTurnedCamera(gls, false);

	std::vector<glm::vec3> before;
	for (auto& p : modelPoints)
		before.push_back(ToClip(gls, p));

	const float zoom = gls.camPos.z;
	gls.SetRotationCenter(Vector3(1.5f, 9.0f, -1.0f));
	CHECK(gls.camPos.z == zoom);

	for (size_t i = 0; i < std::size(modelPoints); i++) {
		glm::vec3 after = ToClip(gls, modelPoints[i]);
		CHECK(after.x == Catch::Approx(before[i].x).margin(1e-4));
		CHECK(after.y == Catch::Approx(before[i].y).margin(1e-4));
	}
}

TEST_CASE("Looking at the rotation center puts it in the middle at the distance", "[Camera]") {
	const Vector3 center(1.0f, 8.0f, 2.0f);

	for (bool perspective : {true, false}) {
		GLSurface gls;
		SetTurnedCamera(gls, perspective);
		gls.LookAtRotationCenter(center, 7.0f);

		glm::mat4 projection, view;
		gls.ComputeCameraMatrices(1.5f, projection, view);
		glm::vec3 inView = ToView(view, center);
		CHECK(inView.x == Catch::Approx(0.0f).margin(1e-4));
		CHECK(inView.y == Catch::Approx(0.0f).margin(1e-4));
		if (perspective)
			CHECK(inView.z == Catch::Approx(-7.0f).margin(1e-4));
		else
			CHECK(gls.camPos.z == -7.0f);

		// The camera turns around the center, so it stays in the middle
		gls.camRot = Vector3(-40.0f, 200.0f, 0.0f);
		gls.ComputeCameraMatrices(1.5f, projection, view);
		inView = ToView(view, center);
		CHECK(inView.x == Catch::Approx(0.0f).margin(1e-4));
		CHECK(inView.y == Catch::Approx(0.0f).margin(1e-4));
		if (perspective)
			CHECK(inView.z == Catch::Approx(-7.0f).margin(1e-4));
	}
}
