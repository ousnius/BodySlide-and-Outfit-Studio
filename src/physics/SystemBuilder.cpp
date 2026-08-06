/*
BodySlide and Outfit Studio
See the included LICENSE file

Port of hdtSMP64's hdtSkyrimSystem.cpp / hdtSkyrimBone.cpp / hdtSkyrimBody.cpp
(SkyrimSystemCreator, SkyrimSystem, SkyrimBone, SkyrimBody). Game-engine data
sources are replaced: mesh/skin data comes from nifly + AnimInfo, bones bind to
AnimSkeleton's AnimBone instead of NiNode, and logging goes to a warning list.
The XML parsing semantics are kept identical to upstream.
*/

#ifdef USE_BULLET

#include "SystemBuilder.h"

#include "NiflyBullet.h"
#include "hdt/XmlReader.h"
#include "hdt/hdtSkinnedMeshShape.h"

#include "../components/Anim.h"

#include <NifFile.hpp>

#include <algorithm>
#include <cfloat>

namespace Physics {
using hdt::btQsTransform;
using hdt::IDStr;
using hdt::RESET_PHYSICS;

// nifly rotation matrix -> Bullet quaternion (via btMatrix3x3)
static btQuaternion ToBtQuaternion(const nifly::Matrix3& m) {
	btQuaternion q;
	ToBt(m).getRotation(q);
	return q;
}

// Replacement for upstream's convertNi(RE::NiTransform): nifly MatTransform ->
// btQsTransform with an explicit scale slot.
static btQsTransform ToBtQs(const nifly::MatTransform& t, float scaleOverride) {
	if (!(scaleOverride > FLT_EPSILON))
		scaleOverride = 1.0f;
	return btQsTransform(ToBtQuaternion(t.rotation), ToBt(t.translation), scaleOverride);
}

btEmptyShape SystemBuilder::BoneTemplate::emptyShape[1];

// ---------------------------------------------------------------- PreviewBone

PreviewBone::PreviewBone(const IDStr& name, AnimBone* animBone, PreviewSystem* system, btRigidBody::btRigidBodyConstructionInfo& ci)
	: hdt::SkinnedMeshBone(name, ci)
	, m_animBone(animBone)
	, m_system(system) {
	if (ci.m_mass)
		m_rig.setCollisionFlags(0);
	else
		m_rig.setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);

	m_depth = 0;
	for (auto i = animBone; i; i = i->parent)
		++m_depth;
}

void PreviewBone::readTransform(float timeStep) {
	// Upstream read the NiNode's world transform here. The kinematic input is
	// now the pose-to-global transform of the AnimBone, shifted by the
	// controller's root motion. Scale is treated as a constant 1 (poseScale
	// is ignored), so upstream's scale-changed branch is dropped entirely.
	m_currentTransform = btQsTransform(m_system->m_rootMotion * ToBt(m_animBone->xformPoseToGlobal), 1.0f);

	auto current = m_rig.getWorldTransform();
	auto isStaticOrKinematic = m_rig.isStaticOrKinematicObject();

	auto dest = m_currentTransform.asTransform() * m_localToRig;
	if (timeStep <= RESET_PHYSICS) {
		static const btVector3 zero(0, 0, 0);
		m_rig.setWorldTransform(dest);
		m_rig.setInterpolationWorldTransform(dest);
		m_rig.setLinearVelocity(zero);
		m_rig.setAngularVelocity(zero);
		m_rig.setInterpolationLinearVelocity(zero);
		m_rig.setInterpolationAngularVelocity(zero);
		m_rig.updateInertiaTensor();
	}
	else if (isStaticOrKinematic) {
		btVector3 linVel, angVel;
		btTransformUtil::calculateVelocity(current, dest, timeStep, linVel, angVel);
		m_rig.setLinearVelocity(linVel);
		m_rig.setAngularVelocity(angVel);
		m_rig.setInterpolationLinearVelocity(linVel);
		m_rig.setInterpolationAngularVelocity(angVel);
	}
}

void PreviewBone::writeTransform() {
	auto transform = m_rig.getWorldTransform() * m_rigToLocal;

	m_currentTransform.setBasis(transform.getBasis());
	m_currentTransform.setOrigin(transform.getOrigin());

	// Upstream wrote the NiNode's world transform. Here the simulated pose is
	// published to the controller's override map, expressed without the root
	// motion so the skinning code can use it in place of xformPoseToGlobal.
	// Kinematic bones write nothing (the caller skips them too).
	if (m_rig.isKinematicObject())
		return;

	auto* overrides = m_system->m_poseOverrides;
	if (overrides)
		(*overrides)[m_animBone->boneName] = FromBt(m_system->m_rootMotion.inverseTimes(transform));
}

// ---------------------------------------------------------------- PreviewBody

bool PreviewBody::canCollideWith(const hdt::SkinnedMeshBody* rhs) const {
	auto body = (PreviewBody*)rhs;
	if (m_disabled || body->m_disabled)
		return false;

	// The preview simulates a single actor, so every system shares the same
	// skeleton: "internal" always passes and "external" never does.
	switch (m_shared) {
		case SharedType::SHARED_PUBLIC: break;
		case SharedType::SHARED_INTERNAL: break;
		case SharedType::SHARED_EXTERNAL: return false;
		case SharedType::SHARED_PRIVATE:
			if (m_mesh != body->m_mesh)
				return false;
			break;
		default: return false;
	}

	return hdt::SkinnedMeshBody::canCollideWith(rhs);
}

void PreviewBody::internalUpdate() {
	if (m_disabled)
		return;
	hdt::SkinnedMeshBody::internalUpdate();
}

// -------------------------------------------------------------- PreviewSystem

hdt::SkinnedMeshBone* PreviewSystem::findBone(const IDStr& name) {
	for (auto i : m_bones) {
		if (i->m_name == name)
			return i.get();
	}

	return nullptr;
}

hdt::SkinnedMeshBody* PreviewSystem::findBody(const IDStr& name) {
	for (auto i : m_meshes) {
		if (i->m_name == name)
			return i.get();
	}

	return nullptr;
}

int PreviewSystem::findBoneIdx(const IDStr& name) {
	for (size_t i = 0; i < m_bones.size(); ++i) {
		if (m_bones[i]->m_name == name)
			return static_cast<int>(i);
	}

	return -1;
}

