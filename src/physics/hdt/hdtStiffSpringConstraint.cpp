#ifdef USE_BULLET

#include "hdtStiffSpringConstraint.h"

namespace hdt
{
	StiffSpringConstraint::StiffSpringConstraint(SkinnedMeshBone* a, SkinnedMeshBone* b) :
		BoneScaleConstraint(a, b, this), btTypedConstraint(MAX_CONSTRAINT_TYPE, a->m_rig, b->m_rig)
	{
		auto distance = a->m_currentTransform.getOrigin().distance(b->m_currentTransform.getOrigin());
		m_equilibriumPoint = m_minDistance = m_maxDistance = distance;
		m_oldDiff = 0;
		m_stiffness = 0;
		m_damping = 0;
		// m_minDistance *= 0.8;
		// m_maxDistance /= 0.8;
	}

	void StiffSpringConstraint::scaleConstraint()
	{
		auto newScaleA = m_boneA->m_currentTransform.getScale();
		auto newScaleB = m_boneB->m_currentTransform.getScale();

		if (btFuzzyZero(newScaleA - m_scaleA) && btFuzzyZero(newScaleB - m_scaleB))
			return;

		float w0 = m_boneA->m_rig.getInvMass();
		float w1 = m_boneB->m_rig.getInvMass();
		auto factor = (newScaleA / m_scaleA * w0 + newScaleB / m_scaleB * w1) / (w0 + w1);
		auto factor3 = factor * factor * factor;

		m_minDistance *= factor;
		m_maxDistance *= factor;
		m_equilibriumPoint *= factor;
		m_stiffness *= factor3;
		m_damping *= factor3;

		m_scaleA = newScaleA;
		m_scaleB = newScaleB;
	}

	void StiffSpringConstraint::getInfo1(btConstraintInfo1* info)
	{
		auto a = m_boneA->m_rigToLocal * m_rbA.getWorldTransform();
		auto b = m_boneB->m_rigToLocal * m_rbB.getWorldTransform();
		auto a2b = a.getOrigin() - b.getOrigin();
		auto distance = a2b.length();

		info->m_numConstraintRows = !btFuzzyZero(distance);
		info->nub = 0;
	}

	void StiffSpringConstraint::getInfo2(btConstraintInfo2* info)
	{
		auto a = m_boneA->m_rigToLocal * m_rbA.getWorldTransform();
		auto b = m_boneB->m_rigToLocal * m_rbB.getWorldTransform();
		auto a2b = a.getOrigin() - b.getOrigin();
		auto distance = a2b.length();
		auto dir = a2b.normalized();

		info->m_J1linearAxis[0] = dir[0];
		info->m_J1linearAxis[1] = dir[1];
		info->m_J1linearAxis[2] = dir[2];
		info->m_J2linearAxis[0] = -dir[0];
		info->m_J2linearAxis[1] = -dir[1];
		info->m_J2linearAxis[2] = -dir[2];

		int currentLimit;
		float currentLimitError;
		if (distance < m_minDistance) {
			currentLimit = 2;
			currentLimitError = distance - m_minDistance;
		} else if (distance > m_maxDistance) {
			currentLimit = 1;
			currentLimitError = distance - m_maxDistance;
		} else {
			currentLimit = 0;
			currentLimitError = 0;
		}

		if (!currentLimit) {
			// get current position of constraint
			auto delta = distance - m_equilibriumPoint;
			auto vel = (delta - m_oldDiff) * info->fps;

			// spring force is (delta * m_stiffness) according to Hooke's Law
			btScalar force = (delta + m_oldDiff) * 0.5f * m_stiffness;
			btScalar friction = m_damping * vel;
			force += force * friction < 0 ? btClamped(friction, -abs(force), abs(force)) : friction;
			btScalar velFactor = info->fps / btScalar(info->m_numIterations);
			auto m_targetVelocity = velFactor * force;
			auto m_maxMotorForce = btFabs(force) / info->fps;

			btScalar mot_fact = getMotorFactor(distance, m_minDistance, m_maxDistance, m_targetVelocity,
				info->fps * info->erp);
			info->m_constraintError[0] = mot_fact * m_targetVelocity * (m_rbA.getInvMass() + m_rbB.getInvMass());
			info->m_lowerLimit[0] = -m_maxMotorForce;
			info->m_upperLimit[0] = m_maxMotorForce;

			m_oldDiff = delta;
		} else {
			btScalar k = info->fps * info->erp;
			info->m_constraintError[0] = k * currentLimitError;
			if (m_minDistance == m_maxDistance) {
				// limited low and high simultaneously
				info->m_lowerLimit[0] = -SIMD_INFINITY;
				info->m_upperLimit[0] = SIMD_INFINITY;
			} else {
				if (currentLimit == 1) {
					info->m_lowerLimit[0] = -SIMD_INFINITY;
					info->m_upperLimit[0] = 0;
				} else {
					info->m_lowerLimit[0] = 0;
					info->m_upperLimit[0] = SIMD_INFINITY;
				}
			}
		}
	}
}

#endif  // USE_BULLET
