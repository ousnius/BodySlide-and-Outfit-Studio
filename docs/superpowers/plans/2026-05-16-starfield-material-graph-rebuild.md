# Starfield Material Graph Rebuild Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current Starfield material texture resolver with a graph-based resolver that selects the intended primary material texture set instead of random secondary layer, face-detail, or blender textures.

**Architecture:** Keep slice 1's loose `.mat` UI and file loading work, but rewrite the texture extraction logic around a shared Starfield material graph model. Both loose JSON `.mat` files and archived `.cdb` records should feed the same resolver: start at the requested material object, follow `LayerID[0] -> MaterialID -> TextureSetID`, collect texture files only from that selected texture set subtree, and ignore secondary layers/blenders for Outfit Studio's simple texture grid.

**Tech Stack:** C++17, nlohmann/json, BodySlide/Outfit Studio archive layer, Starfield reflection CDB format, lightweight `g++` tests, Release x64 MSBuild.

---

## Ground Truth From Exported Material JSON

### Body: `naked_f_body.mat`

The reference material export at `C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\resources\Materials\Actors\Human\Naked_Body\Female\naked_f_body.mat` has this relevant graph:

- Root object `naked_f_body`
  - `BSMaterial::LayerID[0]` -> `Naked_F_Body_Layer1`
    - `BSMaterial::MaterialID[0]` -> `Naked_F_Body_Material1`
      - `BSMaterial::TextureSetID[0]` -> `Naked_F_Body_TextureSet1`
        - Slot 0: `textures/actors/human/naked_body/nakedbodyf_sk3_color.dds`
        - Slot 1: `textures/actors/human/naked_body/nakedbodyf_normal.dds`
        - Slot 3: `textures/actors/human/naked_body/nakedbodyf_rough.dds`
        - Slot 5: `textures/actors/human/naked_body/nakedbodyf_ao.dds`
  - `BSMaterial::LayerID[1]` -> secondary layer
    - Contains `textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds`
  - `BSMaterial::BlenderID[0]` -> blender
    - Contains `textures/actors/human/naked_body/nakedbodyf_mask.dds`

For the current Outfit Studio texture grid, the expected result is the primary layer 0 texture set only. The face detail and blender mask are valid material data, but they are not the primary mesh texture slots.

### Hat: `outfit_colonist_quarterpaddedvest_01_hat.mat`

The second reference material export at `C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\resources\Materials\Clothes\outfit_colonist_quarterpaddedvest_01_hat.mat` confirms the same resolver shape on a different NIF:

- Root object `outfit_colonist_quarterpaddedvest_01_hat`
  - `BSMaterial::LayerID[0]` -> `res:CEA22BEC:0006E3E9:A6D9E67F`
    - `BSMaterial::MaterialID[0]` -> `res:D691332E:0005F7C7:A7F82E21`
      - `BSMaterial::TextureSetID[0]` -> `res:2AC59295:00069284:A21B614A`
        - Slot 0: `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_color.DDS`
        - Slot 1: `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_normal.DDS`
        - Slot 3: `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_rough.DDS`
        - Slot 5: `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_ao.dds`
  - `BSMaterial::LayerID[1]` -> denim textile secondary layer, not primary grid data
  - `BSMaterial::LayerID[2]` -> leather textile secondary layer, not primary grid data
  - `BSMaterial::BlenderID[0]` / `[1]` -> mask/blender data, not primary grid data
  - `VeryLow_*` / LOD material links -> valid LOD data, not primary grid data