float PreviewSystem::prepareForRead(float timeStep) {
	// Port of SkyrimSystem::prepareForRead. The game watched the skeleton
	// root's world rotation and damped fast rotation to keep the simulation
	// from exploding; here m_rootMotion (the camera turntable yaw) is the only
	// root movement, so the damper acts on its rotation delta instead. The
	// game-only branches (skeleton reparenting, player camera state) are gone.
	if (!m_initialized) {
		timeStep = RESET_PHYSICS;
		m_initialized = true;
	}

	if (timeStep <= RESET_PHYSICS) {
		m_lastRootRotation = m_rootMotion.getRotation();
	}
	else {
		btQuaternion newRot = m_rootMotion.getRotation();
		btVector3 rotAxis;
		float rotAngle;
		btTransformUtil::calculateDiffAxisAngleQuaternion(m_lastRootRotation, newRot, rotAxis, rotAngle);

		if (m_clampRotations) {
			float limit = m_rotationSpeedLimit * timeStep;

			if (rotAngle < -limit || rotAngle > limit) {
				rotAngle = btClamped(rotAngle, -limit, limit);
				btQuaternion clampedRot(rotAxis, rotAngle);
				m_lastRootRotation = clampedRot * m_lastRootRotation;
				// Upstream wrote the damped rotation back into the skeleton
				// root; the equivalent here is damping the root motion itself.
				m_rootMotion.setRotation(m_lastRootRotation);
			}
			else
				m_lastRootRotation = newRot;
		}
		else if (m_unclampedResets) {
			float limit = m_unclampedResetAngle * timeStep;

			if (rotAngle < -limit || rotAngle > limit)
				timeStep = RESET_PHYSICS;

			m_lastRootRotation = newRot;
		}
		else
			m_lastRootRotation = newRot;
	}

	return timeStep;
}

// ---------------------------------------------------- SystemBuilder

void SystemBuilder::warn(const std::string& msg) {
	if (m_warnings)
		m_warnings->push_back(m_filePath + ": " + msg);
}

void SystemBuilder::indexBone(PreviewBone* bone) {
	m_boneIndex.emplace(bone->m_name, bone);
}

PreviewBone* SystemBuilder::findBoneFromIndex(const IDStr& name) const {
	auto it = m_boneIndex.find(name);
	return it != m_boneIndex.end() ? it->second : nullptr;
}

// Replacement for upstream's findObjectByName: resolves a bone name to an
// AnimBone of the application skeleton. Falls back to loading the node from
// the NIF as a custom bone; the initial lookup retries with the NIF node's
// exact spelling because AnimSkeleton is case-sensitive while SMP names
// (BSFixedString upstream, IDStr here) are not.
AnimBone* SystemBuilder::findAnimBone(const IDStr& name) {
	auto& skel = AnimSkeleton::getInstance();

	AnimBone* bone = skel.GetBonePtr(name.str(), true);
	if (bone)
		return bone;

	std::string nodeName = name.str();
	if (m_nif) {
		for (auto* node : m_nif->GetNodes()) {
			if (IDStr(node->name.get()) == name) {
				nodeName = node->name.get();
				break;
			}
		}
	}

	bone = skel.GetBonePtr(nodeName, true);
	if (!bone && m_nif)
		bone = skel.LoadCustomBoneFromNif(m_nif, nodeName);

	return bone;
}

PreviewBone* SystemBuilder::getOrCreateBone(const IDStr& name) {
	auto bone = findBoneFromIndex(name);
	if (bone)
		return bone;

	warn("Bone " + name.str() + " used before being created, trying to create it with current default values");
	return createBoneFromNodeName(name);
}

hdt::Ref<PreviewSystem> SystemBuilder::Build(const BuildInput& input, std::vector<std::string>& outWarnings) {
	m_warnings = &outWarnings;
	m_filePath = input.xmlName;

	if (!input.xmlData || input.xmlData->empty() || !input.nif || !input.anim)
		return nullptr;

	m_nif = input.nif;
	m_anim = input.anim;
	m_nifShapes = &input.shapes;

	hdt::XMLReader reader(reinterpret_cast<const uint8_t*>(input.xmlData->data()), input.xmlData->size());
	m_reader = &reader;

	auto system = readSystem();

	// The reader only lives for this call
	m_reader = nullptr;
	return system;
}

hdt::Ref<PreviewSystem> SystemBuilder::readSystem() {
	// The whole document is parsed up front, so a malformed file is reported
	// as such instead of as a missing <system> element.
	if (m_reader->HasError()) {
		warn(std::string("xml parse error - ") + m_reader->GetErrorMessage());
		return nullptr;
	}

	m_reader->nextStartElement();
	if (m_reader->GetName() != "system") {
		warn("root element is not <system>");
		return nullptr;
	}

	m_mesh = hdt::make_ref(new PreviewSystem);
	m_boneIndex.clear();

	try {
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				const auto name = m_reader->GetName();
				if (name == "bone") {
					readOrUpdateBone();
				}
				else if (name == "bone-default") {
					auto clsname = m_reader->getAttribute("name", "");
					auto extends = m_reader->getAttribute("extends", "");
					auto defaultBoneInfo = getBoneTemplate(extends);
					readBoneTemplate(defaultBoneInfo);
					m_boneTemplates[clsname] = defaultBoneInfo;
				}
				else if (name == "per-vertex-shape") {
					auto shape = readPerVertexShape();
					if (shape && shape->m_vertices.size()) {
						m_mesh->m_meshes.push_back(shape);
						shape->m_mesh = m_mesh.get();
					}
				}
				else if (name == "per-triangle-shape") {
					auto shape = readPerTriangleShape();
					if (shape && shape->m_vertices.size()) {
						m_mesh->m_meshes.push_back(shape);
						shape->m_mesh = m_mesh.get();
					}
				}
				else if (name == "constraint-group") {
					auto constraint = readConstraintGroup();
					if (constraint)
						m_mesh->m_constraintGroups.push_back(constraint);
				}
				else if (name == "generic-constraint") {
					auto constraint = readGenericConstraint();
					if (constraint)
						m_mesh->m_constraints.push_back(constraint);
				}
				else if (name == "stiffspring-constraint") {
					auto constraint = readStiffSpringConstraint();
					if (constraint)
						m_mesh->m_constraints.push_back(constraint);
				}
				else if (name == "conetwist-constraint") {
					auto constraint = readConeTwistConstraint();
					if (constraint)
						m_mesh->m_constraints.push_back(constraint);
				}
				else if (name == "generic-constraint-default") {
					auto clsname = m_reader->getAttribute("name", "");
					auto extends = m_reader->getAttribute("extends", "");
					auto defaultGenericConstraintTemplate = getGenericConstraintTemplate(extends);
					readGenericConstraintTemplate(defaultGenericConstraintTemplate);
					m_genericConstraintTemplates[clsname] = defaultGenericConstraintTemplate;
				}
				else if (name == "stiffspring-constraint-default") {
					auto clsname = m_reader->getAttribute("name", "");
					auto extends = m_reader->getAttribute("extends", "");
					auto defaultStiffSpringConstraintTemplate = getStiffSpringConstraintTemplate(extends);
					readStiffSpringConstraintTemplate(defaultStiffSpringConstraintTemplate);
					m_stiffSpringConstraintTemplates[clsname] = defaultStiffSpringConstraintTemplate;
				}
				else if (name == "conetwist-constraint-default") {
					auto clsname = m_reader->getAttribute("name", "");
					auto extends = m_reader->getAttribute("extends", "");
					auto defaultConeTwistConstraintTemplate = getConeTwistConstraintTemplate(extends);
					readConeTwistConstraintTemplate(defaultConeTwistConstraintTemplate);
					m_coneTwistConstraintTemplates[clsname] = defaultConeTwistConstraintTemplate;
				}
				else if (name == "shape") {
					auto attrName = m_reader->getAttribute("name");
					auto shape = readShape();
					if (shape) {
						m_shapeRefs.push_back(shape);
						m_shapes.insert(std::make_pair(attrName, shape));
					}
				}
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
	}
	catch (const std::string& err) {
		warn("xml parse error - " + err);
		return nullptr;
	}

	// Upstream ran this over TBB above a size threshold and serially below it;
	// hdt::par::for_each is serial, so there is only one path left.
	hdt::par::for_each(m_deferredBuilds.begin(), m_deferredBuilds.end(), [](const DeferredBuild& db) {
		if (db.vertexShape)
			db.vertexShape->autoGen();
		db.body->finishBuild();
	});

	m_deferredBuilds.clear();

	m_mesh->m_shapeRefs.swap(m_shapeRefs);
	std::sort(m_mesh->m_bones.begin(), m_mesh->m_bones.end(), [](const auto& a, const auto& b) {
		return static_cast<PreviewBone*>(a.get())->m_depth < static_cast<PreviewBone*>(b.get())->m_depth;
	});

	return m_mesh->valid() ? m_mesh : nullptr;
}

