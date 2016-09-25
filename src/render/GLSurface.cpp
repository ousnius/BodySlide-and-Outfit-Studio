/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "GLSurface.h"

#include <wx/msgdlg.h>
#include <wx/log.h>

#include <algorithm>
#include <set>
#include <limits>


Vector3::Vector3(const Vertex& other) {
	x = other.x;
	y = other.y;
	z = other.z;
}

Triangle::Triangle() {
	p1 = p2 = p3 = 0.0f;
}

Triangle::Triangle(ushort P1, ushort P2, ushort P3) {
	p1 = P1;
	p2 = P2;
	p3 = P3;
}

void Triangle::trinormal(Vertex* vertref, Vector3* outNormal) {
	outNormal->x = (vertref[p2].y - vertref[p1].y) * (vertref[p3].z - vertref[p1].z) - (vertref[p2].z - vertref[p1].z) * (vertref[p3].y - vertref[p1].y);
	outNormal->y = (vertref[p2].z - vertref[p1].z) * (vertref[p3].x - vertref[p1].x) - (vertref[p2].x - vertref[p1].x) * (vertref[p3].z - vertref[p1].z);
	outNormal->z = (vertref[p2].x - vertref[p1].x) * (vertref[p3].y - vertref[p1].y) - (vertref[p2].y - vertref[p1].y) * (vertref[p3].x - vertref[p1].x);
}

void Triangle::midpoint(Vertex* vertref, Vector3& outPoint) {
	outPoint = vertref[p1];
	outPoint += vertref[p2];
	outPoint += vertref[p3];
	outPoint /= 3;
}

float Triangle::AxisMidPointY(Vertex* vertref) {
	return (vertref[p1].y + vertref[p2].y + vertref[p3].y) / 3.0f;
}

float Triangle::AxisMidPointX(Vertex* vertref) {
	return (vertref[p1].x + vertref[p2].x + vertref[p3].x) / 3.0f;
}

float Triangle::AxisMidPointZ(Vertex* vertref) {
	return (vertref[p1].z + vertref[p2].z + vertref[p3].z) / 3.0f;
}

bool Triangle::IntersectRay(Vertex* vertref, Vector3& origin, Vector3& direction, float* outDistance, Vector3* worldPos) {
	Vector3 c0(vertref[p1].x, vertref[p1].y, vertref[p1].z);
	Vector3 c1(vertref[p2].x, vertref[p2].y, vertref[p2].z);
	Vector3 c2(vertref[p3].x, vertref[p3].y, vertref[p3].z);

	Vector3 e1 = c1 - c0;
	Vector3 e2 = c2 - c0;
	float u, v;

	Vector3 pvec = direction.cross(e2);
	float det = e1.dot(pvec);

	if (det < 0.000001f)
		return false;

	Vector3 tvec = origin - c0;
	u = tvec.dot(pvec);
	if (u < 0 || u > det) return false;

	Vector3 qvec = tvec.cross(e1);
	v = direction.dot(qvec);
	if (v < 0 || u + v > det) return false;
	float dist = e2.dot(qvec);
	if (dist < 0)
		return false;
	dist *= (1.0f / det);

	if (outDistance) (*outDistance) = dist;
	if (worldPos) (*worldPos) = origin + (direction * dist);

	return true;
}

// Triangle/Sphere collision psuedocode by Christer Ericson: http://realtimecollisiondetection.net/blog/?p=103
//   separating axis test on seven features --  3 points, 3 edges, and the tri plane.  For a sphere, this
//   involves finding the minimum distance to each feature from the sphere origin and comparing it to the sphere radius.
bool Triangle::IntersectSphere(Vertex *vertref, Vector3 &origin, float radius) {
	//A = A - P
	//B = B - P
	//C = C - P

	// Triangle points A,B,C.  translate them so the sphere's origin is their origin
	Vector3 A(vertref[p1].x, vertref[p1].y, vertref[p1].z);
	A = A - origin;
	Vector3 B(vertref[p2].x, vertref[p2].y, vertref[p2].z);
	B = B - origin;
	Vector3 C(vertref[p3].x, vertref[p3].y, vertref[p3].z);
	C = C - origin;

	//rr = r * r
	// Squared radius to avoid sqrts.
	float rr = radius * radius;
	//V = cross(B - A, C - A)

	// first test: tri plane.  Calculate the normal V
	Vector3 AB = B - A;
	Vector3 AC = C - A;
	Vector3 V = AB.cross(AC);
	//d = dot(A, V)
	//e = dot(V, V)
	// optimized distance test of the plane to the sphere -- removing sqrts and divides
	float d = A.dot(V);
	float e = V.dot(V);		// e = squared normal vector length -- the normalization factor
	//sep1 = d * d > rr * e
	if (d*d > rr * e)
		return false;
	//aa = dot(A, A)
	//ab = dot(A, B)
	//ac = dot(A, C)
	//bb = dot(B, B)
	//bc = dot(B, C)
	//cc = dot(C, C)

	// second test: tri points.  A sparating axis exists if a point lies outside the sphere, and the other tri points aren't on the other side of the sphere.
	float aa = A.dot(A);	// dist to point A
	float ab = A.dot(B);
	float ac = A.dot(C);
	float bb = B.dot(B);	// dist to point B
	float bc = B.dot(C);
	float cc = C.dot(C);	// dist to point C
	bool sep2 = (aa > rr) && (ab > aa) && (ac > aa);
	bool sep3 = (bb > rr) && (ab > bb) && (bc > bb);
	bool sep4 = (cc > rr) && (ac > cc) && (bc > cc);

	if (sep2 | sep3 | sep4) {
		return false;
	}

	//AB = B - A
	Vector3 BC = C - B;
	Vector3 CA = A - C;

	float d1 = ab - aa;
	d1 = A.dot(AB);
	float d2 = bc - bb;
	d2 = B.dot(BC);
	float d3 = ac - cc;
	d3 = C.dot(CA);

	//e1 = dot(AB, AB)
	//e2 = dot(BC, BC)
	//e3 = dot(CA, CA)
	float e1 = AB.dot(AB);
	float e2 = BC.dot(BC);
	float e3 = CA.dot(CA);

	//Q1 = A * e1 - d1 * AB
	//Q2 = B * e2 - d2 * BC
	//Q3 = C * e3 - d3 * CA
	//QC = C * e1 - Q1
	//QA = A * e2 - Q2
	//QB = B * e3 - Q3
	Vector3 Q1 = (A * e1) - (AB * d1);
	Vector3 Q2 = (B * e2) - (BC * d2);
	Vector3 Q3 = (C * e3) - (CA * d3);
	Vector3 QC = (C * e1) - Q1;
	Vector3 QA = (A * e2) - Q2;
	Vector3 QB = (B * e3) - Q3;

	//sep5 = [dot(Q1, Q1) > rr * e1 * e1] & [dot(Q1, QC) > 0]
	//sep6 = [dot(Q2, Q2) > rr * e2 * e2] & [dot(Q2, QA) > 0]
	//sep7 = [dot(Q3, Q3) > rr * e3 * e3] & [dot(Q3, QB) > 0]

	bool sep5 = (Q1.dot(Q1) > (rr * e1 * e1)) && (Q1.dot(QC) > 0);
	bool sep6 = (Q2.dot(Q2) > (rr * e2 * e2)) && (Q2.dot(QA) > 0);
	bool sep7 = (Q3.dot(Q3) > (rr * e3 * e3)) && (Q3.dot(QB) > 0);
	//separated = sep1 | sep2 | sep3 | sep4 | sep5 | sep6 | sep7
	if (sep5 | sep6 | sep7)
		return false;
	return true;
}

