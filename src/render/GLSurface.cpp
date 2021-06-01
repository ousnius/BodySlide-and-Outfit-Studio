/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLSurface.h"
#include "../utils/ConfigurationManager.h"

#include <wx/msgdlg.h>
#include <wx/log.h>

#include <algorithm>
#include <set>
#include <limits>

using namespace nifly;

extern ConfigurationManager Config;

const wxGLAttributes& GLSurface::GetGLAttribs() {
	static bool attribsInitialized { false };
	static wxGLAttributes attribs;

	if (!attribsInitialized) {
		// 16x AA
		attribs.PlatformDefaults().DoubleBuffer().RGBA().Depth(24).SampleBuffers(1).Samplers(16).EndList();

		bool displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		if (!displaySupported) {
			wxLogWarning("OpenGL attributes with 16x AA not supported. Trying out different ones...");
			attribs.Reset();

			// 8x AA
			attribs.PlatformDefaults().DoubleBuffer().RGBA().Depth(24).SampleBuffers(1).Samplers(8).EndList();
			displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		}

		if (!displaySupported) {
			wxLogWarning("OpenGL attributes with 8x AA not supported. Trying out different ones...");
			attribs.Reset();

			// 4x AA
			attribs.PlatformDefaults().DoubleBuffer().RGBA().Depth(24).SampleBuffers(1).Samplers(4).EndList();
			displaySupported = wxGLCanvas::IsDisplaySupported(attribs);
		}

		if (!displaySupported) {
			wxLogWarning("OpenGL attributes with 4x AA not supported. Trying out different ones...");
			attribs.Reset();

			// No AA
			attribs.PlatformDefaults().DoubleBuffer().RGBA().Depth(24).SampleBuffers(0).EndList();
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
	ambientLight = 0.2f;

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
	// Set current context for resource deletion
	SetContext();

	for (auto &m : meshes)
		delete m;
	
	for (auto &o : overlays)
		delete o;

	meshes.clear();
	overlays.clear();
	activeMeshes.clear();

	selectedMesh = nullptr;

	if (primitiveMat) {
		delete primitiveMat;
		primitiveMat = nullptr;
	}

	resLoader.Cleanup();
}

void GLSurface::SetStartingView(const Vector3& pos, const Vector3& rot, const uint32_t& vpWidth, const uint32_t& vpHeight, const float& fov) {
	perspective = true;
	camPos = pos;
	camOffset.Zero();
	camRot = rot;
	camRotOffset.Zero();
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

void GLSurface::ClampCameraPosition(char axis, float lower, float upper) {
	switch (axis) {
	case 'X':
		camPos.x = std::max(lower, std::min(camPos.x, upper));
		break;
	case 'Y':
		camPos.y = std::max(lower, std::min(camPos.y, upper));
		break;
	case 'Z':
		camPos.z = std::max(lower, std::min(camPos.z, upper));
		break;
	}
}

void GLSurface::UnprojectCamera(Vector3& result) {
	glm::vec4 vp(0.0f, 0.0f, (float)vpW, (float)vpH);
	glm::vec3 win(vp[2] / 2.0f, vp[3] / 2.0f, 0.0f);
	glm::vec3 res = glm::unProject(win, matView, matProjection, vp);

	result.x = res.x;
	result.y = res.y;
	result.z = res.z;
}

void GLSurface::SetView(const char type) {
	camRotOffset.Zero();

	if (type == 'F') {
		camPos = Vector3(0.0f, -5.0f, -15.0f);
		camRot.Zero();
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
	const Vector3& directional0Dir, const Vector3& directional1Dir, const Vector3& directional2Dir)
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

void GLSurface::GetPickRay(int ScreenX, int ScreenY, mesh* m, Vector3& dirVect, Vector3& outNearPos) {
	glm::vec4 vp(0.0f, 0.0f, (float)vpW, (float)vpH);
	glm::vec3 winS(ScreenX, vp[3] - ScreenY, 0.0f);
	glm::vec3 winD(ScreenX, vp[3] - ScreenY, 1.0f);

	auto resS = glm::vec3();
	auto resD = glm::vec3();
	if (m) {
		resS = glm::unProject(winS, matView * m->matModel, matProjection, vp);
		resD = glm::unProject(winD, matView * m->matModel, matProjection, vp);
	}
	else {
		resS = glm::unProject(winS, matView, matProjection, vp);
		resD = glm::unProject(winD, matView, matProjection, vp);
	}

	dirVect.x = resD.x - resS.x;
	dirVect.y = resD.y - resS.y;
	dirVect.z = resD.z - resS.z;

	outNearPos.x = resS.x;
	outNearPos.y = resS.y;
	outNearPos.z = resS.z;

	dirVect.Normalize();
}

mesh* GLSurface::PickMesh(int ScreenX, int ScreenY) {
	Vector3 o;
	Vector3 d;
	float curd = FLT_MAX;
	mesh* pickedMesh = nullptr;

	std::vector<IntersectResult> results;

	for (auto &m : meshes) {
		results.clear();
		if (!m->bVisible || !m->bvh)
			continue;

		GetPickRay(ScreenX, ScreenY, m, d, o);

		if (m->bvh->IntersectRay(o, d, &results)) {
			if (results[0].HitDistance < curd) {
				pickedMesh = m;
				curd = results[0].HitDistance;
			}
		}
	}

	return pickedMesh;
}

bool GLSurface::CollideMeshes(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, bool mirrored, mesh** hitMesh, bool allMeshes, int* outFacet) {
	if (activeMeshes.empty())
		return false;

	bool collided = false;
	std::unordered_map<mesh*, float> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!m->bvh)
			continue;

		if (!allMeshes && m != selectedMesh)
			continue;

		Vector3 d;
		Vector3 o;

		GetPickRay(ScreenX, ScreenY, m, d, o);
		if (mirrored) {
			d.x *= -1.0f;
			o.x *= -1.0f;
		}

		std::vector<IntersectResult> results;
		if (m->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				if (!m->bVisible)
					continue;

				collided = true;

				size_t min_i = 0;
				float minDist = results[0].HitDistance;
				for (size_t i = 1; i < results.size(); i++) {
					if (results[i].HitDistance < minDist) {
						minDist = results[i].HitDistance;
						min_i = i;
					}
				}

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

					m->tris[results[min_i].HitFacet].trinormal(m->verts.get(), &outNormal);

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

bool GLSurface::CollideOverlay(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh, int* outFacet) {
	bool collided = false;
	std::unordered_map<mesh*, float> allHitDistances;
	for (auto &ov : overlays) {
		std::vector<IntersectResult> results;
		if (!ov->bvh)
			continue;

		Vector3 o;
		Vector3 d;
		GetPickRay(ScreenX, ScreenY, ov, d, o);

		if (ov->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				if (!ov->bVisible)
					continue;

				collided = true;

				size_t min_i = 0;
				float minDist = results[0].HitDistance;
				for (size_t i = 1; i < results.size(); i++) {
					if (results[i].HitDistance < minDist) {
						minDist = results[i].HitDistance;
						min_i = i;
					}
				}

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

					ov->tris[results[min_i].HitFacet].trinormal(ov->verts.get(), &outNormal);
				}
			}
		}
	}

	return collided;
}

bool GLSurface::CollidePlane(int ScreenX, int ScreenY, Vector3& outOrigin, const Vector3& inPlaneNormal, float inPlaneDist) {
	Vector3 o;
	Vector3 d;
	GetPickRay(ScreenX, ScreenY, nullptr, d, o);

	float den = inPlaneNormal.dot(d);
	if (fabs(den) < .00001)
		return false;

	float t = -(inPlaneNormal.dot(o) - inPlaneDist) / den;
	outOrigin = o + d*t;

	return true;
}

bool GLSurface::UpdateCursor(int ScreenX, int ScreenY, bool allMeshes, std::string* hitMeshName, int* outHoverPoint, Vector3* outHoverColor, float* outHoverAlpha, Edge* outHoverEdge, Vector3* outHoverCoord) {
	bool collided = false;
	if (activeMeshes.empty())
		return collided;

	if (outHoverPoint)
		(*outHoverPoint) = -1;
	if (outHoverColor)
		(*outHoverColor) = Vector3(1.0f, 1.0f, 1.0f);
	if (outHoverCoord)
		outHoverCoord->Zero();

	std::unordered_map<mesh*, Vector3> allHitDistances;
	for (auto &m : activeMeshes) {
		if (!allMeshes && m != selectedMesh)
			continue;

		Vector3 o;
		Vector3 d;
		GetPickRay(ScreenX, ScreenY, m, d, o);

		std::vector<IntersectResult> results;
		if (m->bvh && m->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				if (!m->bVisible)
					continue;

				collided = true;

				size_t min_i = 0;
				float minDist = results[0].HitDistance;
				for (size_t i = 1; i < results.size(); i++) {
					if (results[i].HitDistance < minDist) {
						minDist = results[i].HitDistance;
						min_i = i;
					}
				}

				Vector3 origin = results[min_i].HitCoord;

				Triangle t = m->tris[results[min_i].HitFacet];

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
					const int dec = 5;

					if (outHoverPoint)
						(*outHoverPoint) = pointid;

					if (outHoverCoord)
						(*outHoverCoord) = origin;

					if (outHoverColor) {
						(*outHoverColor).x = std::floor(m->vcolors[pointid].x * std::pow(10, dec) + 0.5f) / std::pow(10, dec);
						(*outHoverColor).y = std::floor(m->vcolors[pointid].y * std::pow(10, dec) + 0.5f) / std::pow(10, dec);
						(*outHoverColor).z = std::floor(m->vcolors[pointid].z * std::pow(10, dec) + 0.5f) / std::pow(10, dec);
					}

					if (outHoverAlpha)
						(*outHoverAlpha) = std::floor(m->valpha[pointid] * std::pow(10, dec) + 0.5f) / std::pow(10, dec);

					if (hitMeshName)
						(*hitMeshName) = m->shapeName;

					Edge closestEdge = t.ClosestEdge(m->verts.get(), origin);
					if (outHoverEdge)
						*outHoverEdge = closestEdge;

					Vector3 morigin = mesh::ApplyMatrix4(m->matModel, origin);

					Vector3 norm;
					m->tris[results[min_i].HitFacet].trinormal(m->verts.get(), &norm);

					AddVisCircle(morigin, norm, cursorSize, "cursormesh");

					Vector3 mhilitepoint = mesh::ApplyMatrix4(m->matModel, hilitepoint);
					AddVisPoint(mhilitepoint, "pointhilite");
					AddVisPoint(morigin, "cursorcenter")->color = Vector3(1.0f, 0.0f, 0.0f);

					Vector3 mep1 = mesh::ApplyMatrix4(m->matModel, m->verts[closestEdge.p1]);
					Vector3 mep2 = mesh::ApplyMatrix4(m->matModel, m->verts[closestEdge.p2]);
					AddVisSeg(mep1, mep2, "seghilite");
				}

				allHitDistances[m] = hilitepoint;
			}
		}
	}

	ShowCursor(collided);
	return collided;
}

bool GLSurface::GetCursorVertex(int ScreenX, int ScreenY, int* outIndex, mesh* hitMesh) {
	if (!hitMesh && activeMeshes.empty())
		return false;

	if (outIndex)
		(*outIndex) = -1;

	std::vector<mesh*> hitMeshes;
	if (hitMesh)
		hitMeshes.push_back(hitMesh);
	else
		hitMeshes = activeMeshes;

	for (auto &m : hitMeshes) {
		Vector3 o;
		Vector3 d;
		GetPickRay(ScreenX, ScreenY, m, d, o);

		std::vector<IntersectResult> results;
		if (m->bvh && m->bvh->IntersectRay(o, d, &results)) {
			if (results.size() > 0) {
				size_t min_i = 0;
				float minDist = results[0].HitDistance;
				for (size_t i = 1; i < results.size(); i++) {
					if (results[i].HitDistance < minDist) {
						minDist = results[i].HitDistance;
						min_i = i;
					}
				}

				Vector3 origin = results[min_i].HitCoord;

				Triangle t = m->tris[results[min_i].HitFacet];

				float closestdist = fabs(m->verts[t.p1].DistanceTo(origin));
				float nextdist = fabs(m->verts[t.p2].DistanceTo(origin));
				int pointid = t.p1;

				if (nextdist < closestdist) {
					closestdist = nextdist;
					pointid = t.p2;
				}

				nextdist = fabs(m->verts[t.p3].DistanceTo(origin));

				if (nextdist < closestdist)
					pointid = t.p3;

				if (outIndex)
					(*outIndex) = pointid;

				return true;
			}
		}
	}

	return false;
}

void GLSurface::ShowCursor(bool show) {
	SetOverlayVisibility("cursormesh", show && (cursorType & CircleCursor));
	SetOverlayVisibility("pointhilite", show && (cursorType & PointCursor));
	SetOverlayVisibility("cursorcenter", show && (cursorType & CenterCursor));
	SetOverlayVisibility("seghilite", show && (cursorType & SegCursor));
}

void GLSurface::HidePointCursor() {
	SetOverlayVisibility("pointhilite", false);
}

void GLSurface::HideSegCursor() {
	SetOverlayVisibility("seghilite", false);
}

void GLSurface::SetSize(uint32_t w, uint32_t h) {
	if (!SetContext())
		return;

	glViewport(0, 0, w, h);
	vpW = w;
	vpH = h;
}

void GLSurface::GetSize(uint32_t & w, uint32_t & h) {
	w = vpW;
	h = vpH;
}

void GLSurface::UpdateProjection() {
	float aspect = (float)vpW / (float)vpH;
	if (perspective)
		matProjection = glm::perspective(glm::radians(mFov), aspect, 0.1f, 1000.0f);
	else
		matProjection = glm::ortho((camPos.z + camOffset.z) / 2.0f * aspect, (-camPos.z + camOffset.z) / 2.0f * aspect, (camPos.z + camOffset.z) / 2.0f, (-camPos.z + camOffset.z) / 2.0f, 0.1f, 1000.0f);

	auto mat = glm::identity<glm::mat4x4>();
	matView = glm::translate(mat, glm::vec3(camPos.x, camPos.y, camPos.z));
	matView = glm::translate(matView, glm::vec3(camRotOffset.x, camRotOffset.y, camRotOffset.z));
	matView = glm::rotate(matView, glm::radians(camRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matView = glm::rotate(matView, glm::radians(camRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
	matView = glm::translate(matView, -glm::vec3(camRotOffset.x, camRotOffset.y, camRotOffset.z));
	matView = glm::translate(matView, glm::vec3(camOffset.x, camOffset.y, camOffset.z));
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
		renderShader->BindTextures(0, false, false, false, false);
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

	// Set up orthographic projection and identity view 

	matProjection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -100.0f, 100.0f);
	//matProjection = glm::ortho((camPos.z + camOffset.z) / 2.0f * aspect, (-camPos.z + camOffset.z) / 2.0f * aspect, (camPos.z + camOffset.z) / 2.0f, (-camPos.z + camOffset.z) / 2.0f, 0.1f, 100.0f);
	matView = glm::identity<glm::mat4x4>();

	mesh* m = nullptr;
	bool oldDS;
	GLMaterial* oldmat;

	// Render regular meshes only
	for (size_t i = 0; i < meshes.size(); i++) {
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

bool GLSurface::SetContext() {
	if (canvas && context) {
		canvas->SetCurrent(*context);
		return true;
	}

	return false;
}

void GLSurface::RenderOneFrame() {
	if (!canvas)
		return;

	canvas->SetCurrent(*context);

	glClearColor(colorBackground.x, colorBackground.y, colorBackground.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	UpdateProjection();

	// Render regular meshes
	for (auto &m : meshes) {
		if (!m->HasAlphaBlend() && m->bVisible && (m->nTris != 0 || m->nEdges != 0))
			RenderMesh(m);
	}

	// Render meshes with alpha blending only
	for (auto &m : meshes) {
		if (m->HasAlphaBlend() && m->bVisible && (m->nTris != 0 || m->nEdges != 0)) {
			glCullFace(GL_FRONT);
			RenderMesh(m);
			glCullFace(GL_BACK);
			RenderMesh(m);
		}
	}

	// Render overlays on top
	std::vector<mesh*> renderOverlays(overlays);
	std::sort(renderOverlays.begin(), renderOverlays.end(), SortOverlaysLayer());

	uint32_t lastOverlayLayer = static_cast<uint32_t>(-1);
	for (auto &o : renderOverlays) {
		if (o->bVisible) {
			if (o->overlayLayer != lastOverlayLayer)
				glClear(GL_DEPTH_BUFFER_BIT);

			lastOverlayLayer = o->overlayLayer;
			RenderMesh(o);
		}
	}

	canvas->SwapBuffers();
	return;
}

void GLSurface::RenderMesh(mesh* m) {
	m->UpdateBuffers();

	if (!m->genBuffers || !m->material)
		return;

	GLShader& shader = m->material->GetShader();
	if (!shader.Begin())
		return;

	if (!m->HasAlphaBlend()) {
		if (!m->doublesided)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);

		glCullFace(m->cullMode);
	}
	else
		glEnable(GL_CULL_FACE);

	shader.SetAlphaProperties(m->alphaFlags, m->alphaThreshold / 255.0f, m->prop.alpha);
	shader.SetMatrixProjection(matProjection);
	shader.SetMatrixModelView(matView, m->matModel);
	shader.SetColor(m->color);
	shader.SetSubColor(Vector3(1.0f, 1.0f, 1.0f));
	shader.SetModelSpace(m->modelSpace);
	shader.SetSpecularEnabled(m->specular);
	shader.SetEmissive(m->emissive);
	shader.SetBacklightEnabled(m->backlight);
	shader.SetRimlightEnabled(m->rimlight);
	shader.SetSoftlightEnabled(m->softlight);
	shader.SetGlowmapEnabled(m->glowmap);
	shader.SetGreyscaleColorEnabled(m->greyscaleColor);
	shader.SetLightingEnabled(bLighting);
	shader.SetWireframeEnabled(false);
	shader.SetPointsEnabled(false);
	shader.SetNormalMapEnabled(false);
	shader.SetAlphaMaskEnabled(false);
	shader.SetCubemapEnabled(m->cubemap);
	shader.SetEnvMaskEnabled(false);
	shader.SetProperties(m->prop);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);

	glBindVertexArray(m->vao);

	if (m->rendermode == RenderMode::Normal || m->rendermode == RenderMode::LitWire || m->rendermode == RenderMode::UnlitSolid) {
		shader.SetFrontalLight(frontalLight);
		shader.SetDirectionalLight(directionalLight0, 0);
		shader.SetDirectionalLight(directionalLight1, 1);
		shader.SetDirectionalLight(directionalLight2, 2);
		shader.SetAmbientLight(ambientLight);

		if (m->rendermode == RenderMode::LitWire) {
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		if (m->rendermode == RenderMode::UnlitSolid)
			shader.SetLightingEnabled(false);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);			// Positions

		if (m->norms) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[1]);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Normals
		}

		if (m->tangents) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[2]);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Tangents
		}

		if (m->bitangents) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[3]);
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Bitangents
		}

		if (m->vcolors) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[4]);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Colors
		}

		if (m->valpha) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[5]);
			glEnableVertexAttribArray(5);
			glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Alpha
		}

		if (bTextured && m->textured && m->texcoord) {
			glBindBuffer(GL_ARRAY_BUFFER, m->vbo[6]);
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);		// Texture Coordinates

			m->material->BindTextures(largestAF, m->cubemap, m->glowmap, m->backlightMap, m->rimlight || m->softlight);
		}

		// Offset triangles so that points can be visible
		glEnable(GL_POLYGON_OFFSET_FILL);
		if (m->bShowPoints)
			glPolygonOffset(1.0f, 1.0f);
		else
			glPolygonOffset(0.03f, 0.03f);

		GLuint subMeshesSize = 0;
		if (!m->subMeshes.empty()) {
			auto& lastSubMesh = m->subMeshes.back();
			subMeshesSize = lastSubMesh.first + lastSubMesh.second;
		}

		// Render full mesh or remainder of it
		glDrawElements(GL_TRIANGLES, (m->nTris - subMeshesSize) * 3, GL_UNSIGNED_SHORT, (GLvoid*)(subMeshesSize * 3 * sizeof(GLushort)));

		// Render sub meshes
		for (size_t s = 0; s < m->subMeshes.size(); ++s) {
			GLuint subIndex = m->subMeshes[s].first;
			GLuint subSize = m->subMeshes[s].second;
			Vector3 subColor = m->color;

			if (!m->subMeshesColor.empty())
				subColor = m->subMeshesColor[s];

			shader.SetSubColor(subColor);
			glDrawElements(GL_TRIANGLES, subSize * 3, GL_UNSIGNED_SHORT, (GLvoid*)(subIndex * 3 * sizeof(GLushort)));
		}

		glDisable(GL_POLYGON_OFFSET_FILL);

		// Render wireframe (full mesh or remainder of it)
		if (bWireframe && m->rendermode == RenderMode::Normal) {
			shader.SetWireframeEnabled(true);
			shader.SetColor(colorWire);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, (m->nTris - subMeshesSize) * 3, GL_UNSIGNED_SHORT, (GLvoid*)(subMeshesSize * 3 * sizeof(GLushort)));

			// Render wireframes for sub meshes
			for (size_t s = 0; s < m->subMeshes.size(); ++s) {
				GLuint subIndex = m->subMeshes[s].first;
				GLuint subSize = m->subMeshes[s].second;
				glDrawElements(GL_TRIANGLES, subSize * 3, GL_UNSIGNED_SHORT, (GLvoid*)(subIndex * 3 * sizeof(GLushort)));
			}
		}

		if (bTextured && m->textured && m->texcoord)
			glDisableVertexAttribArray(6);

		// Render points
		if (m->bShowPoints && m->vcolors) {
			glDisable(GL_CULL_FACE);
			shader.SetLightingEnabled(false);

			shader.SetPointsEnabled(true);
			glDrawArrays(GL_POINTS, 0, m->nVerts);
		}

		glDisableVertexAttribArray(5);
		glDisableVertexAttribArray(4);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);
	}
	else if (m->rendermode == RenderMode::UnlitWire || m->rendermode == RenderMode::UnlitWireDepth) {
		glDisable(GL_CULL_FACE);

		if (m->rendermode != RenderMode::UnlitWireDepth)
			glDisable(GL_DEPTH_TEST);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_LINES, m->nEdges * 2, GL_UNSIGNED_SHORT, (GLvoid*)0);

		glDisableVertexAttribArray(0);
	}
	else if (m->rendermode == RenderMode::UnlitPoints || m->rendermode == RenderMode::UnlitPointsDepth) {
		glDisable(GL_CULL_FACE);

		if (m->rendermode != RenderMode::UnlitPointsDepth)
			glDisable(GL_DEPTH_TEST);

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

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
		GLShader& shader = m->material->GetShader();
		shader.ShowTexture(bTextured);
		shader.ShowLighting(bLighting);
		shader.ShowMask(bMaskVisible && m->vcolors);
		shader.ShowWeight(bWeightColors && m->vcolors);
		shader.ShowVertexColors(bVertexColors && m->vcolors && m->vertexColors);
		shader.ShowVertexAlpha(bVertexColors && m->valpha && m->vertexColors && m->vertexAlpha);
		shader.SetProperties(m->prop);
	}
}

