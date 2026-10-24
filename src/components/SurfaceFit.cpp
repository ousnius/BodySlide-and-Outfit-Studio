/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SurfaceFit.h"
#include "ClippingFixer.h"
#include "MeshAnalysis.h"
#include "../utils/AABBTree.h"
#include "../utils/ParallelFor.h"

#include "KDMatcher.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <unordered_set>

using namespace nifly;

namespace SurfaceFit {

namespace {

// A vertex closer than this to its closest point counts as on the surface, whatever the direction
constexpr float OnSurface = 1e-3f;
// Least cosine between the line to the closest point and the surface normal there. A smaller one means the closest
// point is on an open edge or a sharp rim of the surface, e.g. the open wrist of a body without hands, and the pull
// would drag the vertex sideways onto that edge.
constexpr float MinLineAgreement = 0.6f;

Vector3 Normalized(Vector3 v) {
	float length = v.length();
	return length > 1e-12f ? v / length : Vector3();
}

void BarycentricOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c, float& u, float& v, float& w) {
	Vector3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = v0.dot(v0), d01 = v0.dot(v1), d11 = v1.dot(v1), d20 = v2.dot(v0), d21 = v2.dot(v1);
	float denom = d00 * d11 - d01 * d01;
	if (std::fabs(denom) < 1e-20f) {
		u = 1.0f;
		v = w = 0.0f;
		return;
	}
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0f - v - w;
	u = std::max(0.0f, u);
	v = std::max(0.0f, v);
	w = std::max(0.0f, w);
	float sum = u + v + w;
	if (sum > 0.0f) {
		u /= sum;
		v /= sum;
		w /= sum;
	}
}

bool CleanPull(const Vector3& from, const SurfaceQuery::Hit& hit) {
	return SurfaceQuery::CleanPull(from, hit);
}

float Falloff(float distance, float maxDistance) {
	float t = std::clamp(distance / maxDistance, 0.0f, 1.0f);
	float f = 1.0f - t * t;
	return f * f;
}

// A pull of a point on the mesh, given by a triangle and barycentric weights, towards a surface point
struct SurfacePull {
	int unknowns[3] = {0, 0, 0};
	float bary[3] = {0.0f, 0.0f, 0.0f};
	Vector3 target;
	float weight = 0.0f;
};

// The welded mesh the solver works on: one unknown per group of vertex copies
struct Welded {
	std::vector<int> unknownOf;			   // per vertex
	std::vector<Vector3> rest;			   // per unknown
	std::vector<Triangle> tris;			   // over unknowns
	std::vector<std::vector<int>> neighbors; // per unknown
};

Welded Weld(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float weldDistance) {
	Welded w;
	w.unknownOf.assign(verts.size(), -1);
	if (weldDistance > 0.0f) {
		for (auto& group : MeshAnalysis::CoincidentGroups(verts, weldDistance)) {
			int id = static_cast<int>(w.rest.size());
			w.rest.push_back(verts[group.front()]);
			for (int v : group)
				w.unknownOf[v] = id;
		}
	}
	for (size_t v = 0; v < verts.size(); v++) {
		if (w.unknownOf[v] >= 0)
			continue;
		w.unknownOf[v] = static_cast<int>(w.rest.size());
		w.rest.push_back(verts[v]);
	}

	std::vector<std::unordered_set<int>> sets(w.rest.size());
	for (auto& t : tris) {
		if (t.p1 >= verts.size() || t.p2 >= verts.size() || t.p3 >= verts.size())
			continue;
		int a = w.unknownOf[t.p1], b = w.unknownOf[t.p2], c = w.unknownOf[t.p3];
		if (a == b || b == c || a == c)
			continue;
		w.tris.emplace_back(static_cast<uint16_t>(a), static_cast<uint16_t>(b), static_cast<uint16_t>(c));
		sets[a].insert(b);
		sets[a].insert(c);
		sets[b].insert(a);
		sets[b].insert(c);
		sets[c].insert(a);
		sets[c].insert(b);
	}

	w.neighbors.resize(w.rest.size());
	for (size_t i = 0; i < sets.size(); i++) {
		w.neighbors[i].assign(sets[i].begin(), sets[i].end());
		std::sort(w.neighbors[i].begin(), w.neighbors[i].end());
	}
	return w;
}

std::vector<Vector3> TriangleNormalSums(const std::vector<Vector3>& positions, const std::vector<Triangle>& tris) {
	std::vector<Vector3> sums(positions.size());
	for (auto& t : tris) {
		if (t.p1 >= positions.size() || t.p2 >= positions.size() || t.p3 >= positions.size())
			continue;
		// The cross product's length is twice the area, so bigger triangles count more
		Vector3 n = (positions[t.p2] - positions[t.p1]).cross(positions[t.p3] - positions[t.p1]);
		sums[t.p1] += n;
		sums[t.p2] += n;
		sums[t.p3] += n;
	}
	for (auto& n : sums)
		n = Normalized(n);
	return sums;
}

// The linear system of one solve: data pulls on single unknowns, pulls on points between unknowns, and the
// smoothness of the displacement (uniform Laplacian, squared)
class System {
public:
	System(const Welded& inMesh, float inLambda)
		: mesh(inMesh)
		, lambda(inLambda) {
		const size_t n = mesh.rest.size();
		dataWeight.assign(n, 0.0f);
		dataTarget.assign(n, Vector3());
		inverseDegree.assign(n, 0.0f);
		for (size_t i = 0; i < n; i++)
			if (!mesh.neighbors[i].empty())
				inverseDegree[i] = 1.0f / static_cast<float>(mesh.neighbors[i].size());
	}

