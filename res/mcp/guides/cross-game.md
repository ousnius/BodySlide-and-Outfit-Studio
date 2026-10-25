# Converting between games
Bring an outfit from one game or body to another, e.g. Fallout 4 CBBE to Skyrim SE CBBE or UBE: importing it, conversion references, weights, partitions, and how shaders, materials and texture maps differ between the games.

The games differ in NIF format, skeleton, shaders and texture layout. The shape and sliders carry over; bone weights, partitions and materials have to be made again for the target game.

## Importing

`load_project` and `import_file` only take the open project's game. **`import_from_game`** brings in another game's NIF, or a set of another game's BodySlide folder (its `.osp`, with `set`; the shape data comes from that folder's `ShapeData`):

- Shapes come over in global space (skin space and node transforms applied), with UVs, normals and vertex colors.
- From a set, the outfit's own slider data and zaps come along with the slider flags (`sliders: "local"`, the default; `"all"` also takes data from the body's shared files).
- Bone weights don't, since the skeletons differ; the shapes come in skinned without weights.
- `skipShapes` leaves out shapes, e.g. the source game's body when you don't need it.
- `dataFolders` (the mod's or game's Data folder that holds `Materials/`) lets it read the Fallout 4 material (BGSM) and set up the Skyrim shader from it. The result lists each shape with its material file and texture paths, `materialNotes` on what doesn't map (each note once, with its shapes) and `texturesNotInGameData`. `verbose: true` adds every material value; `read_material` reads a BGSM or BGEM on its own.
- The `ImportFromGame` automation step does the same in `run_steps` and scripts (`FilePath`, `Set`, `DataFolders`, `Shapes`, `SkipShapes`, `Sliders`), for converting many files in one call (see `basics`, "Many outfits at once").

**Import the source body too** and make it the reference with `set_reference_shape`. Then the source's clipping can be measured, and the next `load_reference` carries its zaps (e.g. `BodyZap`) onto the target body.

## Workflow

Measured on a Fallout 4 CBBE wetsuit with sleeves, belt and thigh holsters, converted to Skyrim SE CBBE and then UBE:

1. **Import** the outfit and its body with `import_from_game`, and set the body as reference.
2. **Load the conversion reference** for the two bodies. Look for "Convert: …" templates in `list_reference_templates`, e.g. "Convert: FO4 CBBE To SSE CBBE" (also "SK CBBE To SSE CBBE" for Skyrim LE, and from other bodies like "Vanilla To CBBE" or "UNP … To CBBE"). At sliders 0 it should match the source body: `analyze_clipping` close to what step 1's body gave (the wetsuit 60 against 26, the belt 585 against 544).
3. **Conform it** (see `conform-sliders`). The Fallout 4 to Skyrim slider also lowers the arms from Fallout 4's rest pose to Skyrim's, which pushes the forearms through tight sleeves: `auto_fix_clipping` on that slider alone took the sleeves from 301 to 20 new clipping vertices. Fix it again after `analysis_exclusions` `op: "zaps"` with `zaps: []`, so it also holds with the body zap off. Rigidify the slider on buckles, clips and pouches, then bake it: slider at 1, `set_base_shape`.
4. **Load the target body** ("CBBE Body"), conform its sliders, rigidify again.
5. **Weights:** `copy_bone_weights` from the target body (see `bone-weights`). When the source had breast and butt physics, copy from the physics variant ("CBBE Body Physics"). A body without hands leaves the old hand bones empty; `delete_bones` removes them.
6. **Partitions or segments:** `copy_partitions` from the target body (below).
7. **Shader and textures** (below).
8. **Check** as for any conversion (`body-conversion`): sweep, presets, poses, `validate_project`.
9. For another body of the target game, continue from the result as a body conversion, e.g. CBBE SE to UBE through "3BA to UBE 2.0" (see `body-conversion`). With 3BA or UBE, copy weights with `excludeBones` for the genital and anus physics bones, or a suit's crotch follows their physics.

## Outfits made for a body preset

Many mods model their outfits on their own body shape, a preset of the body (e.g. a CBBE preset with bigger breasts and hips), not on the body's base shape. The game's conversion template expects the base shape, so take the preset off first, in an instance of the **source** game:

1. **Find the preset.** Mods often ship it (BodySlide `SliderPresets`); copy it into this BodySlide folder's `SliderPresets`. Load the body as reference, import a few skin-tight pieces, and `analyze_clipping` with `preset` for each candidate and with no sliders. The right preset has the fewest vertices inside the body and many close to it. On a Fallout 4 mod a bodysuit had 2611 vertices inside plain CBBE and 100 inside its "nude" preset.
2. **One preset rarely fits every piece.** Presets of one mod often differ only at the breasts: a "nude" one and an "outfit" one with the breasts pushed up and together. Natural tops, swimsuits and bodysuits fitted the nude preset, while corsets and push-up tops fitted the outfit one (a corset: 865 inside the nude preset, 77 inside the outfit one). Nipple piercings and chains only touched the nipples with one preset. Measure each upper-body piece against each preset and take the best. Pieces below the breasts measure the same with every preset.
3. **Make a conversion reference per preset:** load the body, `apply_preset`, `snapshot_save`, `make_conversion_reference` (e.g. slider `PresetToBase`). Check the direction: at 0 `analyze_clipping` should give the preset's numbers, at 1 those of the base shape. Delete any outfit shapes you imported to test, then `save_project` it as a reference set with `copyReference: true`.
4. **Per piece:** `load_reference` with its set, `conform_sliders`, slider at 1, `set_base_shape`, `export_file` the NIF. Then `import_from_game` those NIFs into the target game and convert as above.
5. **Cloth between the breasts** that doesn't follow the cleavage (a shirt or dress top spanning from breast to breast, an open front with a hem across the gap) stays where the bigger breasts pushed it out: the body vertices under it, deep in the cleavage, barely move with the preset, so after the bake the cloth floats in front of the smaller breasts. Make a second conversion reference per preset from the UniBoob shape: its surface spans the gap, so the cloth over it moves in with the breasts.
   - **Don't apply the preset to the UniBoob template's own sliders.** "Sliders: CBBE Uniboob (no nipples)" in Fallout 4 has every CBBE breast slider by name, but they don't shape like the body's: with a Vtaw preset the breast tip moved 0.11 back instead of 2.57, so cups converted from it kept the big breasts, with blocky edges where the blend switched references.
   - Instead give the UniBoob mesh (`ShapeData/References/CBBEUniboobNoNips.nif`, the CBBE mesh with the cleavage filled) the body's morphs: load "CBBE Body", `import_file` the mesh, `conform_sliders`, `apply_preset`, `set_base_shape`. Then `load_reference` the body's preset conversion set and `conform_sliders`. Its breasts now match the body conversion (2.55 against 2.57 at the tip), and the fill takes the average of both breasts.
   - Smooth only the fill: `mask_ops` `from_analysis` metric `clipping` against the body with `target: "protect"`, `threshold: -0.05` (about 780 fill vertices stay unmasked), `smooth` the mask 2 steps, `smooth_slider_deltas` 10 iterations, clear the mask. Then check with `analyze_slider`: 0 flipped triangles, roughness about 0.017.
   - `set_reference_shape` to the UniBoob mesh, `delete_shapes` the body, `save_project` with `copyReference: true`.
   - Check the result before blending: at the breast tip, `get_slider_deltas` of the conversion should match the body's.

   Many pieces mix both (cups that follow each breast with laces or a bridge across the gap), so blend per vertex instead of choosing per piece (`conform-sliders`, "Blending two conversions"):
   - Conform the UniBoob reference, `clone_slider` it to `ConvUni`, load the body's reference with `mergeSliders: true, appendNewSliders: false` and conform only the conversion slider. Conform both with `SmoothResults: true, SmoothIterations: 12, SmoothStrength: 0.5` (see "Spikes under the breasts" below).
   - Decide by the distance to the **body**, not the UniBoob shell: `mask_ops` `from_analysis`, metric `clipping` against the body reference, `sliderValues` with the conversion slider at 0, `threshold: -0.6`, `target: "fix"`, `grow: 1`, `feather: 3`. Cloth within 0.6 of the body stays unmasked and follows the body. Farther cloth is masked and takes the UniBoob. Inside the UniBoob shell is the wrong test: the shell's front lies ahead of the cleavage, so laces between the cups were inside it (567 of 882 vertices) while still on the body.
   - `clear_slider_data` `ConvUni` with the mask, invert, `clear_slider_data` the conversion slider, clear the mask, both sliders to 1, `set_base_shape`.
   - On a laced top with a pushed-together preset, the body-only bake bunched the inner cup edges outward at the cleavage; the blend laid them flat and the laces straight.
   - Mask and blend have no automation step for the measurement, so batch it as `run_steps` (load, import, conform, clone, second reference, conform, `ClearMask`, `InvertMask`), `mask_ops`, `run_steps` (clear, invert, clear, bake, export). The full mask before `mask_ops` gives the UniBoob conversion to shapes with no vertex near the body (bridges, zips, pistols), which `from_analysis` leaves unchanged. Chain each piece's finishing steps with the next piece's setup to need two calls per piece, and keep a list: running the finishing steps on the wrong piece exports it under the next piece's name.
   - Blended this way, 61 upper-body pieces of a Fallout 4 mod (tops, corsets, jackets, dresses, swimsuits, harnesses, necklaces, nipple chains) came out right. Tight pieces stayed almost entirely on the body conversion. A T-shirt went mostly UniBoob and spanned the chest evenly (11 vertices clipping at rest, max 0.16), and a dress's skirt was unaffected because both references agree away from the chest.
