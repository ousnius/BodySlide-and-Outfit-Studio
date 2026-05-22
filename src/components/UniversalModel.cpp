/*
BodySlide and Outfit Studio
See the included LICENSE file

Universal Model Layer Implementation
*/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include <filesystem>
#include <fstream>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../files/FBXWrangler.h"

#include <NifFile.hpp>
#include <Nodes.hpp>
#include <Object3d.hpp>

#include <nlohmann/json.hpp>

// Forward declarations to avoid GL/glew.h dependency
namespace univmodel {
class UniversalMesh;
struct Vertex;
struct Triangle;
struct Submesh;
struct Skeleton;
}

// Use nifly headers for NIF conversion
#include <NifFile.hpp>
#include <Nodes.hpp>
#include <Object3d.hpp>

#include "UniversalModel.h"

using namespace univmodel;

// ============== FORMAT REGISTRY IMPLEMENTATION ==============

FormatRegistry& FormatRegistry::GetInstance() {
	static FormatRegistry instance;
	return instance;
}

void FormatRegistry::RegisterHandler(std::unique_ptr<IFormatHandler> handler) {
	auto info = handler->GetFormatInfo();
	handlers[info.type] = std::move(handler);
	extensionMap[info.extension] = info.type;
}

void FormatRegistry::GetAllFormats(std::vector<FormatInfo>& formats) const {
	formats.clear();
	formats.reserve(handlers.size());
	for (const auto& [type, handler] : handlers) {
		formats.push_back(handler->GetFormatInfo());
	}
}

IFormatHandler* FormatRegistry::GetHandler(FormatType type) {
	auto it = handlers.find(type);
	if (it != handlers.end()) {
		return it->second.get();
	}
	return nullptr;
}

IFormatHandler* FormatRegistry::GetHandler(const std::string& extension) {
	// Check extension map first
	std::string extLower = extension;
	std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
	
	// Remove leading dot if present
	if (!extLower.empty() && extLower[0] == '.') {
		extLower = extLower.substr(1);
	}
	
	auto it = extensionMap.find(extLower);
	if (it != extensionMap.end()) {
		return GetHandler(it->second);
	}
	
	// Fall back to iterating handlers
	for (const auto& [type, handler] : handlers) {
		auto info = handler->GetFormatInfo();
		if (info.extension == extLower) {
			return handler.get();
		}
	}
	
	return nullptr;
}

FormatType FormatRegistry::DetectFormat(const std::string& filePath) {
	std::filesystem::path path(filePath);
	std::string ext = path.extension().string();
	if (!ext.empty() && ext[0] == '.') {
		ext = ext.substr(1);
	}
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	
	auto it = extensionMap.find(ext);
	if (it != extensionMap.end()) {
		return it->second;
	}
	
	// Try to detect by file header
	std::ifstream file(filePath, std::ios::binary);
	if (file) {
		char header[4] = {0};
		file.read(header, 4);
		
		// NIF files start with valid header bytes
		if (header[0] == 'N' && header[1] == 'I') {
			return FormatType::NIF;
		}
		// glTF JSON files start with '{'
		if (header[0] == '{') {
			return FormatType::GLTF;
		}
	}
	
	return FormatType::UNKNOWN;
}

IFormatHandler* FormatRegistry::GetHandlerForFile(const std::string& filePath) {
	FormatType type = DetectFormat(filePath);
	if (type != FormatType::UNKNOWN) {
		return GetHandler(type);
	}
	
	// Try each handler
	for (const auto& [t, handler] : handlers) {
		if (handler->CanHandleFile(filePath)) {
			return handler.get();
		}
	}
	
	return nullptr;
}

std::vector<FormatInfo> FormatRegistry::GetFormatsWithCapability(FormatCapability cap) {
	std::vector<FormatInfo> formats;
	for (const auto& [type, handler] : handlers) {
		auto info = handler->GetFormatInfo();
		if (std::find(info.capabilities.begin(), info.capabilities.end(), cap) != info.capabilities.end()) {
			formats.push_back(info);
		}
	}
	return formats;
}

