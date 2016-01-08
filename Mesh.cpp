/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "Mesh.h"

mesh::mesh() {
	verts = nullptr;
	vcolors = nullptr;
	tris = nullptr;
	edges = nullptr;
	bvh = nullptr;
	kdtree = nullptr;
	texcoord = nullptr;
	vertTris = nullptr;
	vertEdges = nullptr;
	bVisible = true;
	textured = false;
	bBuffersLoaded = false;
	rendermode = RenderMode::Normal;
	doublesided = false;
	smoothSeamNormals = true;
	smoothThresh = 90.0f * DEG2RAD;
	scale = 1.0f;
}

mesh::mesh(mesh* m) {
	verts = nullptr;
	tris = nullptr;
	edges = nullptr;
	vcolors = nullptr;
	vertTris = nullptr;
	vertEdges = nullptr;
	if (m->verts) {
		nVerts = m->nVerts;
		verts = new Vertex[nVerts];
		for (int i = 0; i < nVerts; i++) {
			verts[i] = m->verts[i];
		}
	}
	if (m->tris) {
		nTris = m->nTris;
		tris = new Triangle[nTris];
		for (int i = 0; i < nTris; i++) {
			tris[i] = m->tris[i];
		}
	}
	if (m->edges) {
		nEdges = m->nEdges;
		edges = new Edge[nEdges];
		for (int i = 0; i < nEdges; i++) {
			edges[i] = m->edges[i];
		}
	}
	if (m->vcolors) {
		vcolors = new Vector3[nVerts];
		for (int i = 0; i < nVerts; i++) {
			vcolors[i] = m->vcolors[i];
		}
	}
	if (m->vertTris) {
		BuildTriAdjacency();
	}
	if (m->vertEdges) {
		BuildEdgeList();
	}

	bVisible = true;
	bBuffersLoaded = false;
	smoothSeamNormals = m->smoothSeamNormals;
	smoothThresh = m->smoothThresh;
	rendermode = m->rendermode;
	doublesided = m->doublesided;
	this->color = m->color;
	this->shapeName = m->shapeName;
	scale = m->scale;

	if (m->bvh)
		CreateBVH();
	if (m->kdtree)
		CreateKDTree();
}

mesh::~mesh() {
	if (verts)
		delete[] verts;
	if (tris)
		delete[] tris;
	if (vertTris)
		delete[] vertTris;
	if (vertEdges)
		delete[] vertEdges;
	if (edges)
		delete[] edges;
	if (texcoord)
		delete[] texcoord;
	if (vcolors)
		delete[] vcolors;
	if (kdtree)
		delete kdtree;

	verts = 0;
	tris = 0;
	edges = 0;
}

shared_ptr<AABBTree> mesh::CreateBVH() {
	bvh = make_shared<AABBTree>(verts, tris, nTris, 100, 2); // new AABBTree(verts, tris, nTris, 100, 2);
	return bvh;
}

