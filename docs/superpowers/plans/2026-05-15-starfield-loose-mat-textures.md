# Starfield Loose Mat Textures Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Load loose Starfield `.mat` JSON material files in Outfit Studio, apply their PBR texture paths to previews, and make Shape Properties show Starfield material controls instead of FO4-style specular controls.

**Architecture:** Add a small Starfield-only JSON material reader that is independent of `MaterialFile`, because existing `MaterialFile` handles FO4/FO76 binary `BGSM/BGEM` and is compiled by both BodySlide and Outfit Studio. Wire that reader into `OutfitProject::SetTextures()` only for Starfield NIFs and loose files under `Data/materials/`. Update Shape Properties UI behavior to allow `.mat` selection and label texture rows as Starfield PBR slots.

**Tech Stack:** C++17, wxWidgets UI, existing `nlohmann/json.hpp`, nifly `NiShader`/version checks, existing Outfit Studio texture cache/render path.

---

## File Structure

- Create `src/files/SFMaterialFile.h`: Starfield JSON `.mat` parser interface and PBR texture slot constants.
- Create `src/files/SFMaterialFile.cpp`: Starfield JSON parser using `nlohmann::json`; extracts first `BSMaterial::MRTextureFile` per texture index.
- Create `tests/SFMaterialFileTest.cpp`: lightweight parser smoke test that compiles without wxWidgets or nifly.
- Modify `src/program/OutfitProject.cpp`: resolve loose Starfield material JSON paths and use the parsed texture slots.
- Modify `src/program/ShapeProperties.cpp`: Starfield `.mat` picker, PBR texture row labels, and Starfield-safe shader apply behavior.
- Modify `CMakeLists.txt`: add `SFMaterialFile.cpp` to Outfit Studio and add the parser smoke-test target.
- Modify `OutfitStudio.vcxproj`: add `SFMaterialFile.cpp`/`.h` for Visual Studio builds.
- Modify `OutfitStudio.vcxproj.filters`: place the new source/header under the existing `src\files` filters.

## Starfield PBR Slot Mapping

Use the index values from `StarfieldMeshConverter/scripts/tool_export_mesh/MaterialConverter.py`:

| Index | Texture slot |
| --- | --- |
| 0 | Color |
| 1 | Normal |
| 2 | Opacity |
| 3 | Roughness |
| 4 | Metalness |
| 5 | Ambient Occlusion |
| 6 | Height |
| 7 | Emissive |

`TextureReplacement` components are not applied in slice 1. The parser should only consume `BSMaterial::MRTextureFile` components with a numeric `Index` and a string `Data.FileName`.

## Task 1: Add Starfield `.mat` Parser With Smoke Test

**Files:**
- Create: `src/files/SFMaterialFile.h`
- Create: `src/files/SFMaterialFile.cpp`
- Create: `tests/SFMaterialFileTest.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing parser test**

Create `tests/SFMaterialFileTest.cpp`:

```cpp
#include "../src/files/SFMaterialFile.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

static void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

int main() {
    const char* json = R"json(
{
    "Objects": [
        {
            "Components": [
                {
                    "Data": { "Name": "example" },
                    "Index": 0,
                    "Type": "BSComponentDB::CTName"
                },
                {
                    "Data": { "FileName": "textures/actors/human/body_color.dds" },
                    "Index": 0,
                    "Type": "BSMaterial::MRTextureFile"
                },
                {
                    "Data": { "FileName": "textures/actors/human/body_normal.dds" },
                    "Index": 1,
                    "Type": "BSMaterial::MRTextureFile"
                },
                {
                    "Data": { "FileName": "textures/actors/human/body_rough.dds" },
                    "Index": 3,
                    "Type": "BSMaterial::MRTextureFile"
                },
                {
                    "Data": { "FileName": "textures/actors/human/detail_normal.dds" },
                    "Index": 1,
                    "Type": "BSMaterial::MRTextureFile"
                }
            ],
            "Parent": "materials\\layered\\root\\texturesets.mat"
        }
    ],
    "Version": 1
}
)json";

    std::istringstream input(json);
    SFMaterialFile mat(input);

    Require(!mat.Failed(), "Starfield material JSON failed to parse");
    Require(mat.GetTexture(SFMaterialTextureSlot::Color) == "textures/actors/human/body_color.dds", "Color texture was not parsed");
    Require(mat.GetTexture(SFMaterialTextureSlot::Normal) == "textures/actors/human/body_normal.dds", "First normal texture should win");
    Require(mat.GetTexture(SFMaterialTextureSlot::Roughness) == "textures/actors/human/body_rough.dds", "Roughness texture was not parsed");
    Require(mat.GetTexture(SFMaterialTextureSlot::Metalness).empty(), "Missing metalness slot should be empty");

    auto textureFiles = mat.GetTextureFiles(10);
    Require(textureFiles.size() == 10, "Texture vector should preserve requested slot count");
    Require(textureFiles[0] == "textures/actors/human/body_color.dds", "Vector color slot mismatch");
    Require(textureFiles[1] == "textures/actors/human/body_normal.dds", "Vector normal slot mismatch");
    Require(textureFiles[3] == "textures/actors/human/body_rough.dds", "Vector roughness slot mismatch");

    return 0;
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run from the repository root in a Developer Command Prompt:

