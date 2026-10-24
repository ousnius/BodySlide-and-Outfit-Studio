/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <istream>
#include <string>
#include <vector>

// The bones of an SMP physics XML (linked from a NIF by "HDT Skinned Mesh Physics Object" extra data) and the
// shapes it simulates. SMP reads the file from top to bottom: a <bone> declares a bone, and a shape element
// (<per-vertex-shape>, <per-triangle-shape>) takes every bone its vertices are weighted to. A bone the file
// hasn't declared by then is created on the spot with the unnamed <bone-default> in force at that line, so
// with a mass above 0 there, a bone a weight copy added becomes dynamic and falls.
class PhysicsXmlFile {
public:
	struct Shape {
		std::string name;
		// Bones declared before the shape element
		std::vector<std::string> declaredBones;
		// Mass a bone that isn't declared gets at the shape element. Above 0 the bone is dynamic.
		float implicitMass = 0.0f;

		// Case-insensitive, like SMP
		bool Declares(const std::string& bone) const;
	};

	bool Load(std::istream& stream);
	bool Parse(const std::string& xml);

	const std::string& Error() const { return error; }
	const std::vector<std::string>& Bones() const { return bones; }
	const std::vector<Shape>& Shapes() const { return shapes; }

	// The shape element with this NIF shape name, case-insensitive. Null when the file doesn't simulate it.
	const Shape* FindShape(const std::string& name) const;

private:
	std::string error;
	std::vector<std::string> bones;
	std::vector<Shape> shapes;
};
