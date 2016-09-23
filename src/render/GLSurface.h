/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "GLShader.h"
#include "../files/NifFile.h"
#include "../files/ResourceLoader.h"
#include "../components/Mesh.h"

#include <wx/glcanvas.h>

using namespace std;

class GLSurface {
	wxGLCanvas* canvas = nullptr;
	wxGLContext* context = nullptr;

	bool perspective;
	float mFov;
	Vector3 camPos;
	Vector3 camRot;		// Turntable camera emulation.
	Vector3 camOffset;
	uint vpW;
	uint vpH;

	GLfloat largestAF = 0;

	bool bWireframe;
	bool bLighting;
	bool bTextured;
	bool bMaskVisible;
	bool bWeightColors;
	bool bSegmentColors;

	float defLineWidth;
	float defPointSize;
	float cursorSize;

	Vector3 colorRed = Vector3(1.0f, 0.25f, 0.25f);
	Vector3 colorGreen = Vector3(0.25f, 1.0f, 0.25f);

	ResourceLoader resLoader;
	GLMaterial* noImage = nullptr;

	unordered_map<string, int> namedMeshes;
	unordered_map<string, int> namedOverlays;
	vector<mesh*> meshes;
	vector<mesh*> overlays;

	vector<mesh*> activeMeshes;
	vector<int> activeMeshesID;
	mesh* selectedMesh = nullptr;

	void InitLighting();
	void InitMaterial(Vector3 diffuseColor);
	void InitGLExtensions();
	int InitGLSettings();

	void DeleteMesh(int meshID) {
		if (meshID < meshes.size()) {
			delete meshes[meshID];
			meshes.erase(meshes.begin() + meshID);

			// Renumber meshes after the deleted one
			for (int i = meshID; i < meshes.size(); i++)
				namedMeshes[meshes[i]->shapeName] = i;
		}
	}

public:
	GLSurface();
	~GLSurface();

	// Get the attributes to use for creating a wxGLCanvas
	static const wxGLAttributes& GetGLAttribs();
	static const wxGLContextAttrs& GetGLContextAttribs();

	void DeleteAllMeshes() {
		for (auto &m : meshes)
			delete m;

		meshes.clear();
		namedMeshes.clear();
		activeMeshesID.clear();
		activeMeshes.clear();
	}

	void DeleteMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0) {
			DeleteMesh(id);
			namedMeshes.erase(shapeName);
		}
	}

	void DeleteOverlays() {
		for (int i = 0; i < overlays.size(); i++)
			delete overlays[i];

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
		for (auto &mn : namedMeshes)
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
		if (it != namedOverlays.end())
			return it->second;

		return -1;
	}

	vector<mesh*> GetActiveMeshes() {
		return activeMeshes;
	}

	Vector3 GetActiveCenter(bool useMask = true) {
		if (activeMeshes.empty())
			return Vector3();

		int count = 0;
		Vector3 total;

		for (auto &m : activeMeshes) {
			for (int i = 0; i < m->nVerts; i++) {
				if (!useMask || m->vcolors[i].x == 0.0f) {
					total = total + m->verts[i];
					count++;
				}
			}
		}

		if (count <= 0)
			return Vector3();

		total = total / count;
		return total;
	}

	mesh* GetMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0)
			return meshes[id];

		return nullptr;
	}

	const vector<mesh*>& GetMeshes() {
		return meshes;
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

	static bool IsExtensionSupported(char* szTargetExtension);
	int Initialize(wxGLCanvas* canvas, wxGLContext* context);
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
	bool UpdateCursor(int ScreenX, int ScreenY, bool allMeshes = true, string* hitMeshName = nullptr, int* outHoverTri = nullptr, float* outHoverWeight = nullptr, float* outHoverMask = nullptr);
	bool GetCursorVertex(int ScreenX, int ScreenY, Vertex* outHoverVtx = nullptr);
	void ShowCursor(bool show = true);

	// Ray/mesh collision detection. From a screen point, calculates a ray and finds the nearest collision point and surface normal on
	// the active mesh. Optionally, the ray and ray origin can be provided, which skips the internal call to GetPickRay.
	// Screen x/y are ignored if the ray is provided.
	bool CollideMeshes(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh = nullptr, bool allMeshes = true, int* outFacet = nullptr, Vector3* inRayDir = nullptr, Vector3* inRayOrigin = nullptr);
	bool CollidePlane(int ScreenX, int ScreenY, Vector3& outOrigin, const Vector3& inPlaneNormal, float inPlaneDist);
	bool CollideOverlay(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh = nullptr, int* outFacet = nullptr, Vector3* inRayDir = 0, Vector3* inRayOrigin = 0);

	int AddVisRay(Vector3& start, Vector3& direction, float length);
	int AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const string& name = "RingMesh");
	mesh* AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const string& name = "XRotateMesh");
	mesh* AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const string& name = "XMoveMesh");
	mesh* AddVisPoint(const Vector3& p, const string& name = "PointMesh", const Vector3* color = nullptr);

	void AddMeshFromNif(NifFile* nif, string shapeName, Vector3* color = nullptr, bool smoothNormalSeams = true);
	void Update(const string& shapeName, vector<Vector3>* vertices, vector<Vector2>* uvs = nullptr, vector<int>* changed = nullptr);
	void Update(int shapeIndex, vector<Vector3>* vertices, vector<Vector2>* uvs = nullptr, vector<int>* changed = nullptr);
	void ReloadMeshFromNif(NifFile* nif, string shapeName);
	void RecalculateMeshBVH(const string& shapeName);
	void RecalculateMeshBVH(int shapeIndex);

	void SetMeshVisibility(const string& name, bool visible = true);
	void SetMeshVisibility(int shapeIndex, bool visible = true);
	void SetOverlayVisibility(const string& name, bool visible = true);
	void SetActiveMeshesID(const vector<int>& shapeIndices);
	void SetActiveMeshesID(const vector<string>& shapeNames);
	void SetSelectedMesh(const string& shapeName);

	RenderMode SetMeshRenderMode(const string& name, RenderMode mode);

	GLMaterial* AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile);

	void RenderOneFrame();
	void RenderMesh(mesh* m);

	void ToggleTextures() {
		if (bTextured)
			bTextured = false;
		else
			bTextured = true;

		for (auto &m : meshes)
			if (m->material)
				m->material->GetShader().ShowTexture(bTextured);
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

		for (auto &m : meshes)
			if (m->material)
				m->material->GetShader().ShowLighting(bLighting);
	}

	void SetMaskVisible(bool bVisible = true) {
		bMaskVisible = bVisible;

		for (auto &m : meshes)
			if (m->material)
				m->material->GetShader().ShowMask(bVisible);
	}

	void SetWeightColors(bool bVisible = true) {
		bWeightColors = bVisible;

		for (auto &m : meshes)
			if (m->material)
				m->material->GetShader().ShowWeight(bWeightColors);
	}

	void SetSegmentColors(bool bVisible = true) {
		bSegmentColors = bVisible;

		for (auto &m : meshes)
			if (m->material)
				m->material->GetShader().ShowSegments(bSegmentColors);
	}

	void ToggleWeightColors() {
		if (bWeightColors)
			bWeightColors = false;
		else
			bWeightColors = true;

		for (auto &m : activeMeshes)
			if (m->material)
				m->material->GetShader().ShowWeight(bWeightColors);
	}
};
