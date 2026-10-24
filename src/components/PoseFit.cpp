/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseFit.h"
#include "MeshAnalysis.h"
#include "PoseMath.h"
#include "SurfaceFit.h"
#include "../utils/ParallelFor.h"

#include "KDMatcher.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <map>
#include <unordered_map>

using namespace nifly;

namespace PoseFit {

namespace {

// Least cosine between the normals of matched points, so the front of the body isn't matched to the back of an arm
constexpr float MinNormalAgreement = 0.3f;

// A general 3x4 transform, for blending skin transforms, which leaves them no longer rotations
struct Affine {
	double m[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

	static Affine From(const MatTransform& t) {
		Affine a;
		Vector3 o = t.ApplyTransform(Vector3());
		Vector3 axes[3] = {t.ApplyTransform(Vector3(1.0f, 0.0f, 0.0f)) - o, t.ApplyTransform(Vector3(0.0f, 1.0f, 0.0f)) - o, t.ApplyTransform(Vector3(0.0f, 0.0f, 1.0f)) - o};
		for (int c = 0; c < 3; c++) {
			a.m[0][c] = axes[c].x;
			a.m[1][c] = axes[c].y;
			a.m[2][c] = axes[c].z;
		}
		a.m[0][3] = o.x;
		a.m[1][3] = o.y;
		a.m[2][3] = o.z;
		return a;
	}

	void Add(const Affine& other, double weight) {
		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 4; c++)
				m[r][c] += other.m[r][c] * weight;
	}

	void Scale(double factor) {
		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 4; c++)
				m[r][c] *= factor;
	}

	Vector3 Apply(const Vector3& p) const {
		double out[3];
		for (int r = 0; r < 3; r++)
			out[r] = m[r][0] * p.x + m[r][1] * p.y + m[r][2] * p.z + m[r][3];
		return Vector3(static_cast<float>(out[0]), static_cast<float>(out[1]), static_cast<float>(out[2]));
	}

	Vector3 ApplyDir(const Vector3& d) const {
		double out[3];
		for (int r = 0; r < 3; r++)
			out[r] = m[r][0] * d.x + m[r][1] * d.y + m[r][2] * d.z;
		return Vector3(static_cast<float>(out[0]), static_cast<float>(out[1]), static_cast<float>(out[2]));
	}

	double Determinant() const {
		return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
			   m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
	}

	// False when the linear part can't be inverted (a vertex with weights that cancel out)
	bool Inverse(Affine& out) const {
		const double(&a)[3][4] = m;
		double c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1];
		double c01 = a[1][2] * a[2][0] - a[1][0] * a[2][2];
		double c02 = a[1][0] * a[2][1] - a[1][1] * a[2][0];
		double det = a[0][0] * c00 + a[0][1] * c01 + a[0][2] * c02;
		if (std::fabs(det) < 1e-12)
			return false;
		double inv[3][3] = {
			{c00 / det, (a[0][2] * a[2][1] - a[0][1] * a[2][2]) / det, (a[0][1] * a[1][2] - a[0][2] * a[1][1]) / det},
			{c01 / det, (a[0][0] * a[2][2] - a[0][2] * a[2][0]) / det, (a[0][2] * a[1][0] - a[0][0] * a[1][2]) / det},
			{c02 / det, (a[0][1] * a[2][0] - a[0][0] * a[2][1]) / det, (a[0][0] * a[1][1] - a[0][1] * a[1][0]) / det},
		};
		for (int r = 0; r < 3; r++) {
			for (int c = 0; c < 3; c++)
				out.m[r][c] = inv[r][c];
			out.m[r][3] = -(inv[r][0] * a[0][3] + inv[r][1] * a[1][3] + inv[r][2] * a[2][3]);
		}
		return true;
	}
};

Vector3 Normalized(Vector3 v) {
	float length = v.length();
	return length > 1e-12f ? v / length : Vector3();
}

// The bones that move a set of vertices
template<typename Influences>
std::vector<const AnimBone*> BonesOf(const Influences& perVertex) {
	std::vector<const AnimBone*> bones;
	for (auto& vertex : perVertex)
		for (auto& influence : vertex)
			if (influence.bone && std::find(bones.begin(), bones.end(), influence.bone) == bones.end())
				bones.push_back(influence.bone);
	return bones;
}

// The skin transforms of bones with the current pose
std::unordered_map<const AnimBone*, Affine> SkinTransforms(const std::vector<const AnimBone*>& bones) {
	std::unordered_map<const AnimBone*, Affine> transforms;
	for (const AnimBone* bone : bones)
		transforms[bone] = Affine::From(SkinTransform(bone));
	return transforms;
}

Affine Blend(const std::vector<Template::Influence>& influences, const std::unordered_map<const AnimBone*, Affine>& transforms) {
	Affine blended;
	double total = 0.0;
	for (auto& influence : influences) {
		auto it = transforms.find(influence.bone);
		if (it == transforms.end() || influence.weight <= 0.0f)
			continue;
		blended.Add(it->second, influence.weight);
		total += influence.weight;
	}
	if (total < 1e-6)
		return Affine::From(MatTransform());
	if (std::fabs(total - 1.0) > 1e-6)
		blended.Scale(1.0 / total);
	return blended;
}

bool IsAncestorOrSelf(const AnimBone* ancestor, const AnimBone* bone) {
	for (const AnimBone* b = bone; b; b = b->parent)
		if (b == ancestor)
			return true;
	return false;
}

int Depth(const AnimBone* bone) {
	int depth = 0;
	for (const AnimBone* b = bone->parent; b; b = b->parent)
		depth++;
	return depth;
}

