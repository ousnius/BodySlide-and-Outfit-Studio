/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectOperations.h"
#include "../components/ClippingFixer.h"
#include "../components/MaskOps.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <cfloat>
#include <unordered_set>
#include <wx/filename.h>
#include <wx/log.h>

using namespace nifly;

namespace {
// Moves each rigid part out of the body as a whole where the slider makes it sink in, along the direction the
// distance to the body grows fastest at its deepest vertex, until the part keeps the clearance (or the distance it
// had with all sliders at 0, when that's less). deltas are the rigid deltas of the slider.
void KeepPartsOutside(const std::vector<Vector3>& verts,
					  const std::vector<int>& parts,
					  const std::vector<float>& influence,
					  const std::vector<Vector3>& restBody,
					  const std::vector<Vector3>& sliderBody,
					  const std::vector<Triangle>& bodyTris,
					  float clearance,
					  std::vector<Vector3>& deltas) {
	int partCount = 0;
	for (int p : parts)
		partCount = std::max(partCount, p + 1);
	if (partCount == 0 || restBody.empty() || sliderBody.empty() || bodyTris.empty())
		return;

	// Only fully rigid vertices tell where a part is, partly masked ones blend into the cloth
	std::vector<size_t> members;
	for (size_t v = 0; v < verts.size() && v < parts.size() && v < influence.size(); v++)
		if (parts[v] >= 0 && influence[v] >= 0.999f)
			members.push_back(v);
	if (members.empty())
		return;

	constexpr float searchRadius = 5.0f;
	std::vector<Vector3> points(members.size());
	for (size_t i = 0; i < members.size(); i++)
		points[i] = verts[members[i]];
	std::vector<float> restDistances = ClippingFixer::SignedDistancesToBody(restBody, bodyTris, points, searchRadius);

	std::vector<float> goal(partCount, clearance);
	for (size_t i = 0; i < members.size(); i++)
		if (restDistances[i] != FLT_MAX)
			goal[parts[members[i]]] = std::min(goal[parts[members[i]]], restDistances[i]);

	constexpr float probe = 0.05f;
	constexpr float margin = 0.005f;
	std::vector<Vector3> offset(partCount);
	for (int iteration = 0; iteration < 4; iteration++) {
		for (size_t i = 0; i < members.size(); i++)
			points[i] = verts[members[i]] + deltas[members[i]] + offset[parts[members[i]]];
		std::vector<float> distances = ClippingFixer::SignedDistancesToBody(sliderBody, bodyTris, points, searchRadius);

		// How far each part is short of its goal, at its deepest vertex
		std::vector<float> need(partCount, 0.0f);
		std::vector<int> deepest(partCount, -1);
		for (size_t i = 0; i < members.size(); i++) {
			if (distances[i] == FLT_MAX)
				continue;
			const int p = parts[members[i]];
			const float short_ = goal[p] - distances[i];
			if (short_ > need[p] + 1e-5f) {
				need[p] = short_;
				deepest[p] = static_cast<int>(i);
			}
		}

		// Which way is out: how the distance changes around the deepest vertex
		std::vector<Vector3> probes;
		std::vector<int> probeParts;
		for (int p = 0; p < partCount; p++) {
			if (deepest[p] < 0)
				continue;
			const Vector3 at = points[deepest[p]];
			probes.push_back(at);
			probes.push_back(at + Vector3(probe, 0.0f, 0.0f));
			probes.push_back(at + Vector3(0.0f, probe, 0.0f));
			probes.push_back(at + Vector3(0.0f, 0.0f, probe));
			probeParts.push_back(p);
		}
		if (probes.empty())
			break;

		std::vector<float> probeDistances = ClippingFixer::SignedDistancesToBody(sliderBody, bodyTris, probes, searchRadius);
		for (size_t k = 0; k < probeParts.size(); k++) {
			const float* d = &probeDistances[k * 4];
			if (d[0] == FLT_MAX || d[1] == FLT_MAX || d[2] == FLT_MAX || d[3] == FLT_MAX)
				continue;
			Vector3 outward(d[1] - d[0], d[2] - d[0], d[3] - d[0]);
			if (outward.IsZero(true))
				continue;
			outward.Normalize();
			const int p = probeParts[k];
			offset[p] += outward * (need[p] + margin);
		}
	}

	for (size_t v = 0; v < deltas.size() && v < parts.size() && v < influence.size(); v++)
		if (parts[v] >= 0 && influence[v] > 0.0f)
			deltas[v] += offset[parts[v]] * influence[v];
}
} // namespace

RefreshFlags ProjectOperations::ApplyShapeEdits(UndoStateProject& usp, bool bIsUndo) {
	RefreshFlags refresh = RefreshFlags::PendingChanges;

	if (!usp.sliderName.empty()) {
		float sliderscale = 1 / usp.sliderscale;
		for (auto& uss : usp.usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			auto shape = project.GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
			if (!shape)
				continue;

			if (!uss.restDiffs.empty()) {
				// Pose-independent undo/redo: use stored rest-space NIF diffs
				std::unordered_map<uint16_t, Vector3> morphDiff;
				for (auto& rd : uss.restDiffs) {
					Vector3 nifDiff = rd.second;
					if (bIsUndo)
						nifDiff *= -1.0f;
					morphDiff[static_cast<uint16_t>(rd.first)] = Mesh::TransformDiffNifToMesh(nifDiff);
				}
				project.UpdateMorphResult(shape, usp.sliderName, morphDiff);
			}
			else {
				std::unordered_map<uint16_t, Vector3> strokeDiff;

				for (auto& ps : uss.pointStartState) {
					auto pe = uss.pointEndState.find(ps.first);
					if (pe == uss.pointEndState.end())
						continue;
					if (bIsUndo)
						strokeDiff[ps.first] = (ps.second - pe->second) * sliderscale;
					else
						strokeDiff[ps.first] = (pe->second - ps.second) * sliderscale;
				}

				if (project.bPose)
					project.UndoPoseDiffs(shape, strokeDiff);

				// Store rest-space NIF diffs for future undo/redo
				for (auto& sd : strokeDiff)
					uss.restDiffs[sd.first] = Mesh::TransformDiffMeshToNif(sd.second);

				project.UpdateMorphResult(shape, usp.sliderName, strokeDiff);
			}
		}

		refresh |= RefreshFlags::SliderData;
	}
	else {
		if (usp.undoType == UndoType::Weight) {
			for (auto& uss : usp.usss) {
				// Headless automation has no viewport mesh, but still applies weight edits.
				if (!project.GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName))
					continue;

				for (auto& bw : uss.boneWeights) {
					if (bw.weights.empty())
						continue;
					project.GetWorkAnim()->AddShapeBone(uss.shapeName, bw.boneName, bw.skinToBone);
					auto weights = project.GetWorkAnim()->GetWeightsPtr(uss.shapeName, bw.boneName);
					if (!weights)
						continue;
					for (auto& p : bw.weights) {
						float val = bIsUndo ? p.second.startVal : p.second.endVal;
						if (val == 0.0f)
							weights->erase(p.first);
						else
							(*weights)[p.first] = val;
					}

					// A bone the edit brought to the shape goes again with it, or when the edit left it without weights
					if (bw.addedBone && weights->empty())
						project.GetWorkAnim()->RemoveShapeBone(uss.shapeName, bw.boneName);
				}
				// Weights change the posed positions
				if (project.bPose)
					project.GetHost().UpdateShapeMesh(uss.shapeName);
			}
		}
		else if (usp.undoType == UndoType::Color) {
			for (auto& uss : usp.usss) {
				Mesh* m = GetMesh(uss.shapeName);
				if (!m)
					continue;

				auto colorPtr = project.GetWorkNif()->GetColorsForShape(m->shapeName);
				if (!colorPtr || colorPtr->empty())
					continue;

				std::vector<Color4> vcolors = (*colorPtr);

				if (bIsUndo) {
					for (auto& p : uss.pointStartState) {
						vcolors[p.first].r = p.second.x;
						vcolors[p.first].g = p.second.y;
						vcolors[p.first].b = p.second.z;
					}
				}
				else {
					for (auto& p : uss.pointEndState) {
						vcolors[p.first].r = p.second.x;
						vcolors[p.first].g = p.second.y;
						vcolors[p.first].b = p.second.z;
					}
				}

				project.GetWorkNif()->SetColorsForShape(m->shapeName, vcolors);
			}
		}
		else if (usp.undoType == UndoType::Alpha) {
			for (auto& uss : usp.usss) {
				Mesh* m = GetMesh(uss.shapeName);
				if (!m)
					continue;

				auto colorPtr = project.GetWorkNif()->GetColorsForShape(m->shapeName);
				if (!colorPtr || colorPtr->empty())
					continue;

				std::vector<Color4> vcolors = (*colorPtr);

				if (bIsUndo) {
					for (auto& p : uss.pointStartState)
						vcolors[p.first].a = p.second.x;
				}
				else {
					for (auto& p : uss.pointEndState)
						vcolors[p.first].a = p.second.x;
				}

				project.GetWorkNif()->SetColorsForShape(m->shapeName, vcolors);
			}
		}
	}

	return refresh;
}