6. **Spikes under the breasts:** a plain conform of a big preset change leaves jagged cloth where it bridges the crease under the breasts. Each cloth vertex takes its move from its nearest body points, and those switch between the underside of the breast, which moves a lot, and the ribcage, which barely moves. This isn't caused by blending: body-only, UniBoob-only and blended bakes had the same spikes (tank top 79/79/81 vertices, Top 2 196/190/189). Smoothing the conform result fixes it: 12 iterations at 0.5 took both pieces to 0, while clipping at rest stayed the same (115 against 111 vertices, max 0.14) and the mean gap change rose only from 0.01 to 0.03. 3 or 6 iterations left some spikes. To find them across a batch, export the bakes and the sources to OBJ and compare each vertex's distance from its neighbours' average: vertices where it grew by more than 0.15 of the median edge length are spikes the bake added.
7. **Push-up pieces on the base body:** a top modelled on a pushed-up preset sits higher than the base body's breasts, and the lower breast shows below the cup rim. That's the shape difference, not a conform error; bring the hem down where it shows.

Head items (masks, glasses, face piercings) aren't affected by a body preset and skip this.

## Converting a whole mod

- **List the pieces first:** compare the NIFs (vertex and triangle counts, positions, UVs) to find exact duplicates and texture-only variants. Texture swaps don't need their own set; the plugin points to other textures.
- Use the source game's instance for the preset step and the target game's for the rest, and batch both with scripts (see `basics`, "Many outfits at once"). One script per kind of piece covers most of a mod: worn with the body, without the body, feet, hands.
- **Pieces with feet or hands:** Fallout 4 shoes and gloves often include part of the body's feet or hands; leave those out with `skipShapes` and load "CBBE Feet" or "CBBE Hands" as the target, saved with them. High heels need the `HH_OFFSET` NiFloatExtraData (`set_extra_data`) on the shoe shapes, as other Skyrim heels have it.
- Pieces without a body (belts, pants, jewellery) still conform to and copy weights from the body; save them with `copyReference: false`.

## Partitions and segments

- **Skyrim and older** split skinned shapes into partitions, each with a body slot that decides what hides it when another item with that slot is worn: 30 head, 31 hair, 32 body, 33 hands, 34 forearms, 35 amulet, 36 ring, 37 feet, 38 calves, 39 shield, 40 tail, 41 long hair, 42 circlet, 43 ears, and 44–61 free slots mods use. `get_partitions`, `set_partitions` (assign masked triangles to a slot), `copy_partitions` from the reference.
- `copy_partitions` from the body gives everything slot 32. Pieces worn together with a body piece need another slot (`ChangePartitions` step, `"32"` → `"52"`). What mods commonly use: 44 face or mouth, 45 neck, 46 chest (harness), 47 back, 48 misc, 49 pelvis (panties, pants), 52 pelvis secondary (skirts, belts), 53 and 54 legs, 55 face jewellery, 56 chest secondary, 57 shoulder, 58 and 59 arms, 60 and 61 misc. Follow what the mod's earlier conversions used when there are any.
- **Fallout 4** uses segments and subsegments with a segment file (`.ssf`): `set_segments`, `copy_partitions`.
- Every triangle needs a partition: `validate_project` lists those without.

## Shaders and materials

### Skyrim (LE and SE)

The shader (`BSLightingShaderProperty`) is in the NIF. `get_shape_properties` shows it; `set_shader_properties` changes the shader type and values, `set_texture_paths` the textures, `set_geometry_properties` the geometry flags.

