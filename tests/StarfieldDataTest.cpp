#include "../src/components/StarfieldSpace.h"
#include "../src/files/GameDataStream.h"
#include "../src/files/SFMaterialResolver.h"
#include "../src/files/SFMorphFile.h"
#include "../src/utils/ConfigurationManager.h"

#include "FSEngine/FSManager.h"
#include "NifFile.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using Catch::Approx;
using namespace nifly;
namespace fs = std::filesystem;

// GameDataStream reads the game data path from the app configuration
ConfigurationManager Config;

namespace {
// Vanilla files from the Starfield archives (the MeshesPatch versions the game loads), laid out
// like the game's Data folder: the female body and its swimsuit, their .mesh geometry and the
// chargen morph.dat of both. The swimsuit is the clothing case: a second skinned shape made to
// fit the body, with its own morphs.
const std::string dataDir = std::string(BSOS_TEST_DATA_DIR) + "/starfield/";

struct VanillaMesh {
	const char* label;
	std::string nifPath;
	std::string meshFile;
	std::string morphPath;
	const char* meshRef;
	const char* shapeName;
	const char* materialPath;
	uint32_t vertCount;
	size_t triCount;
	std::vector<size_t> lodTriCounts;
	size_t meshletCount;
	size_t boneCount;
	bool hasVertexColors;
};

const VanillaMesh& Body() {
	static const VanillaMesh body{"naked_f",
								  dataDir + "meshes/actors/human/mesh/naked_body/naked_f.nif",
								  dataDir + "geometries/8e80bdf6c020bc22765a/9cfc9d401dba9c2dcfa1.mesh",
								  dataDir + "meshes/morphs/human/female/chargen/body/morph.dat",
								  "8e80bdf6c020bc22765a\\9cfc9d401dba9c2dcfa1",
								  "Naked_F:0",
								  "Materials\\Actors\\Human\\Naked_Body\\Female\\Naked_F_Body.mat",
								  6616,
								  12132,
								  {1492, 1008},
								  177,
								  38,
								  true};
	return body;
}

const VanillaMesh& Swimsuit() {
	static const VanillaMesh swimsuit{"naked_f_swimsuit",
									  dataDir + "meshes/actors/human/mesh/naked_body/naked_f_swimsuit.nif",
									  dataDir + "geometries/e77c6245b8ffa6555a9a/84e3f6cbd3643497c56e.mesh",
									  dataDir + "meshes/morphs/clothes/naked/naked_f/chargen/naked_f_swimsuit/morph.dat",
									  "e77c6245b8ffa6555a9a\\84e3f6cbd3643497c56e",
									  "Naked_F_Swimsuit:0",
									  "Materials\\Actors\\Human\\Naked_Body\\Female\\Naked_F_Body_Swimsuit.mat",
									  2638,
									  2338,
									  {1112},
									  38,
									  17,
									  false};
	return swimsuit;
}

std::string ReadFile(const std::string& path) {
	std::ifstream file(fs::u8path(path), std::ios::binary);
	REQUIRE(file.is_open());
	return std::string(std::istreambuf_iterator<char>(file), {});
}

// Loads a NIF and its external geometry the way Outfit Studio and the BodySlide preview do
std::unique_ptr<NifFile> LoadWithGeometry(const VanillaMesh& mesh) {
	auto nif = std::make_unique<NifFile>();
	REQUIRE(nif->Load(fs::u8path(mesh.nifPath)) == 0);
	REQUIRE(GameDataStream::LoadExternalGeometry(*nif, dataDir, mesh.nifPath).empty());
	REQUIRE(nif->GetShapes().size() == 1);
	return nif;
}

BSGeometryMeshData& MeshData(NiShape* shape) {
	auto bsGeom = dynamic_cast<BSGeometry*>(shape);
	REQUIRE(bsGeom);
	auto meshData = dynamic_cast<BSGeometryMeshData*>(bsGeom->GetGeomData());
	REQUIRE(meshData);
	return *meshData;
}

std::string SaveMesh(NifFile& nif, NiShape* shape) {
	std::ostringstream stream(std::ios::binary);
	REQUIRE(nif.SaveExternalShapeData(shape, stream, 0));
	return stream.str();
}

std::string SaveNif(NifFile& nif) {
	std::ostringstream stream(std::ios::binary);
	REQUIRE(nif.Save(stream) == 0);
	return stream.str();
}

void RequireTrianglesInRange(const std::vector<Triangle>& tris, const size_t vertCount) {
	for (auto& t : tris) {
		REQUIRE(t.p1 < vertCount);
		REQUIRE(t.p2 < vertCount);
		REQUIRE(t.p3 < vertCount);
	}
}

// Meshlets have to cover every triangle once, within the mesh shader limits they were built for
void RequireMeshletsCoverTriangles(const BSGeometryMeshData& meshData) {
	REQUIRE(!meshData.meshletList.empty());
	REQUIRE(meshData.cullDataList.size() == meshData.meshletList.size());

	size_t primitives = 0;
	uint32_t primitiveOffset = 0;
	for (auto& meshlet : meshData.meshletList) {
		REQUIRE(meshlet.vertCount <= 128);
		REQUIRE(meshlet.primCount <= 128);
		// Three local indices per primitive; each meshlet's range is aligned to four.
		REQUIRE(meshlet.primOffset == primitiveOffset);
		primitiveOffset += (meshlet.primCount * 3 + 3) & ~3u;
		primitives += meshlet.primCount;
	}
	REQUIRE(primitives == meshData.tris.size());
}

// Sum of all bone weights of every vertex
std::vector<float> WeightSums(NifFile& nif, NiShape* shape, const size_t vertCount) {
	std::vector<std::string> bones;
	nif.GetShapeBoneList(shape, bones);

	std::vector<float> sums(vertCount, 0.0f);
	for (uint32_t bone = 0; bone < bones.size(); bone++) {
		std::unordered_map<uint16_t, float> weights;
		nif.GetShapeBoneWeights(shape, bone, weights);
		for (auto& [vertex, weight] : weights) {
			REQUIRE(vertex < vertCount);
			sums[vertex] += weight;
		}
	}
	return sums;
}

SFMorphFile ReadMorphs(const std::string& path) {
	// Same sequence as the Outfit Studio import
	SFMorphFile morphs;
	REQUIRE(morphs.Read(path));
	REQUIRE(morphs.FileToCacheData());
	morphs.UpdateCachedMorphData();
	return morphs;
}

struct TempDir {
	fs::path path;