void mesh::CreateKDTree() {
	if (kdtree)
		delete kdtree;

	kdtree = new kd_tree(this->verts, this->nVerts);
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

	unordered_set<Edge> eset;
	for (int t = 0; t < nTris; t++) {
		eset.emplace(min(tris[t].p1, tris[t].p2), max(tris[t].p1, tris[t].p2));
		eset.emplace(min(tris[t].p2, tris[t].p3), max(tris[t].p2, tris[t].p3));
		eset.emplace(min(tris[t].p3, tris[t].p1), max(tris[t].p3, tris[t].p1));
	}
	nEdges = eset.size();
	edges = new Edge[nEdges];

	int i = 0;
	for (auto &e : eset)
		edges[i++] = e;
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

void mesh::GetAdjacentPoints(int querypoint, set<int>& outPoints, bool sort) {
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

void mesh::SmoothNormals() {
	if (!vertTris)
		return;

	Vector3 norm;
	Vector3 tn;
	for (int v = 0; v < nVerts; v++) {
		norm.x = norm.y = norm.z = 0.0f;

		for (auto &t : vertTris[v]) {
			tris[t].trinormal(verts, &tn);
			norm += tn;
		}

		if (weldVerts.find(v) == weldVerts.end()) {
			bool first = true;
			for (auto &t : vertTris[v]) {
				tris[t].trinormal(verts, &tn);

				if (!first) {
					float angle = fabs(norm.angle(tn));
					if (angle > smoothThresh)
						continue;
				}
				else
					first = false;

				norm += tn;
			}
			norm = norm / (float)vertTris[v].size();
		}
		else if (smoothSeamNormals) {
			for (auto &wv : weldVerts[v]) {
				bool first = true;
				if (vertTris[wv].size() < 2)
					continue;

				for (auto &t : vertTris[wv]) {
					tris[t].trinormal(verts, &tn);
					if (!first) {
						float angle = fabs(norm.angle(tn));
						if (angle > smoothThresh)
							continue;
					}
					else
						first = false;

					norm += tn;
				}
			}
			norm = norm / (float)vertTris[v].size();
		}

		norm.Normalize();
		verts[v].nx = norm.x;
		verts[v].ny = norm.y;
		verts[v].nz = norm.z;
	}
}

void mesh::FacetNormals() {
	Vector3 norm;
	Vector3 tn;
	for (int t = 0; t < nTris; t++) {
		norm.x = norm.y = norm.z = 0.0f;
		tris[t].trinormal(verts, &tn);
		tn.Normalize();
		verts[tris[t].p1].nx = tn.x;
		verts[tris[t].p1].ny = tn.y;
		verts[tris[t].p1].nz = tn.z;
		verts[tris[t].p2].nx = tn.x;
		verts[tris[t].p2].ny = tn.y;
		verts[tris[t].p2].nz = tn.z;
		verts[tris[t].p3].nx = tn.x;
		verts[tris[t].p3].ny = tn.y;
		verts[tris[t].p3].nz = tn.z;
	}
}

void mesh::ColorFill(const Vector3& color) {
	if (!vcolors)
		vcolors = new Vector3[nVerts];

	for (int i = 0; i < nVerts; i++)
		vcolors[i] = color;
}

void mesh::ColorChannelFill(int channel, float value) {
	if (!vcolors)
		vcolors = new Vector3[nVerts];

	for (int i = 0; i < nVerts; i++) {
		if (channel == 0)
			vcolors[i].x = value;
		else if (channel == 1)
			vcolors[i].y = value;
		else if (channel == 2)
			vcolors[i].z = value;
	}
}

bool mesh::ConnectedPointsInSphere(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets) {
	if (!vertTris)
		return false;
	if (!vertEdges)
		return false;
	if (startTri < 0)
		return false;

	Vertex v;
	v.x = center.x;
	v.y = center.y;
	v.z = center.z;

	outFacets.push_back(startTri);
	trivisit[startTri] = true;

	if (verts[tris[startTri].p1].DistanceSquaredTo(&v) <= sqradius) {
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

	if (verts[tris[startTri].p2].DistanceSquaredTo(&v) <= sqradius) {
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

	if (verts[tris[startTri].p3].DistanceSquaredTo(&v) <= sqradius) {
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
			if (!pointvisit[pointBuf[i]] && verts[pointBuf[i]].DistanceSquaredTo(&v) <= sqradius) {
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

	Vertex v;
	v.x = center.x;
	v.y = center.y;
	v.z = center.z;

	outFacets.push_back(startTri);
	trivisit[startTri] = true;

	if (verts[tris[startTri].p1].DistanceSquaredTo(&v) <= sqradius) {
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

	if (verts[tris[startTri].p2].DistanceSquaredTo(&v) <= sqradius) {
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
	if (verts[tris[startTri].p3].DistanceSquaredTo(&v) <= sqradius) {
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
