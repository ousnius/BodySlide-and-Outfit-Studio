#ifdef USE_BULLET

#include "hdtSkinnedMeshShape.h"


namespace hdt
{
	SkinnedMeshShape::SkinnedMeshShape(SkinnedMeshBody* body)
	{
		m_owner = body;
		m_owner->m_shape = hdt::make_ref(this);
	}

	SkinnedMeshShape::~SkinnedMeshShape()
	{
		//m_aabbGridLink.discard_data();
		//m_aabbGridBuffer.discard_data();
	}

	void SkinnedMeshShape::clipColliders()
	{
		m_tree.clipCollider([&, this](const Collider& n) -> bool {
			bool flg = false;
			for (int i = 0; i < getBonePerCollider() && !flg; ++i) {
				float weight = getColliderBoneWeight(&n, i);
				if (weight > FLT_EPSILON && weight > m_owner->m_skinnedBones[getColliderBoneIndex(&n, i)].weightThreshold)
					flg = true;
			}
			return !flg;
		});
	}

	PerVertexShape::PerVertexShape(SkinnedMeshBody* body) :
		SkinnedMeshShape(body)
	{
	}

	PerVertexShape::~PerVertexShape()
	{
	}

	void PerVertexShape::finishBuild()
	{
		m_tree.optimize();
		m_tree.updateKinematic([this](const Collider* n) {
			return m_owner->flexible(m_owner->m_vertices[n->vertex]);
		});

		m_owner->setCollisionFlags(m_tree.isKinematic ? btCollisionObject::CF_KINEMATIC_OBJECT : 0);

		m_tree.exportColliders(m_colliders);
		m_aabb.resize(m_colliders.size());
		m_tree.remapColliders(m_colliders.data(), m_aabb.data());
	}

