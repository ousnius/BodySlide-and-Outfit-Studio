---
name: outfit-conversion
description: Convert, fit and check outfits in Outfit Studio through its MCP server (outfit-studio tools such as load_project, load_reference, conform_sliders, quality_sweep, render_view). Use when asked to convert an outfit to another body (e.g. CBBE SE to CBBE 3BA or UBE), fix clipping or weights, check a conversion's quality, compare it with a finished one, or pose and render a body in Outfit Studio.
---

# Outfit conversion with the Outfit Studio MCP server

Outfit Studio (OS) controls its open window through MCP tools. Everything you do shows in the window, and most edits can be undone there.

## Setup

- Start OS with `OutfitStudio.exe --mcp` (or `--mcp-port N`), or use **File > MCP Server**. Settings has a checkbox that starts the server with OS, and the port (default 47823).
- Connect: `claude mcp add --transport http outfit-studio http://127.0.0.1:47823/mcp`, or use `example.mcp.json` in this folder.
- Only programs on the same machine can connect. Set `MCP/Token` in `OutfitStudio.xml` to also require `Authorization: Bearer <token>`.

## Ground rules

- **Start with `get_status`, and call it again after any failure or timeout.**
- **A busy error is not a bug.** Tools refuse while the user has a dialog open, is painting or is playing an animation. Ask the user, or try again later.
- **Never save over the source project.** `save_project` refuses to replace an existing set without `overwrite: true`. Save conversions under a new `displayName` and `sliderSetFile` unless the user asks otherwise. Test saves go to scratch names (`zz…`), and you delete them afterwards.
- **Take a snapshot before experiments** (`snapshot_save`) and go back with `snapshot_restore`. Snapshots are dropped when another project is opened.
  - Tools the window can't undo (conform, clipping fix, automation steps, shape, bone and slider structure changes) take an automatic snapshot first and name it in `autoSnapshot` (`auto-1`, `auto-2`, …; the last 3 are kept). `snapshot_restore` goes back to one.
  - **`snapshot_diff`** shows what changed since a snapshot: moved vertices, changed sliders with their moved count and largest delta, reweighted bones, added or removed shapes. Use it after every edit to check it changed what it should and nothing else.
- **Undo:** `undo` and `redo` take back one step in one call. A step on a slider's morph enters edit mode for that slider, like in the window.
- **Units:** NIF units. Skeleton global axes are +X the character's right, +Y her front and +Z up. Slider values run from 0 to 1, which is 0–100% in BodySlide.
- **Default shapes:** tools that take `shapes` use the selected outfit shapes when you give none. Name the shapes explicitly.
- **`run_steps`** covers everything automation can do: bones, partitions, shader and texture changes, transforms, zap sliders, mask helpers and more. `get_step_schema` describes the step types.

## Conversion workflow

1. **Load the source.** Use `list_slider_sets` (with `filter`), then `load_project`. Only sets from this BodySlide folder's `SliderSets` open there.
   - For a game mesh without a project, use `browse_archives` and `load_from_archive` (see "Game meshes from archives"), or `import_file` for loose files.
   - **Note whether the set includes the body** (`get_status` `copyReference` is true and the reference is listed). Most outfit sets do, and BodySlide builds the body with the outfit, hidden where the outfit covers it by zap sliders such as `BodyZap` (`list_sliders` with `filter: "zap"`). The conversion has to keep the body and those zaps (see "Keeping the body and its zaps").
2. **Measure the starting clipping** against the source's own reference, before converting: run `analyze_clipping` with sliders at 0 and with a few big sliders. Many released outfits already clip a bit. An open-cleavage top had 211 vertices inside its CBBE body at 0, so judge only what the conversion adds.
3. **Load the target reference.** Use `list_reference_templates`, then `load_reference` with `template`. By default the outfit's sliders are replaced by the reference's and zap sliders are kept.
   - Pick the reference by the garment: the full body (e.g. "CBBE 3BA Body") for anything that follows the body, including tops with an open cleavage.
   - Use "UniBoob" for garments that span between the breasts, and the "Uni…Curtain" references for skirts and dresses that hang free.
   - Right after loading, check `analyze_clipping` at sliders 0. A big jump over step 2 means the wrong reference: the UniBoob reference on an open-cleavage top made 897 vertices clip instead of 217.
   - When the target body has a different shape at 0 than the source body (CBBE to UBE), convert the shape first: see "Bodies without a direct template".
   - The result lists `zapsTransferred`: zap sliders of the old reference move onto the new one by closest surface point (`transferZaps`, default on).
