# Posing
Posing the skeleton with set_preview_pose: the tools in the character's terms, checking a pose, matching a reference picture, what makes a standing pose look good, and arms, hands and fingers.

A preview pose only moves the skeleton and what is skinned to it; the shapes stay unposed in the project. Use it to look at an outfit in motion, to check clipping and weights (see `fix-clipping` and `bone-weights`), or to set up a picture.

## The tools

Global axes: +X is the character's right, +Y her front, +Z up. Positions and distances are in mesh units, like `list_shapes` bounds, also for Starfield, whose skeleton is in meters.

- `list_poses` lists the poses; `set_preview_pose` shows one (by name, a pose file, or `keepCurrent` to adjust step by step); `clear_preview_pose` ends it.
- **Know the skeleton first:** `list_skeleton_bones` with `mainOnly` and `withPoseInfo` lists the joints (pelvis, spine, neck, head, clavicles, arms, legs) with their role, natural range of motion, length, unposed direction and the twist bones of each limb. The games name them differently: Skyrim `NPC L Forearm [LLar]`, Fallout 4 `LArm_ForeArm1` (to `ForeArm3`, a chain), Starfield `L_Biceps`, `L_Forearm`, `L_Wrist`, Fallout 3 and Oblivion `Bip01 L Forearm`.
- **Change bones in the character's terms**, not with axis-angle guesses. Within one call, changes apply parents first: `bones`, `shift`, `worldRotation`, `aim`, `tilt`, `twist`, then `reach`, then `plant`.
  - `tilt: {"<bone>": {"right": 10, "front": -5, "up": 0}}` moves the bone's next joint towards her right, front or up by degrees (negative: left, back, down). Lean the chest with the spine bones, nod with the neck and head.
  - `aim: {"<bone>": [x, y, z]}` points a bone at a direction with the smallest turn. That keeps any twist the bone had, so after earlier turns give `{"direction": [...], "twist": 0}`.
  - `twist: {"<bone>": degrees}` sets a bone's twist about its own length and spreads it over the limb's twist bones, so the skin turns gradually: the hand over the forearm, the upper arm over its own twist bones. Twisting a limb bone alone wrings the skin.
  - **`reach`** solves a whole arm or leg: `{"NPC L Hand [LHnd]": {"target": {"bone": "NPC Head [Head]", "offset": [-6, -8, 4]}, "pole": [-1, 0, 0]}}` puts the hand behind the head with the elbow out to her left. Both bones are solved from scratch, so no roll from earlier attempts stays, and the elbow or knee bends the way it naturally does. The result says `reachable` and how far it `missed`; a miss means the limb is straight and pointing at the target. The hand keeps its turn relative to the forearm, so aim or twist it afterwards, in the same call or the next.
  - `shift: {"NPC COM [COM ]": [3, 0, -1]}` moves the body; with **`plant: ["<left foot>", "<right foot>"]`** the feet stay put and the legs bend to follow.
  - `bones: {"<bone>": {"rotation": [x, y, z]}}` sets a bone's rotation relative to its unposed one, as axis times degrees in the bone's own frame, replacing what it had. Use it for fingers (below) and to set a bone back to rest with `[0, 0, 0]`.
  - Prefer `reach` and `tilt` over `worldRotation` and `bones` for the body and limbs.
- `save_pose` saves to the pose list, or to `.hkx` (a game pose), `.yaml` or `.json`. Save each version the user likes before changing it further; `set_preview_pose` with `pose` brings it back.

## Checking a pose

- The result of `set_preview_pose` has `warnings` for limbs twisted too far, hinges bent backwards and joints folded past their range.
- `get_pose` `withTransforms` (`mainBones: true`, or `bones` for a few) gives each joint's position, direction, `twist`, `bend` and `bendsBackwards`. Compare a hand's direction with its forearm's to find a kinked wrist.
- `balance: true` says whether the body's mass is over the feet.
- `contacts: [{"from": "Hands", "to": "CBBE"}]` measures how close shapes come and whether they go into each other, e.g. a hand on the hip. It counts both hands of the shape, so look at renders of each hand too.
- **See the skeleton:** `render_view` `skeleton: true` draws the posed main bones over the images, red on her right side, blue on her left, labelled with short names. `skeleton: {"only": true}` draws a stick figure without shapes, and `bones` a list of bones, e.g. the finger bones of one hand.
- **Look from more than one side.** A pose that matches from the front can lean back, float a hand off the body or bend a wrist sideways in a view from the side. Render with `orbit` around the bone in question, e.g. `{"target": "NPC R Hand [RHnd]", "yaw": 90, "distance": 60}`.
- Bodies without hands or feet (the CBBE 3BA body) look odd in poses: add "CBBE Hands" and "CBBE Feet" (in `CBBE.osp`) or other hands and shoes with `add_project`. A head and hair from the game's archives (`load_from_archive`, e.g. a facegen NIF) complete the figure.

