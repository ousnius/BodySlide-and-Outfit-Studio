# Working with Outfit Studio
Ground rules, snapshots and undo, masks, looking at results, posing, bringing meshes in and transforming them, the editing tools, saving, common pitfalls and connecting a client. Read this first.

Outfit Studio (OS) shows everything the tools do in its window, and most edits can be undone there. The other guides build on this one:

- `body-conversion`: an outfit to another body of the same game, with reference templates.
- `conform-sliders`: giving an outfit a body's sliders, so it morphs with the body in BodySlide.
- `fix-clipping`: clipping at rest, with sliders and presets, and in poses.
- `bone-weights`: copying and fixing bone weights, skirts and dresses, physics bones, each game's skeleton.
- `cross-game`: an outfit from one game to another, e.g. Fallout 4 to Skyrim, with shaders, materials and textures.
- `posing`: posing the skeleton, matching a reference picture, natural stances, hands and fingers.

## Ground rules

- **Start with `get_status`, and call it again after any failure or timeout.** It lists the shapes, the reference, the sliders, whether the body is saved with the outfit (`copyReference`) and the name the next save uses (`displayName`).
- **A busy error is not a bug.** Tools refuse while the user has a dialog open, is painting or plays an animation. Ask the user, or try again later.
- **Never save over the source project** unless the user asks. `save_project` refuses to replace an existing set without `overwrite: true`. Save conversions under a new `displayName` and `sliderSetFile`. Test saves go to scratch names (e.g. `zz…`), which you delete afterwards.
- **Don't discard the user's work.** `load_project` and `new_project` refuse while there are unsaved changes unless `discardChanges: true`. Ask before passing it.
- **Units** are NIF units. The skeleton's global axes are +X the character's right, +Y the character's front and +Z up. Slider values run from 0 to 1, which is 0–100% in BodySlide.
- **Name the shapes.** Tools that take `shapes` use the selected outfit shapes when you give none, which may not be what you expect.
- **Measure before and after every edit**, and compare with the source before judging: many released outfits already clip or stretch a bit.

## Snapshots and undo

- `snapshot_save` before experiments, `snapshot_restore` to go back. Snapshots are dropped when another project is opened.
- Tools the window can't undo (conform, clipping fixes, automation steps, shape, bone and slider structure changes) take an automatic snapshot first and name it in `autoSnapshot` (`auto-1`, `auto-2`, …; the last 3 are kept).
- **`snapshot_diff`** shows what changed since a snapshot: moved vertices, changed sliders with their moved count and largest delta, reweighted bones, added or removed shapes. Use it after an edit to check that it changed what it should and nothing else.
- `undo` and `redo` take back one step per call. A step on a slider's morph enters edit mode for that slider, like in the window.
- **`snapshot_restore` brings the mask back too.** A leftover mask makes conform and weight copies silently skip the masked vertices: clear it (`mask_ops` `op: "clear"`) after a restore unless you want it.
- Mask edits only go into the undo history when mask history is on in the settings.

## Masks

Masked vertices are left alone by conform, weight copies, clipping fixes, smoothing, brushes and transforms. Partly masked vertices change partly. Use masks to limit every fix to the area that needs it.

- `mask_ops` has `grow`, `shrink`, `smooth`, `feather`, `invert`, `clear`, `connected`, `parts` (pieces of `list_mesh_parts`), `box`, and masks by weights, by bone, by what a slider moves (`slider_affected`) or by asymmetry.
- `mask_ops` `op: "from_analysis"` masks by a measurement in one call: `metric` `clipping`, `roughness`, `flipped`, `gap_change`, `seam_gap`, `weight_deviation`, `pose_stretch` or `weight_jump`. With `target: "fix"` (the default) only the bad vertices and their surroundings stay unmasked, with `"protect"` they are masked. `grow` and `feather` give it a soft border; the feather only spreads outwards. When nothing is found, the mask stays as it is.
- Mask results count `fullyMasked`, `partlyMasked` and `unmasked`. After a feather nearly every vertex is partly masked, so `masked` alone says little.
- `set_mask` sets vertices directly; `mask_file` saves and loads named masks.
- Check a mask with `render_view` `display.mask` and `focus` `{shape, unmasked: true}`.

