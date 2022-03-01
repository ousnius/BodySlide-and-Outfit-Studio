/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Mesh.h"
#include "KDMatcher.hpp"

using namespace nifly;

Mesh::Mesh() {}

Mesh::~Mesh() {
	if (genBuffers) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glDeleteBuffers(static_cast<GLsizei>(vbo.size()), vbo.data());
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}
}

std::shared_ptr<AABBTree> Mesh::CreateBVH() {
	if (verts && tris && nTris > 0)
		bvh = std::make_shared<AABBTree>(verts.get(), tris.get(), nTris, 100, 2);
	else
		bvh.reset();

	return bvh;
}

void Mesh::BuildTriAdjacency() {
	if (!tris)
		return;

	vertTris = std::make_unique<std::vector<int>[]>(nVerts);
	auto vt = vertTris.get();
	for (int t = 0; t < nTris; t++) {
		uint16_t i1 = tris[t].p1;
		uint16_t i2 = tris[t].p2;
		uint16_t i3 = tris[t].p3;
		if (i1 >= nVerts || i2 >= nVerts || i3 >= nVerts)
			continue;

		vt[i1].push_back(t);
		vt[i2].push_back(t);
		vt[i3].push_back(t);
	}
}

void Mesh::BuildVertexAdjacency() {
	if (!tris)
		return;

	adjVerts = std::make_unique<std::vector<int>[]>(nVerts);
	auto av = adjVerts.get();

	std::vector<std::set<int>> adjArray(nVerts);

	auto addWeldVerts = [&](int adj, int qp) {
		auto wv = weldVerts.find(qp);
		if (wv != weldVerts.end()) {
			adjArray[adj].insert(wv->second.begin(), wv->second.end());
		}
	};
	
	for (int t = 0; t < nTris; t++) {
		auto& tri = tris[t];

		adjArray[tri.p1].insert(tri.p2);
		adjArray[tri.p1].insert(tri.p3);
		addWeldVerts(tri.p1, tri.p2);
		addWeldVerts(tri.p1, tri.p3);

		adjArray[tri.p2].insert(tri.p1);
		adjArray[tri.p2].insert(tri.p3);
		addWeldVerts(tri.p2, tri.p1);
		addWeldVerts(tri.p2, tri.p3);

		adjArray[tri.p3].insert(tri.p1);
		adjArray[tri.p3].insert(tri.p2);
		addWeldVerts(tri.p3, tri.p1);
		addWeldVerts(tri.p3, tri.p2);
	}

	for (int v = 0; v < nVerts; v++) {
		av[v].resize(adjArray[v].size());
		std::copy(adjArray[v].begin(), adjArray[v].end(), av[v].begin());
	}
}

void Mesh::MakeEdges() {
	if (!tris)
		return;
	if (edges)
		return;

	nEdges = nTris * 3;
	edges = std::make_unique<Edge[]>(nEdges);

	for (int i = 0; i < nEdges; i++) {
		// Find correct points for edge
		uint16_t* points = &tris[i / 3].p1;
		int pA = i % 3;
		int pB = i % 3 + 1;
		if (pB >= 3)
			pB = 0;

		// Create edge from points
		edges[i] = Edge(std::min(points[pA], points[pB]), std::max(points[pA], points[pB]));
	}
}

void Mesh::BuildEdgeList() {
	if (!edges)
		MakeEdges();

	vertEdges = std::make_unique<std::vector<int>[]>(nVerts);
	auto ve = vertEdges.get();
	for (int e = 0; e < nEdges; e++) {
		uint16_t i1 = edges[e].p1;
		uint16_t i2 = edges[e].p2;
		if (i1 >= nVerts || i2 >= nVerts)
			continue;

		ve[edges[e].p1].push_back(e);
		ve[edges[e].p2].push_back(e);
	}
}

