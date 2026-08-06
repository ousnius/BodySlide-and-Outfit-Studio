#pragma once

#include "hdtBulletHelper.h"
#include "hdtSkinnedMeshBody.h"

namespace hdt
{
	class alignas(16) BoneScaleConstraint : public RefObject
	{
	public:
		BoneScaleConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, btTypedConstraint* constraint);
		virtual ~BoneScaleConstraint();

		virtual void scaleConstraint() = 0;

		btTypedConstraint* getConstraint() const { return m_constraint; }

		float m_scaleA, m_scaleB;

		SkinnedMeshBone* m_boneA;
		SkinnedMeshBone* m_boneB;
		btTypedConstraint* m_constraint;
	};
}