```bat
cl /std:c++17 /EHsc /Ilib\nlohmannjson\include tests\SFMaterialFileTest.cpp src\files\SFMaterialFile.cpp /Fe:build\SFMaterialFileTest.exe
```

Expected: compile fails because `src/files/SFMaterialFile.h` and `src/files/SFMaterialFile.cpp` do not exist yet.

- [ ] **Step 3: Add the parser header**

Create `src/files/SFMaterialFile.h`:

```cpp
/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <array>
#include <cstddef>
#include <istream>
#include <string>
#include <vector>

enum class SFMaterialTextureSlot : size_t {
    Color = 0,
    Normal = 1,
    Opacity = 2,
    Roughness = 3,
    Metalness = 4,
    AmbientOcclusion = 5,
    Height = 6,
    Emissive = 7,
    Count = 8
};

class SFMaterialFile {
    bool failed = false;
    std::array<std::string, static_cast<size_t>(SFMaterialTextureSlot::Count)> textures;

public:
    SFMaterialFile() = default;
    explicit SFMaterialFile(const std::string& fileName);
    explicit SFMaterialFile(std::istream& input);

    int Read(std::istream& input);

    bool Failed() const { return failed; }
    const std::string& GetTexture(SFMaterialTextureSlot slot) const;
    std::vector<std::string> GetTextureFiles(size_t count) const;
};
```

- [ ] **Step 4: Add the parser implementation**

Create `src/files/SFMaterialFile.cpp`:

```cpp
/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialFile.h"
#include "../utils/StringStuff.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace {
constexpr size_t SF_TEXTURE_SLOT_COUNT = static_cast<size_t>(SFMaterialTextureSlot::Count);

bool ReadTextureComponent(const nlohmann::json& component, size_t& outIndex, std::string& outPath) {
    if (!component.is_object())
        return false;

    auto typeIt = component.find("Type");
    if (typeIt == component.end() || !typeIt->is_string() || typeIt->get<std::string>() != "BSMaterial::MRTextureFile")
        return false;

    auto indexIt = component.find("Index");
    if (indexIt == component.end() || !indexIt->is_number_unsigned())
        return false;

    size_t index = indexIt->get<size_t>();
    if (index >= SF_TEXTURE_SLOT_COUNT)
        return false;

    auto dataIt = component.find("Data");
    if (dataIt == component.end() || !dataIt->is_object())
        return false;

    auto fileIt = dataIt->find("FileName");
    if (fileIt == dataIt->end() || !fileIt->is_string())
        return false;

    std::string fileName = fileIt->get<std::string>();
    if (fileName.empty())
        return false;

    outIndex = index;
    outPath = ToOSSlashes(fileName);
    return true;
}
}

SFMaterialFile::SFMaterialFile(const std::string& fileName) {
    std::ifstream input(fileName, std::ifstream::binary);
    if (!input) {
        failed = true;
        return;
    }

    if (Read(input))
        failed = true;
}

SFMaterialFile::SFMaterialFile(std::istream& input) {
    if (!input) {
        failed = true;
        return;
    }

    if (Read(input))
        failed = true;
}

int SFMaterialFile::Read(std::istream& input) {
    nlohmann::json root = nlohmann::json::parse(input, nullptr, false, true);
    if (root.is_discarded() || !root.is_object())
        return 1;

    auto objectsIt = root.find("Objects");
    if (objectsIt == root.end() || !objectsIt->is_array())
        return 1;

    for (const auto& object : *objectsIt) {
        if (!object.is_object())
            continue;

        auto componentsIt = object.find("Components");
        if (componentsIt == object.end() || !componentsIt->is_array())
            continue;

        for (const auto& component : *componentsIt) {
            size_t index = 0;
            std::string path;
            if (!ReadTextureComponent(component, index, path))
                continue;

            if (textures[index].empty())
                textures[index] = std::move(path);
        }
    }

    return 0;
}

const std::string& SFMaterialFile::GetTexture(SFMaterialTextureSlot slot) const {
    return textures[static_cast<size_t>(slot)];
}

std::vector<std::string> SFMaterialFile::GetTextureFiles(size_t count) const {
    std::vector<std::string> textureFiles(count);
    const size_t copyCount = std::min(count, textures.size());
    for (size_t i = 0; i < copyCount; i++)
        textureFiles[i] = textures[i];

    return textureFiles;
}
```