For the current Outfit Studio texture grid, strip a leading `Data\` prefix and preserve the texture path otherwise. The hat fixture is useful because secondary textile layers and LOD material records can otherwise trick a broad scanner.

## Subagent Evidence From ShapeData NIFs

Read-only NIF inspection found:

- `D:\SFMO2\overwrite\Tools\BodySlide\ShapeData\Naked female\naked_f.nif`
  - Shape count: 1
  - Shape name: `Naked_F:0`
  - Material path: `Materials\Actors\Human\Naked_Body\Female\Naked_F_Body.mat`
  - Matching exported JSON: `C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\resources\Materials\Actors\Human\Naked_Body\Female\naked_f_body.mat`
  - Correct primary resource chain:
    - `BSMaterial::LayerID[0]` = `res:7CE127DC:0005C766:A3FB3F95`
    - `BSMaterial::MaterialID[0]` = `res:0ABB9492:0004FE99:A1919594`
    - `BSMaterial::TextureSetID[0]` = `res:94E790C6:0004005C:A3C6B5B3`
  - This NIF cannot provide 2-3 more material lookups because it has one BSGeometry shape and one material path.

- `D:\SFMO2\overwrite\Tools\BodySlide\ShapeData\hat\outfit_colonist_quarterpaddedvest_01_hat_f_facebones.nif`
  - Shape count: 1
  - Shape name: `Outfit_Colonist_QuarterPaddedVest_01_Hat_F_faceBones:0`
  - Material path: `Materials\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat.mat`
  - Matching exported JSON: `C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\resources\Materials\Clothes\outfit_colonist_quarterpaddedvest_01_hat.mat`
  - Correct primary resource chain:
    - `BSMaterial::LayerID[0]` = `res:CEA22BEC:0006E3E9:A6D9E67F`
    - `BSMaterial::MaterialID[0]` = `res:D691332E:0005F7C7:A7F82E21`
    - `BSMaterial::TextureSetID[0]` = `res:2AC59295:00069284:A21B614A`
  - Primary texture slots:
    - 0 = `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_color.DDS`
    - 1 = `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_normal.DDS`
    - 3 = `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_rough.DDS`
    - 5 = `Data\Textures\Clothes\Outfit_Colonist_QuarterPaddedVest_01\Outfit_Colonist_QuarterPaddedVest_01_Hat_ao.dds`
  - Distractor records:
    - `LayerID[1]` = denim textile texture set `res:449C1558:0004CC08:A61F5070`
    - `LayerID[2]` = leather textile texture set `res:CE219147:0006817D:A2B09175`
    - `BlenderID[0]` = mask 1
    - `BlenderID[1]` = mask 2
    - LOD/VeryLow root uses texture set `res:12F23154:0006A7EC:A38E3CE2`

## Cleanup Boundary

Keep:

- Loose `.mat` support and Shape Properties material path UI.
- Starfield PBR-ish texture slot labels in the texture dialog.
- `lib/FSEngine/FSBSA.cpp` unchanged until archive probing proves root-level BA2 enumeration is required for paths like `materials/materialsbeta.cdb`.
- `STARFIELD_TODO.md`, including the PBR preview item marked as later.

Replace or remove:

- Broad texture scanning in `SFMaterialFile`.
- The cleaned-out broad CDB child traversal must not be reintroduced.
- Tests that accept face-detail, hand, or mask records as the primary `Naked_F_Body` texture result.
- Generated/debug files under `build/sf-material-probe/` and temporary `build/SFMaterialDatabaseTest_*.exe` artifacts. These are generated files and must not be staged.

Do not touch:

- `Config.xml`
- `OutfitStudio.xml`
- `Log_OS.txt`

---

### Task 1: Freeze The Correct Loose Material Graph Behavior In Tests

**Files:**
- Modify: `tests/SFMaterialFileTest.cpp`

- [ ] **Step 1: Add a loose JSON graph fixture test**

Add a test JSON string that deliberately places the distracting secondary layer before the primary layer object in the `Objects` array, so a broad scanner fails. The resolver must still follow `LayerID[0]`.

Use this fixture shape inside `tests/SFMaterialFileTest.cpp`:

```cpp
const char* layeredMaterialJson = R"json(
{
    "Objects": [
        {
            "Parent": "materials\\layered\\root\\layeredmaterials.mat",
            "Components": [
                {
                    "Type": "BSComponentDB::CTName",
                    "Index": 0,
                    "Data": { "Name": "naked_f_body" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 1,
                    "Data": { "ID": "res:SECONDARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::BlenderID",
                    "Index": 0,
                    "Data": { "ID": "res:MASK_BLENDER" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:SECONDARY_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:SECONDARY_MATERIAL" }
                }
            ]
        },
        {
            "ID": "res:MASK_BLENDER",
            "Parent": "materials\\layered\\root\\blenders.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_mask.dds" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_normal.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 3,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_rough.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 5,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_ao.dds" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_MATERIAL" }
                }
            ]
        }
    ]
}
)json";
```

Add assertions:

```cpp
std::istringstream layeredInput(layeredMaterialJson);
SFMaterialFile layeredMaterial(layeredInput);
Require(!layeredMaterial.Failed(), "Layered Starfield material should parse");
Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Color) == "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds", "Primary layer color texture should win");
Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Normal) == "textures/actors/human/naked_body/nakedbodyf_normal.dds", "Primary layer normal texture should win");
Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Roughness) == "textures/actors/human/naked_body/nakedbodyf_rough.dds", "Primary layer roughness texture should win");
Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::AmbientOcclusion) == "textures/actors/human/naked_body/nakedbodyf_ao.dds", "Primary layer AO texture should win");
```

- [ ] **Step 2: Run the loose material test and confirm it fails before implementation**

Run:

```powershell
g++ -std=c++17 -I. -Isrc -Ilib\nlohmannjson\include tests\SFMaterialFileTest.cpp src\files\SFMaterialFile.cpp src\utils\StringStuff.cpp -o build\SFMaterialFileTest_graph_red.exe
build\SFMaterialFileTest_graph_red.exe
```

Expected before fix: FAIL because the existing loose parser scans components broadly and can choose the secondary normal or blender mask depending on object order.

Do not keep a compile-only CDB test stub in this task. A CDB test must build/load an explicit fixture and exercise `SFMaterialDatabase`; add it in Task 4 when the database class is reintroduced.

### Task 2: Introduce A Shared Starfield Material Graph Resolver

**Files:**
- Create: `src/files/SFMaterialGraph.h`
- Create: `src/files/SFMaterialGraph.cpp`
- Modify: `CMakeLists.txt`
- Modify: `OutfitStudio.vcxproj`
- Modify: `OutfitStudio.vcxproj.filters`

- [ ] **Step 1: Add the graph data structures**

Create `src/files/SFMaterialGraph.h` with:

```cpp
/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