mesh* GLSurface::ReloadMeshFromNif(NifFile* nif, std::string shapeName) {
	DeleteMesh(shapeName);
	return AddMeshFromNif(nif, shapeName);
}

mesh* GLSurface::AddMeshFromNif(NifFile* nif, const std::string& shapeName, Vector3* color) {
	auto shape = nif->FindBlockByName<NiShape>(shapeName);
	if (!shape)
		return nullptr;

	std::vector<Vector3> nifVerts;
	nif->GetVertsForShape(shape, nifVerts);

	std::vector<Triangle> nifTris;
	shape->GetTriangles(nifTris);

	const std::vector<Vector3>* nifNorms = nif->GetNormalsForShape(shape);
	const std::vector<Vector2>* nifUvs = nif->GetUvsForShape(shape);

	mesh* m = new mesh();

	if (!shape->IsSkinned()) {
		// Calculate transform from shape's CS to global CS
		MatTransform ttg = shape->GetTransformToParent();
		NiNode* parent = nif->GetParentNode(shape);
		while (parent) {
			ttg = parent->GetTransformToParent().ComposeTransforms(ttg);
			parent = nif->GetParentNode(parent);
		}

		ttg.translation = mesh::VecToMeshCoords(ttg.translation);

		// Convert ttg to a glm::mat4x4
		m->matModel = mesh::TransformToMatrix4(ttg);
	}
	else {
		// Skinned meshes
		m->matModel = glm::identity<glm::mat4x4>();

		MatTransform xformGlobalToSkin;
		if (nif->CalcShapeTransformGlobalToSkin(shape, xformGlobalToSkin)) {
			xformGlobalToSkin.translation = mesh::VecToMeshCoords(xformGlobalToSkin.translation);
			m->matModel = glm::inverse(mesh::TransformToMatrix4(xformGlobalToSkin));
		}
	}

	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		m->doublesided = shader->IsDoubleSided();
		m->modelSpace = shader->IsModelSpace();
		m->emissive = shader->IsEmissive();
		m->specular = shader->HasSpecular();
		m->vertexColors = shader->HasVertexColors();
		m->vertexAlpha = shader->HasVertexAlpha();
		m->glowmap = shader->HasGlowmap();
		m->greyscaleColor = shader->HasGreyscaleColor();
		m->cubemap = shader->HasEnvironmentMapping();

		if (nif->GetHeader().GetVersion().Stream() < 130) {
			m->backlight = shader->HasBacklight();
			m->backlightMap = m->backlight;			// Dedicated map pre-130
			m->rimlight = shader->HasRimlight();
			m->softlight = shader->HasSoftlight();
			m->prop.rimlightPower = shader->GetRimlightPower();
			m->prop.softlighting = shader->GetSoftlight();
		}
		else {
			m->backlight = (shader->GetBacklightPower() > 0.0);
			m->softlight = (shader->GetSubsurfaceRolloff() > 0.0);
			m->prop.subsurfaceRolloff = shader->GetSubsurfaceRolloff();
		}

		m->prop.uvOffset = shader->GetUVOffset();
		m->prop.uvScale = shader->GetUVScale();
		m->prop.specularColor = shader->GetSpecularColor();
		m->prop.specularStrength = shader->GetSpecularStrength();
		m->prop.shininess = shader->GetGlossiness();
		m->prop.envReflection = shader->GetEnvironmentMapScale();
		m->prop.backlightPower = shader->GetBacklightPower();
		m->prop.paletteScale = shader->GetGrayscaleToPaletteScale();
		m->prop.fresnelPower = shader->GetFresnelPower();

		Color4 emissiveColor = shader->GetEmissiveColor();
		m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		m->prop.emissiveMultiple = shader->GetEmissiveMultiple();

		m->prop.alpha = shader->GetAlpha();
	}

	NiMaterialProperty* material = nif->GetMaterialProperty(shape);
	if (material) {
		m->emissive = material->IsEmissive();

		m->prop.specularColor = material->GetSpecularColor();
		m->prop.shininess = material->GetGlossiness();

		Color4 emissiveColor = material->GetEmissiveColor();
		m->prop.emissiveColor = Vector3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		m->prop.emissiveMultiple = material->GetEmissiveMultiple();

		m->prop.alpha = material->GetAlpha();
	}

	NiStencilProperty* stencil = nif->GetStencilProperty(shape);
	if (stencil) {
		int drawMode = (stencil->flags & DRAW_MASK) >> DRAW_POS;

		switch (drawMode) {
		case DRAW_CW:
			m->doublesided = false;
			m->cullMode = GL_FRONT;
			break;
		case DRAW_BOTH:
			m->doublesided = true;
			m->cullMode = GL_BACK;
			break;
		case DRAW_CCW:
		default:
			m->doublesided = false;
			m->cullMode = GL_BACK;
			break;
		}
	}

	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
	if (alphaProp) {
		m->alphaFlags = alphaProp->flags;
		m->alphaThreshold = alphaProp->threshold;
	}

	m->nVerts = nifVerts.size();
	m->nTris = nifTris.size();

	if (m->nVerts > 0) {
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->norms = std::make_unique<Vector3[]>(m->nVerts);
		m->tangents = std::make_unique<Vector3[]>(m->nVerts);
		m->bitangents = std::make_unique<Vector3[]>(m->nVerts);
		m->vcolors = std::make_unique<Vector3[]>(m->nVerts);
		m->valpha = std::make_unique<float[]>(m->nVerts);
		m->texcoord = std::make_unique<Vector2[]>(m->nVerts);
	}

	if (m->nTris > 0)
		m->tris = std::make_unique<Triangle[]>(m->nTris);

	m->shapeName = shapeName;

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

		// Calc weldVerts and normals
		m->SmoothNormals();
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

		for (auto &extraDataRef : shape->extraDataRefs) {
			auto integersExtraData = nif->GetHeader().GetBlock<NiIntegersExtraData>(extraDataRef);
			if (integersExtraData && integersExtraData->name == "LOCKEDNORM")
				for (auto& i : integersExtraData->integersData)
					m->lockedNormalIndices.insert(i);
		}

		// Virtually weld verts across UV seams
		m->CalcWeldVerts();
	}

	m->CalcTangentSpace();
	m->CreateBVH();

	if (color)
		m->color = (*color);

	AddMesh(m);

	if (!color) {
		int i = 0;
		auto meshesFiltered = GetMeshesFiltered();
		for (auto &nm : meshesFiltered) {
			float c = 0.25f + (0.25f / meshesFiltered.size() * i);
			nm->color = Vector3(c, c, c);
			i++;
		}
	}

	return m;
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

	if (!SetContext())
		return nullptr;

	m = new mesh();

	m->nVerts = 1;
	m->verts = std::make_unique<Vector3[]>(1);
	m->verts[0] = p;
	m->nEdges = 0;
	m->nTris = 0;

	m->rendermode = RenderMode::UnlitPoints;
	m->shapeName = name;
	m->color = Vector3(0.0f, 1.0f, 1.0f);
	m->material = GetPrimitiveMaterial();
	m->CreateBuffers();

	AddOverlay(m);
	return m;
}