- [ ] **Step 5: Add the parser smoke-test target to CMake**

Modify `CMakeLists.txt` after `add_executable(BodySlide ${BSsources})`:

```cmake
add_executable(SFMaterialFileTest
    tests/SFMaterialFileTest.cpp
    src/files/SFMaterialFile.cpp
    src/utils/StringStuff.cpp
    )
target_include_directories(SFMaterialFileTest PUBLIC
    lib/nlohmannjson/include
    )
```

Also add `src/files/SFMaterialFile.cpp` to `OSsources`, near `src/files/SFMorphFile.cpp`:

```cmake
    src/files/SFMorphFile.cpp
    src/files/SFMaterialFile.cpp
```

- [ ] **Step 6: Run the parser test to verify it passes**

Run:

```bat
cl /std:c++17 /EHsc /Ilib\nlohmannjson\include tests\SFMaterialFileTest.cpp src\files\SFMaterialFile.cpp src\utils\StringStuff.cpp /Fe:build\SFMaterialFileTest.exe
build\SFMaterialFileTest.exe
```

Expected: compile succeeds and the executable exits with code `0`.

- [ ] **Step 7: Commit parser slice**

```bash
git add src/files/SFMaterialFile.h src/files/SFMaterialFile.cpp tests/SFMaterialFileTest.cpp CMakeLists.txt
git commit -m "Outfit Studio: Add Starfield material JSON parser"
```

## Task 2: Resolve Loose Starfield `.mat` Textures In OutfitProject

**Files:**
- Modify: `src/program/OutfitProject.cpp`

- [ ] **Step 1: Add parser include and path normalization helpers**

At the top of `src/program/OutfitProject.cpp`, add:

```cpp
#include "../files/SFMaterialFile.h"
```

Near the existing local helper functions in `OutfitProject.cpp`, add:

```cpp
namespace {
bool HasExtensionInsensitive(const std::string& path, const std::string& extension) {
    if (extension.empty())
        return true;
    if (path.length() < extension.length())
        return false;
    return StringsEqualInsens(path.c_str() + path.length() - extension.length(), extension.c_str());
}

std::string NormalizeMaterialPath(std::string matFile, const std::string& extension) {
    matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");
    matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");
    matFile = std::regex_replace(matFile, std::regex("^/+"), "");
    matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");

    if (!HasExtensionInsensitive(matFile, extension))
        matFile += extension;

    return matFile;
}
}
```

If `OutfitProject.cpp` already has an anonymous namespace, place this inside it. If not, create one near the top after includes.

- [ ] **Step 2: Replace duplicated FO4 material path normalization**

In `OutfitProject::SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles)`, replace the existing FO4 material-path regex block:

```cpp
// Replace all backward slashes with one forward slash
matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");

// Remove everything before the first occurence of "/materials/"
matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");

// Remove all slashes from the front
matFile = std::regex_replace(matFile, std::regex("^/+"), "");

// If the path doesn't start with "materials/", add it to the front
matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");
```

with:

```cpp
matFile = NormalizeMaterialPath(matFile, "");
```

This should preserve FO4 behavior, where `.bgsm`/`.bgem` paths already include their extension in the shader name.

- [ ] **Step 3: Add Starfield material detection**

In the block that currently detects material files:

```cpp
if (shader) {
    // Find material file
    if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76()) {
        matFile = shader->name.get();
        if (!matFile.empty())
            hasMat = true;
    }
}
```