FormatType FormatRegistry::GetDefaultExportFormat(const std::string& useCase) {
	// VRM for virtual YouTuber models
	if (useCase == "vrm" || useCase == "vtuber") {
		return FormatType::VRM;
	}
	// glTF for game engines
	if (useCase == "gltf" || useCase == "gameengine" || useCase == "glb") {
		return FormatType::GLTF;
	}
	// FBX for generic 3D
	if (useCase == "fbx" || useCase == "generic3d") {
		return FormatType::FBX;
	}
	// NIF for Unreal engine (Bethesda)
	if (useCase == "nif" || useCase == "unreal") {
		return FormatType::NIF;
	}
	// Default to glTF as it's the most universal
	return FormatType::GLTF;
}

std::string FormatRegistry::GetFileFilterString() const {
	std::string result;
	for (const auto& [type, handler] : handlers) {
		if (!result.empty()) {						result += "|";
		}
		result += handler->GetFormatInfo().fileFilters[0];
	}
	result += "|All Supported Formats";
	return result;
}

std::string FormatRegistry::GetImportFileFilterString() const {
	std::string result;
	for (const auto& [type, handler] : handlers) {
		if (handler->CanImport()) {
			if (!result.empty()) {
				result += "|";
			}
			result += handler->GetFormatInfo().fileFilters[0];
		}
	}
	return result;
}

std::string FormatRegistry::GetExportFileFilterString() const {
	std::string result;
	for (const auto& [type, handler] : handlers) {
		if (handler->CanExport()) {
			if (!result.empty()) {
				result += "|";
			}
			result += handler->GetFormatInfo().fileFilters[0];
		}
	}
	return result;
}

// ============== NIF CONVERTER IMPLEMENTATION ==============

std::unique_ptr<UniversalModel> NifConverter::NifToUniversal(nifly::NifFile* nif, const std::string& shapeName) {
	auto model = std::make_unique<UniversalModel>();
	
	if (!nif || !nif->IsValid()) {
		return model;
	}
	
	model->name = shapeName.empty() ? "UnnamedModel" : shapeName;
	
	// Get shapes to process
	std::vector<std::string> shapeNames;
	if (shapeName.empty()) {
		shapeNames = nif->GetShapeNames();
	} else {
		shapeNames.push_back(shapeName);
	}
	
	for (const auto& sn : shapeNames) {
		nifly::NiShape* shape = nif->FindBlockByName<nifly::NiShape>(sn);
		if (!shape) {
			continue;
		}
		
		UniversalMesh mesh;
		mesh.name = sn;
		
		// Get vertices
		const auto* verts = nif->GetVertsForShape(shape);
		if (!verts) continue;
		mesh.vertices.resize(verts->size());
		
		for (size_t i = 0; i < verts->size(); ++i) {
			mesh.vertices[i].x = (*verts)[i].x;
			mesh.vertices[i].y = (*verts)[i].y;
			mesh.vertices[i].z = (*verts)[i].z;
			mesh.vertices[i].nx = 0;
			mesh.vertices[i].ny = 1;
			mesh.vertices[i].nz = 0;
			mesh.vertices[i].u = 0;
			mesh.vertices[i].v = 0;
			mesh.vertices[i].r = 1;
			mesh.vertices[i].g = 1;
			mesh.vertices[i].b = 1;
			mesh.vertices[i].a = 1;
			mesh.vertices[i].id = static_cast<uint32_t>(i);
		}
		
		// Get triangles from shape - use shape->GetTriangles() directly
		std::vector<nifly::Triangle> tris;
		shape->GetTriangles(tris);
		mesh.triangles.resize(tris.size());
		for (size_t i = 0; i < tris.size(); ++i) {
			mesh.triangles[i].v1 = tris[i].p1;
			mesh.triangles[i].v2 = tris[i].p2;
			mesh.triangles[i].v3 = tris[i].p3;
			mesh.triangles[i].submeshIndex = 0;
		}
		
		// Get UVs
		const auto* uvs = nif->GetUvsForShape(shape);
		if (uvs) {
			for (size_t i = 0; i < std::min(uvs->size(), mesh.vertices.size()); ++i) {
				mesh.vertices[i].u = (*uvs)[i].u;
				mesh.vertices[i].v = (*uvs)[i].v;
			}
		}
		
		// Get normals
		const auto* norms = nif->GetNormalsForShape(shape);
		if (norms) {
			for (size_t i = 0; i < std::min(norms->size(), mesh.vertices.size()); ++i) {
				mesh.vertices[i].nx = (*norms)[i].x;
				mesh.vertices[i].ny = (*norms)[i].y;
				mesh.vertices[i].nz = (*norms)[i].z;
			}
		}
		
		// Get skinning data
		// Note: Would need AnimInfo from OutfitProject for full skinning support
		
		// Set up single default submesh
		Submesh submesh;
		submesh.startIndex = 0;
		submesh.triangleCount = static_cast<uint32_t>(tris.size());
		submesh.materialName = u8"DefaultMaterial";
		submesh.color[0] = 1;
		submesh.color[1] = 1;
		submesh.color[2] = 1;
		submesh.color[3] = 1;
		submesh.visible = true;
		mesh.submeshes.push_back(submesh);
		
		mesh.ComputeBounds();
		model->AddMesh(mesh);
	}
	
	return model;
}