	const Welded& mesh;
	float lambda;
	// Pull of each unknown to a displacement
	std::vector<float> dataWeight;
	std::vector<Vector3> dataTarget;
	std::vector<SurfacePull> pulls;
	std::vector<float> inverseDegree;

	std::vector<Vector3> Laplacian(const std::vector<Vector3>& d) const {
		std::vector<Vector3> out(d.size());
		ParallelFor(d.size(), 2048, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				const auto& nb = mesh.neighbors[i];
				if (nb.empty())
					continue;
				Vector3 mean;
				for (int j : nb)
					mean += d[j];
				out[i] = d[i] - mean * inverseDegree[i];
			}
		});
		return out;
	}

	std::vector<Vector3> LaplacianTransposed(const std::vector<Vector3>& y) const {
		std::vector<Vector3> out(y.size());
		ParallelFor(y.size(), 2048, [&](size_t start, size_t end) {
			for (size_t k = start; k < end; k++) {
				const auto& nb = mesh.neighbors[k];
				if (nb.empty())
					continue;
				Vector3 sum;
				for (int i : nb)
					sum += y[i] * inverseDegree[i];
				out[k] = y[k] - sum;
			}
		});
		return out;
	}

	// A times d
	std::vector<Vector3> Apply(const std::vector<Vector3>& d) const {
		std::vector<Vector3> out = LaplacianTransposed(Laplacian(d));
		const float membrane = lambda * MembraneShare;
		for (size_t i = 0; i < d.size(); i++) {
			out[i] = out[i] * lambda + d[i] * (dataWeight[i] + Regularization);
			// The membrane: differences to the neighbors
			Vector3 difference;
			for (int j : mesh.neighbors[i])
				difference += d[i] - d[j];
			out[i] += difference * membrane;
		}
		for (auto& p : pulls) {
			Vector3 at = d[p.unknowns[0]] * p.bary[0] + d[p.unknowns[1]] * p.bary[1] + d[p.unknowns[2]] * p.bary[2];
			for (int k = 0; k < 3; k++)
				out[p.unknowns[k]] += at * (p.weight * p.bary[k]);
		}
		return out;
	}

	std::vector<Vector3> RightHandSide() const {
		std::vector<Vector3> b(mesh.rest.size());
		for (size_t i = 0; i < b.size(); i++)
			b[i] = dataTarget[i] * dataWeight[i];
		for (auto& p : pulls) {
			Vector3 restAt = mesh.rest[p.unknowns[0]] * p.bary[0] + mesh.rest[p.unknowns[1]] * p.bary[1] + mesh.rest[p.unknowns[2]] * p.bary[2];
			Vector3 want = p.target - restAt;
			for (int k = 0; k < 3; k++)
				b[p.unknowns[k]] += want * (p.weight * p.bary[k]);
		}
		return b;
	}

	std::vector<float> Diagonal() const {
		const size_t n = mesh.rest.size();
		std::vector<float> diag(n);
		for (size_t k = 0; k < n; k++) {
			float ltl = mesh.neighbors[k].empty() ? 0.0f : 1.0f;
			for (int i : mesh.neighbors[k])
				ltl += inverseDegree[i] * inverseDegree[i];
			diag[k] = lambda * ltl + dataWeight[k] + Regularization + lambda * MembraneShare * static_cast<float>(mesh.neighbors[k].size());
		}
		for (auto& p : pulls)
			for (int k = 0; k < 3; k++)
				diag[p.unknowns[k]] += p.weight * p.bary[k] * p.bary[k];
		return diag;
	}

	// Keeps pieces without any pull from drifting, they follow afterwards
	static constexpr float Regularization = 1e-6f;
	// Weight of the membrane (first order) smoothness against the squared Laplacian. Without it, parts nothing pulls,
	// like the foot of a boot past a body's open ankle, carry the displacement's slope on and drift far off.
	static constexpr float MembraneShare = 0.1f;
};

