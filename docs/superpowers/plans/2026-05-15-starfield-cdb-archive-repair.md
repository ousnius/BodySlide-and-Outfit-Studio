# Starfield CDB Archive Repair Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Starfield archived `.cdb` material lookup resolve real vanilla material texture paths, starting with `Materials\Actors\Human\Naked_Body\Female\Naked_F_Body.mat`.

**Architecture:** Keep loose `.mat` support as the first resolver. Fix the CDB reflection reader so it matches NifSkope/libfo76utils behavior for real Bethesda CDB files, then reuse the existing `SFMaterialDatabase` extraction path.

**Tech Stack:** C++17, BodySlide/Outfit Studio FSEngine BA2 archive layer, Starfield reflection CDB format, lightweight unit tests compiled with `g++`, Release x64 MSBuild verification.

---

### Task 1: Reproduce The Real Failure

**Files:**
- Test: `tests/SFMaterialDatabaseTest.cpp`
- Fixture: `build/sf-material-probe/materialsbeta.cdb`

- [x] **Step 1: Add a fixture-driven failing test**

Use `SF_MATERIAL_CDB_FIXTURE` to load a real extracted `materialsbeta.cdb` and assert `materials/actors/human/naked_body/female/naked_f_body.mat` resolves a color texture.

- [x] **Step 2: Run test to verify it fails**

Run:

```powershell
$env:SF_MATERIAL_CDB_FIXTURE='C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\BodySlide-and-Outfit-Studio\build\sf-material-probe\materialsbeta.cdb'; build\SFMaterialDatabaseTest_probe.exe
```

Expected before fix: fail with `Real CDB fixture should resolve Naked_F_Body material`.

### Task 2: Fix Built-In Reflection String Offsets

**Files:**
- Modify: `src/files/SFMaterialDatabase.cpp`
- Test: `tests/SFMaterialDatabaseTest.cpp`

- [x] **Step 1: Add synthetic coverage for built-in type offsets**

Update `CDBBuilder` so it can write primitive type offsets such as `0xFFFFFF02` for `String` and `0xFFFFFF0D` for `uint32_t` instead of writing those names into the STRT table.

- [x] **Step 2: Run test to verify it fails before production fix**

Run:

```powershell
g++ -std=c++17 -I. -Isrc tests\SFMaterialDatabaseTest.cpp src\files\SFMaterialDatabase.cpp src\utils\StringStuff.cpp -o build\SFMaterialDatabaseTest_builtin_red.exe
build\SFMaterialDatabaseTest_builtin_red.exe
```

Expected before fix: fail because built-in primitive types resolve as empty strings.

- [x] **Step 3: Implement `ReflectionStream::GetString()` built-in fallback**

Match NifSkope behavior: if an offset is not in STRT, map `0xFFFFFF01..0xFFFFFF13` to built-in reflection names (`null`, `String`, `List`, `Map`, `<ref>`, primitive numeric types, `bool`, `float`, `double`, `<unknown>`).

- [x] **Step 4: Run synthetic and real fixture tests**

Run:

```powershell
build\SFMaterialDatabaseTest_builtin_red.exe
$env:SF_MATERIAL_CDB_FIXTURE='C:\Users\DJLegnds\Downloads\Mods\BS & OS commit for SF\BodySlide-and-Outfit-Studio\build\sf-material-probe\materialsbeta.cdb'; build\SFMaterialDatabaseTest_builtin_red.exe
```

Expected after fix: both pass.

### Task 2B: Fix Inherited CE2 Material Link Overrides

**Files:**
- Modify: `src/files/SFMaterialDatabase.cpp`
- Test: `tests/SFMaterialDatabaseTest.cpp`

- [x] **Step 1: Add synthetic coverage for base material texture-set override**

Create a base material with a hand texture set and a derived material with the same `BSMaterial::TextureSetID` key pointing at a body texture set. Expected before fix: derived material incorrectly resolves the inherited hand texture.

- [x] **Step 2: Build an effective component map per object**

Match NifSkope's CDB behavior: inherited base components are copied first, then derived components with the same class/index key override them.

- [x] **Step 3: Follow CE2 material graph links instead of broad child recursion**

Resolve `LayerID -> MaterialID -> TextureSetID -> MRTextureFile/TextureFile`, with a conservative linked-component fallback for non-CE2 synthetic/material data. Do not walk every child object from `EdgeInfo`.

- [x] **Step 4: Run synthetic and real fixture tests**

Expected after fix: synthetic inherited override returns the body texture, and the real `Naked_F_Body.mat` fixture no longer resolves hand textures.

### Task 3: Confirm Archive Integration Still Builds

**Files:**
- Modify only if needed: `src/program/OutfitProject.cpp`, `lib/FSEngine/FSBSA.cpp`

- [x] **Step 1: Run focused loose and CDB tests**

Run both focused tests and confirm exit code 0.

- [x] **Step 2: Run project checks**

Run `git diff --check` for changed files and XML parse for `OutfitStudio.vcxproj` / `.filters`.

- [x] **Step 3: Build Outfit Studio Release x64**

Run:

```powershell
$cleanPath = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User'); [Environment]::SetEnvironmentVariable('PATH', $null, 'Process'); [Environment]::SetEnvironmentVariable('Path', $cleanPath, 'Process'); & 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' OutfitStudio.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Expected: `Build succeeded. 0 Warning(s) 0 Error(s)`.