- **Texture slots:** 0 diffuse (`_d`; alpha is transparency when the shape has an alpha property), 1 normal map (`_n`; **alpha is the specular mask**), 2 glow (`_g`, glow map shader) or subsurface (`_sk`, skin tint shader), 3 height for parallax (`_p`), 4 environment cube map (`_e`), 5 environment mask (`_m`), 6 subsurface or inner layer, 7 back light (`_b`) or specular. Paths are relative to Data, e.g. `textures\armor\outfit\outfit_d.dds`.
- **Specular:** the normal map's alpha masks it, `SpecularPower` (glossiness) sets how sharp the highlight is for the whole shape, `SpecularStrength` and `SpecularColor` its brightness and color.
- **Other features:** two-sided, rim, back and soft lighting, glow (`EmissiveColor`, `EmissiveMultiple`), alpha blending and testing (an alpha property).
- **Environment mapping** needs a cube map in slot 4 and a mask in slot 5. Complex Material mods read an extended mask from slot 5, with a dynamic cube map in slot 4.
- **Bodies** use model-space normal maps (`_msn`); outfits use tangent-space ones.
- **True PBR** (Community Shaders) sets a shader flag and an RMAOS map in slot 5. PBR mods usually patch the built meshes after BodySlide, so a BodySlide source mesh of a PBR mod isn't PBR; don't "fix" that.
- **Texture formats:** Skyrim SE takes BC7; Skyrim LE doesn't (use BC1 without alpha, BC3 with alpha).
- Transparent effects (glass, energy) use an effect shader (`BSEffectShaderProperty`), not the lighting shader.

### Fallout 4

The NIF's shader names a material file (BGSM, or BGEM for effects) under `Materials\`, which the game reads instead of the NIF's values. `read_material` shows one; `write_material` writes one (below).

- **BGSM textures:** diffuse (`_d`), normal (`_n`, usually BC5 with only red and green; the game rebuilds blue), smoothness/specular (`_s`: **red is specular, green is smoothness**), plus glow, environment, greyscale (palette color), inner layer, wrinkles and displacement maps.
- Specular, smoothness, two-sided, alpha, rim and back lighting, subsurface and glow are material values.

### Fallout 4 to Skyrim

`import_from_game` with `dataFolders` sets textures, specular, glossiness, two-sided, alpha, glow, rim and back lighting from the BGSM. What it can't do, and its `notes` say:

- **Normal map with specular:** build a new normal map from the Fallout 4 `_n` with blue rebuilt (z = √(1 − x² − y²)) and the `_s` red channel as alpha. Save it as BC7 (SE) or BC3 (LE). Red and green point the same way in both games (red along +U, green along +V, i.e. down the image: DirectX style), so keep them as they are and don't flip green.
- **SpecularPower:** the tool guesses it from the material's smoothness alone, which is too high when the `_s` green (gloss) is dark. Set it from the average of the `_s` green: about 2^(gloss·10+1). A wetsuit came out at 28, its belt at 24.
- **Environment mapping** isn't set up: make a mask for slot 5 and pick a cube map for slot 4.
- **Greyscale-to-palette coloring** has no Skyrim equivalent: bake the color into the diffuse.
- **BGEM effect materials** need an effect shader in Skyrim; only their textures are set.
- **PBR materials** (Fallout 76) don't map to Skyrim's lighting shader.

`convert_textures` makes Skyrim and Fallout 4 maps from PBR maps (base color, normal, roughness or gloss, metallic, ambient occlusion, opacity); see `external-models`. For channel work it doesn't cover, like rebuilding a Fallout 4 normal map's blue channel, tell the user which maps to make and how, or use a texture tool (e.g. texconv) if one is available to you.

### Skyrim to Fallout 4

The shapes need a BGSM per material (a BGEM for effect shaders). **`write_material`** writes them:

- `shape` starts from a Skyrim shape's shader: textures, specular, smoothness from `SpecularPower`, two-sided, alpha, glow, rim, back and soft lighting. Its `notes` say what's missing.
- `base` starts from another material file instead, e.g. a vanilla one with the right look; `properties` changes values on top, named as `read_material` returns them (`"textures": {"smoothSpec": "MyOutfit/suit_s.dds"}`).
- Write into the mod's `Materials` folder, e.g. `D:/Mods/MyOutfit/Materials/MyOutfit/suit.bgsm`; texture paths are relative to `Textures`. Fallout 4 is version 2 (the default), Fallout 76 20 to 22.
- In a Fallout 4 project, `assignToShapes` names the material in the shapes' shaders. The window shows it once the file is in the game's Data folder.

Textures: the diffuse stays; the Skyrim normal map's alpha becomes the red of an `_s` map (with smoothness in green), and the normal map loses its alpha. Without an `_s` map the shape has no specular, so make one and set `smoothSpec`.

A Fallout 4 shape's own material can be changed the same way: `write_material` with `shape` starts from the material file it has loaded.

### Starfield

Starfield keeps geometry in separate `.mesh` files, morphs in `morph.dat` files (`export_slider_data` `dat`) and materials in its material database. The tools don't map Starfield materials, and skinned vertices can have up to 8 bones. Convert the shape and sliders as above, and tell the user what the materials need.