void ProjectOperations::GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) {
	std::vector<std::string> activeBones;
	project.GetActiveBones(activeBones);

	for (auto& boneName : activeBones) {
		if (editor.lastNormalizeBones.find(boneName) != editor.lastNormalizeBones.end()) {
			if (normBones)
				normBones->push_back(boneName);
		}
		else {
			if (notNormBones)
				notNormBones->push_back(boneName);
		}
	}
}

OpResult ProjectOperations::ChangeSliderDeltas(NiShape* shape, const std::string& sliderName, const std::unordered_map<uint16_t, Vector3>& changes, UndoStateProject& usp) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");
	if (project.IsBaseShape(shape))
		return OpResult::Fail("The morphs of the reference shape aren't changed this way.");
	if (!project.ValidSlider(sliderName))
		return OpResult::Fail("Slider '" + sliderName + "' doesn't exist.");
	if (!usp.sliderName.empty() && usp.sliderName != sliderName)
		return OpResult::Fail("Changes of different sliders can't be one undo step.");

	OpResult result;
	if (changes.empty())
		return result;

	const uint16_t vertexCount = shape->GetNumVertices();
	TargetDataDiffs updates;
	UndoStateShape uss;
	uss.shapeName = shape->name.get();
	for (auto& change : changes) {
		if (change.first >= vertexCount)
			return OpResult::Fail("Vertex " + std::to_string(change.first) + " doesn't exist.");
		if (change.second.IsZero(true))
			continue;

		// Rest-space diffs, which undo applies the same way whatever the slider value and pose
		uss.restDiffs[change.first] = change.second;
		updates[change.first] = Mesh::TransformDiffNifToMesh(change.second);
	}

	if (updates.empty())
		return result;

	project.UpdateMorphResult(shape, sliderName, updates);

	usp.undoType = UndoType::VertexPosition;
	usp.sliderName = sliderName;
	usp.sliderscale = 1.0f;
	usp.usss.push_back(std::move(uss));

	result.refresh = RefreshFlags::PendingChanges | RefreshFlags::SliderData | RefreshFlags::Meshes;
	return result;
}

OpResult ProjectOperations::RigidifySlider(const std::vector<NiShape*>& shapes, const std::string& sliderName, const RigidifyOptions& options, UndoStateProject& usp, size_t* outParts) {
	size_t sliderIndex = 0;
	if (!project.SliderIndexFromName(sliderName, sliderIndex))
		return OpResult::Fail("Slider '" + sliderName + "' doesn't exist.");
	if (project.SliderZap(sliderIndex) || project.SliderUV(sliderIndex))
		return OpResult::Fail("Slider '" + sliderName + "' is a zap or UV slider, it has no morph to rigidify.");

	// The reference with all sliders at 0 and with only this slider, to keep the parts outside it
	NiShape* refShape = project.GetBaseShape();
	const bool keepOutside = options.clearance >= 0.0f && refShape;
	std::vector<Vector3> restBody, sliderBody;
	std::vector<Triangle> bodyTris;
	if (keepOutside) {
		std::vector<float> savedValues;
		for (size_t i = 0; i < project.SliderCount(); i++) {
			savedValues.push_back(project.SliderValue(i));
			project.SliderValue(i) = 0.0f;
		}
		project.GetMorphedVerts(refShape, restBody);
		project.SliderValue(sliderIndex) = 1.0f;
		project.GetMorphedVerts(refShape, sliderBody);
		for (size_t i = 0; i < savedValues.size(); i++)
			project.SliderValue(i) = savedValues[i];
		refShape->GetTriangles(bodyTris);
	}

	OpResult result;
	size_t partCount = 0;
	for (NiShape* shape : shapes) {
		if (!shape)
			continue;
		if (project.IsBaseShape(shape))
			return OpResult::Fail("The reference shape isn't rigidified.");

		std::vector<Vector3> deltas;
		OpResult got = GetSliderDeltas(shape, sliderName, deltas);
		if (!got.ok)
			return got;

		std::vector<Vector3> verts;
		project.GetWorkNif()->GetVertsForShape(shape, verts);
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		if (verts.size() != deltas.size())
			continue;

		// Masked vertices keep their deltas, partly masked ones blend
		std::vector<float> influence = MaskOps::FixWeights(GetMesh(shape->name.get()));
		if (influence.size() != verts.size())
			influence.assign(verts.size(), 1.0f);

		std::vector<bool> include(verts.size());
		for (size_t v = 0; v < verts.size(); v++)
			include[v] = influence[v] > 0.0f;

		std::vector<int> parts(verts.size(), -1);
		if (options.perPart) {
			int count = 0;
			parts = MeshAnalysis::ConnectedParts(verts, tris, options.weldDistance, &include, &count);
			partCount += count;
		}
		else {
			bool any = false;
			for (size_t v = 0; v < verts.size(); v++) {
				if (include[v]) {
					parts[v] = 0;
					any = true;
				}
			}
			partCount += any ? 1 : 0;
		}

		std::vector<Vector3> rigid = MeshAnalysis::RigidifyDeltas(verts, deltas, parts, influence, options.allowScale);
		if (keepOutside)
			KeepPartsOutside(verts, parts, influence, restBody, sliderBody, bodyTris, options.clearance, rigid);

		std::unordered_map<uint16_t, Vector3> changes;
		for (size_t v = 0; v < verts.size(); v++) {
			Vector3 change = rigid[v] - deltas[v];
			if (!change.IsZero(true))
				changes[static_cast<uint16_t>(v)] = change;
		}

		OpResult changed = ChangeSliderDeltas(shape, sliderName, changes, usp);
		result.refresh |= changed.refresh;
		if (!changed.ok) {
			changed.refresh = result.refresh;
			return changed;
		}
	}

	if (outParts)
		*outParts = partCount;
	return result;
}

