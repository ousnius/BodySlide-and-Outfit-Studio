# Converting between games
Bring an outfit from one game or body to another, e.g. Fallout 4 CBBE to Skyrim SE CBBE or UBE: importing it, conversion references, weights, partitions, and how shaders, materials and texture maps differ between the games.

The games differ in NIF format, skeleton, shaders and texture layout. The shape and sliders carry over; bone weights, partitions and materials have to be made again for the target game.

## Importing

`load_project` and `import_file` only take the open project's game. **`import_from_game`** brings in another game's NIF, or a set of another game's BodySlide folder (its `.osp`, with `set`; the shape data comes from that folder's `ShapeData`):

- Shapes come over in global space (skin space and node transforms applied), with UVs, normals and vertex colors.
- From a set, the outfit's own slider data and zaps come along with the slider flags (`sliders: "local"`, the default; `"all"` also takes data from the body's shared files).
- Bone weights don't, since the skeletons differ; the shapes come in skinned without weights.
- `skipShapes` leaves out shapes, e.g. the source game's body when you don't need it.
- `dataFolders` (the mod's or game's Data folder that holds `Materials/`) lets it read the Fallout 4 material (BGSM) and set up the Skyrim shader from it. The result lists what was taken over and `notes` on what doesn't map. `read_material` reads a BGSM or BGEM on its own.

**Import the source body too** and make it the reference with `set_reference_shape`. Then the source's clipping can be measured, and the next `load_reference` carries its zaps (e.g. `BodyZap`) onto the target body.

## Workflow

Measured on a Fallout 4 CBBE wetsuit with sleeves, belt and thigh holsters, converted to Skyrim SE CBBE and then UBE:

1. **Import** the outfit and its body with `import_from_game`, and set the body as reference.
2. **Load the conversion reference** for the two bodies. Look for "Convert: …" templates in `list_reference_templates`, e.g. "Convert: FO4 CBBE To SSE CBBE" (also "SK CBBE To SSE CBBE" for Skyrim LE, and from other bodies like "Vanilla To CBBE" or "UNP … To CBBE"). At sliders 0 it should match the source body: `analyze_clipping` close to what step 1's body gave.
3. **Conform it** (see `conform-sliders`). The Fallout 4 to Skyrim slider also lowers the arms from Fallout 4's rest pose to Skyrim's, which pushes the forearms through tight sleeves: `auto_fix_clipping` on that slider alone took the sleeves from 301 to 20 new clipping vertices. Rigidify the slider on buckles, clips and pouches, then bake it: slider at 1, `set_base_shape`.
4. **Load the target body** ("CBBE Body"), conform its sliders, rigidify again.
5. **Weights:** `copy_bone_weights` from the target body (see `bone-weights`). When the source had breast and butt physics, copy from the physics variant ("CBBE Body Physics"). A body without hands leaves the old hand bones empty; `delete_bones` removes them.
6. **Partitions or segments:** `copy_partitions` from the target body (below).
7. **Shader and textures** (below).
8. **Check** as for any conversion (`body-conversion`): sweep, presets, poses, `validate_project`.
9. For another body of the target game, continue from the result as a body conversion, e.g. CBBE SE to UBE through "3BA to UBE 2.0" (see `body-conversion`). With 3BA or UBE, copy weights with `excludeBones` for the genital and anus physics bones, or a suit's crotch follows their physics.

## Partitions and segments

- **Skyrim and older** split skinned shapes into partitions, each with a body slot that decides what hides it when another item with that slot is worn: 30 head, 31 hair, 32 body, 33 hands, 34 forearms, 35 amulet, 36 ring, 37 feet, 38 calves, 39 shield, 40 tail, 41 long hair, 42 circlet, 43 ears, and 44–61 free slots mods use. `get_partitions`, `set_partitions` (assign masked triangles to a slot), `copy_partitions` from the reference.
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

The NIF's shader names a material file (BGSM, or BGEM for effects) under `Materials\`, which the game reads instead of the NIF's values. `read_material` shows one.

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

The tools don't convert texture files. Tell the user which maps to make and how, or use a texture tool (e.g. texconv) if one is available to you.

### Skyrim to Fallout 4

The shapes need a BGSM per material, which no tool here writes: tell the user to make them with a material editor. Textures: the diffuse stays; the Skyrim normal map's alpha becomes the red of an `_s` map (with smoothness in green), and the normal map loses its alpha.

### Starfield

Starfield keeps geometry in separate `.mesh` files, morphs in `morph.dat` files (`export_slider_data` `dat`) and materials in its material database. The tools don't map Starfield materials, and skinned vertices can have up to 8 bones. Convert the shape and sliders as above, and tell the user what the materials need.