4. **Find the rigid parts.** `list_mesh_parts` lists the connected pieces of a shape (buckles, buttons, plates, straps) with size, bounds and main bone. Conform everything, then keep the rigid ones from bending with `rigidify_slider` (see "Rigid parts").
5. **`conform_sliders`**, then **`copy_bone_weights`**. Both leave masked vertices alone, and the weight copy can be undone.
   - `conform_sliders` lists `noReferenceData`: sliders the reference has no data for keep whatever the shape had. `clearMissing: true` clears them on the unmasked vertices.
   - `copy_bone_weights` skips placeholder bones named like "Don't copy this" (and `excludeBones`), cleans the copied vertices to the game's bone limit and normalizes them, all in one undo step. Bones it added that end up without weights (the leg bones of a top) are removed again (`emptyBonesRemoved`, `removeEmptyBones: false` keeps them). The result lists `bonesAdded`, `skippedBones` and the weight problems left. One `undo` also removes the bones it added.
   - **Long garments:** vertices farther than `proximityRadius` (10) from the body lose their weights. The hem of a long dress left 158 vertices `unweighted` (the result has a `hint`); `undo` and copy again with `proximityRadius: 25`.
   - **Don't copy weights from curtain references for hanging parts.** The UBE curtain references weight the legs to the "Don't copy this" placeholder, so the panels would get no leg weights. Mask the panels and keep their original weights, or copy from the body reference.
   - **A weight copy tears hanging parts in poses.** Every vertex takes the nearest body weights, so a skirt panel between the legs is split down the middle between the two thighs (or calves), with a hard border. Always test and fix them: see "Weights in poses".
6. **Check:**
   - `analyze_weights` with `compareToReference`: no unweighted vertices, no more than 4 bones per vertex, and a low `deviation` from the body's weights where the outfit sits on it.
   - `analyze_poses`: how far the outfit stretches or tears in each pose, next to the body's own stretch (see "Weights in poses").
   - `quality_sweep`: see "Quality loop" below. Add `poses` (e.g. "Arms Up", "Bend Down" from `list_poses`) to check every case in motion too.
   - `analyze_fit`: sliders that squash the outfit into the body or pull it away (see "Fit and seams").
   - `analyze_seams`: seams that sliders pull apart or that are weighted differently.
   - Look at it: see "Visual checks".
7. **Fix** one problem area at a time, masked and at low strength (see "Quality loop"). Then repeat step 6 until the sweep and the images look good.
8. **`validate_project`** before saving: it lists what breaks in game rather than in the view (unweighted vertices, too many bones, triangles without a partition, sliders that move the body under a shape but not the shape, zap sliders that zap nothing, …), and whether the body is saved with the outfit. Fix the errors; read the warnings.
9. **Save** with `save_project` under a new name. `copyReference` defaults to what the project had, so leave it out (or pass true) when the source set included the body. `get_status` `displayName` is the name the next save uses. `export_file` writes NIF, OBJ, FBX, OSD or TRI files when they're asked for.

## Bodies without a direct template

There's no CBBE SE to UBE template, but a 3BA conversion slider works, because the CBBE 3BA body matches CBBE SE at 0. The same pattern works for any body that has a "X to Y" conversion set.

1. `load_reference` `template: "11 3BA to UBE 2.0"`. It has one slider, `3ba`, that morphs the body into UBE. `analyze_clipping` at 0 should match the source (a halter crop top: 85 vs 82 vertices).
2. `conform_sliders`, `set_slider_values` `{"3ba": 1}`, `set_base_shape`. The outfit now has the UBE shape at rest.
3. `load_reference` `template: "UBE SE 2.0 Body"`, then conform, weights and checks as usual. Clipping at rest should be close to the source (88 for that top).

**Hanging parts** (dress panels, skirts) take the curtain variants: "11 3ba lower curtain to UBE 2.0" for the shape (its slider is named `CBBE Dress to UBE`, not `3ba`) and "11 UBE SE 2.0 Uni_Lower_Curtain" for the sliders. Split the shape by part with masks:

- `list_mesh_parts`, then `mask_ops` `op: "parts"` to mask the part that should not change.
- `load_reference` with `mergeSliders: true` keeps the data already conformed while the next reference is loaded.
- The panels still carry the body's `3ba` data: `clear_slider_data` with `useMask: true` clears it from the unmasked part before baking. Set both conversion sliders to 1 for `set_base_shape`.
- The curtain references add helper sliders (`BiggerBody_ForOutfits`, `BiggerBreasts_ForOutfits`, penis and hand sliders) that only move the curtain part. Delete them unless the whole outfit has them: after loading the body again with `mergeSliders: true, appendNewSliders: false`, `delete_sliders` with `notOnReference: true` deletes every slider the body has no data for.
- Conform the curtain sliders with `clearMissing: true` on the hanging part, so it drops the body sliders the curtain doesn't have (inner thighs, knees, glute folds). The sweep showed no clipping from them on a long dress; `validate_project` lists them as `missingMorphs`, which is on purpose there.
- Measure clipping against the body (load it again with `mergeSliders: true, appendNewSliders: false`), not the curtain: the outfit sits inside the curtain shell (860 "clipping" vertices at rest on a long dress).

## Keeping the body and its zaps