void Mesh::CreateBuffers() {
	if (!genBuffers) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(static_cast<GLsizei>(vbo.size()), vbo.data());
		glGenBuffers(1, &ibo);
	}

	// NumVertices * (Position + Normal + Colors + Texture Coordinates)
	glBindVertexArray(vao);

	if (verts) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), verts.get(), GL_DYNAMIC_DRAW);
	}

	if (norms) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), norms.get(), GL_DYNAMIC_DRAW);
	}

	if (tangents) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), tangents.get(), GL_DYNAMIC_DRAW);
	}

	if (bitangents) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), bitangents.get(), GL_DYNAMIC_DRAW);
	}

	if (vcolors) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), vcolors.get(), GL_DYNAMIC_DRAW);
	}

	if (valpha) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), valpha.get(), GL_DYNAMIC_DRAW);
	}

	if (texcoord) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord.get(), GL_DYNAMIC_DRAW);
	}

	if (mask) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), mask.get(), GL_DYNAMIC_DRAW);
	}

	if (weight) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), weight.get(), GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element index array
	if (verts && tris) {
		renderTris = std::make_unique<Triangle[]>(nTris);
		for (int i = 0; i < nTris; ++i)
			renderTris[i] = tris[i];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTris * sizeof(Triangle), renderTris.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else if (verts && edges) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nEdges * sizeof(Edge), edges.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
	genBuffers = true;
}

void Mesh::UpdateBuffers() {
	if (genBuffers) {
		glBindVertexArray(vao);

		if (verts && queueUpdate[UpdateType::Position]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Position]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), verts.get());
			queueUpdate[UpdateType::Position] = false;
		}

		if (norms && queueUpdate[UpdateType::Normals]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Normals]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), norms.get());
			queueUpdate[UpdateType::Normals] = false;
		}

		if (tangents && queueUpdate[UpdateType::Tangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Tangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), tangents.get());
			queueUpdate[UpdateType::Tangents] = false;
		}

		if (bitangents && queueUpdate[UpdateType::Bitangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Bitangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), bitangents.get());
			queueUpdate[UpdateType::Bitangents] = false;
		}

		if (vcolors && queueUpdate[UpdateType::VertexColors]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::VertexColors]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), vcolors.get());
			queueUpdate[UpdateType::VertexColors] = false;
		}

		if (valpha && queueUpdate[UpdateType::VertexAlpha]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::VertexAlpha]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), valpha.get());
			queueUpdate[UpdateType::VertexAlpha] = false;
		}

		if (texcoord && queueUpdate[UpdateType::TextureCoordinates]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::TextureCoordinates]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector2), texcoord.get());
			queueUpdate[UpdateType::TextureCoordinates] = false;
		}

		if (mask && queueUpdate[UpdateType::Mask]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Mask]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), mask.get());
			queueUpdate[UpdateType::Mask] = false;
		}

		if (weight && queueUpdate[UpdateType::Weight]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Weight]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), weight.get());
			queueUpdate[UpdateType::Weight] = false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (queueUpdate[UpdateType::Indices]) {
			if (tris) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nTris * sizeof(Triangle), renderTris.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else if (edges) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nEdges * sizeof(Edge), edges.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			queueUpdate[UpdateType::Indices] = false;
		}

		glBindVertexArray(0);
	}
}

void Mesh::QueueUpdate(const UpdateType& type) {
	queueUpdate[type] = true;
}

void Mesh::UpdateFromMaterialFile(const MaterialFile& matFile) {
	doublesided = matFile.twoSided;
	modelSpace = matFile.modelSpaceNormals;
	emissive = matFile.emitEnabled;
	specular = matFile.specularEnabled;
	backlight = matFile.backLighting;
	rimlight = matFile.rimLighting;
	softlight = matFile.subsurfaceLighting;
	glowmap = matFile.glowMap;
	greyscaleColor = matFile.grayscaleToPaletteColor;
	cubemap = matFile.environmentMapping;

	prop.alpha = matFile.alpha;
	prop.uvOffset = matFile.uvOffset;
	prop.uvScale = matFile.uvScale;
	prop.specularColor = matFile.specularColor;
	prop.specularStrength = matFile.specularMult;
	prop.shininess = matFile.smoothness;
	prop.emissiveColor = matFile.emittanceColor;
	prop.emissiveMultiple = matFile.emittanceMult;
	prop.envReflection = matFile.environmentMappingMaskScale;
	prop.backlightPower = matFile.backLightPower;
	prop.rimlightPower = matFile.rimPower;
	prop.subsurfaceRolloff = matFile.subsurfaceLightingRolloff;
	prop.fresnelPower = matFile.fresnelPower;
	prop.paletteScale = matFile.grayscaleToPaletteScale;
}

