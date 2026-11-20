/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "NifFile.hpp"

#include <functional>
#include <string>
#include <vector>

class AnimSkeleton;

// NIF helpers the parts of OutfitProject share

// Cloth is told apart by the bones an outfit adds to the reference skeleton. Tells the bones of the skeleton.
std::function<bool(const std::string&)> IsSkeletonBone(AnimSkeleton& skeleton);

// HDT-SMP links a model to its physics XML with a NiStringExtraData named "hdt skinned mesh physics object".
// The game only reads it from the model's root node.
bool IsPhysicsExtraData(nifly::NiExtraData* extraData);

// The HDT-SMP links on the root node of a NIF, in block order
std::vector<nifly::NiStringExtraData*> GetRootPhysicsExtraData(nifly::NifFile& nif);

// Points the strings of a block that was cloned out of another NIF at the
// strings of the header it now lives in
void RemapExtraDataStrings(nifly::NifFile& nif, nifly::NiExtraData* extraData);