change it to:

```cpp
bool hasSFMat = false;
if (shader) {
    // Find material file
    if (workNif.GetHeader().GetVersion().IsFO4() || workNif.GetHeader().GetVersion().IsFO76()) {
        matFile = shader->name.get();
        if (!matFile.empty())
            hasMat = true;
    }
    else if (workNif.GetHeader().GetVersion().IsSF()) {
        matFile = shader->name.get();
        if (!matFile.empty()) {
            hasMat = true;
            hasSFMat = true;
        }
    }
}
```

- [ ] **Step 4: Read loose Starfield `.mat` before FO4 binary material handling**

Inside `if (hasMat) {`, after `matFile` is normalized and before constructing/reading `MaterialFile mat`, add:

```cpp
if (hasSFMat) {
    matFile = NormalizeMaterialPath(matFile, ".mat");

    SFMaterialFile sfMat(texturesDir + matFile);
    if (!sfMat.Failed())
        texFiles = sfMat.GetTextureFiles(MAX_TEXTURE_PATHS);
    else if (shader) {
        for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
            workNif.GetTextureSlot(shape, texFiles[i], i);
    }
}
else {
    matFile = NormalizeMaterialPath(matFile, "");
```

Then close the new `else` after the existing FO4/BGSM/BGEM material block:

```cpp
}
```

The Starfield branch intentionally does not search archives. Archived vanilla material lookup belongs to the `.cdb` slice.

- [ ] **Step 5: Run a compile check**

Run:

```bat
msbuild BS_OS.sln /m /p:Configuration=Release /p:Platform=x64
```

Expected: `OutfitStudio.vcxproj` builds without missing symbol or include errors once Task 4 also adds the new file to the Visual Studio project.

- [ ] **Step 6: Commit resolver slice**

```bash
git add src/program/OutfitProject.cpp
git commit -m "Outfit Studio: Resolve loose Starfield material textures"
```

## Task 3: Make Shape Properties Starfield/PBR Aware

**Files:**
- Modify: `src/program/ShapeProperties.cpp`

- [ ] **Step 1: Add PBR texture labels**

In `ShapeProperties::OnSetTextures`, replace the fixed row labels:

```cpp
stTexGrid->SetRowLabelValue(0, "Diffuse");
stTexGrid->SetRowLabelValue(1, "Normal");
stTexGrid->SetRowLabelValue(2, "Glow/Skin");
stTexGrid->SetRowLabelValue(3, "Parallax");
stTexGrid->SetRowLabelValue(4, "Environment");
stTexGrid->SetRowLabelValue(5, "Env Mask");
stTexGrid->SetRowLabelValue(6, "Subsurface");
stTexGrid->SetRowLabelValue(7, "Specular");
stTexGrid->SetRowLabelValue(8, "Backlight");
stTexGrid->SetRowLabelValue(9, "Unused");
```

with:

```cpp
if (nif->GetHeader().GetVersion().IsSF()) {
    stTexGrid->SetRowLabelValue(0, "Color");
    stTexGrid->SetRowLabelValue(1, "Normal");
    stTexGrid->SetRowLabelValue(2, "Opacity");
    stTexGrid->SetRowLabelValue(3, "Roughness");
    stTexGrid->SetRowLabelValue(4, "Metalness");
    stTexGrid->SetRowLabelValue(5, "AO");
    stTexGrid->SetRowLabelValue(6, "Height");
    stTexGrid->SetRowLabelValue(7, "Emissive");
    stTexGrid->SetRowLabelValue(8, "Unused");
    stTexGrid->SetRowLabelValue(9, "Unused");
}
else {
    stTexGrid->SetRowLabelValue(0, "Diffuse");
    stTexGrid->SetRowLabelValue(1, "Normal");
    stTexGrid->SetRowLabelValue(2, "Glow/Skin");
    stTexGrid->SetRowLabelValue(3, "Parallax");
    stTexGrid->SetRowLabelValue(4, "Environment");
    stTexGrid->SetRowLabelValue(5, "Env Mask");
    stTexGrid->SetRowLabelValue(6, "Subsurface");
    stTexGrid->SetRowLabelValue(7, "Specular");
    stTexGrid->SetRowLabelValue(8, "Backlight");
    stTexGrid->SetRowLabelValue(9, "Unused");
}
```

- [ ] **Step 2: Update the material chooser filter**