bool Mesh::HasAlphaBlend() {
	bool alphaBlend = alphaFlags & 1;
	if (prop.alpha < 1.0f)
		alphaBlend = true;

	return alphaBlend;
}

void Mesh::ScaleVertices(const Vector3& center, const float& factor) {
	for (int i = 0; i < nVerts; i++)
		verts[i] = center + (verts[i] - center) * factor;

	CreateBVH();
	queueUpdate[UpdateType::Position] = true;
}

void Mesh::GetAdjacentPoints(int querypoint, std::unordered_set<int>& outPoints) {
	if (querypoint >= nVerts)
		return;

	if (!adjVerts)
		BuildVertexAdjacency();

	if (!adjVerts)
		return;

	auto av = adjVerts.get();
	const auto& avPoint = av[querypoint];

	outPoints.insert(avPoint.begin(), avPoint.end());
}

int Mesh::GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints) {
	if (querypoint >= nVerts)
		return 0;

	if (!adjVerts)
		BuildVertexAdjacency();

	if (!adjVerts)
		return 0;

	auto av = adjVerts.get();
	const auto& avPoint = av[querypoint];

	int n = 0;
	for (auto& p : avPoint) {
		if (n == maxPoints)
			break;

		outPoints[n++] = p;
	}

	return n;
	/* TODO: sort by distance */
}

int Mesh::GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint) const {
	int n = 0;
	for (int p : adjVerts[querypoint])
		if (n + 1 < maxPoints && !visPoint[p]) {
			outPoints[n++] = p;
			visPoint[p] = true;
		}
	return n;
	/* TODO: sort by distance */
}

int Mesh::FindAdjacentBalancedPairs(int pt, int pairs[]) const {
	// If adjPts contains any welded points, this algorithm should pick
	// out just one balanced pair from among the welds.

	const std::vector<int>& adjPts = adjVerts[pt];
	int c = std::min(static_cast<int>(adjPts.size()), MaxAdjacentPoints);
	if (c == 0)
		return 0;

	// Calculate direction from pt to each adjPts
	Vector3 pDirs[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i) {
		Vector3 pDir = verts[adjPts[i]] - verts[pt];
		pDir.Normalize();
		pDirs[i] = pDir;
	}

	// For each point in adjPts, find which of the other neighboring points
	// has the lowest direction dot product with it.  Ideally, we want -1
	// for each matching pair.
	int matchInd[MaxAdjacentPoints];
	float matchDot[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i) {
		int bestInd = i;
		float bestDot = 1;
		for (int j = 0; j < c; ++j) {
			float dot = pDirs[j].dot(pDirs[i]);
			if (dot < bestDot) {
				bestDot = dot;
				bestInd = j;
			}
		}
		matchInd[i] = bestInd;
		matchDot[i] = bestDot;
	}

	// Repeatedly pick out the best pairs.  If a point's best match is to
	// a point that's already been paired, we won't match it to anything.
	bool donePt[MaxAdjacentPoints];
	for (int i = 0; i < c; ++i)
		donePt[i] = false;
	int pairInd = 0;
	bool gotOne = false;
	do {
		gotOne = false;
		float bestDot = 1;
		int bestInd = 0;
		for (int i = 0; i < c; ++i)
			if (!donePt[i] && !donePt[matchInd[i]] && matchDot[i] < bestDot) {
				bestDot = matchDot[i];
				bestInd = i;
				gotOne = true;
			}
		if (gotOne) {
			pairs[pairInd++] = adjPts[bestInd];
			pairs[pairInd++] = adjPts[matchInd[bestInd]];
			donePt[bestInd] = true;
			donePt[matchInd[bestInd]] = true;
		}
	} while (gotOne);

	return pairInd;
}