// Limb bones are fitted with the skin of the rest of their limb, so an upper arm swings to where the whole arm is.
// Torso bones only use their own skin, or a spine would bend towards an arm that is still off.
bool IsLimbBone(const AnimBone* bone) {
	const std::string role = PoseMath::GetBoneRole(bone->boneName).role;
	for (const char* limb : {"upper arm", "forearm", "hand", "thigh", "calf", "foot", "toes"})
		if (role.rfind(limb, 0) == 0)
			return true;
	return bone->boneName.find("Finger") != std::string::npos || bone->boneName.find("finger") != std::string::npos;
}

bool IsFingerBone(const AnimBone* bone) {
	return bone->boneName.find("Finger") != std::string::npos || bone->boneName.find("finger") != std::string::npos;
}

// Arms and shoulders are posed the most differently between figures (A-pose, T-pose), legs of standing figures hardly
bool IsArmBone(const AnimBone* bone) {
	if (IsFingerBone(bone))
		return true;
	const std::string role = PoseMath::GetBoneRole(bone->boneName).role;
	for (const char* arm : {"clavicle", "upper arm", "forearm", "hand"})
		if (role.rfind(arm, 0) == 0)
			return true;
	return false;
}

// How far the fit may turn a bone from where it started, in degrees: about the natural range of the joint, so a bone
// whose skin matches badly can't spin away (a clavicle swinging up over the head)
float TurnLimit(const AnimBone* bone) {
	if (IsFingerBone(bone))
		return 60.0f;
	const std::string role = PoseMath::GetBoneRole(bone->boneName).role;
	struct Limit {
		const char* role;
		float degrees;
	};
	for (const Limit& limit : {Limit{"pelvis", 25.0f},
							   Limit{"spine", 25.0f},
							   Limit{"neck", 40.0f},
							   Limit{"head", 30.0f},
							   Limit{"clavicle", 30.0f},
							   Limit{"upper arm", 110.0f},
							   Limit{"forearm", 110.0f},
							   Limit{"hand", 70.0f},
							   Limit{"thigh", 60.0f},
							   Limit{"calf", 60.0f},
							   Limit{"foot", 40.0f},
							   Limit{"toes", 40.0f}})
		if (role.rfind(limit.role, 0) == 0)
			return limit.degrees;
	return 30.0f;
}

// The angle in degrees between two rotations of a bone, given as pose rotation vectors
float TurnBetween(const Vector3& a, const Vector3& b) {
	Matrix3 relative = RotVecToMat(a) * RotVecToMat(b).Transpose();
	float trace = relative[0][0] + relative[1][1] + relative[2][2];
	return std::acos(std::clamp((trace - 1.0f) / 2.0f, -1.0f, 1.0f)) / DEG2RAD;
}

// The rotation vector (axis times angle in radians, right-handed) of a rotation matrix
Vector3 RotationVector(const double (&r)[3][3]) {
	double trace = r[0][0] + r[1][1] + r[2][2];
	double angle = std::acos(std::clamp((trace - 1.0) / 2.0, -1.0, 1.0));
	Vector3 axis(static_cast<float>(r[2][1] - r[1][2]), static_cast<float>(r[0][2] - r[2][0]), static_cast<float>(r[1][0] - r[0][1]));
	if (axis.length() < 1e-9f)
		return Vector3();
	return Normalized(axis) * static_cast<float>(angle);
}

struct Match {
	size_t vertex = 0; // in the combined templates
	Vector3 target;
	float distance = 0.0f;
	// Found from the figure's side: the template point closest to a figure point
	bool fromTarget = false;
};

// The templates as one set of points with their influences and normals
struct Combined {
	std::vector<Vector3> rest;
	std::vector<Vector3> restNormals;
	std::vector<const std::vector<Template::Influence>*> influences;
};

Combined Combine(const std::vector<Template>& templates) {
	Combined c;
	for (auto& t : templates) {
		std::vector<Vector3> normals = SurfaceFit::VertexNormals(t.verts, t.tris, 1e-4f);
		for (size_t i = 0; i < t.verts.size(); i++) {
			c.rest.push_back(t.verts[i]);
			c.restNormals.push_back(normals[i]);
			c.influences.push_back(i < t.influences.size() ? &t.influences[i] : nullptr);
		}
	}
	return c;
}