	void PerVertexShape::internalUpdate()
	{
		const VertexPos* __restrict vertices = m_owner->m_vpos.data();
		const Collider* __restrict colliders = m_colliders.data();
		Aabb* __restrict aabbs = m_aabb.data();
		const size_t size = m_colliders.size();

		const __m128 marginFactor = _mm_set1_ps(m_shapeProp.margin);

		for (size_t i = 0; i < size; ++i) {
			__m128 p0 = vertices[colliders[i].vertex].m_data;

			// Broadcast the W component and multiply by margin factor
			__m128 bcast = _mm_shuffle_ps(p0, p0, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 margin = _mm_mul_ps(bcast, marginFactor);

			// Force 16-byte aligned vector stores to prevent MSVC scalar assignment fallback
			_mm_store_ps(reinterpret_cast<float*>(&aabbs[i].m_min), _mm_sub_ps(p0, margin));
			_mm_store_ps(reinterpret_cast<float*>(&aabbs[i].m_max), _mm_add_ps(p0, margin));
		}

		m_tree.updateAabb();
	}

	void PerVertexShape::autoGen()
	{
		m_tree.children.clear();
		std::vector<U32> keys;
		for (U32 i = 0; i < m_owner->m_vertices.size(); ++i) {
			keys.clear();
			for (int j = 0; j < 4; ++j) {
				if (m_owner->m_vertices[i].m_weight[j] > FLT_EPSILON)
					keys.push_back(m_owner->m_vertices[i].getBoneIdx(j));
			}
			m_tree.insertCollider(keys.data(), keys.size(), Collider(i));
		}
	}

	void PerVertexShape::markUsedVertices(bool* flags)
	{
		for (auto& i : m_colliders)
			flags[i.vertex] = true;
	}

	void PerVertexShape::remapVertices(uint32_t* map)
	{
		for (auto& i : m_colliders)
			i.vertex = map[i.vertex];
	}

	PerTriangleShape::PerTriangleShape(SkinnedMeshBody* body) :
		SkinnedMeshShape(body)
	{
	}

	PerTriangleShape::~PerTriangleShape()
	{
	}

	// Note: Don't waste your time trying to optimize this...
	// 1: The compiler auto-vertorizes, unrolls, and broadcasts W already (Very sensitive to changes)
	// 2: Memory wall is the main issue
	// 3: AVX2 would just have overhead
	void PerTriangleShape::internalUpdate()
	{
		auto& vertices = m_owner->m_vpos;

		size_t size = m_colliders.size();
		for (size_t i = 0; i < size; ++i) {
			auto c = &m_colliders[i];
			auto p0 = vertices[c->vertices[0]].m_data;
			auto p1 = vertices[c->vertices[1]].m_data;
			auto p2 = vertices[c->vertices[2]].m_data;
			auto margin4 = p0 + p1 + p2;

			auto aabbMin = _mm_min_ps(_mm_min_ps(p0, p1), p2);
			auto aabbMax = _mm_max_ps(_mm_max_ps(p0, p1), p2);
			auto prenetration = _mm_set_ss(m_shapeProp.penetration);
			prenetration = _mm_andnot_ps(_mm_set_ss(-0.0f), prenetration);  // abs
			margin4 = _mm_max_ss(_mm_set_ss(getLane3(margin4) * m_shapeProp.margin / 3), prenetration);
			margin4 = _mm_shuffle_ps(margin4, margin4, 0);
			aabbMin = aabbMin - margin4;
			aabbMax = aabbMax + margin4;

			m_aabb[i].m_min = aabbMin;
			m_aabb[i].m_max = aabbMax;
		}

		m_tree.updateAabb();
	}

	void PerTriangleShape::finishBuild()
	{
		m_tree.optimize();
		m_tree.updateKinematic([=](const Collider* c) {
			float k = m_owner->flexible(m_owner->m_vertices[c->vertices[0]]);
			k += m_owner->flexible(m_owner->m_vertices[c->vertices[1]]);
			k += m_owner->flexible(m_owner->m_vertices[c->vertices[2]]);
			return k / 3;
		});

		m_owner->setCollisionFlags(m_tree.isKinematic ? btCollisionObject::CF_KINEMATIC_OBJECT : 0);

		m_tree.exportColliders(m_colliders);
		m_aabb.resize(m_colliders.size());
		m_tree.remapColliders(m_colliders.data(), m_aabb.data());

		Ref<PerTriangleShape> holder = hdt::make_ref(this);
		m_verticesCollision = make_ref(new PerVertexShape(m_owner));
		m_verticesCollision->m_shapeProp.margin = m_shapeProp.margin;
		m_owner->m_shape = hdt::make_ref(this);

		m_verticesCollision->autoGen();
		m_verticesCollision->clipColliders();
		m_verticesCollision->finishBuild();
	}

	void PerTriangleShape::markUsedVertices(bool* flags)
	{
		for (auto& i : m_colliders) {
			flags[i.vertices[0]] = true;
			flags[i.vertices[1]] = true;
			flags[i.vertices[2]] = true;
		}

		m_verticesCollision->markUsedVertices(flags);
	}

	void PerTriangleShape::remapVertices(uint32_t* map)
	{
		for (auto& i : m_colliders) {
			i.vertices[0] = map[i.vertices[0]];
			i.vertices[1] = map[i.vertices[1]];
			i.vertices[2] = map[i.vertices[2]];
		}

		m_verticesCollision->remapVertices(map);
	}

	void PerTriangleShape::addTriangle(int a, int b, int c)
	{
		assert(a < m_owner->m_vertices.size());
		assert(b < m_owner->m_vertices.size());
		assert(c < m_owner->m_vertices.size());
		Collider collider(a, b, c);

		// Stacklocal fixed arrays, max 12 unique bones (3 verts * 4 weights)
		U32 keys[12];
		float w[12];
		int count = 0;

		const auto& v0 = m_owner->m_vertices[a];
		const auto& v1 = m_owner->m_vertices[b];
		const auto& v2 = m_owner->m_vertices[c];
		const Vertex* verts[3] = { &v0, &v1, &v2 };

		for (int vi = 0; vi < 3; ++vi) {
			for (int wi = 0; wi < 4; ++wi) {
				float weight = verts[vi]->m_weight[wi];
				if (weight < FLT_EPSILON)
					continue;
				U32 bone = verts[vi]->getBoneIdx(wi);

				int found = -1;
				for (int k = 0; k < count; ++k) {
					if (keys[k] == bone) {
						found = k;
						break;
					}
				}
				if (found >= 0) {
					w[found] += weight;
				} else {
					keys[count] = bone;
					w[count] = weight;
					++count;
				}
			}
		}

		for (int i = 1; i < count; ++i) {
			float wTemp = w[i];
			U32 kTemp = keys[i];
			int j = i;
			while (j > 0 && (w[j - 1] < wTemp || (w[j - 1] == wTemp && keys[j - 1] > kTemp))) {
				w[j] = w[j - 1];
				keys[j] = keys[j - 1];
				--j;
			}
			w[j] = wTemp;
			keys[j] = kTemp;
		}

		m_tree.insertCollider(keys, count, collider);
	}
}

#endif  // USE_BULLET
