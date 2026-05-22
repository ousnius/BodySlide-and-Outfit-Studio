/*
BodySlide and Outfit Studio
See the included LICENSE file

Universal Model Layer - Abstract interface for any 3D model format
*/

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>

// Forward declarations for nifly types (used internally)
namespace nifly {
	class NifFile;
	class NiShape;
	struct Vector3;
	struct Triangle;
	struct Vector2;
	struct MatTransform;
	class AnimSkin;
}

namespace univmodel {

// ============== CORE DATA STRUCTURES ==============

struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float r, g, b, a;  // Vertex colors
	uint32_t id;       // Unique identifier for welding/skinning
};

struct Triangle {
	uint32_t v1, v2, v3;
	uint32_t submeshIndex;
};

struct Submesh {
	uint32_t startIndex;
	uint32_t triangleCount;
	std::string materialName;
	float color[4];
	bool visible;
};

struct BoneWeight {
	uint8_t boneIndex;
	float weight;
};

struct VertexSkinData {
	std::vector<BoneWeight> weights;
};

struct Bone {
	std::string name;
	std::string parentName;
	float transform[16];  // 4x4 matrix (column-major)
	float localTransform[16];  // Transform to parent
	bool isStandardBone;
};

struct Skeleton {
	std::vector<Bone> bones;
	std::string rootBoneName;
};

// ============== MESH DATA ==============

class UniversalMesh {
public:
	std::string name;
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;
	std::vector<Submesh> submeshes;
	std::vector<std::string> texturePaths;
	
	// Skinning data (optional)
	std::optional<std::vector<VertexSkinData>> skinData;
	std::optional<Skeleton> skeleton;
	
	// Bounding data
	float boundsMin[3];
	float boundsMax[3];
	float boundsRadius;
	
	UniversalMesh() = default;
	UniversalMesh(const UniversalMesh&) = default;
	UniversalMesh& operator=(const UniversalMesh&) = default;
	
	void ComputeBounds();
	size_t GetVertexCount() const { return vertices.size(); }
	size_t GetTriangleCount() const { return triangles.size(); }
	
	// Apply weld results to merge vertices
	void ApplyWeld(const std::map<uint32_t, std::vector<uint32_t>>& weldMap);
};

// ============== MODEL HANDLE ==============

class UniversalModel {
public:
	std::string name;
	std::vector<UniversalMesh> meshes;
	std::map<std::string, std::string> metadata;  // Format-specific data
	
	UniversalModel() = default;
	UniversalModel(const UniversalModel&) = default;
	UniversalModel& operator=(const UniversalModel&) = default;
	
	void AddMesh(const UniversalMesh& mesh) { meshes.push_back(mesh); }
	size_t GetMeshCount() const { return meshes.size(); }
	UniversalMesh* GetMesh(const std::string& name);
	UniversalMesh* GetMesh(size_t index);
};

// ============== ANIMATION DATA ==============

struct AnimKeyframe {
	float time;
	float value[3];  // For position/rotation (quaternion or euler)
	float rotation[4];  // Quaternion (x, y, z, w)
};

struct AnimTrack {
	std::string boneName;
	std::vector<AnimKeyframe> positionKeys;
	std::vector<AnimKeyframe> rotationKeys;
	std::vector<AnimKeyframe> scaleKeys;
};

class UniversalAnimation {
public:
	std::string name;
	float duration;
	float framesPerSecond;
	std::vector<AnimTrack> tracks;
	std::map<std::string, std::string> metadata;
};

// ============== FORMAT CAPABILITIES ==============

enum class FormatType {
	NIF,      // Bethesda NIF
	FBX,      // Autodesk FBX
	GLTF,     // GL Transmission Format (glTF 2.0)
	VRM,      // VRoid/Virtual YouTuber format
	OBJ,      // Wavefront OBJ
	UNKNOWN   // Auto-detected or fallback
};

enum class FormatCapability {
	ImportMeshes,
	ExportMeshes,
	ImportSkinning,
	ExportSkinning,
	ImportAnimations,
	ExportAnimations,
	ImportMaterials,
	ExportMaterials,
	ImportTextures,
	ExportTextures,
	ImportMorphTargets,
	ExportMorphTargets
};

struct FormatInfo {
	FormatType type;
	std::string name;
	std::string extension;
	std::vector<FormatCapability> capabilities;
	std::vector<std::string> fileFilters;  // For file dialogs
	bool supportsMultipleMeshes;
	bool isBinaryFormat;
};

// ============== FORMAT EXPORTER/IMPORTER INTERFACE ==============

class IFormatHandler {
public:
	virtual ~IFormatHandler() = default;
	
	virtual FormatInfo GetFormatInfo() const = 0;
	virtual bool CanImport() const = 0;
	virtual bool CanExport() const = 0;
	
