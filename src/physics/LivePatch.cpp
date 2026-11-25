/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "LivePatch.h"

#ifdef USE_BULLET

#include "Controller.h"
#include "NiflyBullet.h"
#include "SystemBuilder.h"
#include "physics/hdt/hdtConeTwistConstraint.h"
#include "physics/hdt/hdtGeneric6DofConstraint.h"
#include "physics/hdt/hdtSkinnedMeshShape.h"
#include "physics/hdt/hdtStiffSpringConstraint.h"

#include <algorithm>

namespace Physics {
namespace {
	// Reading a ValueVariant the way the property it belongs to is spelled.
	// An int where a float is wanted is accepted: the schema calls some of
	// these flexFloat and a file may hold either.
	bool PatchFloat(const ValueVariant& value, float& out) {
		if (const float* number = std::get_if<float>(&value)) {
			out = *number;
			return true;
		}
		if (const int* number = std::get_if<int>(&value)) {
			out = static_cast<float>(*number);
			return true;
		}
		return false;
	}

	bool PatchInt(const ValueVariant& value, int& out) {
		if (const int* number = std::get_if<int>(&value)) {
			out = *number;
			return true;
		}
		return false;
	}

	bool PatchBool(const ValueVariant& value, bool& out) {
		if (const bool* flag = std::get_if<bool>(&value)) {
			out = *flag;
			return true;
		}
		return false;
	}

	bool PatchVector(const ValueVariant& value, btVector3& out) {
		if (const nifly::Vector3* vector = std::get_if<nifly::Vector3>(&value)) {
			out = ToBt(*vector);
			return true;
		}
		return false;
	}

	// A repeated text element comes back as a list; one written once comes
	// back as the single string, and means a list of one.
	bool PatchStrings(const ValueVariant& value, std::vector<std::string>& out) {
		if (const auto* list = std::get_if<std::vector<std::string>>(&value)) {
			out = *list;
			return true;
		}
		if (const auto* text = std::get_if<std::string>(&value)) {
			out.assign(1, *text);
			return true;
		}
		// An absent list is an empty one, which is a meaningful value here
		if (std::holds_alternative<std::monostate>(value)) {
			out.clear();
			return true;
		}
		return false;
	}

	bool PatchBone(PreviewSystem* system, const PatchRequest& request) {
		auto* bone = system->findBone(hdt::IDStr(request.elementName));
		if (!bone)
			return false;

		btRigidBody& rig = bone->m_rig;
		const std::string& property = request.property;
		float number = 0.0f;

		if (property == "mass") {
			if (!PatchFloat(request.value, number) || number < 0.0f)
				return false;

			// Crossing 0 turns the body between kinematic and dynamic, which
			// changes its collision flags, whether it publishes a pose
			// override at all and which shapes count as affected by physics.
			// None of that can be patched; it is a rebuild.
			if ((rig.getInvMass() > 0.0f) != (number > 0.0f))
				return false;

			rig.setMassProps(number, rig.getLocalInertia());
			rig.updateInertiaTensor();
			return true;
		}

		if (property == "inertia") {
			btVector3 inertia(0, 0, 0);
			if (!PatchVector(request.value, inertia))
				return false;

			const btScalar mass = rig.getInvMass() > 0.0f ? 1.0f / rig.getInvMass() : 0.0f;
			rig.setMassProps(mass, inertia);
			rig.updateInertiaTensor();
			return true;
		}

		if (property == "linearDamping" || property == "angularDamping") {
			if (!PatchFloat(request.value, number))
				return false;

			const bool linear = property == "linearDamping";
			rig.setDamping(linear ? number : rig.getLinearDamping(), linear ? rig.getAngularDamping() : number);
			return true;
		}

		if (!PatchFloat(request.value, number))
			return false;

		// The same clamps readBoneTemplate applies, so a patched value behaves
		// exactly like the same value after a rebuild.
		if (property == "friction")
			rig.setFriction(number);
		else if (property == "rollingFriction")
			rig.setRollingFriction(number);
		else if (property == "restitution")
			rig.setRestitution(number);
		else if (property == "margin-multiplier")
			bone->m_marginMultipler = number;
		else if (property == "gravity-factor")
			bone->m_gravityFactor = btClamped(number, 0.0f, 1.0f);
		else if (property == "wind-factor")
			bone->m_windFactor = std::max(number, 0.0f);
		else
			return false;

		return true;
	}