int Mesh::FindOpposingPoint(int p1, int p2, float maxdot) const {
	const std::vector<int>& adjPts = adjVerts[p1];
	int c = static_cast<int>(adjPts.size());
    if (c == 0)
        return -1;

    Vector3 p2dir = verts[p2] - verts[p1];
    p2dir.Normalize();

    float bestdot = maxdot;
    int bestpt = -1;
    for (int i = 0; i < c; ++i) {
        Vector3 dir = verts[adjPts[i]] - verts[p1];
        dir.Normalize();
        float dot = dir.dot(p2dir);
        if (dot < bestdot) {
            bestdot = dot;
            bestpt = adjPts[i];
        }
    }

    return bestpt;
}

void Mesh::CalcWeldVerts() {
	weldVerts.clear();

	SortingMatcher matcher(verts.get(), static_cast<uint16_t>(nVerts));
	for (const auto& matchset : matcher.matches) {
		for (size_t j = 0; j < matchset.size(); ++j) {
			std::vector<int>& wv = weldVerts[matchset[j]];
			for (size_t k = 0; k < matchset.size(); ++k) {
				if (j != k)
					wv.push_back(matchset[k]);
			}
		}
	}
	bGotWeldVerts = true;
}

float Mesh::GetSmoothThreshold() {
	return (smoothThresh * 180) / PI;
}

void Mesh::SetSmoothThreshold(float degrees) {
	smoothThresh = degrees * DEG2RAD;
}

void Mesh::SmoothNormals(const std::unordered_set<int>& vertices) {
	if (lockNormals || !norms)
		return;

	// Copy old normals into a working temporary
	std::vector<Vector3> tnorms(norms.get(), norms.get() + nVerts);

	// Zero old normals
	bool noVertices = vertices.empty();
	for (int i = 0; i < nVerts; i++) {
		if (!noVertices && vertices.count(i) == 0)
			continue;

		if (lockedNormalIndices.count(i) != 0)
			continue;

		tnorms[i].Zero();
	}

	// Face normals
	for (int t = 0; t < nTris; t++) {
		Triangle& tri = tris[t];
		bool bn1 = (noVertices || vertices.count(tri.p1) != 0);
		bool bn2 = (noVertices || vertices.count(tri.p2) != 0);
		bool bn3 = (noVertices || vertices.count(tri.p3) != 0);

		// None of the three normals should change
		if (!bn1 && !bn2 && !bn3)
			continue;

		Vector3 tn = tri.trinormal(verts.get());

		if (bn1 && lockedNormalIndices.count(tri.p1) == 0)
			tnorms[tri.p1] += tn;

		if (bn2 && lockedNormalIndices.count(tri.p2) == 0)
			tnorms[tri.p2] += tn;

		if (bn3 && lockedNormalIndices.count(tri.p3) == 0)
			tnorms[tri.p3] += tn;
	}

	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.count(i) != 0)
			continue;

		tnorms[i].Normalize();
	}

	// Smooth welded vertex normals
	if (smoothSeamNormals) {
		if (!bGotWeldVerts)
			CalcWeldVerts();

		std::vector<std::pair<int, Vector3>> seamNorms;

		for (auto& wvp : weldVerts) {
			auto& key = wvp.first;
			if (!noVertices && vertices.count(key) == 0)
				continue;

			if (lockedNormalIndices.count(key) != 0)
				continue;

			const Vector3& n = tnorms[key];
			Vector3 sn = n;
			auto& value = wvp.second;
			for (int wvi : value)
				if (n.angle(tnorms[wvi]) < smoothThresh)
					sn += tnorms[wvi];

			sn.Normalize();
			seamNorms.emplace_back(key, sn);
		}

		for (auto& snp : seamNorms)
			tnorms[snp.first] = snp.second;
	}

	// Copy temporary back into the main.
	std::copy(tnorms.begin(), tnorms.end(), norms.get());

	queueUpdate[UpdateType::Normals] = true;
	CalcTangentSpace();
}