## Looking at the result

Numbers find problems, images confirm them. Render what the numbers point at, not everything.

- **`render_view`**: `views` (`front`, `left`, `right`, `back`, …), `frame` on shapes, `sliderValues` or `preset` for the case, `pose`, `show` to pick shapes, `ghost` on the body to see inner surfaces, `display.wireframe` for folds.
  - `focus` zooms in on vertices (a list, or the masked or unmasked ones); listed vertices show red on a grey shape. **Pick the side they face:** the result says `focusFacesAway` for views they're hidden from. Armpit and underbust vertices face inwards or down.
  - `variants` renders several slider values or poses, and `grid: true` puts them into one labelled image, e.g. before and after: `[{"sliderValues": {"Breasts": 0}}, {"sliderValues": {"Breasts": 1}}, {"pose": "Arms Up"}]`.
  - `sliderValues` apply on top of the current values; `zeroOthers: true` shows only them. `otherSlidersSet` in the result warns about sliders you didn't pass that are on.
    An evenly grey image means the red is on the other side. Single vertices show as small dots, so pass whole areas, e.g. the `flippedVertices` of `analyze_slider`.
  - `display.textures: false` when textures are missing (the log warns "Texture file … not found"): missing ones render as a "No image" pattern that hides heatmaps and masks.
  - `weightBone` colors a bone's weights, e.g. the breast bones after a weight copy.
  - Renders cost tokens: 384–512 px is enough for checks.
- **`show_heatmap`**: `kind` `clipping` (red inside the body, blue outside), `slider` (how far each vertex moves), `weights` (one bone), or a metric of `from_analysis`. `kind: "off"` turns it off. The mask display hides vertex colors, so heatmaps turn it off (`maskDisplayTurnedOff`).
- **`toggle_display`** flips display toggles (`toggle: ["wireframe", "textures"]`) or sets them (`set: {"textures": false}`), and returns them all. Plain grey shapes show clipping best; wireframe shows folds and how dense the mesh is where the body pokes through.
- **`set_preview_pose`** shows clipping in motion, e.g. raised arms for tops and bent legs for skirts.
- `frame_view`, `set_view_state` and `get_view_state` control the camera. **`orbit`** (in `set_view_state` and `render_view`) places it in the character's terms: `{"target": "<bone or shape>", "yaw": 20, "pitch": -5, "distance": 150, "fov": 50}`. Yaw 0 looks at her front, 90 from her right, -90 from her left, 180 from behind; positive pitch looks from above. `get_view_state` returns the camera as an orbit too.

## Posing

Posing has its own guide, `posing`: the posing tools, checking a pose, matching a reference picture and what makes a pose look good. For a shot from below, use an orbit with negative pitch, a short distance and a wide `fov`.

## Bringing meshes in

- `list_slider_sets` (with `filter`) and `load_project` open sets of this BodySlide folder's `SliderSets`.
- `import_file` reads NIF, OBJ, FBX and head TRI files, or a folder.
- **Game archives:** `browse_archives` lists the meshes of the game's loaded BSA/BA2 archives as one folder tree, like File > Browse Archives. Without arguments it shows the archives that hold meshes and the top folders; `folder` lists one folder (subfolders with mesh counts, and files); `filter` (text in the path), `pattern` (wildcards, e.g. `*/clothes/monk/*_f_1.nif`) and `archive` (text in the archive name) search below `folder`.
- `load_from_archive` loads files by `files` (exact paths) or the same selection arguments. `mode: "import"` (the default) adds the shapes; `"replace"` replaces the outfit with the first file and imports the rest, keeping the reference. It refuses more than `maxFiles` (10) matches: pick one weight (`_1` the big one, `_0` the small one), not both. The result lists the shapes each file added; a name that's taken gets a suffix (`boots_2`).
- `import_from_game` brings shapes from another game's NIF or BodySlide set: see `cross-game`.
- `list_shapes` gives each shape's bounds and `get_shape_geometry` its center and size, to place loaded meshes.

## Transforming shapes

`transform_shapes` moves, rotates, scales and inflates shapes like the Move, Rotate, Scale and Inflate dialogs, as one undo step.

