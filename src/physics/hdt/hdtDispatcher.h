#pragma once

#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "hdtBulletHelper.h"
#include <mutex>
#include <vector>

namespace hdt
{
	class SkinnedMeshBody;

	// Upstream derives from btCollisionDispatcherMt, which requires a global
	// Bullet task scheduler and a BT_THREADSAFE build of Bullet (stock
	// vcpkg/distro packages have neither). This port dispatches serially, so
	// the plain btCollisionDispatcher base provides identical behavior.
	class CollisionDispatcher : public btCollisionDispatcher
	{
	public:
		CollisionDispatcher(btCollisionConfiguration* collisionConfiguration) :
			btCollisionDispatcher(
				collisionConfiguration)
		{
		}

		btPersistentManifold* getNewManifold(const btCollisionObject* b0, const btCollisionObject* b1) override
		{
			std::lock_guard<decltype(m_lock)> l(m_lock);
			auto ret = btCollisionDispatcher::getNewManifold(b0, b1);
			return ret;
		}

		void releaseManifold(btPersistentManifold* manifold) override
		{
			std::lock_guard<decltype(m_lock)> l(m_lock);
			btCollisionDispatcher::releaseManifold(manifold);
		}

		bool needsCollision(const btCollisionObject* body0, const btCollisionObject* body1) override;
		void dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, const btDispatcherInfo& dispatchInfo,
			btDispatcher* dispatcher) override;

		int getNumManifolds() const override;
		btPersistentManifold** getInternalManifoldPointer() override;
		btPersistentManifold* getManifoldByIndexInternal(int index) override;

		void clearAllManifold();

		hdt::SpinLock m_lock;
		std::vector<std::pair<SkinnedMeshBody*, SkinnedMeshBody*>> m_pairs;
	};
}
