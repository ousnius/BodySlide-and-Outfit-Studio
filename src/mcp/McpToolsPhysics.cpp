/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/Anim.h"
#include "../components/MeshAnalysis.h"
#include "../components/PhysicsChains.h"
#include "../physics/Controller.h"
#include "../program/OutfitStudio.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_set>

using nlohmann::json;
using namespace nifly;

namespace {

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

Vector3 VectorArg(const json& args, const char* name, const Vector3& fallback) {
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return fallback;
	if (!value->is_array() || value->size() != 3 || !(*value)[0].is_number() || !(*value)[1].is_number() || !(*value)[2].is_number())
		McpArgs::Wrong(name, "an array of three numbers");
	return Vector3((*value)[0].get<float>(), (*value)[1].get<float>(), (*value)[2].get<float>());
}

std::vector<Triangle> Triangles(NiShape* shape) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	return tris;
}

// Custom bones whose name starts with prefix and a space
std::vector<std::string> PrefixBones(const std::string& prefix) {
	std::vector<std::string> names;
	AnimSkeleton::getInstance().GetBoneNames(names);
	std::vector<std::string> found;
	for (auto& name : names) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
		if (bone && !bone->isStandardBone && name.size() > prefix.size() && name.compare(0, prefix.size() + 1, prefix + " ") == 0)
			found.push_back(name);
	}
	return found;
}

PhysicsChains::Options ChainOptions(const json& args) {
	PhysicsChains::Options o;
	o.linkDistance = McpArgs::Float(args, "linkDistance", o.linkDistance);
	o.hangStart = McpArgs::Float(args, "hangStart", o.hangStart);
	o.minHang = McpArgs::Float(args, "minHang", o.minHang);
	o.boneSpacing = McpArgs::Float(args, "boneSpacing", o.boneSpacing);
	o.maxBones = std::clamp(McpArgs::Int(args, "maxBones", o.maxBones), 1, 20);
	o.maxChains = std::clamp(McpArgs::Int(args, "maxChains", o.maxChains), 1, 32);
	o.sectorAngle = McpArgs::Float(args, "sectorAngle", o.sectorAngle);
	o.minVertices = std::max(1, McpArgs::Int(args, "minVertices", o.minVertices));
	o.sides = McpArgs::Strings(args, "sides");
	o.minDrop = McpArgs::Float(args, "minDrop", o.minDrop);
	o.minTipGap = McpArgs::Float(args, "minTipGap", o.minTipGap);
	o.lockGap = McpArgs::Float(args, "lockGap", o.lockGap);
	o.minJointDrop = McpArgs::Float(args, "minJointDrop", o.minJointDrop);
	if (McpArgs::Has(args, "hangBelow"))
		o.hangCeiling = McpArgs::Float(args, "hangBelow", 0.0f);
	return o;
}

// Names of the bones of each chain: "<prefix> <side> <n>", with the side numbered when chains share it
std::vector<std::vector<std::string>> ChainBoneNames(const std::string& prefix, const std::vector<PhysicsChains::Chain>& chains) {
	std::map<std::string, int> sideCount;
	for (auto& c : chains)
		sideCount[c.side]++;
	std::map<std::string, int> sideIndex;
	std::vector<std::vector<std::string>> names;
	for (auto& c : chains) {
		std::string side = c.side;
		if (sideCount[c.side] > 1)
			side += std::to_string(++sideIndex[c.side]);
		std::vector<std::string> bones;
		for (size_t j = 0; j < c.joints.size(); j++)
			bones.push_back(prefix + " " + side + " " + std::to_string(j + 1));
		names.push_back(std::move(bones));
	}
	return names;
}

