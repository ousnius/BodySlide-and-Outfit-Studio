# Fixing clipping
Find and fix where the body shows through an outfit: at rest in the base shape, with sliders, presets and combinations, and in poses. Measure, leave out what's inside by design, mask the area, fix gently, check.

**The clipping fix can distort meshes.** It pushes vertices out of the body and trades clipping for folds and bumps. Never apply it to whole shapes, and always start at a low strength. Fix only the area that clips, with everything else masked.

## Measuring

- **`analyze_clipping`** counts outfit vertices inside the body (`penetrating`, `maxDepth`) and close to it, with the current slider values and preview pose, or with `sliderValues` or a `preset` (other sliders at 0 unless `zeroOthers: false`). `worstVertices` returns the deepest vertices.
  - **The body poking through counts too.** Large outfit triangles over a curve of the body can let it show between their vertices while every vertex is outside (spots on the butt of a long dress with a UBE preset). Those triangles' vertices count by how far the body pokes out, and `pokeThrough` says how many body vertices show. The CBBE dress already had 21 at rest; an old UBE conversion of it had 87.
  - `against` measures one outfit shape against another, e.g. a cloak against the armour under it. `quality_sweep` and `show_heatmap` take it too.
- **`quality_sweep`** measures every slider on its own, plus `combinations`, `presets` and `poses`, and lists the worst cases. `clipping.newlyPenetrating` is what a case adds over sliders at 0: the clipping to fix. Sliders that go both ways ("n|p" in the name) are also checked at -1, and with presets each slider is also checked at the most extreme value outside 0 to 1 the presets set (`presetExtremes`: the UBE "for Converting Outfits" preset sets BreastsZero to -1.53 and BreastUnderCurve to 2).
- **`show_heatmap`** `kind: "clipping"` with the case's `sliderValues` or `preset` shows where: red is inside, blue outside.
- **Watch `maxDepth` as well as the count.** A fix that lowers the count can push the deepest vertices further in.
- Measure the source outfit on its own body first. Many released outfits clip a bit at rest by design (a hem tucked under the breasts); judge what's new.

## What counts

Set this up right after loading the reference, before measuring anything. Results report `excluded` and `hiddenByZaps` counts.

- **Zaps:** clipping is measured the way BodySlide builds the outfit. What the zaps BodySlide turns on by default remove (`BodyZap` under a bodysuit) can't show through, so it doesn't count: on a laced blouse BodyZap leaves 98 of 450 clipping vertices at rest, and under a bodysuit nothing counts. `analysis_exclusions` `op: "zaps"` picks other zaps: `zaps: []` measures the whole body (for users who turn the zap off), or a list for a variant. Without `zaps` it goes back to the defaults.
- **Parts inside by design** count as clipping in every case and can dominate a sweep: end caps of sleeves inside the arm, cuffs past the body's open wrists and ankles, a hem tucked under the breasts. A fix there does nothing or pushes them through the limb. `analysis_exclusions` leaves them out of every measurement and keeps them out of every fix:
  - `op: "deep_at_rest"`: deeper than `minDepth` (1) inside the body at rest, grown by 2 rings. Real clipping rarely goes that deep.
  - `op: "reference_edges"`: where the body has an open edge (neck, wrists, ankles), within `edgeDistance` (1.5).
  - `from_mask`, `vertices`, `show` (puts them into the mask to look at), `clear`. They add up unless `replace: true`, and are dropped when another project opens.
  - On a bodysuit, the two together took 310 vertices inside at rest (max depth 3.82) to 5 (0.25), and the sleeves from 342 to 33. What was left was real clipping at the underbust and the gluteal fold. Before them, one slider's "142 new" were nearly all shoulder caps.

## Fixing one case by hand

1. Find the area: `show_heatmap` or `analyze_clipping` with a large `worstVertices`.
2. Mask everything but the area, with a soft border, in one call: `mask_ops` `op: "from_analysis"`, `metric: "clipping"`, the case's `sliderValues` or `preset`, `grow` 2–3, `feather` 3–5. Partly masked vertices around the area take part of the fix, so it blends in instead of leaving a step. Check the mask with `render_view` `display.mask` and `focus` `{shape, unmasked: true}`. By hand: `set_mask` with the vertices, `mask_ops` `grow` and `smooth`, then `invert`.
3. `fix_clipping` with `strength` 0.1 (the default):
   - `mode: "shapes"` for clipping at rest, in the base shape. It measures with the current slider values, so `zero_sliders` first.
   - `mode: "sliders"` with only the case's slider(s), for clipping a slider adds. The fix goes into those sliders' morphs.
   - `mode: "combination"` with the combination's `sliderValues` or a `preset`, for clipping only a combination has. It shares the push among the sliders by how far each moves the vertex.
4. Check with `analyze_clipping` or `quality_sweep` for that case (new clipping, flipped triangles, roughness), and a `render_view`. If clipping remains, raise the strength by 0.05–0.1 and repeat. **Stop as soon as flipped triangles or roughness rise noticeably**: a second pass at 0.2 made one slider's clipping deeper (0.14→0.27) with more folds.
5. `mask_ops` `op: "clear"`.