enum class SFMaterialTextureSlot : size_t {
    Color = 0,
    Normal,
    Opacity,
    Roughness,
    Metalness,
    AmbientOcclusion,
    Height,
    Emissive,
    Count
};

struct SFMaterialComponent {
    std::string type;
    uint32_t index = 0;
    std::string fileName;
    std::string linkedID;
};

struct SFMaterialGraphObject {
    std::string id;
    std::string parent;
    std::vector<SFMaterialComponent> components;
    std::vector<std::string> children;
};

class SFMaterialGraph {
    std::vector<SFMaterialGraphObject> objects;
    std::map<std::string, size_t> objectsByID;

    const SFMaterialGraphObject* FindRootObject() const;
    const SFMaterialGraphObject* FindObject(const std::string& id) const;
    const SFMaterialComponent* FindLink(const SFMaterialGraphObject& object, const std::string& type, uint32_t index) const;
    void CollectTextureSetTextures(const SFMaterialGraphObject& textureSet, std::vector<std::string>& textureFiles) const;

public:
    void Clear();
    void AddObject(SFMaterialGraphObject object);
    void BuildChildLinks();
    bool ResolvePrimaryTextures(std::vector<std::string>& textureFiles, size_t numTextures) const;
};
```

- [ ] **Step 2: Implement deterministic primary-layer traversal**

Create `src/files/SFMaterialGraph.cpp`. The resolver must:

1. Find the root object with `parent == materials\layered\root\layeredmaterials.mat` or empty `id`.
2. Follow `BSMaterial::LayerID` index 0.
3. Follow `BSMaterial::MaterialID` index 0.
4. Follow `BSMaterial::TextureSetID` index 0.
5. Collect only `BSMaterial::MRTextureFile` and `BSMaterial::TextureFile` records from that selected texture set.
6. Also collect texture records from child objects whose `Edges`/CDB `EdgeInfo` parent is the selected texture set, because real CDB records can store texture components as children of the texture set object.
7. If the exact layer 0 chain has no textures, fall back to the first `LayerID` by ascending index that produces textures.
8. Do not traverse `BlenderID` for the primary texture grid.

Use this helper behavior in the implementation:

```cpp
static std::string NormalizeMaterialPath(std::string path) {
    std::replace(path.begin(), path.end(), '/', '\\');
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return path;
}

static std::string NormalizeTexturePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.length() > 5 && StringsEqualInsens(path.substr(0, 5).c_str(), "data/"))
        path.erase(0, 5);
    return path;
}
```

Include `../utils/StringStuff.h` and the standard headers needed for `std::replace`, `std::sort`, and `std::tolower`.

- [ ] **Step 3: Wire the new graph files into build systems**

Add `src/files/SFMaterialGraph.cpp` to `OSsources` in `CMakeLists.txt`, and add it to both test executables:

```cmake
add_executable(SFMaterialFileTest
    tests/SFMaterialFileTest.cpp
    src/files/SFMaterialFile.cpp
    src/files/SFMaterialGraph.cpp
    src/utils/StringStuff.cpp
    )