McpToolResult MakePhysicsChains(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	OutfitStudioFrame* frame = mcp.GetFrame();
	AnimInfo* anim = project.GetWorkAnim();
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();

	// The shapes rigged together: their vertices are one mesh for finding the chains (a hair and a separate part with
	// the rest of its strands), the weights go back to each
	std::vector<std::string> shapeNames = McpArgs::Strings(args, "shapes");
	if (shapeNames.empty())
		shapeNames.push_back(McpArgs::String(args, "shape"));
	for (auto& name : shapeNames)
		mcp.Shape(name);
	const bool preview = McpArgs::Bool(args, "preview", false);
	const std::string prefix = McpArgs::String(args, "prefix", "");
	if (!preview && prefix.empty())
		McpArgs::Wrong("prefix", "the start of the new bone names, e.g. the name of the hairstyle");

	const std::string anchorName = McpArgs::String(args, "anchorBone", "NPC Head [Head]");
	AnimBone* anchor = skeleton.GetBonePtr(anchorName);
	if (!anchor)
		throw McpToolError("The skeleton has no bone '" + anchorName + "'.");
	const float units = McpBoneUnits(mcp);
	const Vector3 anchorPos = anchor->xformToGlobal.translation * units;

	// Positions at rest: unposed, with all sliders at 0. offsets[i] is where shape i's vertices start.
	std::vector<Vector3> verts;
	std::vector<size_t> offsets;
	{
		McpSliderValueScope sliders(project);
		sliders.Zero();
		McpPoseScope pose(mcp);
		pose.Set("none");
		for (auto& name : shapeNames) {
			offsets.push_back(verts.size());
			auto shapeVerts = McpGlobalLiveVerts(mcp, mcp.Shape(name));
			verts.insert(verts.end(), shapeVerts.begin(), shapeVerts.end());
		}
	}
	offsets.push_back(verts.size());

	// The roots: where the mesh sits on the attachment surface, or within a sphere around the anchor
	std::vector<bool> roots(verts.size(), false);
	const float rootMinHeight = McpArgs::Float(args, "rootMinHeight", -6.0f);
	const std::string rootShapeName = McpArgs::String(args, "rootShape", "");
	if (!rootShapeName.empty()) {
		NiShape* rootShape = mcp.Shape(rootShapeName);
		std::vector<Vector3> surface;
		{
			McpSliderValueScope sliders(project);
			sliders.Zero();
			McpPoseScope pose(mcp);
			pose.Set("none");
			surface = McpGlobalLiveVerts(mcp, rootShape);
		}
		// Close to the surface, or lying over it above ear level: volume on the scalp rests on the head and doesn't
		// hang, however far its outer layers are from it
		const float rootDistance = McpArgs::Float(args, "rootDistance", 2.5f);
		const float supportDistance = std::max(rootDistance, McpArgs::Float(args, "rootSupportDistance", 0.0f));
		const float supportHeight = anchorPos.z + McpArgs::Float(args, "rootSupportHeight", 2.0f);
		auto closest = MeshAnalysis::ClosestSurfacePoints(surface, Triangles(rootShape), verts, supportDistance);
		for (size_t v = 0; v < verts.size(); v++) {
			if (closest[v].triangle < 0 || verts[v].z < anchorPos.z + rootMinHeight)
				continue;
			roots[v] = closest[v].distance <= rootDistance || verts[v].z >= supportHeight;
		}
	}
	else {
		const Vector3 center = anchorPos + VectorArg(args, "rootCenter", Vector3(0.0f, 1.0f, 5.0f));
		const float radius = McpArgs::Float(args, "rootRadius", 11.0f);
		for (size_t v = 0; v < verts.size(); v++)
			roots[v] = verts[v].DistanceTo(center) <= radius && verts[v].z >= anchorPos.z + rootMinHeight;
	}

	const PhysicsChains::Options options = ChainOptions(args);
	static const std::vector<std::string> sideNames = {"Front", "FrontR", "R", "BackR", "Back", "BackL", "L", "FrontL"};
	for (auto& side : options.sides)
		if (std::find(sideNames.begin(), sideNames.end(), side) == sideNames.end())
			McpArgs::Wrong("sides", "side names: Front, FrontR, R, BackR, Back, BackL, L, FrontL");

	// Vertices an earlier run gave to chains (mostly on custom bones) stay out of this one
	std::vector<bool> blocked;
	if (McpArgs::Bool(args, "skipChained", false)) {
		blocked.assign(verts.size(), false);
		std::vector<float> custom(verts.size(), 0.0f);
		for (size_t i = 0; i < shapeNames.size(); i++) {
			auto it = anim->shapeBones.find(shapeNames[i]);
			if (it == anim->shapeBones.end())
				continue;
			const size_t count = offsets[i + 1] - offsets[i];
			for (auto& b : it->second) {
				AnimBone* bone = skeleton.GetBonePtr(b);
				if (!bone || bone->isStandardBone)
					continue;
				if (auto w = anim->GetWeightsPtr(shapeNames[i], b))
					for (auto& vw : *w)
						if (vw.first < count)
							custom[offsets[i] + vw.first] += vw.second;
			}
		}
		for (size_t v = 0; v < verts.size(); v++)
			blocked[v] = custom[v] >= 0.5f;

		// A separate piece of the mesh (a lock, a hair card) an earlier run chained anywhere is left out whole: a
		// second chain on its upper part would pull the piece apart where the two meet. Large pieces (a hair made
		// of one mesh) are left out only where they are chained.
		for (size_t i = 0; i < shapeNames.size(); i++) {
			const size_t count = offsets[i + 1] - offsets[i];
			std::vector<int> parent(count);
			for (size_t v = 0; v < count; v++)
				parent[v] = int(v);
			auto find = [&](int a) {
				while (parent[a] != a)
					a = parent[a] = parent[parent[a]];
				return a;
			};
			for (auto& t : Triangles(mcp.Shape(shapeNames[i]))) {
				if (t.p1 >= count || t.p2 >= count || t.p3 >= count)
					continue;
				parent[find(t.p1)] = find(t.p2);
				parent[find(t.p2)] = find(t.p3);
			}
			std::unordered_map<int, size_t> pieceSize;
			std::unordered_set<int> chainedPieces;
			for (size_t v = 0; v < count; v++) {
				const int piece = find(int(v));
				pieceSize[piece]++;
				if (blocked[offsets[i] + v])
					chainedPieces.insert(piece);
			}
			for (size_t v = 0; v < count; v++) {
				const int piece = find(int(v));
				if (chainedPieces.count(piece) && pieceSize[piece] * 4 <= count)
					blocked[offsets[i] + v] = true;
			}
		}
	}

	// Close vertices of all shapes, then each shape's own edges (the shapes' triangles index their own vertices)
	auto neighbors = PhysicsChains::Neighbors(verts, {}, options.linkDistance);
	for (size_t i = 0; i < shapeNames.size(); i++) {
		auto link = [&](size_t a, size_t b) {
			a += offsets[i];
			b += offsets[i];
			if (a == b || a >= offsets[i + 1] || b >= offsets[i + 1])
				return;
			auto& na = neighbors[a];
			if (std::find(na.begin(), na.end(), int(b)) == na.end()) {
				na.push_back(int(b));
				neighbors[b].push_back(int(a));
			}
		};
		for (auto& t : Triangles(mcp.Shape(shapeNames[i]))) {
			link(t.p1, t.p2);
			link(t.p2, t.p3);
			link(t.p3, t.p1);
		}
	}
	PhysicsChains::Result detected = PhysicsChains::Detect(verts, neighbors, roots, anchorPos, options, blocked.empty() ? nullptr : &blocked);
	auto boneNames = ChainBoneNames(prefix.empty() ? "Chain" : prefix, detected.chains);

	McpToolResult result;
	json chains = json::array();
	for (size_t c = 0; c < detected.chains.size(); c++) {
		auto& chain = detected.chains[c];
		json joints = json::array();
		for (auto& j : chain.joints)
			joints.push_back(Vec(j));
		chains.push_back({
			{"side", chain.side},
			{"azimuth", std::round(chain.azimuth)},
			{"length", chain.length},
			{"vertices", chain.vertexCount},
			{"bones", boneNames[c]},
			{"joints", joints},
		});
	}
	result.data["chains"] = chains;
	result.data["rootVertices"] = detected.rootVertices;
	result.data["hangingVertices"] = detected.hangingVertices;
	result.data["unreachableVertices"] = detected.unreachableVertices;
	result.data["shortParts"] = detected.shortParts;
	result.data["mergedParts"] = detected.mergedParts;
	if (!options.sides.empty())
		result.data["otherSides"] = detected.otherSides;
	result.data["flatParts"] = detected.flatParts;
	if (!blocked.empty())
		result.data["continuedParts"] = detected.continuedParts;
	float maxDistance = 0.0f;
	for (float d : detected.distance)
		maxDistance = std::max(maxDistance, d);
	result.data["longestHang"] = maxDistance;
	float lowest = verts.empty() ? 0.0f : verts[0].z;
	for (auto& v : verts)
		lowest = std::min(lowest, v.z);
	result.data["lowestPoint"] = lowest;
	if (detected.rootVertices == 0)
		result.data["hint"] = "No vertex is on the attachment. Use rootShape (e.g. a head mesh) or a larger rootRadius.";
	else if (detected.chains.empty())
		result.data["hint"] = "Nothing hangs far enough for a chain (minHang).";

	if (preview || detected.chains.empty()) {
		result.data["preview"] = preview;
		return result;
	}

	// Bones of an earlier run with this prefix go first, with their weights
	std::vector<std::string> oldBones = PrefixBones(prefix);
	if (!oldBones.empty()) {
		if (!McpArgs::Bool(args, "replace", false))
			throw McpToolError("There are bones named '" + prefix + " ...' already. Pass replace: true to make them again.");
		// Their weights go to the first bone up the skeleton that stays (the anchor), or deleting them would leave the
		// vertices this run doesn't chain again without weights
		const std::set<std::string> oldSet(oldBones.begin(), oldBones.end());
		for (auto& shapeName : project.GetWorkNif()->GetShapeNames()) {
			for (auto& old : oldBones) {
				auto w = anim->GetWeightsPtr(shapeName, old);
				if (!w || w->empty())
					continue;
				AnimBone* keep = skeleton.GetBonePtr(old);
				while (keep && oldSet.count(keep->boneName))
					keep = keep->parent;
				if (!keep)
					continue;
				anim->AddShapeBone(shapeName, keep->boneName);
				std::unordered_map<uint16_t, float> merged;
				anim->GetWeights(shapeName, keep->boneName, merged);
				for (auto& vw : *anim->GetWeightsPtr(shapeName, old))
					merged[vw.first] += vw.second;
				anim->SetWeights(shapeName, keep->boneName, merged);
			}
		}
		McpRunStep(mcp, {{"type", "DeleteBones"}, {"BoneNames", oldBones}, {"FromProject", true}});
		result.data["replacedBones"] = oldBones;
	}

	// The joints as custom bones: each chain's root on the anchor, every other joint on the one before it, all
	// turned like the anchor
	for (size_t c = 0; c < detected.chains.size(); c++) {
		auto& chain = detected.chains[c];
		for (size_t j = 0; j < chain.joints.size(); j++) {
			const std::string parentName = j == 0 ? anchorName : boneNames[c][j - 1];
			AnimBone* parent = skeleton.GetBonePtr(parentName);
			if (!parent)
				throw McpToolError("The bone '" + parentName + "' wasn't added.");
			Vector3 local = parent->xformToGlobal.InverseTransform().ApplyTransform(chain.joints[j] / units);
			McpRunStep(mcp,
					   {{"type", "AddCustomBone"},
						{"BoneName", boneNames[c][j]},
						{"ParentBone", parentName},
						{"TransX", local.x},
						{"TransY", local.y},
						{"TransZ", local.z},
						{"RotX", 0.0f},
						{"RotY", 0.0f},
						{"RotZ", 0.0f}});
		}
	}

	// Custom bones are added to every shape; only the rigged one should have them, or the other shapes' physics
	// would take them (a head collider would claim the chain's bones as its own)
	{
		std::vector<std::string> allNew;
		for (auto& names : boneNames)
			allNew.insert(allNew.end(), names.begin(), names.end());
		json others = json::array();
		for (auto& name : project.GetWorkNif()->GetShapeNames())
			if (std::find(shapeNames.begin(), shapeNames.end(), name) == shapeNames.end())
				others.push_back(name);
		if (!others.empty())
			McpRunStep(mcp, {{"type", "DeleteBones"}, {"BoneNames", allNew}, {"FromProject", false}, {"TargetMeshes", others}});
	}

	// The vertices' current bones keep what the chains leave them. One column per bone of any of the shapes, then
	// the chain bones.
	auto weights = PhysicsChains::JointWeights(detected, options);
	std::vector<std::string> bones;
	for (auto& name : shapeNames) {
		auto it = anim->shapeBones.find(name);
		if (it != anim->shapeBones.end())
			for (auto& b : it->second)
				if (std::find(bones.begin(), bones.end(), b) == bones.end())
					bones.push_back(b);
	}
	const size_t oldBoneCount = bones.size();
	std::vector<std::vector<int>> chainColumns(detected.chains.size());
	for (size_t c = 0; c < detected.chains.size(); c++)
		for (size_t j = 0; j < boneNames[c].size(); j++) {
			chainColumns[c].push_back(int(bones.size()));
			bones.push_back(boneNames[c][j]);
		}

	MeshAnalysis::DenseWeights before(verts.size(), std::vector<float>(bones.size(), 0.0f));
	for (size_t i = 0; i < shapeNames.size(); i++) {
		const size_t count = offsets[i + 1] - offsets[i];
		for (size_t b = 0; b < oldBoneCount; b++)
			if (auto w = anim->GetWeightsPtr(shapeNames[i], bones[b]))
				for (auto& vw : *w)
					if (vw.first < count)
						before[offsets[i] + vw.first][b] = vw.second;
	}

	// On a chain, what is left for the other body bones (a hair's neck and shoulder weights on the spine) goes to the
	// anchor the chain hangs on, the more the further down: hair over the chest that follows the spine and a chain at
	// once collides, but can't move out of the body (jitter)
	const bool fadeBody = McpArgs::Bool(args, "fadeBodyBones", true);
	int anchorColumn = -1;
	for (size_t b = 0; b < oldBoneCount; b++)
		if (bones[b] == anchorName)
			anchorColumn = int(b);

	MeshAnalysis::DenseWeights after = before;
	std::vector<float> influence(verts.size(), 0.0f);
	for (size_t v = 0; v < verts.size(); v++) {
		const auto& w = weights[v];
		if (w.keep >= 1.0f)
			continue;
		influence[v] = 1.0f;
		float total = 0.0f;
		for (size_t b = 0; b < oldBoneCount; b++)
			total += before[v][b];
		float moved = 0.0f;
		for (size_t b = 0; b < oldBoneCount; b++) {
			after[v][b] = total > 0.0f ? before[v][b] / total * w.keep : 0.0f;
			if (fadeBody && anchorColumn >= 0 && int(b) != anchorColumn) {
				moved += after[v][b] * (1.0f - w.keep);
				after[v][b] *= w.keep;
			}
		}
		if (anchorColumn >= 0)
			after[v][anchorColumn] += moved;
		const int c = detected.chain[v];
		for (auto& jw : w.joints)
			after[v][chainColumns[c][jw.first]] = jw.second;
	}

	// Blend the chains where their strands touch, so neighboring strands don't tear apart
	const float smoothRadius = McpArgs::Float(args, "smoothRadius", 2.0f);
	if (smoothRadius > 0.0f) {
		std::vector<bool> chainBones(bones.size(), false);
		for (size_t b = oldBoneCount; b < bones.size(); b++)
			chainBones[b] = true;
		double edgeSum = 0.0;
		size_t edgeCount = 0;
		for (size_t v = 0; v < verts.size(); v++)
			for (int n : neighbors[v]) {
				edgeSum += verts[v].DistanceTo(verts[n]);
				edgeCount++;
			}
		const float meanEdge = edgeCount ? float(edgeSum / edgeCount) : 1.0f;
		const float rings = smoothRadius / std::max(0.01f, meanEdge);
		const int iterations = std::clamp(int(std::ceil(rings * rings / 0.5f)), 1, 500);
		after = MeshAnalysis::SmoothWeights(after, neighbors, influence, iterations, 0.5f, &chainBones);
	}

	// Blend the chains and the anchor where a chain's vertices meet those that stay (a strand split off the hair next to it, the
	// hang ceiling): there the weights would jump from the chain to the head across one edge and the planes stretch
	const float borderRadius = McpArgs::Float(args, "borderRadius", 3.0f);
	if (borderRadius > 0.0f) {
		std::vector<float> share(verts.size(), 0.0f);
		for (size_t v = 0; v < verts.size(); v++)
			for (size_t b = oldBoneCount; b < bones.size(); b++)
				share[v] += after[v][b];
		// Distance along the mesh from the edges where the share jumps, up to borderRadius
		std::vector<float> dist(verts.size(), -1.0f);
		using Entry = std::pair<float, int>;
		std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
		for (size_t v = 0; v < verts.size(); v++)
			for (int n : neighbors[v])
				if (std::fabs(share[v] - share[n]) > 0.25f) {
					dist[v] = 0.0f;
					queue.emplace(0.0f, int(v));
					break;
				}
		while (!queue.empty()) {
			auto [d, u] = queue.top();
			queue.pop();
			if (d > dist[u])
				continue;
			for (int n : neighbors[u]) {
				const float nd = d + verts[u].DistanceTo(verts[n]);
				if (nd > borderRadius || (dist[n] >= 0.0f && nd >= dist[n]))
					continue;
				dist[n] = nd;
				queue.emplace(nd, n);
			}
		}
		// Not the roots (they stay on the head) nor what an earlier run chained (its own chains)
		std::vector<float> border(verts.size(), 0.0f);
		size_t borderCount = 0;
		double edgeSum = 0.0;
		size_t edgeCount = 0;
		for (size_t v = 0; v < verts.size(); v++) {
			if (dist[v] < 0.0f || roots[v] || (!blocked.empty() && blocked[v]))
				continue;
			border[v] = 1.0f - 0.5f * dist[v] / borderRadius;
			borderCount++;
			for (int n : neighbors[v]) {
				edgeSum += verts[v].DistanceTo(verts[n]);
				edgeCount++;
			}
		}
		if (borderCount) {
			const float meanEdge = edgeCount ? float(edgeSum / edgeCount) : 1.0f;
			const float rings = borderRadius / std::max(0.01f, meanEdge);
			const int iterations = std::clamp(int(std::ceil(rings * rings / 0.5f)), 1, 500);
			// Between the chains and the anchor only: body bones (spine) stay as they are
			std::vector<bool> blended(bones.size(), false);
			for (size_t b = oldBoneCount; b < bones.size(); b++)
				blended[b] = true;
			if (anchorColumn >= 0)
				blended[anchorColumn] = true;
			after = MeshAnalysis::SmoothWeights(after, neighbors, border, iterations, 0.5f, &blended);
			for (size_t v = 0; v < verts.size(); v++)
				if (border[v] > 0.0f)
					influence[v] = 1.0f;
		}
		result.data["borderVertices"] = borderCount;
	}

	const int maxInfluences = McpArgs::Int(args, "maxInfluences", McpInfluenceLimit());
	for (size_t v = 0; v < verts.size(); v++)
		if (influence[v] > 0.0f)
			MeshAnalysis::LimitWeights(after[v], maxInfluences, 0.01f);

	size_t changed = 0;
	for (size_t i = 0; i < shapeNames.size(); i++) {
		std::map<std::string, std::unordered_map<uint16_t, float>> changes;
		for (size_t v = offsets[i]; v < offsets[i + 1]; v++)
			for (size_t b = 0; b < bones.size(); b++)
				if (std::fabs(after[v][b] - before[v][b]) > 1e-6f)
					changes[bones[b]][static_cast<uint16_t>(v - offsets[i])] = after[v][b];
		changed += McpApplyWeights(mcp, mcp.Shape(shapeNames[i]), changes);

		// The roots carry no weights, but the chains hang on them: give the shapes the bones, so they are saved
		for (size_t c = 0; c < detected.chains.size(); c++)
			if (anim->GetShapeBoneIndex(shapeNames[i], boneNames[c][0]) < 0)
				anim->AddShapeBone(shapeNames[i], boneNames[c][0]);
	}
	result.data["changedWeights"] = changed;

	frame->UpdateAnimationGUI();
	return result;
}