OpResult ProjectOperations::CopyBoneWeights(const std::vector<NiShape*>& shapes, const BoneWeightCopyOptions& options, UndoStateProject& usp) {
	NiShape* baseShape = project.GetBaseShape();
	if (!baseShape)
		return OpResult::Fail("No reference shape is loaded.");

	IProjectHost& host = project.GetHost();
	AnimInfo& workAnim = *project.GetWorkAnim();
	usp.undoType = UndoType::Weight;

	std::vector<std::string> baseBones;
	if (!options.selectedBones.empty())
		baseBones = options.selectedBones;
	else
		baseBones = workAnim.shapeBones[baseShape->name.get()];

	std::sort(baseBones.begin(), baseBones.end());

	int nCopyBones = static_cast<int>(baseBones.size());
	std::vector<std::string> lockedBones;
	bool bSpreadWeight = false;

	// When copying a subset of bones, compute normalization info
	if (!options.selectedBones.empty()) {
		std::unordered_set<std::string> selBones{baseBones.begin(), baseBones.end()};
		std::vector<std::string> normBones, notNormBones;
		GetNormalizeBones(&normBones, &notNormBones);

		for (auto& bone : normBones)
			if (!selBones.count(bone))
				baseBones.push_back(bone);

		bSpreadWeight = static_cast<int>(baseBones.size()) > nCopyBones;

		if (bSpreadWeight) {
			for (auto& bone : notNormBones)
				if (!selBones.count(bone))
					lockedBones.push_back(bone);
		}
		else {
			for (auto& bone : notNormBones)
				if (!selBones.count(bone))
					baseBones.push_back(bone);
		}
	}

	OpResult result;
	const int inc = shapes.empty() ? 0 : 100 / static_cast<int>(shapes.size()) - 1;

	for (size_t i = 0; i < shapes.size(); i++) {
		NiShape* shape = shapes[i];
		if (project.IsBaseShape(shape)) {
			wxLogWarning("Shape '%s' is the reference shape, its weights are not copied.", shape->name.get());
			continue;
		}

		wxLogMessage("Copying bone weights to '%s'...", shape->name.get());
		host.StartSubProgress(static_cast<int>(i) * inc, static_cast<int>(i) * inc + inc);

		if (options.copySkinTransform) {
			MatTransform globalToBaseShape = workAnim.GetTransformGlobalToShape(baseShape);
			MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);

			if (options.transformGeometry && !globalToBaseShape.IsNearlyEqualTo(globalToShape)) {
				MatTransform shapeToBaseShape = globalToBaseShape.ComposeTransforms(globalToShape.InverseTransform());
				project.ApplyTransformToShapeGeometry(shape, shapeToBaseShape);
			}

			workAnim.SetTransformGlobalToShape(shape, globalToBaseShape);
			result.refresh |= RefreshFlags::ShapeList;
		}

		usp.usss.resize(usp.usss.size() + 1);
		usp.usss.back().shapeName = shape->name.get();

		std::unordered_map<uint16_t, float> mask;
		if (Mesh* m = GetMesh(shape->name.get()))
			mask = MaskOps::GetMasked(*m);

		std::vector<std::string> mergedBones = baseBones;

		// For full copy, also add shape-specific bones not already in baseBones
		if (options.selectedBones.empty()) {
			std::vector<std::string> bones = workAnim.shapeBones[shape->name.get()];
			for (auto& b : bones)
				if (!std::binary_search(baseBones.begin(), baseBones.end(), b))
					mergedBones.push_back(b);
		}

		const std::vector<std::string> bonesBefore = workAnim.shapeBones[shape->name.get()];
		project.CopyBoneWeights(shape, options.proximityRadius, options.maxResults, mask, mergedBones, nCopyBones, lockedBones, usp.usss.back(), bSpreadWeight);
		for (auto& bw : usp.usss.back().boneWeights)
			bw.addedBone = std::find(bonesBefore.begin(), bonesBefore.end(), bw.boneName) == bonesBefore.end();
		host.EndProgress();
	}

	if (!usp.usss.empty())
		result.refresh |= ApplyShapeEdits(usp, false);

	project.morpher.ClearProximityCache();
	workAnim.CleanupBones();
	result.refresh |= RefreshFlags::Anim;
	return result;
}

RefreshFlags ProjectOperations::ZeroSliders() {
	if (project.AllSlidersZero())
		return RefreshFlags::None;

	for (size_t s = 0; s < project.SliderCount(); s++) {
		if (project.SliderClamp(s))
			continue;

		project.SliderValue(s) = 0.0f;
	}

	return RefreshFlags::SliderValues | RefreshFlags::Meshes;
}

std::map<std::string, float> ProjectOperations::GetPresetValues(PresetCollection& presets, const std::string& preset, bool high) {
	std::map<std::string, float> values;
	for (size_t i = 0; i < project.SliderCount(); i++) {
		if (project.SliderClamp(i))
			continue;

		const std::string name = project.GetSliderName(i);
		float value = 0.0f;
		bool found = high ? presets.GetBigPreset(preset, name, value) : presets.GetSmallPreset(preset, name, value);

		if (!found) {
			// Zaps without a value in the preset are cleared so they don't show up as protruding shapes
			if (project.SliderZap(i)) {
				values[name] = 0.0f;
				continue;
			}

			// Sliders without a value in the preset fall back to their default for the chosen weight
			value = project.SliderDefault(i, high) / 100.0f;
		}

		if (project.SliderInvert(i))
			value = 1.0f - value;

		values[name] = value;
	}

	return values;
}

OpResult ProjectOperations::SetSliderValues(const std::map<std::string, float>& values) {
	for (auto& value : values)
		if (!project.ValidSlider(value.first))
			return OpResult::Fail("Slider '" + value.first + "' doesn't exist.");

	for (auto& value : values)
		project.SliderValue(value.first) = value.second;

	OpResult result;
	result.refresh = RefreshFlags::SliderValues | RefreshFlags::Meshes;
	return result;
}

