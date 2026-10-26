/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ClippingFixer.h"

#include "../utils/AABBTree.h"

#include "../utils/ParallelFor.h"

#include <cfloat>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <unordered_map>

using namespace nifly;

namespace {
constexpr size_t clippingQueryMinItemsPerWorker = 4096;

struct EdgeRecord {
	uint32_t first = 0;
	uint32_t second = 0;
};

struct MeshTopology {
	std::vector<std::vector<int>> adjacency;
	std::vector<uint8_t> boundaryVerts;
};

bool operator<(const EdgeRecord& left, const EdgeRecord& right) {
	if (left.first != right.first)
		return left.first < right.first;

	return left.second < right.second;
}

bool operator==(const EdgeRecord& left, const EdgeRecord& right) {
	return left.first == right.first && left.second == right.second;
}

MeshTopology BuildTopology(int nVerts, const std::vector<Triangle>& tris) {
	MeshTopology topology;
	topology.adjacency.resize(nVerts);
	topology.boundaryVerts.assign(nVerts, 0);

	std::vector<EdgeRecord> edges;
	edges.reserve(tris.size() * 3);

	auto addEdge = [&](uint32_t first, uint32_t second) {
		if (first >= static_cast<uint32_t>(nVerts) || second >= static_cast<uint32_t>(nVerts) || first == second)
			return;

		if (first > second)
			std::swap(first, second);

		edges.push_back({first, second});
	};

	for (auto& tri : tris) {
		addEdge(tri.p1, tri.p2);
		addEdge(tri.p2, tri.p3);
		addEdge(tri.p3, tri.p1);
	}

	std::sort(edges.begin(), edges.end());

	for (size_t edgeIndex = 0; edgeIndex < edges.size();) {
		const EdgeRecord current = edges[edgeIndex];
		size_t nextEdge = edgeIndex + 1;
		while (nextEdge < edges.size() && edges[nextEdge] == current)
			nextEdge++;

		topology.adjacency[current.first].push_back(static_cast<int>(current.second));
		topology.adjacency[current.second].push_back(static_cast<int>(current.first));

		if (nextEdge - edgeIndex == 1) {
			topology.boundaryVerts[current.first] = 1;
			topology.boundaryVerts[current.second] = 1;
		}

		edgeIndex = nextEdge;
	}

	return topology;
}

float ComputeNormalSignFromPoint(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const Vector3& point) {
	float orientation = 0.0f;

	for (auto& tri : tris) {
		if (tri.p1 >= verts.size() || tri.p2 >= verts.size() || tri.p3 >= verts.size())
			continue;

		Vector3 triCenter = (verts[tri.p1] + verts[tri.p2] + verts[tri.p3]) / 3.0f;
		Vector3 normal = tri.trinormal(verts.data());
		orientation += (triCenter - point).dot(normal);
	}

	return (orientation >= 0.0f) ? 1.0f : -1.0f;
}

// Distance along the ray to the triangle, from either side, or a negative value when the ray misses it
float RayTriangleDistance(const Vector3& origin, const Vector3& direction, const Vector3& c0, const Vector3& c1, const Vector3& c2) {
	Vector3 e1 = c1 - c0;
	Vector3 e2 = c2 - c0;
	Vector3 pvec = direction.cross(e2);
	float det = e1.dot(pvec);
	if (std::fabs(det) < 1e-12f)
		return -1.0f;

	float inv = 1.0f / det;
	Vector3 tvec = origin - c0;
	float u = tvec.dot(pvec) * inv;
	if (u < 0.0f || u > 1.0f)
		return -1.0f;

	Vector3 qvec = tvec.cross(e1);
	float v = direction.dot(qvec) * inv;
	if (v < 0.0f || u + v > 1.0f)
		return -1.0f;

	return e2.dot(qvec) * inv;
}

// 1 when the triangles of a body face away from its inside, -1 when they face into it: most faces have to point away
// from the centroid. trinormal() direction depends on winding order which varies across NIF files.
Vector3 Centroid(const std::vector<Vector3>& verts) {
	Vector3 centroid;
	for (auto& v : verts)
		centroid += v;
	return centroid / static_cast<float>(verts.size());
}

float OutwardSign(const std::vector<Vector3>& bodyVerts, const std::vector<Triangle>& bodyTris) {
	const Vector3 bodyCentroid = Centroid(bodyVerts);

	int outwardCount = 0;
	int inwardCount = 0;
	for (auto& tri : bodyTris) {
		if (tri.p1 >= bodyVerts.size() || tri.p2 >= bodyVerts.size() || tri.p3 >= bodyVerts.size())
			continue;
		Vector3 triCenter = (bodyVerts[tri.p1] + bodyVerts[tri.p2] + bodyVerts[tri.p3]) / 3.0f;
		if ((triCenter - bodyCentroid).dot(tri.trinormal(bodyVerts.data())) > 0.0f)
			outwardCount++;
		else
			inwardCount++;
	}

	return (outwardCount >= inwardCount) ? 1.0f : -1.0f;
}

// Turns vertex normals of a body to point away from its inside
std::vector<Vector3> Oriented(std::vector<Vector3> normals, float normalSign) {
	for (auto& n : normals)
		n = n * normalSign;
	return normals;
}
}

