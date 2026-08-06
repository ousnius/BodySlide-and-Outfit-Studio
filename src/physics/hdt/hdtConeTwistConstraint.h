#pragma once
#include "hdtBoneScaleConstraint.h"

namespace hdt
{
	class ConeTwistConstraint :
		public BoneScaleConstraint,
		public btConeTwistConstraint
	{
	public:
		BT_DECLARE_ALIGNED_ALLOCATOR();

		ConeTwistConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, const btTransform& frameInA, const btTransform& frameInB);
		virtual ~ConeTwistConstraint() override = default;

		void scaleConstraint() override;
	};
}
