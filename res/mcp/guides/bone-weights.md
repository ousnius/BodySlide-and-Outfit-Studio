# Bone weights
Copy and fix bone weights so an outfit moves well in poses and clips less: copying from the body, checking in poses, skirts and dresses, rigid parts, physics bones, and each game's main skeleton bones.

Bone weights decide how each vertex follows the skeleton. Where the outfit sits on the body, it should move like the body under it, or the body pokes through in motion. Where it hangs free (skirts, capes, sleeves that flare), copying the body's weights tears it apart. Weights only show their problems when the body moves, so always check in poses.

## Copying from the body

1. Load the body as reference (see `conform-sliders`) and mask what should keep its weights: hanging parts you'll weight separately, rigid pieces, parts weighted to physics bones of their own.
2. **`copy_bone_weights`**: each vertex blends the weights of the nearby body vertices.
   - It skips placeholder bones named like "Don't copy this" (curtain references weight the legs to one) and `excludeBones`.
   - `cleanup` (on) limits the copied vertices to the game's bones per vertex (4, Starfield 8) and normalizes them. Bones it added that end up without weights (the leg bones of a top) are removed again (`removeEmptyBones`).
   - The result lists `bonesAdded`, `skippedBones` and the weight problems left. One `undo` takes it all back, added bones included.
   - Vertices farther than `proximityRadius` (10) from the body get no weights: the hem of a long dress left 158 vertices `unweighted` (the result has a `hint`). `undo` and copy again with a larger radius, e.g. 25.
3. **Check** with `analyze_weights`: no `unweighted` vertices, none over `influenceLimit` bones; with `compareToReference` a low `deviation` from the body where the outfit is within `maxDistance` (2).
4. **Check in poses** (below).

`transfer_weights` copies by vertex index instead, for shapes made from the body mesh (same vertex count and order).

## Checking in poses

`analyze_poses` skins the shapes in each pose of `list_poses` (or `poses`) and measures **edge strain**: 0 when an edge only moves or turns, 1 when it doubles or halves in length. Each pose lists `maxStrain`, `stretchedEdges` (above `threshold`, 0.5), the worst vertices, and the body's own strain as `reference`, since skin stretches over joints too.

- **A tear is a peak far above the body's.** A dress after a weight copy peaked at 23.5 in a leg pose where the body had 1.5: the hem of the front panel, one side weighted 0.99 to the right calf and the other 0.99 to the left.
- Compare with the source outfit: the hand-weighted original of that dress peaked at 7.6 in the same pose.
- `show_heatmap` or `mask_ops` `from_analysis` with `metric: "pose_stretch"` and a `pose` show where; `metric: "weight_jump"` shows hard weight borders without a pose.
- Leg, bend, sitting and HKX game poses tell the most. Scale poses ("Breasts Up") barely stretch anything.
- Peaks on very short edges (0.002–0.1 long) are measurement noise. Look at a render before acting on them.
- `render_view` with `variants` of poses, and `weightBone` to color one bone's weights.

## Skirts, dresses and other hanging parts

Every vertex of a weight copy takes the nearest body weights, so a skirt panel between the legs is split down the middle between the two thighs (or calves), with a hard border. It tears in every leg pose.

1. **Don't copy from a curtain reference** whose legs are weighted to a placeholder bone: the panels would get no leg weights. Mask the panels and keep their original weights, or copy from the body and fix them.
2. **Hanging parts shouldn't follow the calves:** `reassign_weights` `from` the calf bones `to` the thigh of the same side (Skyrim: `NPC L Calf [LClf]` and `NPC L RearCalf [LrClf]` to `NPC L Thigh [LThg]`, the same for the right). Otherwise the hem folds with the knees. `toParent: true` moves each bone onto its parent instead. Remove emptied bones with `delete_bones` and `shapes` afterwards.
3. **Mask what sits tight on the body** (`mask_ops` `parts` with the bodice or waistband), so it keeps the body's weights.
4. **`auto_fix_weights`** with the poses (default all). It finds where a pose stretches the shape more than the body, blends there only the bones whose weights jump across those edges (the two thighs), over a radius that grows per step (2, 4, … 12), and keeps a step only when the peaks over all poses go down without one pose rising more than `maxPoseIncrease`. It applies the result as one undo step.
   - The dress: 23.5 → 1.4 and 40.7 → 3.3 in two leg poses, every pose under the body's strain, nothing torn in renders.
   - `stretchedEdges` goes **up** (148 → 286): the tear is spread over the panel. That's the point; judge the peaks.
