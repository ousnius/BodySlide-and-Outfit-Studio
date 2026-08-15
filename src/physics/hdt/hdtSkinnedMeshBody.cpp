#ifdef USE_BULLET

#include "hdtSkinnedMeshBody.h"
#include "hdtSkinnedMeshShape.h"


namespace hdt
{

	SkinnedMeshBody::SkinnedMeshBody()
	{
		m_collisionShape = &m_bulletShape;
	}

	SkinnedMeshBody::~SkinnedMeshBody()
	{
		//m_bonesGPU.discard_data();
		//m_verticesGPU.discard_data();
	}

	HDT_FORCEINLINE __m128 calcVertexState(__m128 skinPos, const Bone& bone, __m128 w)
	{
		auto p = bone.m_vertexToWorld * fromSimd(skinPos);
		auto pm = _mm_blend_ps(toSimd(p), _mm_load_ps(bone.m_reserved), 0x8);
		return _mm_mul_ps(w, pm);
	}

#if defined(__AVX2__)
	HDT_FORCEINLINE __m128 calcVertexStateFMA(__m128 skinPos, const Bone& bone, __m128 w)
	{
		__m128 px = pshufd<0x00>(skinPos);
		__m128 py = pshufd<0x55>(skinPos);
		__m128 pz = pshufd<0xAA>(skinPos);
		__m128 r = _mm_fmadd_ps(toSimd(bone.m_vertexToWorld.m_col[2]), pz, toSimd(bone.m_vertexToWorld.m_col[3]));
		r = _mm_fmadd_ps(toSimd(bone.m_vertexToWorld.m_col[1]), py, r);
		r = _mm_fmadd_ps(toSimd(bone.m_vertexToWorld.m_col[0]), px, r);
		r = _mm_blend_ps(r, _mm_load_ps(bone.m_reserved), 0x8);
		return _mm_mul_ps(w, r);
	}
#endif