double Dot(const std::vector<Vector3>& a, const std::vector<Vector3>& b) {
	double sum = 0.0;
	for (size_t i = 0; i < a.size(); i++)
		sum += static_cast<double>(a[i].x) * b[i].x + static_cast<double>(a[i].y) * b[i].y + static_cast<double>(a[i].z) * b[i].z;
	return sum;
}

// Preconditioned conjugate gradients from the displacement d as it is
void Solve(const System& system, std::vector<Vector3>& d, int maxIterations) {
	const size_t n = d.size();
	std::vector<Vector3> b = system.RightHandSide();
	std::vector<float> diag = system.Diagonal();

	std::vector<Vector3> ad = system.Apply(d);
	std::vector<Vector3> r(n), z(n), p(n);
	for (size_t i = 0; i < n; i++) {
		r[i] = b[i] - ad[i];
		z[i] = r[i] / diag[i];
	}
	p = z;
	double rz = Dot(r, z);
	const double bNorm = std::max(Dot(b, b), 1e-20);

	for (int it = 0; it < maxIterations; it++) {
		if (Dot(r, r) < 1e-12 * bNorm)
			break;

		std::vector<Vector3> ap = system.Apply(p);
		double pap = Dot(p, ap);
		if (pap <= 0.0)
			break;
		float alpha = static_cast<float>(rz / pap);
		for (size_t i = 0; i < n; i++) {
			d[i] += p[i] * alpha;
			r[i] -= ap[i] * alpha;
			z[i] = r[i] / diag[i];
		}
		double rzNew = Dot(r, z);
		float beta = static_cast<float>(rzNew / std::max(rz, 1e-30));
		rz = rzNew;
		for (size_t i = 0; i < n; i++)
			p[i] = z[i] + p[i] * beta;
	}
}

// Connected pieces of the welded mesh
std::vector<int> Components(const Welded& mesh, int& outCount) {
	std::vector<int> component(mesh.rest.size(), -1);
	outCount = 0;
	std::vector<int> stack;
	for (size_t start = 0; start < mesh.rest.size(); start++) {
		if (component[start] >= 0)
			continue;
		component[start] = outCount;
		stack.assign(1, static_cast<int>(start));
		while (!stack.empty()) {
			int i = stack.back();
			stack.pop_back();
			for (int j : mesh.neighbors[i])
				if (component[j] < 0) {
					component[j] = outCount;
					stack.push_back(j);
				}
		}
		outCount++;
	}
	return component;
}

} // namespace

SurfaceQuery::SurfaceQuery(const std::vector<Vector3>& inVerts, const std::vector<Triangle>& inTris, const std::vector<Vector3>& inNormals)
	: verts(inVerts)
	, tris(inTris)
	, normals(inNormals) {
	if (!verts.empty() && !tris.empty()) {
		tree = std::make_unique<AABBTree>(verts.data(), tris.data(), static_cast<uint32_t>(tris.size()), 100, 2);
		AABBTree::BuildTriangleDistanceData(verts.data(), tris.data(), static_cast<uint32_t>(tris.size()), distanceData);
	}
}