mesh* GLSurface::AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const std::string& name) {
	Matrix4 rotMat;
	rotMat.Align(Vector3(0.0f, 0.0f, 1.0f), normal);
	rotMat.Translate(center);

	float rStep = DEG2RAD * 5.0f;

	mesh* m = GetOverlay(name);
	if (m) {
		float i = 0.0f;
		for (int j = 0; j < m->nVerts; j++) {
			m->verts[j].x = sin(i) * radius;
			m->verts[j].y = cos(i) * radius;
			m->verts[j].z = 0;
			i += rStep;
			m->verts[j] = rotMat * m->verts[j];
		}

		m->QueueUpdate(mesh::UpdateType::Position);
		return m;
	}

	if (!SetContext())
		return nullptr;

	m = new mesh();

	m->nVerts = 360 / 5;
	m->nEdges = m->nVerts;

	m->verts = std::make_unique<Vector3[]>(m->nVerts);
	m->edges = std::make_unique<Edge[]>(m->nEdges);

	m->shapeName = name;
	m->color = Vector3(1.0f, 0.0f, 0.0f);
	m->rendermode = RenderMode::UnlitWire;
	m->material = GetPrimitiveMaterial();

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

	AddOverlay(m);
	return m;
}

mesh* GLSurface::AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const std::string& name) {
	if (!SetContext())
		return nullptr;

	const int nRingSegments = 36;
	const int nRingSteps = 4;

	mesh* m = GetOverlay(name);
	if (!m) {
		m = new mesh();

		m->nVerts = nRingSegments * nRingSteps;
		m->nTris = m->nVerts * 2;
		m->nEdges = 0;
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->tris = std::make_unique<Triangle[]>(m->nTris);

		m->shapeName = name;
		m->rendermode = RenderMode::UnlitSolid;
		m->material = GetPrimitiveMaterial();

		AddOverlay(m);
	}

	m->color = color;

	Matrix4 xf1;
	xf1.Align(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f));
	xf1.Translate(Vector3(0.0f, holeRadius, 0.0f));

	Matrix4 xf3;
	xf3.Align(Vector3(0.0f, 0.0f, 1.0f), normal);
	xf3.Translate(center);

	float rStep = DEG2RAD * (360.0f / nRingSteps);
	float i = 0.0f;
	Vector3 loftVerts[nRingSteps];
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
	for (int vi = 0; vi < m->nVerts;) {
		xf2.Identity();
		xf2.Rotate(rStep * seg, 0.0f, 0.0f, 1.0f);
		for (int r = 0; r < nRingSteps; r++) {
			m->verts[vi] = xf3 * xf2 * loftVerts[r];
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
	return m;
}


mesh* GLSurface::AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const std::string& name) {
	if (!SetContext())
		return nullptr;

	const int nRingVerts = 36;
	Matrix4 rotMat;

	mesh* m = GetOverlay(name);
	if (!m) {
		m = new mesh();
		m->nVerts = nRingVerts * 3 + 1; // base ring, inner point ring, outer point ring, and the tip
		m->nTris = nRingVerts * 5;
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->tris = std::make_unique<Triangle[]>(m->nTris);

		m->shapeName = name;
		m->rendermode = RenderMode::UnlitSolid;
		m->material = GetPrimitiveMaterial();

		AddOverlay(m);
	}

	m->nEdges = 0;
	m->color = color;

	rotMat.Align(Vector3(0.0f, 0.0f, 1.0f), direction);
	rotMat.Translate(origin);

	float rStep = DEG2RAD * (360.0f / nRingVerts);
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
		else {
			p2 = 0;
			p4 = nRingVerts;
			p6 = nRingVerts * 2;
		}
		m->tris[t].p1 = p1;
		m->tris[t].p2 = p2;
		m->tris[t].p3 = p3;
		m->tris[t + 1].p1 = p3;
		m->tris[t + 1].p2 = p2;
		m->tris[t + 1].p3 = p4;
		m->tris[t + 2].p1 = p3;
		m->tris[t + 2].p2 = p4;
		m->tris[t + 2].p3 = p5;
		m->tris[t + 3].p1 = p5;
		m->tris[t + 3].p2 = p4;
		m->tris[t + 3].p3 = p6;
		m->tris[t + 4].p1 = p5;
		m->tris[t + 4].p2 = p6;
		m->tris[t + 4].p3 = p7;
		t += 5;
	}

	m->CreateBuffers();
	return m;
}