- Order: `scale`, then `rotate` (degrees about X, Y, Z), both about `origin` (`"zero"`, `"center"` of the unmasked vertices, or a point), then `move`, then `inflate` along the normals (negative deflates).
- Masked vertices stay and partly masked ones move partly: mask with a smooth border to move only a part, e.g. to lift a hem or widen a sleeve.
- `mirrorMove: ["x"]` moves the left side the opposite way, so `move: [0.5, 0, 0]` widens both sides.
- In slider edit mode (`set_slider_edit`) the change goes into that slider's morph instead of the base shape.
- Keep inflate small (0.1–0.5) for clipping: it pushes the whole unmasked surface out, not only the clipping part.

## Mesh detail

Outfits made for a smaller or lower-poly body often look faceted once they're converted or a slider makes them bigger: flat panels over the breasts and butt, corners along curves. `refine_mesh_adaptive` adds vertices only where they're needed, on the curve of the surface, so the shape gets rounder and smoother. The new vertices take over weights, slider data and UVs from their neighbors.

- `maxAngle` splits the edges whose vertex normals turn by more than that many degrees: where a curve looks faceted. Start at 20–25. Flat parts stay as they are, and creases sharper than `creaseAngle` (75) stay sharp.
- Measure where it shows: with `preset` or `sliderValues` it picks the edges that are faceted with those values, e.g. the largest common preset.
- Mask everything but the area, and look first with `preview: true`, which highlights the edges. Modelled cloth folds count as curves too: at 15 an unmasked dress would have gained 38% more vertices.
- `passes: 2` rounds further, since each pass halves the angles it splits. Compare with `render_view` before and after.
- `maxEdgeLength` splits long edges anywhere, `maxEdges` caps how many are split.
- Every vertex costs: weights, morphs and the limit of 65535 per shape. `refine_mesh` splits every unmasked triangle instead.

For clipping because the outfit is coarser than the body, see `fix-clipping`.

## Editing tools

Most window commands have their own tool. `run_steps` runs automation steps (`get_step_schema` describes them), for script features like variables.

- **Shapes:** `rename_shape`, `duplicate_shape`, `delete_shapes`, `set_reference_shape`, `delete_vertices` and `separate_vertices` (on the mask), `merge_shapes`, `refine_mesh` (every unmasked triangle), `refine_mesh_adaptive` (only where the mesh lacks detail, see `fix-clipping`), `mirror_shapes`, `symmetrize_vertices`, `invert_uvs`, `recalc_normals`, `shape_transforms`, `set_base_shape`, `remove_unused_nodes`.
- **Vertices:** `get_shape_geometry`, `set_vertex_data`, `edit_topology`, `brush_stroke`.
- **Properties:** `get_shape_properties`, `set_shader_properties`, `set_texture_paths`, `set_geometry_properties`, `set_extra_data` (e.g. BODYTRI on the root). Fallout 4 and 76 materials: `read_material`, `write_material` (see `cross-game`).
- **Partitions and segments:** `get_partitions`, `set_partitions` (Skyrim and older), `set_segments` (Fallout 4), `copy_partitions` from the reference.
- **Bones and weights:** see `bone-weights`.
- **Sliders:** see `conform-sliders`.
- **View:** `set_view_state` takes `overlays` for bones, nodes and the floor.
- The base shape only changes with all sliders at 0 (`zero_sliders`), and a slider's morph only in `set_slider_edit` mode with the slider on, like in the window. This goes for `transform_shapes`, `set_vertex_data` positions and the shaping brushes.
- The vertex, topology, transform, mask and weight tools are in the undo history. Like automation, the shape, bone and slider structure tools aren't: they take an automatic snapshot, or take a `snapshot_save` yourself.

## Many outfits at once

Converting a whole mod (dozens of pieces) one tool call at a time costs a lot. Do the first piece by hand to settle the steps, then batch the rest:

