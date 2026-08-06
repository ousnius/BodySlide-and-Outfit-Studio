# Ported HDT-SMP physics core

The files in this directory are a port of the `hdtSkinnedMesh` core of
**Faster HDT-SMP (FSMP)**, the skinned-mesh physics plugin for Skyrim SE/AE/VR:

* Source: <https://github.com/DaymareOn/hdtSMP64> (`dev` branch, ported August 2026)
* Maintained by DaydreamingDay / DaymareOn, a fork of
  [Karonar1/hdtSMP64](https://github.com/Karonar1/hdtSMP64), itself a fork of
  [aers/hdtSMP64](https://github.com/aers/hdtSMP64), from the original
  [hdt-skyrimse-mods](https://github.com/HydrogensaysHDT/hdt-skyrimse-mods) by
  HydrogensaysHDT
* License: GPL-3.0, the same license as BodySlide and Outfit Studio

They exist so Outfit Studio can preview the physics of the very same XML files
the game plugin consumes, with the same behavior. Please keep them close to
upstream so the two stay diffable: fix bugs upstream where possible, and do not
restructure or "improve" the simulation code here.

`XmlReader` keeps the pull-style interface of its upstream counterpart, so the
parsing code in `PhysicsSystemBuilder` stays comparable with upstream, but it
is implemented on the tinyxml2 this project already ships rather than on the
XmlInspector parser vendored by FSMP. The physics XML format is ordinary
well-formed XML - FSMP's own validator reads the same files with pugixml and
validates them against XSD and Schematron schemas - so no behavior depends on
which conformant parser reads them. What does matter for compatibility is the
value conversion in `XmlReader.cpp` (decimal commas, hexadecimal and octal
integers, leading `+`, `true`/`1`), and that is ported as-is.

## Changes made during the port

The simulation, collision and constraint code is ported as-is. Only the
dependencies on the game and its engine were replaced:

* `RE::` / CommonLibSSE types (`BSTSmartPointer`, `BSIntrusiveRefCounted`,
  `BSFixedString`) and Intel TBB parallel loops are replaced by the standard
  C++ equivalents in `hdtRefUtils.h`, which is the only file here without an
  upstream counterpart. `IDStr` keeps the case-insensitive comparison and
  hashing that bone and tag matching depends on.
* The skeleton is read from and written to Outfit Studio's `AnimSkeleton` and
  `AnimInfo` instead of the game's `NiNode` tree. That glue lives one directory
  up, in `PhysicsSystemBuilder` (the port of `hdtSkyrimSystem`) and
  `PhysicsController`.
* Bullet's multithreaded classes (`btDiscreteDynamicsWorldMt`,
  `btCollisionDispatcherMt`, `btParallelFor`) are replaced by their
  single-threaded equivalents. They require a Bullet built with
  `BT_THREADSAFE` plus a task scheduler, which stock Bullet packages do not
  provide.
* MSVC-only and Windows-only constructs are replaced by portable ones, since
  this code also builds on Linux.
* `XmlReader` is reimplemented on tinyxml2, as described above, which drops the
  vendored XmlInspector parser (about 14000 lines) without changing the
  interface the parsing code uses.
* Game-side pieces that have no meaning in a mesh editor are not ported:
  game hooks, `defaultBBPs.xml` name mapping (physics files are found through
  the `"HDT Skinned Mesh Physics Object"` extra data only), and the actor and
  weather queries that drive wind, which the preview replaces with its own
  wind controls.

## Known differences from the game

* Non-Hookean spring parameters are parsed but not applied. Upstream needs a
  patched Bullet for them; the preview warns and uses the ordinary springs.
* Bone pose scaling other than 1 is treated as 1.
* Wind is a preview control (strength and direction) rather than the weather
  and actor state the game derives it from, and it is stronger at maximum than
  the strongest vanilla weather.