hdt::Ref<hdt::ConstraintGroup> SystemBuilder::readConstraintGroup() {
	hdt::Ref<hdt::ConstraintGroup> ret = hdt::make_ref(new hdt::ConstraintGroup);

	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();

			if (name == "generic-constraint") {
				auto constraint = readGenericConstraint();
				if (constraint)
					ret->m_constraints.push_back(constraint);
			}
			else if (name == "stiffspring-constraint") {
				auto constraint = readStiffSpringConstraint();
				if (constraint)
					ret->m_constraints.push_back(constraint);
			}
			else if (name == "conetwist-constraint") {
				auto constraint = readConeTwistConstraint();
				if (constraint)
					ret->m_constraints.push_back(constraint);
			}
			else if (name == "generic-constraint-default") {
				auto clsname = m_reader->getAttribute("name", "");
				auto extends = m_reader->getAttribute("extends", "");
				auto defaultGenericConstraintTemplate = getGenericConstraintTemplate(extends);
				readGenericConstraintTemplate(defaultGenericConstraintTemplate);
				m_genericConstraintTemplates[clsname] = defaultGenericConstraintTemplate;
			}
			else if (name == "stiffspring-constraint-default") {
				auto clsname = m_reader->getAttribute("name", "");
				auto extends = m_reader->getAttribute("extends", "");
				auto defaultStiffSpringConstraintTemplate = getStiffSpringConstraintTemplate(extends);
				readStiffSpringConstraintTemplate(defaultStiffSpringConstraintTemplate);
				m_stiffSpringConstraintTemplates[clsname] = defaultStiffSpringConstraintTemplate;
			}
			else if (name == "conetwist-constraint-default") {
				auto clsname = m_reader->getAttribute("name", "");
				auto extends = m_reader->getAttribute("extends", "");
				auto defaultConeTwistConstraintTemplate = getConeTwistConstraintTemplate(extends);
				readConeTwistConstraintTemplate(defaultConeTwistConstraintTemplate);
				m_coneTwistConstraintTemplates[clsname] = defaultConeTwistConstraintTemplate;
			}
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
	return ret;
}

void SystemBuilder::readBoneTemplate(BoneTemplate& cinfo) {
	bool clearCollide = true;
	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();
			if (name == "mass")
				cinfo.m_mass = m_reader->readFloat();
			else if (name == "inertia")
				cinfo.m_localInertia = m_reader->readVector3();
			else if (name == "centerOfMassTransform")
				cinfo.m_centerOfMassTransform = m_reader->readTransform();
			else if (name == "linearDamping")
				cinfo.m_linearDamping = m_reader->readFloat();
			else if (name == "angularDamping")
				cinfo.m_angularDamping = m_reader->readFloat();
			else if (name == "friction")
				cinfo.m_friction = m_reader->readFloat();
			else if (name == "rollingFriction")
				cinfo.m_rollingFriction = m_reader->readFloat();
			else if (name == "restitution")
				cinfo.m_restitution = m_reader->readFloat();
			else if (name == "margin-multiplier")
				cinfo.m_marginMultipler = m_reader->readFloat();
			else if (name == "shape") {
				auto shape = readShape();
				if (shape) {
					m_shapeRefs.push_back(shape);
					cinfo.m_collisionShape = shape.get();
				}
				else
					cinfo.m_collisionShape = BoneTemplate::emptyShape;
			}
			else if (name == "collision-filter")
				cinfo.m_collisionFilter = m_reader->readInt();
			else if (name == "can-collide-with-bone") {
				if (clearCollide) {
					cinfo.m_canCollideWithBone.clear();
					cinfo.m_noCollideWithBone.clear();
					clearCollide = false;
				}
				cinfo.m_canCollideWithBone.push_back(m_reader->readText());
			}
			else if (name == "no-collide-with-bone") {
				if (clearCollide) {
					cinfo.m_canCollideWithBone.clear();
					cinfo.m_noCollideWithBone.clear();
					clearCollide = false;
				}
				cinfo.m_noCollideWithBone.push_back(m_reader->readText());
			}
			else if (name == "gravity-factor") {
				cinfo.m_gravityFactor = btClamped(m_reader->readFloat(), 0.0f, 1.0f);
			}
			else if (name == "wind-factor") {
				cinfo.m_windFactor = std::max(m_reader->readFloat(), 0.0f);
			}
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
}