	explicit TempDir(const char* name) {
		path = fs::temp_directory_path() / name;
		std::error_code ec;
		fs::remove_all(path, ec);
		fs::create_directories(path);
	}

	~TempDir() {
		std::error_code ec;
		fs::remove_all(path, ec);
	}
};

// Area weighted vertex normals of a triangle list
std::vector<Vector3> FaceNormalsPerVertex(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris) {
	std::vector<Vector3> normals(verts.size());
	for (auto& t : tris) {
		const Vector3 n = (verts[t.p2] - verts[t.p1]).cross(verts[t.p3] - verts[t.p1]);
		normals[t.p1] += n;
		normals[t.p2] += n;
		normals[t.p3] += n;
	}
	for (auto& n : normals)
		n.Normalize();
	return normals;
}
} // namespace

TEST_CASE("Vanilla Starfield meshes load their external geometry", "[starfield][gamedata]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	NifFile nif;
	REQUIRE(nif.Load(fs::u8path(mesh.nifPath)) == 0);
	REQUIRE(nif.GetHeader().GetVersion().IsSF());
	REQUIRE(nif.GetHeader().GetVersion().Stream() == 175);

	auto shapes = nif.GetShapes();
	REQUIRE(shapes.size() == 1);
	NiShape* shape = shapes[0];
	REQUIRE(shape->name.get() == mesh.shapeName);
	REQUIRE(shape->IsSkinned());

	auto bsGeom = dynamic_cast<BSGeometry*>(shape);
	REQUIRE(bsGeom);
	REQUIRE(!bsGeom->HasInternalGeomData());

	auto meshPaths = nif.GetExternalGeometryPathRefs(shape);
	REQUIRE(meshPaths.size() == 1);
	REQUIRE(meshPaths[0].get() == mesh.meshRef);

	auto shader = nif.GetShader(shape);
	REQUIRE(shader);
	REQUIRE(shader->name.get() == mesh.materialPath);

	REQUIRE(GameDataStream::LoadExternalGeometry(nif, dataDir, mesh.nifPath).empty());

	const BSGeometryMeshData& meshData = MeshData(shape);
	REQUIRE(meshData.nVertices == mesh.vertCount);
	REQUIRE(meshData.vertices.size() == mesh.vertCount);
	REQUIRE(meshData.normals.size() == mesh.vertCount);
	REQUIRE(meshData.tangents.size() == mesh.vertCount);
	REQUIRE(meshData.vColors.size() == (mesh.hasVertexColors ? mesh.vertCount : 0));
	// Both UV streams are read, but these meshes only use the first
	REQUIRE(meshData.uvSets.size() == 2);
	REQUIRE(meshData.uvSets[0].size() == mesh.vertCount);
	REQUIRE(meshData.uvSets[1].empty());

	REQUIRE(meshData.tris.size() == mesh.triCount);
	RequireTrianglesInRange(meshData.tris, mesh.vertCount);

	// LOD levels live in the .mesh as reduced triangle lists over the same vertices
	REQUIRE(meshData.lods.size() == mesh.lodTriCounts.size());
	for (size_t lod = 0; lod < meshData.lods.size(); lod++) {
		REQUIRE(meshData.lods[lod].size() == mesh.lodTriCounts[lod]);
		RequireTrianglesInRange(meshData.lods[lod], mesh.vertCount);
	}

	REQUIRE(meshData.meshletList.size() == mesh.meshletCount);
	RequireMeshletsCoverTriangles(meshData);

	std::vector<std::string> bones;
	REQUIRE(nif.GetShapeBoneList(shape, bones) == mesh.boneCount);
	for (float sum : WeightSums(nif, shape, mesh.vertCount))
		REQUIRE(sum == Approx(1.0f).margin(0.01f));

	// Vertices are scaled from meters to the havokScale units of the older games
	float minZ = meshData.vertices[0].z, maxZ = minZ;
	for (auto& v : meshData.vertices) {
		minZ = std::min(minZ, v.z);
		maxZ = std::max(maxZ, v.z);
	}
	REQUIRE(maxZ - minZ > 10.0f);
}

