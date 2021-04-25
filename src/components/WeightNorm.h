/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "UndoState.h"

class AnimInfo;

/* BoneWeightAutoNormalizer: this class has the algorithm for
automatic normalization of bone weights.  The algorithm itself is
in AdjustWeights.  The rest is setup.

The algorithm adjusts the weights of the normalize bones to follow the
following rules, from highest to lowest priority:
1.  The locked bones' weights are not changed (and are not even present
in uss->boneWeights).
2.  All weights must be between zero and one.
4.  The sum of the weights is no more than one.
5.  The specified weights are set.
6.  If not bSpreadWeight, zero weights are not changed.
7.  The sum of the weights is at least one.
8.  Weights are multiplied by a scale factor if nonzero; an amount
is added to each if zero.

Note that, while uss->boneWeights is the main output, it can also be
a source of input, containing the results of previous changes.
*/
class BoneWeightAutoNormalizer {
	static constexpr double WEIGHT_EPSILON = .001;
	UndoStateShape *uss;
	std::vector<std::unordered_map<uint16_t, float>*> wPtrs, lWPtrs;
	uint32_t nMBones;
	bool bSpreadWeight;
public:
	/* SetUp: fills in the class's private data.  ussi->boneWeights
	must either already match boneNames or be empty, in which case
	it is initialized.  boneNames and lockedBoneNames must not have
	any common names, and together they must include all bones with
	non-zero weights for the shape.  boneNames is the "normalize
	bones", the bones whose weights can be adjusted, and must start
	with the bones whose weights will actually be modified; the number
	of these modified bones is nMBones.  */
	void SetUp(UndoStateShape *ussi, AnimInfo *animInfo, const std::string &shapeName, const std::vector<std::string> &boneNames, const std::vector<std::string> &lockedBoneNames, uint32_t nMBones, bool bSprWt);

	/* GrabOneVertexStartingWeights: initializes missing startVal
	and endVal in uss->boneweights for normalize bones for the vertex
	with the given index by copying weights from animInfo.  For
	bones with index >= nMBones, only nonzero weights are copied. */
	void GrabOneVertexStartingWeights(int ptInd);

	/* GrabStartingWeights: calls GrabOneVertexStartingWeights for
	the given vertex indices. */
	void GrabStartingWeights(const int *points, int nPoints);

	/* AdjustWeights: normalizes the weights for the vertex, preferring
	to adjust weights for bones with index >= nMBones (the unmodified
	but normalizable bones) before those with index < nBones (the
	modified bones).  If adjFlag is not null, it points to an array
	of size nMBones, with each array element indicating whether the
	weight for that bone was adjusted or not.  Note that only weight
	data in uss is accessed for unlocked bones, so you must ensure
	that all necessary data is grabbed first. */
	void AdjustWeights(int ptInd, bool *adjFlag = nullptr);

	/* SetWeight: sets the weight for the vertex with the given
	index for bone 0 (indexed into boneNames or uss->boneWeights) and
	normalizes the weights for the vertex (by calling AdjustWeights).
	Returns the adjusted w. */
	float SetWeight(int ptInd, float w);
};