	// Import/Export model
	virtual std::unique_ptr<UniversalModel> ImportModel(const std::string& filePath) = 0;
	virtual bool ExportModel(const UniversalModel& model, const std::string& filePath) = 0;
	
	// Import/Export animation
	virtual std::unique_ptr<UniversalAnimation> ImportAnimation(const std::string& filePath) = 0;
	virtual bool ExportAnimation(const UniversalAnimation& anim, const std::string& filePath) = 0;
	
	// Check if this handler can handle the given file
	virtual bool CanHandleFile(const std::string& filePath) const = 0;
	
	// Get format type
	virtual FormatType GetFormatType() const = 0;
};

// ============== FORMAT REGISTRY ==============

class FormatRegistry {
public:
	static FormatRegistry& GetInstance();
	
	// Register a format handler
	void RegisterHandler(std::unique_ptr<IFormatHandler> handler);
	
	// Get all registered formats (fills the provided vector)
	void GetAllFormats(std::vector<FormatInfo>& formats) const;
	
	// Find handler for a specific format
	IFormatHandler* GetHandler(FormatType type);
	IFormatHandler* GetHandler(const std::string& extension);
	
	// Auto-detect format from file
	FormatType DetectFormat(const std::string& filePath);
	IFormatHandler* GetHandlerForFile(const std::string& filePath);
	
	// Get formats supporting specific capability
	std::vector<FormatInfo> GetFormatsWithCapability(FormatCapability cap);
	
	// Get default export format for a use case
	FormatType GetDefaultExportFormat(const std::string& useCase);
	
	// Get file filter string for file dialogs
	std::string GetFileFilterString() const;
	std::string GetImportFileFilterString() const;
	std::string GetExportFileFilterString() const;
	
private:
	FormatRegistry() = default;
	// Non-copyable due to unique_ptr members
	FormatRegistry(const FormatRegistry&) = delete;
	FormatRegistry& operator=(const FormatRegistry&) = delete;
	// Moveable
	FormatRegistry(FormatRegistry&&) = default;
	FormatRegistry& operator=(FormatRegistry&&) = default;
	
	std::map<FormatType, std::unique_ptr<IFormatHandler>> handlers;
	std::map<std::string, FormatType> extensionMap;
};

// ============== NIF CONVERTER ==============

class NifConverter {
public:
	// Convert from NIF to UniversalModel
	static std::unique_ptr<UniversalModel> NifToUniversal(nifly::NifFile* nif, const std::string& shapeName = {});
	
	// Convert from UniversalModel to NIF
	static void UniversalToNif(const UniversalModel& model, nifly::NifFile* nif, const std::string& shapeName = {});
	
	// Get available shapes from NIF
	static std::vector<std::string> GetNifShapes(nifly::NifFile* nif);
	
	// Copy skinning data
	static void CopySkinning(nifly::NifFile* sourceNif, nifly::NifFile* targetNif, 
							 const std::string& sourceShape, const std::string& targetShape);
};

// ============== MESH UTILITIES ==============

class MeshUtils {
public:
	// Merge multiple meshes into one
	static UniversalMesh MergeMeshes(const std::vector<UniversalMesh*>& meshes);
	
	// Split mesh by materials/submeshes
	static std::vector<UniversalMesh> SplitByMaterials(const UniversalMesh& mesh);
	
	// Apply transform to mesh
	static void ApplyTransform(UniversalMesh& mesh, const float transform[16]);
	
	// Mirror mesh along an axis
	static void MirrorMesh(UniversalMesh& mesh, int axis, float threshold = 0.0f);
	
	// Weld vertices by position proximity
	static void WeldVertices(UniversalMesh& mesh, float tolerance);
	
	// Generate smooth normals
	static void GenerateSmoothNormals(UniversalMesh& mesh, float angleThreshold = 60.0f);
	
	// Generate tangent space
	static void GenerateTangents(UniversalMesh& mesh);
	
	// Create weld vertex map (for handling UV seams)
	static std::map<uint32_t, std::vector<uint32_t>> CreateWeldMap(const UniversalMesh& mesh, float tolerance);
};

// ============== SKELETON UTILITIES ==============

class SkeletonUtils {
public:
	// Create basic humanoid skeleton
	static Skeleton CreateHumanoidSkeleton();
	
	// Bind mesh to skeleton with automatic weight calculation
	static void AutoBindSkeleton(UniversalMesh& mesh, const Skeleton& skeleton, float maxDistance = 50.0f);
	
	// Copy bone weights from one mesh to another (same vertex count required)
	static void CopyWeights(UniversalMesh& target, const UniversalMesh& source, 
							const std::map<uint32_t, uint32_t>& vertexMapping);
	
	// Normalize weights to sum to 1.0
	static void NormalizeWeights(UniversalMesh& mesh);
	
	// Remove zero-weight assignments
	static void PruneWeights(UniversalMesh& mesh, float minWeight = 0.001f);
};

} // namespace univmodel