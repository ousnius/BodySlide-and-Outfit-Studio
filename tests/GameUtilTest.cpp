#include "utils/GameUtil.h"
#include "utils/TargetGame.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("Each game has its reference skeleton and root bone", "[GameUtil]") {
	auto check = [](int game, const std::string& file, const std::string& root) {
		GameUtil::DefaultSkeleton skeleton = GameUtil::GetDefaultSkeleton(game);
		CHECK(skeleton.file == file);
		CHECK(skeleton.rootName == root);
	};

	check(OB, "res/skeleton_ob.nif", "Bip01");
	check(FO3, "res/skeleton_fo3nv.nif", "Bip01");
	check(FONV, "res/skeleton_fo3nv.nif", "Bip01");
	check(SKYRIM, "res/skeleton_female_sk.nif", "NPC Root [Root]");
	check(SKYRIMSE, "res/skeleton_female_sse.nif", "NPC Root [Root]");
	check(SKYRIMVR, "res/skeleton_female_sse.nif", "NPC Root [Root]");
	check(FO4, "res/skeleton_fo4.nif", "Root");
	check(FO4VR, "res/skeleton_fo4.nif", "Root");
	check(FO76, "res/skeleton_fo4.nif", "Root");
	check(SF, "res/skeleton_female_sf.nif", "Root");
}