// The height where the legs of a standing figure part: the top of the slices in which the points near the middle
// form two sides with a gap between them. Arms hanging at the sides are left out by only looking near the middle.
// Below the floor when there's no such gap, e.g. under a long dress.
// Meshes are cut with each slice's plane, so the test doesn't depend on how dense they are.
float CrotchHeight(const std::vector<const std::vector<Vector3>*>& vertsOf, const std::vector<const std::vector<Triangle>*>& trisOf) {
	float minZ = FLT_MAX, maxZ = -FLT_MAX;
	std::vector<float> xs;
	for (auto* verts : vertsOf)
		for (auto& p : *verts) {
			minZ = std::min(minZ, p.z);
			maxZ = std::max(maxZ, p.z);
			xs.push_back(p.x);
		}
	if (xs.empty())
		return -FLT_MAX;
	std::nth_element(xs.begin(), xs.begin() + xs.size() / 2, xs.end());
	const float center = xs[xs.size() / 2];
	const float height = maxZ - minZ;
	if (height <= 0.0f)
		return -FLT_MAX;

	const int slices = 200;
	const float sliceHeight = height / slices;
	const float near = height * 0.12f;	// how far from the middle the legs are looked for
	const float gap = height * 0.006f;	// the free space between the legs
	std::vector<char> left(slices, 0), right(slices, 0), middle(slices, 0);
	for (size_t m = 0; m < vertsOf.size(); m++) {
		const auto& verts = *vertsOf[m];
		for (auto& t : *trisOf[m]) {
			if (t.p1 >= verts.size() || t.p2 >= verts.size() || t.p3 >= verts.size())
				continue;
			const Vector3 c[3] = {verts[t.p1], verts[t.p2], verts[t.p3]};
			const float lo = std::min({c[0].z, c[1].z, c[2].z}), hi = std::max({c[0].z, c[1].z, c[2].z});
			int first = std::max(0, static_cast<int>(std::ceil((lo - minZ) / sliceHeight - 0.5f)));
			int last = std::min(slices - 1, static_cast<int>(std::floor((hi - minZ) / sliceHeight - 0.5f)));
			for (int s = first; s <= last; s++) {
				// Where the triangle crosses the slice's middle plane, across the body
				const float z = minZ + (s + 0.5f) * sliceHeight;
				float a = FLT_MAX, b = -FLT_MAX;
				for (int e = 0; e < 3; e++) {
					const Vector3& p = c[e];
					const Vector3& q = c[(e + 1) % 3];
					if ((p.z - z) * (q.z - z) > 0.0f || p.z == q.z)
						continue;
					float x = p.x + (q.x - p.x) * (z - p.z) / (q.z - p.z) - center;
					a = std::min(a, x);
					b = std::max(b, x);
				}
				if (a > b || b < -near || a > near)
					continue;
				if (a < gap && b > -gap)
					middle[s] = 1;
				if (a < -gap)
					left[s] = 1;
				if (b > gap)
					right[s] = 1;
			}
		}
	}
	std::vector<char> parted(slices, 0);
	for (int s = 0; s < slices; s++)
		parted[s] = left[s] && right[s] && !middle[s];

	// The highest parted slice of the lower body: knees or thighs that touch break the gap lower down, and above the
	// crotch the torso has skin in the middle
	for (int s = static_cast<int>(slices * 0.65f); s >= static_cast<int>(slices * 0.1f); s--)
		if (parted[s])
			return minZ + (s + 1) * sliceHeight;
	return -FLT_MAX;
}

} // namespace

MatTransform SkinTransform(const AnimBone* bone) {
	return bone->xformPoseToGlobal.ComposeTransforms(bone->xformToGlobal.InverseTransform());
}

std::vector<Vector3> PoseVerts(const Template& skin) {
	auto transforms = SkinTransforms(BonesOf(skin.influences));
	std::vector<Vector3> posed(skin.verts.size());
	for (size_t i = 0; i < skin.verts.size(); i++)
		posed[i] = i < skin.influences.size() ? Blend(skin.influences[i], transforms).Apply(skin.verts[i]) : skin.verts[i];
	return posed;
}

