/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <Object3d.hpp>

#include <string>
#include <vector>

// Bone chains for physics on the hanging parts of a mesh, e.g. the strands of a hairstyle: which parts hang
// away from where the mesh is attached (the scalp), where a chain of joints runs along each of them, and how
// much each vertex follows each joint.
namespace PhysicsChains {

struct Options {
	// Vertices closer than this to each other count as joined, so separate hair cards lying on each other
	// are one strand
	float linkDistance = 1.0f;
	// Distance along the mesh from the roots where the first moving joint sits. Closer vertices stay on
	// their current bones.
	float hangStart = 4.0f;
	// Vertices higher than this above the axis origin don't hang, however far from the roots they are: chains
	// start below it (e.g. ear level for strands framing the face, so the volume on the head stays put)
	float hangCeiling = 1e9f;
	// Each joint after the first has to be at least this much lower than the one above, relative to the joint
	// spacing; the chain ends where it isn't. 0 for off (a ponytail may first stick out backwards).
	float minJointDrop = 0.0f;
	// Where a chain's band of vertices falls apart into pieces whose middles are farther apart than this, they are
	// separate locks and the chain follows one of them instead of running between them
	float lockGap = 2.0f;
	// A part needs to reach this far along the mesh from the roots to get a chain. Shorter ones (buns,
	// volume on the scalp) stay rigid.
	float minHang = 12.0f;
	// Distance between joints along the mesh
	float boneSpacing = 8.0f;
	// Most moving joints per chain. Longer parts get a larger spacing.
	int maxBones = 7;
	// Most chains. More strands are merged into their nearest neighbors.
	int maxChains = 6;
	// A hanging part that starts around the head over a wider angle than this (degrees) is split into
	// several chains, like the back and the sides of long open hair
	float sectorAngle = 110.0f;
	// Split parts whose groups end closer together than this at their tips are one strand (a thick ponytail)
	// and get one chain
	float minTipGap = 6.0f;
	// Parts with fewer vertices don't get a chain
	int minVertices = 30;
	// How much a part has to go down: the drop from where it starts hanging to its tip, relative to its length
	// along the mesh. Parts that run around the head instead (volume, wrapped braids) stay rigid.
	float minDrop = 0.5f;
	// Only chains starting at these sides (see Chain::side), e.g. the strands framing the face. Empty for all.
	std::vector<std::string> sides;
};

struct Chain {
	// Joint positions: [0] is the root on the attachment (kinematic), [1..] the joints that move
	std::vector<nifly::Vector3> joints;
	// Distance along the mesh from the roots of each joint
	std::vector<float> jointDistance;
	// Name part from where the chain starts around the axis: Front, FrontR, R, BackR, Back, BackL, L, FrontL
	std::string side;
	// Degrees around the vertical axis where the chain starts: 0 in front, 90 at the right (+X), 180 behind
	float azimuth = 0.0f;
	// Longest distance along the mesh from the roots
	float length = 0.0f;
	size_t vertexCount = 0;
};

struct Result {
	std::vector<Chain> chains;
	// Per vertex: distance along the mesh from the roots, -1 where no root is reachable
	std::vector<float> distance;
	// Per vertex: the chain it hangs on, -1 for none
	std::vector<int> chain;
	size_t rootVertices = 0;
	size_t hangingVertices = 0;	  // Farther from the roots than hangStart
	size_t unreachableVertices = 0;
	size_t shortParts = 0;		  // Hanging parts too short or small for a chain
	size_t mergedParts = 0;		  // Parts merged into another chain because of maxChains
	size_t otherSides = 0;		  // Parts left out because they start at a side not in Options::sides
	size_t flatParts = 0;		  // Parts left out because they don't go down enough (Options::minDrop)
	size_t continuedParts = 0;	  // Parts left out because their tip runs into blocked (already chained) vertices
};

// Vertex neighbors: the edges of the triangles, plus vertices closer than linkDistance
std::vector<std::vector<int>> Neighbors(const std::vector<nifly::Vector3>& verts, const std::vector<nifly::Triangle>& tris, float linkDistance);

// Finds the chains of a mesh. roots marks the vertices on the attachment (e.g. those on the scalp), axisOrigin
// is a point on the vertical axis the chains are named and split around (e.g. the head bone). neighbors is
// from Neighbors. Vertices set in blocked (optional) take no part, e.g. those an earlier pass gave a chain.
Result Detect(const std::vector<nifly::Vector3>& verts,
			  const std::vector<std::vector<int>>& neighbors,
			  const std::vector<bool>& roots,
			  const nifly::Vector3& axisOrigin,
			  const Options& options,
			  const std::vector<bool>* blocked = nullptr);

// How a vertex follows a chain: weight on each moving joint (by joint index, 1 for the first one that
// moves) and keep, the share left for the vertex's current bones
struct VertexWeights {
	float keep = 1.0f;
	std::vector<std::pair<int, float>> joints;
};

// Weights of each vertex on the joints of its chain. Up to hangStart a vertex keeps its bones, then blends
// along the chain between the middles of the segments, each segment following the joint at its upper end.
// Below the last joint vertices follow it fully.
std::vector<VertexWeights> JointWeights(const Result& result, const Options& options);

} // namespace PhysicsChains