Vector3 Mesh::GetOneVertexNormal(int vi) {
	if (vi < 0 || vi >= nVerts)
		return Vector3(1,0,0);
	if (norms)
		return norms[vi];

	// Without norms, we have to calculate it from scratch
	if (!bGotWeldVerts)
		CalcWeldVerts();

	// Sum the normals of every triangle containing vi
	Vector3 sum;
	for (int ti = 0; ti < nTris; ++ti) {
		const Triangle& t = tris[ti];
		if (t.p1 != vi && t.p2 != vi && t.p3 != vi)
			continue;
		Vector3 tn = t.trinormal(verts.get());
		tn.Normalize();
		sum += tn;
	}

	// Also sum the normals of every triangle containing a vertex welded to vi
	auto wvit = weldVerts.find(vi);
	if (wvit != weldVerts.end())
		for (int wvi : wvit->second)
			for (int ti = 0; ti < nTris; ++ti) {
				const Triangle& t = tris[ti];
				if (t.p1 != wvi && t.p2 != wvi && t.p3 != wvi)
					continue;
				Vector3 tn = t.trinormal(verts.get());
				tn.Normalize();
				sum += tn;
			}

	sum.Normalize();
	return sum;
}

void Mesh::FacetNormals() {
	if (lockNormals)
		return;

	// Zero old normals
	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.find(i) != lockedNormalIndices.end())
			continue;

		Vector3& pn = norms[i];
		pn.Zero();
	}

	for (int t = 0; t < nTris; t++) {
		auto& tri = tris[t];
		if (tri.p1 >= nVerts || tri.p2 >= nVerts || tri.p3 >= nVerts)
			continue;

		Vector3 tn = tri.trinormal(verts.get());

		if (lockedNormalIndices.find(tri.p1) == lockedNormalIndices.end()) {
			Vector3& pn1 = norms[tri.p1];
			pn1 += tn;
		}

		if (lockedNormalIndices.find(tri.p2) == lockedNormalIndices.end()) {
			Vector3& pn2 = norms[tri.p2];
			pn2 += tn;
		}

		if (lockedNormalIndices.find(tri.p3) == lockedNormalIndices.end()) {
			Vector3& pn3 = norms[tri.p3];
			pn3 += tn;
		}
	}

	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.find(i) != lockedNormalIndices.end())
			continue;

		Vector3& pn = norms[i];
		pn.Normalize();
	}

	queueUpdate[UpdateType::Normals] = true;
	CalcTangentSpace();
}

void Mesh::ColorFill(const Vector3& vcolor) {
	if (!vcolors)
		return;

	for (int i = 0; i < nVerts; i++)
		vcolors[i] = vcolor;

	queueUpdate[UpdateType::VertexColors] = true;
}

void Mesh::AlphaFill(float alpha) {
	if (!valpha)
		return;

	for (int i = 0; i < nVerts; i++)
		valpha[i] = alpha;

	queueUpdate[UpdateType::VertexAlpha] = true;
}

void Mesh::MaskFill(float maskValue) {
	if (!mask)
		return;

	for (int i = 0; i < nVerts; i++)
		mask[i] = maskValue;

	queueUpdate[UpdateType::Mask] = true;
}

void Mesh::WeightFill(float weightValue) {
	if (!weight)
		return;

	for (int i = 0; i < nVerts; i++)
		weight[i] = weightValue;

	queueUpdate[UpdateType::Weight] = true;
}

void Mesh::ColorChannelFill(int channel, float value) {
	if (!vcolors)
		return;

	for (int i = 0; i < nVerts; i++) {
		if (channel == 0)
			vcolors[i].x = value;
		else if (channel == 1)
			vcolors[i].y = value;
		else if (channel == 2)
			vcolors[i].z = value;
	}

	queueUpdate[UpdateType::VertexColors] = true;
}

