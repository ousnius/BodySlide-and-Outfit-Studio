# Converting models from other programs
Bring an outfit modelled in another program (an OBJ or FBX from Blender, a sculpting tool, an asset store) onto a game body: import, size, pose, fit without clipping or distortion, high heels, gloves and boots, textures.

Such models have no skeleton, other proportions, another rest pose (an A-pose or T-pose) and usually their own body under the outfit. They are fitted in this order: size, pose, unpose, fit, then the usual conversion (sliders, weights, partitions, see `body-conversion` and `bone-weights`).

## 1. Import

- `import_file` with `upAxis: "y"` for files that are Y up (Blender's default OBJ export). `skipObjects` leaves out parts the game handles itself: eyes, eyebrows, eyelashes, hair (a separate item at most).
- Keep the model's own skin body for now, even if it's only the visible parts: it is the best guide for posing and for keeping each garment's gap. Delete it at the end.
- The result lists each object's material and texture files (`objObjects`), for `convert_textures`.

## 2. Templates

- `load_reference` with the target body, e.g. "CBBE 3BA Body".
- Bodies without hands or feet need them as templates: `add_project` the body's hands and feet sets.
- **High heels**: when the model stands on heels, use high heel feet instead (feet modelled on tiptoe that reach below the ground, from a heels set of the body; delete the shoe shapes it brings along). Flat feet make the size and pose wrong and flatten the heels.

## 3. Size first

`scale_to_body` scales all the model's shapes evenly about their feet and moves the feet onto the body's.

- It measures leg length, floor to crotch, by cutting both with horizontal slices. Pass `targets` that cover the torso: the skin body plus close-fitting torso garments (a corset), because skin bodies of models often have holes where clothes cover them. Boots count for the legs.
- Check `method` in the result: "leg length" is reliable; "best match" (no gap between the legs, e.g. under a long dress) isn't. Always check with `render_view` from the front and the side: crotch, waist, chest and shoulders should be at the body's heights. Give `scale` to correct it.
- Don't scale during the pose fit (`scaleBody` stays off): arms in an A-pose skew any scale fitted together with the pose.

## 4. Pose

`fit_pose_to_shapes` with `templates` (body, hands, feet) and `targets` (skin body, gloves, boots, close-fitting torso garments; not a cape or hat).

- It moves the body as a whole first, then fits each joint within its natural range, then corrects bone lengths slightly. `distanceBefore`/`distanceAfter` and per-bone `distance` show how well it matched; under 1 is good.
- Check with `render_view` and `skeleton: true` from the front and the side. Correct bones with `set_preview_pose keepCurrent`.
- Leave `fingers` off unless the model's hands are close to the body's relaxed hands. Stylized hands (long, curled, clawed) match the wrong fingers.

## 5. Unpose

`unpose_shapes` takes the model's shapes to the skeleton's rest pose.

- `rigidParts` keeps small pieces (chain links, buckles, nails) whole.
- A cape or other loose part that shouldn't swing with the arms: unpose it on its own with `excludeBones` on the upper arms.
- `maxStrain` in the result shows how much edges were stretched; over 0.25 on a whole shape is worth a look.

## 6. Fit to the body

`fit_to_body`, a shape or group at a time, with the target body (and hands or feet only where they show). Read `maxStrain`, `meanStrain`, `flippedTriangles`, `insideAfter` and `bodyPokingThroughAfter` in the result, and render. Flipped triangles are turned back with `fix_flipped_triangles` and the fit's automatic snapshot.

- **Garments over the model's skin body** (corset, top, pants): `gapMode: "fromShape"` with `gapFrom` = the model's skin body keeps each vertex's gap to it. `rigidParts` with `maxPartVertices` around 120 keeps buckles, studs and chain links whole.
- **Stiff accessories** (collar, choker, bracelet, belt): `rigidParts` with `maxPartVertices` as large as the shape and `rigidScale: true`: the piece moves and scales as a whole instead of bending onto the body. A choker from a long thin neck otherwise flares down onto the shoulders.
- **Gloves, boots and shoes that cover the hands or feet**: fit them against the body only (not the hands or feet), with a constant `gap` (0.25 to 0.4) and a short `maxDistance` (3). The game hides the hands and feet under them, so their hand and foot parts keep the model's own shape, heels included, and clipping there doesn't matter. Exclude it from measurements with `analysis_exclusions` `reference_edges`.
- **Loose parts** (cape, long coat, skirt panels): `gapMode: "keep"` with a `minGap` of 0.3 only pushes out what clips.
- Clipping left at the end is pushed out by the fit (`clippingPasses`), including the body bulging through between a coarse garment's vertices. Where the fit would stretch edges more than `maxStrain` (0.3), it gives way and keeps the shape's form (`keptForm`).
- For what still clips afterwards: `fit_to_body` again with `gapMode: "keep"` and `minGap` 0.1 moves only the clipping vertices, along the body's normal.
- Then delete the model's skin body and the hand and foot templates, and `analyze_clipping` against the body.

## 7. High heels

Skyrim's high heels sink below the ground; the game lifts the character by the heel height. After fitting the shoes, `set_extra_data` on them: name `HH_OFFSET`, type `NiFloatExtraData`, value how far their lowest point is below zero (the `bounds` min z of `list_shapes`, as a positive number, e.g. 6.4).

## 8. The rest of the conversion

As for any outfit (see `body-conversion`, `bone-weights`, `fix-clipping`), with these differences for models from other programs:

- Delete the model's skin body first, then `conform_sliders` on the outfit shapes. A separate item like a hat gets no body sliders.
- `rigidify_slider` on the rigid pieces only: mask the large pieces first (`list_mesh_parts`, `mask_ops` `parts`), so bands around a limb still follow its sliders while nails, gems, soles and chain links stay rigid.
- **Weights from several templates.** `copy_bone_weights` from the body leaves the parts past its open wrists and ankles unweighted (the result's `unweighted`). Then `set_reference_shape` the hands template and copy only the hand and finger bones (`bones`) to the gloves, the feet template with the foot and toe bones to the shoes, and `set_reference_shape` the body again. Exclude the genital and anus physics bones of bodies like 3BA (`list_skeleton_bones` `inProject` finds them).
- **Fingers of stylized gloves** (long, clawed or curled) copy the wrong fingers' weights and tear when fingers move (`analyze_poses` max strain 50 and more). Until they're weighted by hand, `reassign_weights` all finger bones onto each hand bone: the glove fingers then stay stiff and never tear.
- **Capes and other hanging parts:** copy from the body with a large `proximityRadius` (45) so every vertex gets weights, then `reassign_weights` the leg and butt bones onto the pelvis and each arm's bones onto its clavicle, and `smooth_weights` with a `radius` around 12. Copying only torso bones leaves the hem unweighted, since the body vertices closest to it are on the legs.
- In `analyze_poses`, compare gloves and shoes with the hands and feet templates in the same pose, not with a body without hands or feet: poses that scale the hand or foot bones stretch the templates as much.
- `copy_partitions` from the body, then `set_partitions` gloves 33 and shoes 37 (they hide the hands and feet in game), a hat or hood 31 or 42.
- Thin chains lying on the chest clip with presets that push the breasts together (cleavage presets), shallowly: the links can only move as wholes. `auto_fix_clipping` and `rigidify_slider` `clearance` barely help there; tell the user rather than distorting the chains.
- A separate item (hat): keep a snapshot, delete it from the project and save the outfit; restore, delete everything else, `set_geometry_properties` `skinned: true`, `add_bones` the head bone, `set_weights` every vertex to it, set its partition, and save it as another set of the same slider set file with `copyReference: false`.

## Textures

`convert_textures` turns PBR maps into the game's: for Skyrim `_d`, `_n` with the specular mask in alpha (from roughness and metallic) and `_m` environment mask (from metallic), with the normal map's green turned when it is OpenGL style. Maps are scaled to `maxSize` (2048) with mipmaps.

- Write them where the game and the window find them: the mod's or the game's `Data\textures\...` folder as `outputFolder`. Texture paths in the shapes are relative to Data, e.g. `textures\armor\mine\mine_d.dds`.
- Give `texconv` for BC7 (Skyrim SE, Fallout 4); without it the maps are DXT1/DXT5. When the user has no texconv.exe, recommend downloading it from https://github.com/Microsoft/DirectXTex/releases/latest/download/texconv.exe.
- Apply the files with `set_texture_paths` (0 diffuse, 1 normal, 4 cube map, 5 environment mask) and the `suggestedShader` values with `set_shader_properties`. `ShaderType` takes numbers: `BSLighting:1` is the environment map shader. Vanilla Skyrim cube maps are in `textures\cubemaps\`, e.g. `shinybright_e.dds` or `chrome_e.dds`.
- Shapes imported from OBJ have a default shader: the emissive flag on and specular off. `set_shader_properties` `flags`: `{"specular": true, "emissive": false}`, and `{"doubleSided": true}` for capes and other parts seen from both sides.