TEST_CASE("External Starfield geometry is found beside the meshes folder", "[starfield][gamedata]") {
	const VanillaMesh& body = Body();
	NifFile nif;
	REQUIRE(nif.Load(fs::u8path(body.nifPath)) == 0);

	SECTION("without a data folder, by the NIF's meshes folder") {
		REQUIRE(GameDataStream::LoadExternalGeometry(nif, "", body.nifPath).empty());
		REQUIRE(nif.GetShapes()[0]->GetNumVertices() == body.vertCount);
	}

	SECTION("missing geometry is reported and leaves the shape empty") {
		const auto missing = GameDataStream::LoadExternalGeometry(nif, dataDir + "missing/", "");
		REQUIRE(missing.size() == 1);
		REQUIRE(missing[0] == body.meshRef);
		REQUIRE(nif.GetShapes()[0]->GetNumVertices() == 0);
	}
}

TEST_CASE("Vanilla Starfield meshes save back byte for byte with external geometry", "[starfield][gamedata]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	auto nif = LoadWithGeometry(mesh);
	NiShape* shape = nif->GetShapes()[0];

	REQUIRE(SaveMesh(*nif, shape) == ReadFile(mesh.meshFile));
	REQUIRE(SaveNif(*nif) == ReadFile(mesh.nifPath));
}

TEST_CASE("Vanilla Starfield meshes convert to internal geometry and back", "[starfield][gamedata]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	auto nif = LoadWithGeometry(mesh);
	NiShape* shape = nif->GetShapes()[0];
	const BSGeometryMeshData original = MeshData(shape);
	const std::vector<float> originalWeights = WeightSums(*nif, shape, mesh.vertCount);

	auto bsGeom = dynamic_cast<BSGeometry*>(shape);
	bsGeom->SetInternalGeomData(true);
	const std::string internalNif = SaveNif(*nif);

	// The geometry is part of the NIF now and needs no .mesh to load
	NifFile internal;
	std::istringstream internalStream(internalNif, std::ios::binary);
	REQUIRE(internal.Load(internalStream) == 0);

	NiShape* internalShape = internal.GetShapes()[0];
	auto internalGeom = dynamic_cast<BSGeometry*>(internalShape);
	REQUIRE(internalGeom);
	REQUIRE(internalGeom->HasInternalGeomData());
	REQUIRE(internal.GetExternalGeometryPathRefs(internalShape).empty());

	const BSGeometryMeshData& loaded = MeshData(internalShape);
	REQUIRE(loaded.vertices.size() == original.vertices.size());
	for (size_t i = 0; i < original.vertices.size(); i++)
		REQUIRE(loaded.vertices[i] == original.vertices[i]);
	REQUIRE(loaded.tris == original.tris);
	REQUIRE(loaded.lods == original.lods);
	REQUIRE(loaded.meshletList.size() == original.meshletList.size());
	REQUIRE(loaded.vColors.size() == original.vColors.size());
	REQUIRE(WeightSums(internal, internalShape, mesh.vertCount) == originalWeights);

	REQUIRE(SaveNif(internal) == internalNif);

	// Switching back needs a .mesh path again, which internal geometry doesn't keep. Outfit Studio
	// generates one on export, so give it the original one for comparison.
	internalGeom->SetInternalGeomData(false);
	auto meshPaths = internal.GetExternalGeometryPathRefs(internalShape);
	REQUIRE(meshPaths.size() == 1);
	REQUIRE(meshPaths[0].get().empty());
	meshPaths[0].get() = mesh.meshRef;

	REQUIRE(SaveMesh(internal, internalShape) == ReadFile(mesh.meshFile));
	REQUIRE(SaveNif(internal) == ReadFile(mesh.nifPath));
}

