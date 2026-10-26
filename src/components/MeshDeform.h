/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <vector>

// Changes meshes as a whole on plain vertex and triangle data in one coordinate space: turning flipped triangles back
// and making body morphs where an outfit presses in. Doesn't depend on the editor.
namespace MeshDeform {

struct UntangleOptions {
	int maxRings = 4;		   // How many rings of neighbors around the flipped triangles the fix may change
	int iterations = 40;	   // Smoothing passes per ring
	float strength = 0.5f;	   // Of each pass, 0 to 1
	float weldDistance = 1e-4f; // Copies of a vertex along split seams closer than this move together
};

struct UntangleResult {
	size_t flippedBefore = 0;
	size_t flippedAfter = 0;
	size_t movedVertices = 0;
	float maxMove = 0.0f;
	int rings = 0; // Rings of neighbors the fix needed
};

// Turns triangles that flipped between two vertex positions back: the change from before to after is smoothed
// around them, ring by ring, until they face the way they did before. The rest of the change stays, so the mesh
// keeps its relief. after is changed in place. freedom (optional, 0 to 1 per vertex) keeps masked vertices where
// they are. Nothing changes when the fix would leave more flipped triangles than there were.
UntangleResult UntangleFlips(const std::vector<nifly::Vector3>& before,
							 std::vector<nifly::Vector3>& after,
							 const std::vector<nifly::Triangle>& tris,
							 const std::vector<float>* freedom,
							 const UntangleOptions& options);

struct ContactMorphOptions {
	float contactDistance = 0.45f; // Body vertices this close to the cover are under it
	float tentDistance = 2.0f;	   // And those with the cover in front of them this close, where it tents over the body
	float depth = 1.3f;			   // How far the covered body sinks in
	float depthWidth = 1.8f;	   // How far in from the edge of the covered area the full depth is reached
	float bulge = 0.85f;		   // How far the body swells out past the edge, where the body next to it sank the full depth
	float bulgeWidth = 1.2f;	   // Distance from the edge to the top of the swell, which fades out after it
	int smoothing = 5;			   // Passes that smooth the result
	float weldDistance = 1e-4f;	   // Copies of a vertex along split seams closer than this move together
};

struct ContactMorph {
	// Per body vertex, along the outward vertex normal: negative sinks in, positive swells out
	std::vector<float> offsets;
	std::vector<nifly::Vector3> deltas;
	// Per body vertex, the distance along the surface to the edge of the covered area: positive under the cover
	std::vector<float> edgeDistance;
	size_t covered = 0;
	float maxDepth = 0.0f;
	float maxBulge = 0.0f;
};

// A body morph for an outfit that presses into it, e.g. a tight top squishing the breasts: the body under the cover
// sinks in, smoothly from the edge of the covered area, and swells out just past it. Distances are measured along the
// body's surface, so the shape follows the cover's outline without the creases of a box mask. freedom (optional, 0 to 1
// per body vertex) keeps masked vertices where they are.
ContactMorph MakeContactMorph(const std::vector<nifly::Vector3>& bodyVerts,
							  const std::vector<nifly::Triangle>& bodyTris,
							  const std::vector<nifly::Vector3>& coverVerts,
							  const std::vector<nifly::Triangle>& coverTris,
							  const std::vector<float>* freedom,
							  const ContactMorphOptions& options);

// Deltas for vertices that sit on or near a body, following the body's morph: each moves like the closest point of
// the body's surface, fully up to three quarters of the radius from it and fading out to nothing at the radius. With
// pointTris the motion is smoothed over the points' mesh so pieces move as a whole, and points that would end up
// closer to the body than they were go back out to their old gap.
std::vector<nifly::Vector3> FollowDeltas(const std::vector<nifly::Vector3>& bodyVerts,
										 const std::vector<nifly::Triangle>& bodyTris,
										 const std::vector<nifly::Vector3>& bodyDeltas,
										 const std::vector<nifly::Vector3>& points,
										 const std::vector<nifly::Triangle>& pointTris,
										 float radius);

} // namespace MeshDeform
