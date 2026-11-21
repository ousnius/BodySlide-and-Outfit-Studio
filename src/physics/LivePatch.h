/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

namespace Physics {
class PreviewSystem;
struct PatchRequest;

// Writes one changed property of a physics XML straight into the objects of a running system, for
// Controller::PatchProperty. False when the value can't be changed live (it is baked in at build time, the
// element has no live counterpart, a name doesn't resolve, the address matches more than one constraint, or
// the property was cleared back to the *-default cascade), and the system has to be rebuilt instead.
bool PatchLiveProperty(PreviewSystem& system, const PatchRequest& request);
} // namespace Physics