void NifConverter::UniversalToNif(const UniversalModel& model, nifly::NifFile* nif, const std::string& shapeName) {
	if (!nif) {
		return;
	}
	
	for (const auto& mesh : model.meshes) {
		std::string name = shapeName.empty() ? mesh.name : shapeName;
		
		// Create vertices
		std::vector<nifly::Vector3> verts;
		verts.resize(mesh.vertices.size());
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			verts[i].x = mesh.vertices[i].x;
			verts[i].y = mesh.vertices[i].y;
			verts[i].z = mesh.vertices[i].z;
		}
		
		// Create triangles
		std::vector<nifly::Triangle> tris;
		tris.resize(mesh.triangles.size());
		for (size_t i = 0; i < mesh.triangles.size(); ++i) {
			tris[i].p1 = mesh.triangles[i].v1;
			tris[i].p2 = mesh.triangles[i].v2;
			tris[i].p3 = mesh.triangles[i].v3;
		}
		
		// Create UV coordinates
		std::vector<nifly::Vector2> uvs;
		uvs.resize(mesh.vertices.size());
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			uvs[i].u = mesh.vertices[i].u;
			uvs[i].v = mesh.vertices[i].v;
		}
		
		// Create normals
		std::vector<nifly::Vector3> norms;
		norms.resize(mesh.vertices.size());
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			norms[i].x = mesh.vertices[i].nx;
			norms[i].y = mesh.vertices[i].ny;
			norms[i].z = mesh.vertices[i].nz;
		}
		
		nif->CreateShapeFromData(name.c_str(), &verts, &tris, &uvs, &norms);
	}
}

std::vector<std::string> NifConverter::GetNifShapes(nifly::NifFile* nif) {
	if (nif) {
		return nif->GetShapeNames();
	}
	return {};
}

void NifConverter::CopySkinning(nifly::NifFile* sourceNif, nifly::NifFile* targetNif,
								const std::string& sourceShape, const std::string& targetShape) {
	// Implementation would copy skinning data between shapes
}

// ============== MESH UTILITIES IMPLEMENTATION ==============

void UniversalMesh::ComputeBounds() {
	if (vertices.empty()) {
		boundsMin[0] = boundsMin[1] = boundsMin[2] = 0;
		boundsMax[0] = boundsMax[1] = boundsMax[2] = 0;
		boundsRadius = 0;
		return;
	}
	
	boundsMin[0] = boundsMin[1] = boundsMin[2] = std::numeric_limits<float>::max();
	boundsMax[0] = boundsMax[1] = boundsMax[2] = -std::numeric_limits<float>::max();
	
	for (const auto& v : vertices) {
		boundsMin[0] = std::min(boundsMin[0], v.x);
		boundsMin[1] = std::min(boundsMin[1], v.y);
		boundsMin[2] = std::min(boundsMin[2], v.z);
		boundsMax[0] = std::max(boundsMax[0], v.x);
		boundsMax[1] = std::max(boundsMax[1], v.y);
		boundsMax[2] = std::max(boundsMax[2], v.z);
	}
	
	float dx = boundsMax[0] - boundsMin[0];
	float dy = boundsMax[1] - boundsMin[1];
	float dz = boundsMax[2] - boundsMin[2];
	boundsRadius = std::sqrt(dx*dx + dy*dy + dz*dz) * 0.5f;
}

