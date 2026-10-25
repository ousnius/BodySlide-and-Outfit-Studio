# Conforming sliders
Give an outfit a body's sliders so it morphs with the body in BodySlide: loading the reference, conforming, rigid parts, hanging parts, zap sliders, editing morphs by hand, body morphs that only one outfit has, and checking each slider.

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
3. `rigidify_slider`. Without `sliders` it takes every morph slider with data on the shapes, and the result lists the sliders that moved the parts. Each part gets the move and turn that best fits its morph; `allowScale` lets it also grow evenly (plates on a thigh that gets thicker). `perPart: false` moves all unmasked vertices of a shape as one. It's one undo step per slider; the window has it as Slider > Rigidify Parts.
4. A rigid part can't follow a valley a slider makes, so its edges would sink in. `rigidify_slider` moves each part out as a whole where needed, keeping it `clearance` (0.02) outside the body with the slider at 1, or as far as it was at rest when that's less. `clearance: -1` turns it off. The back buckle of a crop top sank 0.30 into the spine valley with one slider before this; now it stays outside.
5. Rigidify a conversion slider too before baking it with `set_base_shape`, so the parts keep their shape on the new body. The 29 eyelets of a laced blouse stayed round on every breast slider this way.
6. **Chains:** per-link rigidify keeps the links whole, but they only stay linked when neighbouring links had nearly the same morph before. Conform chains wide and smooth (`proximityRadius` 20, `maxResults` 1000, smoothed), then rigidify per part. Masking links instead leaves them behind, and the chain comes apart (see `cross-game`, "Chains and jewellery across the chest").
7. **A straight strap between parts that move apart** (a bikini's centre bar between the cups) can't be rigid: the cups spread with the sliders and a rigid strap comes off them. Mask only its end loops, then `smooth_slider_deltas` without `slider` (every slider with data on it) and `iterations: 100, strength: 1`. The ends keep the cups' moves, and the strap in between takes an even blend of them, so it stays straight.
8. **Small and long parts in one shape** (rings on a wire): `rigidify_slider` with `maxPartLength` (or `maxPartVertices`) leaves the long parts bending with the cloth, so only the small ones go rigid. `compare_shapes` against a copy from before the conform shows which parts bent (`bend`, worst first).

## Hanging parts

Skirts, dress panels, loincloths and capes don't follow the legs. Conformed to the body, a panel between the legs splits and stretches with each thigh slider.

- Conform them to a curtain reference, whose surface spans the legs: mask everything else, load it with `mergeSliders: true`, conform with `clearMissing: true`, then load the body again with `mergeSliders: true, appendNewSliders: false`.
- `delete_sliders` with `notOnReference: true` removes helper sliders the curtain brought that the body doesn't have.
- Their weights need the same care: see `bone-weights`.

## Zap sliders

A zap slider removes vertices in BodySlide instead of moving them: the body under a bodysuit (`BodyZap`), a hood, a belt the user can turn off.

- `new_slider` `kind: "zap"` creates one that removes the unmasked vertices of the shapes, the reference included. Mask everything but what it should remove first.
- `set_slider_properties`: `zapped` (on by default in BodySlide), `zapToggles` (other zaps it turns off), `hidden`, `invert`, `defaultSmall`, `defaultBig`, `newName`.
- Zaps that hide the body change what clipping counts: see `fix-clipping`.
- Check one with `mask_ops` `op: "slider_affected"` and `render_view` `display.mask`, or `render_view` `zaps: true` to see the outfit the way BodySlide builds it.

**A zap for the body under the outfit** (`BodyZap`, `FeetZap`, `HandsZap`), made from what the outfit covers:
1. `mask_ops` `op: "covered"` masks the body vertices the outfit covers along their normal: the outfit no further than `outside` in front of them (default 2) or `inside` behind (default 1, where the body pokes through). `by` picks the covering shapes, by default all outfit shapes.
2. `mask_ops` `invert`, then `grow` 1–2 steps: a zap removes every triangle with a zapped vertex, so the zapped area has to stay a ring inside the outfit's edge or a gap shows there.
3. `new_slider` `kind: "zap"` on the reference, `set_slider_properties` `defaultSmall`/`defaultBig` 100 and `hidden: true`, like the usual body zaps; `mask_ops` `clear`.
4. Check with `render_view` `zaps: [the zap]` and `show` the body: what's left is the body the outfit leaves visible.

Loose cloth stands further off the body: a Fallout 4 bodysuit left 1253 of 13554 CBBE vertices uncovered at the default 2 (its cups stand off the breasts), 122 at `outside: 6`. Seams the cloth bridges, like the cleft of the buttocks, stay uncovered unless `outside` reaches across. The `MaskCovered` automation step does step 1 in scripts (`CoverShapes`, `Outside`, `Inside`; target meshes default to the reference).

## Checking sliders

- **`quality_sweep`** measures every slider on its own (and `combinations`, `presets`), with all other sliders at 0, and lists the worst cases first:
  - `clipping.newlyPenetrating`: vertices the slider pushes into the body. See `fix-clipping`.
  - `morph.flippedTriangles`: triangles that turn over, i.e. folds and pinches. A few on a large morph is normal.
  - `morph.roughness` and `normalizedRoughness`: bumpiness of the morph. Read both; compare a slider's numbers before and after a fix, not against a fixed limit.
  - `missingMorph`: the body moves with this slider but the shape doesn't. Usually a conform without that slider, or a mask was in the way.
  - `gap`: vertices the slider squashes into the body or pulls away from it.
- **`analyze_slider`** looks at one slider; `withFlipped` returns the vertices of the folds to look at or mask.
- **`analyze_fit`** checks that each slider keeps the outfit's distance to the body where it's close (`maxDistance`, default 2): `tightened` hug or squash into the body, `loosened` drift away. A good conform keeps the gap. Fix it with `mask_ops` `from_analysis` `metric: "gap_change"`, then smoothing or a brush on that slider.
  - **A slider may not leave the outfit behind the skin** by more than the gap it has. Skin-tight shapes that move a little less than the body under them poke through at high slider values, even when every slider looks fine at 0.5.
- **Layers crossing at rest.** A conform works on each shape on its own, so an outer layer can cross the one under it after a conform. Released conversions show it too: fix it only where it shows (`analyze_clipping` `against` the inner layer, then `render_view`).
- **`analyze_seams`** finds vertices at the same place (split UV seams, shapes meeting, the outfit meeting the body at the neck or wrists) that sliders pull apart, and seams whose sides are weighted differently. **`weld_seams`** gives each seam one morph (the body's where the outfit meets it), with `weights: true` also the same weights.
- **`render_view`** with `variants` at 0 and 1 of the worst sliders, and `show_heatmap` `kind: "slider"` for how far each vertex moves.

## Editing morphs by hand

- `set_slider_edit` with a slider puts it in edit mode: `brush_stroke`, `transform_shapes` and `set_vertex_data` then change that slider's morph, not the base shape. The slider has to be on (`set_slider_values` 1). `set_slider_edit` without a slider ends it.
- `smooth_slider_deltas` evens out bumps a conform left (masked vertices keep theirs). Smoothing trades folds for clipping, so check both after.
- `scale_slider_deltas` makes a morph weaker or stronger, e.g. 0.8.
- `get_slider_deltas` and `set_slider_deltas` read and write morphs directly.
- `new_slider` (`empty`, `zap`, or `combined` from the current slider values), `clone_slider`, `negate_sliders`, `clear_slider_data`, `delete_sliders`.
- `clear_slider_data` with `useMask` (the default) multiplies each vertex's data by its mask value: unmasked vertices lose it, masked ones keep it, and partly masked ones (a smoothed or feathered border) keep that share. Without a mask it clears all of it.

### Blending two conversions

When one part of an outfit should follow one reference and another part a different one, conform both and blend them per vertex with a mask, instead of cutting the outfit at a hard border:

1. `load_reference` A, `conform_sliders`, `clone_slider` the conversion slider to e.g. `ConvA`.
2. `load_reference` B with `mergeSliders: true, appendNewSliders: false`, `conform_sliders` with only the conversion slider (`sliders`), so `ConvA` keeps A's data.
3. Mask the vertices that should take A (1) and those that should take B (0), with a soft border: `mask_ops` `from_analysis` with `feather` 3, or `smooth` steps. A shape where `from_analysis` finds nothing keeps its mask (`unchanged: true` in the result), so set every shape's mask to what such a shape should get first: e.g. `clear`, then `invert` for all masked. Otherwise a separate shape far from the body, such as a bridge between bikini cups, silently takes the wrong reference.
4. `clear_slider_data` `ConvA` with `useMask`, `mask_ops` `invert`, `clear_slider_data` the conversion slider with `useMask`. Each vertex now has m·A + (1−m)·B across the two sliders.
5. `mask_ops` `clear`, set both sliders to 1, `set_base_shape`.

Where both references agree (away from where they differ), the mask doesn't matter. To check that the blend itself is clean, set both sliders to 1, `new_slider` `combined`, and `analyze_slider` it: its roughness should match a single conversion's. The partial sliders on their own look rough and have flipped triangles, which is expected.

A conversion with large deltas can leave spikes where cloth spans a crease of the body (under the breasts, between the buttocks): the nearest body points of neighbouring cloth vertices move differently. `ConformSliders` with `SmoothResults: true` (12 iterations, 0.5 strength) or `smooth_slider_deltas` on the conversion slider before baking removes them, with little change to clipping. Render both sliders against only the second before baking to see what the blend changed.
- `import_slider_data` reads OSD and TRI files, or a NIF, OBJ, FBX or BSD file with the same vertex order as one slider (e.g. a morph sculpted elsewhere). `export_slider_data` writes them.
  - A BSD file holds only the moved vertices: a uint32 count, then per vertex a uint32 index and three floats, the delta in NIF units as `get_slider_deltas` shows it. With `fromFolder`, files named `Shape#Slider.bsd` fill that slider of that shape. That makes it the simplest way to bring in a morph computed by a script.
- Sliders that go both ways ("n|p" or "p|n" in the name) share one morph for both directions: fixing it at -1 can break +1, so sweep both after.

## Outfit-specific body morphs

When the body is saved with the outfit (`copyReference`), a slider can move the body too, and only under this outfit: tight fabric pressing into the skin, flesh bulging over an edge or a strap. Make it on the body, then let the outfit follow:

1. `new_slider` (empty). Set the slider values the morph is meant for first: a morph made on one body shape (a preset) won't line up with the outfit's edges on a much different one. Save it in that preset, or give it `defaultSmall`/`defaultBig` 100 when it should always be on.
2. Edit the body's morph. Tools that change geometry or morphs (`transform_shapes`, `scale_slider_deltas`, `clear_slider_data`) refuse the reference: `set_reference_shape` `unset: true`, edit, then `set_reference_shape` back. Changing the reference sets the slider values back to 0, so set them again before measuring or rendering.
3. `conform_sliders` with only that slider onto the outfit, so it sits on the moved body. Or compute the outfit's deltas together with the body's (below).
4. `analyze_clipping` with the slider and the values it's made for (`zeroOthers: false`), and `render_view` before and after.

**Masks give hard edges.** Box masks with a move or inflate leave creases at the mask border that `mask_ops` `smooth` doesn't remove, and outfit and body masks soften differently (different mesh density), so the outfit sinks in or floats. For a smooth result, compute the deltas instead:

- Read positions, normals and triangles with `get_shape_geometry` (in ranges of up to 20000 vertices). The body's weights (`get_weights` `all: true`) tell the limbs apart, e.g. to keep a bulge off the arms; bodies often weight the sides of the breasts to the upper arm too, so use the weight values, not `bone_weighted`.
- Mark the body vertices under the fabric (within a few tenths of a unit of an outfit triangle) and take the signed distance to the edge of that area, negative under the fabric.
- Move the body along its normals by a smooth profile of that distance: sinking under the fabric, gently from the edge inwards, and bulging just outside, peaking about a unit past the edge and fading out. Zero slope on both sides of the edge avoids a crease; the fabric edge then sits in a groove between the two. Smooth the result over neighbours within a radius.
- Give each outfit vertex the body's displacement under it (blend the nearest body vertices), then write both as BSD files and import them.
- Parts that bridge a gap over the body (a band across the cleavage, straps between knots) sit away from the surface. Check them after a conform: they can take uneven morphs and kink, or end up behind a bulge. `set_slider_deltas` with `add: true` moves them by hand.
