#pragma once
#include "hdtBoneScaleConstraint.h"

namespace hdt
{
	class ConstraintGroup : public RefObject
	{
	public:
		void scaleConstraint()
		{
			for (auto& i : m_constraints)
				i->scaleConstraint();
		}
		std::vector<Ref<BoneScaleConstraint>> m_constraints;
	};
}