SurfaceQuery::~SurfaceQuery() = default;

SurfaceQuery::Hit SurfaceQuery::Closest(const Vector3& point, float radius) const {
	Hit hit;
	if (!tree)
		return hit;

	Vector3 origin = point;
	uint32_t facet = 0;
	Vector3 closest;
	if (!tree->ClosestFacetInSphere(origin, radius, facet, &closest, &distanceData) || facet >= tris.size())
		return hit;

	const Triangle& t = tris[facet];
	if (t.p1 >= verts.size() || t.p2 >= verts.size() || t.p3 >= verts.size())
		return hit;

	BarycentricOnTriangle(closest, verts[t.p1], verts[t.p2], verts[t.p3], hit.bary[0], hit.bary[1], hit.bary[2]);
	if (normals.size() == verts.size())
		hit.normal = Normalized(normals[t.p1] * hit.bary[0] + normals[t.p2] * hit.bary[1] + normals[t.p3] * hit.bary[2]);
	if (hit.normal.IsZero())
		hit.normal = Normalized((verts[t.p2] - verts[t.p1]).cross(verts[t.p3] - verts[t.p1]));
	hit.found = true;
	hit.triangle = facet;
	hit.point = closest;
	return hit;
}

bool SurfaceQuery::CleanPull(const Vector3& from, const Hit& hit) {
	Vector3 line = from - hit.point;
	float distance = line.length();
	if (distance < OnSurface)
		return true;
	return std::fabs(line.dot(hit.normal)) / distance >= MinLineAgreement;
}