void UniversalMesh::ApplyWeld(const std::map<uint32_t, std::vector<uint32_t>>& weldMap) {
	if (weldMap.empty()) return;
	
	// Build remap table - each cluster of vertices maps to one canonical vertex
	std::vector<uint32_t> remap(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		remap[i] = static_cast<uint32_t>(i);
	}
	
	// Mark duplicates - all vertices in a cluster map to the canonical (first one)
	for (const auto& [target, sources] : weldMap) {
		for (uint32_t src : sources) {
			if (src < remap.size()) {
				remap[src] = target;
			}
		}
	}
	
	// Build canonical list - vertices that are kept (not duplicates)
	std::vector<uint32_t> canonical;
	std::vector<uint32_t> oldToNew(vertices.size());
	std::set<uint32_t> duplicateSet;
	
	for (const auto& [target, sources] : weldMap) {
		duplicateSet.insert(target);
		for (uint32_t src : sources) {
			duplicateSet.insert(src);
		}
	}
	
	uint32_t newIndex = 0;
	for (size_t i = 0; i < vertices.size(); ++i) {
		if (duplicateSet.find(static_cast<uint32_t>(i)) == duplicateSet.end()) {
			oldToNew[i] = newIndex++;
			canonical.push_back(static_cast<uint32_t>(i));
		}
	}
	
	// Create new vertex array with only canonical vertices
	std::vector<Vertex> newVertices;
	newVertices.reserve(canonical.size());
	for (uint32_t oldIdx : canonical) {
		newVertices.push_back(vertices[oldIdx]);
	}
	
	// Remap triangle indices
	for (auto& tri : triangles) {
		if (tri.v1 < remap.size()) tri.v1 = oldToNew[remap[tri.v1]];
		if (tri.v2 < remap.size()) tri.v2 = oldToNew[remap[tri.v2]];
		if (tri.v3 < remap.size()) tri.v3 = oldToNew[remap[tri.v3]];
	}
	
	vertices = std::move(newVertices);
}

UniversalMesh* UniversalModel::GetMesh(const std::string& name) {
	for (auto& mesh : meshes) {
		if (mesh.name == name) {
			return &mesh;
		}
	}
	return nullptr;
}

UniversalMesh* UniversalModel::GetMesh(size_t index) {
	if (index < meshes.size()) {
		return &meshes[index];
	}
	return nullptr;
}

void MeshUtils::ApplyTransform(UniversalMesh& mesh, const float transform[16]) {
	glm::mat4 mat(
		transform[0], transform[4], transform[8], transform[12],
		transform[1], transform[5], transform[9], transform[13],
		transform[2], transform[6], transform[10], transform[14],
		transform[3], transform[7], transform[11], transform[15]
	);
	
	for (auto& v : mesh.vertices) {
		glm::vec4 pos(v.x, v.y, v.z, 1);
		auto result = mat * pos;
		v.x = result.x;
		v.y = result.y;
		v.z = result.z;
		
		glm::vec4 norm(v.nx, v.ny, v.nz, 0);
		auto nresult = mat * norm;
		v.nx = nresult.x;
		v.ny = nresult.y;
		v.nz = nresult.z;
	}
}

void MeshUtils::MirrorMesh(UniversalMesh& mesh, int axis, float threshold) {
	for (auto& v : mesh.vertices) {
		if (std::abs((axis == 0) ? v.x : (axis == 1) ? v.y : v.z) > threshold) {
			if (axis == 0) v.x = -v.x;
			else if (axis == 1) v.y = -v.y;
			else v.z = -v.z;
		}
	}
	
	// Flip triangle winding order for mirrored half
	for (auto& t : mesh.triangles) {
		std::swap(t.v2, t.v3);
	}
}