GLSurface::GLSurface() {
	mFov = 90.0f;
	bTextured = true;
	bWireframe = false;
	bLighting = true;
	bMaskVisible = false;
	bWeightColors = false;
	bSegmentColors = false;
	cursorSize = 0.5f;
}

GLSurface::~GLSurface() {
	Cleanup();
}

const wxGLAttributes& GLSurface::GetGLAttribs() {
	static bool attribsInitialized { false };
	static wxGLAttributes attribs;

	if (!attribsInitialized) {
		// 8x AA
		attribs.PlatformDefaults().RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(16).Level(0).SampleBuffers(1).Samplers(8).EndList();

		bool displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		if (!displaySupported) {
			wxLogWarning("OpenGL attributes not supported. Trying out different ones...");
			attribs.Reset();

			// 4x AA
			attribs.RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(16).Level(0).SampleBuffers(1).Samplers(4).EndList();
			displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		}

		if (!displaySupported) {
			wxLogWarning("OpenGL attributes not supported. Trying out different ones...");
			attribs.Reset();

			// No AA
			attribs.RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(16).Level(0).SampleBuffers(0).EndList();
			displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		}

		if (!displaySupported)
			wxLogWarning("No supported OpenGL attributes could be found!");
		else
			wxLogMessage("OpenGL attributes are supported.");

		attribsInitialized = true;
	}

	return attribs;
}

const wxGLContextAttrs& GLSurface::GetGLContextAttribs() {
	static bool ctxAttribsInitialized{ false };
	static wxGLContextAttrs ctxAttribs;

	if (!ctxAttribsInitialized) {
		ctxAttribs.PlatformDefaults().CoreProfile().OGLVersion(5, 1).EndList();
		ctxAttribsInitialized = true;
	}

	return ctxAttribs;
}

void GLSurface::InitLighting() {
	glEnable(GL_LIGHTING);
	float amb[] = { 0.20f, 0.2288f, 0.0f, 1.0f };
	float diff[] = { 0.55f, 0.55f, 0.55f, 1.0f };
	float spec[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	float lightpos[] = { 1.0f, 0.2f, 0.5f, 0.0f };
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	float lightpos2[] = { -1.0f, -1.0f, 1.0f, 0.0f };
	float diff2[] = { 0.45f, 0.45f, 0.45f, 1.0f };
	float rimspec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diff2);
	glLightfv(GL_LIGHT1, GL_SPECULAR, rimspec);
	glLightfv(GL_LIGHT1, GL_POSITION, lightpos2);

	float lightpos3[] = { -2.0f, 2.0f, 1.0f, 0.0f };
	float diff3[] = { 0.45f, 0.45f, 0.45f, 1.0f };
	glEnable(GL_LIGHT2);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, diff3);
	glLightfv(GL_LIGHT2, GL_SPECULAR, spec);
	glLightfv(GL_LIGHT2, GL_POSITION, lightpos3);
}

void GLSurface::InitMaterial(Vector3 diffuseColor) {
	float spec[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	float amb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float matcolor[4];
	matcolor[0] = diffuseColor.x;
	matcolor[1] = diffuseColor.x;
	matcolor[2] = diffuseColor.x;
	matcolor[3] = 1.0f;
	float shiny[] = { 10.0f };
	glMaterialfv(GL_FRONT, GL_DIFFUSE, matcolor);
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialfv(GL_FRONT, GL_SHININESS, shiny);
}

int GLSurface::Initialize(wxGLCanvas* can, wxGLContext* ctx) {
	canvas = can;
	context = ctx;

	canvas->SetCurrent(*context);
	
	wxLogMessage("OpenGL Context Info:");
	wxLogMessage(wxString::Format("-> Vendor:   '%s'", wxString(glGetString(GL_VENDOR))));
	wxLogMessage(wxString::Format("-> Renderer: '%s'", wxString(glGetString(GL_RENDERER))));
	wxLogMessage(wxString::Format("-> Version:  '%s'", wxString(glGetString(GL_VERSION))));

	InitGLExtensions();
	return InitGLSettings();
}

void GLSurface::InitGLExtensions() {
	InitExtensions();
	if (IsExtensionSupported("GL_EXT_texture_filter_anisotropic"))
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largestAF);
}

int GLSurface::InitGLSettings() {
	glShadeModel(GL_SMOOTH);

	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Offset lines forward to avoid clipping issues
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-0.03f, -0.03f);

	// Get supported line width range
	GLfloat lineRange[2];
	glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, lineRange);

	if (lineRange[0] > 1.0f)
		defLineWidth = lineRange[0];
	else if (lineRange[1] < 1.0f)
		defLineWidth = lineRange[1];
	else
		defLineWidth = 1.0f;

	// Get supported point size range
	GLfloat pointRange[2];
	glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, pointRange);

	if (pointRange[0] > 5.0f)
		defPointSize = pointRange[0];
	else if (pointRange[1] < 5.0f)
		defPointSize = pointRange[1];
	else
		defPointSize = 5.0f;

	glLineWidth(defLineWidth);
	glPointSize(defPointSize);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	InitLighting();
	InitMaterial(Vector3(0.8f, 0.8f, 0.8f));

	return 0;
}

void GLSurface::Cleanup() {
	for (auto &m : meshes)
		delete m;
	
	for (auto &o : overlays)
		delete o;

	meshes.clear();
	overlays.clear();
	namedMeshes.clear();
	namedOverlays.clear();
	activeMeshesID.clear();
	activeMeshes.clear();

	selectedMesh = nullptr;
	noImage = nullptr;

	resLoader.Cleanup();
}

void GLSurface::Begin() {
	canvas->SetCurrent(*context);
}

void GLSurface::SetStartingView(const Vector3& pos, const Vector3& rot, const uint& vpWidth, const uint& vpHeight, const float& fov) {
	perspective = true;
	camPos = pos;
	camRot = rot;
	camOffset.Zero();
	vpW = vpWidth;
	vpH = vpHeight;
	mFov = fov;
	SetSize(vpWidth, vpHeight);
}
	
void GLSurface::TurnTableCamera(int dScreenX) {
	float pct = (float)dScreenX / (float)vpW;
	camRot.y += (pct * 500.0f);
}

void GLSurface::PitchCamera(int dScreenY) {
	float pct = (float)dScreenY / (float)vpH;
	camRot.x += (pct * 500.0f);
	if (camRot.x > 80.0f)
		camRot.x = 80.0f;
	if (camRot.x < -80.0f)
		camRot.x = -80.0f;
}

