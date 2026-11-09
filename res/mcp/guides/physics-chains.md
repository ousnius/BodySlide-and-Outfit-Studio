# Physics chains
Giving hair, ponytails, braids, ribbons and hanging jewelry SMP physics: the workflow, chains of custom bones made from the mesh or by hand, their weights, the physics XML and its settings, colliders, other files of the same hair (hairline, variants, wigs), checking it in a simulation, and the traps.

Skyrim's SMP (HDT Skinned Mesh Physics) moves custom bones that an XML linked from the NIF declares, and the mesh follows them through its weights. Hair needs: a chain of bones down every part that hangs, weights that blend smoothly along each chain, an XML with the bones, their constraints and the colliders, and the link on the NIF's root node.

## Workflow

1. Load the hair and the head collider it rests on (a head collision mesh of the hair mod, usually hidden in game). The collider is the attachment the chains start from and what the hair collides with.
2. `make_physics_chains` with `preview: true`: which parts hang, how many chains, how long. Nothing that hangs (a short cut, a bun): leave the hair static.
3. Rig: `make_physics_chains` for long hair, a second pass for the strands framing the face (below), hand-made chains for what it can't find. One chain per strand: two parallel chains on the same strand pull it apart.
4. The other shapes of the hair take the weights (below).
5. `write_physics_xml` from a template XML, then `set_extra_data` for the link.
6. `simulate_physics`, `validate_project` (no unweighted vertices, no bone the XML doesn't declare), renders of the chains (`render_view` `skeleton`, `weightBone`).
7. Export. Ask the user to judge it in game.

## Making the chains

`make_physics_chains` does what rigging by hand does:

1. **Roots.** The vertices on the attachment: give `rootShape`, a head collision mesh the hair sits on (vertices within `rootDistance` of it), or the tool uses a sphere around `anchorBone` (`rootCenter`, `rootRadius`). `rootMinHeight` keeps the neck and shoulders out. Hand-made hair rigs hand over from the head to the chains at about ear level; `rootMinHeight: 0` (roots only from the head bone's height up) does the same, so hair lying on the cheeks and neck already hangs on the chain.
2. **Hanging parts.** Every vertex gets its distance from the roots along the mesh. Separate hair cards closer than `linkDistance` count as joined, so layered cards are one strand. A part that hangs farther than `minHang` gets a chain; buns and volume on the scalp don't.
3. **Splitting.** A part that starts hanging around the head over a wider angle than `sectorAngle` is split (open hair: the back and the two sides). Groups that come together again at their tips (`minTipGap`, a thick ponytail) stay one chain. A part that runs around the head instead of down (`minDrop`) stays rigid: gravity would pull it off the head. `minJointDrop` ends a chain where the strand stops going down, and `lockGap` makes a chain follow one lock where a band of hair falls apart into separate locks.
4. **Bones.** Each chain has a root on the attachment, parented to `anchorBone` and not simulated, and moving joints every `boneSpacing` along the strand, at most `maxBones`. Names are `<prefix> <side> <n>`, side as seen from her: Front, FrontR, R, BackR, Back, BackL, L, FrontL.
5. **Weights.** Vertices keep their bones up to `hangStart`, then blend along the chain, each segment following the joint at its top; chains are blended where strands touch (`smoothRadius`), and the chains and the anchor where a chain meets hair that stays (`borderRadius`), so planes across the border don't stretch. What a chained vertex keeps of body bones other than the anchor (a hair's spine weights at the neck) moves to the anchor the further down the chain it is (`fadeBodyBones`): hair resting on the chest that follows the spine and a chain at once can't move off the body collider and jitters. One undo step; the bones are not in the undo history (an automatic snapshot is taken).

Run it with `preview: true` first: it reports the chains, their length and where they start, and `longestHang` and `lowestPoint` of the shape, without changing anything. Draw them with `render_view` `skeleton` (the chain bones) and `display: {"lighting": false}` to see `weightBone` colors on dark hair.

**Two passes** match hand-made rigs best:

- Long hair with the defaults (`boneSpacing` 8, `minHang` 12).
- Then the loose strands framing the face with a second `prefix`, `skipChained: true` (leaves out what the first pass took), short settings (`hangStart` 1.5, `minHang` 4, `boneSpacing` 3.5, `maxBones` 3, `rootDistance` 1, `maxChains` 2, `minTipGap` 0), `hangBelow` about 2 (only what hangs from ear level down moves, the volume on the head stays) and `sides` limited to the front and the sides. Hand-made short hairs usually have exactly two such chains.

`replace: true` makes the chains of a prefix again; the old bones' weights go back to the bone they hung on first. A mesh made of several shapes that hang together (a hair whose locks are a separate shape) is rigged in one run with `shapes`, so both get the same chains.

## Chains by hand

Some parts the detection can't find or splits wrong: a braid that starts on the crown and lies on the scalp before it hangs, many thin strands close together (bead curtains, tassels), jewelry hanging from a static pin. Rig those yourself:

1. **Find the parts.** `list_mesh_parts` lists the separate pieces with their bounds and centers; `mask_ops` with op `parts` and `get_mask` give a piece's vertices, `get_shape_geometry` their positions. A strand's centerline: the middle of its vertices in height slices from top to bottom.
2. **Bones.** `add_custom_bone` with `globalPosition` (a point on the centerline) and `parent` (the head for the first bone, then the bone above), evenly spaced along the strand from where it starts to move to near its tip. Custom bones stay in the skeleton across projects: when one exists already, `edit_bone` moves it and `add_bones` gives it to the project.
3. **Weights.** `set_weights` with `normalize: false`, and the shape's other bones set to 0 on those vertices. Make them **linear over the whole length**: each bone 1 at its own point on the strand and fading to 0 at the bones above and below, so every vertex blends its two nearest bones. Weights that are 1 down to a joint and then switch (or only blend in a short band below each joint) look like hinges. A rigid piece (a disc, a bead, a clasp) can follow one bone fully.
4. Parts that shouldn't move (the pin, a band on the head) stay on the head.
5. `write_physics_xml` with `chains` (`[head, bone 1, …, tip]` per strand) and a `profile` for each.

## The physics XML

`write_physics_xml` writes the XML with the settings of a template XML: its `<bone-default>` and `<generic-constraint-default>` blocks are profiles 0, 1, …; `profiles` picks one per chain root (e.g. stiffer settings for the short face strands), or each entry of `chains` has its own. It declares the chain roots and every other bone of the simulated shapes first, without mass, so they follow the skeleton; then the moving joints with their profile, each constrained to the joint above; then the shapes (`perVertexShapes`, also names of shapes in other files like an elf variant) and last the colliders (`perTriangleShapes`, `{name, like}` copies a collider block of the template).

- **Order matters.** SMP gives a shape every bone it is weighted to when it reads the shape; a collider written before the chain bones makes them all kinematic, and the chains don't move.
- Bones of colliders in other files go in `kinematicBones`.
- **Frames.** A template's limits are usually made for hair hanging down: around Z it twists, around X and Y it swings. `alignFrames: true` turns each joint's frame along its strand, so a ponytail sticking out backwards swings with the swing limits instead of sweeping sideways with the twist limits.
- **One XML per NIF.** The root links one file. A NIF with the hair and a moving accessory needs both in one XML: one system with all kinematic bones first, then each part's defaults with its bones, the constraints, the shapes, and every collider once at the end.
- Link it with `set_extra_data` on the root node: name `HDT Skinned Mesh Physics Object`, value the XML's path as the game finds it (`Data\meshes\…`). Setting or deleting it there replaces any link of the loaded files.
- **Settings only:** when only masses, damping or limits change, edit or write the XML again; the NIF stays as it is.

## Settings by kind

Keep a template with a profile per kind of part and pick per chain:

- **Long hair down the back:** free (about ±0.35 swing, ±0.1 twist, no spring stiffness), mass about 0.3, damping about 0.4 linear and 0.6 angular.
- **Short strands at the face, short hair at the back:** small swing limits and some spring stiffness, so they wiggle and come back.
- **Braids, thick locks:** heavier and more damped (mass about 0.5, damping 0.6 and 0.8) than loose hair, or they look light and float.
- **Jewelry, beads, tassels:** light (mass about 0.05) with wide limits. A bone that carries many such bodies (a disc the tassels hang from) needs a normal hair mass or more, or the light bone can't hold them and its joint stretches.
- Hair that rests on a body collider (strands over the chest) needs room to move off it: tight limits or a stiff spring pull it back into the collider, and it jitters, worst on larger bodies. Free limits work better there.

## Colliders

- The head collider stops hair going through the head. Hair that moves right on the scalp (braids from the crown) can pass a thin one (a small `penetration`) in one step: give the head collider block a larger `margin` and `penetration` (e.g. 0.2 and 1) in that hair's XML.
- A collider's settings come from the XML of the NIF it is in. When the head collider comes from another file with its own XML (a shared collision mesh of an armor), add the collider shape to the hair's NIF too, without a shader so it isn't drawn, to get the hair's settings.
- Hair modeled inside a body collider's margin (lying on the shoulders) starts inside it, gets pushed out and stretches; check with and without that collider.

## Other files of the same hair

- **Copies with the same vertices** (a hairline or alpha-test copy, a second LOD): `set_reference_shape` on the rigged shape, `transfer_weights`, `set_reference_shape` with `unset`.
- **Similar meshes** (a variant for another race or head): `copy_bone_weights` from the rigged shape. Check strands the copy misses (thin braids far from the reference): weight those along their own length on the matching chain's bones. Put every shape's name in `perVertexShapes`, also those in other files.
- **Accessories** that hang with one hair go into that hair's chains and XML; accessories shared by several hairs keep their own.
- **Wigs (Skyrim armor):** the armor addons of one armor that use the same body slot show only one mesh, so accessories of a wig go into the wig's mesh, and the mesh links one XML that has all its moving parts. Wig meshes don't use the head part's dynamic shape type. The inventory or ground mesh has no skinning and no physics link.

## Checking it

`simulate_physics` runs SMP without the window: settle, turn her back and forth, settle again. Load the colliders the XML names (the head, body collision meshes) so it collides like in game; `xml` uses a file that isn't installed yet.

- `exploded`, `invalid`: broken setup.
- `stretched`: a link far longer than at rest. The constraint gives way: the mesh starts inside a collider (test without it to confirm), or a light bone carries heavy parts (see Settings).
- `sags`: hangs far off its pose. Some sag is normal, hand-made rigs settle a few units lower too.
- `doesNotReturn`: settles somewhere else after the motion; caught on a collider or tangled. Long heavy chains often settle elsewhere in the simulation and are fine in game.
- `jitters`: keeps shaking at rest (`jitter`, the frame-to-frame change of its velocity). The simulation's contacts shake more than the game's, so compare against a rig that is known to be fine instead of trusting the flag alone. `swingAfter` is still swinging after the motion (little damping).

The tool only checks the mechanics. Ask the user to look at the hair in game.

## Traps

- Stretching in one part of a strand: look for two chains on it (two passes, or a strand split by the detection) and keep one, moving the other's weights to it.
- A loaded NIF's XML link is read from the game's Data folder. When an older XML is installed there, new chain bones it doesn't declare fall forever in the window's preview; check the installed file first.
- Bones a project doesn't have any more can still be in the skeleton from an earlier file (`list_skeleton_bones` shows `inProject`): add them with `add_bones` or move them with `edit_bone` instead of `add_custom_bone`.
- Vertex alpha on hair shaders: set it only on shapes that already have an alpha property. `set_alpha_property` with `vertexAlpha` creates one and turns on vertex colors, which makes an opaque part (thick locks) alpha-tested.
