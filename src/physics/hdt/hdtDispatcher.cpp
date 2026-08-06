#ifdef USE_BULLET

#include "hdtDispatcher.h"
#include "hdtSkinnedMeshAlgorithm.h"
#include "hdtSkinnedMeshBody.h"

#include <LinearMath/btPoolAllocator.h>
#include <algorithm>

namespace hdt
{
	void CollisionDispatcher::clearAllManifold()
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		for (int i = 0; i < m_manifoldsPtr.size(); ++i) {
			auto manifold = m_manifoldsPtr[i];
			manifold->~btPersistentManifold();
			if (m_persistentManifoldPoolAllocator->validPtr(manifold))
				m_persistentManifoldPoolAllocator->freeMemory(manifold);
			else
				btAlignedFree(manifold);
		}
		m_manifoldsPtr.clear();
	}

	bool needsCollision(const SkinnedMeshBody* shape0, const SkinnedMeshBody* shape1)
	{
		if (!shape0 || !shape1 || shape0 == shape1)
			return false;

		if (shape0->m_isKinematic && shape1->m_isKinematic)
			return false;

		return shape0->canCollideWith(shape1) && shape1->canCollideWith(shape0);
	}

	static inline bool isSkinnedMesh(const btCollisionObject* obj)
	{
		return obj->getCollisionShape()->getShapeType() == CUSTOM_CONCAVE_SHAPE_TYPE;
	}

	bool CollisionDispatcher::needsCollision(const btCollisionObject* body0, const btCollisionObject* body1)
	{
		bool skinned0 = isSkinnedMesh(body0);
		bool skinned1 = isSkinnedMesh(body1);

		if (skinned0 || skinned1) {
			auto shape0 = skinned0 ? static_cast<const SkinnedMeshBody*>(body0) : nullptr;
			auto shape1 = skinned1 ? static_cast<const SkinnedMeshBody*>(body1) : nullptr;
			return hdt::needsCollision(shape0, shape1);
		}

		if (body0->isStaticOrKinematicObject() && body1->isStaticOrKinematicObject())
			return false;

		// Todo: This is likely dead code as only skinned objects can collide as of right now (3/20/2026)
		if (body0->checkCollideWith(body1) || body1->checkCollideWith(body0)) {
			auto rb0 = static_cast<SkinnedMeshBone*>(body0->getUserPointer());
			auto rb1 = static_cast<SkinnedMeshBone*>(body1->getUserPointer());

			return rb0->canCollideWith(rb1) && rb1->canCollideWith(rb0);
		} else
			return false;
	}

	// Docs: This is called by Bullet's broad phase, which finds pairs that may be colliding. We built these aabb's inside SkinnedMeshBody::updateBoundingSphereAabb() using Skyrim's bone
	// spheres. This function checks if those pairs are even allowed to collide, updates the skinned shapes, then passes it to the next few phases of collision checks.
	// Collision phases are: Bullet's broadphase, our BVH midphase, then finally our narrowphase
	void CollisionDispatcher::dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, [[maybe_unused]] const btDispatcherInfo& dispatchInfo, [[maybe_unused]] btDispatcher* dispatcher)
	{
		BT_PROFILE("HDTSMP_dispatchAllCollisionPairs");

		auto size = pairCache->getNumOverlappingPairs();
		if (!size)
			return;

		m_pairs.reserve(size);
		auto pairs = pairCache->getOverlappingPairArrayPtr();
		std::vector<SkinnedMeshBody*> bodies;

		// SkinnedMeshBody:internalUpdate() already calls m_shape->internalUpdate() for both
		// PerVertexShape and PerTriangleShape, so separate vertex/triangle shape update lists are
		// unnecessary. The only shapes not covered are the m_verticesCollision companions
		// that PerTriangleShape creates for triangle-vs-triangle collision pairs.
		// Tldr: Triangle shapes ARE still updated because body->internalUpdate() handles them
		std::vector<PerVertexShape*> extra_vertex_shapes;

		bodies.reserve(size * 2);
		extra_vertex_shapes.reserve(size);

		for (int i = 0; i < size; ++i) {
			auto& pair = pairs[i];
			auto obj0 = static_cast<btCollisionObject*>(pair.m_pProxy0->m_clientObject);
			auto obj1 = static_cast<btCollisionObject*>(pair.m_pProxy1->m_clientObject);

			bool skinned0 = isSkinnedMesh(obj0);
			bool skinned1 = isSkinnedMesh(obj1);

			if (skinned0 || skinned1) {
				auto shape0 = skinned0 ? static_cast<SkinnedMeshBody*>(obj0) : nullptr;
				auto shape1 = skinned1 ? static_cast<SkinnedMeshBody*>(obj1) : nullptr;

				if (hdt::needsCollision(shape0, shape1)) {
					bodies.push_back(shape0);
					bodies.push_back(shape1);
					m_pairs.push_back(std::make_pair(shape0, shape1));

					auto a = shape0->m_shape->asPerTriangleShape();
					auto b = shape1->m_shape->asPerTriangleShape();

					if (a && b) {
						extra_vertex_shapes.push_back(a->m_verticesCollision.get());
						extra_vertex_shapes.push_back(b->m_verticesCollision.get());
					}
				}
			} else {
				// [3/13/2026]
				// This should never be called. We do addRigidBody(&system->m_bones[i]->m_rig, 0, 0) which means it's invisible
				// to bullet's collision detector entirely. Why is this here? Likely in preparation for non-skinned objects..?
				getNearCallback()(pair, *this, dispatchInfo);
			}
		}

		// Wipe duplicates, since we don't want to reskin anything!
		std::sort(bodies.begin(), bodies.end());
		bodies.erase(std::unique(bodies.begin(), bodies.end()), bodies.end());

		std::sort(extra_vertex_shapes.begin(), extra_vertex_shapes.end());
		extra_vertex_shapes.erase(std::unique(extra_vertex_shapes.begin(), extra_vertex_shapes.end()), extra_vertex_shapes.end());

		par::for_each(bodies.begin(), bodies.end(), [](SkinnedMeshBody* shape) {
			if (shape->m_useBoundingSphere)
				shape->internalUpdate();
		});

		if (!extra_vertex_shapes.empty()) {
			par::for_each(extra_vertex_shapes.begin(), extra_vertex_shapes.end(), [](PerVertexShape* shape) {
				shape->internalUpdate();
			});
		}

		{
			BT_PROFILE("HDTSMP_collision_pair_checks");

			par::for_each(m_pairs.begin(), m_pairs.end(), [&, this](const std::pair<SkinnedMeshBody*, SkinnedMeshBody*>& i) {
				// collapseCollideL is a candidate for optimizations since we'll re-traverse the tree in checkCollisionL. However,
				// it's a bit tricky since collapse has an early-exit, and avoiding boilerplate is ideal. A traversal resume maybe?
				if (i.first->m_shape->m_tree.collapseCollideL(&i.second->m_shape->m_tree)) {
					BT_PROFILE("HDTSMP_processCollision");
					SkinnedMeshAlgorithm::processCollision(i.first, i.second, this);
				}
			});
		}

		m_pairs.clear();
	}

	int CollisionDispatcher::getNumManifolds() const
	{
		return m_manifoldsPtr.size();
	}

	btPersistentManifold* CollisionDispatcher::getManifoldByIndexInternal(int index)
	{
		return m_manifoldsPtr[index];
	}

	btPersistentManifold** CollisionDispatcher::getInternalManifoldPointer()
	{
		return btCollisionDispatcher::getInternalManifoldPointer();
	}
}

#endif  // USE_BULLET