void GLSurface::PanCamera(int dScreenX, int dScreenY) {
	float fabsZ = fabs(camPos.z);
	if (fabsZ < 0.5f)
		fabsZ = 0.5f;

	camPos.y -= ((float)dScreenY / vpH) * fabsZ;
	camPos.x += ((float)dScreenX / vpW) * fabsZ;
}

void GLSurface::DollyCamera(int dAmt) {
	camPos.z += (float)dAmt / 200.0f;
}

void GLSurface::UnprojectCamera(Vector3& result) {
	GLint vp[4];
	GLdouble proj[16];
	GLdouble model[16];
	double dx, dy, dz;
	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	gluUnProject(vp[2] / 2.0f, vp[3] / 2.0f, 0.0f, model, proj, vp, &dx, &dy, &dz);
	result.x = dx;
	result.y = dy;
	result.z = dz;
}

void GLSurface::SetView(const char& type) {
	if (type == 'F') {
		camPos = Vector3(0.0f, -5.0f, -15.0f);
		camRot = Vector3();
	}
	else if (type == 'B') {
		camPos = Vector3(0.0f, -5.0f, -15.0f);
		camRot = Vector3(0.0f, 180.0f, 0.0f);
	}
	else if (type == 'L') {
		camPos = Vector3(0.0f, -5.0f, -15.0f);
		camRot = Vector3(0.0f, -90.0f, 0.0f);
	}
	else if (type == 'R') {
		camPos = Vector3(0.0f, -5.0f, -15.0f);
		camRot = Vector3(0.0f, 90.0f, 0.0f);
	}
	UpdateProjection();
}

void GLSurface::SetPerspective(const bool& enabled) {
	perspective = enabled;
	UpdateProjection();
}

void GLSurface::SetFieldOfView(const int& fieldOfView) {
	mFov = fieldOfView;
	UpdateProjection();
}

void GLSurface::UpdateLights(const int& ambient, const int& brightness1, const int& brightness2, const int& brightness3) {
	float amb[] = { 0.01f * ambient, 0.00286f * ambient, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);

	float diff1[] = { 0.01f * brightness1, 0.01f * brightness1, 0.01f * brightness1, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff1);

	float diff2[] = { 0.01f * brightness2, 0.01f * brightness2, 0.01f * brightness2, 1.0f };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diff2);

	float diff3[] = { 0.01f * brightness3, 0.01f * brightness3, 0.01f * brightness3, 1.0f };
	glLightfv(GL_LIGHT2, GL_DIFFUSE, diff3);
}

void GLSurface::GetPickRay(int ScreenX, int ScreenY, Vector3& dirVect, Vector3& outNearPos) {
	GLint vp[4];
	GLdouble proj[16];
	GLdouble model[16];
	double sx, sy, sz;
	double dx, dy, dz;
	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetDoublev(GL_MODELVIEW_MATRIX, model);

	gluUnProject(ScreenX, vp[3] - ScreenY, 0.0f, model, proj, vp, &sx, &sy, &sz);
	gluUnProject(ScreenX, vp[3] - ScreenY, 1.0f, model, proj, vp, &dx, &dy, &dz);

	dirVect.x = dx - sx;
	dirVect.y = dy - sy;
	dirVect.z = dz - sz;

	outNearPos.x = sx;
	outNearPos.y = sy;
	outNearPos.z = sz;

	dirVect.Normalize();
}

int GLSurface::PickMesh(int ScreenX, int ScreenY) {
	Vector3 o;
	Vector3 d;
	float curd = FLT_MAX;
	int result = -1;

	GetPickRay(ScreenX, ScreenY, d, o);
	vector<IntersectResult> results;

	for (int i = 0; i < meshes.size(); i++) {
		results.clear();
		if (!meshes[i]->bVisible || !meshes[i]->bvh)
			continue;

		if (meshes[i]->bvh->IntersectRay(o, d, &results)) {
			if (results[0].HitDistance < curd) {
				result = i;
				curd = results[0].HitDistance;
			}
		}
	}

	return result;
}

bool GLSurface::CollideMeshes(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh, bool allMeshes, int* outFacet, Vector3* inRayDir, Vector3* inRayOrigin) {
	if (activeMeshes.empty())
		return false;

	Vector3 o;
	Vector3 d;

	if (!inRayDir) {
		GetPickRay(ScreenX, ScreenY, d, o);
	}
	else {
		d = (*inRayDir);
		o = (*inRayOrigin);
	}

	bool collided = false;
	unordered_map<mesh*, float> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!m->bvh)
			continue;

		if (!allMeshes && m != selectedMesh)
			continue;

		vector<IntersectResult> results;
		if (m->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				collided = true;

				if (!m->bVisible) {
					collided = false;
					continue;
				}

				int min_i = 0;
				float minDist = results[0].HitDistance;
				for (int i = 1; i < results.size(); i++)
					if (results[i].HitDistance < minDist)
						min_i = i;

				Vector3 origin = results[min_i].HitCoord;

				bool closest = true;
				float viewDistance = origin.DistanceTo(o);

				for (auto &p : allHitDistances)
					if (viewDistance > p.second)
						closest = false;

				allHitDistances[m] = viewDistance;

				if (closest) {
					outOrigin = origin;

					if (outFacet)
						(*outFacet) = results[min_i].HitFacet;

					m->tris[results[min_i].HitFacet].trinormal(m->verts, &outNormal);

					if (hitMesh)
						(*hitMesh) = m;

					if (!allMeshes)
						return true;
				}
			}
		}
	}

	return collided;
}

bool GLSurface::CollideOverlay(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh, int* outFacet, Vector3* inRayDir, Vector3* inRayOrigin) {
	Vector3 o;
	Vector3 d;

	if (!inRayDir) {
		GetPickRay(ScreenX, ScreenY, d, o);
	}
	else {
		d = (*inRayDir);
		o = (*inRayOrigin);
	}

	bool collided = false;
	unordered_map<mesh*, float> allHitDistances;
	for (auto &ov : overlays) {
		vector<IntersectResult> results;
		if (!ov->bvh)
			continue;

		if (ov->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				collided = true;

				int min_i = 0;
				float minDist = results[0].HitDistance;
				for (int i = 1; i < results.size(); i++)
					if (results[i].HitDistance < minDist)
						min_i = i;

				Vector3 origin = results[min_i].HitCoord;

				bool closest = true;
				float viewDistance = origin.DistanceTo(o);

				for (auto &p : allHitDistances)
					if (viewDistance > p.second)
						closest = false;

				allHitDistances[ov] = viewDistance;

				if (closest) {
					outOrigin = origin;

					if (hitMesh)
						(*hitMesh) = ov;

					if (outFacet)
						(*outFacet) = results[min_i].HitFacet;

					ov->tris[results[min_i].HitFacet].trinormal(ov->verts, &outNormal);
				}
			}
		}
	}

	return collided;
}

