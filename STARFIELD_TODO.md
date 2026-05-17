# Starfield Support To-Do

Working checklist for the remaining Starfield-specific work before a wider Nexus release.

## Material And Texture Support

- [x] Load loose Starfield `.mat` material files referenced by BSGeometry shapes.
- [x] Populate Outfit Studio shader/material UI from loaded loose `.mat` data.
- [x] Apply texture paths from loose material files to the shape texture controls.
- [x] Support vanilla material lookup through the `.cdb` component database.
- [x] Decide how loose mod material files and archived vanilla material records are resolved when both exist.
- [x] Verify texture UI/export behavior with Starfield NIFs whose Shader tab is editable in Shape Properties.
- [ ] Verify archived vanilla `.cdb` resolution in Outfit Studio with a vanilla Starfield NIF.
- [ ] Later: investigate Starfield-specific PBR preview/shader rendering once texture path support is complete; for now keep sensible non-PBR defaults.

## Morph Export

- [ ] Decide the correct Starfield `morph.dat` output path strategy with ousnius.
- [ ] Support exporting custom normal deltas from BodySlide sliders to `morph.dat`.
- [ ] Support exporting custom tangent deltas from BodySlide sliders to `morph.dat`.
- [ ] Add or verify vertex color target export once the expected data source is confirmed.
- [ ] Binary-compare generated `morph.dat` output against StarfieldMeshConverter output for matching test inputs.

## LOD Meshes

- [ ] Investigate current NifSkope Starfield LOD handling as a reference.
- [ ] Load and display Starfield LOD mesh data in Outfit Studio.
- [ ] Decide whether Outfit Studio should edit LOD meshes directly or only preserve/export them.
- [ ] Investigate LOD generation requirements for Starfield external and internal geometry.
- [ ] Add export verification for NIFs containing LOD mesh data.

## Release Readiness

- [ ] Build a small Starfield sample matrix: vanilla body, armor/clothing, external geometry, internal geometry, morph-enabled.
- [ ] Confirm ShapeData NIFs still save with internal geometry only.
- [ ] Confirm BodySlide builds inherit the project geometry mode.
- [ ] Confirm no local user files are included in commits: `Config.xml`, `OutfitStudio.xml`, `Log_OS.txt`.
- [ ] Prepare release notes that clearly list supported Starfield features and known limitations.
