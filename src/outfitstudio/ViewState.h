/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "render/GLSurface.h"

#include <map>
#include <string>

// How a shape is shown in the view, the same as the states of the shape list items
enum class ShapeDisplayMode { Visible = 0, Hidden = 1, Ghost = 2 };

// Everything about how the view looks, to change it for a render and restore it afterwards
struct ViewState {
	GLSurface::CameraState camera;
	GLSurface::DisplaySettings display;
	std::map<std::string, ShapeDisplayMode> shapes;
};