OpResult ProjectOperations::ConformSliders(const std::vector<NiShape*>& shapes, const ConformOptions& options) {
	if (!project.GetBaseShape())
		return OpResult::Fail("No reference shape is loaded.");

	IProjectHost& host = project.GetHost();

	OpResult result;
	result.refresh = ZeroSliders();

	project.InitConform();

	const int inc = shapes.empty() ? 0 : 100 / static_cast<int>(shapes.size()) - 1;
	for (size_t i = 0; i < shapes.size(); i++) {
		NiShape* shape = shapes[i];
		if (project.IsBaseShape(shape))
			continue;

		std::string shapeName = shape->name.get();
		wxLogMessage("Conforming '%s'...", shapeName);
		host.UpdateProgress(static_cast<int>(i) * inc, _("Conforming: ") + shapeName);
		host.StartSubProgress(static_cast<int>(i) * inc, static_cast<int>(i) * inc + inc);

		if (Mesh* m = GetMesh(shapeName))
			project.morpher.CopyMeshMask(m, shapeName);

		project.ConformShape(shape, options);
		host.EndProgress();
	}

	project.morpher.ClearProximityCache();
	project.morpher.UnlinkRefDiffData();

	result.refresh |= RefreshFlags::SliderData | RefreshFlags::PendingChanges;
	return result;
}

OpResult ProjectOperations::FixClipping(const std::vector<NiShape*>& shapes, ClippingFixMode mode, float strength, const std::vector<std::string>& sliderNames) {
	NiShape* refShape = project.GetBaseShape();
	if (!refShape)
		return OpResult::Fail("No reference shape is set.");

	ClippingFixOptions options;
	options.strength = std::max(0.0f, std::min(1.0f, strength));
	options.fixPokeThrough = true;

	OpResult result;
	if (options.strength <= 0.0f) {
		wxLogWarning("Fix clipping: strength is 0, nothing to do.");
		return result;
	}

	// How much of the fix each vertex takes: none where masked, all where unmasked, and a blend
	// on a soft mask border
	auto fixWeights = [&](NiShape* shape) { return MaskOps::FixWeights(GetMesh(shape->name.get())); };

	if (mode == ClippingFixMode::Shapes) {
		// Fix the base geometry of the shapes, with no sliders applied
		std::vector<Vector3> bodyVerts;
		std::vector<Triangle> bodyTris;
		project.GetWorkNif()->GetVertsForShape(refShape, bodyVerts);
		refShape->GetTriangles(bodyTris);

		for (auto* shape : shapes) {
			if (project.IsBaseShape(shape))
				continue;

			if (!ClippingFixer::IsEligibleForFix(*project.GetWorkNif(), shape))
				continue;

			std::vector<Vector3> outfitVerts;
			project.GetWorkNif()->GetVertsForShape(shape, outfitVerts);

			std::vector<Triangle> outfitTris;
			shape->GetTriangles(outfitTris);

			std::vector<Vector3> fixedVerts = outfitVerts;
			ClippingFixer::FixClipping(bodyVerts, bodyTris, fixedVerts, outfitTris, options);

			std::vector<float> weights = fixWeights(shape);
			for (size_t i = 0; i < weights.size() && i < fixedVerts.size(); i++)
				fixedVerts[i] = outfitVerts[i] + (fixedVerts[i] - outfitVerts[i]) * weights[i];

			bool changed = false;
			for (size_t i = 0; i < outfitVerts.size(); i++) {
				if (!(fixedVerts[i] - outfitVerts[i]).IsZero(true)) {
					changed = true;
					break;
				}
			}

			if (changed) {
				wxLogMessage("Fix clipping: fixed the base geometry of '%s'.", shape->name.get());
				project.GetWorkNif()->SetVertsForShape(shape, fixedVerts);
				result.refresh |= RefreshFlags::Meshes | RefreshFlags::PendingChanges;
			}
		}

		return result;
	}

	// Sliders mode: fix the clipping of each slider on its own
	std::vector<size_t> sliderIndices;
	if (sliderNames.empty()) {
		for (size_t i = 0; i < project.SliderCount(); i++) {
			if (project.activeSet[i].bZap || project.activeSet[i].bUV)
				continue;
			sliderIndices.push_back(i);
		}
	}
	else {
		for (const auto& name : sliderNames) {
			size_t idx;
			if (!project.SliderIndexFromName(name, idx))
				return OpResult::Fail("Slider '" + name + "' doesn't exist.");
			sliderIndices.push_back(idx);
		}
	}

	for (size_t si : sliderIndices) {
		std::string sliderName = project.GetSliderName(si);
		wxLogMessage("Fix clipping: processing slider '%s'...", sliderName);

		for (auto* shape : shapes) {
			if (project.IsBaseShape(shape))
				continue;

			TargetDataDiffs* diffSet = project.GetDiffSet(project.activeSet[si], shape);
			if (!diffSet || diffSet->empty())
				continue;

			std::vector<float> weights = fixWeights(shape);
			std::unordered_set<uint16_t> allowed;
			for (size_t i = 0; i < weights.size(); i++)
				if (weights[i] > 0.0f)
					allowed.insert(static_cast<uint16_t>(i));

			if (!weights.empty() && allowed.empty())
				continue;

			// Applies only this slider while calculating, other slider values don't matter
			TargetDataDiffs morphDiffs;
			project.CalcSliderClippingCorrection(shape, sliderName, options.strength, morphDiffs, weights.empty() ? nullptr : &allowed);

			for (auto& diff : morphDiffs)
				if (diff.first < weights.size())
					diff.second *= weights[diff.first];

			if (!morphDiffs.empty()) {
				wxLogMessage("Fix clipping: updated %zu vertices of '%s' for slider '%s'.", morphDiffs.size(), shape->name.get(), sliderName);
				project.UpdateMorphResult(shape, sliderName, morphDiffs);
				result.refresh |= RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
			}
		}
	}

	return result;
}