std::shared_ptr<btCollisionShape> SystemBuilder::readShape() {
	auto typeStr = m_reader->getAttribute("type");
	if (typeStr == "ref") {
		auto shapeName = m_reader->getAttribute("name");
		m_reader->skipCurrentElement();
		auto iter = m_shapes.find(shapeName);
		if (iter != m_shapes.end())
			return iter->second;
		warn("unknown shape - " + shapeName);
		return nullptr;
	}
	if (typeStr == "box") {
		btVector3 halfExtend(0, 0, 0);
		float margin = 0;
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				auto name = m_reader->GetName();
				if (name == "halfExtend")
					halfExtend = m_reader->readVector3();
				else if (name == "margin")
					margin = m_reader->readFloat();
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
		auto ret = std::make_shared<btBoxShape>(halfExtend);
		ret->setMargin(margin);
		return ret;
	}
	if (typeStr == "sphere") {
		float radius = 0;
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				auto name = m_reader->GetName();
				if (name == "radius")
					radius = m_reader->readFloat();
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
		return std::make_shared<btSphereShape>(radius);
	}
	if (typeStr == "capsule") {
		float radius = 0;
		float height = 0;
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				auto name = m_reader->GetName();
				if (name == "radius")
					radius = m_reader->readFloat();
				else if (name == "height")
					height = m_reader->readFloat();
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
		return std::make_shared<btCapsuleShape>(radius, height);
	}
	if (typeStr == "hull") {
		float margin = 0;
		auto ret = std::make_shared<btConvexHullShape>();
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				auto name = m_reader->GetName();
				if (name == "point")
					ret->addPoint(m_reader->readVector3(), false);
				else if (name == "margin")
					margin = m_reader->readFloat();
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
		(void)margin; // never applied upstream either
		ret->recalcLocalAabb();
		return ret->getNumPoints() ? ret : nullptr;
	}
	if (typeStr == "cylinder") {
		float height = 0;
		float radius = 0;
		float margin = 0;
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				auto name = m_reader->GetName();
				if (name == "height")
					height = m_reader->readFloat();
				else if (name == "radius")
					radius = m_reader->readFloat();
				else if (name == "margin")
					margin = m_reader->readFloat();
				else {
					warn("unknown element - " + name);
					m_reader->skipCurrentElement();
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}

		if (radius >= 0 && height >= 0) {
			auto ret = std::make_shared<btCylinderShape>(btVector3(radius, height, radius));
			ret->setMargin(margin);
			return ret;
		}
		return nullptr;
	}
	if (typeStr == "compound") {
		auto ret = std::make_shared<btCompoundShape>();
		while (m_reader->Inspect()) {
			if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
				if (m_reader->GetName() == "child") {
					btTransform tr;
					std::shared_ptr<btCollisionShape> shape;

					while (m_reader->Inspect()) {
						if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
							if (m_reader->GetName() == "transform") {
								tr = m_reader->readTransform();
							}
							else if (m_reader->GetName() == "shape") {
								shape = readShape();
							}
							else {
								warn("unknown element - " + m_reader->GetName());
								m_reader->skipCurrentElement();
							}
						}
						else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
							break;
					}

					if (shape) {
						ret->addChildShape(tr, shape.get());
						m_shapeRefs.push_back(shape);
					}
				}
			}
			else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
				break;
		}
		return ret->getNumChildShapes() ? ret : nullptr;
	}
	warn("Unknown shape type " + typeStr);
	return nullptr;
}

void SystemBuilder::readOrUpdateBone() {
	IDStr name = m_reader->getAttribute("name");
	if (findBoneFromIndex(name)) {
		warn("Bone " + name.str() + " already exists, skipped");
		m_reader->skipCurrentElement();
		return;
	}

	IDStr cls = m_reader->getAttribute("template", "");
	if (!createBoneFromNodeName(name, cls, true))
		m_reader->skipCurrentElement();
}

PreviewBone* SystemBuilder::createBoneFromNodeName(const IDStr& bodyName, const IDStr& templateName, const bool readTemplate) {
	auto animBone = findAnimBone(bodyName);
	if (animBone) {
		auto boneTemplate = getBoneTemplate(templateName);
		if (readTemplate)
			readBoneTemplate(boneTemplate);
		auto bone = new PreviewBone(animBone->boneName, animBone, m_mesh.get(), boneTemplate);
		bone->m_localToRig = boneTemplate.m_centerOfMassTransform;
		bone->m_rigToLocal = boneTemplate.m_centerOfMassTransform.inverse();
		bone->m_marginMultipler = boneTemplate.m_marginMultipler;
		bone->m_gravityFactor = boneTemplate.m_gravityFactor;
		bone->m_windFactor = boneTemplate.m_windFactor;

		bone->readTransform(RESET_PHYSICS);

		m_mesh->m_bones.push_back(bone);
		indexBone(bone);
		return bone;
	}
	warn("Node named " + bodyName.str() + " doesn't exist, skipped, no bone created");
	return nullptr;
}

std::pair<hdt::Ref<PreviewBody>, SystemBuilder::VertexOffsetMap> SystemBuilder::generateMeshBody(const std::string& name) {
	hdt::Ref<PreviewBody> body = hdt::make_ref(new PreviewBody);
	body->m_name = name;

	int vertexStart = 0;
	int boneStart = 0;

	VertexOffsetMap vertexOffsetMap;

	const IDStr nameId(name);
	for (auto* shape : *m_nifShapes) {
		if (IDStr(shape->name.get()) != nameId)
			continue;

		// Upstream skipped meshes without a skin instance
		auto skinIt = m_anim->shapeSkinning.find(shape->name.get());
		if (skinIt == m_anim->shapeSkinning.end())
			continue;
		auto& skin = skinIt->second;

		std::vector<nifly::Vector3> verts;
		if (!m_nif->GetVertsForShape(shape, verts) || verts.empty())
			continue;

		// Skin bones ordered by their skin bone index, so per-vertex bone
		// indices keep the same meaning as in the NIF vertex data upstream.
		int numBones = 0;
		for (auto& bn : skin.boneNames)
			numBones = std::max(numBones, bn.second + 1);

		std::vector<const std::string*> boneNamesByIdx(static_cast<size_t>(numBones), nullptr);
		for (auto& bn : skin.boneNames)
			if (bn.second >= 0 && bn.second < numBones)
				boneNamesByIdx[bn.second] = &bn.first;

		const size_t skinnedBonesBefore = body->m_skinnedBones.size();
		bool boneFailed = false;

		for (int boneIdx = 0; boneIdx < numBones && !boneFailed; ++boneIdx) {
			if (!boneNamesByIdx[boneIdx]) {
				warn("Shape " + std::string(shape->name.get()) + " has a gap in its skin bone indices, skipped");
				boneFailed = true;
				break;
			}
			const std::string& boneName = *boneNamesByIdx[boneIdx];

			auto bone = static_cast<hdt::SkinnedMeshBone*>(findBoneFromIndex(boneName));
			if (!bone) {
				auto defaultBoneInfo = getBoneTemplate(IDStr(""));
				auto animBone = findAnimBone(boneName);
				if (!animBone) {
					// Preserving the vertex bone indices requires every skin
					// bone, so a missing node drops the whole mesh.
					warn("Bone node " + boneName + " of shape " + std::string(shape->name.get()) + " doesn't exist, shape skipped");
					boneFailed = true;
					break;
				}
				auto newBone = new PreviewBone(animBone->boneName, animBone, m_mesh.get(), defaultBoneInfo);
				// Upstream left the rig at identity here; seating it on the
				// pose keeps constraint frames correct for skin-created bones.
				newBone->readTransform(RESET_PHYSICS);
				m_mesh->m_bones.push_back(newBone);
				indexBone(newBone);
				bone = newBone;
				warn("Created bone " + boneName + " added to body " + name + ", created without default values");
			}

			btQsTransform skinToBone = btQsTransform::getIdentity();
			hdt::BoundingSphere boundingSphere(btVector3(0, 0, 0), 0.0f);
			auto awIt = skin.boneWeights.find(boneIdx);
			if (awIt != skin.boneWeights.end()) {
				const AnimWeight& aw = awIt->second;
				skinToBone = ToBtQs(aw.xformSkinToBone, aw.xformSkinToBone.scale);
				boundingSphere = hdt::BoundingSphere(ToBt(aw.bounds.center), aw.bounds.radius);
			}

			body->addBone(bone, skinToBone, boundingSphere);
		}

		if (boneFailed) {
			body->m_skinnedBones.resize(skinnedBonesBefore);
			continue;
		}

		body->m_vertices.resize(vertexStart + verts.size());

		for (size_t j = 0; j < verts.size(); ++j)
			body->m_vertices[vertexStart + j].m_skinPos = ToBt(verts[j]);

		// Distribute the bone weights over the 4 slots of each vertex,
		// keeping the 4 largest weights (upstream read the NIF's 4 weight
		// slots directly; AnimSkin stores weights per bone instead).
		for (int boneIdx = 0; boneIdx < numBones; ++boneIdx) {
			auto awIt = skin.boneWeights.find(boneIdx);
			if (awIt == skin.boneWeights.end())
				continue;

			for (const auto& vw : awIt->second.weights) {
				if (vw.first >= verts.size())
					continue;

				auto& v = body->m_vertices[vertexStart + vw.first];
				int minSlot = 0;
				for (int k = 1; k < 4; ++k)
					if (v.m_weight[k] < v.m_weight[minSlot])
						minSlot = k;

				if (vw.second > v.m_weight[minSlot]) {
					v.m_weight[minSlot] = vw.second;
					v.setBoneIdx(minSlot, static_cast<hdt::U32>(boneStart + boneIdx));
				}
			}
		}

		vertexOffsetMap.emplace_back(shape, vertexStart);
		boneStart = static_cast<int>(body->m_skinnedBones.size());
		vertexStart = static_cast<int>(body->m_vertices.size());
	}

	if (0 == vertexStart) {
		m_reader->skipCurrentElement();
		return {nullptr, {}};
	}

	for (auto& i : body->m_vertices)
		i.sortWeight();

	return {body, vertexOffsetMap};
}