TEST_CASE("Deleting vertices of a vanilla Starfield mesh keeps its LODs and meshlets valid", "[starfield][gamedata]") {
	const VanillaMesh& body = Body();
	auto nif = LoadWithGeometry(body);
	NiShape* shape = nif->GetShapes()[0];

	std::vector<uint16_t> deleteIndices;
	for (uint16_t i = 0; i < body.vertCount; i += 50)
		deleteIndices.push_back(i);
	REQUIRE(!nif->DeleteVertsForShape(shape, deleteIndices));

	const uint32_t remaining = body.vertCount - static_cast<uint32_t>(deleteIndices.size());
	{
		const BSGeometryMeshData& meshData = MeshData(shape);
		REQUIRE(meshData.nVertices == remaining);
		REQUIRE(meshData.vColors.size() == remaining);
		REQUIRE(meshData.uvSets[0].size() == remaining);
		RequireTrianglesInRange(meshData.tris, remaining);
		REQUIRE(meshData.lods.size() == body.lodTriCounts.size());
		for (auto& lod : meshData.lods)
			RequireTrianglesInRange(lod, remaining);
		RequireMeshletsCoverTriangles(meshData);
		REQUIRE(WeightSums(*nif, shape, remaining).size() == remaining);
	}

	// The edited geometry saves to a .mesh that loads back the same
	const std::string meshFile = SaveMesh(*nif, shape);
	const std::vector<Triangle> savedTris = MeshData(shape).tris;
	const auto savedLods = MeshData(shape).lods;

	NifFile reloaded;
	REQUIRE(reloaded.Load(fs::u8path(body.nifPath)) == 0);
	std::istringstream meshStream(meshFile, std::ios::binary);
	NiShape* reloadedShape = reloaded.GetShapes()[0];
	REQUIRE(reloaded.LoadExternalShapeData(reloadedShape, meshStream, 0));

	const BSGeometryMeshData& meshData = MeshData(reloadedShape);
	REQUIRE(meshData.nVertices == remaining);
	REQUIRE(meshData.tris == savedTris);
	REQUIRE(meshData.lods == savedLods);
	RequireMeshletsCoverTriangles(meshData);
}

TEST_CASE("Meshlets are generated for Starfield meshes missing them", "[starfield][gamedata]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	auto nif = LoadWithGeometry(mesh);
	auto bsGeom = dynamic_cast<BSGeometry*>(nif->GetShapes()[0]);
	BSGeometryMeshData& meshData = MeshData(bsGeom);

	meshData.meshletList.clear();
	meshData.cullDataList.clear();
	REQUIRE(!bsGeom->HasMeshlets());

	bsGeom->GenerateMeshlets();
	REQUIRE(bsGeom->HasMeshlets());
	RequireMeshletsCoverTriangles(meshData);

	// Intact meshlets are kept
	const auto generated = meshData.meshletList.size();
	bsGeom->GenerateMeshlets(64, 64);
	REQUIRE(meshData.meshletList.size() == generated);
}

TEST_CASE("Vanilla Starfield morph.dat files match their meshes", "[starfield][gamedata][SFMorphFile]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	SFMorphFile morphs = ReadMorphs(mesh.morphPath);
	REQUIRE(morphs.GetMorphNames() == std::vector<std::string>{"Overweight", "Thin", "Strong"});
	REQUIRE(morphs.morphOffsetsCache.size() == 3);

	for (size_t m = 0; m < morphs.morphOffsetsCache.size(); m++) {
		INFO("morph " << m);
		const auto& offsets = morphs.morphOffsetsCache[m];
		REQUIRE(!offsets.empty());
		for (auto& [vertex, offset] : offsets) {
			REQUIRE(vertex < mesh.vertCount);
			REQUIRE(morphs.morphNormalsCache[m].count(vertex) == 1);
			REQUIRE(morphs.morphTangentsCache[m].count(vertex) == 1);
			REQUIRE(morphs.morphColorsCache[m].count(vertex) == 1);
		}
	}
}