bool GLSurface::CollidePlane(int ScreenX, int ScreenY, Vector3& outOrigin, const Vector3& inPlaneNormal, float inPlaneDist) {
	Vector3 o;
	Vector3 d;
	GetPickRay(ScreenX, ScreenY, d, o);

	float den = inPlaneNormal.dot(d);
	if (fabs(den) < .00001)
		return false;

	float t = -(inPlaneNormal.dot(o) + inPlaneDist) / den;
	outOrigin = o + d*t;

	return true;
}

bool GLSurface::UpdateCursor(int ScreenX, int ScreenY, bool allMeshes, string* hitMeshName, int* outHoverTri, float* outHoverWeight, float* outHoverMask) {
	bool ret = false;
	if (activeMeshes.empty())
		return ret;

	Vector3 o;
	Vector3 d;
	Vector3 v;
	Vector3 vo;

	if (outHoverTri)
		(*outHoverTri) = -1;
	if (outHoverWeight)
		(*outHoverWeight) = 0.0f;
	if (outHoverMask)
		(*outHoverMask) = 0.0f;

	GetPickRay(ScreenX, ScreenY, d, o);

	unordered_map<mesh*, Vector3> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!allMeshes && m != selectedMesh)
			continue;

		vector<IntersectResult> results;
		if (m->bvh->IntersectRay(o, d, &results)) {
			ret = true;
			if (results.size() > 0) {
				if (!m->bVisible)
					continue;

				int min_i = 0;
				float minDist = results[0].HitDistance;
				for (int i = 1; i < results.size(); i++)
					if (results[i].HitDistance < minDist)
						min_i = i;

				Vector3 origin = results[min_i].HitCoord;

				Triangle t;
				t = m->tris[results[min_i].HitFacet];

				Vertex v1 = (m->verts[t.p1]);
				Vertex v2 = (m->verts[t.p2]);
				Vertex v3 = (m->verts[t.p3]);

				Vector3 p1(v1.x, v1.y, v1.z);
				Vector3 p2(v2.x, v2.y, v2.z);
				Vector3 p3(v3.x, v3.y, v3.z);

				Vector3 hilitepoint = p1;
				float closestdist = fabs(p1.DistanceTo(origin));
				float nextdist = fabs(p2.DistanceTo(origin));
				int pointid = t.p1;

				if (nextdist < closestdist) {
					closestdist = nextdist;
					hilitepoint = p2;
					pointid = t.p2;
				}
				nextdist = fabs(p3.DistanceTo(origin));
				if (nextdist < closestdist) {
					hilitepoint = p3;
					pointid = t.p3;
				}

				bool closest = true;
				float viewDistance = hilitepoint.DistanceTo(o);

				for (auto &p : allHitDistances) {
					if (viewDistance > p.second.DistanceTo(o))
						closest = false;
				}

				if (closest) {
					int dec = 5;
					if (outHoverTri)
						(*outHoverTri) = pointid;
					if (outHoverWeight)
						(*outHoverWeight) = floor(m->vcolors[pointid].y * pow(10, dec) + 0.5f) / pow(10, dec);
					if (outHoverMask)
						(*outHoverMask) = floor(m->vcolors[pointid].x * pow(10, dec) + 0.5f) / pow(10, dec);
					if (hitMeshName)
						(*hitMeshName) = m->shapeName;

					Vector3 norm;
					m->tris[results[min_i].HitFacet].trinormal(m->verts, &norm);
					int ringID = AddVisCircle(results[min_i].HitCoord, norm, cursorSize, "cursormesh");
					overlays[ringID]->scale = 2.0f;

					AddVisPoint(hilitepoint, "pointhilite");
					AddVisPoint(origin, "cursorcenter")->color = Vector3(1.0f, 0.0f, 0.0f);
				}

				allHitDistances[m] = hilitepoint;
			}
		}
	}

	ShowCursor(ret);
	return ret;
}

bool GLSurface::GetCursorVertex(int ScreenX, int ScreenY, Vertex* outHoverVtx) {
	if (activeMeshes.empty())
		return false;

	Vector3 o;
	Vector3 d;
	Vector3 v;
	Vector3 vo;

	if (outHoverVtx)
		(*outHoverVtx) = Vertex();

	GetPickRay(ScreenX, ScreenY, d, o);

	for (auto &m : activeMeshes) {
		vector<IntersectResult> results;
		if (m->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				int min_i = 0;
				float minDist = results[0].HitDistance;
				for (int i = 1; i < results.size(); i++)
					if (results[i].HitDistance < minDist)
						min_i = i;

				Vector3 origin = results[min_i].HitCoord;

				Triangle t;
				t = m->tris[results[min_i].HitFacet];

				Vertex v1 = m->verts[t.p1];
				Vertex v2 = m->verts[t.p2];
				Vertex v3 = m->verts[t.p3];

				Vector3 p1(v1.x, v1.y, v1.z);
				Vector3 p2(v2.x, v2.y, v2.z);
				Vector3 p3(v3.x, v3.y, v3.z);

				Vector3 hilitepoint = p1;
				float closestdist = fabs(p1.DistanceTo(origin));
				float nextdist = fabs(p2.DistanceTo(origin));
				int pointid = t.p1;

				if (nextdist < closestdist) {
					closestdist = nextdist;
					hilitepoint = p2;
					pointid = t.p2;
				}

				nextdist = fabs(p3.DistanceTo(origin));

				if (nextdist < closestdist) {
					hilitepoint = p3;
					pointid = t.p3;
				}

				(*outHoverVtx) = m->verts[pointid];
				return true;
			}
		}
	}

	return false;
}

void GLSurface::ShowCursor(bool show) {
	SetOverlayVisibility("cursormesh", show);
	SetOverlayVisibility("pointhilite", show);
	SetOverlayVisibility("cursorcenter", show);
}

void GLSurface::SetSize(uint w, uint h) {
	glViewport(0, 0, w, h);
	vpW = w;
	vpH = h;
	UpdateProjection();
}

void GLSurface::UpdateProjection() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double aspect = (double)vpW / (double)vpH;
	if (perspective)
		gluPerspective(mFov, aspect, 0.1, 100);
	else
		glOrtho((camPos.z + camOffset.z) / 2 * aspect, (-camPos.z + camOffset.z) / 2 * aspect, (camPos.z + camOffset.z) / 2, (-camPos.z + camOffset.z) / 2, 0.1, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(camPos.x, camPos.y, camPos.z);
	glRotatef(camRot.x, 1.0f, 0.0f, 0.0f);
	glRotatef(camRot.y, 0.0f, 1.0f, 0.0f);
	glTranslatef(camOffset.x, camOffset.y, camOffset.z);
}