// The chains of the project's custom bones with names starting with prefix: from each bone whose parent isn't one of them
// down to the end, following the only child
std::vector<std::vector<std::string>> ChainsFromPrefix(AnimInfo* anim, const std::string& prefix) {
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	// Only bones a shape of the project has: the skeleton keeps custom bones of files loaded before
	std::set<std::string> used;
	for (auto& sb : anim->shapeBones)
		used.insert(sb.second.begin(), sb.second.end());
	std::vector<std::string> bones;
	for (auto& b : PrefixBones(prefix))
		if (used.count(b))
			bones.push_back(b);
	std::set<std::string> set(bones.begin(), bones.end());
	std::map<std::string, std::vector<std::string>> children;
	std::vector<std::string> roots;
	for (auto& b : bones) {
		AnimBone* bone = skeleton.GetBonePtr(b);
		if (bone->parent && set.count(bone->parent->boneName))
			children[bone->parent->boneName].push_back(b);
		else
			roots.push_back(b);
	}
	std::vector<std::vector<std::string>> chains;
	for (auto& root : roots) {
		std::vector<std::string> chain{root};
		while (children[chain.back()].size() == 1)
			chain.push_back(children[chain.back()][0]);
		chains.push_back(std::move(chain));
	}
	return chains;
}

McpToolResult WritePhysicsXml(OutfitStudioMcp& mcp, const json& args) {
	using namespace tinyxml2;
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();

	const std::string path = McpFilePath(McpArgs::String(args, "path"));
	McpCheckOverwrite(path, args);

	XMLDocument templ;
	const std::string templatePath = McpFilePath(McpArgs::String(args, "template"));
	if (templ.LoadFile(templatePath.c_str()) != XML_SUCCESS || !templ.FirstChildElement("system"))
		throw McpToolError("Can't read the template XML '" + templatePath + "'.");
	XMLElement* templSystem = templ.FirstChildElement("system");
	std::vector<XMLElement*> boneDefaults, constraintDefaults;
	XMLElement* vertexShapeTemplate = nullptr;
	std::map<std::string, XMLElement*> triangleShapes;
	XMLElement* firstTriangleShape = nullptr;
	for (XMLElement* e = templSystem->FirstChildElement(); e; e = e->NextSiblingElement()) {
		const std::string tag = e->Name();
		if (tag == "bone-default" && !e->Attribute("name"))
			boneDefaults.push_back(e);
		else if (tag == "generic-constraint-default" && !e->Attribute("name"))
			constraintDefaults.push_back(e);
		else if (tag == "per-vertex-shape" && !vertexShapeTemplate)
			vertexShapeTemplate = e;
		else if (tag == "per-triangle-shape" && e->Attribute("name")) {
			triangleShapes[e->Attribute("name")] = e;
			if (!firstTriangleShape)
				firstTriangleShape = e;
		}
	}
	if (boneDefaults.empty() || constraintDefaults.empty())
		throw McpToolError("The template has no <bone-default> or <generic-constraint-default>.");

	// Chains: given, or found by bone name prefix
	struct ChainSpec {
		std::vector<std::string> bones;
		size_t profile = 0;
	};
	std::vector<ChainSpec> chains;
	if (const json* given = McpArgs::Find(args, "chains")) {
		if (!given->is_array())
			McpArgs::Wrong("chains", "a list of {bones, profile}");
		for (auto& c : *given) {
			ChainSpec spec;
			spec.bones = McpArgs::Strings(c, "bones");
			spec.profile = size_t(std::max(0, McpArgs::Int(c, "profile", 0)));
			if (spec.bones.size() < 2)
				throw McpToolError("A chain needs a root and at least one bone that moves.");
			chains.push_back(std::move(spec));
		}
	}
	for (auto& prefix : McpArgs::Strings(args, "prefixes")) {
		for (auto& bones : ChainsFromPrefix(anim, prefix)) {
			if (bones.size() < 2)
				continue;
			ChainSpec spec;
			spec.bones = bones;
			chains.push_back(std::move(spec));
		}
	}
	if (chains.empty())
		throw McpToolError("No chains: give chains or prefixes of chain bones.");

	// Profiles of individual chains by their root bone
	if (const json* profiles = McpArgs::Find(args, "profiles"))
		for (auto& c : chains)
			if (profiles->is_object() && profiles->contains(c.bones[0]) && (*profiles)[c.bones[0]].is_number_integer())
				c.profile = size_t(std::max(0, (*profiles)[c.bones[0]].get<int>()));

	const size_t profileCount = std::min(boneDefaults.size(), constraintDefaults.size());
	for (auto& c : chains)
		if (c.profile >= profileCount)
			throw McpToolError("Chain '" + c.bones[0] + "' uses profile " + std::to_string(c.profile) + ", but the template has profiles 0 to " +
							   std::to_string(profileCount - 1) + " (pairs of <bone-default> and <generic-constraint-default>).");

	std::set<std::string> dynamicBones;
	for (auto& c : chains)
		for (size_t i = 1; i < c.bones.size(); i++)
			dynamicBones.insert(c.bones[i]);

	// Shapes the hair is in: given, or the outfit shapes weighted to a chain bone. Their other bones are
	// declared without mass, so they follow the skeleton.
	std::vector<std::string> vertexShapes = McpArgs::Strings(args, "perVertexShapes");
	std::vector<std::string> kinematic;
	auto addKinematic = [&](const std::string& bone) {
		if (!dynamicBones.count(bone) && std::find(kinematic.begin(), kinematic.end(), bone) == kinematic.end())
			kinematic.push_back(bone);
	};
	for (auto& c : chains)
		addKinematic(c.bones[0]);
	std::vector<std::string> loadedShapes;
	for (auto* shape : project.GetWorkNif()->GetShapes()) {
		auto it = anim->shapeBones.find(shape->name.get());
		if (it == anim->shapeBones.end())
			continue;
		bool chained = std::any_of(it->second.begin(), it->second.end(), [&](const std::string& b) { return dynamicBones.count(b) > 0; });
		if (!chained)
			continue;
		loadedShapes.push_back(shape->name.get());
		for (auto& b : it->second)
			addKinematic(b);
	}
	if (vertexShapes.empty())
		vertexShapes = loadedShapes;
	for (auto& b : McpArgs::Strings(args, "kinematicBones"))
		addKinematic(b);

	XMLDocument doc;
	doc.InsertEndChild(doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\""));
	XMLElement* system = doc.NewElement("system");
	system->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	system->SetAttribute("xsi:noNamespaceSchemaLocation", "description.xsd");
	doc.InsertEndChild(system);

	// Colliders go last, after every bone is declared: SMP gives a shape all bones it is weighted to, and
	// declarations after it would be skipped. Their bones are declared without mass first, those of loaded
	// colliders here, others with kinematicBones.
	std::vector<XMLElement*> colliderElements;
	json colliders = json::array();
	if (const json* given = McpArgs::Find(args, "perTriangleShapes")) {
		if (!given->is_array())
			McpArgs::Wrong("perTriangleShapes", "a list of names or {name, like}");
		for (auto& entry : *given) {
			std::string name = entry.is_string() ? entry.get<std::string>() : McpArgs::String(entry, "name");
			std::string like = entry.is_object() ? McpArgs::String(entry, "like", name) : name;
			XMLElement* source = triangleShapes.count(like) ? triangleShapes[like] : firstTriangleShape;
			if (!source)
				throw McpToolError("The template has no <per-triangle-shape> to copy for '" + name + "'.");
			XMLElement* copy = source->DeepClone(&doc)->ToElement();
			copy->SetAttribute("name", name.c_str());
			colliderElements.push_back(copy);
			colliders.push_back(name);
			auto it = anim->shapeBones.find(name);
			if (it != anim->shapeBones.end())
				for (auto& b : it->second)
					addKinematic(b);
		}
	}

	for (auto& b : kinematic) {
		XMLElement* bone = doc.NewElement("bone");
		bone->SetAttribute("name", b.c_str());
		system->InsertEndChild(bone);
	}

	std::set<size_t> profilesUsed;
	for (auto& c : chains)
		profilesUsed.insert(c.profile);
	for (size_t p : profilesUsed) {
		system->InsertEndChild(boneDefaults[std::min(p, boneDefaults.size() - 1)]->DeepClone(&doc));
		for (auto& c : chains) {
			if (c.profile != p)
				continue;
			for (size_t i = 1; i < c.bones.size(); i++) {
				XMLElement* bone = doc.NewElement("bone");
				bone->SetAttribute("name", c.bones[i].c_str());
				system->InsertEndChild(bone);
			}
		}
	}
	// With alignFrames, each joint's constraint frame is turned so its -Z runs along the part of the strand the joint
	// moves (to the next joint, or from the joint above at the tip): the template's limits are meant for hair hanging
	// straight down, where Z is the twist around the strand and X and Y swing it. Unturned, a ponytail sticking out
	// backwards would swing sideways with the twist limits.
	const bool alignFrames = McpArgs::Bool(args, "alignFrames", false);
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	auto bonePos = [&](const std::string& name, bool& ok) {
		AnimBone* bone = skeleton.GetBonePtr(name);
		ok = bone != nullptr;
		return bone ? bone->xformToGlobal.translation : Vector3();
	};
	auto frameFor = [&](const std::vector<std::string>& bones, size_t i, XMLElement* constraint) {
		bool ok1 = false, ok2 = false;
		Vector3 from, to;
		if (i + 1 < bones.size()) {
			from = bonePos(bones[i], ok1);
			to = bonePos(bones[i + 1], ok2);
		}
		else {
			from = bonePos(bones[i - 1], ok1);
			to = bonePos(bones[i], ok2);
		}
		Vector3 d = to - from;
		if (!ok1 || !ok2 || d.length() < 1e-4f)
			return;
		d.Normalize();
		// Shortest turn from straight down to d, in the joint's own space (its rotation is the parent's, the
		// identity for chains from make_physics_chains)
		AnimBone* bone = skeleton.GetBonePtr(bones[i]);
		Matrix3 toLocal = bone->xformToGlobal.rotation.Transpose();
		d = toLocal * d;
		const Vector3 down(0.0f, 0.0f, -1.0f);
		Vector3 axis = down.cross(d);
		const float cosA = std::clamp(down.dot(d), -1.0f, 1.0f);
		float qx = 0.0f, qy = 0.0f, qz = 0.0f, qw = 1.0f;
		if (axis.length() < 1e-5f) {
			if (cosA < 0.0f) {
				qx = 1.0f;
				qw = 0.0f;
			}
		}
		else {
			axis.Normalize();
			const float half = std::acos(cosA) * 0.5f;
			qx = axis.x * std::sin(half);
			qy = axis.y * std::sin(half);
			qz = axis.z * std::sin(half);
			qw = std::cos(half);
		}
		XMLElement* frame = constraint->InsertNewChildElement("frameInA");
		XMLElement* basis = frame->InsertNewChildElement("basis");
		basis->SetAttribute("x", qx);
		basis->SetAttribute("y", qy);
		basis->SetAttribute("z", qz);
		basis->SetAttribute("w", qw);
		XMLElement* origin = frame->InsertNewChildElement("origin");
		origin->SetAttribute("x", 0);
		origin->SetAttribute("y", 0);
		origin->SetAttribute("z", 0);
	};

	for (size_t p : profilesUsed) {
		system->InsertEndChild(constraintDefaults[std::min(p, constraintDefaults.size() - 1)]->DeepClone(&doc));
		XMLElement* group = doc.NewElement("constraint-group");
		for (auto& c : chains) {
			if (c.profile != p)
				continue;
			for (size_t i = 1; i < c.bones.size(); i++) {
				XMLElement* constraint = doc.NewElement("generic-constraint");
				constraint->SetAttribute("bodyA", c.bones[i].c_str());
				constraint->SetAttribute("bodyB", c.bones[i - 1].c_str());
				if (alignFrames)
					frameFor(c.bones, i, constraint);
				group->InsertEndChild(constraint);
			}
		}
		system->InsertEndChild(group);
	}

	for (auto& name : vertexShapes) {
		XMLElement* shape;
		if (vertexShapeTemplate)
			shape = vertexShapeTemplate->DeepClone(&doc)->ToElement();
		else {
			shape = doc.NewElement("per-vertex-shape");
			shape->InsertNewChildElement("margin")->SetText("0");
			shape->InsertNewChildElement("shared")->SetText("internal");
			shape->InsertNewChildElement("tag")->SetText("hair");
		}
		shape->SetAttribute("name", name.c_str());
		system->InsertEndChild(shape);
	}
	for (XMLElement* collider : colliderElements)
		system->InsertEndChild(collider);

	XMLPrinter printer;
	doc.Print(&printer);
	std::string text = printer.CStr();
	// tinyxml2 indents with 4 spaces; the game's files use tabs
	std::string out;
	std::istringstream lines(text);
	for (std::string line; std::getline(lines, line);) {
		size_t spaces = 0;
		while (spaces < line.size() && line[spaces] == ' ')
			spaces++;
		out += std::string(spaces / 4, '\t') + line.substr(spaces) + "\n";
	}
	std::ofstream file(path, std::ios::binary);
	if (!file)
		throw McpToolError("Can't write '" + path + "'.");
	file << out;

	McpToolResult result;
	json chainList = json::array();
	for (auto& c : chains)
		chainList.push_back({{"bones", c.bones}, {"profile", c.profile}});
	result.data["path"] = path;
	result.data["chains"] = chainList;
	result.data["kinematicBones"] = kinematic;
	result.data["perVertexShapes"] = vertexShapes;
	result.data["perTriangleShapes"] = colliders;
	return result;
}