TEST_CASE("Vanilla Starfield morph.dat files are written back byte for byte", "[starfield][gamedata][SFMorphFile]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	SFMorphFile morphs = ReadMorphs(mesh.morphPath);

	// Rebuilt from the cached morphs the way the exporters fill a new file
	SFMorphFile rebuilt;
	rebuilt.SetVertexCount(mesh.vertCount);
	const auto names = morphs.GetMorphNames();
	for (size_t m = 0; m < names.size(); m++)
		REQUIRE(rebuilt.AddMorph(names[m], morphs.morphOffsetsCache[m], morphs.morphColorsCache[m], morphs.morphNormalsCache[m], morphs.morphTangentsCache[m]));
	rebuilt.CacheToFileData();

	TempDir temp("bsos_sf_morph_rewrite");
	const std::string outPath = (temp.path / "morph.dat").u8string();
	REQUIRE(rebuilt.Write(outPath));
	REQUIRE(ReadFile(outPath) == ReadFile(mesh.morphPath));
}

TEST_CASE("Vanilla Starfield morph normals are deltas from the base normals", "[starfield][gamedata][SFMorphFile]") {
	// morph.dat stores how much each normal changes, not the morphed normal. Exporters have to
	// write the difference to the shape's normals.
	const VanillaMesh& body = Body();
	auto nif = LoadWithGeometry(body);
	NiShape* shape = nif->GetShapes()[0];

	const std::vector<Vector3> baseVerts = *nif->GetVertsForShape(shape);
	const std::vector<Vector3> baseNormals = *nif->GetNormalsForShape(shape);
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	SFMorphFile morphs = ReadMorphs(body.morphPath);
	for (size_t m = 0; m < morphs.morphOffsetsCache.size(); m++) {
		INFO("morph " << m);
		std::vector<Vector3> morphedVerts = baseVerts;
		for (auto& [vertex, offset] : morphs.morphOffsetsCache[m])
			morphedVerts[vertex] += offset;
		const std::vector<Vector3> morphedGeometryNormals = FaceNormalsPerVertex(morphedVerts, tris);

		double baseMatch = 0.0, deltaMatch = 0.0, absoluteMatch = 0.0;
		for (auto& [vertex, normal] : morphs.morphNormalsCache[m]) {
			Vector3 baseWithDelta = baseNormals[vertex] + normal;
			baseWithDelta.Normalize();
			Vector3 asAbsolute = normal;
			asAbsolute.Normalize();

			const Vector3& expected = morphedGeometryNormals[vertex];
			baseMatch += baseNormals[vertex].dot(expected);
			deltaMatch += baseWithDelta.dot(expected);
			absoluteMatch += asAbsolute.dot(expected);
		}

		const double count = static_cast<double>(morphs.morphNormalsCache[m].size());
		baseMatch /= count;
		deltaMatch /= count;
		absoluteMatch /= count;

		REQUIRE(deltaMatch > 0.99);
		REQUIRE(deltaMatch > baseMatch);
		REQUIRE(absoluteMatch < 0.5);
	}
}

