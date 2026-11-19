/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: bones, skin weights, bad bones and pose transforms

#include "OutfitProject.h"
#include "components/MaskOps.h"
#include "components/WeightNorm.h"
#include "NifUtil.hpp"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <filesystem>
#include <limits>
#include <regex>
#include <sstream>
#include <wx/log.h>

using namespace nifly;

size_t OutfitProject::GetActiveBoneCount() {
	return AnimSkeleton::getInstance().GetActiveBoneCount();
}

void OutfitProject::GetActiveBones(std::vector<std::string>& outBoneNames) {
	AnimSkeleton::getInstance().GetActiveBoneNames(outBoneNames);
}

void OutfitProject::CopyBoneWeights(NiShape* shape,
									const float proximityRadius,
									const int maxResults,
									std::unordered_map<uint16_t, float>& mask,
									const std::vector<std::string>& boneList,
									int nCopyBones,
									const std::vector<std::string>& lockedBones,
									UndoStateShape& uss,
									bool bSpreadWeight) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->name.get();
	std::string baseShapeName = baseShape->name.get();

	UpdateProgress(1, _("Gathering bones..."));

	int nBones = boneList.size();
	if (nBones <= 0 || nCopyBones <= 0) {
		UpdateProgress(90);
		return;
	}

	DiffDataSets dds;
	for (int bi = 0; bi < nCopyBones; ++bi) {
		const std::string& bone = boneList[bi];
		std::string wtSet = bone + "_WT_";
		dds.AddEmptySet(wtSet, "Weight");

		auto weights = workAnim.GetWeightsPtr(baseShapeName, bone);
		if (weights) {
			for (auto& w : *weights) {
				Vector3 tmp;
				tmp.y = w.second;
				dds.UpdateDiff(wtSet, "Weight", w.first, tmp);
			}
		}
	}

	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, &workAnim, shapeName, boneList, lockedBones, nCopyBones, bSpreadWeight);
	std::unordered_set<int> vertList;

	UpdateProgress(10, _("Initializing proximity data..."));

	InitConform();
	morpher.LinkRefDiffData(&dds);

	// Masked vertices of the reference aren't copied from (masked vertices of the shape keep their weights)
	std::unordered_map<uint16_t, float> refMask;
	if (Mesh* refMesh = host->GetShapeMesh(baseShapeName))
		refMask = MaskOps::GetMasked(*refMesh);

	std::set<uint16_t> refMaskIndices;
	for (auto& m : refMask)
		refMaskIndices.insert(m.first);

	morpher.BuildProximityCache(shapeName, proximityRadius, refMaskIndices.empty() ? nullptr : &refMaskIndices);
	CreateSkinning(shape);

	int step = 40 / nCopyBones;
	int prog = 40;
	UpdateProgress(prog);

	for (int bi = 0; bi < nCopyBones; ++bi) {
		const std::string& boneName = boneList[bi];
		uss.boneWeights[bi].skinToBone = workAnim.GetMissingBoneTransformForCopy(baseShapeName, shapeName, boneName);
		auto& ubw = uss.boneWeights[bi].weights;
		// Zero out unmasked weights
		auto weights = workAnim.GetWeightsPtr(shapeName, boneName);
		if (weights) {
			for (auto& pi : *weights) {
				if (mask[pi.first] > 0.0f)
					continue;
				if (vertList.find(pi.first) == vertList.end()) {
					vertList.insert(pi.first);
					nzer.GrabOneVertexStartingWeights(pi.first);
				}
				ubw[pi.first].endVal = 0.0;
			}
		}

		// Calculate new values for bone's weights
		std::string wtSet = boneName + "_WT_";
		morpher.GenerateResultDiff(shapeName, wtSet, wtSet, false, maxResults);

		std::unordered_map<uint16_t, Vector3> diffResult;
		morpher.GetRawResultDiff(shapeName, wtSet, diffResult);

		// Copy unmasked weights from diffResult into uss
		for (auto& dr : diffResult) {
			if (mask[dr.first] > 0.0f)
				continue;
			if (vertList.find(dr.first) == vertList.end()) {
				vertList.insert(dr.first);
				nzer.GrabOneVertexStartingWeights(dr.first);
			}
			ubw[dr.first].endVal = dr.second.y;
		}

		UpdateProgress(prog += step, _("Copying bone weights..."));
	}
	morpher.UnlinkRefDiffData();

	// Normalize
	for (auto vInd : vertList)
		nzer.AdjustWeights(vInd);

	UpdateProgress(90);
}

