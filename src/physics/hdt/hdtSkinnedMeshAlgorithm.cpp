#ifdef USE_BULLET

#include "hdtSkinnedMeshAlgorithm.h"
#include "hdtCollider.h"

namespace hdt
{

	// CollisionCheckBase1 provides data members and the basic constructor for the target types. Note that we
	// always collide a vertex shape against something else, so only the second type is templated.
	template <typename T>
	struct CollisionCheckBase1
	{
		typedef typename PerVertexShape::ShapeProp SP0;
		typedef typename T::ShapeProp SP1;

		CollisionCheckBase1(PerVertexShape* a, T* b, CollisionResult* r)
		{
			v0 = a->m_owner->m_vpos.data();
			v1 = b->m_owner->m_vpos.data();
			c0 = &a->m_tree;
			c1 = &b->m_tree;
			sp0 = &a->m_shapeProp;
			sp1 = &b->m_shapeProp;
			results = r;
			numResults = 0;
		}

		VertexPos* v0;
		VertexPos* v1;
		ColliderTree* c0;
		ColliderTree* c1;
		SP0* sp0;
		SP1* sp1;

		std::atomic_long numResults;
		CollisionResult* results;
	};

	// CollisionCheckBase2 provides the method to add results, swapping the colliders if necessary. This
	// means we can support triangle-sphere collisions by reversing the input shapes and setting SwapResults
	// to true, instead of having two almost identical versions of the same lower-level algorithm.
	template <typename T, bool SwapResults>
	struct CollisionCheckBase2;

	template <typename T>
	struct CollisionCheckBase2<T, false> : public CollisionCheckBase1<T>
	{
		template <typename... Ts>
		CollisionCheckBase2(Ts&&... ts) :
			CollisionCheckBase1<T>(std::forward<Ts>(ts)...)
		{}

		bool addResult(const CollisionResult& res)
		{
			int p = this->numResults.fetch_add(1);
			if (p < SkinnedMeshAlgorithm::MaxCollisionCount) {
				this->results[p] = res;
				return true;
			}
			return false;
		}
	};

	template <typename T>
	struct CollisionCheckBase2<T, true> : public CollisionCheckBase1<T>
	{
		template <typename... Ts>
		CollisionCheckBase2(Ts&&... ts) :
			CollisionCheckBase1<T>(std::forward<Ts>(ts)...)
		{}

		bool addResult(const CollisionResult& res)
		{
			int p = this->numResults.fetch_add(1);
			if (p < SkinnedMeshAlgorithm::MaxCollisionCount) {
				this->results[p].posA = res.posB;
				this->results[p].posB = res.posA;
				this->results[p].colliderA = res.colliderB;
				this->results[p].colliderB = res.colliderA;
				this->results[p].normOnB = -res.normOnB;
				this->results[p].depth = res.depth;
				return true;
			}
			return false;
		}
	};

	// CollisionChecker provides the checkCollide method, which handles a single pair of colliders. This does
	// the accurate collision check for the CPU algorithms. GPU algorithms will provide their own methods for
	// this, and should derive directly from CollisionCheckBase2.
	template <typename T, bool SwapResults>
	struct CollisionChecker;

	template <bool SwapResults>
	struct CollisionChecker<PerVertexShape, SwapResults> : public CollisionCheckBase2<PerVertexShape, SwapResults>
	{
		template <typename... Ts>
		CollisionChecker(Ts&&... ts) :
			CollisionCheckBase2<PerVertexShape, SwapResults>(std::forward<Ts>(ts)...)
		{}

		bool checkCollide(Collider* a, Collider* b, CollisionResult& res)
		{
			auto s0 = this->v0[a->vertex];
			auto r0 = s0.marginMultiplier() * this->sp0->margin;
			auto s1 = this->v1[b->vertex];
			auto r1 = s1.marginMultiplier() * this->sp1->margin;

			auto pos0 = s0.pos();
			auto pos1 = s1.pos();
			auto diff = pos0 - pos1;
			auto dist2 = diff.length2();
			auto radiusSum = r0 + r1;
			if (dist2 > radiusSum * radiusSum)
				return false;

			auto len = btSqrt(dist2);
			auto normal = btVector3(1, 0, 0);
			if (len > FLT_EPSILON)
				normal = diff / len;

			res.normOnB = normal;
			res.depth = len - radiusSum;
			res.posA = pos0 - normal * r0;
			res.posB = pos1 + normal * r1;
			res.colliderA = a;
			res.colliderB = b;
			return true;
		}
	};