Replace `ShapeProperties::OnChooseMaterial` with:

```cpp
void ShapeProperties::OnChooseMaterial(wxCommandEvent& WXUNUSED(event)) {
    bool isSF = nif->GetHeader().GetVersion().IsSF();
    wxString defaultExt = isSF ? ".mat" : ".bgsm";
    wxString wildcard = isSF ? "Starfield material files (*.mat)|*.mat|All material files (*.mat;*.bgsm;*.bgem)|*.mat;*.bgsm;*.bgem"
                             : "Material files (*.bgsm;*.bgem)|*.bgsm;*.bgem";

    wxString fileName = wxFileSelector(_("Choose material file"), wxEmptyString, wxEmptyString, defaultExt, wildcard, wxFD_FILE_MUST_EXIST, this);
    if (fileName.empty())
        return;

    wxString findStr = wxString::Format("%cmaterials%c", PathSepChar, PathSepChar);
    int index = fileName.Lower().Find(findStr);
    if (index != wxNOT_FOUND && fileName.length() - 1 > (size_t)index + 1)
        fileName = fileName.Mid(index + 1);

    shaderName->SetValue(fileName);
}
```

- [ ] **Step 3: Hide legacy specular controls for Starfield**

In the constructor after:

```cpp
if (version.Stream() >= 130) {
    lbShaderName->SetLabel(_("Material"));
    btnMaterialChooser->Show();
    pgShader->Layout();
}
```

add:

```cpp
if (version.IsSF()) {
    auto showLabeledControl = [](wxWindow* control, bool show) {
        wxSizer* sizer = control ? control->GetContainingSizer() : nullptr;
        if (!sizer) {
            if (control)
                control->Show(show);
            return;
        }

        wxSizerItemList& items = sizer->GetChildren();
        wxSizerItem* previous = nullptr;
        for (auto item : items) {
            if (item->GetWindow() == control) {
                if (previous && previous->GetWindow())
                    previous->GetWindow()->Show(show);
                control->Show(show);
                return;
            }
            previous = item;
        }

        control->Show(show);
    };

    showLabeledControl(shaderType, false);
    showLabeledControl(specularColor, false);
    showLabeledControl(specularStrength, false);
    showLabeledControl(specularPower, false);
    shaderFlagsPane->Hide();
    advancedShaderPane->Hide();
    pgShader->Layout();
}
```

This keeps the Material path, emissive, alpha, vertex color, double-sided, transparency, and texture controls visible.

- [ ] **Step 4: Avoid writing legacy FO4 specular/shader fields for Starfield**

In `ShapeProperties::ApplyChanges`, after `auto& version = nif->GetHeader().GetVersion();`, add:

```cpp
bool isSF = version.IsSF();
```

Inside the `BSLightingShaderProperty` branch, wrap shader type, environment-map toggles, specular writes, and FO4-era advanced field writes:

```cpp
if (!isSF) {
    bslsp->SetShaderType(type);

    if (oldType != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && type == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP) {
        bslsp->SetEnvironmentMapping(true);
    }
    else if (oldType == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && type != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP) {
        bslsp->SetEnvironmentMapping(false);
    }

    bslsp->SetSpecularColor(specColor);
    bslsp->SetSpecularStrength(specStrength);
    bslsp->SetGlossiness(specPower);
}
```

Keep these writes outside that guard:

```cpp
bslsp->SetEmissiveColor(emisColor);
bslsp->SetEmissiveMultiple(emisMultiple);
bslsp->SetAlpha(alphaValue);
```

Wrap the legacy advanced property block so it only runs for non-Starfield:

```cpp
if (!isSF) {
    // Existing Advanced properties writes stay here unchanged.
}
```

Also guard shader flag saving:

```cpp
if (bssp && !isSF) {
    uint32_t sf1 = 0;
    uint32_t sf2 = 0;
    ...
}
```

- [ ] **Step 5: Build Outfit Studio**

Run:

```bat
msbuild BS_OS.sln /m /p:Configuration=Release /p:Platform=x64
```

Expected: Release x64 build succeeds.

- [ ] **Step 6: Commit UI slice**

```bash
git add src/program/ShapeProperties.cpp
git commit -m "Outfit Studio: Show Starfield PBR material controls"
```

## Task 4: Wire Visual Studio Project Files

**Files:**
- Modify: `OutfitStudio.vcxproj`
- Modify: `OutfitStudio.vcxproj.filters`