add_executable(SFMaterialDatabaseTest
    tests/SFMaterialDatabaseTest.cpp
    src/files/SFMaterialDatabase.cpp
    src/files/SFMaterialGraph.cpp
    src/utils/StringStuff.cpp
    )
```

Add `src\files\SFMaterialGraph.cpp` and `src\files\SFMaterialGraph.h` to `OutfitStudio.vcxproj` and `.filters` next to the other `src\files` entries.

### Task 3: Refactor Loose JSON `.mat` Parsing Onto The Graph

**Files:**
- Modify: `src/files/SFMaterialFile.h`
- Modify: `src/files/SFMaterialFile.cpp`
- Test: `tests/SFMaterialFileTest.cpp`

- [ ] **Step 1: Remove the duplicated texture-slot enum from `SFMaterialFile.h`**

Replace the enum in `SFMaterialFile.h` with:

```cpp
#include "SFMaterialGraph.h"
```

Keep the public `SFMaterialFile` API unchanged:

```cpp
const std::string& GetTexture(SFMaterialTextureSlot slot) const;
std::vector<std::string> GetTextureFiles(size_t numTextures) const;
```

- [ ] **Step 2: Convert JSON objects into `SFMaterialGraphObject` records**

In `SFMaterialFile::Read`, replace broad component scanning with:

```cpp
SFMaterialGraph graph;

for (const auto& objectJson : *objects) {
    SFMaterialGraphObject object;
    if (auto id = objectJson.find("ID"); id != objectJson.end() && id->is_string())
        object.id = id->get<std::string>();
    if (auto parent = objectJson.find("Parent"); parent != objectJson.end() && parent->is_string())
        object.parent = parent->get<std::string>();

    const auto components = objectJson.find("Components");
    if (components != objectJson.end() && components->is_array()) {
        for (const auto& componentJson : *components) {
            SFMaterialComponent component;
            const auto type = componentJson.find("Type");
            const auto index = componentJson.find("Index");
            if (type == componentJson.end() || !type->is_string())
                continue;

            component.type = type->get<std::string>();
            if (index != componentJson.end() && index->is_number_unsigned())
                component.index = index->get<uint32_t>();

            const auto data = componentJson.find("Data");
            if (data != componentJson.end() && data->is_object()) {
                const auto fileName = data->find("FileName");
                if (fileName != data->end() && fileName->is_string())
                    component.fileName = fileName->get<std::string>();

                const auto linkedID = data->find("ID");
                if (linkedID != data->end() && linkedID->is_string())
                    component.linkedID = linkedID->get<std::string>();
            }

            object.components.push_back(std::move(component));
        }
    }

    graph.AddObject(std::move(object));
}
```

After loading all objects, call:

```cpp
graph.BuildChildLinks();
std::vector<std::string> textureFiles;
if (!graph.ResolvePrimaryTextures(textureFiles, textures.size())) {
    failed = true;
    return 1;
}

for (size_t i = 0; i < textures.size(); ++i)
    textures[i] = i < textureFiles.size() ? textureFiles[i] : std::string();