hdt::Ref<PreviewBody> SystemBuilder::readPerVertexShape() {
	auto name = m_reader->getAttribute("name");

	auto body = generateMeshBody(name).first;
	if (!body)
		return nullptr;

	auto shape = hdt::make_ref(new hdt::PerVertexShape(body.get()));

	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto nodeName = m_reader->GetName();
			if (nodeName == "priority") {
				warn("priority is deprecated and no longer used");
				m_reader->skipCurrentElement();
			}
			else if (nodeName == "margin") {
				shape->m_shapeProp.margin = m_reader->readFloat();
			}
			else if (nodeName == "shared") {
				auto str = m_reader->readText();
				if (str == "public") {
					body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
				}
				else if (str == "internal") {
					body->m_shared = PreviewBody::SharedType::SHARED_INTERNAL;
				}
				else if (str == "external") {
					body->m_shared = PreviewBody::SharedType::SHARED_EXTERNAL;
				}
				else if (str == "private") {
					body->m_shared = PreviewBody::SharedType::SHARED_PRIVATE;
				}
				else {
					warn("unknown shared value, use default value \"public\"");
					body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
				}
			}
			else if (nodeName == "tag") {
				body->m_tags.push_back(m_reader->readText());
			}
			else if (nodeName == "can-collide-with-tag") {
				body->m_canCollideWithTags.insert(m_reader->readText());
			}
			else if (nodeName == "no-collide-with-tag") {
				body->m_noCollideWithTags.insert(m_reader->readText());
			}
			else if (nodeName == "can-collide-with-bone") {
				auto bone = getOrCreateBone(m_reader->readText());
				if (bone)
					body->m_canCollideWithBones.insert(bone);
			}
			else if (nodeName == "no-collide-with-bone") {
				auto bone = getOrCreateBone(m_reader->readText());
				if (bone)
					body->m_noCollideWithBones.insert(bone);
			}
			else if (nodeName == "weight-threshold") {
				auto boneName = m_reader->getAttribute("bone");
				float wt = m_reader->readFloat();
				for (size_t i = 0; i < body->m_skinnedBones.size(); ++i) {
					if (body->m_skinnedBones[i].ptr->m_name == IDStr(boneName)) {
						body->m_skinnedBones[i].weightThreshold = wt;
						break;
					}
				}
			}
			else if (nodeName == "disable-tag") {
				body->m_disableTag = m_reader->readText();
			}
			else if (nodeName == "disable-priority") {
				body->m_disablePriority = m_reader->readInt();
			}
			else {
				warn("unknown element - " + nodeName);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag) {
			break;
		}
	}

	m_deferredBuilds.push_back({body.get(), shape.get()});

	return body;
}

hdt::Ref<PreviewBody> SystemBuilder::readPerTriangleShape() {
	auto name = m_reader->getAttribute("name");

	auto bodyData = generateMeshBody(name);
	auto body = bodyData.first;
	auto vertexOffsetMap = bodyData.second;
	if (!body)
		return nullptr;

	auto shape = hdt::make_ref(new hdt::PerTriangleShape(body.get()));

	for (auto& entry : vertexOffsetMap) {
		int offset = entry.second;

		std::vector<nifly::Triangle> tris;
		if (!entry.first->GetTriangles(tris)) {
			warn("Shape " + std::string(entry.first->name.get()) + " has no triangle data, skipped");
			return nullptr;
		}

		for (const auto& t : tris)
			shape->addTriangle(t.p1 + offset, t.p2 + offset, t.p3 + offset);
	}

	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto nodeName = m_reader->GetName();
			if (nodeName == "priority") {
				warn("priority is deprecated and no longer used");
				m_reader->skipCurrentElement();
			}
			else if (nodeName == "margin") {
				shape->m_shapeProp.margin = m_reader->readFloat();
			}
			else if (nodeName == "shared") {
				auto str = m_reader->readText();
				if (str == "public") {
					body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
				}
				else if (str == "internal") {
					body->m_shared = PreviewBody::SharedType::SHARED_INTERNAL;
				}
				else if (str == "external") {
					body->m_shared = PreviewBody::SharedType::SHARED_EXTERNAL;
				}
				else if (str == "private") {
					body->m_shared = PreviewBody::SharedType::SHARED_PRIVATE;
				}
				else {
					warn("unknown shared value, use default value \"public\"");
					body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
				}
			}
			else if (nodeName == "prenetration" || nodeName == "penetration") {
				shape->m_shapeProp.penetration = m_reader->readFloat();
			}
			else if (nodeName == "tag") {
				body->m_tags.push_back(m_reader->readText());
			}
			else if (nodeName == "no-collide-with-tag") {
				body->m_noCollideWithTags.insert(m_reader->readText());
			}
			else if (nodeName == "can-collide-with-tag") {
				body->m_canCollideWithTags.insert(m_reader->readText());
			}
			else if (nodeName == "can-collide-with-bone") {
				auto bone = getOrCreateBone(m_reader->readText());
				if (bone)
					body->m_canCollideWithBones.insert(bone);
			}
			else if (nodeName == "no-collide-with-bone") {
				auto bone = getOrCreateBone(m_reader->readText());
				if (bone)
					body->m_noCollideWithBones.insert(bone);
			}
			else if (nodeName == "weight-threshold") {
				auto boneName = m_reader->getAttribute("bone");
				float wt = m_reader->readFloat();
				for (size_t i = 0; i < body->m_skinnedBones.size(); ++i) {
					if (body->m_skinnedBones[i].ptr->m_name == IDStr(boneName)) {
						body->m_skinnedBones[i].weightThreshold = wt;
					}
				}
			}
			else if (nodeName == "disable-tag") {
				body->m_disableTag = m_reader->readText();
			}
			else if (nodeName == "disable-priority") {
				body->m_disablePriority = m_reader->readInt();
			}
			else {
				warn("unknown element - " + nodeName);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag) {
			break;
		}
	}

	m_deferredBuilds.push_back({body.get(), nullptr});

	return body;
}