5. **By hand:** `smooth_weights` with a `radius` (NIF units) on the unmasked area, with `bones` so only those are blended (each vertex keeps its total on them). Thighs only, at radius 8, did about as well as the auto fix.

Pitfalls:

- **A small radius makes it worse.** Half-blending two bones that turn apart pulls the vertices together, which also counts as strain: radius 1.5 raised a peak from 40 to 47. Wider gradients share the turn over many rings.
- **Blending all bones pulls spine and pelvis weights into a skirt,** and a spine bend pose went from 1.2 to 11. Blend only the bones that split the part.
- `keepClose` keeps vertices that close to the body unchanged. A long dress touches the legs at rest, so even 0.3 froze most of the skirt.
- **Skirt bones:** Skyrim skeletons like XPMSE have `SkirtFBone01`–`03`, `SkirtBBone01`–`03`, `SkirtLBone01`–`03` and `SkirtRBone01`–`03` (front, back, left, right). Outfits weighted to them only move if the user's skeleton has them and something drives them (animations or physics). Don't add them unless the user asks; keep them when the source had them.

## Rigid parts

Buckles, plates and holster pieces need even weights, or they bend and tear in poses.

- Mask everything else (`mask_ops` `parts`, then `invert`), then `smooth_weights` with many iterations at full strength (e.g. 400 at 1): one piece went from 9.7 to 3.9 in a SAM pose.
- Or give the whole piece one bone with `set_weights`.

## Cleaning and editing

- **`clean_weights`** drops tiny weights (`pruneBelow`, e.g. 0.01), keeps the largest `maxInfluences` bones and normalizes. Run it after hand edits.
- **`mirror_weights`** copies one side's weights to the other with left and right bones swapped, after fixing one side by hand.
- **Weights unlike the body:** `analyze_weights` with `compareToReference`, then `mask_ops` `from_analysis` `metric: "weight_deviation"` and `copy_bone_weights` on what stays unmasked.
- `get_weights` and `set_weights` read and write one bone's weights; `list_bones` lists a shape's bones; `brush_stroke` with `weight`, `unweight` or `smooth_weight` paints them.
- `weld_seams` with `weights: true` gives both sides of a seam the same weights, so it doesn't open in poses.
- Bone structure: `list_skeleton_bones`, `add_bones`, `add_custom_bone` (bones the skeleton doesn't have, e.g. for physics), `edit_bone`, `delete_bones`, `set_bone_transforms`, `remove_skinning`, `fix_bad_bones` (transforms that don't match the skeleton). These aren't in the undo history: take a `snapshot_save` first.
- `apply_pose_to_mesh` bakes a pose into the shapes.

## Physics

Physics mods move extra bones of the body (breasts, butt, belly, thighs) or cloth. An outfit only moves with them when it's weighted to the same bones.