Result Fit(const std::vector<Template>& templates, const std::vector<Vector3>& targetVerts, const std::vector<Triangle>& targetTris, const Options& options) {
	Result result;
	if (options.bones.empty() || targetVerts.empty() || targetTris.empty())
		return result;

	// Parents first
	std::vector<AnimBone*> bones = options.bones;
	std::stable_sort(bones.begin(), bones.end(), [](const AnimBone* a, const AnimBone* b) { return Depth(a) < Depth(b); });
	AnimBone* root = bones.front();
	bool rootIsAbove = std::all_of(bones.begin(), bones.end(), [root](const AnimBone* b) { return IsAncestorOrSelf(root, b); });

	const Combined skin = Combine(templates);
	const size_t count = skin.rest.size();
	std::vector<const AnimBone*> skinBones;
	for (auto& t : templates)
		for (const AnimBone* bone : BonesOf(t.influences))
			if (std::find(skinBones.begin(), skinBones.end(), bone) == skinBones.end())
				skinBones.push_back(bone);

	// Each vertex belongs to the fitted bone that moves it most, or the closest fitted bone above that
	std::vector<int> owner(count, -1);
	for (size_t i = 0; i < count; i++) {
		if (!skin.influences[i])
			continue;
		const Template::Influence* strongest = nullptr;
		for (auto& influence : *skin.influences[i])
			if (influence.bone && (!strongest || influence.weight > strongest->weight))
				strongest = &influence;
		if (!strongest)
			continue;
		for (const AnimBone* b = strongest->bone; b && owner[i] < 0; b = b->parent) {
			auto it = std::find(bones.begin(), bones.end(), b);
			if (it != bones.end())
				owner[i] = static_cast<int>(it - bones.begin());
		}
	}

	// Which bones' skin each bone is fitted with
	const size_t boneCount = bones.size();
	std::vector<std::vector<char>> uses(boneCount, std::vector<char>(boneCount, 0));
	for (size_t k = 0; k < boneCount; k++) {
		const bool limb = IsLimbBone(bones[k]);
		for (size_t o = 0; o < boneCount; o++) {
			if (k == o)
				uses[k][o] = 1;
			else if (k == 0 && rootIsAbove && options.moveRoot)
				// The body as a whole goes by the torso, not by limbs that are still off
				uses[k][o] = !IsLimbBone(bones[o]);
			else if (limb && IsAncestorOrSelf(bones[k], bones[o]))
				uses[k][o] = 1;
		}
	}

	const std::vector<Vector3> targetNormals = SurfaceFit::VertexNormals(targetVerts, targetTris, 1e-4f);
	const SurfaceFit::SurfaceQuery target(targetVerts, targetTris, targetNormals);

	// Figure points that look for their closest template point, spread evenly when there are many
	std::vector<size_t> targetSamples;
	const size_t step = std::max<size_t>(1, targetVerts.size() / 30000);
	for (size_t j = 0; j < targetVerts.size(); j += step)
		targetSamples.push_back(j);

	std::vector<Vector3> startRotation(boneCount);
	std::vector<float> startScale(boneCount);
	for (size_t k = 0; k < boneCount; k++) {
		startRotation[k] = bones[k]->poseRotVec;
		startScale[k] = bones[k]->poseScale;
	}

	auto posedAll = [&](std::vector<Vector3>& positions, std::vector<Vector3>& normals) {
		auto transforms = SkinTransforms(skinBones);
		positions.resize(count);
		normals.resize(count);
		ParallelFor(count, 1024, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				if (!skin.influences[i]) {
					positions[i] = skin.rest[i];
					normals[i] = skin.restNormals[i];
					continue;
				}
				Affine blended = Blend(*skin.influences[i], transforms);
				positions[i] = blended.Apply(skin.rest[i]);
				normals[i] = Normalized(blended.ApplyDir(skin.restNormals[i]));
			}
		});
	};

	// Mean distance of the posed template to the figure, over the points that find it
	auto measure = [&](size_t* outMatches) {
		std::vector<Vector3> positions, normals;
		posedAll(positions, normals);
		double sum = 0.0;
		size_t matched = 0;
		for (size_t i = 0; i < count; i++) {
			if (owner[i] < 0)
				continue;
			auto hit = target.Closest(positions[i], options.maxDistance);
			if (!hit.found || normals[i].dot(hit.normal) < MinNormalAgreement)
				continue;
			sum += positions[i].DistanceTo(hit.point);
			matched++;
		}
		if (outMatches)
			*outMatches = matched;
		return matched ? static_cast<float>(sum / static_cast<double>(matched)) : 0.0f;
	};

	result.distanceBefore = measure(nullptr);
	std::vector<size_t> lastMatches(boneCount, 0);

	// The body as a whole first: its place, turn and size, by the torso. Then the joints, once it's there.
	const int wholeBodyIterations = rootIsAbove && options.moveRoot ? std::max(3, options.iterations / 2) : 0;
	for (int iteration = 0; iteration < wholeBodyIterations + std::max(1, options.iterations); iteration++) {
		std::vector<Vector3> positions, normals;
		posedAll(positions, normals);

		// Matches both ways: template points to the figure, and figure points to the template. The second pulls a
		// limb towards where the figure's limb is when the closest figure surface of the template's limb is the torso.
		std::vector<std::vector<Match>> matchesOf(boneCount);
		std::vector<Match> forward(count);
		std::vector<char> hasForward(count, 0);
		ParallelFor(count, 1024, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				if (owner[i] < 0)
					continue;
				auto hit = target.Closest(positions[i], options.maxDistance);
				if (!hit.found || normals[i].dot(hit.normal) < MinNormalAgreement || !SurfaceFit::SurfaceQuery::CleanPull(positions[i], hit))
					continue;
				forward[i] = {i, hit.point, positions[i].DistanceTo(hit.point)};
				hasForward[i] = 1;
			}
		});
		for (size_t i = 0; i < count; i++)
			if (hasForward[i])
				matchesOf[owner[i]].push_back(forward[i]);

		std::vector<Vector3> ownedPositions;
		std::vector<size_t> ownedVertex;
		for (size_t i = 0; i < count; i++)
			if (owner[i] >= 0) {
				ownedPositions.push_back(positions[i]);
				ownedVertex.push_back(i);
			}
		if (!ownedPositions.empty()) {
			kd_tree<uint32_t> tree(ownedPositions.data(), static_cast<uint32_t>(ownedPositions.size()));
			for (size_t j : targetSamples) {
				Vector3 point = targetVerts[j];
				if (tree.kd_nn(&point, 0.0f) == 0)
					continue;
				size_t i = ownedVertex[tree.queryResult.front().vertex_index];
				float distance = positions[i].DistanceTo(point);
				if (distance > options.maxDistance || normals[i].dot(targetNormals[j]) < MinNormalAgreement)
					continue;
				// Only figure points in front of or behind the template's skin, not beside it: the figure's head above
				// the open neck of a body without one would pull the body up
				if (distance > 1e-3f && std::fabs((point - positions[i]).dot(normals[i])) / distance < 0.6f)
					continue;
				matchesOf[owner[i]].push_back({i, point, distance, true});
			}
		}

		// How well the skin of bones matches the figure now, both ways: template points to the closest figure surface,
		// and figure points to the template points they matched. Spread over at most a few thousand matches.
		// The worst matches are left out as in the fits, so parts only one side has (the figure's head over a body
		// without one) don't pull.
		auto matchError = [&](const std::vector<Match>& matches, bool templateSideOnly) {
			auto transforms = SkinTransforms(skinBones);
			const size_t stride = std::max<size_t>(1, matches.size() / 3000);
			std::vector<float> forward, backward;
			for (size_t m = 0; m < matches.size(); m += stride) {
				const Match& match = matches[m];
				if (templateSideOnly && match.fromTarget)
					continue;
				Vector3 p = Blend(*skin.influences[match.vertex], transforms).Apply(skin.rest[match.vertex]);
				if (match.fromTarget)
					backward.push_back(std::min(p.DistanceTo(match.target), options.maxDistance));
				else {
					auto hit = target.Closest(p, options.maxDistance);
					forward.push_back(hit.found ? p.DistanceTo(hit.point) : options.maxDistance);
				}
			}
			auto trimmedMean = [&](std::vector<float>& values) {
				if (values.empty())
					return 0.0;
				std::sort(values.begin(), values.end());
				size_t keep = std::max<size_t>(1, static_cast<size_t>(static_cast<float>(values.size()) * (1.0f - std::clamp(options.trim, 0.0f, 0.9f))));
				double sum = 0.0;
				for (size_t i = 0; i < keep; i++)
					sum += values[i];
				return sum / static_cast<double>(keep);
			};
			return trimmedMean(forward) + trimmedMean(backward);
		};

		// Tries the bone a little smaller and larger, and keeps the size that matches best
		auto searchScale = [&](AnimBone* bone, const std::vector<Match>& matches, float step) {
			const float start = bone->poseScale;
			float best = start;
			double bestError = matchError(matches, false);
			for (float factor : {1.0f / step, step}) {
				float scale = std::clamp(start * factor, options.minScale, options.maxScale);
				if (scale == start)
					continue;
				bone->poseScale = scale;
				bone->UpdatePoseTransform();
				double error = matchError(matches, false);
				if (error < bestError) {
					bestError = error;
					best = scale;
				}
			}
			bone->poseScale = best;
			bone->UpdatePoseTransform();
		};

		const bool wholeBodyOnly = iteration < wholeBodyIterations;
		const int jointIteration = iteration - wholeBodyIterations;

		// The best fit of the matches of some bones, with the template points where the bones fitted so far put them
		// now and the worst matches left out: parts of the figure the template doesn't have, or the other way round.
		// pivot pins the joint, so the fit only turns and scales about it.
		auto fitMatches = [&](const std::vector<Match>& matches, const Vector3* pivot, bool withScale, MeshAnalysis::SimilarityTransform& outFit) {
			if (matches.size() < 6)
				return false;
			auto transforms = SkinTransforms(skinBones);
			std::vector<Vector3> from(matches.size()), to(matches.size());
			std::vector<float> distance(matches.size());
			for (size_t m = 0; m < matches.size(); m++) {
				from[m] = Blend(*skin.influences[matches[m].vertex], transforms).Apply(skin.rest[matches[m].vertex]);
				to[m] = matches[m].target;
				distance[m] = from[m].DistanceTo(to[m]);
			}

			std::vector<size_t> order(matches.size());
			for (size_t m = 0; m < order.size(); m++)
				order[m] = m;
			std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return distance[a] < distance[b]; });
			size_t keep = std::max<size_t>(6, static_cast<size_t>(static_cast<float>(order.size()) * (1.0f - std::clamp(options.trim, 0.0f, 0.9f))));
			keep = std::min(keep, order.size());

			std::vector<Vector3> fromKept, toKept;
			std::vector<float> weights;
			for (size_t m = 0; m < keep; m++) {
				fromKept.push_back(from[order[m]]);
				toKept.push_back(to[order[m]]);
				weights.push_back(1.0f);
			}
			if (pivot) {
				fromKept.push_back(*pivot);
				toKept.push_back(*pivot);
				weights.push_back(10.0f * static_cast<float>(keep));
			}
			outFit = MeshAnalysis::BestFitTransform(fromKept, toKept, &weights, withScale);
			return true;
		};

		for (size_t k = 0; k < boneCount; k++) {
			AnimBone* bone = bones[k];
			const bool moveWhole = k == 0 && rootIsAbove && options.moveRoot;

			if (wholeBodyOnly && !moveWhole)
				continue;
			// Fingers only once the hands are in place, or they curl into the wrong fingers of the figure
			if (IsFingerBone(bone) && jointIteration < options.iterations / 2)
				continue;

			// The turn comes from the skin of the bone and, for limbs, the rest of the limb below it
			std::vector<Match> matches;
			for (size_t o = 0; o < boneCount; o++)
				if (uses[k][o])
					matches.insert(matches.end(), matchesOf[o].begin(), matchesOf[o].end());

			const Vector3 pivot = bone->xformPoseToGlobal.translation;
			// The body's size by trying sizes against how well both ways match: a least squares scale from closest
			// points shrinks, since closest points crowd together, and one way alone shrinks to nothing
			if (moveWhole && options.scaleBody) {
				for (float step : {1.1f, 1.03f, 1.01f}) {
					const float startScale = bone->poseScale;
					float best = startScale;
					double bestError = matchError(matches, false);
					for (float factor : {1.0f / step, step}) {
						bone->poseScale = startScale * factor;
						bone->UpdatePoseTransform();
						double error = matchError(matches, false);
						if (error < bestError) {
							bestError = error;
							best = bone->poseScale;
						}
					}
					bone->poseScale = best;
					bone->UpdatePoseTransform();
				}
			}

			MeshAnalysis::SimilarityTransform fit;
			if (fitMatches(matches, moveWhole ? nullptr : &pivot, false, fit)) {
				Vector3 rotation = RotationVector(fit.rotation);
				const float maxTurn = options.maxTurnDegrees * DEG2RAD;
				if (rotation.length() > maxTurn)
					rotation = Normalized(rotation) * maxTurn;

				// Within the joint's natural range from where the bone started, or as far towards it as that allows
				const Vector3 before = bone->poseRotVec;
				for (float share : {1.0f, 0.5f, 0.25f, 0.0f}) {
					bone->poseRotVec = before;
					bone->UpdatePoseTransform();
					if (share > 0.0f && !rotation.IsZero())
						PoseMath::TurnBoneGlobal(bone, rotation * share);
					if (TurnBetween(bone->poseRotVec, startRotation[k]) <= TurnLimit(bone))
						break;
				}
				if (TurnBetween(bone->poseRotVec, startRotation[k]) > TurnLimit(bone)) {
					bone->poseRotVec = before;
					bone->UpdatePoseTransform();
				}

				if (moveWhole) {
					// What the fit does to the joint, beyond the turn about it
					Vector3 shift = fit.Apply(pivot) - pivot;
					const float maxShift = options.maxDistance * 0.5f;
					if (shift.length() > maxShift)
						shift = Normalized(shift) * maxShift;
					if (!shift.IsZero())
						PoseMath::ShiftBoneGlobal(bone, shift);
				}
			}

			// Proportions after the pose: a bone a little larger or smaller, by what matches its own skin best, so the
			// next bone of a limb can't make up for it. Fingers have too little skin to tell.
			if (options.allowScale && !moveWhole && !IsFingerBone(bone) && matchesOf[k].size() >= 50 && jointIteration >= options.iterations / 3)
				searchScale(bone, matchesOf[k], 1.02f);

			lastMatches[k] = matchesOf[k].size();
		}
	}

	result.distanceAfter = measure(&result.matchesAfter);

	// Per bone: how far its own skin is from the figure now, and how far it was turned
	std::vector<Vector3> positions, normals;
	posedAll(positions, normals);
	std::vector<double> sums(boneCount, 0.0);
	std::vector<size_t> counts(boneCount, 0);
	for (size_t i = 0; i < count; i++) {
		if (owner[i] < 0)
			continue;
		auto hit = target.Closest(positions[i], options.maxDistance);
		if (!hit.found || normals[i].dot(hit.normal) < MinNormalAgreement)
			continue;
		sums[owner[i]] += positions[i].DistanceTo(hit.point);
		counts[owner[i]]++;
	}
	for (size_t k = 0; k < boneCount; k++) {
		BoneFit fit;
		fit.bone = bones[k];
		fit.matches = lastMatches[k];
		fit.distance = counts[k] ? static_cast<float>(sums[k] / static_cast<double>(counts[k])) : 0.0f;
		fit.turnedDegrees = TurnBetween(bones[k]->poseRotVec, startRotation[k]);
		fit.scale = startScale[k] > 0.0f ? bones[k]->poseScale / startScale[k] : 1.0f;
		result.bones.push_back(fit);
	}
	return result;
}