TEST_CASE("Starfield morph exports derive the normal deltas vanilla morphs store", "[starfield][gamedata][SFMorphFile]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	auto nif = LoadWithGeometry(mesh);
	NiShape* shape = nif->GetShapes()[0];
	const SFMorphShape morphShape = SFMorphShape::FromShape(*nif, shape);

	const std::vector<Vector3> fileNormals = *nif->GetNormalsForShape(shape);
	const std::vector<Vector3> fileTangents = *nif->GetTangentsForShape(shape);

	// Exported again from nothing but the vanilla position offsets
	SFMorphFile vanilla = ReadMorphs(mesh.morphPath);
	for (size_t m = 0; m < vanilla.morphOffsetsCache.size(); m++) {
		INFO("morph " << m);
		const auto& offsets = vanilla.morphOffsetsCache[m];

		std::unordered_map<uint16_t, Vector3> normals, tangents;
		morphShape.CalcNormalDeltas(offsets, normals, tangents);
		REQUIRE(normals.size() == offsets.size());
		REQUIRE(tangents.size() == offsets.size());

		// Applied to the shape's own normals, both deltas give the same morphed normal
		auto agreement = [&](const std::vector<Vector3>& base, const std::unordered_map<uint16_t, Vector3>& ours, const std::unordered_map<uint16_t, Vector3>& theirs) {
			double sum = 0.0;
			for (auto& [vertex, delta] : ours) {
				Vector3 a = base[vertex] + delta;
				Vector3 b = base[vertex] + theirs.at(vertex);
				a.Normalize();
				b.Normalize();
				sum += a.dot(b);
			}
			return sum / static_cast<double>(ours.size());
		};

		std::unordered_map<uint16_t, Vector3> unchanged;
		for (auto& [vertex, delta] : normals)
			unchanged[vertex] = Vector3();

		const double normalAgreement = agreement(fileNormals, normals, vanilla.morphNormalsCache[m]);
		const double tangentAgreement = agreement(fileTangents, tangents, vanilla.morphTangentsCache[m]);
		const double unchangedNormalAgreement = agreement(fileNormals, unchanged, vanilla.morphNormalsCache[m]);
		INFO("normals " << normalAgreement << " (unchanged " << unchangedNormalAgreement << "), tangents " << tangentAgreement);

		// The body matches closely. The swimsuit's vanilla normals were made a little differently
		// (they don't come from the nearest body vertices either), but still much closer than
		// leaving the normals as they are.
		REQUIRE(normalAgreement > unchangedNormalAgreement);
		if (&mesh == &Body()) {
			REQUIRE(normalAgreement > 0.99);
			REQUIRE(tangentAgreement > 0.99);
		}
		else {
			REQUIRE(normalAgreement > 0.97);
			REQUIRE(tangentAgreement > 0.95);
		}
	}
}

TEST_CASE("Starfield morph normal deltas follow the shape's seam smoothing and locked normals", "[starfield][gamedata][SFMorphFile]") {
	const VanillaMesh& body = Body();
	auto nif = LoadWithGeometry(body);
	NiShape* shape = nif->GetShapes()[0];
	const SFMorphFile morphs = ReadMorphs(body.morphPath);
	const auto& offsets = morphs.morphOffsetsCache[0];

	auto deltas = [&](const SFMorphNormalSettings& settings) {
		std::unordered_map<uint16_t, Vector3> normals, tangents;
		SFMorphShape::FromShape(*nif, shape, settings).CalcNormalDeltas(offsets, normals, tangents);
		REQUIRE(normals.size() == offsets.size());
		REQUIRE(tangents.size() == offsets.size());
		return std::make_pair(normals, tangents);
	};

	// Morphed vertices split along seams, that moved together
	const std::vector<Vector3> verts = *nif->GetVertsForShape(shape);
	std::map<std::tuple<float, float, float>, std::vector<uint16_t>> byPosition;
	for (auto& [vertex, offset] : offsets)
		byPosition[std::make_tuple(verts[vertex].x, verts[vertex].y, verts[vertex].z)].push_back(vertex);

	std::vector<std::pair<uint16_t, uint16_t>> seamPairs;
	for (auto& [position, group] : byPosition)
		if (group.size() > 1 && offsets.at(group[0]) == offsets.at(group[1]))
			seamPairs.emplace_back(group[0], group[1]);
	REQUIRE(seamPairs.size() > 10);

	SFMorphNormalSettings settings;
	auto countSameAcrossSeams = [&](const std::unordered_map<uint16_t, Vector3>& normals) {
		size_t same = 0;
		for (auto& [a, b] : seamPairs)
			if (normals.at(a).IsNearlyEqualTo(normals.at(b)))
				same++;
		return same;
	};

	SECTION("seams smoothed at any angle change the same on both sides") {
		settings.smoothSeamNormalsAngle = 180.0f;
		REQUIRE(countSameAcrossSeams(deltas(settings).first) == seamPairs.size());
	}

	SECTION("without seam smoothing, both sides change on their own") {
		settings.smoothSeamNormals = false;
		const auto unsmoothed = deltas(settings).first;
		REQUIRE(countSameAcrossSeams(unsmoothed) < seamPairs.size());

		// A zero angle smooths nothing either
		settings.smoothSeamNormals = true;
		settings.smoothSeamNormalsAngle = 0.0f;
		for (auto& [vertex, delta] : deltas(settings).first)
			REQUIRE(delta.IsNearlyEqualTo(unsmoothed.at(vertex)));
	}

	SECTION("locked normals of the shape don't change") {
		settings.lockNormals = true;
		const auto [normals, tangents] = deltas(settings);
		for (auto& [vertex, delta] : normals) {
			REQUIRE(delta.IsZero());
			REQUIRE(tangents.at(vertex).IsZero());
		}
	}

	SECTION("locked vertex normals don't change") {
		for (auto& [vertex, offset] : offsets)
			if (vertex % 2 == 0)
				settings.lockedNormals.insert(vertex);

		const auto [normals, tangents] = deltas(settings);
		size_t changed = 0;
		for (auto& [vertex, delta] : normals) {
			if (vertex % 2 == 0) {
				REQUIRE(delta.IsZero());
				REQUIRE(tangents.at(vertex).IsZero());
			}
			else if (!delta.IsZero(true))
				changed++;
		}
		REQUIRE(changed > normals.size() / 4);
	}
}

