#pragma once

#include "hdtBulletHelper.h"
#include "hdtConstraintGroup.h"

namespace hdt
{
	struct SkinnedMeshBone;
	class SkinnedMeshBody;
	class SkinnedMeshShape;
	class SkinnedMeshWorld;
	class BoneScaleConstraint;

	class SkinnedMeshSystem :
		public RefCounted
	{
		friend class hdt::SkinnedMeshWorld;

	public:
		virtual ~SkinnedMeshSystem() = default;

		virtual float prepareForRead(float timeStep) { return timeStep; }
		virtual void readTransform(float timeStep);
		virtual void writeTransform();

		void internalUpdate();

		void gather(std::vector<SkinnedMeshBody*>& bodies, std::vector<SkinnedMeshShape*>& shapes);

		bool valid() const { return !m_bones.empty(); }

		std::vector<std::shared_ptr<btCollisionShape>> m_shapeRefs;
		SkinnedMeshWorld* m_world = nullptr;

		// wind factor for the whole system; lived on the game-side subclass
		// (SkyrimSystem) upstream, but applyWind reads it generically
		float m_windFactor = 1.f;

		bool block_resetting = false;
		std::vector<Ref<SkinnedMeshBone>>& getBones() { return m_bones; };

	protected:
		std::vector<Ref<SkinnedMeshBone>> m_bones;
		std::vector<Ref<SkinnedMeshBody>> m_meshes;
		std::vector<Ref<BoneScaleConstraint>> m_constraints;
		std::vector<Ref<ConstraintGroup>> m_constraintGroups;

	private:
	};
}
