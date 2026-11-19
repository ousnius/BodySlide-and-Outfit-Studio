/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: conforming sliders to the reference and correcting clipping

#include "OutfitProject.h"
#include "components/ClippingFixer.h"
#include "components/MaskOps.h"
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

void OutfitProject::InitConform() {
	if (baseShape) {
		morpher.SetRef(workNif, baseShape, &workAnim);
		morpher.LinkRefDiffData(&baseDiffData);
		morpher.SourceShapesFromNif(workNif, &workAnim);
	}
}

void OutfitProject::GetConformSliderNames(const ConformOptions& options, std::vector<std::string>& outSliderNames) {
	outSliderNames.clear();

	for (size_t i = 0; i < activeSet.size(); i++) {
		if (SliderZap(i) || SliderUV(i))
			continue;

		if (!options.sliderNames.empty()) {
			bool found = false;
			for (const auto& sliderName : options.sliderNames) {
				if (sliderName == activeSet[i].name) {
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		outSliderNames.push_back(activeSet[i].name);
	}
}

namespace {
struct SavedSliderState {
	float value = 0.0f;
	bool show = false;
};

Vector3 RemoveNeutralClippingCorrection(const Vector3& sliderCorrection, const Vector3& neutralCorrection) {
	float neutralLenSq = neutralCorrection.length2();
	if (neutralLenSq <= 1e-12f)
		return sliderCorrection;

	float neutralShare = sliderCorrection.dot(neutralCorrection) / neutralLenSq;
	if (neutralShare <= 0.0f)
		return sliderCorrection;

	return sliderCorrection - neutralCorrection * std::min(1.0f, neutralShare);
}
}

bool OutfitProject::BuildClippingCorrectionCache(NiShape* shape, float strength, ClippingCorrectionCache& cache) {
	cache = ClippingCorrectionCache();

	if (!shape || IsBaseShape(shape) || !baseShape || strength <= 0.0f)
		return false;

	if (!ClippingFixer::IsEligibleForFix(workNif, shape))
		return false;

	baseShape->GetTriangles(cache.bodyTris);
	shape->GetTriangles(cache.outfitTris);
	if (cache.bodyTris.empty() || cache.outfitTris.empty())
		return false;

	std::vector<SavedSliderState> savedStates(activeSet.size());
	for (size_t sliderIndex = 0; sliderIndex < activeSet.size(); sliderIndex++) {
		savedStates[sliderIndex].value = activeSet[sliderIndex].curValue;
		savedStates[sliderIndex].show = activeSet[sliderIndex].bShow;
		activeSet[sliderIndex].curValue = 0.0f;
		activeSet[sliderIndex].bShow = false;
	}

	GetLiveVerts(baseShape, cache.neutralBodyVerts);
	GetLiveVerts(shape, cache.neutralOutfitVerts);

	for (size_t sliderIndex = 0; sliderIndex < activeSet.size(); sliderIndex++) {
		activeSet[sliderIndex].curValue = savedStates[sliderIndex].value;
		activeSet[sliderIndex].bShow = savedStates[sliderIndex].show;
	}

	if (cache.neutralBodyVerts.empty() || cache.neutralOutfitVerts.empty())
		return false;

	ClippingFixOptions fixOptions;
	fixOptions.strength = std::max(0.0f, std::min(1.0f, strength));
	fixOptions.fixPokeThrough = true;

	cache.fixedNeutralVerts = cache.neutralOutfitVerts;
	ClippingFixer::FixClipping(cache.neutralBodyVerts, cache.bodyTris, cache.fixedNeutralVerts, cache.outfitTris, fixOptions);
	cache.valid = true;
	return true;
}

void OutfitProject::CalcSliderClippingCorrection(NiShape* shape,
											 const std::string& sliderName,
											 float strength,
											 TargetDataDiffs& outMorphDiffs,
											 const std::unordered_set<uint16_t>* allowedVerts) {
	CalcSliderClippingCorrection(shape, sliderName, strength, outMorphDiffs, allowedVerts, nullptr);
}

void OutfitProject::CalcSliderClippingCorrection(NiShape* shape,
											 const std::string& sliderName,
											 float strength,
											 TargetDataDiffs& outMorphDiffs,
											 const std::unordered_set<uint16_t>* allowedVerts,
											 const ClippingCorrectionCache* cache) {
	outMorphDiffs.clear();

	if (!shape || IsBaseShape(shape) || !baseShape || strength <= 0.0f)
		return;

	if (!ClippingFixer::IsEligibleForFix(workNif, shape))
		return;

	size_t sliderIndex = 0;
	if (!SliderIndexFromName(sliderName, sliderIndex))
		return;

	TargetDataDiffs* diffSet = GetDiffSet(activeSet[sliderIndex], shape);
	if (!diffSet || diffSet->empty())
		return;

	const ClippingCorrectionCache* validCache = (cache && cache->valid) ? cache : nullptr;
	std::vector<Triangle> bodyTris;
	std::vector<Triangle> outfitTris;
	const std::vector<Triangle>* bodyTrisPtr = nullptr;
	const std::vector<Triangle>* outfitTrisPtr = nullptr;

	if (validCache) {
		bodyTrisPtr = &validCache->bodyTris;
		outfitTrisPtr = &validCache->outfitTris;
	}
	else {
		baseShape->GetTriangles(bodyTris);
		shape->GetTriangles(outfitTris);
		bodyTrisPtr = &bodyTris;
		outfitTrisPtr = &outfitTris;
	}

	if (bodyTrisPtr->empty() || outfitTrisPtr->empty())
		return;

	std::vector<SavedSliderState> savedStates(activeSet.size());
	for (size_t activeSliderIndex = 0; activeSliderIndex < activeSet.size(); activeSliderIndex++) {
		savedStates[activeSliderIndex].value = activeSet[activeSliderIndex].curValue;
		savedStates[activeSliderIndex].show = activeSet[activeSliderIndex].bShow;
		activeSet[activeSliderIndex].curValue = 0.0f;
		activeSet[activeSliderIndex].bShow = false;
	}

	std::vector<Vector3> neutralBodyVerts;
	std::vector<Vector3> neutralOutfitVerts;
	if (!validCache) {
		GetLiveVerts(baseShape, neutralBodyVerts);
		GetLiveVerts(shape, neutralOutfitVerts);
	}

	activeSet[sliderIndex].curValue = 1.0f;
	activeSet[sliderIndex].bShow = true;

	std::vector<Vector3> sliderBodyVerts;
	std::vector<Vector3> sliderOutfitVerts;
	GetLiveVerts(baseShape, sliderBodyVerts);
	GetLiveVerts(shape, sliderOutfitVerts);

	for (size_t activeSliderIndex = 0; activeSliderIndex < activeSet.size(); activeSliderIndex++) {
		activeSet[activeSliderIndex].curValue = savedStates[activeSliderIndex].value;
		activeSet[activeSliderIndex].bShow = savedStates[activeSliderIndex].show;
	}

	const std::vector<Vector3>* neutralBodyVertsPtr = validCache ? &validCache->neutralBodyVerts : &neutralBodyVerts;
	const std::vector<Vector3>* neutralOutfitVertsPtr = validCache ? &validCache->neutralOutfitVerts : &neutralOutfitVerts;
	if (neutralBodyVertsPtr->empty() || neutralOutfitVertsPtr->empty() || sliderBodyVerts.empty() || sliderOutfitVerts.empty())
		return;

	ClippingFixOptions fixOptions;
	fixOptions.strength = std::max(0.0f, std::min(1.0f, strength));
	fixOptions.fixPokeThrough = true;

	std::vector<Vector3> fixedNeutralVerts;
	const std::vector<Vector3>* fixedNeutralVertsPtr = nullptr;
	if (validCache) {
		fixedNeutralVertsPtr = &validCache->fixedNeutralVerts;
	}
	else {
		fixedNeutralVerts = *neutralOutfitVertsPtr;
		ClippingFixer::FixClipping(*neutralBodyVertsPtr, *bodyTrisPtr, fixedNeutralVerts, *outfitTrisPtr, fixOptions);
		fixedNeutralVertsPtr = &fixedNeutralVerts;
	}

	std::vector<Vector3> fixedSliderVerts = sliderOutfitVerts;
	ClippingFixer::FixClipping(sliderBodyVerts, *bodyTrisPtr, fixedSliderVerts, *outfitTrisPtr, fixOptions);

	size_t vertCount = std::min(neutralOutfitVertsPtr->size(), std::min(fixedNeutralVertsPtr->size(), std::min(sliderOutfitVerts.size(), fixedSliderVerts.size())));
	constexpr size_t maxTargetDataVertexCount = static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1;
	if (vertCount > maxTargetDataVertexCount) {
		wxLogWarning("Clipping correction for shape '%s' slider '%s' is limited to vertices 0-%u because slider diffs use 16-bit vertex indices.",
					 shape->name.get(), sliderName.c_str(), static_cast<unsigned int>(std::numeric_limits<uint16_t>::max()));
	}

	auto addCorrection = [&](uint16_t vertIndex) {
		if (diffSet->find(vertIndex) == diffSet->end())
			return;

		if (allowedVerts && allowedVerts->find(vertIndex) == allowedVerts->end())
			return;

		if (vertIndex >= vertCount)
			return;

		Vector3 neutralCorrection = (*fixedNeutralVertsPtr)[vertIndex] - (*neutralOutfitVertsPtr)[vertIndex];
		Vector3 sliderCorrection = fixedSliderVerts[vertIndex] - sliderOutfitVerts[vertIndex];
		Vector3 nifDiff = RemoveNeutralClippingCorrection(sliderCorrection, neutralCorrection);
		if (nifDiff.IsZero(true))
			return;

		outMorphDiffs[vertIndex] = Mesh::TransformDiffNifToMesh(nifDiff);
	};

	if (allowedVerts && allowedVerts->size() < diffSet->size()) {
		for (uint16_t vertIndex : *allowedVerts)
			addCorrection(vertIndex);
	}
	else {
		for (auto& diffEntry : *diffSet)
			addCorrection(diffEntry.first);
	}
}

void OutfitProject::ApplyClippingFixToConformedSlider(NiShape* shape, const std::string& sliderName, float strength) {
	TargetDataDiffs morphDiffs;
	CalcSliderClippingCorrection(shape, sliderName, strength, morphDiffs);
	if (!morphDiffs.empty())
		UpdateMorphResult(shape, sliderName, morphDiffs);
}

void OutfitProject::ConformShape(NiShape* shape, const ConformOptions& options) {
	if (!workNif.IsValid() || !baseShape)
		return;

	std::unordered_map<uint16_t, float> mask;
	if (Mesh* refMesh = host->GetShapeMesh(baseShape->name.get()))
		mask = MaskOps::GetMasked(*refMesh);

	std::set<uint16_t> maskIndices;
	for (auto& m : mask)
		maskIndices.insert(m.first);

	morpher.BuildProximityCache(shape->name.get(), options.proximityRadius, &maskIndices);

	std::string refTarget = ShapeToTarget(baseShape->name.get());
	std::string resultTarget = SliderDataTargetForShape(shape);
	std::vector<std::string> conformSliderNames;
	GetConformSliderNames(options, conformSliderNames);

	int conformedCount = static_cast<int>(conformSliderNames.size());
	int skippedByFilter = 0;
	if (!options.sliderNames.empty()) {
		// Count sliders excluded only by the name filter (not by zap/UV).
		size_t eligible = 0;
		for (size_t i = 0; i < activeSet.size(); i++)
			if (!SliderZap(i) && !SliderUV(i))
				eligible++;

		skippedByFilter = static_cast<int>(eligible) - conformedCount;
		if (skippedByFilter < 0)
			skippedByFilter = 0;
	}

	ClippingCorrectionCache clippingCache;
	bool clippingCacheAttempted = false;

	for (const auto& sliderName : conformSliderNames) {
		std::string refDataName = activeSet[sliderName].TargetDataName(refTarget);
		bool hasReferenceDiff = !refDataName.empty() && baseDiffData.GetDiffSet(refDataName);

		morpher.GenerateResultDiff(shape->name.get(),
									   sliderName,
									   refDataName,
									   true,
									   options.maxResults,
									   options.noSqueeze,
									   options.solidMode,
									   options.axisX,
									   options.axisY,
									   options.axisZ,
									   resultTarget);
		if (options.smoothResultDeltas)
			morpher.SmoothResultDiff(shape->name.get(), sliderName, resultTarget, options.smoothIterations, options.smoothStrength);
		EnsureSliderDataLocal(sliderName, shape);
		if (options.fixClipping && hasReferenceDiff && morpher.GetResultDiffSize(resultTarget, sliderName) > 0) {
			if (!clippingCacheAttempted) {
				clippingCacheAttempted = true;
				BuildClippingCorrectionCache(shape, options.fixClippingStrength, clippingCache);
			}

			if (clippingCache.valid) {
				TargetDataDiffs morphDiffs;
				CalcSliderClippingCorrection(shape, sliderName, options.fixClippingStrength, morphDiffs, nullptr, &clippingCache);
				if (!morphDiffs.empty())
					UpdateMorphResult(shape, sliderName, morphDiffs);
			}
			else {
				ApplyClippingFixToConformedSlider(shape, sliderName, options.fixClippingStrength);
			}
		}
	}

	if (!options.sliderNames.empty())
		wxLogMessage("ConformShape '%s': conformed %d slider(s), skipped %d by name filter (of %zu total).",
					 shape->name.get(), conformedCount, skippedByFilter, activeSet.size());
}
