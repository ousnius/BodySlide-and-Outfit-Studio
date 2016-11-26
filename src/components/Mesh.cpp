/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "Mesh.h"

mesh::mesh() {
	vbo.resize(4, 0);
	queueUpdate.resize(vbo.size() + 1, false);

	verts = nullptr;
	norms = nullptr;
	vcolors = nullptr;
	texcoord = nullptr;
	tris = nullptr;
	edges = nullptr;
	bvh = nullptr;
	vertTris = nullptr;
	vertEdges = nullptr;
	bVisible = true;
	bShowPoints = false;
	textured = false;
	rendermode = RenderMode::Normal;
	doublesided = false;
	smoothSeamNormals = true;
	smoothThresh = 90.0f * DEG2RAD;
	scale = 1.0f;
}

mesh::~mesh() {
	if (genBuffers) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glDeleteBuffers(vbo.size(), vbo.data());
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}

	if (verts)
		delete[] verts;
	if (norms)
		delete[] norms;
	if (vcolors)
		delete[] vcolors;
	if (texcoord)
		delete[] texcoord;
	if (tris)
		delete[] tris;
	if (vertTris)
		delete[] vertTris;
	if (vertEdges)
		delete[] vertEdges;
	if (edges)
		delete[] edges;

	verts = nullptr;
	norms = nullptr;
	vcolors = nullptr;
	texcoord = nullptr;
	tris = nullptr;
	edges = nullptr;
	vertTris = nullptr;
	vertEdges = nullptr;
}

shared_ptr<AABBTree> mesh::CreateBVH() {
	bvh = make_shared<AABBTree>(verts, tris, nTris, 100, 2);
	return bvh;
}

void mesh::BuildTriAdjacency() {
	if (!tris)
		return;

	vertTris = new vector<int>[nVerts];
	for (int t = 0; t < nTris; t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}
}

void mesh::MakeEdges() {
	if (!tris)
		return;
	if (edges)
		return;

	nEdges = nTris * 3;
	edges = new Edge[nEdges];

	for (int i = 0; i < nEdges; i++) {
		// Find correct points for edge
		ushort* points = &tris[i / 3].p1;
		int pA = i % 3;
		int pB = i % 3 + 1;
		if (pB >= 3)
			pB = 0;

		// Create edge from points
		edges[i] = Edge(min(points[pA], points[pB]), max(points[pA], points[pB]));
	}
}

void mesh::BuildEdgeList() {
	if (!edges)
		MakeEdges();

	vertEdges = new vector<int>[nVerts];
	for (int e = 0; e < nEdges; e++) {
		vertEdges[edges[e].p1].push_back(e);
		vertEdges[edges[e].p2].push_back(e);
	}
}

void mesh::CreateBuffers() {
	if (!genBuffers) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(vbo.size(), vbo.data());
		glGenBuffers(1, &ibo);
	}

	// NumVertices * (Position + Normal + Colors + Texture Coordinates)
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), verts, GL_DYNAMIC_DRAW);

	if (norms) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), norms, GL_DYNAMIC_DRAW);
	}

	if (vcolors) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), vcolors, GL_DYNAMIC_DRAW);
	}

	if (texcoord) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element index array
	if (tris) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTris * sizeof(Triangle), tris, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else if (edges) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nEdges * sizeof(Edge), edges, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
	genBuffers = true;
}

void mesh::UpdateBuffers() {
	if (genBuffers) {
		glBindVertexArray(vao);

		if (queueUpdate[UpdateType::Position]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Position]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), verts);
			queueUpdate[UpdateType::Position] = false;
		}

		if (norms && queueUpdate[UpdateType::Normals]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Normals]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), norms);
			queueUpdate[UpdateType::Normals] = false;
		}

		if (vcolors && queueUpdate[UpdateType::VertexColors]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::VertexColors]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), vcolors);
			queueUpdate[UpdateType::VertexColors] = false;
		}

		if (texcoord && queueUpdate[UpdateType::TextureCoordinates]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::TextureCoordinates]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector2), texcoord);
			queueUpdate[UpdateType::TextureCoordinates] = false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (queueUpdate[UpdateType::Indices]) {
			if (tris) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nTris * sizeof(Triangle), tris);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else if (edges) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nEdges * sizeof(Edge), edges);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			queueUpdate[UpdateType::Indices] = false;
		}

		glBindVertexArray(0);
	}
}

