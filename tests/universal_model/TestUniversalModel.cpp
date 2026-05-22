/*
BodySlide and Outfit Studio - Universal Model Tests
*/

#include <iostream>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstring>

#include <UniversalModel.h>
#include <ImageScanner.h>

using namespace univmodel;

static int testsPassed = 0;
static int testsFailed = 0;

void check(bool condition, const char* file, int line, const char* expr) {
    if (!(condition)) {
        std::cerr << file << ':' << line << " FAILED: " << expr << '\n';
        testsFailed++;
    }
}

#define REQUIRE(expr) check((expr), __FILE__, __LINE__, #expr)

// Test: Vertex creation and data
bool TestVertexCreation() {
    Vertex v;
    v.x = 1.0f; v.y = 2.0f; v.z = 3.0f;
    v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f;
    v.u = 0.5f; v.v = 0.5f;
    v.r = 1.0f; v.g = 0.0f; v.b = 0.0f; v.a = 1.0f;
    v.id = 42;

    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.id == 42);

    return true;
}

// Test: Triangle creation and access
bool TestTriangleCreation() {
    Triangle t;
    t.v1 = 0; t.v2 = 1; t.v3 = 2;
    t.submeshIndex = 0;

    REQUIRE(t.v1 == 0);
    REQUIRE(t.v2 == 1);
    REQUIRE(t.v3 == 2);

    return true;
}

// Test: UniversalMesh bounds computation
bool TestMeshBounds() {
    UniversalMesh mesh;
    
    // Add vertices in a unit cube
    mesh.vertices.push_back({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0});
    mesh.vertices.push_back({1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1});
    mesh.vertices.push_back({1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2});
    mesh.vertices.push_back({0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3});
    
    mesh.ComputeBounds();
    
    REQUIRE(mesh.boundsMin[0] == 0.0f);
    REQUIRE(mesh.boundsMin[1] == 0.0f);
    REQUIRE(mesh.boundsMin[2] == 0.0f);
    REQUIRE(mesh.boundsMax[0] == 1.0f);
    REQUIRE(mesh.boundsMax[1] == 1.0f);
    REQUIRE(mesh.boundsMax[2] == 1.0f);
    
    return true;
}

// Test: ApplyWeld with duplicate vertices
bool TestMeshApplyWeld() {
    UniversalMesh mesh;
    
    // Create 4 vertices where v0, v1, v2 are at same position
    mesh.vertices.push_back({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0});  // canonical
    mesh.vertices.push_back({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1});  // duplicate
    mesh.vertices.push_back({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2});  // duplicate
    mesh.vertices.push_back({0.1f, 0.1f, 0.1f, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3});  // different
    
    // Triangles
    mesh.triangles.push_back({0, 1, 2, 0});
    mesh.triangles.push_back({0, 2, 3, 0});
    
    // Weld map: vertex 0 is canonical, vertices 1 and 2 are duplicates
    std::map<uint32_t, std::vector<uint32_t>> weldMap;
    weldMap[0] = {1, 2};  // vertex 0 absorbs 1 and 2
    
    size_t vertexCountBefore = mesh.vertices.size();
    size_t triCountBefore = mesh.triangles.size();
    
    mesh.ApplyWeld(weldMap);
    
    // After welding, should have 2 vertices (0 kept, 3 kept; 1,2 merged into 0)
    REQUIRE(mesh.vertices.size() == 2);
    REQUIRE(mesh.triangles.size() == triCountBefore);
    
    // Verify all triangle vertex indices are valid (within bounds)
    for (const auto& tri : mesh.triangles) {
        REQUIRE(tri.v1 < mesh.vertices.size());
        REQUIRE(tri.v2 < mesh.vertices.size());
        REQUIRE(tri.v3 < mesh.vertices.size());
    }
    
    // Verify triangles still reference valid vertices (v3 was index 3, now should be 1)
    // After weld: original 0 -> new 0, original 3 -> new 1
    for (const auto& tri : mesh.triangles) {
        // All indices should be 0 or 1 since we only have 2 vertices now
        REQUIRE(tri.v1 <= 1);
        REQUIRE(tri.v2 <= 1);
        REQUIRE(tri.v3 <= 1);
    }
    
    return true;
}

// Test: FormatInfo structure
bool TestFormatInfo() {
    FormatInfo info;
    info.type = FormatType::NIF;
    info.name = "Bethesda NIF";
    info.extension = "nif";
    info.capabilities = {FormatCapability::ImportMeshes, FormatCapability::ExportMeshes};
    info.supportsMultipleMeshes = true;
    info.isBinaryFormat = true;

    REQUIRE(info.type == FormatType::NIF);
    REQUIRE(info.extension == "nif");
    REQUIRE(info.capabilities.size() == 2);

    return true;
}