NiShape* ClippingFixer::FindReferenceShape(NifFile& nif) {
	for (auto& shape : nif.GetShapes()) {
		NiShader* shader = nif.GetShader(shape);
		if (shader && shader->IsSkinTinted())
			return shape;
	}
	return nullptr;
}

bool ClippingFixer::IsEligibleForFix(NifFile& nif, NiShape* shape) {
	if (!shape)
		return false;

	if ((shape->flags & 1) != 0)
		return false;

	NiShader* shader = nif.GetShader(shape);
	if (!shader || shader->IsSkinTinted())
		return false;

	return true;
}

std::vector<Vector3> ClippingFixer::ComputeVertexNormals(const std::vector<Vector3>& verts,
														 const std::vector<Triangle>& tris) {
	std::vector<Vector3> normals(verts.size());

	for (auto& tri : tris) {
		if (tri.p1 >= verts.size() || tri.p2 >= verts.size() || tri.p3 >= verts.size())
			continue;

		Vector3 faceNormal = tri.trinormal(verts.data());
		normals[tri.p1] += faceNormal;
		normals[tri.p2] += faceNormal;
		normals[tri.p3] += faceNormal;
	}

	for (auto& n : normals)
		n.Normalize();

	return normals;
}

void ClippingFixer::BarycentricCoords(const Vector3& p,
									  const Vector3& v1,
									  const Vector3& v2,
									  const Vector3& v3,
									  float& u, float& v, float& w) {
	Vector3 e0 = v2 - v1;
	Vector3 e1 = v3 - v1;
	Vector3 ep = p - v1;
	float d00 = e0.dot(e0);
	float d01 = e0.dot(e1);
	float d11 = e1.dot(e1);
	float d20 = ep.dot(e0);
	float d21 = ep.dot(e1);
	float denom = d00 * d11 - d01 * d01;
	if (std::abs(denom) < 1e-12f) {
		u = v = w = 1.0f / 3.0f;
		return;
	}
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
}

std::vector<std::vector<int>> ClippingFixer::BuildAdjacency(int nVerts, const std::vector<Triangle>& tris) {
	return BuildTopology(nVerts, tris).adjacency;
}

