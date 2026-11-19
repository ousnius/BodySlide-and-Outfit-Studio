/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitStudioMcp.h"
#include "project/OutfitProject.h"

#include <cmath>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <wx/string.h>

// Small helpers the MCP tool files share

// A vector as a JSON array of three numbers
inline nlohmann::json McpVector(const nifly::Vector3& v) {
	return nlohmann::json::array({v.x, v.y, v.z});
}

// The names of shapes as a JSON array
inline nlohmann::json ShapeNames(const std::vector<nifly::NiShape*>& shapes) {
	nlohmann::json names = nlohmann::json::array();
	for (nifly::NiShape* shape : shapes)
		names.push_back(shape->name.get());
	return names;
}

inline std::vector<nifly::Triangle> ShapeTriangles(nifly::NiShape* shape) {
	std::vector<nifly::Triangle> tris;
	shape->GetTriangles(tris);
	return tris;
}

// The vertices of a shape in the NIF, without sliders or pose
inline std::vector<nifly::Vector3> RestVerts(OutfitProject& project, nifly::NiShape* shape) {
	std::vector<nifly::Vector3> verts;
	project.GetWorkNif()->GetVertsForShape(shape, verts);
	return verts;
}

// The mesh of a shape in the view. Throws if there is none.
inline Mesh* ShapeMesh(OutfitStudioMcp& mcp, nifly::NiShape* shape) {
	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	if (!m)
		throw McpToolError("The shape '" + shape->name.get() + "' has no mesh in the view.");
	return m;
}

// A value rounded to a multiple of step, for readable results
inline float RoundTo(float v, float step = 0.01f) {
	return std::round(v / step) * step;
}

inline std::string ToUTF8(const wxString& text) {
	return text.ToUTF8().data();
}
