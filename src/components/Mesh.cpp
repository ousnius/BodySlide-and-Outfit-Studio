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

void Mesh::GetAdjacentPoints(int querypoint, std::set<int>& outPoints) {
	int tp1 = 0;
	int tp2 = 0;
	int tp3 = 0;
	int wq = 0;

	if (!vertTris)
		return;

	auto vtris = vertTris.get();
	auto addWeldVerts = [&](int qp) {
		if (weldVerts.find(qp) != weldVerts.end()) {
			for (size_t v = 0; v < weldVerts[qp].size(); v++) {
				wq = weldVerts[qp][v];
				outPoints.insert(wq);
			}
		}
	};

	addWeldVerts(querypoint);

	for (size_t t = 0; t < vtris[querypoint].size(); t++) {
		tp1 = tris[vtris[querypoint][t]].p1;
		tp2 = tris[vtris[querypoint][t]].p2;
		tp3 = tris[vtris[querypoint][t]].p3;

		if (tp1 != querypoint) {
			outPoints.insert(tp1);
			addWeldVerts(tp1);
		}

		if (tp2 != querypoint) {
			outPoints.insert(tp2);
			addWeldVerts(tp2);
		}

		if (tp3 != querypoint) {
			outPoints.insert(tp3);
			addWeldVerts(tp3);
		}
	}
}

int Mesh::GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints) {
	int ep1;
	int ep2;
	int wq;
	int n = 0;

	if (!vertEdges)
		return 0;

	auto vedges = vertEdges.get();
	for (uint32_t e = 0; e < vedges[querypoint].size(); e++) {
		ep1 = edges[vedges[querypoint][e]].p1;
		ep2 = edges[vedges[querypoint][e]].p2;
		if (n + 1 < maxPoints) {
			if (ep1 != querypoint)
				outPoints[n++] = ep1;
			else
				outPoints[n++] = ep2;
		}
	}
	if (weldVerts.find(querypoint) != weldVerts.end()) {
		for (uint32_t v = 0; v < weldVerts[querypoint].size(); v++) {
			wq = weldVerts[querypoint][v];
			for (uint32_t e = 0; e < vedges[wq].size(); e++) {
				ep1 = edges[vedges[wq][e]].p1;
				ep2 = edges[vedges[wq][e]].p2;
				if (n + 1 < maxPoints) {
					if (ep1 != wq)
						outPoints[n++] = ep1;
					else
						outPoints[n++] = ep2;
				}
			}
		}
	}
	return n;
	/* TODO: sort by distance */
}

int Mesh::GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint) {
	if (!vertEdges)
		return 0;

	int ep1;
	int ep2;
	int n = 0;

	auto vedges = vertEdges.get();
	for (uint32_t e = 0; e < vedges[querypoint].size(); e++) {
		ep1 = edges[vedges[querypoint][e]].p1;
		ep2 = edges[vedges[querypoint][e]].p2;
		if (n + 1 < maxPoints) {
			if (ep1 != querypoint && !visPoint[ep1]) {
				outPoints[n++] = ep1;
				visPoint[ep1] = true;
			}
			else if (!visPoint[ep2]) {
				outPoints[n++] = ep2;
				visPoint[ep2] = true;
			}
		}
	}
	if (weldVerts.find(querypoint) != weldVerts.end()) {
		for (uint32_t v = 0; v < weldVerts[querypoint].size(); v++) {
			int wq = weldVerts[querypoint][v];
			for (uint32_t e = 0; e < vedges[wq].size(); e++) {
				ep1 = edges[vedges[wq][e]].p1;
				ep2 = edges[vedges[wq][e]].p2;
				if (n + 1 < maxPoints) {
					if (ep1 != wq && !visPoint[ep1]) {
						outPoints[n++] = ep1;
						visPoint[ep1] = true;
					}
					else if (!visPoint[ep2]) {
						outPoints[n++] = ep2;
						visPoint[ep2] = true;
					}
				}
			}
		}
	}
	return n;
	/* TODO: sort by distance */
}

