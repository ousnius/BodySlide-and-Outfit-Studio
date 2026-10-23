# Working with Outfit Studio
Ground rules, snapshots and undo, masks, looking at results, posing, bringing meshes in and transforming them, the editing tools, saving, common pitfalls and connecting a client. Read this first.

Outfit Studio (OS) shows everything the tools do in its window, and most edits can be undone there. The other guides build on this one:

- `body-conversion`: an outfit to another body of the same game, with reference templates.
- `conform-sliders`: giving an outfit a body's sliders, so it morphs with the body in BodySlide.
- `fix-clipping`: clipping at rest, with sliders and presets, and in poses.
- `bone-weights`: copying and fixing bone weights, skirts and dresses, physics bones, each game's skeleton.
- `cross-game`: an outfit from one game to another, e.g. Fallout 4 to Skyrim, with shaders, materials and textures.

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
- `frame_view`, `set_view_state` and `get_view_state` control the camera. `set_view_state` `offset` is minus the look-at point in NIF units / 10, `position[2]` the distance.

## Posing

- `list_poses` lists the poses; `set_preview_pose` shows one (by name, a pose file, or `keepCurrent` to adjust step by step); `clear_preview_pose` ends it.
- `aim: {"<bone>": [x, y, z]}` points a bone (shoulder to elbow, hip to knee) in a global direction, parents first; bone names come from `list_shapes` with `withBones`. `worldRotation` turns a bone about the global axes in degrees. `get_pose` with `withTransforms` shows where the joints are and which way the bones point.
- `save_pose` saves to the pose list, or to `.hkx` (a game pose), `.yaml` or `.json`.
- Bodies without hands or feet (the CBBE 3BA body) look odd in poses: add "CBBE Hands" and "CBBE Feet" (in `CBBE.osp`) with `add_project` for renders.
- For a shot from below, set a low, close camera with a wide `fov` (`set_view_state`).

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

## Editing tools

Most window commands have their own tool. `run_steps` runs automation steps (`get_step_schema` describes them), for script features like variables.

- **Shapes:** `rename_shape`, `duplicate_shape`, `delete_shapes`, `set_reference_shape`, `delete_vertices` and `separate_vertices` (on the mask), `merge_shapes`, `refine_mesh`, `mirror_shapes`, `symmetrize_vertices`, `invert_uvs`, `recalc_normals`, `shape_transforms`, `set_base_shape`, `remove_unused_nodes`.
- **Vertices:** `get_shape_geometry`, `set_vertex_data`, `edit_topology`, `brush_stroke`.
- **Properties:** `get_shape_properties`, `set_shader_properties`, `set_texture_paths`, `set_geometry_properties`, `set_extra_data` (e.g. BODYTRI on the root). Fallout 4 and 76 materials: `read_material`, `write_material` (see `cross-game`).
- **Partitions and segments:** `get_partitions`, `set_partitions` (Skyrim and older), `set_segments` (Fallout 4), `copy_partitions` from the reference.
- **Bones and weights:** see `bone-weights`.
- **Sliders:** see `conform-sliders`.
- **View:** `set_view_state` takes `overlays` for bones, nodes and the floor.
- The base shape only changes with all sliders at 0 (`zero_sliders`), and a slider's morph only in `set_slider_edit` mode with the slider on, like in the window. This goes for `transform_shapes`, `set_vertex_data` positions and the shaping brushes.
- The vertex, topology, transform, mask and weight tools are in the undo history. Like automation, the shape, bone and slider structure tools aren't: they take an automatic snapshot, or take a `snapshot_save` yourself.

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
