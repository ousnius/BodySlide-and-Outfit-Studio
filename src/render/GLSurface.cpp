/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "GLSurface.h"

#include <wx/msgdlg.h>
#include <wx/log.h>

#include <algorithm>
#include <set>
#include <limits>


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
		attribs.PlatformDefaults().RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(24).Level(0).SampleBuffers(1).Samplers(8).EndList();

		bool displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		if (!displaySupported) {
			wxLogWarning("OpenGL attributes not supported. Trying out different ones...");
			attribs.Reset();

			// 4x AA
			attribs.RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(24).Level(0).SampleBuffers(1).Samplers(4).EndList();
			displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		}

		if (!displaySupported) {
			wxLogWarning("OpenGL attributes not supported. Trying out different ones...");
			attribs.Reset();

			// No AA
			attribs.RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(24).Level(0).SampleBuffers(0).EndList();
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
		ctxAttribs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
		ctxAttribsInitialized = true;
	}

	return ctxAttribs;
}

void GLSurface::InitLighting() {
	ambientLight = 0.10f;

	frontalLight.diffuse = Vector3(0.2f, 0.2f, 0.2f);

	directionalLight0.diffuse = Vector3(0.6f, 0.6f, 0.6f);
	directionalLight0.direction = Vector3(-0.90f, 0.1f, 1.0f);
	directionalLight0.direction.Normalize();

	directionalLight1.diffuse = Vector3(0.6f, 0.6f, 0.6f);
	directionalLight1.direction = Vector3(0.70f, 0.1f, 1.0f);
	directionalLight1.direction.Normalize();

	directionalLight2.diffuse = Vector3(0.85f, 0.85f, 0.85f);
	directionalLight2.direction = Vector3(0.3f, 0.2f, -1.0f);
	directionalLight2.direction.Normalize();
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
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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

	InitLighting();

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

	if (primitiveMat) {
		delete primitiveMat;
		primitiveMat = nullptr;
	}

	resLoader.Cleanup();
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
	glm::vec4 vp(0.0f, 0.0f, (float)vpW, (float)vpH);
	glm::vec3 win(vp[2] / 2.0f, vp[3] / 2.0f, 0.0f);
	glm::vec3 res = glm::unProject(win, modelView, projection, vp);

	result.x = res.x;
	result.y = res.y;
	result.z = res.z;
}

void GLSurface::SetView(const char type) {
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
}

void GLSurface::SetPerspective(const bool enabled) {
	perspective = enabled;
}

void GLSurface::SetFieldOfView(const int fieldOfView) {
	mFov = fieldOfView;
}

void GLSurface::UpdateLights(const int ambient, const int frontal, const int directional0, const int directional1, const int directional2,
	const Vector3 directional0Dir, const Vector3 directional1Dir, const Vector3 directional2Dir)
{
	ambientLight = 0.01f * ambient;
	frontalLight.diffuse = Vector3(0.01f * frontal, 0.01f * frontal, 0.01f * frontal);
	directionalLight0.diffuse = Vector3(0.01f * directional0, 0.01f * directional0, 0.01f * directional0);
	directionalLight1.diffuse = Vector3(0.01f * directional1, 0.01f * directional1, 0.01f * directional1);
	directionalLight2.diffuse = Vector3(0.01f * directional2, 0.01f * directional2, 0.01f * directional2);

	if (Vector3() != directional0Dir)
		directionalLight0.direction = directional0Dir;
	if (Vector3() != directional1Dir)
		directionalLight1.direction = directional1Dir;
	if (Vector3() != directional2Dir)
		directionalLight2.direction = directional2Dir;

	directionalLight0.direction.Normalize();
	directionalLight1.direction.Normalize();
	directionalLight2.direction.Normalize();
}

void GLSurface::GetPickRay(int ScreenX, int ScreenY, Vector3& dirVect, Vector3& outNearPos) {
	glm::vec4 vp(0.0f, 0.0f, (float)vpW, (float)vpH);

	glm::vec3 winS(ScreenX, vp[3] - ScreenY, 0.0f);
	glm::vec3 resS = glm::unProject(winS, modelView, projection, vp);

	glm::vec3 winD(ScreenX, vp[3] - ScreenY, 1.0f);
	glm::vec3 resD = glm::unProject(winD, modelView, projection, vp);

	dirVect.x = resD.x - resS.x;
	dirVect.y = resD.y - resS.y;
	dirVect.z = resD.z - resS.z;

	outNearPos.x = resS.x;
	outNearPos.y = resS.y;
	outNearPos.z = resS.z;

	dirVect.Normalize();
}

