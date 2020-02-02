/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "WeightNorm.h"
#include "Anim.h"

void BoneWeightAutoNormalizer::SetUp(UndoStateShape *ussi, AnimInfo *animInfo, const std::string &shapeName, const std::vector<std::string> &boneNames, const std::vector<std::string> &lockedBoneNames, bool bSprWt) {
	uss = ussi;
	wPtrs.clear();
	lWPtrs.clear();
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

void BoneWeightAutoNormalizer::GrabStartingWeights(const int *points, int nPoints) {
	const unsigned int nBones = wPtrs.size();
	// Fill in uss start and end values for vertices that don't have them yet
	for (int pi = 0; pi < nPoints; pi++) {
		int i = points[pi];
		for (unsigned int bi = 0; bi < nBones; ++bi) {
			auto &bw = uss->boneWeights[bi].weights;
			if (bw.find(i) == bw.end()) {
				float val = 0.0;
				if (wPtrs[bi] && wPtrs[bi]->find(i) != wPtrs[bi]->end())
					val = (*wPtrs[bi])[i];
				if (val > 0.0 || bi == 0)
					bw[i].startVal = bw[i].endVal = val;
			}
		}
	}
}

float BoneWeightAutoNormalizer::SetWeight(int bonInd, int vInd, float w) {
	const unsigned int nBones = wPtrs.size();
	const unsigned int nLBones = lWPtrs.size();
	// Calculate total locked and normalizable weight
	float totW = 0.0;
	for (unsigned int bi = 0; bi < nBones; ++bi) {
		if (bi == bonInd) continue;
		auto &bw = uss->boneWeights[bi].weights;
		if (bw.find(vInd) != bw.end())
			totW += bw[vInd].endVal;
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
	// Limit result
	if (w < WEIGHT_EPSILON) w = 0.0;
	if (w > availW) w = availW;
	if (w - 1.0 > -WEIGHT_EPSILON) w = 1.0;
	uss->boneWeights[bonInd].weights[vInd].endVal = w;
	// Normalize
	float redFac = totW <= WEIGHT_EPSILON ? 0.0 : (availW - w) / totW;
	totW = 0.0;
	for (unsigned int bi = 0; bi < nBones; ++bi) {
		if (bi == bonInd) continue;
		auto owi = uss->boneWeights[bi].weights.find(vInd);
		if (owi == uss->boneWeights[bi].weights.end()) continue;
		float &ow = owi->second.endVal;
		ow *= redFac;
		if (ow < WEIGHT_EPSILON) ow = 0.0;
		if (ow - 1.0 > -WEIGHT_EPSILON) ow = 1.0;
		totW += ow;
	}
	// Check if normalization didn't work; if so, split the missing
	// weight among the normalize bones.
	if (1.0 - totW - w - totLW > WEIGHT_EPSILON && nBones >= 2 && bSpreadWeight) {
		float remainW = (1.0 - totW - w - totLW) / (nBones - 1);
		for (unsigned int bi = 0; bi < nBones; ++bi) {
			if (bi == bonInd) continue;
			auto &bw = uss->boneWeights[bi].weights;
			auto owi = bw.find(vInd);
			if (owi == bw.end())
				bw[vInd].startVal = bw[vInd].endVal = 0.0;
			bw[vInd].endVal += remainW;
		}
	}
	return w;
}