mesh* GLSurface::AddVis3dCube(const Vector3& center, const Vector3& normal, float radius, const Vector3& color, const std::string& name) {
	if (!SetContext())
		return nullptr;

	const int nCubeVerts = 8;
	const int nCubeTris = 12;

	mesh* m = GetOverlay(name);
	if (!m) {
		m = new mesh();

		m->nVerts = nCubeVerts;
		m->nTris = nCubeTris;
		m->nEdges = 0;
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->tris = std::make_unique<Triangle[]>(m->nTris);

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

		m->shapeName = name;
		m->rendermode = RenderMode::UnlitSolid;
		m->material = GetPrimitiveMaterial();

		AddOverlay(m);
	}

	m->color = color;

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

	m->CreateBuffers();
	return m;
}

mesh* GLSurface::AddVisPlane(const Matrix4& mat, const Vector2& size, float uvScale, float uvOffset, const std::string& name, const Vector3* color, const bool asMesh) {
	mesh* m = nullptr;

	if (!name.empty()) {
		if (asMesh)
			m = GetMesh(name);
		else
			m = GetOverlay(name);
	}

	if (!SetContext())
		return nullptr;

	if (!m) {
		m = new mesh();

		m->nVerts = 4;
		m->nTris = 2;
		m->verts = std::make_unique<Vector3[]>(m->nVerts);
		m->texcoord = std::make_unique<Vector2[]>(m->nVerts);
		m->tris = std::make_unique<Triangle[]>(m->nTris);

		m->tris[0] = Triangle(0, 1, 2);
		m->tris[1] = Triangle(2, 3, 0);

		m->shapeName = name;
		m->rendermode = RenderMode::UnlitSolid;
		m->material = GetPrimitiveMaterial();

		if (asMesh)
			AddMesh(m);
		else
			AddOverlay(m);
	}

	if (color)
		m->color = *color;

	m->verts[0] = mat * Vector3(-size.u / 2.0f, -size.v / 2.0f, 0.0f);
	m->verts[1] = mat * Vector3(-size.u / 2.0f, size.v / 2.0f, 0.0f);
	m->verts[2] = mat * Vector3(size.u / 2.0f, size.v / 2.0f, 0.0f);
	m->verts[3] = mat * Vector3(size.u / 2.0f, -size.v / 2.0f, 0.0f);

	m->texcoord[0] = Vector2(uvOffset, uvScale + uvOffset);
	m->texcoord[1] = Vector2(uvOffset, uvOffset);
	m->texcoord[2] = Vector2(uvScale + uvOffset, uvOffset);
	m->texcoord[3] = Vector2(uvScale + uvOffset, uvScale + uvOffset);

	m->textured = true;

	m->CreateBuffers();
	return m;
}