void mesh::QueueUpdate(const UpdateType& type) {
	queueUpdate[type] = true;
}

void mesh::ScaleVertices(const Vector3& center, const float& factor) {
	for (int i = 0; i < nVerts; i++)
		verts[i] = center + (verts[i] - center) * factor;

	CreateBVH();
	queueUpdate[UpdateType::Position] = true;
}

void mesh::GetAdjacentPoints(int querypoint, set<int>& outPoints) {
	int tp1;
	int tp2;
	int tp3;
	int wq;
	if (!vertTris)
		return;

	for (uint t = 0; t < vertTris[querypoint].size(); t++) {
		tp1 = tris[vertTris[querypoint][t]].p1;
		tp2 = tris[vertTris[querypoint][t]].p2;
		tp3 = tris[vertTris[querypoint][t]].p3;
		if (tp1 != querypoint)
			outPoints.insert(tp1);
		if (tp2 != querypoint)
			outPoints.insert(tp2);
		if (tp3 != querypoint)
			outPoints.insert(tp3);		
	}
	if (weldVerts.find(querypoint) != weldVerts.end()) {
		for (uint v = 0; v < weldVerts[querypoint].size(); v++) {
			wq = weldVerts[querypoint][v];
			for (uint t = 0; t < vertTris[wq].size(); t++) {
				tp1 = tris[vertTris[wq][t]].p1;
				tp2 = tris[vertTris[wq][t]].p2;
				tp3 = tris[vertTris[wq][t]].p3;
				if (tp1 != wq)
					outPoints.insert(tp1);
				if (tp2 != wq)
					outPoints.insert(tp2);
				if (tp3 != wq)
					outPoints.insert(tp3);		
			}
		}
	}
	/* TODO: sort by distance */
}