void OutfitProject::TransferSelectedWeights(NiShape* shape, std::unordered_map<uint16_t, float>* mask, std::vector<std::string>* inBoneList) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->name.get();
	std::string baseShapeName = baseShape->name.get();

	UpdateProgress(10, _("Gathering bones..."));

	std::vector<std::string>* boneList;
	std::vector<std::string> allBoneList;
	if (!inBoneList) {
		for (auto& boneName : workAnim.shapeBones[baseShapeName])
			allBoneList.push_back(boneName);

		boneList = &allBoneList;
	}
	else
		boneList = inBoneList;

	if (boneList->size() <= 0) {
		UpdateProgress(100, _("Finished"));
		return;
	}

	int step = 50 / boneList->size();
	int prog = 40;
	UpdateProgress(prog, _("Transferring bone weights..."));

	for (auto& boneName : *boneList) {
		std::unordered_map<uint16_t, float> weights;
		std::unordered_map<uint16_t, float> oldWeights;
		workAnim.GetWeights(baseShapeName, boneName, weights);
		workAnim.GetWeights(shapeName, boneName, oldWeights);

		for (auto& w : weights) {
			if (mask) {
				if (1.0f - (*mask)[w.first] > 0.0f)
					weights[w.first] = w.second * (1.0f - (*mask)[w.first]);
				else
					weights[w.first] = oldWeights[w.first];
			}
			else
				weights[w.first] = w.second;
		}

		workAnim.AddShapeBone(shapeName, boneName, workAnim.GetMissingBoneTransformForCopy(baseShapeName, shapeName, boneName));
		workAnim.SetWeights(shapeName, boneName, weights);
		UpdateProgress(prog += step, "");
	}

	UpdateProgress(100, _("Finished"));
}