```

- [ ] **Step 3: Run loose material tests**

Run:

```powershell
g++ -std=c++17 -I. -Isrc -Ilib\nlohmannjson\include tests\SFMaterialFileTest.cpp src\files\SFMaterialFile.cpp src\files\SFMaterialGraph.cpp src\utils\StringStuff.cpp -o build\SFMaterialFileTest_graph_green.exe
build\SFMaterialFileTest_graph_green.exe
```

Expected after fix: PASS.

### Task 4: Refactor CDB Resolution Onto The Same Graph

**Files:**
- Create: `src/files/SFMaterialDatabase.h`
- Create: `src/files/SFMaterialDatabase.cpp`
- Test: `tests/SFMaterialDatabaseTest.cpp`

- [ ] **Step 1: Add a CDB graph test with an explicit fixture path**

Create `tests/SFMaterialDatabaseTest.cpp` with a test that either:

- loads a real `.cdb` fixture from `SF_MATERIAL_CDB_FIXTURE`, or
- uses a small test helper that writes valid CDB bytes for a synthetic material graph.

Do not use an empty fixture or a default-constructed database as the data source. The test must exercise `SFMaterialDatabase` and assert:

```cpp
std::vector<std::string> graphTextures = database.GetTextureFiles("materials/actors/human/naked_body/female/naked_f_body.mat", 10);
Require(graphTextures[0] == "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds", "CDB graph should resolve primary layer color texture");
Require(graphTextures[1] == "textures/actors/human/naked_body/nakedbodyf_normal.dds", "CDB graph should resolve primary layer normal texture");
Require(graphTextures[3] == "textures/actors/human/naked_body/nakedbodyf_rough.dds", "CDB graph should resolve primary layer roughness texture");
Require(graphTextures[5] == "textures/actors/human/naked_body/nakedbodyf_ao.dds", "CDB graph should resolve primary layer AO texture");
Require(graphTextures[1] != "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds", "CDB graph must ignore secondary layer face detail for the primary grid");
Require(graphTextures[0] != "textures/actors/human/naked_body/nakedbodyf_mask.dds", "CDB graph must ignore blender mask for the primary color slot");
```

- [ ] **Step 2: Keep binary CDB parsing, replace texture collection**

Keep the CDB reader pieces that decode:

- STRT/TYPES/CLAS chunks
- built-in reflection string offsets
- object list
- component list
- edge list
- `BSComponentDB2::ID` links
- primitive `String` values

Remove the broad `CollectTextures` behavior that walks all child objects from the material root.

- [ ] **Step 3: Build an `SFMaterialGraph` for the requested material**

In `ResolveMaterial`, after finding `MaterialObject* materialObject`, build a graph containing:

- the material root object
- every object referenced by `BSMaterial::LayerID`, `BSMaterial::MaterialID`, `BSMaterial::TextureSetID`, and `BSMaterial::UVStreamID`
- children whose parent is the selected texture set object, so texture components stored in CDB child records are still collected

Represent each CDB object as:

```cpp
SFMaterialGraphObject graphObject;
graphObject.id = ResourceIDToString(object->persistentID);
graphObject.parent = MaterialRootPathForObjectType(GetMaterialObjectType(object));
```

Represent each component as:

```cpp
SFMaterialComponent graphComponent;
graphComponent.type = component.className;
graphComponent.index = component.index;
graphComponent.linkedID = ResourceIDToString(linkedObject->persistentID);
graphComponent.fileName = *fileName;
```

Use the existing `FindLinkedObject` and `FindFileName` helpers for CDB values.

- [ ] **Step 4: Add CDB resource ID string formatting**

Add a private helper in `SFMaterialDatabase.cpp`:

```cpp
std::string ResourceIDToString(const SFMaterialResourceID& id) {
    std::ostringstream stream;
    stream << "res:"
           << std::uppercase << std::hex << std::setfill('0')
           << std::setw(8) << id.file << ":"
           << std::setw(8) << id.ext << ":"
           << std::setw(8) << id.dir;
    return stream.str();
}
```

Include `<iomanip>` and `<sstream>`.

- [ ] **Step 5: Run CDB tests**

Run:

```powershell
g++ -std=c++17 -I. -Isrc tests\SFMaterialDatabaseTest.cpp src\files\SFMaterialDatabase.cpp src\files\SFMaterialGraph.cpp src\utils\StringStuff.cpp -o build\SFMaterialDatabaseTest_graph_green.exe
build\SFMaterialDatabaseTest_graph_green.exe
```

Expected after fix: PASS.

- [ ] **Step 6: Run real fixture check against `materialsbeta.cdb`**

Run:

```powershell
$env:SF_MATERIAL_CDB_FIXTURE='C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\BodySlide-and-Outfit-Studio\build\sf-material-probe\materialsbeta.cdb'
build\SFMaterialDatabaseTest_graph_green.exe
```

Expected after fix: PASS and the real fixture assertions should require:

```cpp
Require(fixtureTextures[0].find("nakedbodyf_sk3_color.dds") != std::string::npos, "Real CDB fixture should resolve Naked_F_Body primary color texture");
Require(fixtureTextures[1].find("nakedbodyf_normal.dds") != std::string::npos, "Real CDB fixture should resolve Naked_F_Body primary normal texture");
Require(fixtureTextures[1].find("young_cheek_detail_normal.dds") == std::string::npos, "Real CDB fixture should ignore secondary face detail normal for primary grid");
Require(fixtureTextures[0].find("nakedbodyf_mask.dds") == std::string::npos, "Real CDB fixture should ignore blender mask for primary color slot");
```

### Task 5: Cleanup Slice 2 Scratch Artifacts And Stale Assumptions

**Files:**
- Modify: `docs/superpowers/plans/2026-05-15-starfield-cdb-archive-repair.md`
- Generated files only: `build/sf-material-probe/*`, `build/SFMaterialDatabaseTest_*.exe`, `build/SFMaterialFileTest_*.exe`

- [ ] **Step 1: Mark the old repair plan as superseded**

At the top of `docs/superpowers/plans/2026-05-15-starfield-cdb-archive-repair.md`, add:

```markdown
> Superseded by `docs/superpowers/plans/2026-05-16-starfield-material-graph-rebuild.md`.
> The old plan proved archive loading and CDB reflection parsing, but its broad graph traversal can select secondary layer/blender textures.
```

- [ ] **Step 2: Remove generated scratch binaries from the working tree**

Use PowerShell only, and verify targets first:

```powershell
Get-ChildItem -LiteralPath build -Filter 'SFMaterialDatabaseTest_*.exe'
Get-ChildItem -LiteralPath build -Filter 'SFMaterialFileTest_*.exe'
Get-ChildItem -LiteralPath build\sf-material-probe -Force
```

If the listed files are only generated test/debug artifacts, delete them:

```powershell
Remove-Item -LiteralPath (Get-ChildItem -LiteralPath build -Filter 'SFMaterialDatabaseTest_*.exe').FullName
Remove-Item -LiteralPath (Get-ChildItem -LiteralPath build -Filter 'SFMaterialFileTest_*.exe').FullName
```

Do not delete `build/sf-material-probe/materialsbeta.cdb` until the real fixture test no longer needs it.

- [ ] **Step 3: Confirm forbidden local files are not staged**

Run:

```powershell
git status --short
```

Expected: no staged or modified `Config.xml`, `OutfitStudio.xml`, or `Log_OS.txt`.

### Task 6: Integration Build And Outfit Studio Verification

**Files:**
- Modify only if tests expose integration bugs: `src/program/OutfitProject.cpp`
- Modify only if project file entries are missing: `OutfitStudio.vcxproj`, `OutfitStudio.vcxproj.filters`, `CMakeLists.txt`

- [ ] **Step 1: Run project checks**

Run:

```powershell
git diff --check
[xml](Get-Content -Path 'OutfitStudio.vcxproj' -Raw) | Out-Null
[xml](Get-Content -Path 'OutfitStudio.vcxproj.filters' -Raw) | Out-Null
```

Expected: `git diff --check` has no whitespace errors; XML parse commands return exit code 0.

- [ ] **Step 2: Build Outfit Studio Release x64**

Run:

```powershell
$cleanPath = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $cleanPath, 'Process')
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' OutfitStudio.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`.

- [ ] **Step 3: Verify the vanilla body in Outfit Studio**

Open the same Starfield `Naked_F_Body.mat` NIF in Outfit Studio and use Shape Properties -> Textures.

Expected primary texture grid:

- Color: `textures/actors/human/naked_body/nakedbodyf_sk3_color.dds`
- Normal: `textures/actors/human/naked_body/nakedbodyf_normal.dds`
- Roughness: `textures/actors/human/naked_body/nakedbodyf_rough.dds`
- AO: `textures/actors/human/naked_body/nakedbodyf_ao.dds`

Expected not present in the primary grid:

- `textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds`
- `textures/actors/human/naked_body/nakedbodyf_mask.dds`
- hand texture paths

### Task 7: Final Documentation And Status

**Files:**
- Modify: `STARFIELD_TODO.md`

- [ ] **Step 1: Update Starfield TODO material status**

Set material items to:

```markdown
- [x] Load loose Starfield `.mat` material files referenced by BSGeometry shapes.
- [x] Resolve the primary Starfield material texture set from loose JSON `.mat` graphs.
- [x] Resolve the primary Starfield material texture set from archived `.cdb` material graphs.
- [x] Keep secondary layer, blender, and full PBR preview support as later work.
- [ ] Verify archived vanilla `.cdb` resolution in Outfit Studio with a small sample matrix beyond `Naked_F_Body`.
```

- [ ] **Step 2: Run final status check**

Run:

```powershell
git status --short
```

Expected:

- No forbidden local config/log files.
- Source/test/docs changes are visible.
- Generated build artifacts are not staged.