// Test: FormatRegistry singleton
bool TestFormatRegistry() {
    FormatRegistry& reg1 = FormatRegistry::GetInstance();
    FormatRegistry& reg2 = FormatRegistry::GetInstance();
    
    // Should be the same instance
    REQUIRE(&reg1 == &reg2);
    
    // GetAllFormats should not throw
    std::vector<FormatInfo> formats;
    reg1.GetAllFormats(formats);
    
    // Should have at least one format registered (NIF)
    REQUIRE(formats.size() > 0);
    
    // GetHandler should return valid handler or nullptr
    IFormatHandler* handler = reg1.GetHandler(FormatType::NIF);
    REQUIRE(handler != nullptr);
    
    // Verify the handler's format info
    FormatInfo info = handler->GetFormatInfo();
    REQUIRE(info.type == FormatType::NIF);
    REQUIRE(!info.name.empty());
    
    return true;
}

// Test: MeshUtils::MergeMeshes
bool TestMeshUtilsMerge() {
    UniversalMesh mesh1;
    mesh1.name = "Mesh1";
    mesh1.vertices.push_back({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0});
    mesh1.vertices.push_back({1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1});
    mesh1.vertices.push_back({0.5f, 1.0f, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2}); // valid 3rd vertex
    mesh1.triangles.push_back({0, 1, 2, 0});
    
    UniversalMesh mesh2;
    mesh2.name = "Mesh2";
    mesh2.vertices.push_back({0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3});
    mesh2.vertices.push_back({1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 4});
    
    // Merge
    std::vector<UniversalMesh*> meshes = {&mesh1, &mesh2};
    UniversalMesh merged = MeshUtils::MergeMeshes(meshes);
    
    // Should have 5 vertices
    REQUIRE(merged.vertices.size() == 5);
    
    return true;
}

// Test: Skeleton creation
bool TestSkeletonCreation() {
    Skeleton skeleton = SkeletonUtils::CreateHumanoidSkeleton();
    
    // Humanoid skeleton should have bones
    REQUIRE(skeleton.bones.size() > 0);
    
    // Should have a root bone
    REQUIRE(!skeleton.rootBoneName.empty());
    
    // Verify skeleton has some standard bones (Root, Pelvis, Spine, Head)
    bool hasRoot = false, hasPelvis = false, hasSpine = false, hasHead = false;
    for (const auto& bone : skeleton.bones) {
        if (bone.name == u8"Root") hasRoot = true;
        if (bone.name == u8"Pelvis") hasPelvis = true;
        if (bone.name == u8"Spine") hasSpine = true;
        if (bone.name == u8"Head") hasHead = true;
    }
    REQUIRE(hasRoot);  // Must have Root bone
    REQUIRE(hasPelvis || skeleton.bones.size() >= 5);  // Should have pelvis or substantial skeleton
    
    // Verify root bone is actually named correctly
    REQUIRE(skeleton.rootBoneName == u8"Root");
    
    return true;
}

// Test: DepthMap access
bool TestDepthMapAccess() {
    DepthMap dm;
    dm.Allocate(10, 10);
    
    dm.depth[0] = 1.0f;
    dm.depth[5] = 2.0f;
    dm.depth[99] = 3.0f;
    
    REQUIRE(dm.width == 10);
    REQUIRE(dm.height == 10);
    REQUIRE(dm.depth.size() == 100);
    
    float* ptr = dm.GetDepthPtr(5, 0);
    REQUIRE(ptr != nullptr);
    REQUIRE(*ptr == 2.0f);
    
    const float* invalidPtr = dm.GetDepthPtr(15, 0);
    REQUIRE(invalidPtr == nullptr);
    
    return true;
}

// Test: ImageData validation
bool TestImageDataValidation() {
    ImageData img;
    
    REQUIRE(!img.IsValid());
    
    img.width = 100;
    img.height = 100;
    img.pixels.resize(100 * 100 * 4);
    img.channels = 4;
    
    REQUIRE(img.IsValid());
    
    return true;
}

// Main test runner
int main() {
    std::cout << "=== Universal Model Unit Tests ===" << std::endl << std::endl;
    
    auto runTest = [&](const char* name, bool (*test)()) {
        std::cout << "Running: " << name << "... ";
        if (test()) {
            std::cout << "PASSED" << std::endl;
            testsPassed++;
        } else {
            std::cout << "FAILED" << std::endl;
            testsFailed++;
        }
    };
    
    runTest("Vertex Creation", TestVertexCreation);
    runTest("Triangle Creation", TestTriangleCreation);
    runTest("Mesh Bounds", TestMeshBounds);
    runTest("Mesh ApplyWeld", TestMeshApplyWeld);
    runTest("FormatInfo", TestFormatInfo);
    runTest("FormatRegistry", TestFormatRegistry);
    runTest("MeshUtils Merge", TestMeshUtilsMerge);
    runTest("Skeleton Creation", TestSkeletonCreation);
    runTest("DepthMap Access", TestDepthMapAccess);
    runTest("ImageData Validation", TestImageDataValidation);
    
    std::cout << std::endl << "=== Results: " << testsPassed << " passed, " << testsFailed << " failed ===" << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}
