#pragma once

#include "hdtBone.h"

#include <cstring>

namespace hdt
{
	struct alignas(16) Vertex
	{
		Vertex()
		{
			memset(this, 0, sizeof(*this));
		}

		Vertex(float x, float y, float z) :
			Vertex() { m_skinPos.setValue(x, y, z); }

		// skin info;
		btVector3 m_skinPos;
		float m_weight[4];
		U32 m_boneIdx[4];

		U32 getBoneIdx(int i) const { return m_boneIdx[i]; }

		void setBoneIdx(int i, U32 idx)
		{
			m_boneIdx[i] = idx;
		}

		void sortWeight();
	};

	struct alignas(16) VertexPos
	{
		// position info

		void set(const btVector3& p, float m)
		{
			m_data = setLane3(p.get128(), m);
		}

		void set(const btVector4& pm)
		{
			m_data = pm.get128();
		}

		btVector3 pos() const { return m_data; }
		__m128 marginMultiplier4() const { return pshufd<0xFF>(m_data); }
		float marginMultiplier() const { return _mm_cvtss_f32(marginMultiplier4()); }

		__m128 m_data;
	};
}