void SystemBuilder::readFrameLerp(btTransform& tr) {
	tr.setIdentity();
	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();
			if (name == "translationLerp")
				tr.getOrigin().setX(m_reader->readFloat());
			else if (name == "rotationLerp")
				tr.getOrigin().setY(m_reader->readFloat());
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
}

bool SystemBuilder::parseFrameType(const std::string& name, FrameType& frameType, btTransform& frame) {
	if (name == "frameInA") {
		frameType = FrameInA;
		frame = m_reader->readTransform();
	}
	else if (name == "frameInB") {
		frameType = FrameInB;
		frame = m_reader->readTransform();
	}
	else if (name == "frameInLerp") {
		frameType = FrameInLerp;
		readFrameLerp(frame);
	}
	else
		return false;
	return true;
}

void SystemBuilder::readGenericConstraintTemplate(GenericConstraintTemplate& dest) {
	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();
			if (parseFrameType(name, dest.frameType, dest.frame))
				;
			else if (name == "enableLinearSprings")
				dest.enableLinearSprings = m_reader->readBool();
			else if (name == "enableAngularSprings")
				dest.enableAngularSprings = m_reader->readBool();
			else if (name == "linearStiffnessLimited")
				dest.linearStiffnessLimited = m_reader->readBool();
			else if (name == "angularStiffnessLimited")
				dest.angularStiffnessLimited = m_reader->readBool();

			else if (name == "springDampingLimited")
				dest.springDampingLimited = m_reader->readBool();
			else if (name == "linearNonHookeanDamping")
				dest.linearNonHookeanDamping = m_reader->readVector3();
			else if (name == "angularNonHookeanDamping")
				dest.angularNonHookeanDamping = m_reader->readVector3();
			else if (name == "linearNonHookeanStiffness")
				dest.linearNonHookeanStiffness = m_reader->readVector3();
			else if (name == "angularNonHookeanStiffness")
				dest.angularNonHookeanStiffness = m_reader->readVector3();

			else if (name == "linearMotors")
				dest.linearMotors = m_reader->readBool();
			else if (name == "angularMotors")
				dest.angularMotors = m_reader->readBool();
			else if (name == "linearServoMotors")
				dest.linearServoMotors = m_reader->readBool();
			else if (name == "angularServoMotors")
				dest.angularServoMotors = m_reader->readBool();
			else if (name == "linearTargetVelocity")
				dest.linearTargetVelocity = m_reader->readVector3();
			else if (name == "angularTargetVelocity")
				dest.angularTargetVelocity = m_reader->readVector3();
			else if (name == "linearMaxMotorForce")
				dest.linearMaxMotorForce = m_reader->readVector3();
			else if (name == "angularMaxMotorForce")
				dest.angularMaxMotorForce = m_reader->readVector3();

			else if (name == "stopERP")
				dest.stopERP = m_reader->readFloat();
			else if (name == "stopCFM")
				dest.stopCFM = m_reader->readFloat();
			else if (name == "motorERP")
				dest.motorERP = m_reader->readFloat();
			else if (name == "motorCFM")
				dest.motorCFM = m_reader->readFloat();

			else if (name == "useLinearReferenceFrameA")
				dest.useLinearReferenceFrameA = m_reader->readBool();
			else if (name == "linearLowerLimit")
				dest.linearLowerLimit = m_reader->readVector3();
			else if (name == "linearUpperLimit")
				dest.linearUpperLimit = m_reader->readVector3();
			else if (name == "angularLowerLimit")
				dest.angularLowerLimit = m_reader->readVector3();
			else if (name == "angularUpperLimit")
				dest.angularUpperLimit = m_reader->readVector3();
			else if (name == "linearStiffness")
				dest.linearStiffness = m_reader->readVector3();
			else if (name == "angularStiffness")
				dest.angularStiffness = m_reader->readVector3();
			else if (name == "linearDamping")
				dest.linearDamping = m_reader->readVector3();
			else if (name == "angularDamping")
				dest.angularDamping = m_reader->readVector3();
			else if (name == "linearEquilibrium")
				dest.linearEquilibrium = m_reader->readVector3();
			else if (name == "angularEquilibrium")
				dest.angularEquilibrium = m_reader->readVector3();
			else if (name == "linearBounce")
				dest.linearBounce = m_reader->readVector3();
			else if (name == "angularBounce")
				dest.angularBounce = m_reader->readVector3();
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
}

bool SystemBuilder::findBones(const IDStr& bodyAName, const IDStr& bodyBName, PreviewBone*& bodyA, PreviewBone*& bodyB) {
	bodyA = findBoneFromIndex(bodyAName);
	bodyB = findBoneFromIndex(bodyBName);

	if (!bodyA) {
		warn("constraint " + bodyAName.str() + " <-> " + bodyBName.str() + " : bone for bodyA doesn't exist, will try to create it");
		bodyA = createBoneFromNodeName(bodyAName);
		if (!bodyA) {
			m_reader->skipCurrentElement();
			return false;
		}
	}
	if (!bodyB) {
		warn("constraint " + bodyAName.str() + " <-> " + bodyBName.str() + " : bone for bodyB doesn't exist, will try to create it");
		bodyB = createBoneFromNodeName(bodyBName);
		if (!bodyB) {
			m_reader->skipCurrentElement();
			return false;
		}
	}
	if (bodyA == bodyB) {
		warn("constraint between same object " + bodyAName.str() + " <-> " + bodyBName.str() + ", skipped");
		m_reader->skipCurrentElement();
		return false;
	}

	if (bodyA->m_rig.isKinematicObject() && bodyB->m_rig.isKinematicObject()) {
		warn("constraint between two kinematic object " + bodyAName.str() + " <-> " + bodyBName.str() + ", skipped");
		m_reader->skipCurrentElement();
		return false;
	}

	return true;
}

