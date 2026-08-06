#pragma once

#include "hdtAABB.h"
#include "hdtBulletHelper.h"
#include <memory>

namespace hdt
{
	class SkinnedMeshBody;

	// Sentinel time step: bone readTransform implementations treat any step
	// <= this as "teleport to the kinematic pose without dynamics". Upstream
	// this lived in hdtSkyrimPhysicsWorld.h.
	constexpr float RESET_PHYSICS = -10.0f;
	struct HDT_ALIGN16 SkinnedMeshBone :
		public RefCounted
	{
		BT_DECLARE_ALIGNED_ALLOCATOR();

		SkinnedMeshBone(const IDStr& name, btRigidBody::btRigidBodyConstructionInfo& ci);
		virtual ~SkinnedMeshBone();

		IDStr m_name;
		float m_marginMultipler;
		float m_boudingSphereMultipler = 1.0f;
		float m_gravityFactor = 1.0f;
		float m_windFactor = 1.0f;  // Mapped to <wind-factor> in the XML. Acts as a multiplier for the global wind force applied to this bone (0.0 = no wind, 2.0 = double wind)

		btRigidBody m_rig;
		btTransform m_localToRig;
		btTransform m_rigToLocal;
		btQsTransform m_currentTransform;

		std::vector<IDStr> m_canCollideWithBone;
		std::vector<IDStr> m_noCollideWithBone;

		virtual void readTransform(float timeStep) = 0;
		virtual void writeTransform() = 0;

		void internalUpdate();

		bool canCollideWith(SkinnedMeshBone* rhs);
	};
}
