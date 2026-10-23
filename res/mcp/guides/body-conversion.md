# Converting to another body
Convert an outfit to another body of the same game with reference templates, e.g. CBBE SE to CBBE 3BA or UBE: reading the source, choosing the reference, bodies without a direct template, keeping the body and its zaps, openings and hems, many outfits at once, what only the game shows, building, checking and scoring the result.

A conversion moves the outfit onto the new body's shape, gives it the new body's sliders and weights, and fixes what that breaks. The steps below link to the other guides for the details: `conform-sliders`, `bone-weights` and `fix-clipping`. For an outfit of another game, read `cross-game` first.

## Workflow

1. **Load the source.** `list_slider_sets` (with `filter`), then `load_project`. For a game mesh without a project, use `load_from_archive` or `import_file`.
   - **Note whether the set includes the body:** `get_status` shows `copyReference` true and lists the reference. Most outfit sets do. BodySlide builds the body with the outfit and hides it where the outfit covers it with zap sliders like `BodyZap` (`list_sliders` with `filter: "zap"`). The conversion has to keep the body and its zaps (see below).
2. **Measure the starting point** against the source's own body before converting: `analyze_clipping` with sliders at 0 and with a few big sliders, and `analyze_poses` for the weights. Many released outfits already clip a bit (an open-cleavage top had 211 vertices inside its body at rest), so judge only what the conversion adds.
3. **Load the target reference:** `list_reference_templates`, then `load_reference` with `template`. By default the outfit's sliders are replaced by the reference's, zap sliders are kept, and the zaps of the old body move onto the new one (`zapsTransferred`).
   - Pick the reference by the garment (next section).
   - Right after loading, run `analyze_clipping` at sliders 0. A big jump over step 2 means the wrong reference, or a body whose shape at rest differs from the source's (see "Bodies without a direct template").
   - Set up `analysis_exclusions` now, before measuring anything else (see `fix-clipping`).
4. **Find the rigid parts** with `list_mesh_parts`: buckles, buttons, plates, gems. They get `rigidify_slider` after the conform (see `conform-sliders`).
5. **`conform_sliders`**, then **`copy_bone_weights`** (see `conform-sliders` and `bone-weights`). Mask what should keep its own data first, e.g. hanging panels that take another reference.
6. **Check:**
   - `quality_sweep` for every slider, with `presets` and `combinations`, and `poses` to check in motion.
   - `analyze_weights` with `compareToReference`, and `analyze_poses` against the source's numbers from step 2.
   - `analyze_fit` and `analyze_seams`.
   - `render_view` with `variants` of the worst cases.
7. **Fix one problem at a time**, masked and at low strength (see `fix-clipping`), then check again.
8. **`validate_project`**, then **`save_project`** under a new name, keeping the body when the source had it.

## Reading the source

Most mistakes of a conversion come from reading the source wrong, not from the fit.