mesh* GLSurface::AddVisSeg(const Vector3& p1, const Vector3& p2, const std::string& name, const bool asMesh) {
	mesh* m = nullptr;

	if (!name.empty()) {
		if (asMesh)
			m = GetMesh(name);
		else
			m = GetOverlay(name);

		if (m) {
			m->verts[0] = p1;
			m->verts[1] = p2;
			m->bVisible = true;
			m->QueueUpdate(mesh::UpdateType::Position);
			return m;
		}
	}

	if (!SetContext())
		return nullptr;

	m = new mesh();

	m->nVerts = 2;
	m->nEdges = 1;
	m->verts = std::make_unique<Vector3[]>(2);
	m->edges = std::make_unique<Edge[]>(1);
	m->verts[0] = p1;
	m->verts[1] = p2;
	m->edges[0].p1 = 0;
	m->edges[0].p2 = 1;

	m->shapeName = name;
	m->color = Vector3(0.0f, 1.0f, 1.0f);
	m->material = GetPrimitiveMaterial();
	m->CreateBuffers();

	if (asMesh) {
		m->rendermode = RenderMode::UnlitWireDepth;
		AddMesh(m);
	}
	else {
		m->rendermode = RenderMode::UnlitWire;
		AddOverlay(m);
	}

	return m;
}

