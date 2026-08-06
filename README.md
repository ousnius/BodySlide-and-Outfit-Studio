BodySlide and Outfit Studio
===========================

BodySlide and Outfit Studio, a tool to convert, create, and customize outfits and bodies for The Elder Scrolls, Fallout and Starfield.

[![CMake Release](https://github.com/ousnius/BodySlide-and-Outfit-Studio/actions/workflows/cmake-release.yml/badge.svg)](https://github.com/ousnius/BodySlide-and-Outfit-Studio/actions/workflows/cmake-release.yml)

## Documentation
* [Wiki Overview](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki)
* [Guides and Documentation](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki/Guides-and-Documentation)

## Build Instructions
* [Building on Windows (Visual Studio or CMake)](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki/Building-on-Windows-%28Visual-Studio-or-CMake%29)
* [Building on Linux](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki/Building-on-Linux)
* [Installation and Settings](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki/Installation-and-Settings)
* [Installation on Linux](https://github.com/ousnius/BodySlide-and-Outfit-Studio/wiki/Installation-on-Linux)

## Credits / Libraries
**Created by and/or with the help of:**
* Caliente
* ousnius
* jonwd7 for NIF and general help
* degenerated1123 for help with shaders
* NifTools team

**Relevant work:**
* [nifly](https://github.com/ousnius/nifly): C++ NIF library for the NetImmerse File Format (NetImmerse, Gamebryo, Creation Engine).
* [NiflySharp](https://github.com/ousnius/NiflySharp): Native C# / .NET version of nifly that uses source generation based on [nifxml](https://github.com/niftools/nifxml).
* [Faster HDT-SMP (FSMP)](https://github.com/DaymareOn/hdtSMP64): Skinned mesh physics for Skyrim SE/AE/VR, maintained by DaydreamingDay / DaymareOn, forked from [Karonar1](https://github.com/Karonar1/hdtSMP64) and [aers](https://github.com/aers/hdtSMP64), from the original [work](https://github.com/HydrogensaysHDT/hdt-skyrimse-mods) by HydrogensaysHDT.
  Outfit Studio's physics preview is a port of its `hdtSkinnedMesh` core (GPL-3.0, like this project) so it simulates the same physics XML files with the same behavior. See [src/physics/hdt/README.md](src/physics/hdt/README.md) for the port notes.

**Libraries used:**
* [wxWidgets](https://wxwidgets.org/)
* [nifly](https://github.com/ousnius/nifly)
* [OpenGL](https://www.opengl.org/)
* [OpenGL Image (GLI)](https://github.com/g-truc/gli)
* [Simple OpenGL Image Library 2 (SOIL2)](https://github.com/SpartanJ/SOIL2)
* [half - IEEE 754-based half-precision floating point library](https://half.sourceforge.net/)
* [Miniball](https://github.com/hbf/miniball)
* [LZ4(F)](https://github.com/lz4/lz4)
* [TinyXML-2](https://github.com/leethomason/tinyxml2)
* [nlohmann/json](https://github.com/nlohmann/json)
* [fkYAML](https://github.com/fktn-k/fkYAML)
* [Catch2 v3](https://github.com/catchorg/Catch2) (optional tests)
* FSEngine (BSA/BA2 library)
* [Autodesk FBX SDK](https://aps.autodesk.com/developer/overview/fbx-sdk) (optional FBX import/export)
* [Bullet](https://github.com/bulletphysics/bullet3) (optional physics preview)