int mesh::GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints) {
	int ep1;
	int ep2;
	int wq;
	int n = 0;

	if (!vertEdges)
		return 0;

	for (uint e = 0; e < vertEdges[querypoint].size(); e++) {
		ep1 = edges[vertEdges[querypoint][e]].p1;
		ep2 = edges[vertEdges[querypoint][e]].p2;
		if (n + 1 < maxPoints) {
			if (ep1 != querypoint)
				outPoints[n++] = ep1;
			else
				outPoints[n++] = ep2;
		}
	}
	if (weldVerts.find(querypoint) != weldVerts.end()) {
		for (uint v = 0; v < weldVerts[querypoint].size(); v++) {
			wq = weldVerts[querypoint][v];
			for (uint e = 0; e < vertEdges[wq].size(); e++) {
				ep1 = edges[vertEdges[wq][e]].p1;
				ep2 = edges[vertEdges[wq][e]].p2;
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

int mesh::GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint) {
	if (!vertEdges)
		return 0;

	int ep1;
	int ep2;
	int n = 0;
	for (uint e = 0; e < vertEdges[querypoint].size(); e++) {
		ep1 = edges[vertEdges[querypoint][e]].p1;
		ep2 = edges[vertEdges[querypoint][e]].p2;
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
		for (uint v = 0; v < weldVerts[querypoint].size(); v++) {
			int wq = weldVerts[querypoint][v];
			for (uint e = 0; e < vertEdges[wq].size(); e++) {
				ep1 = edges[vertEdges[wq][e]].p1;
				ep2 = edges[vertEdges[wq][e]].p2;
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

float mesh::GetSmoothThreshold() {
	return (smoothThresh * 180) / PI;
}

void mesh::SetSmoothThreshold(float degrees) {
	smoothThresh = degrees * DEG2RAD;
}

void mesh::SmoothNormals(const set<int>& vertices) {
	// Zero old normals
	for (int i = 0; i < nVerts; i++) {
		if (!vertices.empty() && vertices.find(i) == vertices.end())
			continue;

		Vector3& pn = norms[i];
		pn.Zero();
	}

	// Face normals
	Vector3 tn;
	for (int t = 0; t < nTris; t++) {
		bool bn1 = (vertices.empty() || vertices.find(tris[t].p1) != vertices.end());
		bool bn2 = (vertices.empty() || vertices.find(tris[t].p2) != vertices.end());
		bool bn3 = (vertices.empty() || vertices.find(tris[t].p3) != vertices.end());

		// None of the three normals should change
		if (!bn1 && !bn2 && !bn3)
			continue;

		tris[t].trinormal(verts, &tn);
		Vector3& pn1 = norms[tris[t].p1];
		Vector3& pn2 = norms[tris[t].p2];
		Vector3& pn3 = norms[tris[t].p3];

		if (bn1)
			pn1 += tn;
		if (bn2)
			pn2 += tn;
		if (bn3)
			pn3 += tn;
	}

	for (int i = 0; i < nVerts; i++) {
		Vector3& pn = norms[i];
		pn.Normalize();
	}

	// Smooth normals
	if (smoothSeamNormals) {
		kd_matcher matcher(verts, nVerts);
		for (int i = 0; i < matcher.matches.size(); i++) {
			pair<Vector3*, int>& a = matcher.matches[i].first;
			pair<Vector3*, int>& b = matcher.matches[i].second;

			if (!vertices.empty()) {
				if (vertices.find(a.second) == vertices.end() ||
					vertices.find(b.second) == vertices.end())
					continue;
			}

			Vector3& an = norms[a.second];
			Vector3& bn = norms[b.second];
			if (an.angle(bn) < smoothThresh) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (int i = 0; i < nVerts; i++) {
			Vector3& pn = norms[i];
			pn.Normalize();
		}
	}

	queueUpdate[UpdateType::Normals] = true;
}

void mesh::FacetNormals() {
	// Zero old normals
	for (int i = 0; i < nVerts; i++) {
		Vector3& pn = norms[i];
		pn.Zero();
	}

	Vector3 tn;
	for (int t = 0; t < nTris; t++) {
		tris[t].trinormal(verts, &tn);
		Vector3& pn1 = norms[tris[t].p1];
		Vector3& pn2 = norms[tris[t].p2];
		Vector3& pn3 = norms[tris[t].p3];
		pn1 += tn;
		pn2 += tn;
		pn3 += tn;
	}

	for (int i = 0; i < nVerts; i++) {
		Vector3& pn = norms[i];
		pn.Normalize();
	}

	queueUpdate[UpdateType::Normals] = true;
}

void mesh::ColorFill(const Vector3& color) {
	for (int i = 0; i < nVerts; i++)
		vcolors[i] = color;

	queueUpdate[UpdateType::VertexColors] = true;
}

void mesh::ColorChannelFill(int channel, float value) {
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

bool mesh::ConnectedPointsInSphere(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets) {
	if (!vertTris)
		return false;
	if (!vertEdges)
		return false;
	if (startTri < 0)
		return false;

	outFacets.push_back(startTri);
	trivisit[startTri] = true;

	if (verts[tris[startTri].p1].DistanceSquaredTo(center) <= sqradius) {
		outPoints[nOutPoints++] = tris[startTri].p1;
		pointvisit[tris[startTri].p1] = true;
		auto wv = weldVerts.find(tris[startTri].p1);
		if (wv != weldVerts.end()) {
			for (auto &w : wv->second) {
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
			for (auto &w : wv->second) {
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
			for (auto &w : wv->second) {
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

bool mesh::ConnectedPointsInSphere2(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets) {
	if (!vertTris)
		return false;
	if (startTri < 0)
		return false;

	outFacets.push_back(startTri);
	trivisit[startTri] = true;

	if (verts[tris[startTri].p1].DistanceSquaredTo(center) <= sqradius) {
		if (!pointvisit[tris[startTri].p1]) {
			pointvisit[tris[startTri].p1] = true;
			outPoints[nOutPoints++] = tris[startTri].p1;
		}
		for (auto &t : vertTris[tris[startTri].p1]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p1);
		if (wv != weldVerts.end()) {
			for (auto &w : wv->second) {
				for (auto &t : vertTris[w]) {
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
		for (auto &t : vertTris[tris[startTri].p2]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p2);
		if (wv != weldVerts.end()) {
			for (auto &w : wv->second) {
				for (auto &t : vertTris[w]) {
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
		for (auto &t : vertTris[tris[startTri].p3]) {
			if (trivisit[t])
				continue;
			ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
		}
		auto wv = weldVerts.find(tris[startTri].p3);
		if (wv != weldVerts.end()) {
			for (auto &w : wv->second) {
				for (auto &t : vertTris[w]) {
					if (trivisit[t])
						continue;
					ConnectedPointsInSphere(center, sqradius, t, trivisit, pointvisit, outPoints, nOutPoints, outFacets);
				}
			}
		}
	}
	return true;
}