ScaleEstimate EstimateScale(const std::vector<Template>& templates, const std::vector<Vector3>& targetVerts, const std::vector<Triangle>& targetTris) {
	ScaleEstimate estimate;
	if (targetVerts.empty() || targetTris.empty())
		return estimate;

	// Template skin of the legs and torso: the main bone of each vertex's strongest weight isn't an arm
	std::vector<Vector3> templatePoints;
	for (auto& t : templates)
		for (size_t i = 0; i < t.verts.size() && i < t.influences.size(); i++) {
			const Template::Influence* strongest = nullptr;
			for (auto& influence : t.influences[i])
				if (influence.bone && (!strongest || influence.weight > strongest->weight))
					strongest = &influence;
			if (!strongest)
				continue;
			const AnimBone* main = strongest->bone;
			while (main && !PoseMath::IsMainBone(main->boneName) && !IsFingerBone(main))
				main = main->parent;
			if (main && IsArmBone(main))
				continue;
			templatePoints.push_back(t.verts[i]);
		}
	if (templatePoints.empty())
		return estimate;

	// The feet: the middle of the points near the bottom
	auto floorOf = [](const std::vector<Vector3>& points) {
		float minZ = points.front().z, maxZ = points.front().z;
		for (auto& p : points) {
			minZ = std::min(minZ, p.z);
			maxZ = std::max(maxZ, p.z);
		}
		const float band = minZ + (maxZ - minZ) * 0.03f;
		Vector3 sum;
		size_t count = 0;
		for (auto& p : points)
			if (p.z <= band) {
				sum += p;
				count++;
			}
		Vector3 floor = sum / static_cast<float>(std::max<size_t>(count, 1));
		floor.z = minZ;
		return floor;
	};
	estimate.templateFloor = floorOf(templatePoints);
	estimate.figureFloor = floorOf(targetVerts);

	auto sample = [](const std::vector<Vector3>& points, size_t most) {
		std::vector<Vector3> out;
		const size_t step = std::max<size_t>(1, points.size() / most);
		for (size_t i = 0; i < points.size(); i += step)
			out.push_back(points[i]);
		return out;
	};
	const std::vector<Vector3> templateSamples = sample(templatePoints, 4000);
	const std::vector<Vector3> figureSamples = sample(targetVerts, 4000);
	const SurfaceFit::SurfaceQuery target(targetVerts, targetTris, std::vector<Vector3>());

	// Both ways, in the templates' units, with the worst matches left out: parts only one side has, like the
	// figure's arms and head against a body template without them
	auto trimmedMean = [](std::vector<float>& values) {
		if (values.empty())
			return 0.0;
		std::sort(values.begin(), values.end());
		const size_t keep = std::max<size_t>(1, values.size() * 7 / 10);
		double sum = 0.0;
		for (size_t i = 0; i < keep; i++)
			sum += values[i];
		return sum / static_cast<double>(keep);
	};
	float templateTop = estimate.templateFloor.z;
	for (auto& p : templatePoints)
		templateTop = std::max(templateTop, p.z);

	// The error of a scale, with the figure's best offset across the floor for it (feet differ, like boots and bare
	// feet, so the middles of the feet don't line up the bodies exactly). Figure points above the top of the
	// templates don't count: a figure with a head over a body without one would otherwise shrink until the head
	// matched the shoulders.
	auto evaluate = [&](float scale, Vector3* outOffset) {
		Vector3 offset;
		std::vector<Vector3> moved(templateSamples.size());
		std::vector<float> forward(moved.size());
		std::vector<Vector3> lines(moved.size());
		std::vector<char> found(moved.size(), 0);
		for (int pass = 0; pass < 3; pass++) {
			// The templates moved into the figure's space, which is the same as scaling the figure the other way
			for (size_t i = 0; i < moved.size(); i++)
				moved[i] = estimate.figureFloor + (templateSamples[i] - estimate.templateFloor) / scale - offset / scale;

			ParallelFor(moved.size(), 256, [&](size_t start, size_t end) {
				for (size_t i = start; i < end; i++) {
					auto hit = target.Closest(moved[i], 1e6f);
					found[i] = hit.found ? 1 : 0;
					forward[i] = hit.found ? moved[i].DistanceTo(hit.point) * scale : 1e6f;
					lines[i] = hit.found ? (hit.point - moved[i]) * scale : Vector3();
				}
			});
			if (pass == 2)
				break;

			// Shift across the floor by the median of the matches' offsets
			std::vector<float> xs, ys;
			for (size_t i = 0; i < moved.size(); i++)
				if (found[i]) {
					xs.push_back(lines[i].x);
					ys.push_back(lines[i].y);
				}
			if (xs.empty())
				break;
			std::nth_element(xs.begin(), xs.begin() + xs.size() / 2, xs.end());
			std::nth_element(ys.begin(), ys.begin() + ys.size() / 2, ys.end());
			offset.x -= xs[xs.size() / 2];
			offset.y -= ys[ys.size() / 2];
		}

		kd_tree<uint32_t> tree(moved.data(), static_cast<uint32_t>(moved.size()));
		std::vector<float> backward;
		const float top = templateTop + (templateTop - estimate.templateFloor.z) * 0.02f;
		for (auto& p : figureSamples) {
			if (estimate.templateFloor.z + (p.z - estimate.figureFloor.z) * scale > top)
				continue;
			Vector3 point = p;
			if (tree.kd_nn(&point, 0.0f) == 0)
				continue;
			backward.push_back(point.DistanceTo(*tree.queryResult.front().v) * scale);
		}
		if (outOffset)
			*outOffset = offset;
		return trimmedMean(forward) + trimmedMean(backward);
	};
	auto error = [&](float scale) { return evaluate(scale, nullptr); };

	// Standing figures: the ratio of their leg lengths, from the floor to where the legs part
	std::vector<const std::vector<Vector3>*> templateVerts;
	std::vector<const std::vector<Triangle>*> templateTris;
	for (auto& t : templates) {
		templateVerts.push_back(&t.verts);
		templateTris.push_back(&t.tris);
	}
	const float templateCrotch = CrotchHeight(templateVerts, templateTris);
	const float figureCrotch = CrotchHeight({&targetVerts}, {&targetTris});
	estimate.templateCrotch = templateCrotch;
	estimate.figureCrotch = figureCrotch;
	if (templateCrotch > estimate.templateFloor.z && figureCrotch > estimate.figureFloor.z) {
		estimate.scale = (templateCrotch - estimate.templateFloor.z) / (figureCrotch - estimate.figureFloor.z);
		estimate.byLegs = true;
		Vector3 offset;
		estimate.distance = static_cast<float>(evaluate(estimate.scale, &offset) / 2.0);
		estimate.templateFloor += offset;
		return estimate;
	}

	// Else the size that matches best, coarse over a wide range of sizes (a figure in meters, centimeters or the
	// game's units), then finer
	float best = 1.0f;
	double bestError = error(best);
	for (int i = -40; i <= 40; i++) {
		float scale = std::pow(10.0f, i / 40.0f * 1.5f);
		double e = error(scale);
		if (e < bestError) {
			bestError = e;
			best = scale;
		}
	}
	for (float step = 1.05f; step > 1.0005f; step = std::sqrt(step)) {
		bool improved = true;
		while (improved) {
			improved = false;
			for (float factor : {step, 1.0f / step}) {
				double e = error(best * factor);
				if (e < bestError) {
					bestError = e;
					best *= factor;
					improved = true;
				}
			}
		}
	}

	estimate.scale = best;
	Vector3 offset;
	estimate.distance = static_cast<float>(evaluate(best, &offset) / 2.0);
	// Where the figure's feet go: the templates' feet, shifted across the floor by the best offset
	estimate.templateFloor += offset;
	return estimate;
}