mesh* GLSurface::AddVisSeamEdges(const mesh* refMesh, bool asMesh) {
	if (!refMesh)
		return nullptr;

	std::string name = refMesh->shapeName + "_seamedges";

	if (!SetContext())
		return nullptr;

	if (!refMesh->bGotWeldVerts || refMesh->weldVerts.empty())
		return nullptr;

	std::vector<Vector3> verts(refMesh->nVerts);
	for (int i = 0; i < refMesh->nVerts; i++)
		verts[i] = refMesh->verts[i];

	std::vector<Edge> edges;

	// Find edges of welded vertices
	for (int e = 0; e < refMesh->nEdges; e++) {
		auto& edge = refMesh->edges[e];
		auto p1It = refMesh->weldVerts.find(edge.p1);
		auto p2It = refMesh->weldVerts.find(edge.p2);
		if (p1It != refMesh->weldVerts.end() && p2It != refMesh->weldVerts.end()) {
			// Both points of the edge are welded
			edges.push_back(edge);
		}
	}

	// Erase edges that aren't outer seam edges
	for (auto it = edges.begin(); it != edges.end(); ) {
		size_t p1Count = 0;
		size_t p2Count = 0;
		auto& edge = *it;
		for (const auto& otherEdge : edges) {
			if (otherEdge.p1 == edge.p1 || otherEdge.p2 == edge.p1)
				p1Count++;
			if (otherEdge.p2 == edge.p2 || otherEdge.p1 == edge.p2)
				p2Count++;
		}

		// If both points occur three or more times, the edge is not an outer edge
		if (p1Count >= 3 && p2Count >= 3)
			it = edges.erase(it);
		else
			it++;
	}

	auto m = new mesh();
	m->nVerts = static_cast<int>(verts.size());
	m->nEdges = static_cast<int>(edges.size());

	m->verts = std::make_unique<Vector3[]>(m->nVerts);
	m->edges = std::make_unique<Edge[]>(m->nEdges);

	for (size_t v = 0; v < verts.size(); v++)
		m->verts[v] = verts[v];

	for (size_t e = 0; e < edges.size(); e++)
		m->edges[e] = edges[e];

	m->shapeName = name;
	m->color = Vector3(1.0f, 1.0f, 0.0f);
	m->material = GetPrimitiveMaterial();
	m->CreateBuffers();

	if (asMesh) {
		m->rendermode = RenderMode::UnlitWireDepth;
		AddMesh(m);
	}
	else {
		m->rendermode = RenderMode::UnlitWire;
		AddOverlay(m);
	}

	return m;
}