- **Convert the project that builds the mesh the game loads.** In a setup with many mods, several sets can build a mesh of the same name. Pick the one whose output path and file name (`get_status`) match the mesh that wins in the game's data, not just the newest or highest-priority set. The wrong one converts a different design: a pair of sandals came out as socks.
- **The body may be cut down.** Some sets cut the body under the armour, and first-person sets only have arms. Cut the new body to the same footprint after loading it (mask what the old body didn't cover, `delete_vertices`), and keep the zaps.
- **High heels.** Sets for high heels pose the body's feet for the heels. Pose the new body's feet to match before conforming (`apply_pose_to_mesh` on the feet, with the rest masked).
- **Shapes that belong to the body** (genitals, hand and foot stubs that come with it) go with the body: replace or remove them along with it instead of converting them as outfit shapes.
- **Head parts** (BSDynamicTriShape, `get_shape_properties` shows `geometry.dynamic`) don't follow the body: leave them as they are.

## Choosing the reference

- **The full body** (e.g. "CBBE 3BA Body", "UBE SE 2.0 Body") for anything that follows the body, including tops with an open cleavage.
- **UniBoob** references for garments that span between the breasts without following them in: a dress top or a tight shirt. On an open-cleavage top it's wrong: 897 vertices clipped instead of 217.
- **Curtain references** ("Uni…Curtain", "…Lower_Curtain") for skirts and dresses that hang free between the legs: their surface spans the legs, so the panels don't split between them.
- One outfit can need several references: mask by part (`list_mesh_parts`, `mask_ops` `op: "parts"`), conform one part per reference, and load the next with `mergeSliders: true` to keep what's already conformed.
- **Measure clipping against the body, not a curtain or UniBoob shell:** the outfit sits inside the shell by design. Load the body again with `mergeSliders: true, appendNewSliders: false` before measuring.

## Bodies without a direct template

When the target body has another shape at rest than the source body, the outfit has to take the new shape first. Use a conversion reference: a template whose slider morphs the source body into the target body ("X to Y" sets, e.g. "3BA to UBE 2.0"). It works from any body that matches the template's base at rest; CBBE SE and CBBE 3BA match at rest, so a 3BA conversion template works for CBBE SE outfits too.

1. `load_reference` with the conversion template. `analyze_clipping` at 0 should match the source (85 against 82 vertices on a crop top).
2. Rigid parts: mask them out (or rigidify the conversion slider after the conform, see `conform-sliders`).
3. `conform_sliders`, set the conversion slider to 1 (`set_slider_values`), then `set_base_shape`. The outfit now has the new body's shape at rest.
4. `load_reference` with the target body, then conform, weights and checks as usual. Clipping at rest should be close to the source.

CBBE SE to UBE: "11 3BA to UBE 2.0" (one slider, `3ba`), then "UBE SE 2.0 Body". The crop top had 88 clipping vertices at rest after it, against 82 on CBBE.

Hanging parts take the curtain variants of both templates: "11 3ba lower curtain to UBE 2.0" for the shape (its slider is `CBBE Dress to UBE`, not `3ba`) and "11 UBE SE 2.0 Uni_Lower_Curtain" for the sliders. Split the shape by part with masks (`list_mesh_parts`, `mask_ops` `op: "parts"` on the part that shouldn't change), and:

- `clear_slider_data` with `useMask: true` clears the body's conversion data from the unmasked part before the curtain conversion is conformed onto it.
- Set every conversion slider to 1 for `set_base_shape`. Their names can differ from the template's name (check `list_sliders`).
- Curtain references bring helper sliders that only move the curtain (`BiggerBody_ForOutfits`, `BiggerBreasts_ForOutfits`, penis and hand sliders). Delete them unless the whole outfit has them: after loading the body again with `mergeSliders: true, appendNewSliders: false`, `delete_sliders` with `notOnReference: true` deletes every slider the body has no data for.
- Conform the curtain's sliders with `clearMissing: true` on the hanging part, so it drops body sliders the curtain doesn't have (inner thighs, knees, glute folds). On a long dress the sweep showed no clipping from them. `validate_project` lists those as `missingMorphs`, which is on purpose there.
- Measure clipping against the body, not the curtain: a long dress had 860 "clipping" vertices at rest against the curtain shell it sits inside.
- **Don't copy weights from a curtain reference** when it weights the legs to a placeholder bone (named like "Don't copy this"); see `bone-weights`.

To make a conversion slider of your own, set the sliders of a loaded body to the target shape (or `apply_preset`) and use `make_conversion_reference`. It clears the undo history, so take a snapshot first, and check which way the new slider goes with `analyze_clipping` at 0 and 1 before baking anything with it.

## Keeping the body and its zaps

- Save with the body when the source had it (`copyReference`), or BodySlide builds the outfit without a body. Each of four outfit sets converted to UBE had it.
- Zap data lives on the body. `load_reference` moves it onto the new body by the closest surface point (`transferZaps`, on by default). For a conversion where it was lost, `transfer_zaps` takes it from the original project (`file`, `set`, and its body as `shape`). Missing zap sliders are created with the source's defaults and flags.
- Check a zap with `mask_ops` `op: "slider_affected"` on the body and `render_view` `display.mask`; the view doesn't hide zapped vertices. On a bodysuit, BodyZap removes 13550 of 13554 CBBE vertices and 29291 of 29298 UBE ones, BodyZapKeepArms 83% and 81%.
- `validate_project` warns about zap sliders without data on any shape (`emptyZaps`).
- Loading a set with the body and saving it again keeps its link to the reference template.

## Openings and hems

- **Keep the rings at the body's openings whole.** The neck, wrist and ankle openings of the new body can sit further out than the old body's. Zaps taken from the old body then cut into the ring at the opening and leave a see-through band where the outfit ends. Check each opening with `mask_ops` `op: "slider_affected"` and `render_view`. To unzap a ring, mask everything but the ring and `clear_slider_data` on the zap slider with `useMask: true`. In one batch of conversions this took exposed necks from 45 to 1.
- **Hems at an opening that moved.** When an opening of the new body sits lower or higher than the old one, hems that ended at it leave a gap. Move the hem band (`transform_shapes` with everything else masked, sliders at 0), positions only.

## Many outfits at once

- **Match one known-good conversion first** (`compare_to_project`), then scale up.
- **Take rules from what most released conversions agree on**, and ask the user where they disagree.
- **Check the whole batch, not a sample.** A fix that helps one outfit can break another. After each fix, run every outfit it can touch again, and check that the ones it shouldn't change come out the same. In one batch a sample of 40 found 1 case of a fault that the full scan found 6 times.
- **Outfits a check skips hide bugs.** "Nothing to judge" is a result to look at, not a pass.
- **Write down what was decided** and why, so the next session doesn't propose it again.

## What only the game shows

The tools find regressions; they don't judge quality. In one large batch of conversions, 8 of 9 kinds of fault were found in game, not by the checks: motion, physics, head turns, how cloth hangs. Give the user a short list of what to look at for each outfit: equip it, walk, run, crouch, switch presets, and look closely at the worst cases the checks found.

## Building and the game side

- **BodySlide can build without its window:** `BodySlide x64.exe -gbuild "<group>" -p "<preset>" -t "<folder>" -tri` (or `-b "<set>"` for one set). Always pass `-t`, or the meshes go wherever BodySlide's settings point. Under a mod manager, run it through the manager so it sees the mods' files.
- **BODYTRI:** BodySlide rewrites the BODYTRI string when it builds with tri morphs (`-tri`). Built without them, the mesh keeps the string of the source NIF, which can point the game at the old body's morphs: the outfit then follows the wrong sliders in game.
- Tell the user what the converted meshes need outside BodySlide:
  - **Slots and partitions.** A shape only shows when one of its partitions matches a slot of the armor addon. A new body with a partition the old one didn't have needs that slot on the armor addon, or there's a gap.
  - **Alternate textures** of armor records count shapes by position. Adding, removing or reordering shapes (e.g. replacing the body shape) shifts them.

## Body presets

- `list_presets` lists the installed presets (the `SliderPresets` folder). `quality_sweep` checks them with `presets` or `presetFilter` (e.g. "CBBE"), at both weights (`sliders: []` checks only presets). 22 CBBE presets at both weights took 0.8 s on a top.
- `analyze_clipping`, `fix_clipping`, `show_heatmap`, `mask_ops` `from_analysis` and `render_view` (and its `variants`) take `preset` and `presetWeight` instead of `sliderValues`, e.g. `variants: [{}, {"preset": "UBE Necoco V1"}]`.
- `apply_preset` sets the sliders in the window like Load Preset and returns the values other than 0 (`returnValues: false` returns only the count). They stay set and show in every later render: `zero_sliders` afterwards, or render with `zeroOthers: true`.
- The preset meant for converting to the body (e.g. "UBE Necoco for Converting Outfits" for UBE) matters most. Fix what the common presets show; `auto_fix_clipping` with `presets` does it (see `fix-clipping`).
- **Making a preset** (e.g. from a reference image): load the body as reference, turn textures off (`toggle_display`) and render `front` and `left` with `frame` on the body. Body-type sliders reshape the whole figure: compare a few at 1 side by side (`variants`, `grid`), start from the closest, then add the sliders of each area. Compare proportions rather than sizes, e.g. hip to waist width and where the underbust sits between shoulders and navel. Set the result with `set_slider_values`, then `save_preset` with the body's `groups`. It saves the visible sliders that aren't at their default, for the high weight only (`size="big"` in the XML): the low weight keeps the sliders' defaults unless `size="small"` entries are added to the file.
- Some extreme presets never look perfect on a given outfit, depending on its shape, topology and mesh resolution: "CBBE Big & Strong" pushed 364 vertices of a top into the body where most presets added under 40. Judge a preset by what it adds over the typical ones, and tell the user about the extreme ones rather than distorting the outfit for them.

## Scoring against a finished conversion

When a released conversion of the same outfit exists, `compare_to_project` with its slider set file and set matches shapes by name (or `shapeMap`) and compares each slider. A plain conform matched 62 of 157 sliders of a released 3BA top exactly, and a laced blouse converted to UBE through 3BA 76 of 197 of a released UBE conversion; the rest were sliders fixed, rigidified or edited by hand, or that only one side has (`onlyHere`). Use it to check you're in the right ballpark, not as the target.
