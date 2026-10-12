/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <vector>

namespace MaskTransfer {
// Transfers a vertex mask from a source mesh to a target mesh with a different vertex layout.
// Each target vertex looks up the closest point on the source surface and takes the mask value
// of the nearest corner of that triangle, so mask borders stay as sharp as on the source rather
// than being blurred by an average. Target vertices with no source surface within maxDistance
// get a mask of 0 and are false in outMatched.
void CopyMask(const std::vector<nifly::Vector3>& sourceVerts,
			  const std::vector<nifly::Triangle>& sourceTris,
			  const std::vector<float>& sourceMask,
			  const std::vector<nifly::Vector3>& targetVerts,
			  float maxDistance,
			  std::vector<float>& outMask,
			  std::vector<bool>& outMatched);
} // namespace MaskTransfer