std::vector<std::vector<Template::Influence>> WeightsFromTemplates(const std::vector<Template>& templates, const std::vector<Vector3>& points, float searchRadius) {
	std::vector<std::vector<Template::Influence>> weights(points.size());

	struct Posed {
		std::vector<Vector3> verts;
		std::unique_ptr<SurfaceFit::SurfaceQuery> query;
	};
	std::vector<Posed> posed;
	for (auto& t : templates) {
		Posed p;
		p.verts = PoseVerts(t);
		p.query = std::make_unique<SurfaceFit::SurfaceQuery>(p.verts, t.tris, std::vector<Vector3>());
		posed.push_back(std::move(p));
	}

	ParallelFor(points.size(), 512, [&](size_t start, size_t end) {
		for (size_t i = start; i < end; i++) {
			int best = -1;
			SurfaceFit::SurfaceQuery::Hit bestHit;
			float bestDistance = 0.0f;
			for (size_t t = 0; t < posed.size(); t++) {
				auto hit = posed[t].query->Closest(points[i], searchRadius);
				if (!hit.found)
					continue;
				float distance = points[i].DistanceTo(hit.point);
				if (best < 0 || distance < bestDistance) {
					best = static_cast<int>(t);
					bestHit = hit;
					bestDistance = distance;
				}
			}
			if (best < 0)
				continue;

			// Blend the influences of the triangle's corners
			const Template& t = templates[best];
			const Triangle& tri = t.tris[bestHit.triangle];
			const uint16_t corners[3] = {tri.p1, tri.p2, tri.p3};
			std::map<AnimBone*, float> blended;
			for (int c = 0; c < 3; c++) {
				if (corners[c] >= t.influences.size())
					continue;
				for (auto& influence : t.influences[corners[c]])
					blended[influence.bone] += influence.weight * bestHit.bary[c];
			}
			float total = 0.0f;
			for (auto& b : blended)
				total += b.second;
			if (total <= 0.0f)
				continue;
			for (auto& b : blended)
				if (b.second > 1e-4f)
					weights[i].push_back({b.first, b.second / total});
		}
	});
	return weights;
}