OpResult ProjectOperations::FixClippingCombination(const std::vector<NiShape*>& shapes,
													 const std::map<std::string, float>& values,
													 const std::vector<std::string>& receivers,
													 float strength,
													 size_t* outChanged) {
	NiShape* refShape = project.GetBaseShape();
	if (!refShape)
		return OpResult::Fail("No reference shape is set.");
	if (values.empty())
		return OpResult::Fail("The combination has no slider values.");

	for (auto& value : values)
		if (!project.ValidSlider(value.first))
			return OpResult::Fail("Slider '" + value.first + "' doesn't exist.");

	// The sliders taking the fix: they need a value in the combination to divide by
	std::vector<std::string> targets = receivers;
	if (targets.empty()) {
		for (auto& value : values) {
			size_t index = 0;
			project.SliderIndexFromName(value.first, index);
			if (value.second != 0.0f && !project.SliderZap(index) && !project.SliderUV(index))
				targets.push_back(value.first);
		}
	}
	for (auto& target : targets) {
		size_t index = 0;
		if (!project.SliderIndexFromName(target, index))
			return OpResult::Fail("Slider '" + target + "' doesn't exist.");
		if (project.SliderZap(index) || project.SliderUV(index))
			return OpResult::Fail("Slider '" + target + "' is a zap or UV slider, it has no morph to take the fix.");
		auto value = values.find(target);
		if (value == values.end() || value->second == 0.0f)
			return OpResult::Fail("Slider '" + target + "' needs a value other than 0 in the combination to take a part of the fix.");
	}
	if (targets.empty())
		return OpResult::Fail("No slider of the combination can take the fix.");

	ClippingFixOptions options;
	options.strength = std::clamp(strength, 0.0f, 1.0f);
	options.fixPokeThrough = true;

	OpResult result;
	if (options.strength <= 0.0f)
		return result;

	// Measured unposed with only the combination, like the sliders mode measures each slider on its own
	std::vector<float> savedValues;
	for (size_t i = 0; i < project.SliderCount(); i++) {
		savedValues.push_back(project.SliderValue(i));
		project.SliderValue(i) = 0.0f;
	}
	// Zaps hide vertices rather than move them, so they stay off
	for (auto& value : values) {
		size_t index = 0;
		project.SliderIndexFromName(value.first, index);
		if (!project.SliderZap(index))
			project.SliderValue(value.first) = value.second;
	}

	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	project.GetMorphedVerts(refShape, bodyVerts);
	refShape->GetTriangles(bodyTris);

	size_t changed = 0;
	for (auto* shape : shapes) {
		if (project.IsBaseShape(shape) || !ClippingFixer::IsEligibleForFix(*project.GetWorkNif(), shape))
			continue;

		std::vector<Vector3> live;
		project.GetMorphedVerts(shape, live);
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);

		std::vector<Vector3> fixed = live;
		ClippingFixer::FixClipping(bodyVerts, bodyTris, fixed, tris, options);
		std::vector<float> weights = MaskOps::FixWeights(GetMesh(shape->name.get()));

		std::map<std::string, std::vector<Vector3>> deltas;
		for (auto& target : targets)
			GetSliderDeltas(shape, target, deltas[target]);

		std::map<std::string, TargetDataDiffs> updates;
		for (size_t v = 0; v < live.size() && v < fixed.size(); v++) {
			Vector3 correction = fixed[v] - live[v];
			if (v < weights.size())
				correction *= weights[v];
			if (correction.IsZero(true))
				continue;

			// Each slider takes the part of the push that matches how much it moves the vertex
			std::map<std::string, float> share;
			float sum = 0.0f;
			for (auto& target : targets) {
				float s = std::fabs(values.at(target)) * (v < deltas[target].size() ? deltas[target][v].length() : 0.0f);
				share[target] = s;
				sum += s;
			}
			if (sum <= 1e-6f) {
				// None of them moves it, so the slider with the largest value takes it all
				auto largest = std::max_element(targets.begin(), targets.end(), [&](const auto& a, const auto& b) { return std::fabs(values.at(a)) < std::fabs(values.at(b)); });
				share[*largest] = 1.0f;
				sum = 1.0f;
			}

			for (auto& target : targets) {
				if (share[target] <= 0.0f)
					continue;
				// A slider set low in the combination would need its morph changed many times over to take its part,
				// which shows as a spike when it's set high on its own: it takes at most twice its part
				const float value = values.at(target);
				const float divisor = std::copysign(std::max(std::fabs(value), 0.5f), value);
				Vector3 part = correction * (share[target] / sum) / divisor;
				updates[target][static_cast<uint16_t>(v)] = Mesh::TransformDiffNifToMesh(part);
			}
			changed++;
		}

		for (auto& update : updates) {
			if (update.second.empty())
				continue;
			wxLogMessage("Fix clipping: updated %zu vertices of '%s' for slider '%s' in the combination.", update.second.size(), shape->name.get(), update.first);
			project.UpdateMorphResult(shape, update.first, update.second);
			result.refresh |= RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
		}
	}

	for (size_t i = 0; i < savedValues.size() && i < project.SliderCount(); i++)
		project.SliderValue(i) = savedValues[i];

	if (outChanged)
		*outChanged = changed;
	return result;
}

OpResult ProjectOperations::GetSliderDeltas(NiShape* shape, const std::string& sliderName, std::vector<Vector3>& outDeltas) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");
	if (!project.ValidSlider(sliderName))
		return OpResult::Fail("Slider '" + sliderName + "' doesn't exist.");

	outDeltas.assign(shape->GetNumVertices(), Vector3());
	project.GetSliderDiff(shape, sliderName, outDeltas);
	return OpResult();
}

OpResult ProjectOperations::SetSliderDeltas(NiShape* shape, const std::string& sliderName, const std::unordered_map<uint16_t, Vector3>& deltas, bool add) {
	std::vector<Vector3> current;
	OpResult result = GetSliderDeltas(shape, sliderName, current);
	if (!result.ok)
		return result;

	std::unordered_map<uint16_t, Vector3> updates;
	for (auto& delta : deltas) {
		if (delta.first >= current.size())
			return OpResult::Fail("Vertex " + std::to_string(delta.first) + " doesn't exist.");

		Vector3 change = add ? delta.second : delta.second - current[delta.first];
		if (!change.IsZero(true))
			updates[delta.first] = Mesh::TransformDiffNifToMesh(change);
	}

	if (updates.empty())
		return result;

	project.UpdateMorphResult(shape, sliderName, updates);
	result.refresh = RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
	return result;
}

OpResult ProjectOperations::ScaleSliderDeltas(NiShape* shape, const std::string& sliderName, float factor) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");
	if (!project.ValidSlider(sliderName))
		return OpResult::Fail("Slider '" + sliderName + "' doesn't exist.");

	project.ScaleMorphResult(shape, sliderName, factor);

	OpResult result;
	result.refresh = RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
	return result;
}

OpResult ProjectOperations::SmoothSliderDeltas(NiShape* shape, const std::string& sliderName, int iterations, float strength) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");
	if (project.IsBaseShape(shape))
		return OpResult::Fail("The morphs of the reference shape can't be smoothed.");
	if (!project.ValidSlider(sliderName))
		return OpResult::Fail("Slider '" + sliderName + "' doesn't exist.");

	OpResult result;
	if (!project.SmoothSliderDiff(shape, sliderName, iterations, strength))
		return result;
	result.refresh = RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
	return result;
}

std::unique_ptr<EditSnapshot> ProjectOperations::CaptureSnapshot() {
	auto snapshot = std::make_unique<EditSnapshot>();
	snapshot->project = project.CaptureSnapshot();

	for (auto& shapeName : project.GetWorkNif()->GetShapeNames())
		if (Mesh* m = GetMesh(shapeName))
			snapshot->masks[shapeName] = MaskOps::GetMask(*m);

	return snapshot;
}

RefreshFlags ProjectOperations::RestoreSnapshot(const EditSnapshot& snapshot) {
	if (!snapshot.project)
		return RefreshFlags::None;

	project.RestoreSnapshot(*snapshot.project);

	// Slider edit mode refers to a slider that might not exist anymore
	editor.activeSlider.clear();
	editor.bEditSlider = false;

	return RefreshFlags::ShapeList | RefreshFlags::SliderSet | RefreshFlags::SliderValues | RefreshFlags::Meshes | RefreshFlags::SliderData
		| RefreshFlags::Anim | RefreshFlags::PendingChanges;
}

void ProjectOperations::RestoreMasks(const EditSnapshot& snapshot) {
	for (auto& mask : snapshot.masks)
		if (Mesh* m = GetMesh(mask.first))
			MaskOps::SetMask(*m, mask.second);
}

