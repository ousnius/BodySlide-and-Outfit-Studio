/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "UndoState.h"

class AnimInfo;

/* BoneWeightAutoNormalizer: this class has the algorithm for automatic
normalization of bone weights.  The algorithm itself is in SetWeight.
The rest is setup.

The algorithm adjusts the weights of the normalize bones to follow the
following rules, from highest to lowest priority:
1.  The locked bones' weights are not changed (and are not even present
in uss->boneWeights).
2.  All weights must be between zero and one.
4.  The sum of the weights is no more than one.
5.  The specified weight is set.
6.  If not bSpreadWeight, zero weights are not changed.
7.  The sum of the weights is at least one.

Note that, while uss->boneWeights is the main output, it can also be
a source of input, containing the results of previous changes.
*/
class BoneWeightAutoNormalizer {
	static constexpr double WEIGHT_EPSILON = .001;
	UndoStateShape *uss;
	std::vector<std::unordered_map<ushort, float>*> wPtrs, lWPtrs;
	bool bSpreadWeight;
public:
	/* SetUp: fills in the class's private data.  ussi->boneWeights
	must either already match boneNames or be empty, in which case
	it is initialized.  boneNames and lockedBoneNames must not have any
	common names, and together they must include all bones with non-zero
	weights for the shape.  boneNames is the "normalize bones", the bones
	whose weights can be adjusted, and must include the bone whose weights
	will actually be modified.  */
	void SetUp(UndoStateShape *ussi, AnimInfo *animInfo, const std::string &shapeName, const std::vector<std::string> &boneNames, const std::vector<std::string> &lockedBoneNames, bool bSprWt);

	/* GrabStartingWeights: initializes missing startVal and endVal
	in uss->boneWeights for normalize bones for the vertices with
	the given indices by copying weights from animInfo. */
	void GrabStartingWeights(const int *points, int nPoints);

	/* SetWeight:  sets the weight for the vertex with the given index
	for the given bone (indexed into boneNames or uss->boneWeights)
	and normalizes the weights for the vertex.  */
	float SetWeight(int bonInd, int ptInd, float w);
};
