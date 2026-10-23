# Conforming sliders
Give an outfit a body's sliders so it morphs with the body in BodySlide: loading the reference, conforming, rigid parts, hanging parts, zap sliders, editing morphs by hand, and checking each slider.

Conforming copies the body's slider morphs onto the outfit: each outfit vertex takes the morphs of the nearby body vertices. It's how an outfit without sliders gets them, and the core step of a body conversion (see `body-conversion`). It only works where the outfit has the body's shape at rest; if it doesn't, convert the shape first.

## Steps

1. **Load the body as reference.** `list_reference_templates`, then `load_reference` with `template` (or `file`, `set` and `shape`).
   - Default: the outfit's sliders are replaced by the reference's, zap sliders are kept (`mergeZaps`).
   - `mergeSliders: true` keeps the outfit's sliders and adds the reference's data to them, e.g. to conform another part from a second reference. `appendNewSliders: false` then only fills sliders the project already has.
   - `analyze_clipping` at sliders 0 should show about what the outfit had on its old body. If not, the reference is the wrong one or has another shape at rest.
2. **Mask what shouldn't take the body's morphs:** rigid parts (better rigidified after, see below), hanging panels that take a curtain reference, parts that are far from the body and should keep their own data.
3. **`conform_sliders`.** Options:
   - `sliders` limits it to some sliders; by default every slider but zap and UV sliders.
   - `proximityRadius` (default 10) is how far it searches for body vertices, `maxResults` (10) how many it blends. Parts far from the body (a long skirt's hem, a cape) take what's within that radius; raise it for them.
   - `noSqueeze` keeps vertices from moving inwards; `axisX`/`axisY`/`axisZ` limit the morph to some directions; `solidMode` moves connected vertices together.
   - `noReferenceData` in the result lists sliders the reference has no data for: they keep what the shape had. `clearMissing: true` clears them on the unmasked vertices instead.
   - Don't use `fixClipping` here: it fixes whole shapes (see `fix-clipping`).
4. **Rigid parts:** `rigidify_slider` (below).
5. **Check every slider** (below), then fix the bad ones.

## Rigid parts

A conform bends buckles, gems, buttons, eyelets and armour plates with the body under them, the clearest sign of a careless conversion.

1. `list_mesh_parts` lists the connected pieces of a shape with size, bounds and main bone. Pick the rigid ones.
2. `mask_ops` `op: "parts"` with those parts, then `invert`, so only they are unmasked. For a rigid piece that's part of the cloth mesh, mask by hand or with `connected`.
3. `rigidify_slider`. Without `sliders` it takes every morph slider with data on the shapes, and the result lists the sliders that moved the parts. Each part gets the move and turn that best fits its morph; `allowScale` lets it also grow evenly (plates on a thigh that gets thicker). `perPart: false` moves all unmasked vertices of a shape as one.
4. A rigid part can't follow a valley a slider makes, so its edges would sink in. `rigidify_slider` moves each part out as a whole where needed, keeping it `clearance` (0.02) outside the body, or as far as it was at rest when that's less. `clearance: -1` turns it off.
5. Rigidify a conversion slider too before baking it with `set_base_shape`, so the parts keep their shape on the new body.

## Hanging parts

Skirts, dress panels, loincloths and capes don't follow the legs. Conformed to the body, a panel between the legs splits and stretches with each thigh slider.

- Conform them to a curtain reference, whose surface spans the legs: mask everything else, load it with `mergeSliders: true`, conform with `clearMissing: true`, then load the body again with `mergeSliders: true, appendNewSliders: false`.
- `delete_sliders` with `notOnReference: true` removes helper sliders the curtain brought that the body doesn't have.
- Their weights need the same care: see `bone-weights`.

## Zap sliders

A zap slider removes vertices in BodySlide instead of moving them: the body under a bodysuit (`BodyZap`), a hood, a belt the user can turn off.

- `new_slider` `kind: "zap"` creates one that removes the unmasked vertices of the shapes. Mask everything but what it should remove first.
- `set_slider_properties`: `zapped` (on by default in BodySlide), `zapToggles` (other zaps it turns off), `hidden`, `invert`, `defaultSmall`, `defaultBig`, `newName`.
- Zaps that hide the body change what clipping counts: see `fix-clipping`.
- Check one with `mask_ops` `op: "slider_affected"` and `render_view` `display.mask`.

## Checking sliders

- **`quality_sweep`** measures every slider on its own (and `combinations`, `presets`), with all other sliders at 0, and lists the worst cases first:
  - `clipping.newlyPenetrating`: vertices the slider pushes into the body. See `fix-clipping`.
  - `morph.flippedTriangles`: triangles that turn over, i.e. folds and pinches. A few on a large morph is normal.
  - `morph.roughness` and `normalizedRoughness`: bumpiness of the morph. Read both; compare a slider's numbers before and after a fix, not against a fixed limit.
  - `missingMorph`: the body moves with this slider but the shape doesn't. Usually a conform without that slider, or a mask was in the way.
  - `gap`: vertices the slider squashes into the body or pulls away from it.
- **`analyze_slider`** looks at one slider; `withFlipped` returns the vertices of the folds to look at or mask.
- **`analyze_fit`** checks that each slider keeps the outfit's distance to the body where it's close (`maxDistance`, default 2): `tightened` hug or squash into the body, `loosened` drift away.
- **`analyze_seams`** finds vertices at the same place (split UV seams, shapes meeting, the outfit meeting the body at the neck or wrists) that sliders pull apart. **`weld_seams`** gives each seam one morph (the body's where the outfit meets it), with `weights: true` also the same weights.
- **`render_view`** with `variants` at 0 and 1 of the worst sliders, and `show_heatmap` `kind: "slider"` for how far each vertex moves.

## Editing morphs by hand

- `set_slider_edit` with a slider puts it in edit mode: `brush_stroke`, `transform_shapes` and `set_vertex_data` then change that slider's morph, not the base shape. The slider has to be on (`set_slider_values` 1). `set_slider_edit` without a slider ends it.
- `smooth_slider_deltas` evens out bumps a conform left (masked vertices keep theirs). Smoothing trades folds for clipping, so check both after.
- `scale_slider_deltas` makes a morph weaker or stronger, e.g. 0.8.
- `get_slider_deltas` and `set_slider_deltas` read and write morphs directly.
- `new_slider` (`empty`, `zap`, or `combined` from the current slider values), `clone_slider`, `negate_sliders`, `clear_slider_data` (`useMask` for the unmasked part only), `delete_sliders`.
- `import_slider_data` reads OSD and TRI files, or a NIF, OBJ, FBX or BSD file with the same vertex order as one slider (e.g. a morph sculpted elsewhere). `export_slider_data` writes them.
- Sliders that go both ways ("n|p" or "p|n" in the name) share one morph for both directions: fixing it at -1 can break +1, so sweep both after.