std::vector<float> ClippingFixer::SignedDistancesToBody(const std::vector<Vector3>& bodyVerts,
													 const std::vector<Triangle>& bodyTris,
													 const std::vector<Vector3>& points,
													 float searchRadius) {
	std::vector<float> distances(points.size(), FLT_MAX);
	if (bodyVerts.empty() || bodyTris.empty() || points.empty())
		return distances;

	// Same outward direction as FixClipping
	const float normalSign = OutwardSign(bodyVerts, bodyTris);
	std::vector<Vector3> bodyNormals = Oriented(ComputeVertexNormals(bodyVerts, bodyTris), normalSign);

	// Open edges: edges of only one triangle, and their vertices
	std::unordered_map<uint32_t, int> edgeUse;
	auto edgeKey = [](uint16_t a, uint16_t b) { return a < b ? (static_cast<uint32_t>(a) << 16) | b : (static_cast<uint32_t>(b) << 16) | a; };
	for (auto& tri : bodyTris) {
		edgeUse[edgeKey(tri.p1, tri.p2)]++;
		edgeUse[edgeKey(tri.p2, tri.p3)]++;
		edgeUse[edgeKey(tri.p3, tri.p1)]++;
	}
	std::vector<bool> openVertex(bodyVerts.size(), false);
	for (auto& edge : edgeUse)
		if (edge.second == 1) {
			uint16_t a = static_cast<uint16_t>(edge.first >> 16);
			uint16_t b = static_cast<uint16_t>(edge.first & 0xFFFF);
			if (a < openVertex.size())
				openVertex[a] = true;
			if (b < openVertex.size())
				openVertex[b] = true;
		}
	auto isOpenEdge = [&](uint16_t a, uint16_t b) {
		auto it = edgeUse.find(edgeKey(a, b));
		return it != edgeUse.end() && it->second == 1;
	};

	AABBTree bodyBVH(const_cast<Vector3*>(bodyVerts.data()), const_cast<Triangle*>(bodyTris.data()), static_cast<uint32_t>(bodyTris.size()), 100, 2);
	std::vector<AABBTriangleDistanceData> bodyTriangleDistanceData;
	AABBTree::BuildTriangleDistanceData(bodyVerts.data(), bodyTris.data(), static_cast<uint32_t>(bodyTris.size()), bodyTriangleDistanceData);

	ParallelFor(points.size(), clippingQueryMinItemsPerWorker, [&](size_t startIndex, size_t endIndex) {
		for (size_t i = startIndex; i < endIndex; i++) {
			uint32_t facet = 0;
			Vector3 closestPoint;
			Vector3 point = points[i];
			if (!bodyBVH.ClosestFacetInSphere(point, searchRadius, facet, &closestPoint, &bodyTriangleDistanceData) || facet >= bodyTris.size())
				continue;

			const Triangle& tri = bodyTris[facet];
			if (tri.p1 >= bodyVerts.size() || tri.p2 >= bodyVerts.size() || tri.p3 >= bodyVerts.size())
				continue;

			float bu, bv, bw;
			BarycentricCoords(closestPoint, bodyVerts[tri.p1], bodyVerts[tri.p2], bodyVerts[tri.p3], bu, bv, bw);
			bu = std::max(0.0f, bu);
			bv = std::max(0.0f, bv);
			bw = std::max(0.0f, bw);
			float bsum = bu + bv + bw;
			if (bsum > 0.0f) {
				bu /= bsum;
				bv /= bsum;
				bw /= bsum;
			}

			Vector3 outward = bodyNormals[tri.p1] * bu + bodyNormals[tri.p2] * bv + bodyNormals[tri.p3] * bw;
			if (outward.IsZero(true))
				outward = tri.trinormal(bodyVerts.data()) * normalSign;
			outward.Normalize();

			distances[i] = (point - closestPoint).dot(outward);

			// Closest to an open edge, the point is past the opening: the normals there point sideways, so
			// a point over the hole would read as inside, or as on the surface
			constexpr float onEdge = 1e-3f;
			if (bu < onEdge || bv < onEdge || bw < onEdge) {
				bool open = (bu < onEdge && isOpenEdge(tri.p2, tri.p3)) || (bv < onEdge && isOpenEdge(tri.p3, tri.p1)) || (bw < onEdge && isOpenEdge(tri.p1, tri.p2));
				// At a corner of the triangle, the edge the point is past may belong to a neighbor
				open = open || (bv < onEdge && bw < onEdge && openVertex[tri.p1]) || (bu < onEdge && bw < onEdge && openVertex[tri.p2]) ||
					   (bu < onEdge && bv < onEdge && openVertex[tri.p3]);
				if (open)
					distances[i] = (point - closestPoint).length();
			}
		}
	});

	return distances;
}

std::vector<Vector3> ClippingFixer::OutwardVertexNormals(const std::vector<Vector3>& bodyVerts, const std::vector<Triangle>& bodyTris) {
	if (bodyVerts.empty())
		return {};
	return Oriented(ComputeVertexNormals(bodyVerts, bodyTris), OutwardSign(bodyVerts, bodyTris));
}