- **Write the steps as a script** in the BodySlide folder's `Automations` folder (`<AutomationScript version="1">` with one `<Step type="…" active="true">` per step, parameters as child elements named like `get_step_schema` lists them), e.g. `Automations/MyMod/Body.xml`. `run_automation_script` runs it by name (`MyMod/Body`); `list_automation_scripts` lists them.
- **Placeholders** are `{{name}}` with double braces, in any text parameter, filled in from `variables`: one script with `{{file}}`, `{{folder}}` and `{{name}}` serves every piece (`run_automation_script` with `variables`, or `run_steps` with `variables`).
- **One script can do many pieces:** `ClearProject`, then load, import, conform, weight, save, and the next piece's `ClearProject`. A script of twelve pieces runs in one call.
- `ImportFromGame` is a step too (see `cross-game`), so conversions from another game batch the same way.
- `ExportFile` and `SaveProject` create missing folders.
- `ChangePartitions` takes slot numbers (`"32"` → `"52"`), e.g. after `CopySegPart` gave everything the body's slot 32.
- Steps that work on shapes take `"TargetRegex": true, "TargetMeshes": [".*"]` for every shape without knowing their names (an empty list means the selected ones for some steps). `.*` includes the reference: `ClearSliderData` and mask steps change its data too. That doesn't matter after the conforms that need it, but renders of the body are wrong afterwards; render with `show` set to only the outfit.
- **Two references on one outfit** in steps: `ConformSliders` with `SliderNames` conforms only those sliders, so others keep their data. `CloneSlider` (`SourceSlider`, `SliderName`) keeps a conform's result under a new name before the next reference is loaded (`LoadReference` with `MergeSliders: true, AppendNewSliders: false` keeps the outfit's sliders). `ClearSliderData` with `UseMask` scales each vertex's data by its mask value, and `InvertMask` swaps the mask. Together they blend two conversions per vertex (see `conform-sliders`, "Blending two conversions").
- **Masks from a measurement** (`mask_ops` `from_analysis`) have no step: split the batch into `run_steps` before and after the `mask_ops` call. Shapes imported by steps aren't selected, so give `mask_ops` the shape names; they're the NIF's shape names, which a script can read from the file beforehand. Steps leave the outfit's masks as they are; `ClearMask` clears them.
- Try the script on one piece and look at it (`validate_project`, `analyze_clipping`, a render) before running the batch, then check every piece of it, not a sample (see `body-conversion`, "Many outfits at once").

## Saving

1. `validate_project` lists what breaks in game rather than in the view: unweighted vertices, too many bones per vertex, triangles without a partition, sliders that move the body under a shape but not the shape, zap sliders that zap nothing, and whether the body is saved with the outfit. Fix the errors and read the warnings.
2. `save_project` with a new `displayName` and `sliderSetFile`, and `shapeDataFolder`, `outputDataPath` and `outputFileName` as the user wants them. `copyReference` defaults to what the project had: keep the body when the source set had it. `genWeights` builds `_0` and `_1` meshes for Skyrim.
3. `export_file` writes NIF, OBJ, FBX, OSD or TRI files when they're asked for: `shapes` exports only those, `headTri` a head TRI file.

## Pitfalls

- **A call that times out may still finish.** Check `get_status` before repeating it.
- **Measure sliders one at a time.** A superposed morph hides which slider is bad. `quality_sweep` and `analyze_slider` already do this.
- **Read absolute numbers too.** A weak morph looks rough when normalized, a strong one smooth.
- **Sliders stay set.** `apply_preset` and `set_slider_values` change the window and every later measurement and render: `zero_sliders` afterwards.
- **Automation steps can replace shapes.** Look shapes up by name again after `run_steps`.
- **New tool arguments after an OS update:** the client may keep an old tool schema until it reconnects, and then sends unknown list arguments as text. List arguments also take a JSON list in a string or a single name.
- Long calls (conform, weight copy, sweeps) send progress notifications when the client asks for them.

## Connecting

For telling the user how to set up a client:

- Start OS with `OutfitStudio.exe --mcp` (or `--mcp-port N`), or use **File > MCP Server**. Settings has a checkbox that starts the server with OS, and the port (default 47823).
- Claude Code: `claude mcp add --transport http outfit-studio http://127.0.0.1:47823/mcp`. Other clients take an HTTP server at that URL, e.g. in `.mcp.json`: `{"mcpServers": {"outfit-studio": {"type": "http", "url": "http://127.0.0.1:47823/mcp"}}}`.
- Only programs on the same machine can connect. Set `MCP/Token` in `OutfitStudio.xml` to also require `Authorization: Bearer <token>`.