TEST_CASE("Starfield morph exports keep the vertex colors like vanilla morphs", "[starfield][gamedata][SFMorphFile]") {
	const VanillaMesh& mesh = GENERATE(std::cref(Body()), std::cref(Swimsuit()));
	INFO(mesh.label);

	auto nif = LoadWithGeometry(mesh);
	const SFMorphShape morphShape = SFMorphShape::FromShape(*nif, nif->GetShapes()[0]);

	SFMorphFile vanilla = ReadMorphs(mesh.morphPath);
	SFMorphFile exported;
	exported.SetVertexCount(mesh.vertCount);
	const auto names = vanilla.GetMorphNames();
	for (size_t m = 0; m < names.size(); m++)
		REQUIRE(morphShape.AddMorph(exported, names[m], vanilla.morphOffsetsCache[m]));
	exported.CacheToFileData();

	TempDir temp("bsos_sf_morph_export");
	const std::string outPath = (temp.path / "morph.dat").u8string();
	REQUIRE(exported.Write(outPath));

	// The color of the body is stored as its vertex color, the swimsuit has none and stores black
	SFMorphFile reloaded = ReadMorphs(outPath);
	REQUIRE(reloaded.GetMorphNames() == names);
	for (size_t m = 0; m < names.size(); m++) {
		INFO("morph " << m);
		REQUIRE(reloaded.morphOffsetsCache[m] == vanilla.morphOffsetsCache[m]);
		REQUIRE(reloaded.morphColorsCache[m].size() == vanilla.morphColorsCache[m].size());
		for (auto& [vertex, color] : vanilla.morphColorsCache[m]) {
			const Color3& ours = reloaded.morphColorsCache[m].at(vertex);
			REQUIRE(ours.r == color.r);
			REQUIRE(ours.g == color.g);
			REQUIRE(ours.b == color.b);
		}
	}
}

TEST_CASE("Starfield morph colors round-trip through RGB565", "[starfield][SFMorphFile]") {
	// Every 5 and 6 bit step decodes to a color that encodes back to the same step
	std::unordered_map<uint16_t, Vector3> offsets;
	std::unordered_map<uint16_t, Color3> colors;
	for (uint16_t step = 0; step < 64; step++) {
		offsets[step] = Vector3(1.0f, 0.0f, 0.0f);
		colors[step] = Color3((step % 32) / 31.0f, step / 63.0f, (31 - step % 32) / 31.0f);
	}

	SFMorphFile file;
	file.SetVertexCount(64);
	REQUIRE(file.AddMorph("Colors", offsets, colors, {}, {}));
	file.CacheToFileData();

	TempDir temp("bsos_sf_morph_colors");
	const std::string firstPath = (temp.path / "first.dat").u8string();
	REQUIRE(file.Write(firstPath));

	SFMorphFile first = ReadMorphs(firstPath);
	for (auto& [vertex, color] : colors) {
		const Color3& decoded = first.morphColorsCache[0].at(vertex);
		REQUIRE(decoded.r == Approx(color.r).margin(0.5f / 255.0f));
		REQUIRE(decoded.g == Approx(color.g).margin(0.5f / 255.0f));
		REQUIRE(decoded.b == Approx(color.b).margin(0.5f / 255.0f));
	}

	SFMorphFile second;
	second.SetVertexCount(64);
	REQUIRE(second.AddMorph("Colors", first.morphOffsetsCache[0], first.morphColorsCache[0], {}, {}));
	second.CacheToFileData();
	const std::string secondPath = (temp.path / "second.dat").u8string();
	REQUIRE(second.Write(secondPath));
	REQUIRE(ReadFile(secondPath) == ReadFile(firstPath));
}

