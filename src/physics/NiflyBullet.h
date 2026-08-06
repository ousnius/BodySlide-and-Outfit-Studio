/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#ifdef USE_BULLET

#include <Object3d.hpp>

#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>

// Conversions between nifly math types and Bullet math types. The physics
// simulation runs in NIF global space (the same space as
// AnimBone::xformPoseToGlobal), so no axis swaps or unit scaling happen here.
// MatTransform scale is intentionally dropped: physics bones are treated as
// scale 1 (the builder warns when a posed bone is scaled).
namespace bsos {
inline btVector3 ToBt(const nifly::Vector3& v) {
	return btVector3(v.x, v.y, v.z);
}

inline nifly::Vector3 FromBt(const btVector3& v) {
	return nifly::Vector3(v.x(), v.y(), v.z());
}

inline btMatrix3x3 ToBt(const nifly::Matrix3& m) {
	return btMatrix3x3(m[0].x, m[0].y, m[0].z, m[1].x, m[1].y, m[1].z, m[2].x, m[2].y, m[2].z);
}

inline nifly::Matrix3 FromBt(const btMatrix3x3& m) {
	nifly::Matrix3 r;
	for (int i = 0; i < 3; ++i) {
		r[i].x = m[i].x();
		r[i].y = m[i].y();
		r[i].z = m[i].z();
	}
	return r;
}

inline btTransform ToBt(const nifly::MatTransform& t) {
	return btTransform(ToBt(t.rotation), ToBt(t.translation));
}

inline nifly::MatTransform FromBt(const btTransform& t) {
	nifly::MatTransform r;
	r.rotation = FromBt(t.getBasis());
	r.translation = FromBt(t.getOrigin());
	r.scale = 1.0f;
	return r;
}
}

#endif	// USE_BULLET