bool OutfitProject::HasUnweighted(std::vector<std::string>* shapeNames) {
	bool hasUnweighted = false;

	for (auto& shape : workNif.GetShapes()) {
		if (!shape || !shape->IsSkinned())
			continue;

		std::string shapeName = shape->name.get();
		std::vector<Vector3> verts;
		workNif.GetVertsForShape(shape, verts);

		std::unordered_map<int, int> influences;
		for (size_t i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		if (workAnim.shapeBones.find(shapeName) != workAnim.shapeBones.end()) {
			for (auto& b : workAnim.shapeBones[shapeName]) {
				auto weights = workAnim.GetWeightsPtr(shapeName, b);
				if (weights) {
					for (size_t i = 0; i < verts.size(); i++) {
						auto id = weights->find(i);
						if (id != weights->end() && id->second > 0.0f)
							influences.at(i)++;
					}
				}
			}
		}

		bool shapeUnweighted = false;
		Mesh* m = host->GetShapeMesh(shapeName);
		if (m) {
			for (auto& i : influences) {
				if (i.second == 0) {
					if (!shapeUnweighted)
						m->MaskFill(0.0f);

					m->mask[i.first] = 1.0f;
					shapeUnweighted = true;
				}
			}

			if (shapeUnweighted) {
				hasUnweighted = true;

				if (shapeNames)
					shapeNames->push_back(shapeName);
			}

			m->QueueUpdate(Mesh::UpdateType::Mask);
		}
	}

	return hasUnweighted;
}

void OutfitProject::AddBoneRef(const std::string& boneName) {
	for (auto& s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const MatTransform& xformToParent) {
	AnimBone& customBone = AnimSkeleton::getInstance().AddCustomBone(boneName);
	customBone.SetTransformBoneToParent(xformToParent);
	// An empty name is no parent, not a bone that happens to have no name
	customBone.SetParentBone(parentBone.empty() ? nullptr : AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto& s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::ModifyCustomBone(AnimBone* bPtr, const std::string& parentBone, const MatTransform& xformToParent) {
	bPtr->SetTransformBoneToParent(xformToParent);
	bPtr->SetParentBone(parentBone.empty() ? nullptr : AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto& s : workNif.GetShapeNames())
		workAnim.RecursiveRecalcXFormSkinToBone(s, bPtr);
}

std::string OutfitProject::DuplicateBoneName(const std::string& boneName, const std::string& find, const std::string& replace, const std::string& suffix) {
	std::string newName = boneName;
	if (!find.empty()) {
		size_t pos = 0;
		while ((pos = newName.find(find, pos)) != std::string::npos) {
			newName.replace(pos, find.length(), replace);
			pos += replace.length();
		}
	}
	return newName + suffix;
}

std::vector<std::string> OutfitProject::DuplicateBoneNames(const std::vector<std::string>& boneNames,
														   const std::string& find,
														   const std::string& replace,
														   const std::string& suffix) {
	std::vector<std::string> newNames;
	newNames.reserve(boneNames.size());
	for (auto& bone : boneNames)
		newNames.push_back(DuplicateBoneName(bone, find, replace, suffix));
	return newNames;
}

std::vector<OutfitProject::DuplicateBoneConflict> OutfitProject::DuplicateBoneConflicts(const std::vector<std::string>& boneNames,
																						 const std::vector<std::string>& newNames) {
	AnimSkeleton& skel = AnimSkeleton::getInstance();
	const std::set<std::string> originals(boneNames.begin(), boneNames.end());

	std::vector<DuplicateBoneConflict> conflicts;
	conflicts.reserve(newNames.size());
	std::set<std::string> seen;
	for (auto& newName : newNames) {
		// A custom bone left in the skeleton from an earlier project can be reused, unless other bones hang from
		// it: it could be an ancestor of the bone it would copy
		const AnimBone* existing = newName.empty() ? nullptr : skel.GetBonePtr(newName);

		DuplicateBoneConflict conflict = DuplicateBoneConflict::None;
		if (newName.empty())
			conflict = DuplicateBoneConflict::EmptyName;
		else if (skel.GetBonePtr(newName, false))
			conflict = DuplicateBoneConflict::StandardBone;
		else if (originals.count(newName))
			conflict = DuplicateBoneConflict::Original;
		else if (skel.GetBoneRefCount(newName) > 0)
			conflict = DuplicateBoneConflict::InProject;
		else if (existing && !existing->children.empty())
			conflict = DuplicateBoneConflict::ParentBone;
		else if (!seen.insert(newName).second)
			conflict = DuplicateBoneConflict::Repeated;
		conflicts.push_back(conflict);
	}
	return conflicts;
}

std::string OutfitProject::DuplicateBoneConflictText(DuplicateBoneConflict conflict, const std::string& newName) {
	switch (conflict) {
		case DuplicateBoneConflict::EmptyName: return "A new bone name is empty.";
		case DuplicateBoneConflict::StandardBone: return "'" + newName + "' is a bone of the reference skeleton.";
		case DuplicateBoneConflict::Original: return "'" + newName + "' is one of the bones to duplicate.";
		case DuplicateBoneConflict::InProject: return "Bone '" + newName + "' already exists in the project.";
		case DuplicateBoneConflict::ParentBone: return "Bone '" + newName + "' already exists in the skeleton as the parent of other bones.";
		case DuplicateBoneConflict::Repeated: return "More than one bone would be named '" + newName + "'.";
		default: return std::string();
	}
}

std::string OutfitProject::NextFreeDuplicateSuffix(const std::vector<std::string>& boneNames, const std::string& find, const std::string& replace) {
	// No suffix when the replaced names are free already
	for (int n = 0;; ++n) {
		const std::string suffix = n == 0 ? std::string() : "." + std::to_string(n);
		const auto conflicts = DuplicateBoneConflicts(boneNames, DuplicateBoneNames(boneNames, find, replace, suffix));
		const bool free = std::all_of(conflicts.begin(), conflicts.end(), [](DuplicateBoneConflict c) { return c == DuplicateBoneConflict::None; });
		if (free || n >= 10000)
			return suffix;
	}
}

std::set<std::string> OutfitProject::DuplicateChainParents(const std::vector<std::string>& boneNames) {
	AnimSkeleton& skel = AnimSkeleton::getInstance();

	std::set<AnimBone*> bones;
	for (auto& name : boneNames)
		if (AnimBone* bPtr = skel.GetBonePtr(name))
			bones.insert(bPtr);

	std::set<std::string> parents;
	for (AnimBone* bPtr : bones)
		if (!bones.count(bPtr->parent))
			parents.insert(bPtr->parent ? bPtr->parent->boneName : std::string());
	return parents;
}

bool OutfitProject::DuplicateBones(const std::vector<std::string>& boneNames,
								   const std::string& find,
								   const std::string& replace,
								   const std::string& suffix,
								   const std::optional<std::string>& newParent,
								   std::vector<std::string>& outNewNames,
								   std::string& error) {
	outNewNames.clear();
	AnimSkeleton& skel = AnimSkeleton::getInstance();

	std::vector<AnimBone*> bones;
	for (auto& name : boneNames) {
		AnimBone* bPtr = skel.GetBonePtr(name);
		if (!bPtr) {
			error = "There is no bone '" + name + "'.";
			return false;
		}
		bones.push_back(bPtr);
	}

	// A bone listed twice gets the same name twice, a conflict too
	const std::vector<std::string> newNames = DuplicateBoneNames(boneNames, find, replace, suffix);
	const std::vector<DuplicateBoneConflict> conflicts = DuplicateBoneConflicts(boneNames, newNames);
	for (size_t i = 0; i < newNames.size(); ++i) {
		if (conflicts[i] != DuplicateBoneConflict::None) {
			error = DuplicateBoneConflictText(conflicts[i], newNames[i]);
			return false;
		}
	}

	AnimBone* parentOverride = nullptr;
	if (newParent && !newParent->empty()) {
		if (std::find(newNames.begin(), newNames.end(), *newParent) != newNames.end()) {
			error = "The copies can't hang from one of themselves, '" + *newParent + "'.";
			return false;
		}
		parentOverride = skel.GetBonePtr(*newParent);
		if (!parentOverride) {
			error = "There is no parent bone '" + *newParent + "'.";
			return false;
		}
	}

	std::unordered_map<AnimBone*, std::string> copyNames;
	for (size_t i = 0; i < bones.size(); ++i)
		copyNames[bones[i]] = newNames[i];

	// Parents are added first, so their copies exist for their children
	for (AnimBone* bPtr : SortBonesParentsFirst(bones)) {
		auto parentCopy = bPtr->parent ? copyNames.find(bPtr->parent) : copyNames.end();

		std::string parentName;
		MatTransform xformToParent = bPtr->xformToParent;
		if (parentCopy != copyNames.end()) {
			// The parent's copy is where the parent is, so the transform to it stays
			parentName = parentCopy->second;
		}
		else if (newParent) {
			// Keeps its place in the skeleton under the other parent
			MatTransform parentToGlobal;
			if (parentOverride)
				parentToGlobal = parentOverride->xformToGlobal;
			parentName = *newParent;
			xformToParent = parentToGlobal.InverseTransform().ComposeTransforms(bPtr->xformToGlobal);
		}
		else if (bPtr->parent)
			parentName = bPtr->parent->boneName;

		const std::string& newName = copyNames[bPtr];
		wxLogMessage("Duplicating bone '%s' as '%s' (parent: '%s').", bPtr->boneName, newName, parentName);
		AddCustomBoneRef(newName, parentName, xformToParent);
	}

	outNewNames = newNames;
	return true;
}

std::vector<UndoStateBoneXform> OutfitProject::MirrorBonesX(const std::vector<std::string>& boneNames) {
	AnimSkeleton& skel = AnimSkeleton::getInstance();

	std::vector<AnimBone*> bones;
	for (auto& name : boneNames) {
		AnimBone* bPtr = skel.GetBonePtr(name);
		if (!bPtr) {
			wxLogWarning("Can't mirror bone '%s', there is no such bone.", name);
			continue;
		}
		if (bPtr->isStandardBone) {
			wxLogWarning("Can't mirror bone '%s', it's a bone of the reference skeleton.", name);
			continue;
		}
		if (std::find(bones.begin(), bones.end(), bPtr) == bones.end())
			bones.push_back(bPtr);
	}
	bones = SortBonesParentsFirst(bones);

	// Targets come from the bones as they were, before any parent moved
	std::vector<MatTransform> targets;
	targets.reserve(bones.size());
	for (AnimBone* bPtr : bones)
		targets.push_back(MirrorTransformX(bPtr->xformToGlobal));

	std::vector<UndoStateBoneXform> xforms;
	for (size_t i = 0; i < bones.size(); ++i) {
		AnimBone* bPtr = bones[i];

		// The parent is already at its final place, mirrored or not
		MatTransform parentToGlobal;
		if (bPtr->parent)
			parentToGlobal = bPtr->parent->xformToGlobal;

		UndoStateBoneXform& xform = xforms.emplace_back();
		xform.boneName = bPtr->boneName;
		xform.startXformToParent = bPtr->xformToParent;
		xform.endXformToParent = parentToGlobal.InverseTransform().ComposeTransforms(targets[i]);

		wxLogMessage("Mirroring bone '%s' on the X axis.", xform.boneName);
		SetCustomBoneTransform(bPtr, xform.endXformToParent);
	}

	return xforms;
}

void OutfitProject::ApplyBoneXforms(const std::vector<UndoStateBoneXform>& xforms, bool undo) {
	// Parents first in both directions: each change recalculates the skin binds of its whole subtree
	for (auto& xform : xforms) {
		AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(xform.boneName);
		if (bPtr && !bPtr->isStandardBone)
			SetCustomBoneTransform(bPtr, undo ? xform.startXformToParent : xform.endXformToParent);
	}
}

void OutfitProject::SetCustomBoneTransform(AnimBone* bPtr, const MatTransform& xformToParent) {
	bPtr->SetTransformBoneToParent(xformToParent);

	for (auto& s : workNif.GetShapeNames())
		workAnim.RecursiveRecalcXFormSkinToBone(s, bPtr);
}

void OutfitProject::WriteAnimToNif(NifFile& nif, const std::string& shapeException) {
	auto lockedBonesPerVertex = [this](const std::string& shapeName) { return activeSet.GetBonesPerVertex(shapeName); };
	if (workAnim.WriteToNif(&nif, shapeException, lockedBonesPerVertex))
		return;

	wxString message = _("Bone information incomplete. Exported data will not contain correct bone entries! Be sure to load a reference NIF prior to export.");
	wxLogWarning(message);

	if (host->IsInteractive())
		host->ShowMessage(HostMessageLevel::Warning, message, _("Export Warning"));
}

void OutfitProject::CreateSkinning(NiShape* s) {
	if (s->IsSkinned())
		return;
	MatTransform globalToShape = workAnim.GetTransformGlobalToShape(s);
	workNif.CreateSkinning(s);
	workAnim.SetTransformGlobalToShape(s, globalToShape);
}

void OutfitProject::RemoveSkinning(NiShape* s) {
	if (!s->IsSkinned())
		return;
	MatTransform shapeToGlobal = workAnim.GetTransformShapeToGlobal(s);
	workNif.DeleteSkinning(s);
	workAnim.ClearShape(s->name.get());
	workAnim.SetTransformShapeToGlobal(s, shapeToGlobal);
}

void OutfitProject::RemoveSkinning() {
	for (auto& s : workNif.GetShapes()) {
		RemoveSkinning(s);
	}

	workNif.DeleteUnreferencedNodes();
}

BadBonesReport OutfitProject::FindBadBones() {
	BadBonesReport report;
	auto& shapeBBs = report.shapeBBs;
	auto& badCBs = report.badCBs;
	using ShapeBadBones = BadBonesReport::ShapeBadBones;
	using BadCustomBone = BadBonesReport::BadCustomBone;
	bool gotAnyBad = false;

	// Find bad bones for every skinned shape, initializing shapeBBs.
	for (NiShape* s : workNif.GetShapes()) {
		if (!s->IsSkinned())
			continue;

		std::string shapeName = s->name.get();
		ShapeBadBones& sbb = shapeBBs[shapeName];
		workAnim.FindBonesWithInconsistentTransforms(shapeName, sbb.badStandard, sbb.badCustom);
		if (!sbb.badStandard.empty() || !sbb.badCustom.empty())
			gotAnyBad = true;
		if (!sbb.badStandard.empty())
			sbb.fixStanSkin = true;
	}
	if (!gotAnyBad)
		return report;

	report.found = true;

	// For bad custom bones, we need to rearrange the data so it's keyed
	// on bone name rather than shape name.
	for (auto& sbbp : shapeBBs) {
		const std::string& shapeName = sbbp.first;
		ShapeBadBones& sbb = sbbp.second;
		for (auto& brp : sbb.badCustom)
			badCBs[brp.first].badShapes.insert(shapeName);
	}

	// For each bad custom bone...
	for (auto& bcbp : badCBs) {
		const std::string& bone = bcbp.first;
		BadCustomBone& bcb = bcbp.second;

		// ...fill in goodShapes
		for (auto& sbbp : shapeBBs) {
			const std::string& shapeName = sbbp.first;
			if (bcb.badShapes.count(shapeName) == 0 && workAnim.GetShapeBoneIndex(shapeName, bone) >= 0)
				bcb.goodShapes.insert(shapeName);
		}

		// ...and calculate a recommendation:
		// If a skin matches the node, trust them.
		if (!bcb.goodShapes.empty())
			bcb.fixtype = BadCustomBone::TrustNode;
		// Otherwise, if there's only one skin, trust it.
		if (bcb.fixtype == BadCustomBone::DoNothing && bcb.badShapes.size() == 1) {
			bcb.fixtype = BadCustomBone::TrustSkin;
			bcb.trustShape = *bcb.badShapes.begin();
		}
		// Otherwise, if one of the skins is the reference, trust it.
		if (bcb.fixtype == BadCustomBone::DoNothing && baseShape) {
			std::string baseShapeName = baseShape->name.get();
			for (const std::string& shapeName : bcb.badShapes)
				if (shapeName == baseShapeName) {
					bcb.fixtype = BadCustomBone::TrustSkin;
					bcb.trustShape = baseShapeName;
				}
		}
		// Otherwise, pick the skin with the lowest residual error.
		// This is pretty arbitrary.  It's only marginally better than
		// picking randomly.
		if (bcb.fixtype == BadCustomBone::DoNothing) {
			// Calculate smallest rotation residual error
			float minRotErr = 10.0f;
			for (const std::string& shapeName : bcb.badShapes) {
				float rotErr = RotMatToVec(shapeBBs[shapeName].badCustom[bone].rotation).length();
				if (rotErr < minRotErr)
					minRotErr = rotErr;
			}

			// Winnow shapes that have a significantly larger rotation error
			// than the smallest.
			std::unordered_set<std::string> shapesLeft;
			for (const std::string& shapeName : bcb.badShapes) {
				float rotErr = RotMatToVec(shapeBBs[shapeName].badCustom[bone].rotation).length();
				if (FloatsAreNearlyEqual(rotErr, minRotErr))
					shapesLeft.insert(shapeName);
			}

			// Calculate smallest translation residual error
			float minTrErr = FLT_MAX;
			for (const std::string& shapeName : shapesLeft) {
				float trErr = shapeBBs[shapeName].badCustom[bone].translation.length();
				if (trErr < minTrErr)
					minTrErr = trErr;
			}

			// Winnow shapes that have a significantly larger translation error
			// than the smallest.
			std::unordered_set<std::string> shapesLeft2;
			for (const std::string& shapeName : shapesLeft) {
				float trErr = shapeBBs[shapeName].badCustom[bone].translation.length();
				if (FloatsAreNearlyEqual(trErr, minTrErr))
					shapesLeft2.insert(shapeName);
			}
			shapesLeft = std::move(shapesLeft2);
			shapesLeft2.clear();

			// Calculate smallest scale residual error
			float minScErr = FLT_MAX;
			for (const std::string& shapeName : shapesLeft) {
				float scErr = std::fabs(shapeBBs[shapeName].badCustom[bone].scale - 1.0f);
				if (scErr < minScErr)
					minScErr = scErr;
			}

			// Winnow shapes that have a significantly larger scale error
			// than the smallest.
			for (const std::string& shapeName : shapesLeft) {
				float scErr = std::fabs(shapeBBs[shapeName].badCustom[bone].scale - 1.0f);
				if (FloatsAreNearlyEqual(scErr, minScErr))
					shapesLeft2.insert(shapeName);
			}
			shapesLeft = std::move(shapesLeft2);

			// We might have more than one shape left, but it's highly unlikely.
			// Pick the first of the remaining shapes.
			bcb.fixtype = BadCustomBone::TrustSkin;
			bcb.trustShape = *shapesLeft.begin();
		}
	}

	return report;
}

void OutfitProject::FixBadBones(const BadBonesReport& report) {
	for (auto& sbbp : report.shapeBBs) {
		const std::string &shapeName = sbbp.first;
		const BadBonesReport::ShapeBadBones& sbb = sbbp.second;

		if (sbb.fixStanSkin)
			for (auto& brp : sbb.badStandard)
				workAnim.RecalcXFormSkinToBone(shapeName, brp.first);
	}
	for (auto& bcbp : report.badCBs) {
		const std::string& bone = bcbp.first;
		const BadBonesReport::BadCustomBone& bcb = bcbp.second;
		if (bcb.fixtype == BadBonesReport::BadCustomBone::TrustSkin) {
			workAnim.RecalcCustomBoneXFormsFromSkin(bcb.trustShape, bone);
			// After updating the node's bone-to-global transform, any shapes
			// that had good skin-to-bone transforms now have bad ones and
			// have to be updated.
			for (const std::string& shapeName : bcb.goodShapes)
				workAnim.RecalcXFormSkinToBone(shapeName, bone);
		}
		if (bcb.fixtype != BadBonesReport::BadCustomBone::DoNothing)
			for (const std::string& shapeName : bcb.badShapes)
				workAnim.RecalcXFormSkinToBone(shapeName, bone);
	}
}

bool OutfitProject::CheckForBadBones() {
	BadBonesReport report = FindBadBones();
	if (!report.found)
		return false;

	FixBadBones(report);
	return true;
}

bool OutfitProject::ShapeHasBadBones(NiShape* s) {
	if (!s->IsSkinned())
		return false;
	std::unordered_map<std::string, MatTransform> badStandard, badCustom;
	std::string shapeName = s->name.get();
	workAnim.FindBonesWithInconsistentTransforms(shapeName, badStandard, badCustom);
	if (!badStandard.empty() || !badCustom.empty())
		return true;
	return false;
}

// GetAllPoseTransforms: this function calculates the pose transform (in
// shape coordinates) for every vertex.  Note that the resulting transforms
// do not follow the rules: the "rotation" is not necessarily a rotation;
// it might contain a scale, and it'll almost certainly contain rule violations
// because of linear interpolation.
void OutfitProject::GetAllPoseTransforms(NiShape* s, std::vector<MatTransform>& ts) {
	int nVerts = static_cast<int>(s->GetNumVertices());
	ts.resize(nVerts);

	// We're going to _sum_ rotations, so we have to start with zero matrices.
	for (int i = 0; i < nVerts; ++i)
		ts[i].rotation[0][0] = ts[i].rotation[1][1] = ts[i].rotation[2][2] = 0.0f;
	std::vector<float> tws(nVerts, 0.0f); // Total weight for each vertex

	AnimSkin& animSkin = workAnim.shapeSkinning[s->name.get()];
	MatTransform globalToSkin = workAnim.GetTransformGlobalToShape(s);
	bool isSF = workNif.GetHeader().GetVersion().IsSF();

	for (auto& boneNamesIt : animSkin.boneNames) {
		AnimBone* animB = AnimSkeleton::getInstance().GetBonePtr(boneNamesIt.first);
		if (!animB)
			continue;
		AnimWeight& animW = animSkin.boneWeights[boneNamesIt.second];
		// Compose transform: skin -> (posed) bone -> global -> skin
		MatTransform t = globalToSkin.ComposeTransforms(animB->xformPoseToGlobal.ComposeTransforms(animW.xformSkinToBone));

		if (isSF)
			t.translation *= sfHavokScale;
		// Add weighted contributions to vertex transforms for this bone
		for (auto& wIt : animW.weights) {
			int vi = wIt.first;
			float w = wIt.second;
			ts[vi].rotation += t.rotation * (w * t.scale);
			ts[vi].translation += t.translation * w;
			tws[vi] += w;
		}
	}

	// Check if total weight for each vertex was 1
	for (int vi = 0; vi < nVerts; ++vi) {
		if (tws[vi] < EPSILON) { // If weights are missing for this vertex
			ts[vi] = MatTransform();
		}
		else if (std::fabs(tws[vi] - 1.0f) >= EPSILON) { // If weights are not normalized for this vertex
			float normAdjust = 1.0f / tws[vi];
			ts[vi].rotation *= normAdjust;
			ts[vi].translation *= normAdjust;
		}
		// else do nothing because weights totaled 1.
	}

	// Note that the rotations still contain scale and interpolation
	// artifacts that make them not rotations!
}

// Note that ApplyTransformToOneVertexGeometry does not assume that t.rotation
// is a proper rotation, so it can be used with GetAllPoseTransforms.
void OutfitProject::ApplyTransformToOneVertexGeometry(UndoStateVertex& usv, const MatTransform& transform) {
	Vector3 newPos = transform.ApplyTransform(usv.pos);
	if (newPos.IsNearlyEqualTo(usv.pos)) {
		// New position is nearly equal to old position (reduce noise)
		return;
	}

	usv.pos = newPos;
	usv.normal = transform.ApplyTransformToDir(usv.normal);
	usv.normal.Normalize();
	usv.tangent = transform.ApplyTransformToDir(usv.tangent);
	usv.tangent.Normalize();
	usv.bitangent = transform.ApplyTransformToDir(usv.bitangent);
	usv.bitangent.Normalize();

	for (auto& usvsd : usv.diffs) {
		SliderData& sd = activeSet[usvsd.sliderName];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;

		usvsd.diff = transform.ApplyTransformToDiff(usvsd.diff);
	}
}

void OutfitProject::ApplyPoseTransformsToShapeGeometry(NiShape* s, UndoStateShape& uss) {
	int nVerts = static_cast<int>(s->GetNumVertices());
	int nTris = static_cast<int>(s->GetNumTriangles());

	// Get all the shape's vertices
	std::vector<uint16_t> vinds(nVerts);
	for (int i = 0; i < nVerts; ++i)
		vinds[i] = i;
	CollectVertexData(s, uss, vinds);
	uss.addVerts = uss.delVerts;

	// Get all the shape's triangles
	std::vector<uint32_t> tinds(nTris);
	for (int i = 0; i < nTris; ++i)
		tinds[i] = i;
	CollectTriangleData(s, uss, tinds);
	uss.addTris = uss.delTris;

	// Get all the pose transforms
	std::vector<MatTransform> ts;
	GetAllPoseTransforms(s, ts);

	// Apply all the pose transforms
	for (int i = 0; i < nVerts; ++i)
		ApplyTransformToOneVertexGeometry(uss.addVerts[i], ts[i]);
}

void OutfitProject::ApplyPoseTransformsToAllShapeGeometry(UndoStateProject& usp) {
	usp.undoType = UndoType::Mesh;
	std::vector<NiShape*> shapes = workNif.GetShapes();
	usp.usss.resize(shapes.size());
	for (size_t si = 0; si < shapes.size(); ++si) {
		UndoStateShape& uss = usp.usss[si];
		uss.shapeName = shapes[si]->name.get();
		ApplyPoseTransformsToShapeGeometry(shapes[si], uss);
	}
}
