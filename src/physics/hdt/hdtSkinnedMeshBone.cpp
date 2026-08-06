#ifdef USE_BULLET

#include "hdtSkinnedMeshBone.h"

namespace hdt
{
	SkinnedMeshBone::SkinnedMeshBone(const IDStr& name, btRigidBody::btRigidBodyConstructionInfo& ci) :
		m_name(name), m_rig(ci)
	{
		m_rigToLocal.setIdentity();
		m_localToRig.setIdentity();
		m_currentTransform.setScale(1);

		m_marginMultipler = 1.0f;

		m_rig.setUserPointer(this);
	}

	SkinnedMeshBone::~SkinnedMeshBone()
	{
	}

	void SkinnedMeshBone::internalUpdate()
	{
		auto t = m_rigToLocal * m_rig.getInterpolationWorldTransform();
		m_currentTransform.setBasis(t.getBasis());
		m_currentTransform.setOrigin(t.getOrigin());
	}

	bool SkinnedMeshBone::canCollideWith(SkinnedMeshBone* rhs)
	{
		if (m_canCollideWithBone.size()) {
			return std::find(m_canCollideWithBone.begin(), m_canCollideWithBone.end(), rhs->m_name) !=
			       m_canCollideWithBone.end();
		}
		return std::find(m_noCollideWithBone.begin(), m_noCollideWithBone.end(), rhs->m_name) == m_noCollideWithBone.end();
	}
}

#endif  // USE_BULLET
