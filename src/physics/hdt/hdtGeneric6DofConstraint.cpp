#ifdef USE_BULLET

#include "hdtGeneric6DofConstraint.h"

namespace hdt
{
	Generic6DofConstraint::Generic6DofConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b, const btTransform& frameInA, const btTransform& frameInB) :
		BoneScaleConstraint(a, b, static_cast<btTypedConstraint*>(this)),
		btGeneric6DofSpring2Constraint(a->m_rig, b->m_rig, btTransform::getIdentity(), btTransform::getIdentity(), RO_XYZ)
	{
		auto fa = a->m_rigToLocal * frameInA;
		auto fb = b->m_rigToLocal * frameInB;
		setFrames(fa, fb);

		for (int i = 0; i < 6; ++i)
			enableSpring(i, true);

		//m_linearLimits.m_stopERP.setValue(0.1f, 0.1f, 0.1f);

		//m_oldLinearDiff.setZero();
		//m_oldAngularDiff.setZero();
		//m_linearBounce.setZero();
	}

	void Generic6DofConstraint::scaleConstraint()
	{
		auto newScaleA = m_boneA->m_currentTransform.getScale();
		auto newScaleB = m_boneB->m_currentTransform.getScale();

		if (btFuzzyZero(newScaleA - m_scaleA) && btFuzzyZero(newScaleB - m_scaleB))
			return;

		float w0 = m_boneA->m_rig.getInvMass();
		float w1 = m_boneB->m_rig.getInvMass();
		auto factorA = newScaleA / m_scaleA;
		auto factorB = newScaleB / m_scaleB;
		auto factor = (factorA * w0 + factorB * w1) / (w0 + w1);
		auto factor2 = factor * factor;
		auto factor3 = factor2 * factor;
		auto factor5 = factor3 * factor2;

		getFrameOffsetA().setOrigin(getFrameOffsetA().getOrigin() * factorA);
		getFrameOffsetB().setOrigin(getFrameOffsetB().getOrigin() * factorB);
		m_linearLimits.m_equilibriumPoint *= factor;
		// target k = ma / x(kg/s^2)
		m_linearLimits.m_springStiffness *= factor3;
		m_linearLimits.m_upperLimit *= factor;
		m_linearLimits.m_lowerLimit *= factor;
		for (int i = 0; i < 3; ++i) {
			m_angularLimits[i].m_springStiffness *= factor5;
		}

		m_scaleA = newScaleA;
		m_scaleB = newScaleB;
	}
}

#endif  // USE_BULLET
