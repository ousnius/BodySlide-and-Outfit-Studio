/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Anim.h"

#include <cstddef>
#include <vector>

// Fitting the skeleton to a figure that has no skeleton, like a model from another program: the pose (and with it the
// proportions) that puts a skinned body where the figure is. With the pose found, the figure's shapes can be skinned
// to the posed body and taken back to the skeleton's rest pose, which is how an outfit modelled in an A-pose or with
// other proportions gets into a game's rest pose. Works on the AnimBone hierarchy, in the skeleton's global space.
namespace PoseFit {

// A skinned mesh that stands for the body, unposed, in the skeleton's global space
struct Template {
	struct Influence {
		AnimBone* bone = nullptr;
		float weight = 0.0f;
	};

	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<std::vector<Influence>> influences; // per vertex
};

// How a bone's pose moves the skin of its unposed position, in global space
nifly::MatTransform SkinTransform(const AnimBone* bone);

// The vertices of a template with the current pose, by linear blend skinning
std::vector<nifly::Vector3> PoseVerts(const Template& skin);

struct Options {
	// Bones to fit. Parents are fitted before their children; the one above all others also moves the body as a whole.
	std::vector<AnimBone*> bones;
	// Let bones grow or shrink (evenly, with their children) to match other proportions, after the pose is fitted
	bool allowScale = true;
	// Move and turn the body as a whole by the topmost bone, before the joints are fitted
	bool moveRoot = true;
	// With moveRoot: also scale the body as a whole, to the figure's size. Scaling the figure first (EstimateScale) is
	// more reliable, since the arms aren't fitted yet.
	bool scaleBody = false;
	int iterations = 20;
	// Points farther apart than this aren't matched
	float maxDistance = 12.0f;
	// Share of the worst matches of each bone left out, for parts of the figure the template doesn't have
	float trim = 0.2f;
	// Most a bone turns in one iteration, in degrees, so the fit doesn't jump to a wrong match
	float maxTurnDegrees = 15.0f;
	float minScale = 0.8f;
	float maxScale = 1.25f;
};

struct BoneFit {
	AnimBone* bone = nullptr;
	// Matched points of the bone's own skin in the last iteration
	size_t matches = 0;
	// Mean distance of the bone's own skin to the figure after the fit
	float distance = 0.0f;
	// How far the fit turned the bone against its parent from where it started, in degrees, and how much the fit
	// scaled it against its parent
	float turnedDegrees = 0.0f;
	float scale = 1.0f;
};

struct Result {
	std::vector<BoneFit> bones;
	// Mean distance of the template's matched skin to the figure, before and after
	float distanceBefore = 0.0f;
	float distanceAfter = 0.0f;
	size_t matchesAfter = 0;
};

// Poses the bones so the templates match the figure (targetVerts, targetTris), starting from the current pose
Result Fit(const std::vector<Template>& templates, const std::vector<nifly::Vector3>& targetVerts, const std::vector<nifly::Triangle>& targetTris, const Options& options);

struct ScaleEstimate {
	float scale = 1.0f;
	// Where the figure's feet are (the middle of its lowest points), which the scale is about, and the templates'
	nifly::Vector3 figureFloor;
	nifly::Vector3 templateFloor;
	// Mean distance of the matched points after scaling and moving the figure
	float distance = 0.0f;
	// The scale is the ratio of the leg lengths, floor to crotch. Else it's the size that matched best, which is less
	// reliable for figures of other proportions.
	bool byLegs = false;
	// Where the legs part, for the figure and the templates (below the floor when they couldn't be measured)
	float figureCrotch = 0.0f;
	float templateCrotch = 0.0f;
};

// The uniform scale about the figure's feet, and the move of its feet onto the templates', that best match the figure
// (targetVerts, targetTris) to the unposed templates. Only the legs and torso of the templates count, since arms are
// posed differently between figures (an A-pose against the game's rest pose).
ScaleEstimate EstimateScale(const std::vector<Template>& templates, const std::vector<nifly::Vector3>& targetVerts, const std::vector<nifly::Triangle>& targetTris);

// Bone weights for points from the closest point of the posed templates, blended over the triangle there
std::vector<std::vector<Template::Influence>> WeightsFromTemplates(const std::vector<Template>& templates, const std::vector<nifly::Vector3>& points, float searchRadius);

// Blends the weights of each vertex with its neighbors' (copies of a vertex along split seams count as neighbors), so
// they change gradually over the mesh and unposing doesn't tear it where the closest template point jumps
void SmoothWeights(std::vector<std::vector<Template::Influence>>& weights,
				   const std::vector<nifly::Vector3>& verts,
				   const std::vector<nifly::Triangle>& tris,
				   int iterations,
				   float strength);

// The rest position of points that are where the current pose puts them: the inverse of each point's blended skin
// transform, so a point posed with these weights afterwards comes back exactly to where it is now
std::vector<nifly::Vector3> Unpose(const std::vector<nifly::Vector3>& posedPoints, const std::vector<std::vector<Template::Influence>>& weights);

} // namespace PoseFit
