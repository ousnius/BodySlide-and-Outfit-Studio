/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectNifHelpers.h"
#include "components/Anim.h"
#include "utils/StringStuff.h"

using namespace nifly;

std::function<bool(const std::string&)> IsSkeletonBone(AnimSkeleton& skeleton) {
	return [&skeleton](const std::string& bone) { return skeleton.GetBonePtr(bone, false) != nullptr; };
}

static const std::string physicsExtraDataName = "hdt skinned mesh physics object";

bool IsPhysicsExtraData(NiExtraData* extraData) {
	auto stringExtraData = dynamic_cast<NiStringExtraData*>(extraData);
	return stringExtraData && ToLower(stringExtraData->name.get()) == physicsExtraDataName;
}

// The HDT-SMP links on the root node of a NIF, in block order
std::vector<NiStringExtraData*> GetRootPhysicsExtraData(NifFile& nif) {
	std::vector<NiStringExtraData*> physicsData;

	auto root = nif.GetRootNode();
	if (!root)
		return physicsData;

	for (auto& extraDataRef : root->extraDataRefs) {
		auto stringExtraData = nif.GetHeader().GetBlock<NiStringExtraData>(extraDataRef);
		if (stringExtraData && IsPhysicsExtraData(stringExtraData))
			physicsData.push_back(stringExtraData);
	}

	return physicsData;
}

// Points the strings of a block that was cloned out of another NIF at the
// strings of the header it now lives in
void RemapExtraDataStrings(NifFile& nif, NiExtraData* extraData) {
	std::vector<NiStringRef*> stringRefs;
	extraData->GetStringRefs(stringRefs);

	for (auto& stringRef : stringRefs)
		stringRef->SetIndex(nif.GetHeader().AddOrFindStringId(stringRef->get()));
}