void SmoothWeights(std::vector<std::vector<Template::Influence>>& weights, const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, int iterations, float strength) {
	const size_t n = std::min(weights.size(), verts.size());
	if (n == 0 || iterations <= 0 || strength <= 0.0f)
		return;

	std::vector<std::vector<int>> neighbors(n);
	auto link = [&](size_t a, size_t b) {
		if (a >= n || b >= n || a == b)
			return;
		neighbors[a].push_back(static_cast<int>(b));
		neighbors[b].push_back(static_cast<int>(a));
	};
	for (auto& t : tris) {
		link(t.p1, t.p2);
		link(t.p2, t.p3);
		link(t.p3, t.p1);
	}
	std::vector<Vector3> points(verts.begin(), verts.begin() + n);
	for (auto& group : MeshAnalysis::CoincidentGroups(points, 1e-4f))
		for (size_t i = 1; i < group.size(); i++)
			link(group[0], group[i]);
	for (auto& nb : neighbors) {
		std::sort(nb.begin(), nb.end());
		nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
	}

	using WeightMap = std::map<AnimBone*, float>;
	std::vector<WeightMap> current(n);
	for (size_t i = 0; i < n; i++)
		for (auto& influence : weights[i])
			current[i][influence.bone] += influence.weight;

	const float s = std::clamp(strength, 0.0f, 1.0f);
	for (int it = 0; it < iterations; it++) {
		std::vector<WeightMap> next(n);
		ParallelFor(n, 1024, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				if (neighbors[i].empty() || current[i].empty()) {
					next[i] = current[i];
					continue;
				}
				WeightMap mean;
				size_t weighted = 0;
				for (int j : neighbors[i]) {
					if (current[j].empty())
						continue;
					for (auto& w : current[j])
						mean[w.first] += w.second;
					weighted++;
				}
				if (weighted == 0) {
					next[i] = current[i];
					continue;
				}
				WeightMap& out = next[i];
				for (auto& w : current[i])
					out[w.first] += w.second * (1.0f - s);
				for (auto& w : mean)
					out[w.first] += w.second / static_cast<float>(weighted) * s;
			}
		});
		current.swap(next);
	}

	for (size_t i = 0; i < n; i++) {
		weights[i].clear();
		float total = 0.0f;
		for (auto& w : current[i])
			total += w.second;
		if (total <= 0.0f)
			continue;
		for (auto& w : current[i])
			if (w.second / total > 1e-4f)
				weights[i].push_back({w.first, w.second / total});
	}
}