void GLSurface::RenderOneFrame() {
	if (!canvas)
		return;

	mesh* m;
	Begin();

	glClearColor(0.81f, 0.82f, 0.82f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(camPos.x, camPos.y, camPos.z);
	glRotatef(camRot.x, 1.0f, 0.0f, 0.0f);
	glRotatef(camRot.y, 0.0f, 1.0f, 0.0f);
	glTranslatef(camOffset.x, camOffset.y, camOffset.z);

	for (int i = 0; i < meshes.size(); i++) {
		m = meshes[i];
		if (!m->bVisible || m->nTris == 0)
			continue;

		RenderMesh(m);
	}

	glClear(GL_DEPTH_BUFFER_BIT);
	for (int i = 0; i < overlays.size(); i++) {
		m = overlays[i];
		if (!m->bVisible)
			continue;

		RenderMesh(m);
	}

	canvas->SwapBuffers();
	return;
}

void GLSurface::RenderMesh(mesh* m) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (!m->doublesided)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glColor3fv((GLfloat*)&m->color);
	if (!bLighting)
		glDisable(GL_LIGHTING);
	else
		glEnable(GL_LIGHTING);

	if (m->rendermode == RenderMode::Normal || m->rendermode == RenderMode::LitWire) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m->verts[0].x);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(Vertex), &m->verts[0].nx);

		if (m->material && m->material->GetShader().Begin()) {
			auto maskAttrib = m->material->GetShader().GetMaskAttribute();
			if (maskAttrib != -1) {
				if (m->vcolors && bMaskVisible) {
					glEnableVertexAttribArray(maskAttrib);
					glVertexAttribPointer(maskAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vector3), m->vcolors);
				}
				else
					glDisableVertexAttribArray(maskAttrib);
			}

			auto weightAttrib = m->material->GetShader().GetWeightAttribute();
			if (weightAttrib != -1) {
				if (m->vcolors && (bWeightColors || bSegmentColors)) {
					glEnableVertexAttribArray(weightAttrib);
					glVertexAttribPointer(weightAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vector3), &m->vcolors[0].y);
				}
				else
					glDisableVertexAttribArray(weightAttrib);
			}

			auto segmentAttrib = m->material->GetShader().GetSegmentAttribute();
			if (segmentAttrib != -1) {
				if (m->vcolors && bSegmentColors) {
					glEnableVertexAttribArray(segmentAttrib);
					glVertexAttribPointer(segmentAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Vector3), &m->vcolors[0].z);
				}
				else
					glDisableVertexAttribArray(segmentAttrib);
			}
		}

		if (m->material && m->textured && bTextured)
			m->material->ActivateTextures(m->texcoord, largestAF);

		if (m->rendermode == RenderMode::LitWire) {
			glLineWidth(1.5f);
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		glDrawElements(GL_TRIANGLES, (m->nTris * 3), GL_UNSIGNED_SHORT, &m->tris[0].p1);

		if (m->material) {
			if (m->textured && bTextured)
				m->material->DeactivateTextures();

			m->material->GetShader().End();
		}

		if (bWireframe) {
			glDisable(GL_LIGHTING);
			if (m->color.x >= 0.7f)
				glColor3f(0.3f, 0.3f, 0.3f);
			else
				glColor3f(0.7f, 0.7f, 0.7f);

			glLineWidth(defLineWidth);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, (m->nTris) * 3, GL_UNSIGNED_SHORT, &m->tris[0].p1);
			glEnable(GL_LIGHTING);
		}
	}
	else if (m->rendermode == RenderMode::UnlitSolid) {
		glDisable(GL_CULL_FACE);
		if (bLighting)
			glDisable(GL_LIGHTING);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m->verts[0].x);
		glDrawElements(GL_TRIANGLES, (m->nTris * 3), GL_UNSIGNED_SHORT, &m->tris[0].p1);

		if (bLighting)
			glEnable(GL_LIGHTING);

	}
	else if (m->rendermode == RenderMode::UnlitWire) {
		glDisable(GL_CULL_FACE);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &m->verts[0].x);

		glDisable(GL_DEPTH_TEST);
		if (bLighting)
			glDisable(GL_LIGHTING);

		glLineWidth(m->scale);

		glColor3fv((GLfloat*)&m->color);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_LINES, (m->nEdges) * 2, GL_UNSIGNED_SHORT, &m->edges[0].p1);

		if (bLighting)
			glEnable(GL_LIGHTING);

		glEnable(GL_DEPTH_TEST);
		glLineWidth(defLineWidth);

	}
	else if (m->rendermode == RenderMode::UnlitPoints) {
		glDisable(GL_CULL_FACE);

		glDisable(GL_DEPTH_TEST);
		if (bLighting)
			glDisable(GL_LIGHTING);

		glColor3fv((GLfloat*)&m->color);

		glBegin(GL_POINTS);
		for (int pi = 0; pi < m->nVerts; pi++)
			glVertex3f(m->verts[pi].x, m->verts[pi].y, m->verts[pi].z);
		glEnd();

		if (bLighting)
			glEnable(GL_LIGHTING);

		glEnable(GL_DEPTH_TEST);
	}

	if (m->bShowPoints && m->vcolors) {
		glDisable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);

		glBegin(GL_POINTS);
		for (int pi = 0; pi < m->nVerts; pi++) {
			if (m->vcolors[pi].x > 0.0f)
				glColor3fv((GLfloat*)&colorRed);
			else
				glColor3fv((GLfloat*)&colorGreen);

			glVertex3f(m->verts[pi].x + m->verts[pi].nx / 250.0f,
				m->verts[pi].y + m->verts[pi].ny / 250.0f,
				m->verts[pi].z + m->verts[pi].nz / 250.0f);
		}
		glEnd();
	}
}

void  GLSurface::ReloadMeshFromNif(NifFile* nif, string shapeName) {
	DeleteMesh(shapeName);
	AddMeshFromNif(nif, shapeName);
}