std::vector<PokeThrough> ClippingFixer::BodyPokeThrough(const std::vector<Vector3>& bodyVerts,
														const std::vector<Triangle>& bodyTris,
														const std::vector<Vector3>& outfitVerts,
														const std::vector<Triangle>& outfitTris,
														float maxDepth,
														const std::vector<Vector3>* outwardNormals) {
	std::vector<PokeThrough> pokes(bodyVerts.size());
	if (bodyVerts.empty() || bodyTris.empty() || outfitVerts.empty() || outfitTris.empty() || maxDepth <= 0.0f)
		return pokes;

	std::vector<Vector3> ownNormals;
	if (!outwardNormals || outwardNormals->size() != bodyVerts.size()) {
		ownNormals = Oriented(ComputeVertexNormals(bodyVerts, bodyTris), OutwardSign(bodyVerts, bodyTris));
		outwardNormals = &ownNormals;
	}
	const std::vector<Vector3>& bodyNormals = *outwardNormals;

	auto validTri = [](const Triangle& tri, size_t count) { return tri.p1 < count && tri.p2 < count && tri.p3 < count; };

	// The outfit surface closest behind each body vertex, looking inwards
	AABBTree outfitBVH(const_cast<Vector3*>(outfitVerts.data()), const_cast<Triangle*>(outfitTris.data()), static_cast<uint32_t>(outfitTris.size()), 100, 2);
	ParallelFor(bodyVerts.size(), clippingQueryMinItemsPerWorker, [&](size_t startIndex, size_t endIndex) {
		std::vector<IntersectResult> candidates;
		for (size_t b = startIndex; b < endIndex; b++) {
			Vector3 origin = bodyVerts[b];
			Vector3 inward = bodyNormals[b] * -1.0f;
			if (inward.IsZero(true))
				continue;

			candidates.clear();
			if (!outfitBVH.IntersectSphere(origin, maxDepth, &candidates))
				continue;

			for (auto& c : candidates) {
				const Triangle& tri = outfitTris[c.HitFacet];
				if (!validTri(tri, outfitVerts.size()))
					continue;
				float t = RayTriangleDistance(origin, inward, outfitVerts[tri.p1], outfitVerts[tri.p2], outfitVerts[tri.p3]);
				if (t > 1e-5f && t <= maxDepth && (pokes[b].triangle < 0 || t < pokes[b].depth)) {
					pokes[b].triangle = static_cast<int>(c.HitFacet);
					pokes[b].depth = t;
				}
			}
		}
	});

	bool any = false;
	for (auto& poke : pokes)
		any = any || poke.triangle >= 0;
	if (!any)
		return pokes;

	// An outfit surface beyond the body's other side, like a glove's far side behind a thin finger, covers that
	// side and isn't a poke
	AABBTree bodyBVH(const_cast<Vector3*>(bodyVerts.data()), const_cast<Triangle*>(bodyTris.data()), static_cast<uint32_t>(bodyTris.size()), 100, 2);
	ParallelFor(bodyVerts.size(), clippingQueryMinItemsPerWorker, [&](size_t startIndex, size_t endIndex) {
		std::vector<IntersectResult> candidates;
		for (size_t b = startIndex; b < endIndex; b++) {
			if (pokes[b].triangle < 0)
				continue;

			Vector3 origin = bodyVerts[b];
			Vector3 inward = bodyNormals[b] * -1.0f;
			candidates.clear();
			if (!bodyBVH.IntersectSphere(origin, pokes[b].depth, &candidates))
				continue;

			for (auto& c : candidates) {
				const Triangle& tri = bodyTris[c.HitFacet];
				if (!validTri(tri, bodyVerts.size()) || tri.p1 == b || tri.p2 == b || tri.p3 == b)
					continue;
				float t = RayTriangleDistance(origin, inward, bodyVerts[tri.p1], bodyVerts[tri.p2], bodyVerts[tri.p3]);
				if (t > 1e-3f && t < pokes[b].depth) {
					pokes[b] = PokeThrough();
					break;
				}
			}
		}
	});

	return pokes;
}

std::vector<float> ClippingFixer::PokeDepthPerVertex(const std::vector<PokeThrough>& pokes, const std::vector<Triangle>& outfitTris, size_t outfitVertexCount) {
	std::vector<float> depths(outfitVertexCount, 0.0f);
	for (auto& poke : pokes) {
		if (poke.triangle < 0 || static_cast<size_t>(poke.triangle) >= outfitTris.size())
			continue;
		const Triangle& tri = outfitTris[poke.triangle];
		for (uint16_t v : {tri.p1, tri.p2, tri.p3})
			if (v < outfitVertexCount)
				depths[v] = std::max(depths[v], poke.depth);
	}
	return depths;
}