RefreshFlags ProjectOperations::SetPreviewPose(const PoseData& pose) {
	pose.ApplyToSkeleton();
	project.bPose = true;
	return RefreshFlags::Pose | RefreshFlags::Meshes;
}

RefreshFlags ProjectOperations::ClearPreviewPose() {
	project.bPose = false;
	return RefreshFlags::Pose | RefreshFlags::Meshes;
}

void ProjectOperations::ResetPose() {
	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const std::string& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone || bone->IsUnposed())
			continue;

		bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->poseScale = 1.0f;
		bone->UpdatePoseTransform();
	}
}

WeightBrushBones ProjectOperations::GetWeightBrushBones(const std::string& xMirrorBone) {
	WeightBrushBones bones;

	std::vector<std::string> normBones, notNormBones;
	GetNormalizeBones(&normBones, &notNormBones);

	const std::string& activeBone = editor.activeBone;
	bones.boneNames.push_back(activeBone);

	if (!xMirrorBone.empty()) {
		bones.boneNames.push_back(xMirrorBone);
		bones.xMirrorBone = true;
	}

	for (auto& bone : normBones) {
		if (bone != activeBone && bone != xMirrorBone) {
			bones.boneNames.push_back(bone);
			bones.spreadWeight = true;
		}
	}

	for (auto& bone : notNormBones) {
		if (bone == activeBone || bone == xMirrorBone)
			continue;

		if (bones.spreadWeight)
			bones.lockedBoneNames.push_back(bone);
		else
			bones.boneNames.push_back(bone);
	}

	return bones;
}

void ProjectOperations::UpdateShapeFromMesh(NiShape* shape) {
	if (Mesh* m = GetMesh(shape->name.get())) {
		project.UpdateShapeFromMesh(shape, m);
		return;
	}

	// Without a mesh, bake the live (slider-applied) vertices into the shape instead
	std::vector<Vector3> liveVerts;
	project.GetLiveVerts(shape, liveVerts);
	if (!liveVerts.empty())
		project.GetWorkNif()->SetVertsForShape(shape, liveVerts);
}

OpResult ProjectOperations::AnalyzeSlider(NiShape* shape, const std::string& sliderName, MeshAnalysis::MorphStats& outStats) {
	std::vector<Vector3> deltas;
	OpResult result = GetSliderDeltas(shape, sliderName, deltas);
	if (!result.ok)
		return result;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	outStats = MeshAnalysis::AnalyzeMorph(tris, deltas);
	return result;
}

OpResult ProjectOperations::AnalyzeClipping(NiShape* shape, MeshAnalysis::PenetrationStats& outStats, float nearDistance) {
	NiShape* baseShape = project.GetBaseShape();
	if (!baseShape)
		return OpResult::Fail("No reference shape is loaded.");
	if (!shape || shape == baseShape)
		return OpResult::Fail("Choose a shape other than the reference.");

	std::vector<Vector3> bodyVerts, outfitVerts;
	project.GetLiveVerts(baseShape, bodyVerts);
	project.GetLiveVerts(shape, outfitVerts);

	std::vector<Triangle> bodyTris;
	baseShape->GetTriangles(bodyTris);

	outStats = MeshAnalysis::AnalyzePenetration(outfitVerts, bodyVerts, bodyTris, nearDistance);
	return OpResult();
}

OpResult ProjectOperations::AnalyzeWeights(NiShape* shape, MeshAnalysis::WeightStats& outStats, int influenceLimit) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");

	AnimInfo& workAnim = *project.GetWorkAnim();
	std::string shapeName = shape->name.get();

	std::vector<const std::unordered_map<uint16_t, float>*> boneWeights;
	auto shapeBones = workAnim.shapeBones.find(shapeName);
	if (shapeBones != workAnim.shapeBones.end())
		for (auto& bone : shapeBones->second)
			boneWeights.push_back(workAnim.GetWeightsPtr(shapeName, bone));

	outStats = MeshAnalysis::AnalyzeWeights(shape->GetNumVertices(), boneWeights, influenceLimit);
	return OpResult();
}

void ProjectOperations::FillWeightColors(const std::string& bone, const std::vector<NiShape*>& shapes) {
	auto fillFromBone = [&](NiShape* shape) {
		Mesh* m = GetMesh(shape->name.get());
		if (!m)
			return;

		m->WeightFill(0.0f);
		if (auto weights = project.GetWorkAnim()->GetWeightsPtr(shape->name.get(), bone))
			for (auto& bw : *weights)
				m->weight[bw.first] = bw.second;

		m->QueueUpdate(Mesh::UpdateType::Weight);
	};

	for (auto& s : project.GetWorkNif()->GetShapes()) {
		if (Mesh* m = GetMesh(s->name.get())) {
			m->WeightFill(0.0f);
			m->QueueUpdate(Mesh::UpdateType::Weight);
		}
	}

	if (bone.empty())
		return;

	for (auto* shape : shapes)
		if (!project.IsBaseShape(shape))
			fillFromBone(shape);

	// The weights of the reference are always shown, for comparison
	if (NiShape* baseShape = project.GetBaseShape())
		fillFromBone(baseShape);
}

OpResult ProjectOperations::LoadReference(const ReferenceSource& source) {
	if (source.file.empty())
		return OpResult::Fail("No reference file was given.");

	wxString ext = wxFileName(wxString::FromUTF8(source.file)).GetExt().Lower();

	int err = 0;
	if (ext == "nif")
		err = project.LoadReferenceNif(source.file, source.shape, source.mergeSliders, source.mergeZaps);
	else if (!source.set.empty())
		err = project.LoadReference(source.file, source.set, source.shape, source.mergeSliders, source.mergeZaps, source.appendNewSliders);
	else
		err = project.LoadReferenceTemplate(source.file, source.set, source.shape, source.loadAll, source.mergeSliders, source.mergeZaps, source.appendNewSliders);

	if (err)
		return OpResult::Fail("Loading the reference failed with error " + std::to_string(err) + ".");

	project.SetTextures();

	OpResult result;
	result.refresh = RefreshFlags::ShapeList | RefreshFlags::SliderSet | RefreshFlags::SliderValues | RefreshFlags::Meshes | RefreshFlags::Anim
		| RefreshFlags::PendingChanges;
	return result;
}

OpResult ProjectOperations::AddProject(const std::string& file, const std::string& set, bool appendNewSliders) {
	int err = project.AddFromSliderSet(file, set, false, appendNewSliders);
	if (err)
		return OpResult::Fail("Adding the project failed with error " + std::to_string(err) + ".");

	project.SetTextures();

	OpResult result;
	result.refresh = RefreshFlags::ShapeList | RefreshFlags::SliderSet | RefreshFlags::SliderValues | RefreshFlags::Meshes | RefreshFlags::Anim
		| RefreshFlags::PendingChanges;
	return result;
}