void GLSurface::AddMeshFromNif(NifFile* nif, string shapeName, Vector3* color, bool smoothNormalSeams) {
	vector<Vector3> nifVerts;
	vector<Triangle> nifTris;
	nif->GetVertsForShape(shapeName, nifVerts);
	nif->GetTrisForShape(shapeName, &nifTris);

	const vector<Vector3>* nifNorms = nif->GetNormalsForShape(shapeName, false);
	const vector<Vector2>* nifUvs = nif->GetUvsForShape(shapeName);

	mesh* m = new mesh();

	// Check skin and double sided shader flags
	bool isSkin = false;
	m->doublesided = false;

	NiShader* shader = nif->GetShader(shapeName);
	if (shader) {
		isSkin = shader->IsSkinTint();
		m->doublesided = shader->IsDoubleSided();
	}

	m->shapeName = shapeName;
	m->smoothSeamNormals = smoothNormalSeams;

	m->nVerts = nifVerts.size();
	m->verts = new Vertex[m->nVerts];

	m->nTris = nifTris.size();
	m->tris = new Triangle[m->nTris];

	// Load verts. NIF verts are scaled up by approx. 10 and rotated on the x axis (Z up, Y forward).  
	// Scale down by 10 and rotate on x axis by flipping y and z components. To face the camera, this also mirrors
	// on X and Y (180 degree y axis rotation.)
	for (int i = 0; i < m->nVerts; i++) {
		m->verts[i].x = (nifVerts)[i].x / -10.0f;
		m->verts[i].z = (nifVerts)[i].y / 10.0f;
		m->verts[i].y = (nifVerts)[i].z / 10.0f;
		/*m->verts[i].x = (nifVerts)[i].x / 10.0f;
		m->verts[i].y = (nifVerts)[i].y / 10.0f;
		m->verts[i].z = (nifVerts)[i].z / 10.0f;*/
		m->verts[i].indexRef = i;
	}

	if (nifUvs && !nifUvs->empty()) {
		m->texcoord = new Vector2[m->nVerts];
		for (int i = 0; i < m->nVerts; i++) {
			m->texcoord[i].u = (*nifUvs)[i].u;
			m->texcoord[i].v = (*nifUvs)[i].v;
		}
		m->textured = true;
		m->material = noImage;
	}

	if (!nifNorms || nifNorms->empty()) {
		// Load tris. Also sum face normals here.
		for (int j = 0; j < m->nTris; j++) {
			Vector3 norm;
			m->tris[j].p1 = nifTris[j].p1;
			m->tris[j].p2 = nifTris[j].p2;
			m->tris[j].p3 = nifTris[j].p3;
			m->tris[j].trinormal(m->verts, &norm);
			m->verts[m->tris[j].p1].nx += norm.x;
			m->verts[m->tris[j].p1].ny += norm.y;
			m->verts[m->tris[j].p1].nz += norm.z;
			m->verts[m->tris[j].p2].nx += norm.x;
			m->verts[m->tris[j].p2].ny += norm.y;
			m->verts[m->tris[j].p2].nz += norm.z;
			m->verts[m->tris[j].p3].nx += norm.x;
			m->verts[m->tris[j].p3].ny += norm.y;
			m->verts[m->tris[j].p3].nz += norm.z;
		}

		// Normalize all vertex normals to smooth them out.
		for (int i = 0; i < m->nVerts; i++) {
			Vector3* pn = (Vector3*)&m->verts[i].nx;
			pn->Normalize();
		}

		kd_matcher matcher(m->verts, m->nVerts);
		for (int i = 0; i < matcher.matches.size(); i++) {
			Vertex* a = matcher.matches[i].first;
			Vertex* b = matcher.matches[i].second;
			m->weldVerts[a->indexRef].push_back(b->indexRef);
			m->weldVerts[b->indexRef].push_back(a->indexRef);

			if (smoothNormalSeams) {
				float dot = (a->nx * b->nx + a->ny * b->ny + a->nz * b->nz);
				if (dot < 90.0f * DEG2RAD) {
					a->nx = ((a->nx + b->nx) / 2.0f);
					a->ny = ((a->ny + b->ny) / 2.0f);
					a->nz = ((a->nz + b->nz) / 2.0f);
					b->nx = a->nx;
					b->ny = a->ny;
					b->nz = a->nz;
				}
			}
		}

	}
	else {
		// Already have normals, just copy the data over.
		for (int j = 0; j < m->nTris; j++) {
			m->tris[j].p1 = nifTris[j].p1;
			m->tris[j].p2 = nifTris[j].p2;
			m->tris[j].p3 = nifTris[j].p3;
		}
		// Copy normals. Note, normals are transformed the same way the vertices are.
		for (int i = 0; i < m->nVerts; i++) {
			m->verts[i].nx = -(*nifNorms)[i].x;
			m->verts[i].nz = (*nifNorms)[i].y;
			m->verts[i].ny = (*nifNorms)[i].z;
		}

		// virtually Weld verts across uv seams
		kd_matcher matcher(m->verts, m->nVerts);
		for (int i = 0; i < matcher.matches.size(); i++) {
			Vertex* a = matcher.matches[i].first;
			Vertex* b = matcher.matches[i].second;
			m->weldVerts[a->indexRef].push_back(b->indexRef);
			m->weldVerts[b->indexRef].push_back(a->indexRef);
		}
	}

	m->CreateBVH();
	m->CreateKDTree();

	if (color)
		m->color = (*color);

	namedMeshes[m->shapeName] = meshes.size();
	meshes.push_back(m);

	if (!color) {
		for (int i = 0; i < meshes.size(); i++) {
			float c = 0.4f + ((0.6f / (meshes.size())) * i);
			meshes[i]->color = Vector3(c, c, c);
		}
	}

	// Offset camera for skinned FO4 shapes
	if (nif->GetHeader().GetUserVersion() == 12 && nif->GetHeader().GetUserVersion2() == 130) {
		NiShape* shape = nif->FindShapeByName(shapeName);
		if (shape && shape->IsSkinned())
			camOffset.y = 12.0f;
	}
}

int GLSurface::AddVisRay(Vector3& start, Vector3& direction, float length) {
	int rayMesh = GetOverlayID("RayMesh");
	int boxMesh = GetOverlayID("BoxMesh");

	Matrix4 rotMat;

	if (boxMesh >= 0) {
		delete overlays[boxMesh];
		overlays.erase(overlays.begin() + boxMesh);
		boxMesh = 0;
	}
	boxMesh = GetOverlayID("TriMesh");
	if (boxMesh >= 0) {
		delete overlays[boxMesh];
		overlays.erase(overlays.begin() + boxMesh);
		boxMesh = 0;
	}
	if (rayMesh >= 0) {
		delete overlays[rayMesh];
		overlays.erase(overlays.begin() + rayMesh);
		rayMesh = 0;
	}

	mesh* m = new mesh();
	m->rendermode = RenderMode::UnlitWire;
	m->color = Vector3(0.0f, 0.0f, 1.0f);
	m->nVerts = 2;
	m->verts = new Vertex[2];

	m->verts[0].x = start.x;
	m->verts[0].y = start.y;
	m->verts[0].z = start.z;

	m->verts[1] = m->verts[0];
	m->verts[1].x += (direction.x * length);
	m->verts[1].y += (direction.y * length);
	m->verts[1].z += (direction.z * length);

	m->nEdges = 1;
	m->edges = new Edge[1];

	m->edges[0].p1 = 0;
	m->edges[0].p2 = 1;

	m->shapeName = "RayMesh";
	namedOverlays[m->shapeName] = overlays.size();
	overlays.push_back(m);

	Vector3 o(start.x, start.y, start.z);
	Vector3 d(direction.x, direction.y, direction.z);

	for (auto &itMesh : activeMeshes) {
		vector<IntersectResult> results;
		if (itMesh->bvh->IntersectRay(o, d, &results)) {
			mesh* mv = new mesh();
			itMesh->bvh->BuildRayIntersectFrames(o, d, &mv->verts, &mv->nVerts, &mv->edges, &mv->nEdges);
			mv->shapeName = "BoxMesh";
			mv->rendermode = RenderMode::UnlitWire;
			namedOverlays[mv->shapeName] = overlays.size();
			overlays.push_back(mv);
			if (results.size() > 900) {
				int min_i = 0;
				float minDist = results[0].HitDistance;
				for (int i = 1; i < results.size(); i++) {
					if (results[i].HitDistance < minDist)
						min_i = i;
				}

				Vector3 origin;
				origin = results[min_i].HitCoord;
				Vector3 norm;
				meshes[0]->tris[results[min_i].HitFacet].trinormal(meshes[0]->verts, &norm);

				/* KD Nearest Neighbor search */

				Vertex querypoint;
				querypoint = origin;

				meshes[0]->kdtree->kd_nn(&querypoint, .40f);
				mesh* mv = new mesh();
				mv->nVerts = meshes[0]->kdtree->queryResult.size() + 1;
				mv->verts = new Vertex[mv->nVerts];
				mv->rendermode = RenderMode::UnlitPoints;
				mv->color = Vector3(0.0f, 1.0f, 1.0f);
				mv->shapeName = "TriMesh";
				int i = 0;
				for (auto &kd_result_it : meshes[0]->kdtree->queryResult)
					mv->verts[i++] = (*kd_result_it.v);

				mv->verts[i] = origin;
				namedOverlays[mv->shapeName] = overlays.size();
				overlays.push_back(mv);
			}
		}
	}

	return overlays.size() - 1;
}