static btQuaternion rotFromAtoB(const btVector3& a, const btVector3& b) {
	auto axis = a.cross(b);
	if (axis.fuzzyZero())
		return btQuaternion::getIdentity();
	float sinA = axis.length();
	float cosA = a.dot(b);
	float angle = btAtan2(cosA, sinA);
	return btQuaternion(axis, angle);
}

void SystemBuilder::calcFrame(FrameType type, const btTransform& frame, const btQsTransform& trA, const btQsTransform& trB, btTransform& frameA, btTransform& frameB) {
	btQsTransform frameInWorld;
	switch (type) {
		case FrameInA:
			frameA = frame;
			frameInWorld = trA * frame;
			frameB = (trB.inverse() * frameInWorld).asTransform();
			break;
		case FrameInB:
			frameB = frame;
			frameInWorld = trB * frameB;
			frameA = (trA.inverse() * frameInWorld).asTransform();
			break;
		case FrameInLerp: {
			auto trans = trA.getOrigin().lerp(trB.getOrigin(), frame.getOrigin().x());
			auto rot = trA.getBasis().slerp(trB.getBasis(), frame.getOrigin().y());
			frameInWorld = btQsTransform(rot, trans);
			frameA = (trA.inverse() * frameInWorld).asTransform();
			frameB = (trB.inverse() * frameInWorld).asTransform();
			break;
		}
		case AWithXPointToB: {
			btMatrix3x3 matr(trA.getBasis());
			frameInWorld = trA;
			auto old = matr.getColumn(0).normalized();
			auto a2b = (trB.getOrigin() - trA.getOrigin()).normalized();
			auto q = rotFromAtoB(old, a2b);
			frameInWorld.getBasis() *= q;
			frameA = (trA.inverse() * frameInWorld).asTransform();
			frameB = (trB.inverse() * frameInWorld).asTransform();
			break;
		}
		case AWithYPointToB: {
			btMatrix3x3 matr(trA.getBasis());
			frameInWorld = trA;
			auto old = matr.getColumn(1).normalized();
			auto a2b = (trB.getOrigin() - trA.getOrigin()).normalized();
			auto q = rotFromAtoB(old, a2b);
			frameInWorld.getBasis() *= q;
			frameA = (trA.inverse() * frameInWorld).asTransform();
			frameB = (trB.inverse() * frameInWorld).asTransform();
			break;
		}
		case AWithZPointToB: {
			btMatrix3x3 matr(trA.getBasis());
			frameInWorld = trA;
			auto old = matr.getColumn(2).normalized();
			auto a2b = (trB.getOrigin() - trA.getOrigin()).normalized();
			auto q = rotFromAtoB(old, a2b);
			frameInWorld.getBasis() *= q;
			frameA = (trA.inverse() * frameInWorld).asTransform();
			frameB = (trB.inverse() * frameInWorld).asTransform();
			break;
		}
	}
}

hdt::Ref<hdt::Generic6DofConstraint> SystemBuilder::readGenericConstraint() {
	auto bodyAName = IDStr(m_reader->getAttribute("bodyA"));
	auto bodyBName = IDStr(m_reader->getAttribute("bodyB"));
	auto clsname = IDStr(m_reader->getAttribute("template", ""));

	PreviewBone *bodyA, *bodyB;
	if (!findBones(bodyAName, bodyBName, bodyA, bodyB))
		return nullptr;

	auto trA = bodyA->m_currentTransform;
	auto trB = bodyB->m_currentTransform;

	auto cinfo = getGenericConstraintTemplate(clsname);
	readGenericConstraintTemplate(cinfo);
	btTransform frameA, frameB;
	calcFrame(cinfo.frameType, cinfo.frame, trA, trB, frameA, frameB);

	if (!cinfo.linearNonHookeanDamping.isZero() || !cinfo.angularNonHookeanDamping.isZero() || !cinfo.linearNonHookeanStiffness.isZero()
		|| !cinfo.angularNonHookeanStiffness.isZero())
		warn("constraint " + bodyAName.str() + " <-> " + bodyBName.str() + " : non-Hookean spring parameters need hdtSMP64's Bullet fork, ignored");

	hdt::Ref<hdt::Generic6DofConstraint> constraint;
	if (cinfo.useLinearReferenceFrameA) {
		constraint = hdt::make_ref(new hdt::Generic6DofConstraint(bodyB, bodyA, frameB, frameA));
	}
	else {
		constraint = hdt::make_ref(new hdt::Generic6DofConstraint(bodyA, bodyB, frameA, frameB));
	}

	constraint->setLinearLowerLimit(cinfo.linearLowerLimit);
	constraint->setLinearUpperLimit(cinfo.linearUpperLimit);
	constraint->setAngularLowerLimit(cinfo.angularLowerLimit);
	constraint->setAngularUpperLimit(cinfo.angularUpperLimit);
	for (int i = 0; i < 3; ++i) {
		constraint->setStiffness(i, cinfo.linearStiffness[i], cinfo.linearStiffnessLimited);
		constraint->setStiffness(i + 3, cinfo.angularStiffness[i], cinfo.angularStiffnessLimited);
		constraint->setDamping(i, cinfo.linearDamping[i], cinfo.springDampingLimited);
		constraint->setDamping(i + 3, cinfo.angularDamping[i], cinfo.springDampingLimited);

		constraint->setEquilibriumPoint(i, cinfo.linearEquilibrium[i]);
		constraint->setEquilibriumPoint(i + 3, cinfo.angularEquilibrium[i]);

		// Non-Hookean spring terms are an hdtSMP64 extension of Bullet's
		// btGeneric6DofSpring2Constraint; vanilla Bullet has no equivalent.
		// The XML values are still parsed (see the template) but not applied.

		constraint->enableSpring(i, cinfo.enableLinearSprings);
		constraint->enableSpring(i + 3, cinfo.enableAngularSprings);

		constraint->enableMotor(i, cinfo.linearMotors);
		constraint->enableMotor(i + 3, cinfo.angularMotors);
		constraint->setServo(i, cinfo.linearServoMotors);
		constraint->setServo(i + 3, cinfo.angularServoMotors);
		// TODO: Test if servo motors go to [0, 0, 0], or whatever equilibrium is. Provide option to set servo motor target. Hard coded to equilibrium right now.
		constraint->setServoTarget(i, cinfo.linearEquilibrium[i]);
		constraint->setServoTarget(i + 3, cinfo.angularEquilibrium[i]);
		constraint->setTargetVelocity(i, cinfo.linearTargetVelocity[i]);
		constraint->setTargetVelocity(i + 3, cinfo.angularTargetVelocity[i]);
		constraint->setMaxMotorForce(i, cinfo.linearMaxMotorForce[i]);
		constraint->setMaxMotorForce(i + 3, cinfo.angularMaxMotorForce[i]);

		constraint->setParam(BT_CONSTRAINT_ERP, cinfo.motorERP, i);
		constraint->setParam(BT_CONSTRAINT_CFM, cinfo.motorCFM, i);
		constraint->setParam(BT_CONSTRAINT_STOP_ERP, cinfo.stopERP, i);
		constraint->setParam(BT_CONSTRAINT_STOP_CFM, cinfo.stopCFM, i);

		auto rotMotor = constraint->getRotationalLimitMotor(i);
		if (rotMotor) {
			rotMotor->m_motorERP = cinfo.motorERP;
			rotMotor->m_motorCFM = cinfo.motorCFM;
			rotMotor->m_stopERP = cinfo.stopERP;
			rotMotor->m_stopCFM = cinfo.stopCFM;
		}
	}
	constraint->getTranslationalLimitMotor()->m_bounce = cinfo.linearBounce;
	constraint->getRotationalLimitMotor(0)->m_bounce = cinfo.angularBounce[0];
	constraint->getRotationalLimitMotor(1)->m_bounce = cinfo.angularBounce[1];
	constraint->getRotationalLimitMotor(2)->m_bounce = cinfo.angularBounce[2];

	return constraint;
}