void ClippingFixer::FixClipping(const std::vector<Vector3>& bodyVerts,
								const std::vector<Triangle>& bodyTris,
								std::vector<Vector3>& outfitVerts,
								const std::vector<Triangle>& outfitTris,
								const ClippingFixOptions& options) {
	if (bodyVerts.empty() || bodyTris.empty() || outfitVerts.empty())
		return;

	float strength = std::max(0.0f, std::min(1.0f, options.strength));
	if (strength <= 0.0f)
		return;

	// Inflation distance: how far the body "inflates" outward
	float inflationDist = 0.05f + strength * 0.25f;

	// Extra margin beyond inflated surface for smooth falloff
	float influenceMargin = inflationDist * 0.5f;
	float totalInfluence = inflationDist + influenceMargin;

	// The body vertex normals pointing outward are the "inflation direction" at each vertex
	const float normalSign = OutwardSign(bodyVerts, bodyTris);
	std::vector<Vector3> bodyNormals = Oriented(ComputeVertexNormals(bodyVerts, bodyTris), normalSign);

	// Build AABBTree for the body mesh
	AABBTree bodyBVH(const_cast<Vector3*>(bodyVerts.data()),
					  const_cast<Triangle*>(bodyTris.data()),
					  static_cast<uint32_t>(bodyTris.size()),
					  100,
					  2);
	std::vector<AABBTriangleDistanceData> bodyTriangleDistanceData;
	AABBTree::BuildTriangleDistanceData(bodyVerts.data(), bodyTris.data(), static_cast<uint32_t>(bodyTris.size()), bodyTriangleDistanceData);

	float searchRadius = 2.0f + strength * 3.0f;
	int nOutfitVerts = static_cast<int>(outfitVerts.size());

	// First pass: compute displacement for each outfit vertex (don't apply yet)
	std::vector<Vector3> displacements(nOutfitVerts);
	std::vector<uint8_t> propagationEligible(nOutfitVerts, 0);
	MeshTopology topology = BuildTopology(nOutfitVerts, outfitTris);
	const std::vector<uint8_t>& boundaryVerts = topology.boundaryVerts;
	const std::vector<std::vector<int>>& adjacency = topology.adjacency;
	std::vector<Vector3> outfitNormals = ComputeVertexNormals(outfitVerts, outfitTris);
	float outfitNormalSign = ComputeNormalSignFromPoint(outfitVerts, outfitTris, Centroid(bodyVerts));
	for (auto& n : outfitNormals)
		n = n * outfitNormalSign;

	auto calcDisplacement = [&](const Vector3& origin,
								  const Vector3* outfitNormal,
								  bool& outPropagationEligible) {
		outPropagationEligible = false;
		Vector3 displacement;

		Vector3 queryOrigin = origin;
		uint32_t bestFacet = 0;
		Vector3 bestClosestPoint;
		if (!bodyBVH.ClosestFacetInSphere(queryOrigin, searchRadius, bestFacet, &bestClosestPoint, &bodyTriangleDistanceData))
			return displacement;

		if (bestFacet >= bodyTris.size())
			return displacement;

		const Triangle& closestTri = bodyTris[bestFacet];
		if (closestTri.p1 >= bodyVerts.size() || closestTri.p2 >= bodyVerts.size() || closestTri.p3 >= bodyVerts.size())
			return displacement;

		float bu, bv, bw;
		BarycentricCoords(bestClosestPoint,
						  bodyVerts[closestTri.p1],
						  bodyVerts[closestTri.p2],
						  bodyVerts[closestTri.p3],
						  bu, bv, bw);

		bu = std::max(0.0f, bu);
		bv = std::max(0.0f, bv);
		bw = std::max(0.0f, bw);
		float bsum = bu + bv + bw;
		if (bsum > 0.0f) { bu /= bsum; bv /= bsum; bw /= bsum; }

		Vector3 inflationDir = bodyNormals[closestTri.p1] * bu
							 + bodyNormals[closestTri.p2] * bv
							 + bodyNormals[closestTri.p3] * bw;
		if (inflationDir.IsZero(true))
			inflationDir = closestTri.trinormal(bodyVerts.data()) * normalSign;
		inflationDir.Normalize();

		bool oppositeFacingOutfitSurface = false;
		if (outfitNormal && !outfitNormal->IsZero(true))
			oppositeFacingOutfitSurface = outfitNormal->dot(inflationDir) < -0.25f;

		Vector3 surfaceOffset = origin - bestClosestPoint;
		float signedDist = surfaceOffset.dot(inflationDir);
		float tangentDistSq = std::max(0.0f, surfaceOffset.length2() - signedDist * signedDist);
		float propagationInfluence = totalInfluence + inflationDist;
		outPropagationEligible = signedDist < propagationInfluence && tangentDistSq <= propagationInfluence * propagationInfluence;

		if (!oppositeFacingOutfitSurface && signedDist < totalInfluence && tangentDistSq <= totalInfluence * totalInfluence) {
			float pushDist;
			if (signedDist < inflationDist) {
				pushDist = inflationDist - signedDist;
			}
			else {
				float t = (signedDist - inflationDist) / influenceMargin;
				float falloff = (1.0f - t) * (1.0f - t);
				pushDist = falloff * inflationDist * 0.3f;
			}

			displacement = inflationDir * pushDist;
		}

		return displacement;
	};

	auto mergeDisplacement = [&](uint32_t vertIndex, const Vector3& displacement) {
		if (vertIndex >= displacements.size() || displacement.IsZero(true))
			return;

		if (displacement.length2() > displacements[vertIndex].length2())
			displacements[vertIndex] = displacement;
		propagationEligible[vertIndex] = 1;
	};

	ParallelFor(static_cast<size_t>(nOutfitVerts), clippingQueryMinItemsPerWorker, [&](size_t startIndex, size_t endIndex) {
		for (size_t vertIndex = startIndex; vertIndex < endIndex; vertIndex++) {
			Vector3 origin = outfitVerts[vertIndex];
			bool canPropagate = false;
			Vector3 displacement = calcDisplacement(origin,
											   vertIndex < outfitNormals.size() ? &outfitNormals[vertIndex] : nullptr,
										   canPropagate);
			propagationEligible[vertIndex] = canPropagate ? 1 : 0;
			if (!displacement.IsZero(true)) {
				displacements[vertIndex] = displacement;
				propagationEligible[vertIndex] = 1;
			}
		}
	});

	// Low-poly triangles can clip through the body even when their sparse vertices
	// are outside the direct influence band. Sample triangle interiors and large
	// edges, then spread those corrections back to the owning vertices.
	struct SampleContribution {
		Vector3 displacement;
		std::array<uint32_t, 3> vertIndices{{0, 0, 0}};
		uint8_t vertCount = 0;
		bool canPropagate = false;
	};

	struct TriangleContributions {
		std::array<SampleContribution, 4> samples;
		uint8_t sampleCount = 0;
	};

	std::vector<TriangleContributions> triangleContributions(outfitTris.size());
	float longEdgeThresholdSq = totalInfluence * 1.5f;
	longEdgeThresholdSq *= longEdgeThresholdSq;

	ParallelFor(outfitTris.size(), clippingQueryMinItemsPerWorker, [&](size_t startIndex, size_t endIndex) {
		for (size_t triIndex = startIndex; triIndex < endIndex; triIndex++) {
			const Triangle& tri = outfitTris[triIndex];
			if (tri.p1 >= outfitVerts.size() || tri.p2 >= outfitVerts.size() || tri.p3 >= outfitVerts.size())
				continue;

			const Vector3& v1 = outfitVerts[tri.p1];
			const Vector3& v2 = outfitVerts[tri.p2];
			const Vector3& v3 = outfitVerts[tri.p3];
			Vector3 faceNormal = tri.trinormal(outfitVerts.data()) * outfitNormalSign;
			faceNormal.Normalize();
			const Vector3* sampleNormal = faceNormal.IsZero(true) ? nullptr : &faceNormal;

			auto addTriangleSample = [&](const Vector3& samplePoint, uint32_t firstVert, uint32_t secondVert, uint32_t thirdVert, uint8_t vertCount) {
				TriangleContributions& contributions = triangleContributions[triIndex];
				if (contributions.sampleCount >= contributions.samples.size())
					return;

				bool canPropagate = false;
				Vector3 displacement = calcDisplacement(samplePoint, sampleNormal, canPropagate);
				SampleContribution& contribution = contributions.samples[contributions.sampleCount++];
				contribution.displacement = displacement;
				contribution.vertIndices[0] = firstVert;
				contribution.vertIndices[1] = secondVert;
				contribution.vertIndices[2] = thirdVert;
				contribution.vertCount = vertCount;
				contribution.canPropagate = canPropagate;
			};

			addTriangleSample((v1 + v2 + v3) / 3.0f, tri.p1, tri.p2, tri.p3, 3);

			float edge12Sq = (v2 - v1).length2();
			float edge23Sq = (v3 - v2).length2();
			float edge31Sq = (v1 - v3).length2();
			float maxEdgeLenSq = std::max(edge12Sq, std::max(edge23Sq, edge31Sq));
			if (maxEdgeLenSq > longEdgeThresholdSq) {
				addTriangleSample((v1 + v2) / 2.0f, tri.p1, tri.p2, 0, 2);
				addTriangleSample((v2 + v3) / 2.0f, tri.p2, tri.p3, 0, 2);
				addTriangleSample((v3 + v1) / 2.0f, tri.p3, tri.p1, 0, 2);
			}
		}
	});

	for (auto& contributions : triangleContributions) {
		for (uint8_t sampleIndex = 0; sampleIndex < contributions.sampleCount; sampleIndex++) {
			SampleContribution& contribution = contributions.samples[sampleIndex];
			for (uint8_t vertIndexPos = 0; vertIndexPos < contribution.vertCount; vertIndexPos++) {
				uint32_t vertIndex = contribution.vertIndices[vertIndexPos];
				if (vertIndex >= displacements.size())
					continue;

				if (contribution.canPropagate)
					propagationEligible[vertIndex] = 1;

				mergeDisplacement(vertIndex, contribution.displacement);
			}
		}
	}

	// The body poking out between outfit vertices: the triangle it went through moves out over the body vertex. A
	// deep poke (a nipple) moves only part of the way per fix, as moving one triangle far at once makes a tent with
	// folds around it; the strength sets how far.
	if (options.fixPokeThrough) {
		const float maxPush = 0.1f + strength * 0.5f;
		std::vector<PokeThrough> pokes = BodyPokeThrough(bodyVerts, bodyTris, outfitVerts, outfitTris, totalInfluence + 1.0f, &bodyNormals);
		for (size_t b = 0; b < pokes.size(); b++) {
			if (pokes[b].triangle < 0)
				continue;
			const Triangle& tri = outfitTris[pokes[b].triangle];
			Vector3 displacement = bodyNormals[b] * (std::min(pokes[b].depth, maxPush) + inflationDist);
			for (uint16_t v : {tri.p1, tri.p2, tri.p3})
				mergeDisplacement(v, displacement);
		}
	}

	// Propagate displacements through outfit mesh connectivity.
	// This preserves the thickness of thin structures like straps:
	// when inner vertices get pushed, the push carries to outer vertices
	// through mesh edges, moving the whole structure together.
	int propagationRings = 4 + static_cast<int>(std::round(strength * 4.0f));
	constexpr float propagationDecay = 0.85f;

	for (int ring = 0; ring < propagationRings; ring++) {
		std::vector<Vector3> newDisplacements = displacements;
		std::vector<uint8_t> newPropagationEligible = propagationEligible;

		for (int vertIndex = 0; vertIndex < nOutfitVerts; vertIndex++) {
			if (boundaryVerts[vertIndex])
				continue;

			float currentLen = displacements[vertIndex].length();

			// Find the neighbor with the strongest displacement
			Vector3 bestNeighborDisp;
			float bestNeighborLen = 0.0f;

			for (int adj : adjacency[vertIndex]) {
				float adjLen = displacements[adj].length();
				if (adjLen > bestNeighborLen) {
					bestNeighborLen = adjLen;
					bestNeighborDisp = displacements[adj];
				}
			}

			// If a neighbor's decayed displacement exceeds ours, adopt it
			float propagatedLen = bestNeighborLen * propagationDecay;
			if (propagatedLen > currentLen + 0.001f) {
				newDisplacements[vertIndex] = bestNeighborDisp * propagationDecay;
				newPropagationEligible[vertIndex] = 1;
			}
		}

		displacements = std::move(newDisplacements);
		propagationEligible = std::move(newPropagationEligible);
	}

	for (int iter = 0; iter < 2; iter++) {
		std::vector<Vector3> smoothedDisplacements = displacements;

		for (int vertIndex = 0; vertIndex < nOutfitVerts; vertIndex++) {
			if (boundaryVerts[vertIndex] || !propagationEligible[vertIndex])
				continue;

			Vector3 sum = displacements[vertIndex];
			float weight = 1.0f;
			for (int adj : adjacency[vertIndex]) {
				if (!propagationEligible[adj] || displacements[adj].IsZero(true))
					continue;

				sum += displacements[adj];
				weight += 1.0f;
			}

			if (weight <= 1.0f)
				continue;

			Vector3 average = sum / weight;
			Vector3 blended = displacements[vertIndex] * 0.65f + average * 0.35f;
			float currentLen = displacements[vertIndex].length();
			float blendedLen = blended.length();
			if (currentLen > 0.001f && blendedLen < currentLen * 0.8f) {
				if (blendedLen > 1e-6f)
					blended = blended * ((currentLen * 0.8f) / blendedLen);
				else
					blended = displacements[vertIndex] * 0.8f;
			}

			smoothedDisplacements[vertIndex] = blended;
		}

		displacements = std::move(smoothedDisplacements);
	}

	// Unify displacements for co-located ("welded") vertices.
	// Meshes often have duplicate vertices at the same position with different
	// UVs or normals. Without this, one copy gets displaced while the other
	// stays put, tearing the mesh apart at seams.
	{
		// Sort vertex indices by x coordinate for efficient neighbor search
		std::vector<int> sortedInds(nOutfitVerts);
		for (int i = 0; i < nOutfitVerts; i++)
			sortedInds[i] = i;

		std::sort(sortedInds.begin(), sortedInds.end(),
				  [&outfitVerts](int a, int b) { return outfitVerts[a].x < outfitVerts[b].x; });

		// Determine scale-relative epsilon (same approach as SortingMatcher)
		float scale = 0.0f;
		for (int i = 0; i < nOutfitVerts; i++)
			scale = std::max(scale, std::max(std::fabs(outfitVerts[i].x),
							 std::max(std::fabs(outfitVerts[i].y), std::fabs(outfitVerts[i].z))));
		float epsilon = 0.0001f * 0.01f * scale;

		std::vector<bool> used(nOutfitVerts, false);
		for (int si = 0; si < nOutfitVerts; si++) {
			if (used[si])
				continue;

			// Collect co-located vertices and find strongest displacement
			int bestIdx = sortedInds[si];
			float bestLen = displacements[bestIdx].length2();
			std::vector<int> group;

			for (int mi = si + 1; mi < nOutfitVerts; mi++) {
				if (outfitVerts[sortedInds[mi]].x - outfitVerts[sortedInds[si]].x >= epsilon)
					break;
				if (used[mi])
					continue;
				if (std::fabs(outfitVerts[sortedInds[si]].y - outfitVerts[sortedInds[mi]].y) >= epsilon)
					continue;
				if (std::fabs(outfitVerts[sortedInds[si]].z - outfitVerts[sortedInds[mi]].z) >= epsilon)
					continue;

				// Found a co-located vertex
				if (group.empty())
					group.push_back(sortedInds[si]);

				group.push_back(sortedInds[mi]);
				used[mi] = true;

				float len = displacements[sortedInds[mi]].length2();
				if (len > bestLen) {
					bestLen = len;
					bestIdx = sortedInds[mi];
				}
			}

			// Apply the strongest displacement to all vertices in the group
			if (!group.empty()) {
				for (int idx : group)
					displacements[idx] = displacements[bestIdx];
			}
		}
	}

	// Apply final displacements
	for (int vertIndex = 0; vertIndex < nOutfitVerts; vertIndex++)
		outfitVerts[vertIndex] = outfitVerts[vertIndex] + displacements[vertIndex];
}