std::vector<Vector3> VertexNormals(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float weldDistance) {
	Welded welded = Weld(verts, tris, weldDistance);
	std::vector<Vector3> unknownNormals = TriangleNormalSums(welded.rest, welded.tris);
	std::vector<Vector3> normals(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		normals[v] = unknownNormals[welded.unknownOf[v]];
	return normals;
}

Result Fit(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const std::vector<Vector3>& surfaceVerts, const std::vector<Triangle>& surfaceTris, const Options& options) {
	Result result;
	result.positions = verts;
	if (verts.empty() || surfaceVerts.empty() || surfaceTris.empty())
		return result;

	const Welded mesh = Weld(verts, tris, options.weldDistance);
	const size_t n = mesh.rest.size();
	const float maxDistance = std::max(options.maxDistance, 1e-3f);

	// Per unknown: its target gap, how much it's locked and its rigid part, from its vertex copies
	std::vector<float> gapOf(n, options.gap);
	std::vector<float> lockOf(n, 0.0f);
	std::vector<int> partOf(n, -1);
	const bool perVertexGaps = options.targetGaps.size() == verts.size();
	for (size_t v = 0; v < verts.size(); v++) {
		int u = mesh.unknownOf[v];
		if (perVertexGaps)
			gapOf[u] = options.targetGaps[v];
		if (options.lock.size() == verts.size())
			lockOf[u] = std::max(lockOf[u], std::clamp(options.lock[v], 0.0f, 1.0f));
		if (options.rigidParts.size() == verts.size())
			partOf[u] = options.rigidParts[v];
	}

	// The relief of the mesh: how far each vertex stands out along its normal from the mesh smoothed. Taubin's
	// smoothing alternates a shrinking and an inflating step, so a sleeve or boot keeps its width and only its
	// relief counts, not its roundness.
	if (options.reliefSmoothing > 0) {
		std::vector<Vector3> smoothed = mesh.rest;
		auto step = [&](float factor) {
			std::vector<Vector3> next = smoothed;
			for (size_t i = 0; i < n; i++) {
				if (mesh.neighbors[i].empty())
					continue;
				Vector3 mean;
				for (int j : mesh.neighbors[i])
					mean += smoothed[j];
				mean /= static_cast<float>(mesh.neighbors[i].size());
				next[i] = smoothed[i] + (mean - smoothed[i]) * factor;
			}
			smoothed.swap(next);
		};
		for (int pass = 0; pass < options.reliefSmoothing; pass++) {
			step(0.5f);
			step(-0.53f);
		}
		const std::vector<Vector3> restNormals = TriangleNormalSums(mesh.rest, mesh.tris);
		for (size_t i = 0; i < n; i++)
			gapOf[i] += (mesh.rest[i] - smoothed[i]).dot(restNormals[i]);
	}
	for (size_t i = 0; i < n; i++)
		gapOf[i] = std::max(gapOf[i], options.minGap);

	const std::vector<Vector3> surfaceNormals = VertexNormals(surfaceVerts, surfaceTris, 1e-4f);
	const SurfaceQuery surface(surfaceVerts, surfaceTris, surfaceNormals);

	// The gaps as they are, measured the way the fit pulls
	if (options.keepGaps) {
		ParallelFor(n, 512, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				auto hit = surface.Closest(mesh.rest[i], maxDistance);
				if (hit.found && CleanPull(mesh.rest[i], hit))
					gapOf[i] = std::max((mesh.rest[i] - hit.point).dot(hit.normal), options.minGap);
			}
		});
	}

	// Surface points that can pull the mesh in the symmetric fit, spread evenly when there are many
	std::vector<size_t> pullingSurfaceVerts;
	if (options.symmetric) {
		const size_t step = std::max<size_t>(1, surfaceVerts.size() / 40000);
		for (size_t j = 0; j < surfaceVerts.size(); j += step)
			pullingSurfaceVerts.push_back(j);
	}

	std::vector<Vector3> d(n);
	std::vector<float> lastWeight(n, 0.0f);

	// Clipping of the mesh at a displacement: its vertices inside the surface, and surface vertices that poke through
	// it between its vertices (a dense body bulging through a coarse corset)
	auto findClipping = [&](const std::vector<Vector3>& displacement, std::vector<SurfacePull>* outPokes, std::vector<char>* outInside) {
		std::vector<Vector3> current(n);
		for (size_t i = 0; i < n; i++)
			current[i] = mesh.rest[i] + displacement[i];
		std::vector<char> inside(n, 0);
		ParallelFor(n, 512, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				auto hit = surface.Closest(current[i], maxDistance);
				if (hit.found && CleanPull(current[i], hit) && (current[i] - hit.point).dot(hit.normal) < 0.0f)
					inside[i] = 1;
			}
		});

		// Surface vertices with the mesh close behind them, looking into the surface: they show through it
		std::vector<SurfacePull> pokes;
		if (!mesh.tris.empty()) {
			std::vector<PokeThrough> found = ClippingFixer::BodyPokeThrough(surfaceVerts, surfaceTris, current, mesh.tris, options.pokeDepth);
			for (size_t j = 0; j < found.size(); j++) {
				if (found[j].triangle < 0)
					continue;
				const Triangle& t = mesh.tris[found[j].triangle];
				// Where the mesh is crossed, pushed out to just outside the surface vertex
				const Vector3 crossing = surfaceVerts[j] - surfaceNormals[j] * found[j].depth;
				SurfacePull pull;
				pull.unknowns[0] = t.p1;
				pull.unknowns[1] = t.p2;
				pull.unknowns[2] = t.p3;
				BarycentricOnTriangle(crossing, current[t.p1], current[t.p2], current[t.p3], pull.bary[0], pull.bary[1], pull.bary[2]);
				pull.target = surfaceVerts[j] + surfaceNormals[j] * options.minGap;
				pull.weight = 1.0f;
				pokes.push_back(pull);
			}
		}
		if (outPokes)
			*outPokes = pokes;
		if (outInside)
			*outInside = inside;
		return std::make_pair(static_cast<size_t>(std::count(inside.begin(), inside.end(), 1)), pokes.size());
	};

	{
		auto before = findClipping(d, nullptr, nullptr);
		result.insideBefore = before.first;
		result.pokeBefore = before.second;
	}

	// How strongly each vertex is pulled: lowered where pulling would stretch the mesh too much
	std::vector<float> pullScale(n, 1.0f);

	// One correspondence update and solve
	auto iterate = [&](float lambda) {
		std::vector<Vector3> current(n);
		for (size_t i = 0; i < n; i++)
			current[i] = mesh.rest[i] + d[i];
		const std::vector<Vector3> currentNormals = TriangleNormalSums(current, mesh.tris);

		System system(mesh, lambda);
		ParallelFor(n, 512, [&](size_t start, size_t end) {
			for (size_t i = start; i < end; i++) {
				if (lockOf[i] >= 1.0f)
					continue;
				auto hit = surface.Closest(current[i], maxDistance);
				if (!hit.found)
					continue;
				if (options.minNormalAgreement > -1.0f && currentNormals[i].dot(hit.normal) < options.minNormalAgreement)
					continue;
				if (!CleanPull(current[i], hit))
					continue;

				float distance = current[i].DistanceTo(hit.point);
				system.dataWeight[i] = Falloff(distance, maxDistance) * pullScale[i];
				if (options.keepGaps) {
					// Only along the normal, so nothing slides across the surface
					float signedDistance = (current[i] - hit.point).dot(hit.normal);
					system.dataTarget[i] = current[i] + hit.normal * (gapOf[i] - signedDistance) - mesh.rest[i];
				}
				else
					system.dataTarget[i] = hit.point + hit.normal * gapOf[i] - mesh.rest[i];
			}
		});

		// Masked vertices stay where they are, and their neighbors bend around them smoothly
		const float lockWeight = 1000.0f * (1.0f + lambda);
		for (size_t i = 0; i < n; i++) {
			if (lockOf[i] > 0.0f) {
				float keep = lockWeight * lockOf[i];
				float pull = system.dataWeight[i] * (1.0f - lockOf[i]);
				system.dataTarget[i] = (system.dataTarget[i] * pull) / std::max(pull + keep, 1e-20f);
				system.dataWeight[i] = pull + keep;
			}
		}

		if (options.symmetric && !mesh.tris.empty()) {
			// Points on the mesh as it is now, so the pulls go to where it has moved
			SurfaceQuery meshQuery(current, mesh.tris, currentNormals);
			std::vector<SurfacePull> found(pullingSurfaceVerts.size());
			ParallelFor(pullingSurfaceVerts.size(), 512, [&](size_t start, size_t end) {
				for (size_t k = start; k < end; k++) {
					size_t j = pullingSurfaceVerts[k];
					auto hit = meshQuery.Closest(surfaceVerts[j], maxDistance);
					if (!hit.found)
						continue;
					if (options.minNormalAgreement > -1.0f && hit.normal.dot(surfaceNormals[j]) < options.minNormalAgreement)
						continue;
					if (!CleanPull(surfaceVerts[j], hit))
						continue;

					const Triangle& t = mesh.tris[hit.triangle];
					SurfacePull& pull = found[k];
					pull.unknowns[0] = t.p1;
					pull.unknowns[1] = t.p2;
					pull.unknowns[2] = t.p3;
					for (int m = 0; m < 3; m++)
						pull.bary[m] = hit.bary[m];
					pull.target = surfaceVerts[j] + surfaceNormals[j] * options.gap;
					// Half as strong as the mesh's own pulls, and not on locked vertices
					float locked = std::max({lockOf[t.p1], lockOf[t.p2], lockOf[t.p3]});
					float scale = std::min({pullScale[t.p1], pullScale[t.p2], pullScale[t.p3]});
					pull.weight = 0.5f * Falloff(hit.point.DistanceTo(surfaceVerts[j]), maxDistance) * (1.0f - locked) * scale;
				}
			});
			for (auto& pull : found)
				if (pull.weight > 0.0f)
					system.pulls.push_back(pull);
		}

		lastWeight = system.dataWeight;
		Solve(system, d, options.solverIterations);
	};

	const float finalLambda = std::max(options.stiffness, 1e-4f);
	for (int stage = 0; stage < std::max(1, options.stages); stage++) {
		// Stiff first, so the mesh moves as a whole before the details fit
		const int stagesLeft = std::max(1, options.stages) - 1 - stage;
		const float lambda = finalLambda * std::pow(4.0f, static_cast<float>(stagesLeft));
		for (int iteration = 0; iteration < std::max(1, options.iterations); iteration++)
			iterate(lambda);
	}

	// Where the fit stretches or squashes edges by more than maxStrain, the pull there and around it gives way and the
	// mesh keeps its own form, e.g. a choker that the body's shorter neck would drag down over the shoulders
	if (options.maxStrain > 0.0f) {
		std::vector<std::vector<int>> edgesOf(n);
		for (auto& t : mesh.tris) {
			edgesOf[t.p1].push_back(t.p2);
			edgesOf[t.p1].push_back(t.p3);
			edgesOf[t.p2].push_back(t.p3);
		}
		for (int round = 0; round < 4; round++) {
			std::vector<char> strained(n, 0);
			size_t count = 0;
			for (size_t a = 0; a < n; a++)
				for (int b : edgesOf[a]) {
					float before = mesh.rest[a].DistanceTo(mesh.rest[b]);
					float after = (mesh.rest[a] + d[a]).DistanceTo(mesh.rest[b] + d[b]);
					float shorter = std::min(before, after), longer = std::max(before, after);
					if (shorter > 1e-6f && longer / shorter - 1.0f > options.maxStrain) {
						count += !strained[a] + !strained[b];
						strained[a] = strained[b] = 1;
					}
				}
			if (count == 0)
				break;
			result.relaxed += count;

			// The strained vertices and their neighbors pull less
			std::vector<char> relax = strained;
			for (size_t a = 0; a < n; a++)
				if (strained[a])
					for (int b : mesh.neighbors[a])
						relax[b] = 1;
			for (size_t a = 0; a < n; a++)
				if (relax[a])
					pullScale[a] *= 0.25f;

			for (int iteration = 0; iteration < std::max(1, options.iterations); iteration++)
				iterate(finalLambda);
		}
	}

	// Pieces that nothing pulled (a buckle the surface is too far from) move with the closest pulled vertices, as a whole
	int componentCount = 0;
	std::vector<int> component = Components(mesh, componentCount);
	std::vector<double> componentPull(componentCount, 0.0);
	for (size_t i = 0; i < n; i++)
		if (lockOf[i] < 1.0f)
			componentPull[component[i]] += lastWeight[i];

	std::vector<Vector3> pulledPoints;
	std::vector<int> pulledUnknowns;
	for (size_t i = 0; i < n; i++)
		if (componentPull[component[i]] > 0.0) {
			pulledPoints.push_back(mesh.rest[i]);
			pulledUnknowns.push_back(static_cast<int>(i));
		}

	if (!pulledPoints.empty()) {
		kd_tree<uint32_t> tree(pulledPoints.data(), static_cast<uint32_t>(pulledPoints.size()));
		std::vector<Vector3> follow(componentCount);
		std::vector<int> followCount(componentCount, 0);
		for (size_t i = 0; i < n; i++) {
			int c = component[i];
			if (componentPull[c] > 0.0 || lockOf[i] >= 1.0f)
				continue;
			Vector3 point = mesh.rest[i];
			if (tree.kd_nn(&point, 0.0f) == 0)
				continue;
			follow[c] += d[pulledUnknowns[tree.queryResult.front().vertex_index]];
			followCount[c]++;
		}
		for (size_t i = 0; i < n; i++) {
			int c = component[i];
			if (followCount[c] > 0) {
				d[i] = follow[c] / static_cast<float>(followCount[c]);
				result.followed++;
			}
		}
	}

	// Rigid parts move as a whole, by the motion that best fits the displacement of their vertices
	const bool hasRigidParts = std::any_of(partOf.begin(), partOf.end(), [](int p) { return p >= 0; });
	std::vector<float> rigidInfluence(n);
	for (size_t i = 0; i < n; i++)
		rigidInfluence[i] = partOf[i] >= 0 ? 1.0f : 0.0f;
	if (hasRigidParts)
		d = MeshAnalysis::RigidifyDeltas(mesh.rest, d, partOf, rigidInfluence, options.rigidScale);
	std::map<int, size_t> partSize;
	for (int p : partOf)
		if (p >= 0)
			partSize[p]++;
	// Small parts like chain links fold when they give way, so they stay at least half rigid
	constexpr size_t SmallPart = 64;

	for (size_t i = 0; i < n; i++)
		d[i] *= 1.0f - lockOf[i];

	// Clipping left over: vertices inside the surface are pushed out to minGap, and where the surface pokes through
	// between vertices, the mesh there is pushed out over it. Everything else holds its place, and the smoothness
	// spreads each push over the neighbors, so the mesh bulges out gently instead of denting.
	for (int pass = 0; pass < options.clippingPasses; pass++) {
		std::vector<SurfacePull> pokes;
		std::vector<char> inside;
		auto clipping = findClipping(d, &pokes, &inside);
		if (clipping.first == 0 && clipping.second == 0)
			break;

		std::vector<Vector3> current(n);
		for (size_t i = 0; i < n; i++)
			current[i] = mesh.rest[i] + d[i];

		System system(mesh, std::max(options.stiffness, 1e-4f));
		const float push = 20.0f;
		for (size_t i = 0; i < n; i++) {
			// Hold the fit
			system.dataWeight[i] = 1.0f;
			system.dataTarget[i] = d[i];
			if (lockOf[i] > 0.0f) {
				system.dataWeight[i] = 1000.0f * lockOf[i] + 1.0f;
				continue;
			}
			if (!inside[i])
				continue;
			auto hit = surface.Closest(current[i], maxDistance);
			if (!hit.found)
				continue;
			// Out to the least gap, and a little more each pass that still finds it inside
			system.dataWeight[i] = push;
			system.dataTarget[i] = hit.point + hit.normal * (options.minGap * (1.0f + pass)) - mesh.rest[i];
		}
		for (auto& poke : pokes) {
			const float locked = std::max({lockOf[poke.unknowns[0]], lockOf[poke.unknowns[1]], lockOf[poke.unknowns[2]]});
			if (locked >= 1.0f)
				continue;
			poke.weight = push * (1.0f - locked);
			system.pulls.push_back(poke);
		}
		Solve(system, d, options.solverIterations);

		// Rigid parts stay rigid at first, and give way bit by bit where they still clip, so the last passes don't
		// undo their own pushes
		if (hasRigidParts) {
			const float share = 1.0f - static_cast<float>(pass + 1) / static_cast<float>(options.clippingPasses);
			std::vector<float> influence(n);
			for (size_t i = 0; i < n; i++)
				influence[i] = rigidInfluence[i] * (partOf[i] >= 0 && partSize[partOf[i]] <= SmallPart ? std::max(0.5f, share) : share);
			d = MeshAnalysis::RigidifyDeltas(mesh.rest, d, partOf, influence, options.rigidScale);
		}
	}
	{
		auto after = findClipping(d, nullptr, nullptr);
		result.insideAfter = after.first;
		result.pokeAfter = after.second;
	}

	for (size_t v = 0; v < verts.size(); v++)
		result.positions[v] = verts[v] + d[mesh.unknownOf[v]];

	// How well the pulled vertices fit now
	std::vector<Vector3> fitted(n);
	for (size_t i = 0; i < n; i++)
		fitted[i] = mesh.rest[i] + d[i];
	double errorSum = 0.0;
	for (size_t i = 0; i < n; i++) {
		if (lastWeight[i] <= 0.0f || lockOf[i] >= 1.0f)
			continue;
		auto hit = surface.Closest(fitted[i], maxDistance);
		if (!hit.found)
			continue;
		result.pulled++;
		float signedDistance = (fitted[i] - hit.point).dot(hit.normal);
		float error = std::fabs(signedDistance - gapOf[i]);
		errorSum += error;
		result.maxGapError = std::max(result.maxGapError, error);
	}
	if (result.pulled)
		result.meanGapError = static_cast<float>(errorSum / static_cast<double>(result.pulled));

	double moveSum = 0.0;
	for (size_t v = 0; v < verts.size(); v++) {
		float move = verts[v].DistanceTo(result.positions[v]);
		moveSum += move;
		result.maxMove = std::max(result.maxMove, move);
	}
	result.meanMove = static_cast<float>(moveSum / static_cast<double>(verts.size()));
	result.flipped = MeshAnalysis::CountFlippedTriangles(verts, result.positions, tris);

	// How much the fit stretched or squashed the mesh, by its edges
	MeshAnalysis::StrainStats strain = MeshAnalysis::AnalyzeStrain(verts, result.positions, tris, 0.25f);
	result.maxStrain = strain.maxStrain;
	result.meanStrain = strain.meanStrain;
	result.strainedEdges = strain.stretched;
	return result;
}

} // namespace SurfaceFit
