/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Mesh.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

// Vertex mask operations on a mesh, independent of how they are triggered.
// The *Changes functions return the new mask value for each vertex they affect,
// so callers can record them for undo before applying them.
namespace MaskOps {

// One step of growing or shrinking the mask. Welded vertices on the border of the
// mask come first, and only when there are none the adjacent ring of vertices.
std::unordered_map<int, float> GrowShrinkChanges(Mesh& m, bool grow);

// Every vertex with its inverted mask value
std::unordered_map<int, float> InvertChanges(const Mesh& m);

// Every vertex with a mask value of zero
std::unordered_map<int, float> ClearChanges(const Mesh& m);

void ApplyChanges(Mesh& m, const std::unordered_map<int, float>& changes);

// The masked vertices with their mask values
std::unordered_map<uint16_t, float> GetMasked(const Mesh& m);

// Mask values of all vertices
std::vector<float> GetMask(const Mesh& m);

// Returns false if the number of values doesn't match the vertex count.
bool SetMask(Mesh& m, const std::vector<float>& mask);

} // namespace MaskOps