McpToolResult SimulatePhysics(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();

	const std::string xmlOverride = McpArgs::String(args, "xml", "");
	std::string xmlPath = xmlOverride.empty() ? "" : McpFilePath(xmlOverride);
	Physics::XmlStreamResolver resolver;
	if (!xmlPath.empty())
		resolver = [xmlPath](const std::string&) -> std::unique_ptr<std::istream> {
			auto stream = std::make_unique<std::ifstream>(xmlPath, std::ios::binary);
			if (!*stream)
				return nullptr;
			return stream;
		};
	else
		resolver = [&project](const std::string& link) { return project.GetPhysicsXmlStream(link); };

	// Links of the shapes; with xml given, every outfit shape is linked to it
	Physics::ShapePhysicsFileMap links = project.shapePhysicsFiles;
	if (!xmlPath.empty())
		for (auto* shape : project.GetWorkNif()->GetShapes())
			links[shape->name.get()] = {xmlPath};

	McpPoseScope pose(mcp);
	pose.Set("none");

	Physics::Controller controller;
	std::vector<std::string> warnings;
	size_t systems = controller.BuildFromNif(project.GetWorkNif(), project.GetWorkAnim(), resolver, links, warnings);
	McpToolResult result;
	result.data["systems"] = systems;
	if (!warnings.empty())
		result.data["warnings"] = warnings;
	if (systems == 0) {
		result.data["hint"] = "Nothing to simulate: no physics XML is linked or found (pass xml), or physics isn't available in this build.";
		return result;
	}
	controller.ResetDynamics();

	const float settle = std::clamp(McpArgs::Float(args, "settle", 2.0f), 0.0f, 30.0f);
	const float duration = std::clamp(McpArgs::Float(args, "duration", 3.0f), 0.0f, 60.0f);
	const float turnDegrees = McpArgs::Float(args, "turnDegrees", 70.0f);
	const float turnPeriod = std::max(0.2f, McpArgs::Float(args, "turnPeriod", 1.2f));
	const float settleAfter = std::clamp(McpArgs::Float(args, "settleAfter", 2.0f), 0.0f, 30.0f);
	const float units = McpBoneUnits(mcp);
	constexpr float Tick = 1.0f / 60.0f;

	struct BoneTrack {
		Vector3 rest;
		float restLength = 0.0f; // to the parent
		float reach = 0.0f;		 // to the first bone up the chain that the simulation doesn't move
		std::string parent;
		float maxMove = 0.0f;
		float maxStretch = 0.0f;
		float settled = 0.0f;
		Vector3 settledPos;
		float final = 0.0f;
		float returned = 0.0f; // from where it settled at first to where it settled after the motion
		float drop = 0.0f;
		bool invalid = false;
		// In the last second of each settle phase: how much the bone's velocity changes from frame to frame (summed per
		// second), the shaking without the slow swing of a strand still settling. In the first, from its rest pose, a
		// bone that shakes jitters (e.g. pushed by a collider it rests in and pulled back by its constraint). And the
		// path length in the second, after the motion: still swinging (little damping).
		Vector3 lastPos, lastStep;
		int known = 0;
		float shake[2] = {0.0f, 0.0f};
		float path[2] = {0.0f, 0.0f};
	};
	std::map<std::string, BoneTrack> tracks;

	auto positionOf = [&](const std::string& name, bool& known) {
		auto& overrides = controller.PoseOverrides();
		auto it = overrides.find(name);
		known = true;
		if (it != overrides.end())
			return it->second.translation * units;
		AnimBone* bone = skeleton.GetBonePtr(name);
		if (!bone) {
			known = false;
			return Vector3();
		}
		return bone->xformPoseToGlobal.translation * units;
	};

	auto measure = [&](int phase) {
		for (auto& o : controller.PoseOverrides()) {
			BoneTrack& t = tracks[o.first];
			if (t.parent.empty() && t.restLength == 0.0f) {
				AnimBone* bone = skeleton.GetBonePtr(o.first);
				if (bone) {
					t.rest = bone->xformPoseToGlobal.translation * units;
					if (bone->parent) {
						t.parent = bone->parent->boneName;
						t.restLength = t.rest.DistanceTo(bone->parent->xformPoseToGlobal.translation * units);
					}
					AnimBone* fixed = bone->parent;
					while (fixed && controller.PoseOverrides().count(fixed->boneName))
						fixed = fixed->parent;
					if (fixed)
						t.reach = t.rest.DistanceTo(fixed->xformPoseToGlobal.translation * units);
				}
			}
			Vector3 p = o.second.translation * units;
			if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
				t.invalid = true;
				continue;
			}
			const float move = p.DistanceTo(t.rest);
			t.maxMove = std::max(t.maxMove, move);
			if (phase >= 3) {
				const Vector3 step = p - t.lastPos;
				if (t.known >= 1)
					t.path[phase - 3] += step.length();
				if (t.known >= 2)
					t.shake[phase - 3] += (step - t.lastStep).length();
				t.lastPos = p;
				t.lastStep = step;
				t.known++;
				continue;
			}
			if (!t.parent.empty() && t.restLength > 0.01f) {
				bool known = false;
				Vector3 pp = positionOf(t.parent, known);
				if (known)
					t.maxStretch = std::max(t.maxStretch, pp.DistanceTo(p) / t.restLength);
			}
			if (phase == 0) {
				t.settled = move;
				t.settledPos = p;
			}
			else if (phase == 2) {
				t.final = move;
				t.returned = p.DistanceTo(t.settledPos);
				t.drop = t.rest.z - p.z;
			}
		}
	};

	// Jitter is measured over the last second of each settle phase
	const int jitterTicks = int(1.0f / Tick);
	auto settleFor = [&](float seconds, int jitterPhase) {
		const int ticks = int(seconds / Tick);
		for (auto& t : tracks)
			t.second.known = 0;
		for (int i = 0; i < ticks; i++) {
			controller.Step(Tick);
			if (i >= ticks - jitterTicks - 1)
				measure(jitterPhase);
		}
	};
	settleFor(settle, 3);
	measure(0);

	// Turn the character back and forth under the hair
	float yaw = 0.0f;
	const int motionTicks = int(duration / Tick);
	for (int i = 1; i <= motionTicks; i++) {
		const float target = turnDegrees * std::sin(2.0f * 3.14159265f * (i * Tick) / turnPeriod);
		controller.InjectCameraYaw(target - yaw);
		yaw = target;
		controller.Step(Tick);
		measure(1);
	}
	controller.InjectCameraYaw(-yaw);
	settleFor(settleAfter, 4);
	measure(2);

	const float maxSag = McpArgs::Float(args, "maxSag", 0.6f);
	const float minReach = std::max(0.1f, McpArgs::Float(args, "minReach", 8.0f));
	const float maxReturn = McpArgs::Float(args, "maxReturn", 0.2f);
	const float maxStretch = McpArgs::Float(args, "maxStretch", 1.5f);
	const float maxMove = McpArgs::Float(args, "maxMove", 80.0f);
	const float maxJitter = McpArgs::Float(args, "maxJitter", 1.0f);
	const float jitterSeconds = float(jitterTicks) * Tick;
	json bones = json::array();
	json problems = json::array();
	std::vector<std::pair<float, std::string>> order;
	for (auto& t : tracks)
		order.emplace_back(t.second.maxMove, t.first);
	std::sort(order.rbegin(), order.rend());
	for (auto& o : order) {
		BoneTrack& t = tracks[o.second];
		json entry = {
			{"bone", o.second},
			{"maxMove", t.maxMove},
			{"settled", t.settled},
			{"final", t.final},
			{"returned", t.returned},
			{"sag", t.settled / std::max(minReach, t.reach)},
			{"drop", t.drop},
			{"maxStretch", t.maxStretch},
			{"jitter", t.shake[0] / jitterSeconds},
			{"swingAfter", t.path[1] / jitterSeconds},
		};
		std::vector<std::string> flags;
		if (t.invalid)
			flags.push_back("invalid");
		if (t.maxMove > maxMove)
			flags.push_back("exploded");
		if (t.settled / std::max(minReach, t.reach) > maxSag)
			flags.push_back("sags");
		if (t.returned > std::max(2.0f, maxReturn * t.reach))
			flags.push_back("doesNotReturn");
		if (t.maxStretch > maxStretch)
			flags.push_back("stretched");
		if (t.shake[0] / jitterSeconds > maxJitter)
			flags.push_back("jitters");
		if (!flags.empty()) {
			entry["flags"] = flags;
			problems.push_back(entry);
		}
		bones.push_back(entry);
	}
	const int limit = std::max(1, McpArgs::Int(args, "maxBones", 40));
	if (bones.size() > size_t(limit))
		bones.erase(bones.begin() + limit, bones.end());
	result.data["simulatedBones"] = tracks.size();
	result.data["bones"] = bones;
	result.data["problems"] = problems;
	result.data["ok"] = problems.empty();
	return result;
}

} // namespace