- [ ] **Step 1: Add source/header to OutfitStudio project**

In `OutfitStudio.vcxproj`, add near `src\files\SFMorphFile.cpp` or the nearby file list:

```xml
<ClCompile Include="src\files\SFMaterialFile.cpp" />
```

Add near the other `src\files` headers:

```xml
<ClInclude Include="src\files\SFMaterialFile.h" />
```

- [ ] **Step 2: Add filters**

In `OutfitStudio.vcxproj.filters`, add near the other `src\files` compile entries:

```xml
<ClCompile Include="src\files\SFMaterialFile.cpp">
  <Filter>Source Files\files</Filter>
</ClCompile>
```

Add near the other `src\files` header entries:

```xml
<ClInclude Include="src\files\SFMaterialFile.h">
  <Filter>Header Files\files</Filter>
</ClInclude>
```

If the existing filter names differ, match the exact filter used by `src\files\MaterialFile.cpp` and `src\files\MaterialFile.h`.

- [ ] **Step 3: Verify Visual Studio build sees the new source**

Run:

```bat
msbuild OutfitStudio.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Expected: build succeeds without unresolved `SFMaterialFile` symbols.

- [ ] **Step 4: Commit project-file wiring**

```bash
git add OutfitStudio.vcxproj OutfitStudio.vcxproj.filters CMakeLists.txt
git commit -m "Build: Include Starfield material parser in Outfit Studio"
```

## Task 5: Manual Starfield Verification

**Files:**
- Modify: `STARFIELD_TODO.md`

- [ ] **Step 1: Prepare loose material fixture**

Copy or place a Starfield JSON material at:

```text
<Starfield Data>\materials\actors\human\naked_body\female\naked_f_body.mat
```

Use a NIF whose BSGeometry shader/material name is:

```text
Materials\Actors\Human\Naked_Body\Female\Naked_F_Body.mat
```

- [ ] **Step 2: Verify texture preview resolution in Outfit Studio**

Launch the Release x64 `OutfitStudio.exe`, set Target Game to Starfield, and load the NIF.

Expected:
- Shape Properties Material field keeps the `.mat` path.
- `Textures...` shows PBR row labels.
- Color/Normal/Roughness/AO rows are populated from the loose `.mat` JSON.
- The rendered mesh uses the loaded texture paths when the texture files are available.

- [ ] **Step 3: Verify missing loose `.mat` fallback**

Temporarily rename the loose `.mat` file and reload the NIF.

Expected:
- Outfit Studio does not crash.
- Texture rows fall back to existing NIF texture slots or remain empty.
- No `.cdb` lookup is attempted in this slice.

- [ ] **Step 4: Update checklist**

In `STARFIELD_TODO.md`, change:

```markdown
- [ ] Load Starfield `.mat` material files referenced by BSGeometry shapes.
- [ ] Apply texture paths from material files to the shape texture controls.
- [ ] Verify texture UI/export behavior with Starfield NIFs whose Shader tab is editable in Shape Properties.
```

to:

```markdown
- [x] Load loose Starfield `.mat` material files referenced by BSGeometry shapes.
- [x] Apply texture paths from loose material files to the shape texture controls.
- [x] Verify texture UI behavior with Starfield NIFs whose Shader tab is editable in Shape Properties.
```

Keep the `.cdb` item unchecked.

- [ ] **Step 5: Final build and status check**

Run:

```bat
msbuild BS_OS.sln /m /p:Configuration=Release /p:Platform=x64
git status --short
```

Expected:
- Build succeeds.
- Only intentional source/docs/project changes are present.
- `Config.xml`, `OutfitStudio.xml`, and `Log_OS.txt` are not staged.

- [ ] **Step 6: Commit verification/docs**

```bash
git add STARFIELD_TODO.md
git commit -m "Docs: Mark loose Starfield material loading complete"
```

## Self-Review

- Spec coverage: parser, loose material lookup, PBR texture mapping, `.mat` picker, legacy specular UI cleanup, and verification are covered.
- Scope: archived vanilla `.cdb` lookup is deliberately excluded and remains the next slice.
- Type consistency: `SFMaterialFile`, `SFMaterialTextureSlot`, and `GetTextureFiles(size_t)` are defined before use.
- Build coverage: parser smoke test and full Outfit Studio Release x64 build are both included.