void Mesh::CalcWeldVerts() {
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

void Mesh::SmoothNormals(const std::set<int>& vertices) {
	if (lockNormals || !norms)
		return;

	// Zero old normals
	bool noVertices = vertices.empty();
	for (int i = 0; i < nVerts; i++) {
		if (!noVertices && vertices.count(i) == 0)
			continue;

		if (lockedNormalIndices.count(i) != 0)
			continue;

		Vector3& pn = norms[i];
		pn.Zero();
	}

	// Face normals
	Vector3 tn;
	for (int t = 0; t < nTris; t++) {
		Triangle& tri = tris[t];
		bool bn1 = (noVertices || vertices.count(tri.p1) != 0);
		bool bn2 = (noVertices || vertices.count(tri.p2) != 0);
		bool bn3 = (noVertices || vertices.count(tri.p3) != 0);

		// None of the three normals should change
		if (!bn1 && !bn2 && !bn3)
			continue;

		tri.trinormal(verts.get(), &tn);

		if (bn1 && lockedNormalIndices.count(tri.p1) == 0) {
			Vector3& pn1 = norms[tri.p1];
			pn1 += tn;
		}

		if (bn2 && lockedNormalIndices.count(tri.p2) == 0) {
			Vector3& pn2 = norms[tri.p2];
			pn2 += tn;
		}

		if (bn3 && lockedNormalIndices.count(tri.p3) == 0) {
			Vector3& pn3 = norms[tri.p3];
			pn3 += tn;
		}
	}

	for (int i = 0; i < nVerts; i++) {
		if (lockedNormalIndices.count(i) != 0)
			continue;

		Vector3& pn = norms[i];
		pn.Normalize();
	}

	// Smooth welded vertex normals
	if (smoothSeamNormals) {
		if (!bGotWeldVerts)
			CalcWeldVerts();

		std::vector<std::pair<int, Vector3>> seamNorms;

		for (auto& wvp : weldVerts) {
			auto& key = wvp.first;
			if (!noVertices && vertices.count(key) != 0)
				continue;

			if (lockedNormalIndices.count(key) != 0)
				continue;

			const Vector3& n = norms[key];
			Vector3 sn = n;
			auto& value = wvp.second;
			for (int wvi : value)
				if (n.angle(norms[wvi]) < smoothThresh)
					sn += norms[wvi];

			sn.Normalize();
			seamNorms.emplace_back(key, sn);
		}

		for (auto& snp : seamNorms)
			norms[snp.first] = snp.second;
	}

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
		Vector3 tn;
		t.trinormal(verts.get(), &tn);
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
				Vector3 tn;
				t.trinormal(verts.get(), &tn);
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

	Vector3 tn;
	for (int t = 0; t < nTris; t++) {
		auto& tri = tris[t];
		if (tri.p1 >= nVerts || tri.p2 >= nVerts || tri.p3 >= nVerts)
			continue;

		tri.trinormal(verts.get(), &tn);

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

bool Mesh::ConnectedPointsInSphere(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, std::vector<int>& outFacets) {
	if (!vertTris)
		return false;
	if (!vertEdges)
		return false;
	if (startTri < 0)
		return false;

	outFacets.push_back(startTri);

	if (trivisit)
		trivisit[startTri] = true;

	if (verts[tris[startTri].p1].DistanceSquaredTo(center) <= sqradius) {
		outPoints[nOutPoints++] = tris[startTri].p1;
		pointvisit[tris[startTri].p1] = true;
		auto wv = weldVerts.find(tris[startTri].p1);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				if (pointvisit[w])
					continue;
				outPoints[nOutPoints++] = w;
				pointvisit[w] = true;
			}
		}
	}

	if (verts[tris[startTri].p2].DistanceSquaredTo(center) <= sqradius) {
		outPoints[nOutPoints++] = tris[startTri].p2;
		pointvisit[tris[startTri].p2] = true;
		auto wv = weldVerts.find(tris[startTri].p2);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				if (pointvisit[w])
					continue;
				outPoints[nOutPoints++] = w;
				pointvisit[w] = true;
			}
		}
	}

	if (verts[tris[startTri].p3].DistanceSquaredTo(center) <= sqradius) {
		outPoints[nOutPoints++] = tris[startTri].p3;
		pointvisit[tris[startTri].p3] = true;
		auto wv = weldVerts.find(tris[startTri].p3);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				if (pointvisit[w])
					continue;
				outPoints[nOutPoints++] = w;
				pointvisit[w] = true;
			}
		}
	}

	int adjCursor = 0;
	while (adjCursor < nOutPoints) {
		int pointBuf[100];
		int tp = outPoints[adjCursor++];
		int n = GetAdjacentPoints(tp, pointBuf, 100);
		for (int i = 0; i < n; i++) {
			if (!pointvisit[pointBuf[i]] && verts[pointBuf[i]].DistanceSquaredTo(center) <= sqradius) {
				pointvisit[pointBuf[i]] = true;
				outPoints[nOutPoints++] = pointBuf[i];
			}
		}
	}
	return true;
}

bool Mesh::ConnectedPointsInSphere2(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, std::vector<int>& outFacets) {
	if (!vertTris)
		return false;
	if (startTri < 0)
		return false;

	outFacets.push_back(startTri);
	trivisit[startTri] = true;

	auto vtris = vertTris.get();
	if (verts[tris[startTri].p1].DistanceSquaredTo(center) <= sqradius) {
		if (!pointvisit[tris[startTri].p1]) {
			pointvisit[tris[startTri].p1] = true;
			outPoints[nOutPoints++] = tris[startTri].p1;
		}
		for (auto& t : vtris[tris[startTri].p1]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p1);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				for (auto& t : vtris[w]) {
					if (trivisit[t])
						continue;
					ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
				}
			}
		}
	}

	if (verts[tris[startTri].p2].DistanceSquaredTo(center) <= sqradius) {
		if (!pointvisit[tris[startTri].p2]) {
			pointvisit[tris[startTri].p2] = true;
			outPoints[nOutPoints++] = tris[startTri].p2;
		}
		for (auto& t : vtris[tris[startTri].p2]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p2);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				for (auto& t : vtris[w]) {
					if (trivisit[t])
						continue;
					ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
				}
			}
		}
	}
	if (verts[tris[startTri].p3].DistanceSquaredTo(center) <= sqradius) {
		if (!pointvisit[tris[startTri].p3]) {
			pointvisit[tris[startTri].p3] = true;
			outPoints[nOutPoints++] = tris[startTri].p3;
		}
		for (auto& t : vtris[tris[startTri].p3]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p3);
		if (wv != weldVerts.end()) {
			for (auto& w : wv->second) {
				for (auto& t : vtris[w]) {
					if (trivisit[t])
						continue;
					ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
				}
			}
		}
	}
	return true;
}
