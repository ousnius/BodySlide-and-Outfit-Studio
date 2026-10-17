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
					project.GetWorkAnim()->AddShapeBone(uss.shapeName, bw.boneName);
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

		project.CopyBoneWeights(shape, options.proximityRadius, options.maxResults, mask, mergedBones, nCopyBones, lockedBones, usp.usss.back(), bSpreadWeight);
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

	OpResult result;
	if (options.strength <= 0.0f) {
		wxLogWarning("Fix clipping: strength is 0, nothing to do.");
		return result;
	}

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

			// Applies only this slider while calculating, other slider values don't matter
			TargetDataDiffs morphDiffs;
			project.CalcSliderClippingCorrection(shape, sliderName, options.strength, morphDiffs);

			if (!morphDiffs.empty()) {
				wxLogMessage("Fix clipping: updated %zu vertices of '%s' for slider '%s'.", morphDiffs.size(), shape->name.get(), sliderName);
				project.UpdateMorphResult(shape, sliderName, morphDiffs);
				result.refresh |= RefreshFlags::Meshes | RefreshFlags::SliderData | RefreshFlags::PendingChanges;
			}
		}
	}

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
	for (auto* s : project.GetWorkNif()->GetShapes())
		if (options.withReference || !project.IsBaseShape(s))
			shapes.push_back(s);

	int err = 0;
	if (ext == "nif") {
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