void MeshUtils::GenerateSmoothNormals(UniversalMesh& mesh, float angleThreshold) {
	// Simple smooth normal generation
	std::vector<std::vector<size_t>> vertFaces(mesh.vertices.size());
	
	for (size_t i = 0; i < mesh.triangles.size(); ++i) {
		const auto& t = mesh.triangles[i];
		vertFaces[t.v1].push_back(i);
		vertFaces[t.v2].push_back(i);
		vertFaces[t.v3].push_back(i);
	}
	
	for (size_t i = 0; i < mesh.vertices.size(); ++i) {
		float nx = 0, ny = 0, nz = 0;
		for (size_t fi : vertFaces[i]) {
			const auto& t = mesh.triangles[fi];
			// Calculate face normal
			const auto& v1 = mesh.vertices[t.v1];
			const auto& v2 = mesh.vertices[t.v2];
			const auto& v3 = mesh.vertices[t.v3];
			
			float ax = v2.x - v1.x, ay = v2.y - v1.y, az = v2.z - v1.z;
			float bx = v3.x - v1.x, by = v3.y - v1.y, bz = v3.z - v1.z;
			
			float fnx = ay * bz - az * by;
			float fny = az * bx - ax * bz;
			float fnz = ax * by - ay * bx;
			
			nx += fnx;
			ny += fny;
			nz += fnz;
		}
		
		// Normalize
		float len = std::sqrt(nx*nx + ny*ny + nz*nz);
		if (len > 0.0001f) {
			mesh.vertices[i].nx = nx / len;
			mesh.vertices[i].ny = ny / len;
			mesh.vertices[i].nz = nz / len;
		}
	}
}

std::map<uint32_t, std::vector<uint32_t>> MeshUtils::CreateWeldMap(const UniversalMesh& mesh, float tolerance) {
	std::map<uint32_t, std::vector<uint32_t>> weldMap;
	float toleranceSq = tolerance * tolerance;
	
	for (size_t i = 0; i < mesh.vertices.size(); ++i) {
		const auto& v = mesh.vertices[i];
		bool found = false;
		
		for (auto& [key, indices] : weldMap) {
			const auto& ref = mesh.vertices[key];
			float dx = v.x - ref.x, dy = v.y - ref.y, dz = v.z - ref.z;
			float distSq = dx*dx + dy*dy + dz*dz;
			
			if (distSq < toleranceSq) {
				indices.push_back(static_cast<uint32_t>(i));
				found = true;
				break;
			}
		}
		
		if (!found) {
			weldMap[static_cast<uint32_t>(i)] = {static_cast<uint32_t>(i)};
		}
	}
	
	return weldMap;
}

// ============== SKELETON UTILITIES IMPLEMENTATION ==============

