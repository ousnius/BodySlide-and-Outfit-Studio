/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#ifdef USE_BULLET

#include "PhysicsDebugDraw.h"

#include "NiflyBullet.h"

#include "../components/Mesh.h"
#include "../render/GLSurface.h"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>

namespace bsos {
namespace {
	constexpr float meshScale = 0.1f; // NIF units -> mesh units

	const nifly::Vector3 colorKinematic(0.3f, 0.5f, 1.0f);
	const nifly::Vector3 colorDynamic(0.2f, 1.0f, 0.3f);

	nifly::Vector3 ToMeshPos(const btVector3& worldPos) {
		return Mesh::TransformPosNifToMesh(FromBt(worldPos));
	}
}

// Draws one bullet collision shape at the given physics-world transform.
// Compound shapes recurse; unsupported shapes fall back to a sphere of their
// bounding radius.
void PhysicsDebugVis::drawShape(GLSurface& gls, const btCollisionShape* shape, const btTransform& transform, const nifly::Vector3& color) {
	if (!shape)
		return;

	switch (shape->getShapeType()) {
		case EMPTY_SHAPE_PROXYTYPE: return;

		case COMPOUND_SHAPE_PROXYTYPE: {
			auto compound = static_cast<const btCompoundShape*>(shape);
			for (int i = 0; i < compound->getNumChildShapes(); ++i)
				drawShape(gls, compound->getChildShape(i), transform * compound->getChildTransform(i), color);
			return;
		}

		case SPHERE_SHAPE_PROXYTYPE: {
			auto sphere = static_cast<const btSphereShape*>(shape);
			visNames.push_back(nextName());
			gls.AddVis3dSphere(ToMeshPos(transform.getOrigin()), sphere->getRadius() * meshScale, color, visNames.back());
			return;
		}

		case CAPSULE_SHAPE_PROXYTYPE: {
			auto capsule = static_cast<const btCapsuleShape*>(shape);
			const int axis = capsule->getUpAxis();
			btVector3 axisDir(0, 0, 0);
			axisDir[axis] = capsule->getHalfHeight();

			const btVector3 p1 = transform * axisDir;
			const btVector3 p2 = transform * -axisDir;
			const float radius = capsule->getRadius() * meshScale;

			visNames.push_back(nextName());
			gls.AddVis3dSphere(ToMeshPos(p1), radius, color, visNames.back());
			visNames.push_back(nextName());
			gls.AddVis3dSphere(ToMeshPos(p2), radius, color, visNames.back());
			visNames.push_back(nextName());
			gls.AddVisSeg(ToMeshPos(p1), ToMeshPos(p2), visNames.back());
			return;
		}

		case BOX_SHAPE_PROXYTYPE: {
			auto box = static_cast<const btBoxShape*>(shape);
			const btVector3 he = box->getHalfExtentsWithMargin();

			// Wireframe box from its 12 edges
			const btVector3 corners[8] = {
				{-he.x(), -he.y(), -he.z()},
				{he.x(), -he.y(), -he.z()},
				{he.x(), he.y(), -he.z()},
				{-he.x(), he.y(), -he.z()},
				{-he.x(), -he.y(), he.z()},
				{he.x(), -he.y(), he.z()},
				{he.x(), he.y(), he.z()},
				{-he.x(), he.y(), he.z()},
			};
			static const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

			for (auto& edge : edges) {
				visNames.push_back(nextName());
				gls.AddVisSeg(ToMeshPos(transform * corners[edge[0]]), ToMeshPos(transform * corners[edge[1]]), visNames.back());
			}
			return;
		}

		default: {
			// Convex hulls, cylinders etc.: bounding sphere approximation
			btVector3 center;
			btScalar radius = 0.0f;
			shape->getBoundingSphere(center, radius);

			visNames.push_back(nextName());
			gls.AddVis3dSphere(ToMeshPos(transform * center), radius * meshScale, color, visNames.back());
			return;
		}
	}
}

void PhysicsDebugVis::drawConstraint(GLSurface& gls, const btTypedConstraint* constraint, const btTransform& rootMotionInv) {
	if (!constraint)
		return;

	const btVector3 a = rootMotionInv * constraint->getRigidBodyA().getWorldTransform().getOrigin();
	const btVector3 b = rootMotionInv * constraint->getRigidBodyB().getWorldTransform().getOrigin();

	visNames.push_back(nextName());
	gls.AddVisSeg(ToMeshPos(a), ToMeshPos(b), visNames.back());
}

void PhysicsDebugVis::Update(GLSurface& gls, const std::vector<hdt::Ref<BSOSSystem>>& systems) {
	Clear(gls);

	for (auto& system : systems) {
		// The simulation runs in root-motion space (camera yaw); overlays must
		// come back to render space.
		const btTransform rootMotionInv = system->m_rootMotion.inverse();

		for (auto& bone : system->getBones()) {
			const bool kinematic = bone->m_rig.isStaticOrKinematicObject();
			const btTransform transform = rootMotionInv * bone->m_rig.getWorldTransform();
			drawShape(gls, bone->m_rig.getCollisionShape(), transform, kinematic ? colorKinematic : colorDynamic);
		}

		for (auto& constraint : system->constraints())
			drawConstraint(gls, constraint->getConstraint(), rootMotionInv);

		for (auto& group : system->constraintGroups())
			for (auto& constraint : group->m_constraints)
				drawConstraint(gls, constraint->getConstraint(), rootMotionInv);
	}
}

void PhysicsDebugVis::Clear(GLSurface& gls) {
	for (auto& name : visNames)
		gls.DeleteOverlay(name);

	visNames.clear();
}
}

#endif	// USE_BULLET