	void SkinnedMeshBody::internalUpdate()
	{
		const size_t numBones = m_skinnedBones.size();
		const auto* __restrict skinnedBones = m_skinnedBones.data();
		auto* __restrict bonesDst = m_bones.data();

		for (size_t i = 0; i < numBones; ++i) {
			if (i + 8 < numBones)
				_mm_prefetch((const char*)&skinnedBones[i + 8], _MM_HINT_T1);
			if (i + 4 < numBones)
				_mm_prefetch((const char*)skinnedBones[i + 4].ptr, _MM_HINT_T0);
			auto& v = skinnedBones[i];
			auto boneT = v.ptr->m_currentTransform;
			bonesDst[i].m_vertexToWorld = btMatrix4x3T(boneT) * v.vertexToBone;
			bonesDst[i].m_maginMultipler = v.ptr->m_marginMultipler * boneT.getScale();
		}

		const int size = static_cast<int>(m_vpos.size());
		const Vertex* __restrict verts = m_vertices.data();
		VertexPos* __restrict vpos = m_vpos.data();
		const Bone* __restrict bones = bonesDst;

		// We can use AVX2 here due to sequential memory reads..
#if defined(__AVX2__)

		constexpr int PF = 6;
		int idx = 0;
		for (; idx + 1 < size; idx += 2) {
			if (idx + PF < size) {
				_mm_prefetch((const char*)&verts[idx + PF], _MM_HINT_T0);
				_mm_prefetch((const char*)&verts[idx + PF + 1], _MM_HINT_T0);
			}

			{
				auto& v = verts[idx];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexStateFMA(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx].set(pm);
			}
			{
				auto& v = verts[idx + 1];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexStateFMA(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexStateFMA(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx + 1].set(pm);
			}
		}
		for (; idx < size; ++idx) {
			auto& v = verts[idx];
			auto p = toSimd(v.m_skinPos);
			auto w = _mm_load_ps(v.m_weight);
			auto pm = calcVertexStateFMA(p, bones[v.getBoneIdx(0)], setAll0(w));
			pm += calcVertexStateFMA(p, bones[v.getBoneIdx(1)], setAll1(w));
			pm += calcVertexStateFMA(p, bones[v.getBoneIdx(2)], setAll2(w));
			pm += calcVertexStateFMA(p, bones[v.getBoneIdx(3)], setAll3(w));
			vpos[idx].set(pm);
		}

#else

		constexpr int PF = 8;
		int idx = 0;
		for (; idx + 3 < size; idx += 4) {
			if (idx + PF + 3 < size) {
				_mm_prefetch((const char*)&verts[idx + PF], _MM_HINT_T0);
				_mm_prefetch((const char*)&verts[idx + PF + 1], _MM_HINT_T0);
				_mm_prefetch((const char*)&verts[idx + PF + 2], _MM_HINT_T0);
				_mm_prefetch((const char*)&verts[idx + PF + 3], _MM_HINT_T0);
			}

			{
				auto& v = verts[idx];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexState(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx].set(pm);
			}
			{
				auto& v = verts[idx + 1];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexState(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx + 1].set(pm);
			}
			{
				auto& v = verts[idx + 2];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexState(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx + 2].set(pm);
			}
			{
				auto& v = verts[idx + 3];
				auto p = toSimd(v.m_skinPos);
				auto w = _mm_load_ps(v.m_weight);
				auto pm = calcVertexState(p, bones[v.getBoneIdx(0)], setAll0(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(1)], setAll1(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(2)], setAll2(w));
				pm += calcVertexState(p, bones[v.getBoneIdx(3)], setAll3(w));
				vpos[idx + 3].set(pm);
			}
		}
		for (; idx < size; ++idx) {
			auto& v = verts[idx];
			auto p = toSimd(v.m_skinPos);
			auto w = _mm_load_ps(v.m_weight);
			auto pm = calcVertexState(p, bones[v.getBoneIdx(0)], setAll0(w));
			pm += calcVertexState(p, bones[v.getBoneIdx(1)], setAll1(w));
			pm += calcVertexState(p, bones[v.getBoneIdx(2)], setAll2(w));
			pm += calcVertexState(p, bones[v.getBoneIdx(3)], setAll3(w));
			vpos[idx].set(pm);
		}

#endif

		m_shape->internalUpdate();
		m_bulletShape.m_aabb = m_shape->m_tree.aabbAll;
	}

	float SkinnedMeshBody::flexible(const Vertex& v)
	{
		float ret = 0.f;
		for (int i = 0; i < 4; ++i) {
			if (v.m_weight[i] < FLT_EPSILON)
				break;
			int boneIdx = v.getBoneIdx(i);
			if (!m_skinnedBones[boneIdx].isKinematic)
				ret += v.m_weight[i];
		}
		return ret;
	}

	int SkinnedMeshBody::addBone(SkinnedMeshBone* bone, const btQsTransform& verticesToBone,
		const BoundingSphere& boundingSphere)
	{
		m_skinnedBones.push_back(SkinnedBone());
		auto& v = m_skinnedBones.back();
		v.ptr = bone;
		v.vertexToBone = verticesToBone;
		v.localBoundingSphere = boundingSphere;
		v.isKinematic = bone->m_rig.isStaticOrKinematicObject();
		return static_cast<int>(m_skinnedBones.size() - 1);
	}

	void SkinnedMeshBody::finishBuild()
	{
		m_bones.resize(m_skinnedBones.size());
		m_shape->clipColliders();
		m_vpos.resize(m_vertices.size());

		m_isKinematic = true;
		for (auto& i : m_skinnedBones) {
			i.isKinematic = i.ptr->m_rig.isStaticOrKinematicObject();
			if (!i.isKinematic)
				m_isKinematic = false;
		}

		m_shape->finishBuild();

		bool* flags = new bool[m_vertices.size()];
		memset(flags, 0, m_vertices.size());
		m_shape->markUsedVertices(flags);

		uint32_t numUsed = 0;
		std::vector<uint32_t> map(m_vertices.size());
		for (size_t i = 0; i < m_vertices.size(); ++i) {
			if (flags[i]) {
				m_vertices[numUsed] = m_vertices[i];
				m_vpos[numUsed] = m_vpos[i];
				map[i] = numUsed++;
			}
		}
		delete[] flags;
		m_shape->remapVertices(map.data());
		m_vertices.resize(numUsed);
		m_vpos.resize(numUsed);

		m_useBoundingSphere = m_shape->m_colliders.size() > 10;
	}

	bool SkinnedMeshBody::canCollideWith(const SkinnedMeshBody* body) const
	{
		if (m_isKinematic && body->m_isKinematic)
			return false;

		if (m_canCollideWithTags.empty()) {
			for (auto& i : body->m_tags)
				if (m_noCollideWithTags.find(i) != m_noCollideWithTags.end())
					return false;
			return true;
		}
		for (auto& i : body->m_tags)
			if (m_canCollideWithTags.find(i) != m_canCollideWithTags.end())
				return true;
		return false;
	}

	void SkinnedMeshBody::updateBoundingSphereAabb()
	{
		m_bulletShape.m_aabb.invalidate();
		for (auto& i : m_skinnedBones) {
			auto sp = i.localBoundingSphere;
			auto tr = i.ptr->m_currentTransform;
			i.worldBoundingSphere = BoundingSphere(tr * sp.center(), tr.getScale() * sp.radius());
			m_bulletShape.m_aabb.merge(i.worldBoundingSphere.getAabb());
		}

		if (!m_useBoundingSphere)
			internalUpdate();
	}

	bool SkinnedMeshBody::isBoundingSphereCollided(SkinnedMeshBody* rhs)
	{
		if (canCollideWith(rhs) && rhs->canCollideWith(this)) {
			return true;
		}
		return false;
	}
}

#endif  // USE_BULLET