Skeleton SkeletonUtils::CreateHumanoidSkeleton() {
	Skeleton skeleton;
	
	// Basic humanoid bone hierarchy
	std::vector<Bone> bones = {
		{u8"Root", u8"", {}, true},
		{u8"Pelvis", u8"Root", {}, true},
		{u8"Spine", u8"Pelvis", {}, true},
		{u8"Spine1", u8"Spine", {}, true},
		{u8"Spine2", u8"Spine1", {}, true},
		{u8"Neck", u8"Spine2", {}, true},
		{u8"Head", u8"Neck", {}, true},
		{u8"Jaw", u8"Head", {}, true},
		{u8"Eye_L", u8"Head", {}, true},
		{u8"Eye_R", u8"Head", {}, true},
		
		// Arms
		{u8"Clavicle_L", u8"Spine2", {}, true},
		{u8"UpperArm_L", u8"Clavicle_L", {}, true},
		{u8"Forearm_L", u8"UpperArm_L", {}, true},
		{u8"Hand_L", u8"Forearm_L", {}, true},
		{u8"Finger0_L", u8"Hand_L", {}, true},
		{u8"Finger01_L", u8"Finger0_L", {}, true},
		{u8"Finger1_L", u8"Hand_L", {}, true},
		{u8"Finger11_L", u8"Finger1_L", {}, true},
		{u8"Finger2_L", u8"Hand_L", {}, true},
		{u8"Finger21_L", u8"Finger2_L", {}, true},
		{u8"Finger3_L", u8"Hand_L", {}, true},
		{u8"Finger31_L", u8"Finger3_L", {}, true},
		{u8"Finger4_L", u8"Hand_L", {}, true},
		{u8"Finger41_L", u8"Finger4_L", {}, true},
		
		{u8"Clavicle_R", u8"Spine2", {}, true},
		{u8"UpperArm_R", u8"Clavicle_R", {}, true},
		{u8"Forearm_R", u8"UpperArm_R", {}, true},
		{u8"Hand_R", u8"Forearm_R", {}, true},
		{u8"Finger0_R", u8"Hand_R", {}, true},
		{u8"Finger01_R", u8"Finger0_R", {}, true},
		{u8"Finger1_R", u8"Hand_R", {}, true},
		{u8"Finger11_R", u8"Finger1_R", {}, true},
		{u8"Finger2_R", u8"Hand_R", {}, true},
		{u8"Finger21_R", u8"Finger2_R", {}, true},
		{u8"Finger3_R", u8"Hand_R", {}, true},
		{u8"Finger31_R", u8"Finger3_R", {}, true},
		{u8"Finger4_R", u8"Hand_R", {}, true},
		{u8"Finger41_R", u8"Finger4_R", {}, true},
		
		// Legs
		{u8"Thigh_L", u8"Pelvis", {}, true},
		{u8"Shin_L", u8"Thigh_L", {}, true},
		{u8"Foot_L", u8"Shin_L", {}, true},
		{u8"Toe_L", u8"Foot_L", {}, true},
		
		{u8"Thigh_R", u8"Pelvis", {}, true},
		{u8"Shin_R", u8"Thigh_R", {}, true},
		{u8"Foot_R", u8"Shin_R", {}, true},
		{u8"Toe_R", u8"Foot_R", {}, true},
	};

	skeleton.bones = std::move(bones);
	skeleton.rootBoneName = u8"Root";
	
	// Initialize transforms to identity
	float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
	for (auto& bone : skeleton.bones) {
		std::memcpy(bone.transform, identity, sizeof(identity));
		std::memcpy(bone.localTransform, identity, sizeof(identity));
	}
	
	return skeleton;
}

void SkeletonUtils::AutoBindSkeleton(UniversalMesh& mesh, const Skeleton& skeleton, float maxDistance) {
	if (!mesh.skeleton.has_value()) {
		mesh.skeleton = skeleton;
	}
	
	if (!mesh.skinData.has_value()) {
		mesh.skinData = std::vector<VertexSkinData>(mesh.vertices.size());
	}
	
	// Simple distance-based binding (proximity to bone positions)
	// In a real implementation, you'd use the actual bone transforms
	// This is a basic fallback for meshes without existing skinning
	
	for (size_t vi = 0; vi < mesh.vertices.size(); ++vi) {
		auto& skin = (*mesh.skinData)[vi];
		skin.weights.clear();
		
		// Find nearest bone
		size_t nearestBone = 0;
		float nearestDist = maxDistance;
		
		for (size_t bi = 0; bi < skeleton.bones.size(); ++bi) {
			// Simple distance check (would use bone position in real impl)
			float dist = std::abs(mesh.vertices[vi].y - bi * 10.0f);
			if (dist < nearestDist) {
				nearestDist = dist;
				nearestBone = bi;
			}
		}
		
		// Assign full weight to nearest bone
		skin.weights.push_back({static_cast<uint8_t>(nearestBone), 1.0f});
	}
}

void SkeletonUtils::NormalizeWeights(UniversalMesh& mesh) {
	if (!mesh.skinData.has_value()) {
		return;
	}
	
	for (auto& skin : *mesh.skinData) {
		float totalWeight = 0;
		for (const auto& w : skin.weights) {
			totalWeight += w.weight;
		}
		
		if (totalWeight > 0.0001f && std::abs(totalWeight - 1.0f) > 0.001f) {
			float invTotal = 1.0f / totalWeight;
			for (auto& w : skin.weights) {
				w.weight *= invTotal;
			}
		}
	}
}

void SkeletonUtils::PruneWeights(UniversalMesh& mesh, float minWeight) {
	if (!mesh.skinData.has_value()) {
		return;
	}
	
	for (auto& skin : *mesh.skinData) {
		skin.weights.erase(
			std::remove_if(skin.weights.begin(), skin.weights.end(),
						   [minWeight](const BoneWeight& w) { return w.weight < minWeight; }),
			skin.weights.end()
		);
	}
	
	NormalizeWeights(mesh);
}