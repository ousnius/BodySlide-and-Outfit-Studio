/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "GLMaterial.h"
#include "../NIF/NifFile.h"

#include <wx/glcanvas.h>

class GLSurface {
public:
	enum CursorType {
		None = 0,
		CenterCursor = 1,
		PointCursor = 2,
		CircleCursor = 4,
		SegCursor = 8,
		BrushCursor = CenterCursor | PointCursor | CircleCursor,
		VertexCursor = CenterCursor | PointCursor,
		EdgeCursor = CenterCursor | SegCursor
	};
private:
	wxGLCanvas* canvas = nullptr;
	wxGLContext* context = nullptr;

	glm::mat4x4 matProjection = glm::identity<glm::mat4x4>();;
	glm::mat4x4 matView = glm::identity<glm::mat4x4>();;
	bool perspective = true;
	float mFov = 90.0f;
	Vector3 camPos;
	Vector3 camRot;		// Turntable camera emulation.
	Vector3 camOffset;
	uint vpW = 800;
	uint vpH = 600;

	GLfloat largestAF = 0;

	bool bWireframe = false;
	bool bLighting = true;
	bool bTextured = true;
	bool bMaskVisible = true;
	bool bWeightColors = false;
	bool bVertexColors = false;

	float defLineWidth = 1.0f;
	float defPointSize = 5.0f;
	float cursorSize = 0.5f;
	CursorType cursorType = CursorType::None;

	GLShader::DirectionalLight frontalLight;
	GLShader::DirectionalLight directionalLight0;
	GLShader::DirectionalLight directionalLight1;
	GLShader::DirectionalLight directionalLight2;
	float ambientLight = 0.2f;

	Vector3 colorBackground = Vector3(0.82f, 0.82f, 0.82f);
	Vector3 colorWire = Vector3(0.3137f, 0.3137f, 0.3137f);
	Vector3 colorRed = Vector3(1.0f, 0.25f, 0.25f);
	Vector3 colorGreen = Vector3(0.25f, 1.0f, 0.25f);

	ResourceLoader resLoader;
	GLMaterial* primitiveMat = nullptr;

	std::vector<mesh*> meshes;
	std::vector<mesh*> overlays;

	std::vector<mesh*> activeMeshes;
	mesh* selectedMesh = nullptr;

	void InitLighting();
	void InitGLExtensions();
	int InitGLSettings();

public:
	// Get the attributes to use for creating a wxGLCanvas
	static const wxGLAttributes& GetGLAttribs();
	static const wxGLContextAttrs& GetGLContextAttribs();

	Vector3 GetBackgroundColor() {
		return colorBackground;
	}

	void SetBackgroundColor(const Vector3& color) {
		colorBackground = color;
	}

	Vector3 GetWireColor() {
		return colorWire;
	}

	void SetWireColor(const Vector3& color) {
		colorWire = color;
	}

	void ClearMeshes() {
		SetContext();

		for (auto &m : meshes)
			delete m;

		meshes.clear();
		activeMeshes.clear();
	}

	void DeleteMesh(mesh* m) {
		if (!m)
			return;

		auto it = std::find(meshes.begin(), meshes.end(), m);
		if (it != meshes.end()) {
			int meshID = std::distance(meshes.begin(), it);

			activeMeshes.erase(std::remove(activeMeshes.begin(), activeMeshes.end(), m), activeMeshes.end());

			SetContext();

			delete m;
			meshes.erase(meshes.begin() + meshID);

			// Renumber meshes after the deleted one
			for (int i = meshID; i < meshes.size(); i++) {
				m = meshes[i];
			}
		}
	}

	void DeleteMesh(const std::string& shapeName) {
		DeleteMesh(GetMesh(shapeName));
	}

	void ClearOverlays() {
		SetContext();

		for (auto &m : overlays)
			delete m;

		overlays.clear();
	}

	void DeleteOverlay(mesh* m) {
		if (!m)
			return;

		auto it = std::find(overlays.begin(), overlays.end(), m);
		if (it != overlays.end()) {
			int meshID = std::distance(overlays.begin(), it);

			SetContext();

			delete m;
			overlays.erase(overlays.begin() + meshID);

			// Renumber overlays after the deleted one
			for (int i = meshID; i < overlays.size(); i++) {
				m = overlays[i];
			}
		}
	}