## Matching a reference picture

1. **Start from the closest game pose.** `set_preview_pose` with `file` reads a game `.hkx` pose; poser mods have hundreds. Adjusting a hand-made pose with `keepCurrent` gets closer, faster, than building from the unposed skeleton.
2. **Set the camera like the picture's:** `set_view_state` with an `orbit`. A slight yaw and pitch matter: a camera only 8° off the front looks like the front view to the user. A narrow `fov` (about 28) from further away flattens the perspective like a photo.
3. **Compare part by part**, with `skeleton: true` on the render: stance and weight leg, hips, torso lean from the side, head, then each arm, then each hand. Change one part at a time and render again.
4. When the user draws on a render, the arrows say where a joint or limb has to go. Ask when an arrow could mean two things.
5. **Hands last.** They need close-ups from the camera's side and from the side of the hand (below).

## Making a pose look good

These numbers come from 1,944 hand-made Skyrim poses (HalosPoser): 886 standing, the rest sitting, kneeling or lying. Bone names are Skyrim's, but the ideas hold for every game.

- **Contrapposto comes from COM, not the pelvis.** In Skyrim, `NPC Pelvis` and `NPC Spine` are both children of `NPC COM`.
  - Tilt COM sideways so the standing leg's hip is higher. 10° is typical, 5° is subtle, 15° is strong.
  - Turn the three spine bones back by about the same total, spread 5/5/10 from bottom to top, so the shoulders end level or tilted slightly the other way.
  - The pelvis bone itself is almost never turned. Tip it forward a few degrees with `plant` on the feet only to push the butt out.
  - Shift COM 4 to 7 towards the standing leg only for a strong hip-out, and drop it about 1 to 2, with `plant` on both feet.
- **Legs.** The standing knee stays at its rest bend. The free foot goes about 14 out to the side, level with or behind the standing foot, seldom in front. It crosses the midline in about 1 of 10 poses. Feet point by about 18°, up to 45°.
- **Torso and head.** Forward lean and chest arch mostly stay at rest. The attitude is in the neck and head: about 15° of nod, side tilts up to 14° and turns up to 50°. These are independent of the hips; the head often tilts away from the raised hip.

## Arms, hands and fingers

- **Hanging arm.** The elbow bends 5 to 25°. The hand continues the forearm's line, with about 14° at the wrist, and the fingers stay relaxed. A hand resting on the thigh lies flat on it, the back of the hand outwards.
- **Hand on the hip or waist.** The elbow bends 70 to 120° and points out to the side and a little back. Too far back and the arm hides behind the body from the front.
  - On the hip the hand sits about 10 above the pelvis; on the waist, higher, at its narrowest part.
  - Most poses bend the wrist about 30° and point the fingers down along the body or forward and down.
  - For a hand hooked behind the waist (fingers down and back over the hip), aim the hand down and back and roll it about 90° with `twist` on the hand, so the turn spreads over the forearm twist bones. The index and pinky knuckles then run front to back along the side of the body; side by side across the waist, the palm faces backwards.
  - A roll of +90 or -90 decides whether the palm or the back of the hand lies on the body. In the classic hip pose the back of the hand rests on the waist and the palm faces out: on Skyrim's left hand that was `twist: 90`. Straighten the fingers then (curl 0 or bent back a few degrees), or their curl lifts them off the body.
  - Point the hand along the body's surface below it (outwards over the flare of the hip), so the fingers rest on the skin instead of digging in. Keep the wrist a little off the surface.
- **Which way the palm faces.** A twist that looks right in the bones can turn the palm outwards. Check the thumb's direction with `get_pose`: hanging with the palm towards the body, the thumb points forward; with the palm backwards, it points towards the body's middle.
- **Elegant fingers.**
  - Give all three joints of a finger the same small curl, and cascade it from the index to the pinky, e.g. 5, 10, 20 and 25° with `bones` rotations on each joint.
  - In Skyrim the curl is about the joint's local X, positive on the right hand and negative on the left. Check it on one joint first in other games.
  - Straight fingers with the last joint bent back a few degrees look elegant on a hand resting on the body; strongly curled ones look like a fist.
  - Don't `aim` fingers at global directions: that rolls the knuckles and fans the fingers out.
  - To start over, set the finger bones back to `rotation: [0, 0, 0]`, the skeleton's relaxed hand. The thumb is rarely posed.
