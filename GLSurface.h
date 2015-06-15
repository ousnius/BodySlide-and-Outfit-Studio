#pragma once

#include "GLShader.h"
#include "NifFile.h"
#include "Object3d.h"
#include "KDMatcher.h"
#include "Mesh.h"
#include "ResourceLoader.h"
#include "TweakBrush.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include <string>
#include <vector>
#include <hash_map>
#include <wx/glcanvas.h>

using namespace std;

class GLSurface {
	wxGLCanvas* canvas{nullptr};
	wxGLContext* context{nullptr};

	bool perspective;
	float mFov;
	Vector3 camPos;
	Vector3 camRot;		// Turntable camera emulation.
	uint vpW;
	uint vpH;

	bool bUseAF;
	GLfloat largestAF;

	bool bUseVBO;
	static bool multiSampleEnabled;

	bool bWireframe;
	bool bLighting;
	bool bTextured;
	bool bMaskVisible;
	bool bWeightColors;

	float defLineWidth;
	float defPointSize;
	float cursorSize;

	ResourceLoader resLoader;
	GLMaterial* noImage{nullptr};
	GLMaterial* skinMaterial{nullptr};
	unordered_map<string, int> texMats;

	TweakUndo tweakUndo;

	unordered_map<string, int> namedMeshes;
	unordered_map<string, int> namedOverlays;
	vector<mesh*> meshes;
	vector<mesh*> overlays;
	int activeMesh;

	void initLighting();
	void initMaterial(Vector3 diffusecolor);
	void InitGLExtensions();
	int InitGLSettings(bool bUseDefaultShaders);
	static int QueryMultisample(wxWindow* parent);
	static int FindBestNumSamples(HDC hDC);

	void DeleteMesh(int meshID) {
		if (meshID < meshes.size()) {
			delete meshes[meshID];
			meshes.erase(meshes.begin() + meshID);
			for (int i = meshID; i < meshes.size(); i++) { // Renumber meshes after the deleted one.
				namedMeshes[meshes[i]->shapeName] = i;
			}
		}
		if (activeMesh >= meshID)
			activeMesh--;
	}

public:
	bool bEditMode;
	GLSurface(void);
	~GLSurface(void);

	// Get the attributes to use for creating a wxGLCanvas
	static const int* GetGLAttribs(wxWindow* parent);

	void DeleteAllMeshes() {
		for (auto m : meshes) {
			delete m;
		}
		meshes.clear();
		namedMeshes.clear();
	}