## Fixing automatically

**`auto_fix_clipping`** runs that loop on every slider with at least `minNew` (5) new clipping vertices, or on given `sliders`, `combinations` and `presets`, and puts the mask back after. Read `before` and `after` per case, then sweep and render.

- **`rest: true`** first fixes the clipping at sliders 0 in the base shape, and measures everything else from the fixed base. Use it when the conversion added clipping at rest (a long dress: 88→0 in one step; a bodysuit against the whole body: 340→14). Compare with the source first, since some of it is by design: exclude a hem tucked under the breasts with `deep_at_rest` and a lower `minDepth`.
- **Presets:** each slider is first fixed on its own at the extreme values the presets set, then the presets in combination mode. With UBE's "for Converting Outfits" preset (high), a long dress went from 93 to 1 new clipping vertices, and another UBE preset from 26 to 0, which removed the spots on the butt. A halter crop top went from 313 to 142 (max depth 0.77→0.46): the rest was deep in the cleavage where the cups pinch together, and a stronger fix made it worse. Sweep the single sliders afterwards.
- **Steps:** the strength rises from `strength` (0.1) by `strengthStep` (0.05) to `maxStrength` (0.3). A step is taken back when it adds folds (more than 5, 15% of the start, or 1 per 100 vertices the fix moved: `maxFlippedIncrease`, `maxFlippedRatio`, `maxFlippedPerMoved`), roughness (10% or 0.005: `maxRoughnessIncrease`, `minRoughnessAllowance`), deeper clipping (0.05), more clipping vertices, or nothing.
- **Tight sleeves pinch in the armpit.** On a bodysuit's sleeves, the arm muscles of a strong preset poked out on the outside and the fix was stopped by folds that were all in the armpit crease where nobody sees them; `maxFlippedPerMoved: 0.06` took it from 359 to 145 and looked clean. Find where the folds are with `analyze_slider` `withFlipped` and `get_shape_geometry` positions before loosening a limit.
- **A tight bodysuit's clipping is at the creases** (underbust, gluteal fold). Fixes there add slivers quickly, so stop early.
- A slider set low in a combination would need its morph changed many times over, so sliders below 0.5 count as 0.5: the combination comes out partly fixed.
- Fixing an "n|p" slider at -1 can break its +1 case (one morph): sweep again afterwards.

## Other fixes

With everything but the area masked:

- `smooth_slider_deltas` on the case's slider evens out bumps; it does the opposite of the clipping fix (fewer folds, more clipping), so alternate them carefully.
- `brush_stroke` with `inflate` or `smooth` along the clipping vertices (`path`). In `set_slider_edit` mode it changes that slider's morph.
- `transform_shapes` `inflate` 0.1–0.5 pushes the unmasked surface out along its normals, for clipping at rest.
- `mask_ops` `from_analysis` also masks by `roughness`, `flipped`, `gap_change` and `seam_gap`, to smooth what a fix roughened.

What fixing whole shapes costs, measured on an open-cleavage top converted to 3BA (BreastsNewSH, no mask):

| Step | New clipping | Flipped triangles | Roughness |
|---|---|---|---|
| after conform | 83 | 182 | 0.053 |
| `fix_clipping` sliders, strength 0.5, whole shape | 6 | 232 | 0.056 |
| + `smooth_slider_deltas` 3 × 0.5 | 54 | 92 | 0.035 |
| + `fix_clipping` sliders 0.3 on three sliders | 0 | 135 | 0.038 |

The clipping fix trades clipping for folds, and smoothing does the opposite. Masked, low-strength fixes keep that cost to the area that needs it.

## Clipping in poses

The base shape and sliders don't move in poses; the bone weights do. Clipping that only shows in a pose is a weights problem, so don't push the shape out for it.

1. Find it: `quality_sweep` with `poses` (e.g. "Arms Up", "Bend Down", sitting poses from `list_poses`), or `set_preview_pose` and `analyze_clipping`, which measures the shown pose.
2. Look: `render_view` with `variants` of the poses and `show_heatmap` `kind: "clipping"` while the pose is set.
3. Fix the weights where the outfit sits on the body: they should follow the body's. With the pose set, `mask_ops` `from_analysis` `metric: "clipping"`, `clear_preview_pose`, then `copy_bone_weights` on the unmasked area. See `bone-weights` for skirts and loose parts, which shouldn't simply take the body's weights.
4. Check the pose again, and `analyze_poses` for tearing.

Some poses always clip a little where skin folds (deep knee bends, the armpit with arms down). Compare with how the body itself folds there before chasing it.

## Presets and combinations

- Clipping adds up: check combinations users build, e.g. `{"Breasts": 1, "BreastsNewSH": 1, "Butt": 1, "Hips": 1}`, and the common presets.
- Some extreme presets never look perfect on a given outfit. Judge a preset by what it adds over the typical ones, fix what the common presets show, and tell the user about the extreme ones.
