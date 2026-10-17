#include "../src/components/Anim.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>

using namespace nifly;

TEST_CASE("Starfield export writes weights independently of the editing skeleton", "[anim][export][starfield]") {
	AnimSkeleton::getInstance().Clear();
	NifFile nif;
	nif.Create(NiVersion::getSF());
	auto shapeBlock = std::make_unique<BSGeometry>();
	auto shape = shapeBlock.get();
	shape->name.get() = "Cloth";
	shape->SetInternalGeomData(true);
	auto mesh = shape->AddMesh();
	mesh->meshData.version = 2;
	mesh->meshData.scale = 1.0f;
	mesh->meshData.nVertices = 3;
	mesh->meshData.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	mesh->meshData.nWeightsPerVert = 1;
	mesh->meshData.skinWeights = {{{0, 65535}}, {{1, 65535}}, {{1, 65535}}};
	nif.GetRootNode()->childRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(shapeBlock)));

	const std::vector<std::string> names{"Cloth_A", "Cloth_B"};
	auto attach = std::make_unique<SkinAttach>();
	attach->bones.resize(2);
	for (uint32_t i = 0; i < names.size(); ++i)
		attach->bones[i].get() = names[i];
	const auto attachId = nif.AssignExtraData(shape, std::move(attach));
	auto data = std::make_unique<BSSkinBoneData>();
	data->nBones = 2;
	data->boneXforms.resize(2);
	data->boneXforms[1].boneTransform.translation = Vector3(0, 0, 2);
	auto skin = std::make_unique<BSSkinInstance>();
	skin->dataRef.index = nif.GetHeader().AddBlock(std::move(data));
	skin->targetRef.index = nif.GetBlockID(nif.GetRootNode());
	skin->boneRefs.AddBlockRef(NIF_NPOS);
	skin->boneRefs.AddBlockRef(NIF_NPOS);
	shape->SkinInstanceRef()->index = nif.GetHeader().AddBlock(std::move(skin));

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	NifFile output(nif);
	auto outputShape = output.FindBlockByName<BSGeometry>("Cloth");

	SECTION("Complete cloth data exports without skeleton bones") {
		REQUIRE(anim.WriteToNif(&output));
		std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
		REQUIRE(output.Save(stream) == 0);
		NifFile reloaded;
		REQUIRE(reloaded.Load(stream) == 0);
		auto reloadedShape = reloaded.FindBlockByName<BSGeometry>("Cloth");
		REQUIRE(reloadedShape);
		std::vector<std::string> exportedNames;
		reloaded.GetShapeBoneList(reloadedShape, exportedNames);
		REQUIRE(exportedNames == names);
		MatTransform transform;
		REQUIRE(reloaded.GetShapeTransformSkinToBone(reloadedShape, 1, transform));
		REQUIRE(transform.translation.z == 2.0f);
		std::unordered_map<uint16_t, float> weights;
		reloaded.GetShapeBoneWeights(reloadedShape, 1, weights);
		REQUIRE(weights == std::unordered_map<uint16_t, float>{{1, 1.0f}, {2, 1.0f}});
	}
	SECTION("Export writes the shape's bones to SkinAttach and bone data") {
		std::vector<std::string> expectedNames = names;
		std::unordered_map<uint16_t, float> weightsA{{1, 1.0f}};
		std::unordered_map<uint16_t, float> weightsB{{0, 1.0f}, {2, 1.0f}};
		std::unordered_map<uint16_t, float> weightsC;
		SECTION("Painting or copying weights adds a new bone") {
			REQUIRE(anim.AddShapeBone("Cloth", "Cloth_C"));
			expectedNames.push_back("Cloth_C");
			weightsB.erase(2);
			weightsC = {{2, 1.0f}};
			anim.SetWeights("Cloth", "Cloth_C", weightsC);
			MatTransform newBoneTransform;
			newBoneTransform.translation = Vector3(0, 0, 9);
			anim.SetXFormSkinToBone("Cloth", "Cloth_C", newBoneTransform);
		}
		SECTION("Shape has no SkinAttach") {
			outputShape->extraDataRefs.Clear();
		}
		SECTION("SkinAttach has a different bone order") {
			auto outputAttach = output.GetHeader().GetBlock<SkinAttach>(attachId);
			std::swap(outputAttach->bones[0], outputAttach->bones[1]);
		}
		anim.SetWeights("Cloth", "Cloth_A", weightsA);
		anim.SetWeights("Cloth", "Cloth_B", weightsB);
		MatTransform editedTransform;
		editedTransform.translation = Vector3(0, 0, 7);
		anim.SetXFormSkinToBone("Cloth", "Cloth_B", editedTransform);

		CHECK(anim.WriteToNif(&output));
		std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
		REQUIRE(output.Save(stream) == 0);
		NifFile reloaded;
		REQUIRE(reloaded.Load(stream) == 0);
		auto reloadedShape = reloaded.FindBlockByName<BSGeometry>("Cloth");
		REQUIRE(reloadedShape);

		std::vector<std::string> exportedNames;
		reloaded.GetShapeBoneList(reloadedShape, exportedNames);
		CHECK(exportedNames == expectedNames);
		auto reloadedSkin = reloaded.GetHeader().GetBlock<BSSkinInstance>(reloadedShape->SkinInstanceRef());
		REQUIRE(reloadedSkin);
		auto reloadedData = reloaded.GetHeader().GetBlock(reloadedSkin->dataRef);
		REQUIRE(reloadedData);
		CHECK(reloadedData->nBones == expectedNames.size());
		CHECK(reloadedData->boneXforms.size() == expectedNames.size());

		std::unordered_map<uint16_t, float> weights;
		reloaded.GetShapeBoneWeights(reloadedShape, 0, weights);
		CHECK(weights == weightsA);
		reloaded.GetShapeBoneWeights(reloadedShape, 1, weights);
		CHECK(weights == weightsB);
		reloaded.GetShapeBoneWeights(reloadedShape, 2, weights);
		CHECK(weights == weightsC);
		MatTransform transform;
		REQUIRE(reloaded.GetShapeTransformSkinToBone(reloadedShape, 1, transform));
		CHECK(transform.translation.z == 7.0f);
		if (expectedNames.size() > 2) {
			REQUIRE(reloaded.GetShapeTransformSkinToBone(reloadedShape, 2, transform));
			CHECK(transform.translation.z == 9.0f);
		}
	}
	AnimSkeleton::getInstance().Clear();
}

TEST_CASE("Non-Starfield export still warns about missing skeleton bones", "[anim][export]") {
	NifFile nif;
	nif.Create(NiVersion::getFO4());
	const std::vector<Vector3> verts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	const std::vector<Triangle> tris{{0, 1, 2}};
	auto shape = nif.CreateShapeFromData("Cloth", &verts, &tris, nullptr);
	nif.CreateSkinning(shape);
	auto bone = nif.AddNode("Cloth_A", MatTransform());
	std::vector<int> ids{static_cast<int>(nif.GetBlockID(bone))};
	nif.SetShapeBoneIDList(shape, ids);
	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	AnimSkeleton::getInstance().Clear();
	REQUIRE_FALSE(anim.WriteToNif(&nif));
}