int GLSurface::PickMesh(int ScreenX, int ScreenY) {
	Vector3 o;
	Vector3 d;
	float curd = FLT_MAX;
	int result = -1;

	GetPickRay(ScreenX, ScreenY, d, o);
	std::vector<IntersectResult> results;

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
	std::unordered_map<mesh*, float> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!m->bvh)
			continue;

		if (!allMeshes && m != selectedMesh)
			continue;

		std::vector<IntersectResult> results;
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
	std::unordered_map<mesh*, float> allHitDistances;
	for (auto &ov : overlays) {
		std::vector<IntersectResult> results;
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

bool GLSurface::UpdateCursor(int ScreenX, int ScreenY, bool allMeshes, std::string* hitMeshName, int* outHoverTri, float* outHoverWeight, float* outHoverMask) {
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

	std::unordered_map<mesh*, Vector3> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!allMeshes && m != selectedMesh)
			continue;

		std::vector<IntersectResult> results;
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

				Vector3 hilitepoint = m->verts[t.p1];
				float closestdist = fabs(m->verts[t.p1].DistanceTo(origin));
				float nextdist = fabs(m->verts[t.p2].DistanceTo(origin));
				int pointid = t.p1;

				if (nextdist < closestdist) {
					closestdist = nextdist;
					hilitepoint = m->verts[t.p2];
					pointid = t.p2;
				}
				nextdist = fabs(m->verts[t.p3].DistanceTo(origin));
				if (nextdist < closestdist) {
					hilitepoint = m->verts[t.p3];
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

bool GLSurface::GetCursorVertex(int ScreenX, int ScreenY, int* outIndex) {
	if (activeMeshes.empty())
		return false;

	Vector3 o;
	Vector3 d;
	Vector3 v;
	Vector3 vo;

	if (outIndex)
		(*outIndex) = -1;

	GetPickRay(ScreenX, ScreenY, d, o);

	for (auto &m : activeMeshes) {
		std::vector<IntersectResult> results;
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

				Vector3 hilitepoint = m->verts[t.p1];
				float closestdist = fabs(m->verts[t.p1].DistanceTo(origin));
				float nextdist = fabs(m->verts[t.p2].DistanceTo(origin));
				int pointid = t.p1;

				if (nextdist < closestdist) {
					closestdist = nextdist;
					hilitepoint = m->verts[t.p2];
					pointid = t.p2;
				}

				nextdist = fabs(m->verts[t.p3].DistanceTo(origin));

				if (nextdist < closestdist) {
					hilitepoint = m->verts[t.p3];
					pointid = t.p3;
				}

				if (*outIndex)
					(*outIndex) = pointid;

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
}

void GLSurface::GetSize(uint & w, uint & h)
{
	w = vpW;
	h = vpH;
}

void GLSurface::UpdateProjection() {
	float aspect = (float)vpW / (float)vpH;
	if (perspective)
		projection = glm::perspective(glm::radians(mFov), aspect, 0.1f, 1000.0f);
	else
		projection = glm::ortho((camPos.z + camOffset.z) / 2.0f * aspect, (-camPos.z + camOffset.z) / 2.0f * aspect, (camPos.z + camOffset.z) / 2.0f, (-camPos.z + camOffset.z) / 2.0f, 0.1f, 1000.0f);

	modelView = glm::translate(glm::mat4x4(), glm::vec3(camPos.x, camPos.y, camPos.z));
	modelView = glm::rotate(modelView, glm::radians(camRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelView = glm::rotate(modelView, glm::radians(camRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelView = glm::translate(modelView, glm::vec3(camOffset.x, camOffset.y, camOffset.z));
}

void GLSurface::RenderFullScreenQuad(GLMaterial* renderShader, unsigned int w, unsigned int h) {	
	if (!canvas)
		return;
	canvas->SetCurrent(*context);

	// Dummy vertexArrayObject, needs to be bound in order for OGL to activate the shader with Draw Arrays.
	// No array buffer is used, so this shouldn't be too slow to create here, but could be moved if necessary.
	GLuint m_vertexArrayObject = 0;
	glGenVertexArrays(1, &m_vertexArrayObject);
	
	glViewport(0, 0, w, h);
	glClear(GL_DEPTH_BUFFER_BIT);	

	// This relies on shader manipulation of vertex positions to render a single triangle clipped to the surface
	// 
	GLShader shader = renderShader->GetShader();
	shader.Begin();
		renderShader->BindTextures(0, false);
		// bind the dummy array and send three fake positions to the shader.
		glBindVertexArray(m_vertexArrayObject);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glFlush();

	shader.End();

	// clear out the dummy array object.
	glDeleteVertexArrays(1, &m_vertexArrayObject);

	canvas->SwapBuffers();
}

void GLSurface::RenderToTexture(GLMaterial* renderShader) {
	if (!canvas)
		return;

	canvas->SetCurrent(*context);

	//glClearColor(colorBackground.x, colorBackground.y, colorBackground.z, 0.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up orthographic projection and identity model view 

	projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -100.0f, 100.0f);
	//projection = glm::ortho((camPos.z + camOffset.z) / 2.0f * aspect, (-camPos.z + camOffset.z) / 2.0f * aspect, (camPos.z + camOffset.z) / 2.0f, (-camPos.z + camOffset.z) / 2.0f, 0.1f, 100.0f);
	modelView = glm::mat4x4();

	mesh* m = nullptr;
	bool oldDS;
	GLMaterial* oldmat;

	// Render regular meshes only
	for (int i = 0; i < meshes.size(); i++) {
		m = meshes[i];
		if (!m->bVisible || m->nTris == 0)
			continue;

		oldDS = m->doublesided;
		oldmat = m->material;
		m->material = renderShader;
		m->modelSpace = false;
		m->doublesided = true;
		RenderMesh(m);
		m->doublesided = oldDS;
		m->modelSpace = true;
		m->material = oldmat;
	}

	// note no buffer swap

}

void GLSurface::RenderOneFrame() {
	if (!canvas)
		return;

	canvas->SetCurrent(*context);

	glClearColor(colorBackground.x, colorBackground.y, colorBackground.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateProjection();

	mesh* m = nullptr;

	// Render regular meshes
	for (int i = 0; i < meshes.size(); i++) {
		m = meshes[i];
		if (!m->bVisible || m->nTris == 0)
			continue;

		bool alphaBlend = m->alphaFlags & 1;
		if (!alphaBlend)
			RenderMesh(m);
	}

	// Render meshes with alpha blending only
	for (int i = 0; i < meshes.size(); i++) {
		m = meshes[i];
		if (!m->bVisible || m->nTris == 0)
			continue;

		bool alphaBlend = m->alphaFlags & 1;
		if (alphaBlend)
			RenderMesh(m);
	}

	// Render overlays on top
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
	glEnable(GL_DEPTH_TEST);

	if (!m->doublesided)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m->UpdateBuffers();

	if (!m->genBuffers || !m->material)
		return;

	GLShader& shader = m->material->GetShader();
	if (!shader.Begin())
		return;

	shader.SetMatrixProjection(projection);
	shader.SetMatrixModelView(modelView);
	shader.SetColor(m->color);
	shader.SetModelSpace(m->modelSpace);
	//shader.SetModelSpace(false);
	shader.SetSpecularEnabled(m->specular);
	shader.SetEmissive(m->emissive);
	shader.SetLightingEnabled(bLighting);
	shader.SetWireframeEnabled(false);
	shader.SetPointsEnabled(false);

	glBindVertexArray(m->vao);

	if (m->rendermode == RenderMode::Normal || m->rendermode == RenderMode::LitWire || m->rendermode == RenderMode::UnlitSolid) {
		shader.SetFrontalLight(frontalLight);
		shader.SetDirectionalLight(directionalLight0, 0);
		shader.SetDirectionalLight(directionalLight1, 1);
		shader.SetDirectionalLight(directionalLight2, 2);
		shader.SetAmbientLight(ambientLight);
		shader.SetProperties(m->prop);

		if (m->rendermode == RenderMode::LitWire) {
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		if (m->rendermode == RenderMode::UnlitSolid) {
			glDisable(GL_CULL_FACE);
			shader.SetLightingEnabled(false);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);			// Positions
		glEnableVertexAttribArray(0);

		if (m->norms) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[1]);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Normals
			glEnableVertexAttribArray(1);
		}

		if (m->vcolors) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[2]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Colors
			glEnableVertexAttribArray(2);
		}

		if (bTextured && m->textured && m->texcoord) {
			shader.SetAlphaProperties(m->alphaFlags, m->alphaThreshold / 255.0f);

			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[3]);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Texture Coordinates
			glEnableVertexAttribArray(3);

			m->material->BindTextures(largestAF, m->backlight);
		}

		// Offset triangles so that points can be visible
		glEnable(GL_POLYGON_OFFSET_FILL);
		if (m->bShowPoints)
			glPolygonOffset(1.0f, 1.0f);
		else
			glPolygonOffset(0.03f, 0.03f);

		// Render mesh
		glDrawElements(GL_TRIANGLES, m->nTris * 3, GL_UNSIGNED_SHORT, (GLvoid*)0);
		glDisable(GL_POLYGON_OFFSET_FILL);

		// Render wireframe
		if (bWireframe && m->rendermode == RenderMode::Normal) {
			shader.SetWireframeEnabled(true);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, m->nTris * 3, GL_UNSIGNED_SHORT, (GLvoid*)0);
		}

		if (bTextured && m->textured && m->texcoord)
			glDisableVertexAttribArray(3);

		// Render points
		if (m->bShowPoints && m->vcolors) {
			glDisable(GL_CULL_FACE);
			shader.SetLightingEnabled(false);

			shader.SetPointsEnabled(true);
			glDrawArrays(GL_POINTS, 0, m->nVerts);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}
	else if (m->rendermode == RenderMode::UnlitWire) {
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_LINES, m->nEdges * 2, GL_UNSIGNED_SHORT, (GLvoid*)0);

		glDisableVertexAttribArray(0);
	}
	else if (m->rendermode == RenderMode::UnlitPoints) {
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		glEnableVertexAttribArray(0);

		shader.SetPointsEnabled(true);
		glDrawArrays(GL_POINTS, 0, m->nVerts);

		glDisableVertexAttribArray(0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	shader.End();
}

void GLSurface::UpdateShaders(mesh* m) {
	if (m->material) {
		m->material->GetShader().ShowTexture(bTextured);
		m->material->GetShader().ShowLighting(bLighting);
		m->material->GetShader().ShowWeight(bWeightColors);
		m->material->GetShader().ShowSegments(bSegmentColors);
	}
}

void  GLSurface::ReloadMeshFromNif(NifFile* nif, std::string shapeName) {
	DeleteMesh(shapeName);
	AddMeshFromNif(nif, shapeName);
}

void GLSurface::AddMeshFromNif(NifFile* nif, std::string shapeName, Vector3* color, bool smoothNormalSeams) {
	std::vector<Vector3> nifVerts;
	std::vector<Triangle> nifTris;
	nif->GetVertsForShape(shapeName, nifVerts);
	nif->GetTrisForShape(shapeName, &nifTris);

	const std::vector<Vector3>* nifNorms = nif->GetNormalsForShape(shapeName, false);
	const std::vector<Vector2>* nifUvs = nif->GetUvsForShape(shapeName);

	mesh* m = new mesh();

	NiShader* shader = nif->GetShader(shapeName);
	if (shader) {
		m->modelSpace = shader->IsModelSpace();
		m->specular = shader->HasSpecular();
		m->emissive = shader->IsEmissive();
		m->backlight = shader->HasBacklight();
		m->doublesided = shader->IsDoubleSided();

		m->prop.uvOffset = shader->GetUVOffset();
		m->prop.uvScale = shader->GetUVScale();
		m->prop.specularColor = shader->GetSpecularColor();
		m->prop.specularStrength = shader->GetSpecularStrength();
		m->prop.shininess = shader->GetGlossiness();
		m->prop.envReflection = shader->GetEnvironmentMapScale();

		Color4 emissiveColor = shader->GetEmissiveColor();
		m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		m->prop.emissiveMultiple = shader->GetEmissiveMultiple();

		m->prop.alpha = shader->GetAlpha();

		NiMaterialProperty* material = nif->GetMaterialProperty(shapeName);
		if (material) {
			m->emissive = material->IsEmissive();

			m->prop.specularColor = material->GetSpecularColor();
			m->prop.shininess = material->GetGlossiness();

			emissiveColor = material->GetEmissiveColor();
			m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
			m->prop.emissiveMultiple = material->GetEmissiveMultiple();

			m->prop.alpha = material->GetAlpha();
		}

		nif->GetAlphaForShape(shapeName, m->alphaFlags, m->alphaThreshold);
	}

	m->nVerts = nifVerts.size();
	m->nTris = nifTris.size();

	if (m->nVerts > 0) {
		m->verts = new Vector3[m->nVerts];
		m->norms = new Vector3[m->nVerts];
		m->vcolors = new Vector3[m->nVerts];
		m->texcoord = new Vector2[m->nVerts];
	}

	if (m->nTris > 0)
		m->tris = new Triangle[m->nTris];

	m->shapeName = shapeName;
	m->smoothSeamNormals = smoothNormalSeams;

	// Load verts. NIF verts are scaled up by approx. 10 and rotated on the x axis (Z up, Y forward).  
	// Scale down by 10 and rotate on x axis by flipping y and z components. To face the camera, this also mirrors
	// on X and Y (180 degree y axis rotation.)
	for (int i = 0; i < m->nVerts; i++) {
		m->verts[i].x = (nifVerts)[i].x / -10.0f;
		m->verts[i].z = (nifVerts)[i].y / 10.0f;
		m->verts[i].y = (nifVerts)[i].z / 10.0f;
	}

	if (nifUvs && !nifUvs->empty()) {
		for (int i = 0; i < m->nVerts; i++) {
			m->texcoord[i].u = (*nifUvs)[i].u;
			m->texcoord[i].v = (*nifUvs)[i].v;
		}
		m->textured = true;
	}

	if (!nifNorms || nifNorms->empty()) {
		// Copy triangles
		for (int t = 0; t < m->nTris; t++)
			m->tris[t] = nifTris[t];

		// Face normals
		m->FacetNormals();

		// Smooth normals
		if (smoothNormalSeams) {
			kd_matcher matcher(m->verts, m->nVerts);
			for (int i = 0; i < matcher.matches.size(); i++) {
				std::pair<Vector3*, int>& a = matcher.matches[i].first;
				std::pair<Vector3*, int>& b = matcher.matches[i].second;
				m->weldVerts[a.second].push_back(b.second);
				m->weldVerts[b.second].push_back(a.second);

				Vector3& an = m->norms[a.second];
				Vector3& bn = m->norms[b.second];
				if (an.angle(bn) < 60.0f * DEG2RAD) {
					Vector3 anT = an;
					an += bn;
					bn += anT;
				}
			}

			for (int i = 0; i < m->nVerts; i++) {
				Vector3& pn = m->norms[i];
				pn.Normalize();
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
			m->norms[i].x = -(*nifNorms)[i].x;
			m->norms[i].z = (*nifNorms)[i].y;
			m->norms[i].y = (*nifNorms)[i].z;
		}

		// Virtually weld verts across UV seams
		kd_matcher matcher(m->verts, m->nVerts);
		for (int i = 0; i < matcher.matches.size(); i++) {
			std::pair<Vector3*, int>& a = matcher.matches[i].first;
			std::pair<Vector3*, int>& b = matcher.matches[i].second;
			m->weldVerts[a.second].push_back(b.second);
			m->weldVerts[b.second].push_back(a.second);
		}
	}

	m->CreateBVH();

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
	if (nif->GetHeader()->GetVersion().User() == 12 && nif->GetHeader()->GetVersion().User2() == 130) {
		NiShape* shape = nif->FindShapeByName(shapeName);
		if (shape && shape->IsSkinned())
			camOffset.y = 12.0f;
	}
}

mesh* GLSurface::AddVisPoint(const Vector3& p, const std::string& name, const Vector3* color) {
	mesh* m = GetOverlay(name);
	if (m) {
		m->verts[0] = p;
		if (color)
			m->color = (*color);
		else
			m->color = Vector3(0.0f, 1.0f, 1.0f);

		m->bVisible = true;
		m->QueueUpdate(mesh::UpdateType::Position);
		return m;
	}

	m = new mesh();

	m->nVerts = 1;
	m->verts = new Vector3[1];
	m->verts[0] = p;
	m->nEdges = 0;
	m->nTris = 0;

	m->rendermode = RenderMode::UnlitPoints;
	m->shapeName = name;
	m->color = Vector3(0.0f, 1.0f, 1.0f);
	m->material = GetPrimitiveMaterial();
	m->CreateBuffers();

	namedOverlays[m->shapeName] = overlays.size();
	overlays.push_back(m);
	return m;
}

int GLSurface::AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const std::string& name) {
	Matrix4 rotMat;
	int ringMesh = GetOverlayID(name);
	if (ringMesh >= 0)
		delete overlays[ringMesh];
	else
		ringMesh = -1;

	mesh* m = new mesh();

	m->nVerts = 360 / 5;
	m->nEdges = m->nVerts;

	m->verts = new Vector3[m->nVerts];
	m->edges = new Edge[m->nEdges];

	m->shapeName = name;
	m->color = Vector3(1.0f, 0.0f, 0.0f);
	m->rendermode = RenderMode::UnlitWire;
	m->material = GetPrimitiveMaterial();

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
		m->verts[j] = rotMat * m->verts[j];
	}

	m->edges[m->nEdges - 1].p2 = 0;
	m->CreateBuffers();

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

mesh* GLSurface::AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const std::string& name) {
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
	m->nEdges = 0;

	m->verts = new Vector3[m->nVerts];
	m->tris = new Triangle[m->nTris];

	m->shapeName = name;
	m->color = color;
	m->rendermode = RenderMode::UnlitSolid;
	m->material = GetPrimitiveMaterial();

	Vector3* loftVerts = new Vector3[nRingSteps];

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

	m->CreateBuffers();

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


mesh* GLSurface::AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const std::string& name) {
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
	m->nEdges = 0;

	m->verts = new Vector3[m->nVerts];
	m->color = color;
	m->shapeName = name;
	m->rendermode = RenderMode::UnlitSolid;
	m->material = GetPrimitiveMaterial();

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
		m->verts[j] = rotMat * m->verts[j];
		m->verts[j + nRingVerts] = rotMat * m->verts[j + nRingVerts];
		m->verts[j + (nRingVerts * 2)] = rotMat * m->verts[j + (nRingVerts * 2)];
	}

	m->verts[m->nVerts - 1].x = 0;
	m->verts[m->nVerts - 1].y = 0;
	m->verts[m->nVerts - 1].z = length;
	m->verts[m->nVerts - 1] = rotMat * m->verts[m->nVerts - 1];

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

	m->CreateBuffers();

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

mesh* GLSurface::AddVis3dCube(const Vector3& center, const Vector3& normal, float radius, const Vector3& color, const std::string& name) {
	int meshID = GetOverlayID(name);
	if (meshID >= 0)
		delete overlays[meshID];
	else
		meshID = -1;

	mesh* m = new mesh();

	int nCubeVerts = 8;
	int nCubeTris = 12;
	m->nVerts = nCubeVerts;
	m->nTris = nCubeTris;
	m->nEdges = 0;

	m->verts = new Vector3[m->nVerts];
	m->tris = new Triangle[m->nTris];
	m->color = color;
	m->shapeName = name;
	m->rendermode = RenderMode::UnlitSolid;
	m->material = GetPrimitiveMaterial();

	Matrix4 mat;
	mat.Align(Vector3(0.0f, 0.0f, 1.0f), normal);
	mat.Scale(radius, radius, radius);
	mat.Translate(center);

	m->verts[0] = mat * Vector3(-1.0f, -1.0f, 1.0f);
	m->verts[1] = mat * Vector3(1.0f, -1.0f, 1.0f);
	m->verts[2] = mat * Vector3(1.0f, 1.0f, 1.0f);
	m->verts[3] = mat * Vector3(-1.0f, 1.0f, 1.0f);
	m->verts[4] = mat * Vector3(-1.0f, -1.0f, -1.0f);
	m->verts[5] = mat * Vector3(1.0f, -1.0f, -1.0f);
	m->verts[6] = mat * Vector3(1.0f, 1.0f, -1.0f);
	m->verts[7] = mat * Vector3(-1.0f, 1.0f, -1.0f);

	m->tris[0] = Triangle(0, 1, 2);
	m->tris[1] = Triangle(2, 3, 0);
	m->tris[2] = Triangle(1, 5, 6);
	m->tris[3] = Triangle(6, 2, 1);
	m->tris[4] = Triangle(7, 6, 5);
	m->tris[5] = Triangle(5, 4, 7);
	m->tris[6] = Triangle(4, 0, 3);
	m->tris[7] = Triangle(3, 7, 4);
	m->tris[8] = Triangle(4, 5, 1);
	m->tris[9] = Triangle(1, 0, 4);
	m->tris[10] = Triangle(3, 2, 6);
	m->tris[11] = Triangle(6, 7, 3);

	m->CreateBuffers();

	if (meshID >= 0) {
		overlays[meshID] = m;
	}
	else {
		namedOverlays[m->shapeName] = overlays.size();
		overlays.push_back(m);
		meshID = overlays.size() - 1;
	}
	return m;
}

void GLSurface::Update(const std::string& shapeName, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs, std::set<int>* changed) {
	int id = GetMeshID(shapeName);
	if (id < 0)
		return;

	Update(id, vertices, uvs, changed);
}

void GLSurface::Update(int shapeIndex, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs, std::set<int>* changed) {
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
			(*changed).insert(i);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
	if (uvs)
		m->QueueUpdate(mesh::UpdateType::TextureCoordinates);
}

void GLSurface::RecalculateMeshBVH(const std::string& shapeName) {
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

void GLSurface::SetMeshVisibility(const std::string& name, bool visible) {
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

void GLSurface::SetOverlayVisibility(const std::string& name, bool visible) {
	int shapeIndex = GetOverlayID(name);
	if (shapeIndex < 0)
		return;

	mesh* m = overlays[shapeIndex];
	m->bVisible = visible;
}

void GLSurface::SetActiveMeshesID(const std::vector<int>& shapeIndices) {
	activeMeshesID.clear();
	activeMeshes.clear();

	activeMeshesID = shapeIndices;
	for (auto &id : activeMeshesID)
		activeMeshes.push_back(meshes[id]);
}

void GLSurface::SetActiveMeshesID(const std::vector<std::string>& shapeNames) {
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

void GLSurface::SetSelectedMesh(const std::string& shapeName) {
	int id = GetMeshID(shapeName);
	if (id != -1)
		selectedMesh = meshes[id];
	else
		selectedMesh = nullptr;
}

RenderMode GLSurface::SetMeshRenderMode(const std::string& name, RenderMode mode) {
	int shapeIndex = GetMeshID(name);
	if (shapeIndex < 0)
		return RenderMode::Normal;
	mesh* m = meshes[shapeIndex];
	RenderMode r = m->rendermode;
	m->rendermode = mode;
	return r;
}

GLMaterial* GLSurface::AddMaterial(const std::vector<std::string>& textureFiles, const std::string& vShaderFile, const std::string& fShaderFile) {
	GLMaterial* mat = resLoader.AddMaterial(textureFiles, vShaderFile, fShaderFile);
	if (mat) {
		std::string shaderError;
		if (mat->GetShader().GetError(&shaderError)) {
			wxLogError(wxString(shaderError));
			wxMessageBox(shaderError, _("OpenGL Error"), wxICON_ERROR);
		}
	}

	return mat;
}

GLMaterial* GLSurface::GetPrimitiveMaterial() {
	if (!primitiveMat) {
		primitiveMat = new GLMaterial("res\\shaders\\primitive.vert", "res\\shaders\\primitive.frag");

		std::string shaderError;
		if (primitiveMat->GetShader().GetError(&shaderError)) {
			wxLogError(wxString(shaderError));
			wxMessageBox(shaderError, _("OpenGL Error"), wxICON_ERROR);
		}
	}

	return primitiveMat;
}