void Mesh::CalcTangentSpace() {
	if (!norms || !texcoord)
		return;

	for (int i = 0; i < nTris; i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = verts[i1];
		Vector3 v2 = verts[i2];
		Vector3 v3 = verts[i3];

		Vector2 w1 = texcoord[i1];
		Vector2 w2 = texcoord[i2];
		Vector2 w3 = texcoord[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tangents[i1] += tdir;
		tangents[i2] += tdir;
		tangents[i3] += tdir;

		bitangents[i1] += sdir;
		bitangents[i2] += sdir;
		bitangents[i3] += sdir;
	}

	for (int i = 0; i < nVerts; i++) {
		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = norms[i].y;
			tangents[i].y = norms[i].z;
			tangents[i].z = norms[i].x;
			bitangents[i] = norms[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - norms[i] * norms[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - norms[i] * norms[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
	}

	queueUpdate[UpdateType::Tangents] = true;
	queueUpdate[UpdateType::Bitangents] = true;
}

void Mesh::ConnectedPointsInSphere(const Vector3& center, float sqradius, int startTri, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints) {
	// Add the triangle's three vertices to the queue, if they're within the
	// radius.
	for (int tvi = 0; tvi < 3; ++tvi) {
		auto p = tris[startTri][tvi];
		if (!pointvisit[p] && verts[p].DistanceSquaredTo(center) <= sqradius) {
			outPoints[nOutPoints++] = p;
			pointvisit[p] = true;
		}
	}

	// Loop through points in the queue
	for (int adjCursor = 0; adjCursor < nOutPoints; ++adjCursor) {
		int p = outPoints[adjCursor];

		// Add welds of p to the queue
		auto wvit = weldVerts.find(p);
		if (wvit != weldVerts.end())
			for (int wvi : wvit->second)
				if (!pointvisit[wvi]) {
					pointvisit[wvi] = true;
					outPoints[nOutPoints++] = wvi;
				}

		// Add adjacent points of p to the queue, if they're within the radius
		for (int ei : vertEdges[p]) {
			int op = edges[ei].p1 == p ? edges[ei].p2 : edges[ei].p1;
			if (!pointvisit[op] && verts[op].DistanceSquaredTo(center) <= sqradius) {
				pointvisit[op] = true;
				outPoints[nOutPoints++] = op;
			}
		}
	}
}

void Mesh::ConnectedPointsInTwoSpheres(const Vector3& center1, const Vector3& center2, float sqradius, int startTri1, int startTri2, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints) {
	// Add each triangle's three vertices to the queue, if they're within the
	// radius.
	for (int triInd = 0; triInd < 2; ++triInd) {
		int startTri = triInd ? startTri2 : startTri1;
		for (int tvi = 0; tvi < 3; ++tvi) {
			auto p = tris[startTri][tvi];
			if (!pointvisit[p] &&
				(verts[p].DistanceSquaredTo(center1) <= sqradius ||
				verts[p].DistanceSquaredTo(center2) <= sqradius)) {
				outPoints[nOutPoints++] = p;
				pointvisit[p] = true;
			}
		}
	}

	// Loop through points in the queue
	for (int adjCursor = 0; adjCursor < nOutPoints; ++adjCursor) {
		int p = outPoints[adjCursor];

		// Add welds of p to the queue
		auto wvit = weldVerts.find(p);
		if (wvit != weldVerts.end())
			for (int wvi : wvit->second)
				if (!pointvisit[wvi]) {
					pointvisit[wvi] = true;
					outPoints[nOutPoints++] = wvi;
				}

		// Add adjacent points of p to the queue, if they're within the radius
		for (int ei : vertEdges[p]) {
			int op = edges[ei].p1 == p ? edges[ei].p2 : edges[ei].p1;
			if (!pointvisit[op] &&
				(verts[op].DistanceSquaredTo(center1) <= sqradius ||
				verts[op].DistanceSquaredTo(center2) <= sqradius)) {
				pointvisit[op] = true;
				outPoints[nOutPoints++] = op;
			}
		}
	}
}