	bool PatchBody(PreviewSystem* system, const PatchRequest& request) {
		auto* body = static_cast<PreviewBody*>(system->findBody(hdt::IDStr(request.elementName)));
		if (!body)
			return false;

		const std::string& property = request.property;

		if (property == "margin" || property == "penetration") {
			float number = 0.0f;
			if (!PatchFloat(request.value, number))
				return false;

			auto* perTriangle = body->m_shape ? body->m_shape->asPerTriangleShape() : nullptr;
			auto* perVertex = body->m_shape ? body->m_shape->asPerVertexShape() : nullptr;

			if (property == "penetration") {
				if (!perTriangle)
					return false;

				perTriangle->m_shapeProp.penetration = number;
				return true;
			}

			if (perTriangle)
				perTriangle->m_shapeProp.margin = number;

			// PerTriangleShape::finishBuild copies the margin into the
			// per-vertex colliders it generates alongside itself, so writing
			// only the outer shape would leave those at the old size.
			if (perVertex)
				perVertex->m_shapeProp.margin = number;

			return perTriangle || perVertex;
		}

		if (property == "tag" || property == "can-collide-with-tag" || property == "no-collide-with-tag") {
			std::vector<std::string> names;
			if (!PatchStrings(request.value, names))
				return false;

			if (property == "tag") {
				body->m_tags.clear();
				for (const std::string& name : names)
					body->m_tags.push_back(hdt::IDStr(name));

				return true;
			}

			auto& tags = property == "can-collide-with-tag" ? body->m_canCollideWithTags : body->m_noCollideWithTags;
			tags.clear();
			for (const std::string& name : names)
				tags.insert(hdt::IDStr(name));

			return true;
		}

		if (property == "can-collide-with-bone" || property == "no-collide-with-bone") {
			std::vector<std::string> names;
			if (!PatchStrings(request.value, names))
				return false;

			// The builder creates a bone it cannot find; a patch can only bind
			// to one that already exists, so an unknown name is a rebuild.
			std::vector<hdt::SkinnedMeshBone*> bones;
			for (const std::string& name : names) {
				auto* bone = system->findBone(hdt::IDStr(name));
				if (!bone)
					return false;

				bones.push_back(bone);
			}

			auto& target = property == "can-collide-with-bone" ? body->m_canCollideWithBones : body->m_noCollideWithBones;
			target.clear();
			for (auto* bone : bones)
				target.insert(bone);

			return true;
		}

		if (property == "shared") {
			const std::string* text = std::get_if<std::string>(&request.value);
			if (!text)
				return false;

			const hdt::IDStr shared(*text);
			if (shared == hdt::IDStr("public"))
				body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
			else if (shared == hdt::IDStr("internal"))
				body->m_shared = PreviewBody::SharedType::SHARED_INTERNAL;
			else if (shared == hdt::IDStr("external"))
				body->m_shared = PreviewBody::SharedType::SHARED_EXTERNAL;
			else if (shared == hdt::IDStr("private"))
				body->m_shared = PreviewBody::SharedType::SHARED_PRIVATE;
			else
				body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;

			return true;
		}

		if (property == "disable-tag") {
			const std::string* text = std::get_if<std::string>(&request.value);
			if (!text)
				return false;

			body->m_disableTag = hdt::IDStr(*text);
			return true;
		}

		if (property == "disable-priority") {
			int priority = 0;
			if (!PatchInt(request.value, priority))
				return false;

			body->m_disablePriority = priority;
			return true;
		}

		return false;
	}

	// The constraint joining bodyA to bodyB, or null when there is no such
	// constraint or more than one: the XSD puts no key on the pair, so it is a
	// label rather than an address, and an ambiguous one has to rebuild.
	hdt::BoneScaleConstraint* FindConstraint(PreviewSystem* system, const PatchRequest& request) {
		const hdt::IDStr bodyA(request.bodyA);
		const hdt::IDStr bodyB(request.bodyB);

		hdt::BoneScaleConstraint* found = nullptr;
		bool ambiguous = false;

		auto consider = [&](hdt::BoneScaleConstraint* constraint) {
			if (!constraint || !constraint->m_boneA || !constraint->m_boneB)
				return;
			if (constraint->m_boneA->m_name != bodyA || constraint->m_boneB->m_name != bodyB)
				return;

			if (found)
				ambiguous = true;
			else
				found = constraint;
		};

		for (const auto& constraint : system->constraints())
			consider(constraint.get());

		// A constraint inside a constraint-group is held by the group, not by
		// the system's own list
		for (const auto& group : system->constraintGroups()) {
			for (const auto& constraint : group->m_constraints)
				consider(constraint.get());
		}

		return ambiguous ? nullptr : found;
	}

