#ifdef USE_BULLET

#include "hdtBoneScaleConstraint.h"

namespace hdt
{
	BoneScaleConstraint::BoneScaleConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, btTypedConstraint* constraint) :
		m_scaleA(1), m_scaleB(1), m_boneA(a), m_boneB(b), m_constraint(constraint)
	{
	}

	BoneScaleConstraint::~BoneScaleConstraint()
	{
	}
}

#endif  // USE_BULLET