- Save with the body when the source had it (`copyReference`), or BodySlide builds the outfit without a body. Each of four outfit sets converted to UBE had it.
- Zap data lives on the body. `load_reference` moves it onto the new body automatically. For a conversion where it was lost, `transfer_zaps` takes it from the original project: `file` (its slider set file), `set`, `shape` (its body, by shape or target name, e.g. "CBBE"). Missing zap sliders are created with the source's defaults and flags.
- Check a zap with `mask_ops` `op: "slider_affected"` on the body and `render_view` `display.mask`; the view doesn't hide zapped vertices. On a bodysuit, BodyZap removes 13550 of 13554 CBBE vertices and 29291 of 29298 UBE ones, BodyZapKeepArms 83% and 81%.
- `validate_project` warns about `emptyZaps`, zap sliders without data on any shape.
- Loading a set with the body and saving it again keeps its link to the reference template (`ReferenceInfo`).
- **Clipping is measured the way BodySlide builds the outfit:** what the zaps BodySlide turns on by default remove (`BodyZap` under a bodysuit or blouse) can't show through, so it doesn't count. `analyze_clipping` reports `hiddenByZaps`. On a laced blouse, BodyZap leaves 98 of 450 clipping vertices at rest; on a bodysuit it hides the whole body, so nothing counts.
  - `analysis_exclusions` `op: "zaps"` picks the zaps that hide the body instead: `zaps: []` measures the whole body (for users who turn the zap off, e.g. a tight bodysuit), `["BodyZapKeepArms"]` a variant. Without `zaps` it goes back to the defaults.

## Quality loop

`quality_sweep` measures every slider on its own, plus any `combinations` you pass, with all other sliders at 0. Sliders that go both ways ("n|p" or "p|n" in the name) are also checked at -1 (`negativeValues`). With presets, each slider is also checked at the most extreme value outside 0 to 1 that the presets set (`presetExtremes`): the UBE "for Converting Outfits" preset sets BreastsZero to -1.53 and BreastUnderCurve to 2. It lists the worst cases first:

- **`clipping.newlyPenetrating`:** vertices the case pushes into the reference that weren't inside at sliders 0. This is the clipping to fix.
  - **The body poking through counts too.** Large outfit triangles over a curve of the body can let it show through between their vertices while every vertex is outside: spots on the butt of a long dress with a UBE preset. The vertices of such a triangle count as penetrating by how far the body pokes out, and `analyze_clipping` reports `pokeThrough` (body vertices showing). Heatmaps, masks and fixes include them. The CBBE dress already had 21 at rest; the old UBE conversion had 87.
- **`morph.flippedTriangles`:** triangles that turn over when the slider is applied, i.e. folds and pinches. A few on a large morph is normal. Many means a crumpled area. `analyze_slider` with `withFlipped` returns their vertices, to look at with `frame_view` or to mask and fix locally.
- **`morph.roughness`** (absolute) and **`normalizedRoughness`:** bumpiness of the morph. Always read both, since a weaker morph looks rougher when normalized. Compare a slider's numbers before and after a fix, not against a fixed limit.
- **`missingMorph`:** the reference moves with this slider but the shape doesn't. That usually needs a conform with that slider, or a mask was in the way.

**The clipping fix can distort meshes. Never apply it to whole shapes, and always start with a low strength.** Fix only the area that clips, with everything else masked (masked vertices are left alone):

1. Find the area: `show_heatmap` `kind: "clipping"` with the `sliderValues` of the case, or `analyze_clipping` with a large `worstVertices`.
2. Mask everything but the area, with a soft border, in one call: `mask_ops` `op: "from_analysis"`, `metric: "clipping"`, the case's `sliderValues` (or `preset`), `grow` 2–3, `feather` 3–5. Only the clipping vertices and their surroundings stay unmasked; the feather only spreads outwards, so partly masked vertices around the area take part of the fix and it blends in instead of leaving a step. Check it with `render_view`, `display.mask` and `focus` `{shape, unmasked: true}`.
   - By hand: `set_mask` with the vertices, `mask_ops` `grow`, `smooth`, then `invert`.
3. Fix: `fix_clipping` with `strength` 0.1 (the default), in `sliders` mode with only the slider(s) of the case, or in `shapes` mode for clipping that's already there at sliders 0.
4. Check: `analyze_clipping` or `quality_sweep` for that slider (new clipping, flipped triangles, roughness), plus a `render_view`. If clipping remains, repeat with a slightly higher strength, about 0.05–0.1 more each time. Stop as soon as flipped triangles or roughness go up noticeably.
5. Clean up: `mask_ops` `clear`.

**`auto_fix_clipping` runs this loop for you** on every slider with at least `minNew` (5) new clipping vertices, or on `sliders`, `combinations` and `presets`. The mask is put back afterwards. Read `before` and `after` per case, then sweep and render.