	bool PatchGenericConstraint(hdt::Generic6DofConstraint* constraint, const PatchRequest& request) {
		const std::string& property = request.property;

		// Every property of this constraint is either a vector applied per
		// axis or a flag applied to all six, with the linear axes at 0..2 and
		// the angular ones at 3..5 - the same layout readGenericConstraint uses.
		const bool angular = property.rfind("angular", 0) == 0;
		const int base = angular ? 3 : 0;

		auto* linearMotor = constraint->getTranslationalLimitMotor();

		btVector3 vector(0, 0, 0);
		bool flag = false;
		float number = 0.0f;

		if (property == "linearLowerLimit" || property == "angularLowerLimit" || property == "linearUpperLimit" || property == "angularUpperLimit") {
			if (!PatchVector(request.value, vector))
				return false;

			const bool lower = property.find("Lower") != std::string::npos;
			if (angular) {
				if (lower)
					constraint->setAngularLowerLimit(vector);
				else
					constraint->setAngularUpperLimit(vector);
			}
			else {
				if (lower)
					constraint->setLinearLowerLimit(vector);
				else
					constraint->setLinearUpperLimit(vector);
			}

			return true;
		}

		if (property == "linearStiffness" || property == "angularStiffness") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				const bool limited = angular ? constraint->getRotationalLimitMotor(axis)->m_springStiffnessLimited
											 : linearMotor->m_springStiffnessLimited[axis];
				constraint->setStiffness(base + axis, vector[axis], limited);
			}

			return true;
		}