void GLSurface::Update(const std::string& shapeName, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs, std::set<int>* changed) {
	mesh* m = GetMesh(shapeName);
	if (!m)
		return;

	Update(m, vertices, uvs, changed);
}

void GLSurface::Update(mesh* m, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs, std::set<int>* changed) {
	if (m->nVerts != static_cast<int>(vertices->size()))
		return;

	int uvSize = 0;
	if (uvs)
		uvSize = uvs->size();

	Vector3 old;
	for (int i = 0; i < m->nVerts; i++) {
		if (changed)
			old = m->verts[i];

		m->verts[i].x = (*vertices)[i].x / -10.0f;
		m->verts[i].z = (*vertices)[i].y / 10.0f;
		m->verts[i].y = (*vertices)[i].z / 10.0f;

		if (uvSize > i)
			m->texcoord[i] = (*uvs)[i];

		if (changed && old != m->verts[i])
			(*changed).insert(i);
	}

	m->QueueUpdate(mesh::UpdateType::Position);
	if (uvs)
		m->QueueUpdate(mesh::UpdateType::TextureCoordinates);
}

void GLSurface::RecalculateMeshBVH(const std::string& shapeName) {
	mesh* m = GetMesh(shapeName);
	if (!m)
		return;

	m->CreateBVH();
}