- **Body physics bones** come with the body's weights: copying from a body that has them (e.g. CBBE 3BA, UBE) gives the outfit its breast, butt and belly bones. When the source had physics bones and the target reference doesn't, copy from the physics variant of the body if there is one (e.g. "CBBE Body Physics"), loaded with `mergeSliders: true`.
- **Check them** after a copy: `render_view` with `weightBone` on the breast bones, and poses that move them ("Breasts Up").
- **Genital and anus bones** of bodies like 3BA and UBE are driven by physics too. A suit's crotch shouldn't follow them: copy with `excludeBones` for them (e.g. `Clitoral1`, `VaginaB1`, `NPC L Pussy02`, `NPC R Pussy02`, `NPC LB Anus2`, `NPC RB Anus2`, `NPC LT Anus2`, `NPC RT Anus2`; check the body's bones with `list_bones`).
- **Rigid or stiff parts** (armour plates, belts) shouldn't jiggle: keep breast and butt physics bones off them with `reassign_weights` `toParent: true`.
- **Skyrim SMP cloth** (skirts, capes) uses bones or per-vertex physics set up in an XML that the NIF links to. Keep the bones and weights the XML names when converting; renaming or removing them breaks the physics.
- **Fallout 4 cloth** is Havok cloth data (HKX) in the NIF: `physics_data` lists, imports and exports it. It's made for its mesh, so a mesh with changed vertices may need its cloth data rebuilt in other tools; tell the user.
- No tool runs the window's physics preview. Ask the user to check physics in the window or in game.

## Main skeleton bones

`list_skeleton_bones` lists the bones of the game's skeleton, `list_bones` the bones of a shape. Bodies and outfits usually use these, plus the body's physics bones.

**Skyrim (LE and SE).** At most 4 bones per vertex.
- Spine: `NPC Pelvis [Pelv]`, `NPC Spine [Spn0]`, `NPC Spine1 [Spn1]`, `NPC Spine2 [Spn2]`, `NPC Neck [Neck]`, `NPC Head [Head]`.
- Arms: `NPC L Clavicle [LClv]`, `NPC L UpperArm [LUar]`, `NPC L UpperarmTwist1 [LUt1]`, `NPC L UpperarmTwist2 [LUt2]`, `NPC L Forearm [LLar]`, `NPC L ForearmTwist1 [LLt1]`, `NPC L ForearmTwist2 [LLt2]`, `NPC L Hand [LHnd]`, fingers `NPC L Finger00 [LF00]` to `NPC L Finger42 [LF42]`.
- Legs: `NPC L Thigh [LThg]`, `NPC L Calf [LClf]`, `NPC L Foot [Lft ]`, `NPC L Toe0 [LToe]`.
- The same with `R` for the right side.
- Body physics bones (3BA, UBE and others): `NPC L Breast`, `NPC L PreBreast`, `L Breast01`–`L Breast03`, `NPC L Butt`, `NPC Belly`, `NPC L FrontThigh`, `NPC L RearThigh`, `NPC L RearCalf [LrClf]`, and the genital and anus bones.

**Fallout 4.** At most 4 bones per vertex. Bodies and outfits are mostly weighted to the `_skin` helper bones, not the animated bones themselves.
- Spine: `Pelvis`, `SPINE1`, `SPINE2`, `Chest`, `Neck`, `HEAD`; skin bones `Pelvis_skin`, `Pelvis_Rear_skin`, `Spine1_skin`, `Spine2_skin`, `Chest_skin`, `Chest_Upper_skin`, `Chest_Rear_Skin`, `Belly_skin`, `UpperBelly_skin`, `Neck_skin`, `Neck1_skin`.
- Arms: `LArm_Collarbone`, `LArm_UpperArm`, `LArm_UpperTwist1`, `LArm_UpperTwist2`, `LArm_ForeArm1`–`3`, `LArm_Hand`, fingers `LArm_Finger11`–`53`; skin bones `LArm_Collarbone_skin`, `LArm_UpperArm_skin`, `LArm_UpperTwist1_skin`, `LArm_ShoulderFat_skin`, `LArm_UpperFat_skin`, `LArm_ForeArm1_skin`–`3_skin`.
- Legs: `LLeg_Thigh`, `LLeg_Calf`, `LLeg_Foot`, `LLeg_Toe1`; skin bones `LLeg_Thigh_skin`, `LLeg_Thigh_Low_skin`, `LLeg_Thigh_Fat_skin`, `LLeg_Calf_skin`, `LLeg_Calf_Low_skin`, `LButtFat_skin`, `LBreast_skin`.
- The same with `R` for the right side. Body physics bones depend on the body and its physics mod: check the body's `list_bones`.

**Starfield.** At most 8 bones per vertex.
- Spine: `C_Hips`, `C_Spine`, `C_Spine1`, `C_Spine2`, `C_Chest`, `C_Neck`, `C_Neck1`, `C_Head`, plus `C_Waist` and `C_Belly`.
- Arms: `L_Clavicle`, `L_Arm`, `L_Biceps`, `L_Biceps_Twist`, `L_Forearm`, `L_Wrist`, `L_Wrist_Twist`, fingers `L_Thumb`, `L_Index`, `L_Middle`, `L_Ring`, `L_Pinky` (with `1` and `2`); helpers `L_Deltoid`, `L_Elbow`, `L_ArmMass`.
- Legs: `L_Thigh`, `L_Thigh_Twist`, `L_Knee`, `L_Calf`, `L_CalfMass`, `L_Foot`, `L_Toe`.
- Body helpers: `L_Pecs`, `L_Cup`, `L_Nipple`, `L_Butt`.
- The same with `R_` for the right side.

Don't copy weights between games: the skeletons differ. Weight an imported outfit from a body of the target game (see `cross-game`).