- **Cases:** each slider at 1, and sliders that go both ways also at -1 (`negativeValues`). With `presets`, each slider on its own at the extreme values the presets set first (`presetExtremes`: BreastsZero -1.53, BreastUnderCurve 2, …), then the presets. A slider at a value other than 1 is fixed in combination mode.
- **`rest: true`** first fixes the clipping at sliders 0 in the base geometry, and measures everything else from the fixed base. Use it when the conversion added clipping at rest; compare with the source first, since some of it is by design (a hem tucked under the breasts: exclude it with `deep_at_rest` and a lower `minDepth`). A long dress: 88→0 at rest in one step. A bodysuit against the whole body: 340→14.
- **Steps:** the strength goes from 0.1 to 0.3. A step is taken back when it adds folds (more than 5, 15% of the start, or 1 per 100 vertices the fix has moved: `maxFlippedIncrease`, `maxFlippedRatio`, `maxFlippedPerMoved`), roughness (10% or 0.005: `maxRoughnessIncrease`, `minRoughnessAllowance`), deeper clipping (0.05), more clipping vertices, or nothing.
- **Tight sleeves pinch in the armpit.** On a bodysuit's separate sleeves, Amazon's arm muscles poked out on the outside (clearly visible) and the fix was stopped by folds, which were all in the armpit crease where nobody sees them. `maxFlippedPerMoved: 0.06` fixed it (359→145 at Amazon) and looked clean. Find where folds are with `analyze_slider` `withFlipped` and `get_shape_geometry` positions before loosening the limit.
- **Combination mode** divides each slider's share by its value in the combination. A slider set low (0.1 in a preset) would need its morph changed ten times over and showed as a spike when set high on its own, so sliders below 0.5 are treated as 0.5: the combination comes out partly fixed, and the next step does more.

- **Clipping that only a combination has** (a body preset, several big sliders together) can't be fixed slider by slider, since no slider clips on its own. `fix_clipping` `mode: "combination"` with the combination's `sliderValues` or a `preset` shares the push among its sliders by how far each moves the vertex. `auto_fix_clipping` with `presets` does this.
- Measured on UBE conversions with the UBE "for Converting Outfits" preset (high): a long dress 93→1 new clipping vertices; a halter crop top 313→142, max depth 0.77→0.46 (the rest is deep in the cleavage where the cups pinch together, and a stronger fix made it worse); another UBE preset on the dress 26→0, which removed the spots on the butt. Sweep the single sliders afterwards.
- A fix that takes the count down can still push the deepest vertices further in. Watch `maxDepth` as well as the count.

### Parts that are inside by design

End caps of a sleeveless bodysuit sit inside the arm, under the separate sleeves. Cuffs end past the body's open wrists and ankles, where inside and outside can't be told apart. Both count as clipping in every case and can dominate the sweep: on a bodysuit, SquarePeg3d's "142 new" were nearly all shoulder caps. A clipping fix there does nothing or pushes them through the arm.

`analysis_exclusions` leaves such vertices out of `analyze_clipping`, `quality_sweep`, the clipping heatmap and metrics, `validate_project` and `auto_fix_clipping`. `mask_ops` `from_analysis` keeps them masked, and `fix_clipping` leaves them alone.

- `op: "deep_at_rest"`: deeper than `minDepth` (1) inside the body with sliders at 0, grown by 2 rings. Real clipping rarely goes that deep.
- `op: "reference_edges"`: where the body has an open edge (neck, wrists, ankles), within `edgeDistance` (1.5).
- `from_mask`, `vertices`, `show` (puts them into the mask to look at), `clear`. They're added up unless `replace: true`, and dropped when another project is opened.
- On a bodysuit, `deep_at_rest` plus `reference_edges` took the suit from 310 vertices inside at rest (max depth 3.82) to 5 (0.25), and the sleeves from 342 to 33. The sweep counts what was left were real clipping at the underbust and gluteal fold.
- Run them right after loading the target reference, before measuring anything. Results report `excluded` counts.

What fixing whole shapes costs, measured on an open-cleavage top converted to 3BA (all sliders, no mask):

| Step | BreastsNewSH new clipping | flipped triangles | roughness |
|---|---|---|---|
| after conform | 83 | 182 | 0.053 |
| `fix_clipping` sliders, strength 0.5, whole shape | 6 | 232 | 0.056 |
| + `smooth_slider_deltas` 3 × 0.5 | 54 | 92 | 0.035 |
| + `fix_clipping` sliders 0.3 on three sliders | 0 | 135 | 0.038 |

The clipping fix trades clipping for folds, and smoothing does the opposite. Masked, low-strength fixes keep that cost to the area that needs it.