	void DeleteOverlay(const std::string& shapeName) {
		DeleteOverlay(GetOverlay(shapeName));
	}

	void RenameMesh(const std::string& shapeName, const std::string& newShapeName) {
		mesh* m = GetMesh(shapeName);
		if (m)
			m->shapeName = newShapeName;
	}

	std::vector<mesh*> GetActiveMeshes() {
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
					glm::vec3 mv(m->matModel * glm::vec4(m->verts[i].x, m->verts[i].y, m->verts[i].z, 1.0));
					total = total + Vector3(mv.x, mv.y, mv.z);
					count++;
				}
			}
		}

		if (count <= 0)
			return Vector3();

		total = total / count;
		return total;
	}

	void AddMesh(mesh* m) {
		if (!m->shapeName.empty())
			DeleteMesh(m->shapeName);

		meshes.push_back(m);
	}

	void AddOverlay(mesh* m) {
		if (!m->shapeName.empty())
			DeleteOverlay(m->shapeName);

		overlays.push_back(m);
	}

	mesh* GetMesh(const std::string& shapeName) {
		auto it = std::find_if(meshes.begin(), meshes.end(), [&shapeName](mesh* m) { return m->shapeName == shapeName; });
		if (it != meshes.end())
			return (*it);

		return nullptr;
	}

	const std::vector<mesh*>& GetMeshes() {
		return meshes;
	}

	const std::vector<mesh*> GetMeshesFiltered() {
		std::vector<mesh*> filteredMeshes;

		for (auto &m : meshes)
			if (m->rendermode == RenderMode::Normal)
				filteredMeshes.push_back(m);

		return filteredMeshes;
	}

	mesh* GetOverlay(const std::string& shapeName) {
		auto it = std::find_if(overlays.begin(), overlays.end(), [&shapeName](mesh* m) { return m->shapeName == shapeName; });
		if (it != overlays.end())
			return (*it);

		return nullptr;
	}

	float GetCursorSize() {
		return cursorSize;
	}

	void SetCursorSize(float newsize) {
		cursorSize = newsize;
	}

	CursorType GetCursorType() const {
		return cursorType;
	}
	void SetCursorType(CursorType newType) {
		cursorType = newType;
	}

	int Initialize(wxGLCanvas* canvas, wxGLContext* context);
	void Cleanup();

	void SetStartingView(const Vector3& camPos, const Vector3& camRot, const uint& vpWidth, const uint& vpHeight, const float& fov = 65.0f);
	void SetSize(uint w, uint h);
	void GetSize(uint &w, uint &h);
	void UpdateProjection();

	void RenderFullScreenQuad(GLMaterial * renderShader, unsigned int w, unsigned int h);

	void TurnTableCamera(int dScreenX);
	void PitchCamera(int dScreenY);
	void PanCamera(int dScreenX, int dScreenY);
	void DollyCamera(int dAmount);
	void ClampCameraPosition(char axis, float lower, float upper);
	void UnprojectCamera(Vector3& result);

	void SetView(const char type);
	void SetPerspective(const bool enabled);
	void SetFieldOfView(const int fieldOfView);
	void UpdateLights(const int ambient, const int frontal, const int directional0, const int directional1, const int directional2,
		const Vector3& directional0Dir, const Vector3& directional1Dir, const Vector3& directional2Dir);

	void GetPickRay(int ScreenX, int ScreenY, mesh* m, Vector3& dirVect, Vector3& outNearPos);
	mesh* PickMesh(int ScreenX, int ScreenY);
	bool UpdateCursor(int ScreenX, int ScreenY, bool allMeshes = true, std::string* hitMeshName = nullptr, int* outHoverPoint = nullptr, Vector3* outHoverColor = nullptr, float* outHoverAlpha = nullptr, Edge* outHoverEdge = nullptr);
	bool GetCursorVertex(int ScreenX, int ScreenY, int* outIndex = nullptr, mesh* hitMesh = nullptr);
	void ShowCursor(bool show = true);
	void HidePointCursor();
	void HideSegCursor();

	// Ray/mesh collision detection. From a screen point, calculates a ray and finds the nearest collision point and surface normal on
	// the active mesh. Optionally, the ray and ray origin can be provided, which skips the internal call to GetPickRay.
	// Screen x/y are ignored if the ray is provided.
	bool CollideMeshes(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, bool mirrored = false, mesh** hitMesh = nullptr, bool allMeshes = true, int* outFacet = nullptr);
	bool CollidePlane(int ScreenX, int ScreenY, Vector3& outOrigin, const Vector3& inPlaneNormal, float inPlaneDist);
	bool CollideOverlay(int ScreenX, int ScreenY, Vector3& outOrigin, Vector3& outNormal, mesh** hitMesh = nullptr, int* outFacet = nullptr);

	mesh* AddVisCircle(const Vector3& center, const Vector3& normal, float radius, const std::string& name = "RingMesh");
	mesh* AddVis3dRing(const Vector3& center, const Vector3& normal, float holeRadius, float ringRadius, const Vector3& color, const std::string& name);
	mesh* AddVis3dArrow(const Vector3& origin, const Vector3& direction, float stemRadius, float pointRadius, float length, const Vector3& color, const std::string& name);
	mesh* AddVis3dCube(const Vector3& center, const Vector3& normal, float radius, const Vector3& color, const std::string& name);
	mesh* AddVisPoint(const Vector3& p, const std::string& name = "PointMesh", const Vector3* color = nullptr);
	mesh* AddVisPlane(const Vector3& center, const Vector2& size, float uvScale = 1.0f, float uvOffset = 0.0f, const std::string& name = "PlaneMesh", const Vector3* color = nullptr);
	mesh* AddVisSeg(const Vector3& p1, const Vector3& p2, const std::string& name = "", const bool asMesh = false);

	mesh* AddMeshFromNif(NifFile* nif, const std::string& shapeName, Vector3* color = nullptr);
	void SetSkinModelMat(mesh *m, const MatTransform &xformGlobalToSkin);
	void Update(const std::string& shapeName, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs = nullptr, std::set<int>* changed = nullptr);
	void Update(mesh* m, std::vector<Vector3>* vertices, std::vector<Vector2>* uvs = nullptr, std::set<int>* changed = nullptr);
	mesh* ReloadMeshFromNif(NifFile* nif, std::string shapeName);
	void RecalculateMeshBVH(const std::string& shapeName);

	bool SetMeshVisibility(const std::string& name, bool visible = true);
	void SetOverlayVisibility(const std::string& name, bool visible = true);
	void SetActiveMeshes(const std::vector<std::string>& shapeNames);
	void SetSelectedMesh(const std::string& shapeName);

	RenderMode SetMeshRenderMode(const std::string& name, RenderMode mode);

	GLMaterial* AddMaterial(const std::vector<std::string>& textureFiles, const std::string& vShaderFile, const std::string& fShaderFile, const bool reloadTextures = false);
	GLMaterial* GetPrimitiveMaterial();
	ResourceLoader* GetResourceLoader() {
		return &resLoader;
	}

	bool SetContext();
	void RenderOneFrame();
	void RenderToTexture(GLMaterial* renderShader);
	void RenderMesh(mesh* m);

	void UpdateShaders(mesh* m);

	void ToggleTextures() {
		if (bTextured)
			bTextured = false;
		else
			bTextured = true;

		for (auto &m : meshes)
			UpdateShaders(m);

		for (auto &o : overlays)
			UpdateShaders(o);
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
			UpdateShaders(m);

		for (auto &o : overlays)
			UpdateShaders(o);
	}

	void SetMaskVisible(bool bVisible = true) {
		bMaskVisible = bVisible;

		for (auto &m : meshes)
			UpdateShaders(m);

		for (auto &o : overlays)
			UpdateShaders(o);
	}

	void SetWeightColors(bool bVisible = true) {
		bWeightColors = bVisible;

		for (auto &m : meshes)
			UpdateShaders(m);

		for (auto &o : overlays)
			UpdateShaders(o);
	}

	void SetVertexColors(bool bVisible = true) {
		bVertexColors = bVisible;
		bMaskVisible = !bVisible;

		for (auto &m : meshes)
			UpdateShaders(m);

		for (auto &o : overlays)
			UpdateShaders(o);
	}
};