std::vector<Vector3> Unpose(const std::vector<Vector3>& posedPoints, const std::vector<std::vector<Template::Influence>>& weights) {
	auto transforms = SkinTransforms(BonesOf(weights));

	std::unordered_map<const AnimBone*, Affine> inverses;
	std::unordered_map<const AnimBone*, double> determinants;
	for (auto& t : transforms) {
		Affine inverse;
		if (t.second.Inverse(inverse))
			inverses[t.first] = inverse;
		determinants[t.first] = t.second.Determinant();
	}

	std::vector<Vector3> rest(posedPoints.size());
	for (size_t i = 0; i < posedPoints.size(); i++) {
		rest[i] = posedPoints[i];
		if (i >= weights.size() || weights[i].empty())
			continue;

		// Where bones turned different ways share a vertex (two fingers, a knee bent far), the blend loses most of its
		// volume and its inverse throws the vertex far off. There the blend of the bones' inverses is used, which is
		// close and always well-behaved.
		Affine blended = Blend(weights[i], transforms);
		double expected = 0.0, total = 0.0;
		for (auto& influence : weights[i]) {
			auto d = determinants.find(influence.bone);
			if (d != determinants.end()) {
				expected += d->second * influence.weight;
				total += influence.weight;
			}
		}
		if (total > 0.0)
			expected /= total;

		Affine inverse;
		if (expected > 0.0 && blended.Determinant() > 0.6 * expected && blended.Inverse(inverse))
			rest[i] = inverse.Apply(posedPoints[i]);
		else
			rest[i] = Blend(weights[i], inverses).Apply(posedPoints[i]);
	}
	return rest;
}

} // namespace PoseFit