bool GLSurface::SetMeshVisibility(const std::string& name, bool visible) {
	bool changed = false;

	mesh* m = GetMesh(name);
	if (!m)
		return changed;

	if (m->bVisible != visible)
		changed = true;

	m->bVisible = visible;
	return changed;
}

void GLSurface::SetOverlayVisibility(const std::string& name, bool visible) {
	mesh* m = GetOverlay(name);
	if (m)
		m->bVisible = visible;
}

void GLSurface::SetActiveMeshes(const std::vector<std::string>& shapeNames) {
	activeMeshes.clear();

	for (auto &s : shapeNames) {
		mesh* m = GetMesh(s);
		if (m)
			activeMeshes.push_back(m);
	}
}

void GLSurface::SetSelectedMesh(const std::string& shapeName) {
	selectedMesh = GetMesh(shapeName);
}

RenderMode GLSurface::SetMeshRenderMode(const std::string& name, RenderMode mode) {
	mesh* m = GetMesh(name);
	if (!m)
		return RenderMode::Normal;

	RenderMode r = m->rendermode;
	m->rendermode = mode;
	return r;
}

GLMaterial* GLSurface::AddMaterial(const std::vector<std::string>& textureFiles, const std::string& vShaderFile, const std::string& fShaderFile, const bool reloadTextures) {
	if (!SetContext())
		return nullptr;

	GLMaterial* mat = resLoader.AddMaterial(textureFiles, vShaderFile, fShaderFile, reloadTextures);
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
		if (!SetContext())
			return nullptr;

		primitiveMat = new GLMaterial(Config["AppDir"] + "/res/shaders/primitive.vert", Config["AppDir"] + "/res/shaders/primitive.frag");

		std::string shaderError;
		if (primitiveMat->GetShader().GetError(&shaderError)) {
			wxLogError(wxString(shaderError));
			wxMessageBox(shaderError, _("OpenGL Error"), wxICON_ERROR);
		}
	}

	return primitiveMat;
}