OpResult ProjectOperations::SaveProject(const SaveProjectOptions& options) {
	if (options.sliderSetFile.empty() || options.outputFileName.empty())
		return OpResult::Fail("A slider set file and an output file name are needed to save.");

	if (!options.sfMorphTargetShape.empty() && !project.GetWorkNif()->FindBlockByName<NiShape>(options.sfMorphTargetShape))
		wxLogWarning("Starfield morph target shape '%s' not found in the project.", options.sfMorphTargetShape);

	std::string error = project.Save(wxFileName(wxString::FromUTF8(options.sliderSetFile)),
									 wxString::FromUTF8(options.displayName),
									 wxString::FromUTF8(options.shapeDataFolder),
									 wxString::FromUTF8(options.shapeDataFile),
									 wxString::FromUTF8(options.outputDataPath),
									 wxString::FromUTF8(options.outputFileName),
									 options.genWeights,
									 options.copyReference,
									 project.bPreventMorphFile,
									 project.bKeepZappedShapes,
									 wxString::FromUTF8(options.sfMorphPath),
									 wxString::FromUTF8(options.sfMorphTargetShape));

	if (!error.empty())
		return OpResult::Fail(error);

	return OpResult();
}

OpResult ProjectOperations::ExportFile(const std::string& fileName, const ExportOptions& options) {
	wxString ext = wxFileName(wxString::FromUTF8(fileName)).GetExt().Lower();

	std::vector<NiShape*> shapes;
	for (auto* s : project.GetWorkNif()->GetShapes()) {
		if (!options.shapes.empty()) {
			if (std::find(options.shapes.begin(), options.shapes.end(), s->name.get()) != options.shapes.end())
				shapes.push_back(s);
		}
		else if (options.withReference || !project.IsBaseShape(s))
			shapes.push_back(s);
	}

	if (shapes.size() < options.shapes.size())
		return OpResult::Fail("Some of the shapes to export don't exist.");

	int err = 0;
	if (ext == "nif" && !options.shapes.empty()) {
		// Like Export Selected Shapes in the window
		err = project.ExportShapeNIF(fileName, options.shapes);
	}
	else if (ext == "nif") {
		// Writes the live mesh positions and normals, the same as the window
		std::vector<Mesh*> shapeMeshes;
		for (auto* s : shapes)
			if (Mesh* m = GetMesh(s->name.get()))
				shapeMeshes.push_back(m);

		err = project.ExportNIF(fileName, shapeMeshes, options.withReference);
	}
	else if (ext == "obj")
		err = project.ExportOBJ(fileName, shapes, options.global);
	else if (ext == "fbx") {
#ifdef USE_FBXSDK
		err = project.ExportFBX(fileName, shapes, options.global);
#else
		return OpResult::Fail("FBX export isn't available in this build.");
#endif
	}
	else if (ext == "osd")
		err = project.SaveSliderData(fileName) ? 0 : 1;
	else if (ext == "tri" && options.headTri) {
		if (shapes.size() != 1)
			return OpResult::Fail("A head TRI file is written for one shape.");
		err = project.WriteHeadTRI(shapes.front(), fileName) ? 0 : 1;
	}
	else if (ext == "tri")
		err = project.WriteMorphTRI(fileName) ? 0 : 1;
	else
		return OpResult::Fail("Files with the extension '" + std::string(ext.ToUTF8().data()) + "' can't be exported.");

	if (err)
		return OpResult::Fail("Exporting '" + fileName + "' failed with error " + std::to_string(err) + ".");

	return OpResult();
}

OpResult ProjectOperations::ShowHeatmap(NiShape* shape, const std::vector<float>& values, float minValue, float maxValue) {
	if (!shape)
		return OpResult::Fail("Shape doesn't exist.");

	Mesh* m = GetMesh(shape->name.get());
	if (!m || !m->vcolors)
		return OpResult::Fail("The shape has no mesh to color.");
	if (static_cast<int>(values.size()) != m->nVerts)
		return OpResult::Fail("There has to be one value per vertex.");

	const float range = maxValue - minValue;
	for (int i = 0; i < m->nVerts; i++) {
		if (values[i] == FLT_MAX)
			m->vcolors[i] = Vector3(0.5f, 0.5f, 0.5f);
		else
			m->vcolors[i] = MeshAnalysis::HeatmapColor(range > 0.0f ? (values[i] - minValue) / range : 0.0f);

		if (m->valpha)
			m->valpha[i] = 1.0f;
	}

	m->overrideVertexColors = true;
	m->QueueUpdate(Mesh::UpdateType::VertexColors);
	m->QueueUpdate(Mesh::UpdateType::VertexAlpha);

	OpResult result;
	result.refresh = RefreshFlags::Shaders;
	return result;
}

OpResult ProjectOperations::CompareToProject(const std::string& file,
											 const std::string& set,
											 const std::map<std::string, std::string>& shapeMap,
											 ProjectComparison& outComparison) {
	outComparison = ProjectComparison();

	// Unattended and without touching the shared skeleton, so the open project isn't affected
	OutfitProject other(nullptr, false);

	// A project from another BodySlide folder has its ShapeData next to its SliderSets
	std::string shapeDataPath;
	wxFileName fileName(wxString::FromUTF8(file));
	if (fileName.GetDirCount() > 0 && fileName.GetDirs().Last().IsSameAs("SliderSets", false)) {
		fileName.RemoveLastDir();
		fileName.AppendDir("ShapeData");
		shapeDataPath = fileName.GetPath().ToUTF8().data();
	}

	if (int err = other.LoadFromSliderSet(file, set, nullptr, shapeDataPath))
		return OpResult::Fail("Loading '" + set + "' from '" + file + "' failed with error " + std::to_string(err) + ".");

	std::set<std::string> sliderNames;
	for (size_t i = 0; i < project.SliderCount(); i++)
		if (!project.SliderUV(i))
			sliderNames.insert(project.GetSliderName(i));
	for (size_t i = 0; i < other.SliderCount(); i++)
		if (!other.SliderUV(i))
			sliderNames.insert(other.GetSliderName(i));

	for (auto* shape : project.GetWorkNif()->GetShapes()) {
		if (project.IsBaseShape(shape))
			continue;

		std::string shapeName = shape->name.get();
		auto mapped = shapeMap.find(shapeName);
		std::string otherName = mapped != shapeMap.end() ? mapped->second : shapeName;

		NiShape* otherShape = other.GetWorkNif()->FindBlockByName<NiShape>(otherName);
		if (!otherShape) {
			outComparison.skippedShapes.emplace_back(shapeName, "no shape '" + otherName + "' in the other project");
			continue;
		}

		const size_t vertexCount = shape->GetNumVertices();
		if (otherShape->GetNumVertices() != vertexCount) {
			outComparison.skippedShapes.emplace_back(shapeName, "different vertex count");
			continue;
		}

		for (auto& sliderName : sliderNames) {
			std::vector<Vector3> here(vertexCount), there(vertexCount);
			bool hasHere = project.ValidSlider(sliderName);
			bool hasThere = other.ValidSlider(sliderName);
			if (hasHere)
				project.GetSliderDiff(shape, sliderName, here);
			if (hasThere)
				other.GetSliderDiff(otherShape, sliderName, there);

			ProjectComparison::SliderDifference difference;
			difference.shape = shapeName;
			difference.slider = sliderName;
			difference.onlyHere = !hasThere;
			difference.onlyThere = !hasHere;
			difference.deltas = MeshAnalysis::CompareDeltas(here, there);
			outComparison.sliders.push_back(difference);
		}

		AnimInfo& hereAnim = *project.GetWorkAnim();
		AnimInfo& thereAnim = *other.GetWorkAnim();
		std::set<std::string> bones;
		if (auto it = hereAnim.shapeBones.find(shapeName); it != hereAnim.shapeBones.end())
			bones.insert(it->second.begin(), it->second.end());
		if (auto it = thereAnim.shapeBones.find(otherName); it != thereAnim.shapeBones.end())
			bones.insert(it->second.begin(), it->second.end());

		for (auto& bone : bones) {
			auto hereWeights = hereAnim.GetWeightsPtr(shapeName, bone);
			auto thereWeights = thereAnim.GetWeightsPtr(otherName, bone);

			std::set<uint16_t> vertices;
			if (hereWeights)
				for (auto& w : *hereWeights)
					vertices.insert(w.first);
			if (thereWeights)
				for (auto& w : *thereWeights)
					vertices.insert(w.first);

			if (vertices.empty())
				continue;

			auto weightOf = [](const std::unordered_map<uint16_t, float>* weights, uint16_t v) {
				if (!weights)
					return 0.0f;
				auto it = weights->find(v);
				return it != weights->end() ? it->second : 0.0f;
			};

			ProjectComparison::WeightDifference difference;
			difference.shape = shapeName;
			difference.bone = bone;
			difference.vertices = vertices.size();

			double sum = 0.0;
			for (uint16_t v : vertices) {
				float error = std::fabs(weightOf(hereWeights, v) - weightOf(thereWeights, v));
				sum += error;
				difference.maxError = std::max(difference.maxError, error);
			}
			difference.meanError = static_cast<float>(sum / static_cast<double>(vertices.size()));
			outComparison.weights.push_back(difference);
		}
	}

	return OpResult();
}

