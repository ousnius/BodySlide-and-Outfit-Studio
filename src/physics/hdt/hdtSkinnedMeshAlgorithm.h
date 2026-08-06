#pragma once

#include "hdtDispatcher.h"
#include "hdtSkinnedMeshShape.h"

namespace hdt
{
	struct CollisionResult
	{
		btVector3 posA;
		btVector3 posB;
		btVector3 normOnB;
		Collider* colliderA;
		Collider* colliderB;
		float depth;
	};

	class SkinnedMeshAlgorithm
	{
	public:
		// Note: It's possible to exceed this with complex outfits, which is why we cap it.
		// We don't want to stress a simulation island too much!
		static const int MaxCollisionCount = 512;

		static void processCollision(SkinnedMeshBody* body0Wrap, SkinnedMeshBody* body1Wrap,
			CollisionDispatcher* dispatcher);

	protected:
		struct CollisionMerge
		{
			btVector3 normal;  // accumulated weighted normal: length encodes depth, direction encodes contact normal
			btVector3 pos[2];
			float weight;

			CollisionMerge()
			{
				_mm_store_ps(((float*)this), _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 4, _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 8, _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 12, _mm_setzero_ps());
			}

			void reset()
			{
				_mm_store_ps(((float*)this), _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 4, _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 8, _mm_setzero_ps());
				_mm_store_ps(((float*)this) + 12, _mm_setzero_ps());
			}
		};

		struct MergeBuffer
		{
			MergeBuffer() :
				mergeStride(0), mergeSize(0), currentGen(0)
			{
				activeCells.reserve(256);
			}

			~MergeBuffer()
			{
				std::free(buffer);
				std::free(generations);
			}

			MergeBuffer(const MergeBuffer&) = delete;
			MergeBuffer& operator=(const MergeBuffer&) = delete;

			// Just in case!
			MergeBuffer(MergeBuffer&&) = delete;
			MergeBuffer& operator=(MergeBuffer&&) = delete;

			// Buffer is ~2-10% occupied in basic scenes, so instead of zeroing old cells every resize() call we just bump a generation counter.
			// cells get lazily reset on first touch in getAndTrack(). Makes resize() O(1).
			void resize(int x, int y)
			{
				mergeStride = y;
				int needed = x * y;
				if (needed > mergeSize) {
					std::free(buffer);
					std::free(generations);
					mergeSize = needed;
					buffer = static_cast<CollisionMerge*>(std::malloc(needed * sizeof(CollisionMerge)));
					generations = static_cast<uint32_t*>(std::calloc(needed, sizeof(uint32_t)));
				}
				if (++currentGen == 0) {
					// wrap around, shouldn't realistically happen (~4 billion frames lol)
					// This is virtually skipped entirely by the cpu, 0 cost. Just in case since it would
					// create difficult to track down inconsistencies..
					std::memset(generations, 0, mergeSize * sizeof(uint32_t));
					currentGen = 1;
				}
				activeCells.clear();
			}

			CollisionMerge* get(int x, int y) { return &buffer[x * mergeStride + y]; }

			CollisionMerge* getAndTrack(int x, int y)
			{
				int idx = x * mergeStride + y;
				auto* c = &buffer[idx];
				if (generations[idx] != currentGen) {
					c->reset();
					generations[idx] = currentGen;
					activeCells.push_back(idx);
				}
				return c;
			}

			template <class T0, class T1>
			void doMerge(T0* shape0, T1* shape1, CollisionResult* collisions, int count);

			void apply(SkinnedMeshBody* body0, SkinnedMeshBody* body1, CollisionDispatcher* dispatcher);

			int mergeStride;
			int mergeSize;
			uint32_t currentGen;
			CollisionMerge* buffer = nullptr;
			uint32_t* generations = nullptr;
			std::vector<int> activeCells;
		};

		template <class T0, class T1>
		static void processCollision(T0* shape0, T1* shape1, MergeBuffer& merge, CollisionResult* collision);
	};
}