mesh* GLSurface::AddVisPoint(const Vector3& p, const string& name, const Vector3* color) {
	mesh* m = GetOverlay(name);
	if (m) {
		m->verts[0] = p;
		if (color)
			m->color = (*color);
		else
			m->color = Vector3(0.0f, 1.0f, 1.0f);

		m->bVisible = true;
		return m;
	}

	m = new mesh();

	m->nVerts = 1;
	m->verts = new Vertex[1];
	m->rendermode = RenderMode::UnlitPoints;
	m->nEdges = 0;

	m->verts[0] = p;

	m->shapeName = name;
	m->color = Vector3(0.0f, 1.0f, 1.0f);

	namedOverlays[m->shapeName] = overlays.size();
	overlays.push_back(m);
	return m;
}

int  GLSurface::AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const string& name) {
	Matrix4 rotMat;
	int ringMesh = GetOverlayID(name);
	if (ringMesh >= 0)
		delete overlays[ringMesh];
	else
		ringMesh = -1;

	mesh* m = new mesh();

	m->nVerts = 360 / 5;
	m->verts = new Vertex[m->nVerts];
	m->rendermode = RenderMode::UnlitWire;
	m->nEdges = m->nVerts;
	m->edges = new Edge[m->nEdges];

	rotMat.Align(Vector3(0.0f, 0.0f, 1.0f), normal);
	rotMat.Translate(center);

	float rStep = DEG2RAD * 5.0f;
	float i = 0.0f;
	for (int j = 0; j < m->nVerts; j++) {
		m->verts[j].x = sin(i) * radius;
		m->verts[j].y = cos(i) * radius;
		m->verts[j].z = 0;
		m->edges[j].p1 = j;
		m->edges[j].p2 = j + 1;
		i += rStep;
		m->verts[j].pos(rotMat * m->verts[j]);
	}

	m->edges[m->nEdges - 1].p2 = 0;
	m->shapeName = name;
	m->color = Vector3(1.0f, 0.0f, 0.0f);

	if (ringMesh >= 0) {
		overlays[ringMesh] = m;
	}
	else {
		namedOverlays[m->shapeName] = overlays.size();
		overlays.push_back(m);
		ringMesh = overlays.size() - 1;
	}
	return ringMesh;
}

mesh* GLSurface::AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const string& name) {
	int myMesh = GetOverlayID(name);
	if (myMesh >= 0)
		delete overlays[myMesh];
	else
		myMesh = -1;

	mesh* m = new mesh();
	int nRingSegments = 36;
	int nRingSteps = 4;

	m->nVerts = (nRingSegments)* nRingSteps;
	m->nTris = m->nVerts * 2;
	m->verts = new Vertex[m->nVerts];
	m->tris = new Triangle[m->nTris];
	m->rendermode = RenderMode::UnlitSolid;
	m->color = color;

	Vertex* loftVerts = new Vertex[nRingSteps];

	Matrix4 xf1;
	xf1.Align(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f));
	xf1.Translate(Vector3(0.0f, holeRadius, 0.0f));

	Matrix4 xf3;
	xf3.Align(Vector3(0.0f, 0.0f, 1.0f), normal);
	xf3.Translate(center);

	float rStep = DEG2RAD * (360.0f / nRingSteps);
	float i = 0.0f;
	for (int r = 0; r < nRingSteps; r++) {
		loftVerts[r].x = sin(i) * ringRadius;
		loftVerts[r].y = cos(i) * ringRadius;
		loftVerts[r].z = 0.0f;
		i += rStep;
		loftVerts[r] = xf1 * loftVerts[r];
	}

	rStep = DEG2RAD * (360.0f / nRingSegments);
	Matrix4 xf2;
	int seg = 0.0f;
	int ctri = 0;
	int p1, p2, p3, p4;
	for (int vi = 0; vi < m->nVerts;){
		xf2.Identity();
		xf2.Rotate(rStep*seg, 0.0f, 0.0f, 1.0f);
		for (int r = 0; r < nRingSteps; r++){
			m->verts[vi] = xf3*xf2*loftVerts[r];
			if (seg > 0) {
				p1 = vi;
				p3 = vi - nRingSteps;
				if (r == nRingSteps - 1) {
					p2 = vi - (nRingSteps - 1);
				}
				else {
					p2 = vi + 1;
				}
				p4 = p2 - nRingSteps;
				m->tris[ctri++] = Triangle(p1, p2, p3);
				m->tris[ctri++] = Triangle(p3, p2, p4);
			}
			vi++;
		}
		seg++;
	}

	// last segment needs tris
	for (int r = 0; r < nRingSteps; r++) {
		p1 = r;
		p3 = m->nVerts - (nRingSteps - r);
		if (r == (nRingSteps - 1)) {
			p2 = 0;
			p4 = m->nVerts - nRingSteps;
		}
		else {
			p2 = r + 1;
			p4 = p3 + 1;
		}
		m->tris[ctri++] = Triangle(p1, p2, p3);
		m->tris[ctri++] = Triangle(p3, p2, p4);
	}


	m->nEdges = 0;
	m->edges = nullptr;
	m->shapeName = name;

	if (myMesh >= 0) {
		overlays[myMesh] = m;
	}
	else {
		namedOverlays[m->shapeName] = overlays.size();
		overlays.push_back(m);
		myMesh = overlays.size() - 1;
	}

	return m;
}


