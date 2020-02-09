/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "WeightNorm.h"
#include "Anim.h"

void BoneWeightAutoNormalizer::SetUp(UndoStateShape *ussi, AnimInfo *animInfo, const std::string &shapeName, const std::vector<std::string> &boneNames, const std::vector<std::string> &lockedBoneNames, int nMBonesi, bool bSprWt) {
	uss = ussi;
	wPtrs.clear();
	lWPtrs.clear();
	nMBones = nMBonesi;
	bSpreadWeight = bSprWt;

	const unsigned int nBones = boneNames.size();
	const unsigned int nLBones = lockedBoneNames.size();
	// Create uss->boneWeights if necessary
	if (uss->boneWeights.size() != nBones) {
		uss->boneWeights.resize(nBones);
		for (unsigned int bi = 0; bi < nBones; ++bi)
			uss->boneWeights[bi].boneName = boneNames[bi];
	}
	// Stash weights pointers so we don't look them up over and over.
	wPtrs.resize(nBones);
	lWPtrs.resize(nLBones);
	for (unsigned int bi = 0; bi < nBones; ++bi)
		wPtrs[bi] = animInfo->GetWeightsPtr(shapeName, boneNames[bi]);
	for (unsigned int bi = 0; bi < nLBones; ++bi)
		lWPtrs[bi] = animInfo->GetWeightsPtr(shapeName, lockedBoneNames[bi]);
}

void BoneWeightAutoNormalizer::GrabOneVertexStartingWeights(int i) {
	const unsigned int nBones = wPtrs.size();
	// Fill in uss start and end values if the vertex doesn't have them yet.
	for (unsigned int bi = 0; bi < nBones; ++bi) {
		auto &bw = uss->boneWeights[bi].weights;
		if (bw.find(i) == bw.end()) {
			float val = 0.0;
			if (wPtrs[bi] && wPtrs[bi]->find(i) != wPtrs[bi]->end())
				val = (*wPtrs[bi])[i];
			if (val > 0.0 || bi < nMBones)
				bw[i].startVal = bw[i].endVal = val;
		}
	}
}

void BoneWeightAutoNormalizer::GrabStartingWeights(const int *points, int nPoints) {
	for (int pi = 0; pi < nPoints; pi++)
		GrabOneVertexStartingWeights(points[pi]);
}

void BoneWeightAutoNormalizer::AdjustWeights(int vInd, bool *adjFlag) {
	// We have three sets of bones: modified, normalizable, and locked:
	// modified bones are those with bi < nMBones.
	// normalizable bones are those with bi >= nMBones.
	// locked bones are listed separately, in lWPtrs.
	const unsigned int nBones = wPtrs.size();
	const unsigned int nLBones = lWPtrs.size();
	// Calculate total locked and normalizable weight
	float totNW = 0.0;
	for (unsigned int bi = 0; bi < nBones; ++bi) {
		if (bi < nMBones && (!adjFlag || adjFlag[bi])) continue;
		auto &bw = uss->boneWeights[bi].weights;
		if (bw.find(vInd) != bw.end())
			totNW += bw[vInd].endVal;
	}
	float totLW = 0.0;
	for (unsigned int bi = 0; bi < nLBones; ++bi) {
		if (!lWPtrs[bi]) continue;
		auto wpit = lWPtrs[bi]->find(vInd);
		if (wpit != lWPtrs[bi]->end())
			totLW += wpit->second;
	}
	// Calculate available weight
	if (totLW < WEIGHT_EPSILON) totLW = 0.0;
	float availW = 1.0 - totLW;
	if (availW < WEIGHT_EPSILON) availW = 0.0;
	// Limit modified weights and total them.
	float totMW = 0.0;
	for (unsigned int bi = 0; bi < nMBones; ++bi) {
		if (adjFlag && !adjFlag[bi]) continue;
		auto wi = uss->boneWeights[bi].weights.find(vInd);
		if (wi == uss->boneWeights[bi].weights.end()) continue;
		float &w = wi->second.endVal;
		if (w < WEIGHT_EPSILON) w = 0.0;
		if (w - 1.0 > -WEIGHT_EPSILON) w = 1.0;
		totMW += w;
	}
	// If the total modified weight is too high, reduce it.
	if (totMW > availW) {
		float modFac = totMW <= WEIGHT_EPSILON ? 0.0 : availW / totMW;
		totMW = 0.0;
		for (unsigned int bi = 0; bi < nMBones; ++bi) {
			if (adjFlag && !adjFlag[bi]) continue;
			auto wi = uss->boneWeights[bi].weights.find(vInd);
			if (wi == uss->boneWeights[bi].weights.end()) continue;
			wi->second.endVal *= modFac;
			totMW += wi->second.endVal;
		}
		if (totMW > availW) totMW = availW;
	}
	// Normalize, adjusting only normalizable bones
	float nrmFac = totNW <= WEIGHT_EPSILON ? 0.0 : (availW - totMW) / totNW;
	totNW = 0.0;
	for (unsigned int bi = 0; bi < nBones; ++bi) {
		if (bi < nMBones && (!adjFlag || adjFlag[bi])) continue;
		auto owi = uss->boneWeights[bi].weights.find(vInd);
		if (owi == uss->boneWeights[bi].weights.end()) continue;
		float &ow = owi->second.endVal;
		ow *= nrmFac;
		if (ow < WEIGHT_EPSILON) ow = 0.0;
		if (ow - 1.0 > -WEIGHT_EPSILON) ow = 1.0;
		totNW += ow;
	}
	// Check if normalization didn't work; if so, split the missing
	// weight among the normalizable bones, if any, and only if
	// bSpreadWeight is true.
	if (1.0 - totNW - totMW - totLW > WEIGHT_EPSILON && nBones > nMBones && bSpreadWeight) {
		float remainW = (1.0 - totNW - totMW - totLW) / (nBones - nMBones);
		for (unsigned int bi = 0; bi < nBones; ++bi) {
			if (bi < nMBones && (!adjFlag || adjFlag[bi])) continue;
			auto &bw = uss->boneWeights[bi].weights;
			auto owi = bw.find(vInd);
			if (owi == bw.end())
				bw[vInd].startVal = bw[vInd].endVal = 0.0;
			bw[vInd].endVal += remainW;
		}
	}
}

float BoneWeightAutoNormalizer::SetWeight(int vInd, float w) {
	uss->boneWeights[0].weights[vInd].endVal = w;
	AdjustWeights(vInd);
	return uss->boneWeights[0].weights[vInd].endVal;
}