	namespace
	{
		inline __m128 cross_product(__m128 const& vec0, __m128 const& vec1)
		{
			__m128 tmp0 = _mm_shuffle_ps(vec0, vec0, _MM_SHUFFLE(3, 0, 2, 1));
			__m128 tmp1 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(3, 1, 0, 2));
			__m128 tmp2 = _mm_mul_ps(tmp0, vec1);
			__m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			__m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return _mm_sub_ps(tmp3, tmp4);
		}
	}

	template <bool SwapResults>
	struct CollisionChecker<PerTriangleShape, SwapResults> : public CollisionCheckBase2<PerTriangleShape, SwapResults>
	{
		template <typename... Ts>
		CollisionChecker(Ts&&... ts) :
			CollisionCheckBase2<PerTriangleShape, SwapResults>(std::forward<Ts>(ts)...)
		{}

		bool checkCollide(Collider* a, Collider* b, CollisionResult& res)
		{
			auto s = this->v0[a->vertex];
			auto r = s.marginMultiplier() * this->sp0->margin;
			auto p0 = this->v1[b->vertices[0]];
			auto p1 = this->v1[b->vertices[1]];
			auto p2 = this->v1[b->vertices[2]];
			auto margin = (p0.marginMultiplier() + p1.marginMultiplier() + p2.marginMultiplier()) * (1.0f / 3.0f);
			auto penetration = this->sp1->penetration * margin;
			margin *= this->sp1->margin;
			if (penetration > -FLT_EPSILON && penetration < FLT_EPSILON) {
				penetration = 0;
			}

			auto ab = toSimd(p1.pos() - p0.pos());
			auto ac = toSimd(p2.pos() - p0.pos());
			auto raw_normal = cross_product(ab, ac);
			auto len = _mm_sqrt_ps(_mm_dp_ps(raw_normal, raw_normal, 0x77));
			if (_mm_cvtss_f32(len) < FLT_EPSILON) {
				return false;
			}
			auto normal = _mm_div_ps(raw_normal, len);
			if (penetration < 0) {
				normal = _mm_sub_ps(_mm_setzero_ps(), normal);
				penetration = -penetration;
			}

			auto ap = toSimd(s.pos() - p0.pos());
			auto distance = _mm_dp_ps(ap, normal, 0x77);
			float distanceFromPlane = _mm_cvtss_f32(distance);
			auto projection = _mm_sub_ps(toSimd(s.pos()), _mm_mul_ps(normal, distance));
			float radiusWithMargin = r + margin;
			bool isInsideContactPlane;
			if (penetration >= FLT_EPSILON)
				isInsideContactPlane = distanceFromPlane < radiusWithMargin && distanceFromPlane >= -penetration;
			else {
				if (distanceFromPlane < 0) {
					distanceFromPlane = -distanceFromPlane;
					normal = _mm_sub_ps(_mm_setzero_ps(), normal);
				}
				isInsideContactPlane = distanceFromPlane < radiusWithMargin;
			}

			// This has a very high early rejection rate, up to ~60% average
			if (!isInsideContactPlane) {
				return false;
			}

			// Compute (twice) area of each triangle between projection and two triangle points
			ap = _mm_sub_ps(projection, toSimd(p0.pos()));
			auto bp = _mm_sub_ps(projection, toSimd(p1.pos()));
			auto cp = _mm_sub_ps(projection, toSimd(p2.pos()));
			auto aa = cross_product(bp, cp);
			ab = cross_product(cp, ap);
			ac = cross_product(ap, bp);
			aa = _mm_dp_ps(aa, aa, 0x74);
			ab = _mm_dp_ps(ab, ab, 0x72);
			ac = _mm_dp_ps(ac, ac, 0x71);
			aa = _mm_or_ps(aa, ab);
			aa = _mm_or_ps(aa, ac);
			aa = _mm_sqrt_ps(aa);
			// Now if every pair of elements in aa sums to no more than area, then the point is inside the triangle
			aa = _mm_add_ps(aa, _mm_shuffle_ps(aa, aa, _MM_SHUFFLE(3, 0, 2, 1)));
			aa = _mm_cmpgt_ps(aa, len);
			auto pointInTriangle = _mm_test_all_zeros(_mm_set_epi32(0, -1, -1, -1), _mm_castps_si128(aa));

			res.colliderA = a;
			res.colliderB = b;
			if (pointInTriangle) {
				setSimd(res.normOnB, normal);
				res.posA = s.pos() - res.normOnB * r;
				setSimd(res.posB, projection);
				res.depth = distanceFromPlane - radiusWithMargin;
				return res.depth < -FLT_EPSILON;
			}
			return false;
		}
	};

	template <typename T, bool SwapResults>
	struct CollisionCheckDispatcher : public CollisionChecker<T, SwapResults>
	{
		template <typename... Ts>
		CollisionCheckDispatcher(Ts&&... ts) :
			CollisionChecker<T, SwapResults>(std::forward<Ts>(ts)...)
		{}

		// We intentionally don't use a 'Dynamic 1D Sweep and Prune Algorithm' here.
		// O(N*M) is nearly always faster or within a margin of error. Not worth the extra boilerplate code
		void dispatch(ColliderTree* a, ColliderTree* b, std::vector<Aabb*>& listA, std::vector<Aabb*>& listB, const Aabb& refinedBForPruningA)
		{
			CollisionResult result;
			CollisionResult temp;
			bool hasResult = false;

			auto abeg = a->aabb;
			auto bbeg = b->aabb;

			if (listA.size() && listB.size()) {
				for (auto i : listA) {
					if (!i->collideWith(refinedBForPruningA))
						continue;
					for (auto j : listB) {
						if (!i->collideWith(*j))
							continue;
						if (this->checkCollide(&a->cbuf[i - abeg], &b->cbuf[j - bbeg], temp)) {
							if (!hasResult || result.depth > temp.depth) {
								hasResult = true;
								result = temp;
							}
						}
					}
				}
			}

			if (hasResult) {
				this->addResult(result);
			}
		}
	};

	template <typename T, bool SwapResults = false>
	struct CollisionCheckAlgorithm : public CollisionCheckDispatcher<T, SwapResults>
	{
		template <typename... Ts>
		CollisionCheckAlgorithm(Ts&&... ts) :
			CollisionCheckDispatcher<T, SwapResults>(std::forward<Ts>(ts)...)
		{}

		int operator()()
		{
			thread_local std::vector<std::pair<ColliderTree*, ColliderTree*>> pairs;
			pairs.clear();

			if (pairs.capacity() < 256)
				pairs.reserve(256);

			this->c0->checkCollisionL(this->c1, pairs);

			if (pairs.empty())
				return 0;

			// The collision is just too complex to solve. We must quit before we explode the user's computer
			// If this DOES solve, it seems to only cause the collision to become significantly more tangles up
			if (pairs.size() > MaxCollisionPairs) {
				return 0;
			}

			decltype(auto) func = [this](const std::pair<ColliderTree*, ColliderTree*>& pair) {
				if (this->numResults >= SkinnedMeshAlgorithm::MaxCollisionCount)
					return;

				auto a = pair.first, b = pair.second;

				auto abeg = a->aabb;
				auto bbeg = b->aabb;
				auto asize = b->isKinematic ? a->dynCollider : a->numCollider;
				auto bsize = a->isKinematic ? b->dynCollider : b->numCollider;
				auto aend = abeg + asize;
				auto bend = bbeg + bsize;

				Aabb aabbA;
				auto aabbB = b->aabbMe;

				thread_local std::vector<Aabb*> listA;
				thread_local std::vector<Aabb*> listB;

				listA.clear();
				listB.clear();
				listA.reserve(asize);
				listB.reserve(bsize);

				// Colliders in A that intersect full bounding box of B. Compute a new bounding box for just those - this
				// can be MUCH smaller than the original bounding box for A (consider the case where we have two spheres
				// colliding, offset by an equal amount in all three axes).
				for (auto i = abeg; i < aend; ++i) {
					if (i->collideWith(aabbB)) {
						listA.push_back(i);
						aabbA.merge(*i);
					}
				}

				// Colliders in B that intersect the new bounding box for A. Compute a new bounding box for those too.
				if (listA.size()) {
					aabbB.invalidate();
					for (auto i = bbeg; i < bend; ++i) {
						if (i->collideWith(aabbA)) {
							listB.push_back(i);
							aabbB.merge(*i);
						}
					}
				}

				// Now go through both lists and do the real collision (if needed).
				this->dispatch(a, b, listA, listB, aabbB);
			};

			if (pairs.size() >= 32)
				par::for_each(pairs.begin(), pairs.end(), func);
			else
				for (auto& i : pairs) func(i);

			return this->numResults;
		}
	};

	template <class T1>
	int checkCollide(PerVertexShape* a, T1* b, CollisionResult* results)
	{
		return CollisionCheckAlgorithm<T1>(a, b, results)();
	}

	int checkCollide(PerTriangleShape* a, PerVertexShape* b, CollisionResult* results)
	{
		return CollisionCheckAlgorithm<PerTriangleShape, true>(b, a, results)();
	}

	template <class T0, class T1>
	void SkinnedMeshAlgorithm::MergeBuffer::doMerge(T0* a, T1* b, CollisionResult* collision, int count)
	{
		for (int i = 0; i < count; ++i) {
			auto& res = collision[i];
			if (res.depth >= -FLT_EPSILON)
				break;

			auto flexible = std::max(res.colliderA->flexible, res.colliderB->flexible);
			// [3/13/2026]
			// Note: This was using a break before, but logically that doesn't make sense?
			// if we hit a stiffer collider earlier than our depth target, it'd early exit..
			if (flexible < FLT_EPSILON)
				continue;

			float w = flexible * res.depth;
			float w2 = w * w;

			// pre-scale outside the bone loop, these don't depend on bone indices and the inner
			// loop runs bonePerCollider^2 times, so this matters
			auto normScaled = res.normOnB * w * w2;  // cubic weight: bakes depth into normal magnitude
			auto posAScaled = res.posA * w2;
			auto posBScaled = res.posB * w2;

			for (int ib = 0; ib < a->getBonePerCollider(); ++ib) {
				auto w0 = a->getColliderBoneWeight(res.colliderA, ib);
				int boneIdx0 = a->getColliderBoneIndex(res.colliderA, ib);
				if (w0 <= a->m_owner->m_skinnedBones[boneIdx0].weightThreshold)
					continue;

				for (int jb = 0; jb < b->getBonePerCollider(); ++jb) {
					auto w1 = b->getColliderBoneWeight(res.colliderB, jb);
					int boneIdx1 = b->getColliderBoneIndex(res.colliderB, jb);
					if (w1 <= b->m_owner->m_skinnedBones[boneIdx1].weightThreshold)
						continue;

					if (a->m_owner->m_skinnedBones[boneIdx0].isKinematic && b->m_owner->m_skinnedBones[boneIdx1].isKinematic)
						continue;

					auto c = getAndTrack(boneIdx0, boneIdx1);

					// If we already have a primary direction (weight > 0),
					// and this new contact pushes in the opposite direction (dot < 0),
					// reject it entirely. This prevents the vector cancellation that creates
					// unpredictable movement, and preserves the depth/weight ratio
					// [If we get jitter, try removing this]
					if (c->weight > FLT_EPSILON && c->normal.dot(normScaled) < 0) {
						continue;
					}

					c->weight += w2;
					c->normal += normScaled;
					c->pos[0] += posAScaled;
					c->pos[1] += posBScaled;
				}
			}
		}
	}

	void SkinnedMeshAlgorithm::MergeBuffer::apply(SkinnedMeshBody* body0, SkinnedMeshBody* body1,
		CollisionDispatcher* dispatcher)
	{
		// only visit cells that were actually written to this frame,
		// instead of looping all bones0 * bones1 (far fewer iterations)
		for (int flatIdx : activeCells) {
			int i = flatIdx / mergeStride;
			int j = flatIdx % mergeStride;

			auto* c = &buffer[flatIdx];
			if (c->weight < FLT_EPSILON)
				continue;

			if (!body1->canCollideWith(body0->m_skinnedBones[i].ptr))
				continue;
			if (!body0->canCollideWith(body1->m_skinnedBones[j].ptr))
				continue;
			if (body0->m_skinnedBones[i].isKinematic && body1->m_skinnedBones[j].isKinematic)
				continue;

			auto rb0 = body0->m_skinnedBones[i].ptr;
			auto rb1 = body1->m_skinnedBones[j].ptr;
			if (rb0 == rb1)
				continue;

			float invWeight = 1.0f / c->weight;

			auto worldA = c->pos[0] * invWeight;
			auto worldB = c->pos[1] * invWeight;
			auto localA = rb0->m_rig.getWorldTransform().invXform(worldA);
			auto localB = rb1->m_rig.getWorldTransform().invXform(worldB);
			auto normal = c->normal * invWeight;
			if (normal.fuzzyZero())
				continue;

			// depth was baked into normal magnitude during doMerge (weighted cubically instead of storing a separate depth field)
			auto depth = -normal.length();
			normal = -normal.normalized();

			if (depth >= -FLT_EPSILON)
				continue;
			btManifoldPoint newPt(localA, localB, normal, depth);
			newPt.m_positionWorldOnA = worldA;
			newPt.m_positionWorldOnB = worldB;
			newPt.m_combinedFriction = rb0->m_rig.getFriction() * rb1->m_rig.getFriction();
			newPt.m_combinedRestitution = rb0->m_rig.getRestitution() * rb1->m_rig.getRestitution();
			newPt.m_combinedRollingFriction = rb0->m_rig.getRollingFriction() * rb1->m_rig.getRollingFriction();

			auto maniford = dispatcher->getNewManifold(&rb0->m_rig, &rb1->m_rig);
			maniford->addManifoldPoint(newPt);
		}
	}

	template <class T0, class T1>
	void SkinnedMeshAlgorithm::processCollision(T0* shape0, T1* shape1, MergeBuffer& merge, CollisionResult* collision)
	{
		int count = std::min(checkCollide(shape0, shape1, collision), MaxCollisionCount);
		if (count > 0) {
			// results come back in random order from parallel workers, sort so doMerge's
			// early break actually bails on shallow contacts instead of random ones
			std::sort(collision, collision + count, [](const CollisionResult& a, const CollisionResult& b) {
				return a.depth < b.depth;
			});
			merge.doMerge(shape0, shape1, collision, count);
		}
	}

	void SkinnedMeshAlgorithm::processCollision(SkinnedMeshBody* body0, SkinnedMeshBody* body1,
		CollisionDispatcher* dispatcher)
	{
		// thread_local so we don't heap-alloc these 200+ times per frame.
		// MergeBuffer::resize() is O(1) after first call (generation counter, no zeroing).
		// The port runs collision dispatch serially, so there is no
		// work-stealing re-entrancy to guard against; review this if
		// parallelism is ever reintroduced.
		thread_local MergeBuffer merge;
		thread_local auto collision = std::make_unique<CollisionResult[]>(MaxCollisionCount);

		merge.resize(static_cast<int>(body0->m_skinnedBones.size()), static_cast<int>(body1->m_skinnedBones.size()));

		if (body0->m_shape->asPerTriangleShape() && body1->m_shape->asPerTriangleShape()) {
			// Todo: This can actually be further optimized, but would need a re-factor.. However, would the performance increase be worth
			// the extra boilerplate code..?
			processCollision(body0->m_shape->asPerTriangleShape(), body1->m_shape->asPerVertexShape(), merge,
				collision.get());
			processCollision(body0->m_shape->asPerVertexShape(), body1->m_shape->asPerTriangleShape(), merge,
				collision.get());
		} else if (body0->m_shape->asPerTriangleShape())
			processCollision(body0->m_shape->asPerTriangleShape(), body1->m_shape->asPerVertexShape(), merge,
				collision.get());
		else if (body1->m_shape->asPerTriangleShape())
			processCollision(body0->m_shape->asPerVertexShape(), body1->m_shape->asPerTriangleShape(), merge,
				collision.get());
		else
			processCollision(body0->m_shape->asPerVertexShape(), body1->m_shape->asPerVertexShape(), merge, collision.get());

		merge.apply(body0, body1, dispatcher);
	}
}

#endif  // USE_BULLET
