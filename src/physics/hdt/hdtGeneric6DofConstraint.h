#pragma once

#include "hdtBoneScaleConstraint.h"
#include "hdtBulletHelper.h"

namespace hdt
{
	class Generic6DofConstraint :
		public BoneScaleConstraint,
		public btGeneric6DofSpring2Constraint
	{
	public:
		BT_DECLARE_ALIGNED_ALLOCATOR();

		Generic6DofConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, const btTransform& frameInA, const btTransform& frameInB);
		virtual ~Generic6DofConstraint() override = default;

		void scaleConstraint() override;

	private:
	};
}