- **Other local fixes:** with everything but the area masked, `smooth_slider_deltas` or `brush_stroke` (inflate or smooth) along the clipping vertices. In `set_slider_edit` mode, a stroke changes that slider's morph.
- **Mask by any measurement:** `from_analysis` also takes `metric` `roughness` (bumps of a slider), `flipped` (folds), `gap_change` (where the gap to the body changes), `seam_gap` (seams a slider opens) and `weight_deviation` (weights unlike the body's). `target: "protect"` masks the bad vertices instead. When nothing is found, the mask stays as it is. `show_heatmap` shows the same metrics.

### Rigid parts

Conform bends buckles, gems, buttons and armour plates with the body under them. That's the clearest sign of a careless conversion.

1. `list_mesh_parts` on the shape, and pick the rigid pieces by size, bounds and main bone.
2. `mask_ops` `op: "parts"` with those part numbers, then `invert`, so only the rigid parts are unmasked. For a rigid piece that's part of the cloth mesh, mask by hand or with `connected` instead.
3. `rigidify_slider`. Without `sliders` it takes all morph sliders with data on the shapes, and the result lists only the sliders that moved the parts. Each part gets the move and turn that best fits its morph; `allowScale` lets it also grow evenly, e.g. plates on a thigh that gets thicker. One undo step per slider; the window has it as Slider > Rigidify Parts.
4. A rigid part can't follow a valley or curve a slider makes, so it would sink in at the edges: `rigidify_slider` moves each part out as a whole where it would, keeping it `clearance` (0.02) outside the body with the slider at 1, or as far as it is at rest when that's less. The back buckle of a halter crop top sank 0.30 into the spine valley with SquarePeg3d before; now it stays outside. `clearance: -1` turns it off.
5. Rigidify the conversion slider too, before `set_base_shape`, so the parts keep their shape in the new body's rest pose. The 29 eyelets of a laced blouse stayed round on every breast slider this way.

### Fit and seams

- **`analyze_fit`** checks that each slider keeps the outfit's distance to the body where it's close (within `maxDistance`, default 2). `tightened` vertices hug or squash into the body, `loosened` ones drift away. A good conform keeps the gap, and `quality_sweep` reports the same as `gap`. Fix with `from_analysis` `gap_change`, then smoothing or a brush on that slider.
- **`analyze_seams`** finds vertices at the same place (split UV seams, outfit shapes meeting, the outfit meeting the body at the neck or wrists) and the sliders that pull them apart, plus seams whose sides are weighted differently. **`weld_seams`** gives each seam one morph (the body's where the outfit meets it), and with `weights: true` the same weights.
- **Layers:** `analyze_clipping`, `quality_sweep` and `show_heatmap` take `against` to measure one outfit shape against another, e.g. a cloak against the armour under it.

### Weights

- **`clean_weights`** drops tiny weights (`pruneBelow`), keeps the largest `maxInfluences` bones (4 for the game) and normalizes. Run it after hand edits.
- **`mirror_weights`** copies one side's weights to the other with left and right bones swapped, after fixing one side by hand.
- **Weights unlike the body:** `analyze_weights` with `compareToReference`, then `mask_ops` `from_analysis` `metric: "weight_deviation"` and `copy_bone_weights` on what stays unmasked.
- **Don't use `conform_sliders` with `fixClipping`**: it fixes whole shapes.
- **Check combinations users actually build:** pass `combinations` with several big sliders together, e.g. Breasts, BreastsNewSH, Butt and Hips at 1. Clipping adds up.

### Weights in poses

Weights only show their problems when the body moves. `analyze_poses` skins the shapes in each pose of `list_poses` (or `poses`) and measures the **edge strain**: 0 when an edge only moves or turns, 1 when it doubles or halves in length. Each pose lists `maxStrain`, `stretchedEdges` (above `threshold`, 0.5), the worst vertices, and the body's own strain as `reference`, since skin stretches over joints too.

- **A tear is a peak far above the body's.** The CBBE to UBE Vtaw dress after `copy_bone_weights`: "Legs Test (L)" peaked at 23.5 where the body has 1.5, and "test from SAM" at 40.7. The worst vertices were the centre hem of the front panel, one side weighted 0.99 to the right calf and the other 0.99 to the left.
- The original CBBE dress, weighted by hand, peaked at 7.6 in the same pose. Compare with the source project before judging.
- `show_heatmap` / `mask_ops from_analysis` `metric: "pose_stretch"` with a `pose` show where; `metric: "weight_jump"` (0 to 1) shows hard weight borders without a pose.

Fixing them:

1. **Hanging parts shouldn't follow the calves:** `reassign_weights` `from` the calf bones (`NPC L Calf [LClf]`, `NPC L RearCalf [LrClf]`) `to` the thigh (`NPC L Thigh [LThg]`), and the same for the right. Otherwise the hem folds with the knees and tears between the calves. `toParent: true` moves each bone onto its parent instead. Remove the emptied bones with `delete_bones` and `shapes` afterwards.
2. **Mask what sits tight on the body** (`mask_ops` `parts` with the bodice), so its weights stay the body's.
3. **`auto_fix_weights`** with the poses (default all). It finds where a pose stretches the shape more than the body, blends there only the bones whose weights jump across those edges (the two thighs), over a radius that grows per step (2, 4, … 12), and keeps a step only when the peaks over all poses go down without one pose rising more than `maxPoseIncrease` (25%). Then it applies the weights as one undo step.
   - The dress: "Legs Test" 23.5 → 1.4, "test from SAM" 40.7 → 3.3, "2FUW06" 9.8 → 3.2, every pose under the body's strain, and none torn in `render_view` with `variants` of poses.
   - `stretchedEdges` goes **up** (148 → 286): the tear is spread over the panel. That's the point; judge the peaks.
4. **By hand:** `smooth_weights` with a `radius` (NIF units) on the unmasked area, ideally with `bones` (only those are blended, each vertex keeps its total on them). Thighs only at radius 8 gave about the same as the auto fix.

Pitfalls:

- **A small radius makes it worse.** Half blending two bones that turn apart pulls the vertices together (the edges shrink, which counts as strain too): radius 1.5 raised the dress's peak from 40 to 47. Wider gradients share the turn over many rings.
- **Blending all bones pulls spine and pelvis weights down into a skirt,** and "Spine Bend" got 1.2 → 11. Blend only the bones that split the part (`bones`, or the auto fix's choice).
- `keepClose` (off by default) keeps vertices that close to the body. A long dress touches the legs at rest, and 0.3 kept 1856 of the skirt's 2087 vertices, so nothing could change.
- Scale poses ("Breast L (Scale)", "Breasts Up") barely stretch anything; the leg, bend and SAM/HKX poses tell.

### Body presets

- **Finding them:** `list_presets` lists the installed body presets (the `SliderPresets` folder).
- **In the sweep:** `quality_sweep` checks them with `presets` (names) or `presetFilter` (e.g. "CBBE"), at their low and high weight. Pass `sliders: []` to check only presets. An open-cleavage top converted to 3BA: 22 CBBE presets at both weights took 0.8 s.
- **Using one:** `analyze_clipping`, `fix_clipping` (combination mode), `show_heatmap`, `mask_ops` `from_analysis` and `render_view` (and its `variants`) take `preset` and `presetWeight` instead of `sliderValues`, e.g. `variants: [{}, {"preset": "UBE Necoco V1"}]`.
- **`apply_preset`** sets the sliders in the window like Load Preset, and returns the values other than 0 (`returnValues: false` returns only the count; the values are long). **The sliders stay set** and show in every later render: `zero_sliders` afterwards, or render with `zeroOthers: true`. `render_view` lists `otherSlidersSet` when sliders you didn't pass are on.
- **Fixing one:** `auto_fix_clipping` with `presets` (see "Quality loop"). The preset meant for the body (e.g. "UBE Necoco for Converting Outfits" for UBE) matters most.
- **Extreme presets:** some extreme presets never look perfect on a given outfit, depending on its shape, topology and mesh resolution. "CBBE Big & Strong" pushed 364 vertices of that top into the body where most presets added under 40.
  - Judge a preset by what it adds over the typical ones.
  - Fix what the common presets show.
  - Tell the user about the extreme ones rather than distorting the outfit for them.

## Outfits from another game

`load_project` and `import_file` can't bring a Fallout 4 project into a Skyrim SE window (the NIF format differs). **`import_from_game`** can: give it the other BodySlide folder's `.osp` (and `set`), or a NIF.

- Shapes come over in global space, with UVs, normals, vertex colors, the outfit's own slider data and the zaps (`sliders: "local"`), and the slider flags. Weights don't, since the skeletons differ; the shapes are skinned without weights.
- Import the source body too and make it the reference (`set_reference_shape`), so the next `load_reference` carries BodyZap over.
- `dataFolders` (the mod or game Data folder with `Materials/`) lets it read the BGSM and set up the Skyrim shader: textures, specular, two-sided, alpha, glow, rim and back lighting. `read_material` reads a BGSM or BGEM on its own.
- Skyrim keeps specular in the normal map's alpha. Build that map from the FO4 `_n` (BC5, rebuild blue) and the `_s` red channel. Set `SpecularPower` from the `_s` green (gloss) average: about 2^(gloss·10+1). The tool's guess from the smoothness alone is too high.

Fallout 4 CBBE to Skyrim, measured on a wetsuit with sleeves, belt and thigh holsters:

1. Load the conversion reference "Convert: FO4 CBBE To SSE CBBE". At 0 it matches the FO4 body (wetsuit 60 vs 26 clipping vertices, belt 585 vs 544).
2. Conform it. The slider lowers the arms from FO4's A-pose to Skyrim's, which pushes the forearms through tight sleeves (301 new). Fix it with `auto_fix_clipping` on that slider alone (301→20), and again with `analysis_exclusions op zaps []`, so it also holds with the zap off. Rigidify the slider on buckles, clips and holster pouches, then bake it with `set_base_shape`.
3. Load "CBBE Body", conform, and rigidify again. Copy weights from "CBBE Body Physics" (`mergeSliders: true`) when the source had breast and butt bones.
4. For UBE, use "11 3BA to UBE 2.0" from the CBBE SE result, as in "Bodies without a direct template".
   - Copy weights with `excludeBones` for the genital and anus bones (Clitoral1, VaginaB1, NPC L/R Pussy02, NPC L/R T/B Anus2). Otherwise a suit's crotch follows their physics.
   - A body without hands leaves the old hand bones with no weights; `delete_bones` removes them.
- Small rigid pieces need even weights or they tear in leg poses. Mask everything else (`mask_ops parts`), then `smooth_weights` with 400 iterations at strength 1: SAM pose 9.7→3.9.
- Pose strain peaks on edges 0.002–0.1 long are measurement noise. Check them in a render before acting.

## Game meshes from archives

- **`browse_archives`** lists the meshes of the game's loaded BSA/BA2 archives as one folder tree, like File > Browse Archives.
  - Without arguments it shows the archives that hold meshes and the top folders.
  - `folder` lists one folder (subfolders with mesh counts, and files).
  - `filter` (text in the path), `pattern` (wildcards, e.g. `*/clothes/monk/*_f_1.nif`) and `archive` (text in the archive name) search below `folder`.
- **`load_from_archive`** loads files by `files` (exact paths) or by the same selection arguments.
  - `mode: "import"` (the default) adds the shapes; `"replace"` replaces the outfit with the first file and imports the rest. The reference stays.
  - It refuses more than `maxFiles` (10) matches. Pick one weight (`_1` for the big weight, `_0` for the small one) rather than both.
  - The result lists the shapes each file added. A name that's already taken gets a suffix (`boots_2`).

## Transforming shapes

`transform_shapes` moves, rotates, scales and inflates shapes like the Move, Rotate, Scale and Inflate dialogs, as one undo step.

- Order: `scale`, then `rotate` (degrees about X, Y, Z), both about `origin` (`"zero"`, `"center"` of the unmasked vertices, or a point), then `move`, then `inflate` along the normals (negative deflates).
- Masked vertices stay, partly masked ones move partly. Mask with a smooth border to move only a part, e.g. to lift a hem or widen a sleeve.
- `mirrorMove: ["x"]` moves the left side the opposite way, so `move: [0.5, 0, 0]` widens both sides.
- In slider edit mode (`set_slider_edit`) the change goes into that slider's morph instead of the base shape.
- Like in the window, the base shape only changes with all sliders at 0 (`zero_sliders`), and a slider in edit mode has to be on. This goes for `transform_shapes`, `set_vertex_data` positions and the shaping brushes.
- Keep inflate small (0.1–0.5) for clipping: it pushes the whole surface out, not only the clipping part.
- `list_shapes` gives the bounds of each shape, and `get_shape_geometry` gives center and size, to place loaded meshes.

## Other editing tools

Most commands of the window have their own tool. `run_steps` is only needed for script features like variables.

- **Shapes:**
  - `rename_shape`, `duplicate_shape`, `delete_shapes`, `set_reference_shape`
  - `delete_vertices` and `separate_vertices` work on the mask, like the menu: mask what to keep, or what to split off.
  - `merge_shapes`, `refine_mesh`, `mirror_shapes`, `symmetrize_vertices`, `invert_uvs`, `recalc_normals`
  - `shape_transforms` (apply or reset), `set_base_shape`, `make_conversion_reference`, `remove_unused_nodes`
- **Vertices:**
  - `get_shape_geometry` reads positions, normals, UVs, colors and triangles.
  - `set_vertex_data` sets single vertices.
  - `edit_topology` collapses vertices, and flips or splits edges.
- **Shape properties:**
  - `get_shape_properties`
  - `set_shader_properties`, `set_texture_paths`, `set_geometry_properties`
  - `set_extra_data`, e.g. BODYTRI on the root
- **Partitions and segments:**
  - `get_partitions`
  - `set_partitions`: body slots, with masked triangles assigned to a slot
  - `set_segments` (Fallout 4)
  - `copy_partitions` from the reference
- **Bones:**
  - `list_skeleton_bones`, `add_bones`
  - `add_custom_bone`, `edit_bone`, `delete_bones`
  - `set_bone_transforms`
  - `transfer_weights`, by vertex index from the reference
  - `remove_skinning`, `fix_bad_bones`
  - `apply_pose_to_mesh`
- **Masks:**
  - `mask_ops` also masks weighted, bone-weighted, slider-affected and asymmetric vertices, and with `op: "box"` the vertices inside a box (`list_shapes` bounds).
  - Mask results count `fullyMasked`, `partlyMasked` and `unmasked`. After `feather`, nearly every vertex is partly masked, so `masked` alone says little.
  - `mask_file` saves and loads named masks.
- **Sliders:**
  - `new_slider` (empty, zap or combined), `clone_slider`, `negate_sliders`
  - `clear_slider_data`, `delete_sliders`, `set_slider_properties`
  - `import_slider_data`, `export_slider_data`
  - `save_preset`
- **Files:**
  - `export_file` with `shapes` exports selected shapes, and with `headTri` a head TRI file.
  - `import_file` also reads head TRI files and folders.
  - `physics_data` imports and exports cloth physics HKX.
- **View:** `set_view_state` takes `overlays` for bones, nodes and the floor.

Like automation, the shape, bone and slider structure tools aren't in the undo history. Take a `snapshot_save` before them. The vertex, topology, transform, mask and weight tools are undoable.

## Visual checks

- **`render_view`** returns images:
  - `views: ["front","left","right","back"]`, `frame` on the outfit shapes, `sliderValues` for the case you're checking
  - `show` with only the outfit, or `ghost` on the body, to see inner surfaces
  - `display.wireframe` for folds
  - `focus` zooms in on vertices: a list, or the masked or unmasked ones after `from_analysis`. Listed vertices show red on a grey shape (`highlight`), e.g. the `flippedVertices` of `analyze_slider`; single vertices show as small dots, so pass whole areas. **Pick the side the vertices face:** the result says `focusFacesAway` for views they're hidden from (an evenly grey image means the red is on the other side; armpit and underbust vertices face inwards or down).
  - The mask display hides vertex colors: `show_heatmap` and the highlight turn it off (`maskDisplayTurnedOff`).
  - `sliderValues` apply on top of the current values; `zeroOthers: true` shows only them
  - `display.textures: false` when textures are missing (the log warns "Texture file … not found"): missing ones render as a "No image" pattern that hides heatmaps and masks
  - `variants` renders each view for several slider values or poses, and `grid: true` puts them all into one labeled image. Use it for before and after: `[{"sliderValues": {"Breasts": 0}}, {"sliderValues": {"Breasts": 1}}, {"pose": "Arms Up"}]`.
- **`show_heatmap`:**
  - `kind: "clipping"` with `sliderValues` colors the depth inside the body red and outside blue
  - `"slider"` shows how far each vertex moves; `"weights"` shows a bone
  - The metrics `roughness`, `flipped`, `weight_deviation`, `gap_change` (blue tighter, red looser) and `seam_gap`
  - Turn it off with `kind: "off"`
- **`toggle_display`** flips display toggles of the window (`toggle: ["wireframe", "textures"]`) or sets them (`set: {"textures": false}`), and returns them all. Plain grey shapes without textures show clipping best; wireframe shows folds and how dense the mesh is where the body pokes through.
- **`set_preview_pose`** with a pose from `list_poses` shows clipping in motion, e.g. raised arms for tops and bent legs for skirts. `clear_preview_pose` ends it.
- **Weights:** `render_view` with `weightBone` colors a bone's weights, e.g. the breast bones after a weight copy.
- Renders cost tokens. Use 384–512 px for checks and only render what the numbers point at.

## Scoring against a finished conversion

When there's a released conversion of the same outfit, run `compare_to_project` with its slider set file (any folder) and set. Shapes are matched by name, or with `shapeMap`.

- For an open-cleavage top, a plain conform to "CBBE 3BA Body" matched 62 of 157 sliders of the released 3BA version exactly.
- A laced blouse converted to UBE through 3BA matched 76 of 197 sliders of a released UBE conversion. The differences were the sliders that were fixed or rigidified.
- Most of the rest are sliders the release doesn't have (`onlyHere`) or that the author edited by hand.
- Use it as a check that you're in the right ballpark, not as the target to optimise.

## Posing

- **Start from a pose:** `set_preview_pose` takes a pose name, a pose file, or `keepCurrent` to adjust step by step.
- **Pose by eye with `aim`:** `aim: {"NPC R UpperArm [RUar]": [x, y, z]}` points a bone (shoulder to elbow, hip to knee, …) in a global direction. Parents are applied first. Bone names come from `list_shapes` with `withBones`.
- **`worldRotation`** turns a bone about the global axes, in degrees.
- **`get_pose` with `withTransforms`** shows where the joints are and which way bones point.
- **Bodies without hands or feet:** the 3BA body has none, so add them with `add_project` ("CBBE Hands" and "CBBE Feet" in `CBBE.osp`) for poses to read in renders.
- **Camera:** `set_view_state` sets the camera (`offset` is minus the look-at point in units of NIF/10; `position[2]` is the distance; `rotation`; `fov`). A low, close camera with a wide `fov` gives an upward-looking shot.
- **Saving:** `save_pose` saves to the pose list, or to `.hkx` (a game pose), `.yaml` or `.json`.

## Pitfalls

- **Measure sliders one at a time.** A superposed morph hides which slider is bad. `quality_sweep` and `analyze_slider` already do this.
- **Raising the strength doesn't always help.** On a halter crop top, Big_SaggyBreasts went from 104 to 65 new clipping vertices at 0.1, but a second pass at 0.2 made it deeper (0.14→0.27) with more folds. Compare every step with the one before, or let `auto_fix_clipping` do it.
- **A tight bodysuit's clipping is at the creases.** After exclusions, a bodysuit clipped at the underbust and the gluteal fold. Fixes there add slivers quickly, so stop early.
- **Undo for masks:** mask edits only go into the undo history when mask history is on in the settings.
- **Pending changes:** `load_project` and `new_project` refuse while there are unsaved changes, unless `discardChanges: true`. Ask the user before discarding their work.
- **Timeouts:** a call that times out may still finish in OS. Check `get_status` before repeating it.
- **New tool arguments after an OS update:** the client may keep the old schema of a tool until it reconnects, and then sends unknown list arguments as text. List arguments also take a JSON list in a string or a single name.
- **Progress:** long calls (conform, weight copy) send MCP progress notifications when the client asks for them.