mesh* GLSurface::AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const string& name) {
	Matrix4 rotMat;

	int myMesh = GetOverlayID(name);
	if (myMesh >= 0)
		delete overlays[myMesh];
	else
		myMesh = -1;

	mesh* m = new mesh();

	int nRingVerts = 36;

	m->nVerts = nRingVerts * 3 + 1; // base ring, inner point ring, outer point ring, and the tip
	m->nTris = nRingVerts * 5;
	m->verts = new Vertex[m->nVerts];
	m->rendermode = RenderMode::UnlitSolid;
	m->color = color;

	rotMat.Align(Vector3(0.0f, 0.0f, 1.0f), direction);
	rotMat.Translate(origin);

	float rStep = (3.141592653 / 180) * (360.0f / nRingVerts);
	float i = 0.0f;
	float stemlen = length * 4.0 / 5.0;
	for (int j = 0; j < nRingVerts; j++) {
		float si = sin(i);
		float ci = cos(i);
		m->verts[j].x = si * stemRadius;
		m->verts[j + nRingVerts].x = m->verts[j].x;
		m->verts[j + (nRingVerts * 2)].x = si * pointRadius;
		m->verts[j].y = ci * stemRadius;
		m->verts[j + nRingVerts].y = m->verts[j].y;
		m->verts[j + (nRingVerts * 2)].y = ci * pointRadius;
		m->verts[j].z = 0;
		m->verts[j + nRingVerts].z = stemlen;
		m->verts[j + (nRingVerts * 2)].z = stemlen;
		i += rStep;
		m->verts[j].pos(rotMat * m->verts[j]);
		m->verts[j + nRingVerts].pos(rotMat * m->verts[j + nRingVerts]);
		m->verts[j + (nRingVerts * 2)].pos(rotMat * m->verts[j + (nRingVerts * 2)]);
	}
	m->verts[m->nVerts - 1].x = 0;
	m->verts[m->nVerts - 1].y = 0;
	m->verts[m->nVerts - 1].z = length;
	m->verts[m->nVerts - 1].pos(rotMat * m->verts[m->nVerts - 1]);
	int p1, p2, p3, p4, p5, p6, p7;
	p7 = m->nVerts - 1;
	m->tris = new Triangle[m->nTris];
	int t = 0;
	for (int j = 0; j < nRingVerts; j++) {
		p1 = j;
		p3 = j + nRingVerts;
		p5 = j + nRingVerts * 2;
		if (j < nRingVerts - 1) {
			p2 = j + 1;
			p4 = j + nRingVerts + 1;
			p6 = j + nRingVerts * 2 + 1;
		}
		else{
			p2 = 0;
			p4 = nRingVerts;
			p6 = nRingVerts * 2;
		}
		m->tris[t].p1 = p1;	m->tris[t].p2 = p2;	m->tris[t].p3 = p3;
		m->tris[t + 1].p1 = p3;	m->tris[t + 1].p2 = p2;	m->tris[t + 1].p3 = p4;
		m->tris[t + 2].p1 = p3;	m->tris[t + 2].p2 = p4;	m->tris[t + 2].p3 = p5;
		m->tris[t + 3].p1 = p5;	m->tris[t + 3].p2 = p4;	m->tris[t + 3].p3 = p6;
		m->tris[t + 4].p1 = p5;	m->tris[t + 4].p2 = p6;	m->tris[t + 4].p3 = p7;
		t += 5;
	}

	m->nEdges = 0;
	m->edges = nullptr;
	m->shapeName = name;

	if (myMesh >= 0) {
		overlays[myMesh] = m;
	}
	else {
		namedOverlays[m->shapeName] = overlays.size();
		overlays.push_back(m);
		myMesh = overlays.size() - 1;
	}
	return m;
}

void GLSurface::Update(const string& shapeName, vector<Vector3>* vertices, vector<Vector2>* uvs, vector<int>* changed) {
	int id = GetMeshID(shapeName);
	if (id < 0)
		return;

	Update(id, vertices, uvs, changed);
}

void GLSurface::Update(int shapeIndex, vector<Vector3>* vertices, vector<Vector2>* uvs, vector<int>* changed) {
	if (shapeIndex >= meshes.size())
		return;

	mesh* m = meshes[shapeIndex];
	if (m->nVerts != vertices->size())
		return;

	Vector3 old;
	for (int i = 0; i < m->nVerts; i++) {
		if (changed)
			old = m->verts[i];

		m->verts[i].x = (*vertices)[i].x / -10.0f;
		m->verts[i].z = (*vertices)[i].y / 10.0f;
		m->verts[i].y = (*vertices)[i].z / 10.0f;

		if (uvs)
			m->texcoord[i] = (*uvs)[i];

		if (changed && old != m->verts[i])
			(*changed).push_back(i);
	}
	m->bBuffersLoaded = false;
}

void GLSurface::RecalculateMeshBVH(const string& shapeName) {
	int id = GetMeshID(shapeName);
	if (id < 0)
		return;

	RecalculateMeshBVH(id);
}

void GLSurface::RecalculateMeshBVH(int shapeIndex) {
	if (shapeIndex >= meshes.size())
		return;

	mesh* m = meshes[shapeIndex];
	m->CreateBVH();
}

void GLSurface::SetMeshVisibility(const string& name, bool visible) {
	int shapeIndex = GetMeshID(name);
	if (shapeIndex < 0)
		return;

	mesh* m = meshes[shapeIndex];
	m->bVisible = visible;
}

void GLSurface::SetMeshVisibility(int shapeIndex, bool visible) {
	if (shapeIndex >= meshes.size())
		return;

	mesh* m = meshes[shapeIndex];
	m->bVisible = visible;
}

void GLSurface::SetOverlayVisibility(const string& name, bool visible) {
	int shapeIndex = GetOverlayID(name);
	if (shapeIndex < 0)
		return;

	mesh* m = overlays[shapeIndex];
	m->bVisible = visible;
}

void GLSurface::SetActiveMeshesID(const vector<int>& shapeIndices) {
	activeMeshesID.clear();
	activeMeshes.clear();

	activeMeshesID = shapeIndices;
	for (auto &id : activeMeshesID)
		activeMeshes.push_back(meshes[id]);
}

void GLSurface::SetActiveMeshesID(const vector<string>& shapeNames) {
	activeMeshesID.clear();
	activeMeshes.clear();

	for (auto &s : shapeNames) {
		int id = GetMeshID(s);
		if (id != -1) {
			activeMeshesID.push_back(id);
			activeMeshes.push_back(meshes[id]);
		}
	}
}

void GLSurface::SetSelectedMesh(const string& shapeName) {
	int id = GetMeshID(shapeName);
	if (id != -1)
		selectedMesh = meshes[id];
	else
		selectedMesh = nullptr;
}

RenderMode GLSurface::SetMeshRenderMode(const string& name, RenderMode mode) {
	int shapeIndex = GetMeshID(name);
	if (shapeIndex < 0)
		return RenderMode::Normal;
	mesh* m = meshes[shapeIndex];
	RenderMode r = m->rendermode;
	m->rendermode = mode;
	return r;
}

GLMaterial* GLSurface::AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile) {
	GLMaterial* mat = resLoader.AddMaterial(textureFile, vShaderFile, fShaderFile);
	if (!mat) {
		// Use noImage material if loader failed
		if (!noImage)
			noImage = resLoader.AddMaterial("res\\images\\NoImg.png", "res\\shaders\\mask.vert", "res\\shaders\\default.frag");

		mat = noImage;
	}

	if (mat) {
		string shaderError;
		if (mat->GetShader().GetError(&shaderError)) {
			wxLogError(wxString(shaderError));
			wxMessageBox(shaderError, _("OpenGL Error"), wxICON_ERROR);
		}
	}

	return mat;
}