ZapSource ProjectOperations::CaptureZaps(OutfitProject& fromProject, NiShape* shape, const std::vector<std::string>& sliderNames) {
	ZapSource source;
	if (!shape)
		return source;

	source.shapeName = shape->name.get();
	fromProject.GetWorkNif()->GetVertsForShape(shape, source.verts);
	shape->GetTriangles(source.tris);

	for (size_t i = 0; i < fromProject.SliderCount(); i++) {
		SliderData& slider = fromProject.activeSet[i];
		if (!slider.bZap)
			continue;
		if (!sliderNames.empty() && std::find(sliderNames.begin(), sliderNames.end(), slider.name) == sliderNames.end())
			continue;

		ZapSource::Zap zap;
		zap.zapped = fromProject.GetZappedVertices(shape, slider.name);
		if (std::find(zap.zapped.begin(), zap.zapped.end(), true) == zap.zapped.end())
			continue;

		zap.defSmallValue = slider.defSmallValue;
		zap.defBigValue = slider.defBigValue;
		zap.hidden = slider.bHidden;
		source.zaps[slider.name] = std::move(zap);
	}

	return source;
}

OpResult ProjectOperations::CaptureZapsFromProject(const std::string& file,
												   const std::string& set,
												   const std::string& shapeName,
												   const std::vector<std::string>& sliderNames,
												   ZapSource& outSource) {
	// Unattended and without touching the shared skeleton, so the open project isn't affected
	OutfitProject other(nullptr, false);

	std::string shapeDataPath;
	wxFileName fileName(wxString::FromUTF8(file));
	if (fileName.GetDirCount() > 0 && fileName.GetDirs().Last().IsSameAs("SliderSets", false)) {
		fileName.RemoveLastDir();
		fileName.AppendDir("ShapeData");
		shapeDataPath = fileName.GetPath().ToUTF8().data();
	}

	if (int err = other.LoadFromSliderSet(file, set, nullptr, shapeDataPath))
		return OpResult::Fail("Loading '" + set + "' from '" + file + "' failed with error " + std::to_string(err) + ".");

	NiShape* shape = other.GetWorkNif()->FindBlockByName<NiShape>(shapeName);
	if (!shape)
		shape = other.GetWorkNif()->FindBlockByName<NiShape>(other.activeSet.TargetToShape(shapeName));
	if (!shape)
		return OpResult::Fail("'" + set + "' has no shape or target '" + shapeName + "'.");

	for (auto& name : sliderNames) {
		size_t index = 0;
		if (!other.SliderIndexFromName(name, index) || !other.SliderZap(index))
			return OpResult::Fail("'" + set + "' has no zap slider '" + name + "'.");
	}

	outSource = CaptureZaps(other, shape, sliderNames);
	return OpResult();
}

OpResult ProjectOperations::ApplyZaps(const ZapSource& source, NiShape* target, float maxDistance, std::vector<ZapTransfer>& outTransfers) {
	outTransfers.clear();
	if (!target)
		return OpResult::Fail("Shape doesn't exist.");

	OpResult result;
	if (source.zaps.empty())
		return result;

	std::vector<Vector3> verts;
	project.GetWorkNif()->GetVertsForShape(target, verts);
	auto closest = MeshAnalysis::ClosestSurfacePoints(source.verts, source.tris, verts, maxDistance);

	// The source vertex each target vertex takes its zaps from: the corner of the closest triangle nearest to it
	std::vector<int> nearest(verts.size(), -1);
	for (size_t v = 0; v < verts.size(); v++) {
		const MeshAnalysis::SurfacePoint& sp = closest[v];
		if (sp.triangle < 0)
			continue;

		const Triangle& t = source.tris[sp.triangle];
		if (sp.u >= sp.v && sp.u >= sp.w)
			nearest[v] = t.p1;
		else if (sp.v >= sp.w)
			nearest[v] = t.p2;
		else
			nearest[v] = t.p3;
	}

	for (auto& zap : source.zaps) {
		ZapTransfer transfer;
		transfer.slider = zap.first;
		transfer.sourceZapped = static_cast<size_t>(std::count(zap.second.zapped.begin(), zap.second.zapped.end(), true));

		size_t index = 0;
		if (!project.SliderIndexFromName(zap.first, index)) {
			index = project.activeSet.CreateSlider(zap.first);
			project.activeSet[index].bZap = true;
			project.activeSet[index].defSmallValue = zap.second.defSmallValue;
			project.activeSet[index].defBigValue = zap.second.defBigValue;
			project.activeSet[index].bHidden = zap.second.hidden;
			transfer.created = true;
			result.refresh |= RefreshFlags::SliderSet;
		}
		else if (!project.SliderZap(index))
			return OpResult::Fail("Slider '" + zap.first + "' isn't a zap slider here.");

		std::vector<bool> zapped(verts.size(), false);
		for (size_t v = 0; v < verts.size(); v++) {
			if (nearest[v] >= 0 && static_cast<size_t>(nearest[v]) < zap.second.zapped.size() && zap.second.zapped[nearest[v]]) {
				zapped[v] = true;
				transfer.zapped++;
			}
		}

		project.SetZappedVertices(target, zap.first, zapped);
		outTransfers.push_back(transfer);
	}

	result.refresh |= RefreshFlags::SliderData | RefreshFlags::Meshes | RefreshFlags::PendingChanges;
	return result;
}
