/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#ifdef USE_BULLET

#include "SystemBuilder.h"

#include <string>
#include <vector>

class GLSurface;

namespace Physics {
// Draws the bone collision shapes and constraints of a set of physics systems
// as named overlay primitives on a surface. Owns the names it drew so it can
// replace or remove them again. Per-vertex/per-triangle collider clouds are
// not drawn individually (they would be one overlay mesh per collider).
class DebugVis {
public:
	// Replaces everything drawn by the previous call.
	void Update(GLSurface& gls, const std::vector<hdt::Ref<PreviewSystem>>& systems);

	// Removes all overlays of the previous Update.
	void Clear(GLSurface& gls);

private:
	std::string nextName() { return "physvis_" + std::to_string(visNames.size()); }

	void drawShape(GLSurface& gls, const btCollisionShape* shape, const btTransform& transform, const nifly::Vector3& color);
	void drawConstraint(GLSurface& gls, const btTypedConstraint* constraint, const btTransform& rootMotionInv);

	std::vector<std::string> visNames;
};
}

#endif	// USE_BULLET