TEST_CASE("Vanilla Starfield body and swimsuit share a skin space for conforming", "[starfield][gamedata][conform]") {
	auto body = LoadWithGeometry(Body());
	auto swimsuit = LoadWithGeometry(Swimsuit());
	NiShape* bodyShape = body->GetShapes()[0];
	NiShape* swimsuitShape = swimsuit->GetShapes()[0];

	// Every bone the swimsuit is weighted to binds it the same way it binds the body, so both skin
	// spaces are the same and conforming can match them vertex by vertex
	std::vector<std::string> bodyBones, swimsuitBones;
	body->GetShapeBoneList(bodyShape, bodyBones);
	swimsuit->GetShapeBoneList(swimsuitShape, swimsuitBones);
	for (uint32_t s = 0; s < swimsuitBones.size(); s++) {
		INFO(swimsuitBones[s]);
		auto it = std::find(bodyBones.begin(), bodyBones.end(), swimsuitBones[s]);
		REQUIRE(it != bodyBones.end());

		MatTransform bodySkinToBone, swimsuitSkinToBone;
		REQUIRE(body->GetShapeTransformSkinToBone(bodyShape, static_cast<uint32_t>(it - bodyBones.begin()), bodySkinToBone));
		REQUIRE(swimsuit->GetShapeTransformSkinToBone(swimsuitShape, s, swimsuitSkinToBone));
		REQUIRE(bodySkinToBone.IsNearlyEqualTo(swimsuitSkinToBone));
	}

	// Placed in conform space with the same skin transform, the swimsuit sits on the body surface
	MatTransform skinToGlobal;
	REQUIRE(body->GetShapeTransformSkinToBone(bodyShape, 0, skinToGlobal));
	skinToGlobal = skinToGlobal.InverseTransform();
	const MatTransform conformXform = ShapeToGlobalForConform(skinToGlobal, true, true);

	std::vector<Vector3> bodyVerts = *body->GetVertsForShape(bodyShape);
	for (auto& v : bodyVerts)
		v = conformXform.ApplyTransform(v);

	std::vector<float> distances;
	for (auto v : *swimsuit->GetVertsForShape(swimsuitShape)) {
		v = conformXform.ApplyTransform(v);
		float nearest = std::numeric_limits<float>::max();
		for (auto& b : bodyVerts)
			nearest = std::min(nearest, v.DistanceTo(b));
		distances.push_back(nearest);
	}
	std::sort(distances.begin(), distances.end());
	const float median = distances[distances.size() / 2];
	const float p90 = distances[distances.size() * 90 / 100];
	INFO("median " << median << ", 90th percentile " << p90);
	// Measured to the nearest body vertex rather than the surface, and a few swimsuit edges stand off
	REQUIRE(median < 0.5f);
	REQUIRE(p90 < 1.5f);
}

TEST_CASE("Starfield game archives provide the vanilla body and its layered material", "[starfield][gamedata][fixture]") {
	// The game's Data folder. Its archives can't be part of the repository, so this only runs on request.
	const char* gameData = std::getenv("BSOS_SF_DATA_DIR");
	if (!gameData || !gameData[0])
		SKIP("BSOS_SF_DATA_DIR is not set");

	const fs::path gameDir = fs::u8path(gameData);
	std::vector<std::string> archives;
	for (const char* name : {"Starfield - Meshes01.ba2", "Starfield - Meshes02.ba2", "Starfield - MeshesPatch.ba2", "Starfield - Materials.ba2"}) {
		REQUIRE(fs::exists(gameDir / name));
		archives.push_back((gameDir / name).u8string());
	}
	FSManager::addArchives(archives);

	// A data folder without loose files, so that everything comes from the archives
	const std::string noLooseFiles = dataDir + "missing/";

	for (const VanillaMesh* mesh : {&Body(), &Swimsuit()}) {
		INFO(mesh->label);
		const std::string relPath = "meshes/actors/human/mesh/naked_body/" + fs::u8path(mesh->nifPath).filename().u8string();
		auto nifStream = GameDataStream::OpenArchive(relPath);
		REQUIRE(nifStream);

		NifFile nif;
		REQUIRE(nif.Load(*nifStream) == 0);
		REQUIRE(GameDataStream::LoadExternalGeometry(nif, noLooseFiles, "").empty());
		NiShape* shape = nif.GetShapes()[0];
		REQUIRE(shape->GetNumVertices() == mesh->vertCount);

		// Vanilla materials only exist in the material database of the archives
		SFMaterialResolver resolver;
		SFLayeredMaterial material;
		REQUIRE(resolver.Resolve(nif.GetShader(shape)->name.get(), noLooseFiles, material));
		REQUIRE(!material.layers.empty());
		REQUIRE(!material.layers[0].textureSet.files[0].empty());
	}

	FSManager::del();
}