void AddPhysicsTools(OutfitStudioMcp& mcp) {
	McpTool chains;
	chains.name = "make_physics_chains";
	chains.description =
		"Gives the hanging parts of a shape (the strands of a hairstyle, ponytails, braids, ribbons) chains of custom "
		"bones for SMP physics and weights them, like rigging by hand: finds the vertices on the attachment (rootShape, "
		"e.g. a head mesh, or a sphere around the anchor bone), measures how far each vertex is from them along the mesh "
		"(separate hair cards closer than linkDistance count as joined), and gives every part that hangs farther than "
		"minHang a chain. Parts that start hanging around the head over a wider angle than sectorAngle are split (the "
		"back and the sides of open hair). Each chain has a root on the attachment, parented to anchorBone, and moving "
		"joints every boneSpacing along the strand, named '<prefix> <side> <n>' (side: Front, FrontR, R, BackR, Back, "
		"BackL, L, FrontL as seen from her). Vertices keep their bones up to hangStart and then blend along the chain; "
		"chains are blended where strands touch (smoothRadius). preview only reports the chains. Bones aren't in the undo "
		"history (an automatic snapshot is taken), the weights are one undo step. Then write_physics_xml writes the XML "
		"and simulate_physics tests it. Parts it can't find on its own (braids that start on the crown, jewelry, bead "
		"curtains) are rigged by hand with add_custom_bone (globalPosition) and set_weights; get_guide physics-chains "
		"has the whole workflow.";
	chains.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape to rig")},
			{"shapes", McpStringList("Several shapes to rig together, instead of shape: a hair and a separate part with the rest of its strands (locks, a braid) get the same chains, each with its own weights")},
			{"prefix", McpProp("string", "Start of the bone names, e.g. the hairstyle's name")},
			{"anchorBone", McpProp("string", "Bone the chain roots hang on. Default \"NPC Head [Head]\".")},
			{"rootShape", McpProp("string", "Shape whose surface the mesh is attached to, e.g. a head collision mesh. Vertices within rootDistance of it are roots.")},
			{"rootDistance", McpProp("number", "With rootShape: how close to it roots are. Default 2.5.")},
			{"rootSupportDistance", McpProp("number", "With rootShape: above rootSupportHeight, vertices this close to it are roots too, the volume lying on the scalp. Default 0, off.")},
			{"rootSupportHeight", McpProp("number", "Height relative to the anchor bone above which rootSupportDistance applies, about ear level for the head. Default 2.")},
			{"rootCenter", McpProp("array", "Without rootShape: center of the root sphere relative to the anchor bone. Default [0, 1, 5].")},
			{"rootRadius", McpProp("number", "Without rootShape: radius of the root sphere. Default 11.")},
			{"rootMinHeight", McpProp("number", "Roots aren't lower than this relative to the anchor bone. Default -6.")},
			{"linkDistance", McpProp("number", "Vertices closer than this count as joined. Default 1.")},
			{"hangStart", McpProp("number", "Distance from the roots where the first moving joint is. Default 4.")},
			{"minHang", McpProp("number", "How far a part hangs at least to get a chain. Default 12.")},
			{"boneSpacing", McpProp("number", "Distance between joints. Default 8.")},
			{"maxBones", McpProp("integer", "Most moving joints per chain. Default 7.")},
			{"maxChains", McpProp("integer", "Most chains. Default 6.")},
			{"sectorAngle", McpProp("number", "Split parts that start over a wider angle around the head (degrees). Default 110.")},
			{"minVertices", McpProp("integer", "Parts with fewer vertices get no chain. Default 30.")},
			{"hangBelow", McpProp("number", "Height relative to the anchor bone above which nothing hangs: chains start below it, the hair above keeps its bones. E.g. 2 for strands framing the face, so the volume on the head doesn't move with them. Default no limit.")},
			{"minJointDrop", McpProp("number", "Each joint after the first has to be this much lower than the one above, relative to boneSpacing; the chain ends where it isn't, so strands that turn around the head don't get joints that swing down. Default 0, off (ponytails may stick out first).")},
			{"lockGap", McpProp("number", "Where a strand falls apart into pieces farther apart than this (separate locks, dreads), a chain follows one piece instead of running between them. Default 2.")},
			{"minTipGap", McpProp("number", "A part split by sectorAngle stays one chain when its groups end closer together than this, like a thick ponytail. Default 6.")},
			{"minDrop", McpProp("number", "How far a part has to go down from where it starts hanging to its tip, relative to its length along the mesh. Parts running around the head stay rigid. Default 0.5.")},
			{"sides", McpStringList("Only chains starting at these sides, e.g. [\"Front\", \"FrontR\", \"R\", \"FrontL\", \"L\"] for strands framing the face. Default all.")},
			{"skipChained", McpProp("boolean", "Leave out vertices weighted at least half to custom bones, e.g. by an earlier run with another prefix: a second run with short settings then adds chains to the loose strands. Default false.")},
			{"smoothRadius", McpProp("number", "Blends the chains' weights over this distance where strands touch. 0 for off. Default 2.")},
			{"borderRadius", McpProp("number", "Blends the chains and the anchor bone over this distance where a chain's vertices meet hair that stays, so planes across the border don't stretch. 0 for off. Default 3.")},
			{"fadeBodyBones", McpProp("boolean", "On a chain, move what the vertices keep of body bones other than the anchor (spine, neck) to the anchor, the more the further down the chain, so hair over the chest follows only the head and the chain. Default true.")},
			{"maxInfluences", McpProp("integer", "Most bones per vertex. Default 4.")},
			{"replace", McpProp("boolean", "Delete bones named '<prefix> ...' from an earlier run first, their weights going to the bone they hang on. Default false, which refuses.")},
			{"preview", McpProp("boolean", "Only find and report the chains. Default false.")},
		},
		{});
	chains.handler = [&mcp](const json& args) { return MakePhysicsChains(mcp, args); };
	mcp.AddTool(std::move(chains));

	McpTool xml;
	xml.name = "write_physics_xml";
	xml.description =
		"Writes an SMP physics XML for bone chains (e.g. from make_physics_chains), with the bone and constraint "
		"settings of a template XML: its <bone-default> and <generic-constraint-default> blocks, numbered from 0 as "
		"profiles, its first <per-vertex-shape> for the shapes and its <per-triangle-shape>s for the colliders. Chain "
		"roots and the other bones of the simulated shapes are declared without mass so they follow the skeleton, the "
		"moving joints with their profile's settings and constrained to the joint above. A chain's first bone can be a "
		"skeleton bone (e.g. the head) for chains made by hand. Link the XML to the NIF with set_extra_data (\"HDT "
		"Skinned Mesh Physics Object\" on the root node); a NIF links one XML, so everything that moves in it (the hair "
		"and an accessory in the same NIF) goes into one file. Changing only settings needs only the XML written again, "
		"not the NIF.";
	xml.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "XML file to write")},
			{"template", McpProp("string", "XML whose settings are copied")},
			{"prefixes", McpStringList("Find the chains among the custom bones starting with these, e.g. the prefix of make_physics_chains")},
			{"chains", {{"type", "array"}, {"description", "Chains as {bones: [root, ..., tip], profile}, instead of or with prefixes. The root doesn't move; it can be a custom bone or a skeleton bone like the head."}}},
			{"profiles", {{"type", "object"}, {"description", "Profile number by chain root bone, e.g. {\"Hair Back 1\": 1}. Default 0."}}},
			{"perVertexShapes", McpStringList("Names of the simulated shapes, also of shapes in other files (e.g. the elf variant). Default the loaded shapes weighted to a chain.")},
			{"perTriangleShapes", {{"type", "array"}, {"description", "Colliders: names, or {name, like} to copy the template collider 'like'"}}},
			{"kinematicBones", McpStringList("More bones to declare without mass, e.g. the bones of colliders in other files")},
			{"alignFrames", McpProp("boolean", "Turn each joint's constraint frame so its -Z runs along the strand below the joint, so the template's limits (meant for hair hanging down: Z twist, X and Y swing) mean the same for parts sticking out, like a ponytail. Default false, the template's frames.")},
			{"overwrite", McpProp("boolean", "Replace an existing file. Default false.")},
		},
		{"path", "template"});
	xml.handler = [&mcp](const json& args) { return WritePhysicsXml(mcp, args); };
	mcp.AddTool(std::move(xml));

	McpTool simulate;
	simulate.name = "simulate_physics";
	simulate.description =
		"Runs the SMP physics of the loaded shapes without the window's preview: lets them settle at rest, turns the "
		"character back and forth (turnDegrees every turnPeriod seconds, for duration), lets them settle again, and "
		"reports per moving bone how far it moved from its rest position (maxMove), where it settled before (settled) and "
		"after the motion (final, returned: between the two), how far it sagged relative to its distance from the chain's "
		"fixed root (sag) and how much the link to its parent stretched. Hair always sags a little under gravity. Flags "
		"bones that explode (invalid, exploded), hang far off their pose (sags: falling chains, too soft constraints), "
		"don't come back to where they settled (doesNotReturn: tangled or caught on a collider), stretch, or keep moving "
		"at rest (jitter: how much its velocity changes from frame to frame, summed over the last second of the first "
		"settle; jitters: e.g. resting inside a collider's margin; swingAfter: path length in the last second after the "
		"motion, still swinging). xml "
		"simulates every shape with that file instead of the linked ones. Changes nothing.";
	simulate.inputSchema = McpSchema({
		{"xml", McpProp("string", "Physics XML for every shape, instead of the links of the NIFs")},
		{"settle", McpProp("number", "Seconds to settle first. Default 2.")},
		{"duration", McpProp("number", "Seconds of turning. Default 3.")},
		{"turnDegrees", McpProp("number", "How far she turns each way. Default 70.")},
		{"turnPeriod", McpProp("number", "Seconds per turn back and forth. Default 1.2.")},
		{"settleAfter", McpProp("number", "Seconds to settle after. Default 2.")},
		{"maxSag", McpProp("number", "Settled distance from rest relative to the distance from the chain's fixed root (at least minReach) that is flagged. Default 0.6.")},
		{"minReach", McpProp("number", "Smallest distance from the chain's root sag is measured against, so bones next to it aren't flagged for small pushes. Default 8.")},
		{"maxReturn", McpProp("number", "Distance between where a bone settled before and after the motion that is flagged, relative to its distance from the chain's fixed root (at least 2 units). Default 0.2.")},
		{"maxStretch", McpProp("number", "Stretch of a link (length / rest length) that is flagged. Default 1.5.")},
		{"maxMove", McpProp("number", "Distance from rest that counts as exploded. Default 80.")},
		{"maxJitter", McpProp("number", "Frame-to-frame velocity change per second at rest that is flagged as jitter. Default 1.")},
		{"maxBones", McpProp("integer", "Most bones to list, those moving most first. Default 40.")},
	});
	simulate.readOnly = true;
	simulate.handler = [&mcp](const json& args) { return SimulatePhysics(mcp, args); };
	mcp.AddTool(std::move(simulate));
}
