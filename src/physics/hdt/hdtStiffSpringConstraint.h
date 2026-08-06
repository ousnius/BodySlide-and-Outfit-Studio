#pragma once

#include "hdtBoneScaleConstraint.h"
#include "hdtBulletHelper.h"

namespace hdt
{
	class StiffSpringConstraint :
		public BoneScaleConstraint,
		public btTypedConstraint
	{
	public:
		BT_DECLARE_ALIGNED_ALLOCATOR();

		StiffSpringConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b);
		virtual ~StiffSpringConstraint() override = default;

		void scaleConstraint() override;

		float m_minDistance;
		float m_maxDistance;
		float m_stiffness;
		float m_damping;
		float m_equilibriumPoint;

	protected:
		float m_oldDiff;

		///internal method used by the constraint solver, don't use them directly
		void getInfo1(btConstraintInfo1* info) override;
		///internal method used by the constraint solver, don't use them directly
		void getInfo2(btConstraintInfo2* info) override;

		///override the default global value of a parameter (such as ERP or CFM), optionally provide the axis (0..5).
		///If no axis is provided, it uses the default axis for this constraint.
		void setParam([[maybe_unused]] int num, [[maybe_unused]] btScalar value, [[maybe_unused]] int axis = -1) override {
		};

		///return the local value of parameter
		btScalar getParam([[maybe_unused]] int num, [[maybe_unused]] int axis = -1) const override { return 0; };
	};
}