void SystemBuilder::readStiffSpringConstraintTemplate(StiffSpringConstraintTemplate& dest) {
	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();
			if (name == "minDistanceFactor")
				dest.minDistanceFactor = std::max(m_reader->readFloat(), 0.0f);
			else if (name == "maxDistanceFactor")
				dest.maxDistanceFactor = std::max(m_reader->readFloat(), 0.0f);
			else if (name == "stiffness")
				dest.stiffness = std::max(m_reader->readFloat(), 0.0f);
			else if (name == "damping")
				dest.damping = std::max(m_reader->readFloat(), 0.0f);
			else if (name == "equilibrium")
				dest.equilibriumFactor = btClamped(m_reader->readFloat(), 0.0f, 1.0f);
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
}

void SystemBuilder::readConeTwistConstraintTemplate(ConeTwistConstraintTemplate& dest) {
	while (m_reader->Inspect()) {
		if (m_reader->GetInspected() == hdt::XMLReader::Inspected::StartTag) {
			auto name = m_reader->GetName();
			if (parseFrameType(name, dest.frameType, dest.frame))
				;
			else if (name == "swingSpan1" || name == "coneLimit" || name == "limitZ")
				dest.swingSpan1 = std::max(m_reader->readFloat(), 0.f);
			else if (name == "swingSpan2" || name == "planeLimit" || name == "limitY")
				dest.swingSpan2 = std::max(m_reader->readFloat(), 0.f);
			else if (name == "twistSpan" || name == "twistLimit" || name == "limitX")
				dest.twistSpan = std::max(m_reader->readFloat(), 0.f);
			else if (name == "limitSoftness")
				dest.limitSoftness = btClamped(m_reader->readFloat(), 0.f, 1.f);
			else if (name == "biasFactor")
				dest.biasFactor = btClamped(m_reader->readFloat(), 0.f, 1.f);
			else if (name == "relaxationFactor")
				dest.relaxationFactor = btClamped(m_reader->readFloat(), 0.f, 1.f);
			else {
				warn("unknown element - " + name);
				m_reader->skipCurrentElement();
			}
		}
		else if (m_reader->GetInspected() == hdt::XMLReader::Inspected::EndTag)
			break;
	}
}

const SystemBuilder::BoneTemplate& SystemBuilder::getBoneTemplate(const IDStr& name) {
	auto iter = m_boneTemplates.find(name);
	if (iter == m_boneTemplates.end())
		return m_boneTemplates[IDStr()];
	return iter->second;
}

const SystemBuilder::GenericConstraintTemplate& SystemBuilder::getGenericConstraintTemplate(const IDStr& name) {
	auto iter = m_genericConstraintTemplates.find(name);
	if (iter == m_genericConstraintTemplates.end())
		return m_genericConstraintTemplates[IDStr()];
	return iter->second;
}

const SystemBuilder::StiffSpringConstraintTemplate& SystemBuilder::getStiffSpringConstraintTemplate(const IDStr& name) {
	auto iter = m_stiffSpringConstraintTemplates.find(name);
	if (iter == m_stiffSpringConstraintTemplates.end())
		return m_stiffSpringConstraintTemplates[IDStr()];
	return iter->second;
}

const SystemBuilder::ConeTwistConstraintTemplate& SystemBuilder::getConeTwistConstraintTemplate(const IDStr& name) {
	auto iter = m_coneTwistConstraintTemplates.find(name);
	if (iter == m_coneTwistConstraintTemplates.end())
		return m_coneTwistConstraintTemplates[IDStr()];
	return iter->second;
}

hdt::Ref<hdt::StiffSpringConstraint> SystemBuilder::readStiffSpringConstraint() {
	auto bodyAName = IDStr(m_reader->getAttribute("bodyA"));
	auto bodyBName = IDStr(m_reader->getAttribute("bodyB"));
	auto clsname = IDStr(m_reader->getAttribute("template", ""));

	PreviewBone *bodyA, *bodyB;
	if (!findBones(bodyAName, bodyBName, bodyA, bodyB))
		return nullptr;

	StiffSpringConstraintTemplate cinfo = getStiffSpringConstraintTemplate(clsname);
	readStiffSpringConstraintTemplate(cinfo);

	hdt::Ref<hdt::StiffSpringConstraint> constraint = hdt::make_ref(new hdt::StiffSpringConstraint(bodyA, bodyB));
	constraint->m_minDistance *= cinfo.minDistanceFactor;
	constraint->m_maxDistance *= cinfo.maxDistanceFactor;
	constraint->m_stiffness = cinfo.stiffness;
	constraint->m_damping = cinfo.damping;
	constraint->m_equilibriumPoint = constraint->m_minDistance * cinfo.equilibriumFactor + constraint->m_maxDistance * (1 - cinfo.equilibriumFactor);
	return constraint;
}

hdt::Ref<hdt::ConeTwistConstraint> SystemBuilder::readConeTwistConstraint() {
	auto bodyAName = IDStr(m_reader->getAttribute("bodyA"));
	auto bodyBName = IDStr(m_reader->getAttribute("bodyB"));
	auto clsname = IDStr(m_reader->getAttribute("template", ""));

	PreviewBone *bodyA = nullptr, *bodyB = nullptr;
	if (!findBones(bodyAName, bodyBName, bodyA, bodyB)) {
		return nullptr;
	}

	auto trA = bodyA->m_currentTransform;
	auto trB = bodyB->m_currentTransform;

	auto cinfo = getConeTwistConstraintTemplate(clsname);
	readConeTwistConstraintTemplate(cinfo);
	btTransform frameA, frameB;
	calcFrame(cinfo.frameType, cinfo.frame, trA, trB, frameA, frameB);

	hdt::Ref<hdt::ConeTwistConstraint> constraint = hdt::make_ref(new hdt::ConeTwistConstraint(bodyA, bodyB, frameA, frameB));
	constraint->setLimit(cinfo.swingSpan1, cinfo.swingSpan2, cinfo.twistSpan, cinfo.limitSoftness, cinfo.biasFactor, cinfo.relaxationFactor);

	return constraint;
}
}

#endif	// USE_BULLET