	void DeleteMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0) {
			DeleteMesh(id);
			namedMeshes.erase(shapeName);
		}
	}

	void DeleteOverlays() {
		for (int i = 0; i < overlays.size(); i++) {
			delete overlays[i];
		}
		overlays.clear();
		namedOverlays.clear();
	}


	int GetMeshID(const string& shapeName) {
		auto it = namedMeshes.find(shapeName);
		if (it != namedMeshes.end())
			return it->second;

		return -1;
	}
	string GetMeshName(int id) {
		for (auto mn : namedMeshes)
			if (mn.second == id)
				return mn.first;

		return "";
	}

	void RenameMesh(const string& shapeName, const string& newShapeName) {
		if (namedMeshes.find(shapeName) != namedMeshes.end()) {
			int mid = namedMeshes[shapeName];
			namedMeshes[newShapeName] = mid;
			meshes[mid]->shapeName = newShapeName;
			namedMeshes.erase(shapeName);
		}
	}


	int GetOverlayID(const string& shapeName) {
		unordered_map<string, int>::iterator it = namedOverlays.find(shapeName);
		if (it != namedOverlays.end()) {
			return it->second;
		}

		return -1;
	}
	mesh* GetActiveMesh() {
		if (activeMesh >= 0 && activeMesh < meshes.size())
			return meshes[activeMesh];

		return nullptr;
	}

	Vector3 GetActiveCenter(bool useMask = true) {
		mesh* m = GetActiveMesh();
		if (!m) return Vector3(0, 0, 0);
		int count = 0;
		Vector3 total;
		for (int i = 0; i < m->nVerts; i++) {
			if (!useMask || m->vcolors[i].x == 0.0f) {
				total = total + m->verts[i];
				count++;
			}
		}
		total = total / count;
		return total;
	}

	bool IsValidMesh(mesh* query) {
		for (int i = 0; i < meshes.size(); i++) {
			if (meshes[i] == query) {
				return true;
			}
		}
		return false;
	}

	mesh* GetMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0)
			return meshes[id];
		
		return nullptr;
	}

	mesh* GetOverlay(int overlayID) {
		if (overlayID >= 0)
			return overlays[overlayID];
		
		return nullptr;
	}

	mesh* GetOverlay(const string& overlayName) {
		int id = GetOverlayID(overlayName);
		if (id >= 0)
			return overlays[id];
		
		return nullptr;
	}

	float GetCursorSize() {
		return cursorSize;
	}

	void SetCursorSize(float newsize) {
		cursorSize = newsize;
	}

	static bool IsWGLExtensionSupported(char* szTargetExtension);
	static bool IsExtensionSupported(char* szTargetExtension);
	int Initialize(wxGLCanvas* canvas, wxGLContext* context, bool bUseDefaultShaders = true);
	void Begin();
	void Cleanup();

	void SetStartingView(const Vector3& camPos, const Vector3& camRot, const uint& vpWidth, const uint& vpHeight, const float& fov = 65.0f);
	void SetSize(uint w, uint h);
	void UpdateProjection();

	void TurnTableCamera(int dScreenX);
	void PitchCamera(int dScreenY);
	void PanCamera(int dScreenX, int dScreenY);
	void DollyCamera(int dAmount);
	void UnprojectCamera(Vector3& result);

	void SetView(const char& type);
	void SetPerspective(const bool& enabled);
	void SetFieldOfView(const int& fieldOfView);
	void UpdateLights(const int& ambient = 50, const int& brightness1 = 50, const int& brightness2 = 50, const int& brightness3 = 50);

	void GetPickRay(int ScreenX, int ScreenY, Vector3& dirVect, Vector3& outNearPos);
	int PickMesh(int ScreenX, int ScreenY);
	bool UpdateCursor(int ScreenX, int ScreenY, int* outHoverTri = nullptr, float* outHoverWeight = nullptr, float* outHoverMask = nullptr);
	bool GetCursorVertex(int ScreenX, int ScreenY, Vertex* outHoverVtx = nullptr);
	void ShowCursor(bool show = true);

	// Ray/mesh collision detection. From a screen point, calculates a ray and finds the nearest collision point and surface normal on
	// the active mesh. Optionally, the ray and ray origin can be provided, which skips the internal call to GetPickRay.
	// Screen x/y are ignored if the ray is provided.
	bool CollideMesh(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, int* outFacet = nullptr, Vector3* inRayDir = 0, Vector3* inRayOrigin = 0);
	bool CollidePlane(int ScreenX, int ScreenY, Vector3& outOrigin, const Vector3& inPlaneNormal, float inPlaneDist);
	int CollideOverlay(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, int* outFacet = nullptr, Vector3* inRayDir = 0, Vector3* inRayOrigin = 0);

	int AddVisRay(Vector3& start, Vector3& direction, float length);
	int AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const string& name = "RingMesh");
	int AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const string& name = "XRotateMesh");
	int AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const string& name = "XMoveMesh");
	int AddVisPoint(const Vector3& p, const string& name = "PointMesh");
	int AddVisTri(const Vector3& p1, const Vector3& p2, const Vector3& p3, const string& name = "TriMesh");
	int AddVisFacets(vector<int>& triIDs, const string& name = "TriMesh");
	int AddVisFacetsInSphere(Vector3& origin, float radius, const string& name = "SphereFIntersect");
	int AddVisPointsInSphere(Vector3& origin, float radius, const string& name = "SpherePIntersect");

	void BeginEditMode();
	void EditUndo();
	void EditRedo();
	void EndEditMode();

	void AddMeshFromNif(NifFile* nif, string shapeName, Vector3* color = nullptr, bool smoothNormalSeams = true);
	void AddMeshExplicit(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs = nullptr, const string& name = "", float scale = 1.0f);
	void AddMeshDirect(mesh* m);
	void Update(const string& shapeName, vector<Vector3>* vertices, vector<Vector2>* uvs = nullptr);
	void Update(int shapeIndex, vector<Vector3>* vertices, vector<Vector2>* uvs = nullptr);
	void ReloadMeshFromNif(NifFile* nif, string shapeName);
	void RecalculateMeshBVH(const string& shapeName);
	void RecalculateMeshBVH(int shapeIndex);

	void SetMeshVisibility(const string& name, bool visible = true);
	void SetMeshVisibility(int shapeIndex, bool visible = true);
	void SetOverlayVisibility(const string& name, bool visible = true);
	void SetActiveMesh(int shapeIndex);
	void SetActiveMesh(const string& shapeName);

	RenderMode SetMeshRenderMode(const string& name, RenderMode mode);

	GLMaterial* AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile);

	void RenderOneFrame();
	void RenderMesh(mesh* m);

	void ToggleTextures() {
		if (bTextured)
			bTextured = false;
		else
			bTextured = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowTexture(bTextured);
	}

	void ToggleWireframe() {
		if (bWireframe)
			bWireframe = false;
		else
			bWireframe = true;
	}

	void ToggleLighting() {
		if (bLighting)
			bLighting = false;
		else
			bLighting = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->EnableVertexLighting(bLighting);
	}

	void ToggleMask() {
		if (bMaskVisible)
			bMaskVisible = false;
		else
			bMaskVisible = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowMask(bMaskVisible);
	}

	void SetWeightColors(bool bVisible = true) {
		bWeightColors = bVisible;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowWeight(bWeightColors);
	}

	void ToggleWeightColors() {
		if (bWeightColors)
			bWeightColors = false;
		else
			bWeightColors = true;

		if (meshes.size() > 0 && (activeMesh > -1) && meshes[activeMesh]->material)
			meshes[activeMesh]->material->shader->ShowWeight(bWeightColors);
	}
};