		if (property == "linearDamping" || property == "angularDamping") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				const bool limited = angular ? constraint->getRotationalLimitMotor(axis)->m_springDampingLimited
											 : linearMotor->m_springDampingLimited[axis];
				constraint->setDamping(base + axis, vector[axis], limited);
			}

			return true;
		}

		if (property == "linearEquilibrium" || property == "angularEquilibrium") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				constraint->setEquilibriumPoint(base + axis, vector[axis]);
				// readGenericConstraint aims the servos at the equilibrium too
				constraint->setServoTarget(base + axis, vector[axis]);
			}

			return true;
		}

		if (property == "linearTargetVelocity" || property == "angularTargetVelocity") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setTargetVelocity(base + axis, vector[axis]);

			return true;
		}

		if (property == "linearMaxMotorForce" || property == "angularMaxMotorForce") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setMaxMotorForce(base + axis, vector[axis]);

			return true;
		}

		if (property == "linearBounce") {
			if (!PatchVector(request.value, vector))
				return false;

			linearMotor->m_bounce = vector;
			return true;
		}

		if (property == "angularBounce") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->getRotationalLimitMotor(axis)->m_bounce = vector[axis];

			return true;
		}

		if (property == "enableLinearSprings" || property == "enableAngularSprings") {
			if (!PatchBool(request.value, flag))
				return false;

			const int springBase = property == "enableAngularSprings" ? 3 : 0;
			for (int axis = 0; axis < 3; ++axis)
				constraint->enableSpring(springBase + axis, flag);

			return true;
		}

		if (property == "linearMotors" || property == "angularMotors") {
			if (!PatchBool(request.value, flag))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->enableMotor(base + axis, flag);

			return true;
		}

		if (property == "linearServoMotors" || property == "angularServoMotors") {
			if (!PatchBool(request.value, flag))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setServo(base + axis, flag);

			return true;
		}

		if (property == "linearStiffnessLimited" || property == "angularStiffnessLimited") {
			if (!PatchBool(request.value, flag))
				return false;

			// Only the flag changes; the stiffness it qualifies stays put
			for (int axis = 0; axis < 3; ++axis) {
				const btScalar stiffness = angular ? constraint->getRotationalLimitMotor(axis)->m_springStiffness
												   : linearMotor->m_springStiffness[axis];
				constraint->setStiffness(base + axis, stiffness, flag);
			}

			return true;
		}

		if (property == "springDampingLimited") {
			if (!PatchBool(request.value, flag))
				return false;

			// This one flag qualifies both halves
			for (int axis = 0; axis < 3; ++axis) {
				constraint->setDamping(axis, linearMotor->m_springDamping[axis], flag);
				constraint->setDamping(axis + 3, constraint->getRotationalLimitMotor(axis)->m_springDamping, flag);
			}

			return true;
		}

		if (property == "motorERP" || property == "motorCFM" || property == "stopERP" || property == "stopCFM") {
			if (!PatchFloat(request.value, number))
				return false;

			// readGenericConstraint sets the linear axes through setParam and
			// writes the angular motors directly; mirror both.
			const int param = property == "motorERP"	? BT_CONSTRAINT_ERP
							  : property == "motorCFM"	? BT_CONSTRAINT_CFM
							  : property == "stopERP"	? BT_CONSTRAINT_STOP_ERP
														: BT_CONSTRAINT_STOP_CFM;

			for (int axis = 0; axis < 3; ++axis) {
				constraint->setParam(param, number, axis);

				auto* rotationMotor = constraint->getRotationalLimitMotor(axis);
				if (!rotationMotor)
					continue;

				if (property == "motorERP")
					rotationMotor->m_motorERP = number;
				else if (property == "motorCFM")
					rotationMotor->m_motorCFM = number;
				else if (property == "stopERP")
					rotationMotor->m_stopERP = number;
				else
					rotationMotor->m_stopCFM = number;
			}

			return true;
		}

		return false;
	}

	bool PatchStiffSpringConstraint(hdt::StiffSpringConstraint* constraint, const PatchRequest& request) {
		float number = 0.0f;
		if (!PatchFloat(request.value, number))
			return false;

		// The distances are the ones the two bones happened to be apart when
		// the constraint was built, times the factors from the XML; the
		// factors themselves are not kept, so re-applying one would compound.
		if (request.property == "stiffness")
			constraint->m_stiffness = number;
		else if (request.property == "damping")
			constraint->m_damping = number;
		else
			return false;

		return true;
	}

	bool PatchConeTwistConstraint(hdt::ConeTwistConstraint* constraint, const PatchRequest& request) {
		float number = 0.0f;
		if (!PatchFloat(request.value, number))
			return false;

		// btConeTwistConstraint takes all six together and recomputes from
		// them, so the five that did not change are read back out first.
		float swingSpan1 = constraint->getSwingSpan1();
		float swingSpan2 = constraint->getSwingSpan2();
		float twistSpan = constraint->getTwistSpan();
		float softness = constraint->getLimitSoftness();
		float bias = constraint->getBiasFactor();
		float relaxation = constraint->getRelaxationFactor();

		const std::string& property = request.property;
		if (property == "swingSpan1")
			swingSpan1 = number;
		else if (property == "swingSpan2")
			swingSpan2 = number;
		else if (property == "twistSpan")
			twistSpan = number;
		else if (property == "limitSoftness")
			softness = number;
		else if (property == "biasFactor")
			bias = number;
		else if (property == "relaxationFactor")
			relaxation = number;
		else
			return false;

		constraint->setLimit(swingSpan1, swingSpan2, twistSpan, softness, bias, relaxation);
		return true;
	}
} // namespace

bool PatchLiveProperty(PreviewSystem& system, const PatchRequest& request) {
	// Clearing a property hands it back to the ambient *-default cascade, and
	// what that resolves to is not something this end knows; rebuild instead.
	if (std::holds_alternative<std::monostate>(request.value))
		return false;

	switch (request.kind) {
		case ElementKind::Bone: return PatchBone(&system, request);

		case ElementKind::PerVertexShape:
		case ElementKind::PerTriangleShape: return PatchBody(&system, request);

		case ElementKind::GenericConstraint:
		case ElementKind::StiffSpringConstraint:
		case ElementKind::ConeTwistConstraint: break;

		// A *-default declares a template rather than an object, a <shape> is
		// baked into the bone that carries it, and a constraint-group is only
		// a bracket. None of them has anything live to write to.
		default: return false;
	}

	hdt::BoneScaleConstraint* constraint = FindConstraint(&system, request);
	if (!constraint)
		return false;

	if (auto* generic = dynamic_cast<hdt::Generic6DofConstraint*>(constraint))
		return PatchGenericConstraint(generic, request);
	if (auto* stiffSpring = dynamic_cast<hdt::StiffSpringConstraint*>(constraint))
		return PatchStiffSpringConstraint(stiffSpring, request);
	if (auto* coneTwist = dynamic_cast<hdt::ConeTwistConstraint*>(constraint))
		return PatchConeTwistConstraint(coneTwist, request);

	return false;
}
} // namespace Physics

#endif // USE_BULLET
