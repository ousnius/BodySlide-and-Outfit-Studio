#include "components/Anim.h"
#include "components/Mesh.h"
#include "components/UndoState.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
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

TEST_CASE("Starfield export saves the bones per vertex the shape needs or is locked to", "[anim][export][starfield]") {
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
	mesh->meshData.nWeightsPerVert = 2;
	mesh->meshData.skinWeights = {{{0, 65535}, {0, 0}}, {{1, 65535}, {0, 0}}, {{1, 65535}, {0, 0}}};
	nif.GetRootNode()->childRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(shapeBlock)));

	std::vector<std::string> names;
	for (int i = 0; i < 9; i++)
		names.push_back("Cloth_" + std::to_string(i));
	auto attach = std::make_unique<SkinAttach>();
	attach->bones.resize(names.size());
	for (size_t i = 0; i < names.size(); ++i)
		attach->bones[i].get() = names[i];
	nif.AssignExtraData(shape, std::move(attach));
	auto data = std::make_unique<BSSkinBoneData>();
	data->nBones = static_cast<uint32_t>(names.size());
	data->boneXforms.resize(names.size());
	auto skin = std::make_unique<BSSkinInstance>();
	skin->dataRef.index = nif.GetHeader().AddBlock(std::move(data));
	skin->targetRef.index = nif.GetBlockID(nif.GetRootNode());
	for (size_t i = 0; i < names.size(); ++i)
		skin->boneRefs.AddBlockRef(NIF_NPOS);
	shape->SkinInstanceRef()->index = nif.GetHeader().AddBlock(std::move(skin));

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	REQUIRE(anim.GetMostBonesPerVertex("Cloth") == 1);

	// Vertex 0 is weighted to six bones, the strongest first
	const float vertexWeights[] = {0.3f, 0.25f, 0.2f, 0.12f, 0.08f, 0.05f};
	for (int i = 0; i < 6; i++) {
		std::unordered_map<uint16_t, float> weights;
		anim.GetWeights("Cloth", names[i], weights);
		weights[0] = vertexWeights[i];
		anim.SetWeights("Cloth", names[i], weights);
	}
	REQUIRE(anim.GetMostBonesPerVertex("Cloth") == 6);

	NifFile output(nif);
	auto outputShape = output.FindBlockByName<BSGeometry>("Cloth");
	REQUIRE(outputShape);
	auto outputData = dynamic_cast<BSGeometryMeshData*>(outputShape->GetGeomData());
	REQUIRE(outputData);

	SECTION("Without a lock, the most any vertex needs") {
		// Weights that round to 0 in the mesh don't count
		std::unordered_map<uint16_t, float> weights{{0, 0.000001f}};
		anim.SetWeights("Cloth", names[6], weights);
		REQUIRE(anim.GetMostBonesPerVertex("Cloth") == 6);

		REQUIRE(anim.WriteToNif(&output));
		REQUIRE(output.GetShapeBonesPerVertex(outputShape) == 6);

		std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
		REQUIRE(output.Save(stream) == 0);
		NifFile reloaded;
		REQUIRE(reloaded.Load(stream) == 0);
		auto reloadedShape = reloaded.FindBlockByName<BSGeometry>("Cloth");
		REQUIRE(reloadedShape);
		REQUIRE(reloaded.GetShapeBonesPerVertex(reloadedShape) == 6);
		std::unordered_map<uint16_t, float> reloadedWeights;
		reloaded.GetShapeBoneWeights(reloadedShape, 5, reloadedWeights);
		REQUIRE(reloadedWeights.count(0) == 1);
		CHECK(std::abs(reloadedWeights[0] - 0.05f) < 0.001f);
	}

	SECTION("All 8 bones of a vertex are kept") {
		for (int i = 6; i < 9; i++) {
			std::unordered_map<uint16_t, float> weights{{0, 0.05f}};
			anim.SetWeights("Cloth", names[i], weights);
		}
		REQUIRE(anim.GetMostBonesPerVertex("Cloth") == 9);

		REQUIRE(anim.WriteToNif(&output));
		REQUIRE(output.GetShapeBonesPerVertex(outputShape) == 8);
		const auto& vw = outputData->skinWeights[0];
		REQUIRE(vw.size() == 8);
		CHECK(vw[7].weight > 0);
	}

	SECTION("A lock below the need keeps the strongest bones") {
		REQUIRE(anim.WriteToNif(&output, "", [](const std::string& shapeName) { return shapeName == "Cloth" ? uint8_t(4) : uint8_t(0); }));
		REQUIRE(output.GetShapeBonesPerVertex(outputShape) == 4);
		const auto& vw = outputData->skinWeights[0];
		REQUIRE(vw.size() == 4);
		uint32_t sum = 0;
		for (size_t i = 0; i < vw.size(); i++) {
			CHECK(vw[i].boneIndex == i);
			sum += vw[i].weight;
		}
		CHECK(sum >= 65533);
		CHECK(sum <= 65537);
		CHECK(vw[0].weight == static_cast<uint16_t>(std::lround(0.3f / 0.87f * 65535.0f)));
	}

	SECTION("A lock above the need adds empty slots") {
		REQUIRE(anim.WriteToNif(&output, "", [](const std::string&) { return uint8_t(8); }));
		REQUIRE(output.GetShapeBonesPerVertex(outputShape) == 8);
		REQUIRE(outputData->skinWeights[1].size() == 8);
		CHECK(outputData->skinWeights[1][0].boneIndex == 1);
		CHECK(outputData->skinWeights[1][1].weight == 0);
	}
}

TEST_CASE("Starfield import keeps SkinAttach bones when a node shares a bone's name", "[anim][import][starfield]") {
	AnimSkeleton::getInstance().Clear();
	NifFile source;
	source.Create(NiVersion::getSF());
	// Physics helper node that has the same name as the first skin bone
	source.AddNode("Cloth_A", MatTransform());
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
	mesh->meshData.skinWeights = {{{0, 65535}}, {{1, 65535}}, {{2, 65535}}};
	source.GetRootNode()->childRefs.AddBlockRef(source.GetHeader().AddBlock(std::move(shapeBlock)));

	const std::vector<std::string> names{"Cloth_A", "Cloth_B", "Cloth_C"};
	auto attach = std::make_unique<SkinAttach>();
	attach->bones.resize(3);
	for (uint32_t i = 0; i < names.size(); ++i)
		attach->bones[i].get() = names[i];
	source.AssignExtraData(shape, std::move(attach));
	auto data = std::make_unique<BSSkinBoneData>();
	data->nBones = 3;
	data->boneXforms.resize(3);
	auto skin = std::make_unique<BSSkinInstance>();
	skin->dataRef.index = source.GetHeader().AddBlock(std::move(data));
	skin->targetRef.index = source.GetBlockID(source.GetRootNode());
	for (size_t i = 0; i < names.size(); ++i)
		skin->boneRefs.AddBlockRef(NIF_NPOS);
	shape->SkinInstanceRef()->index = source.GetHeader().AddBlock(std::move(skin));

	NifFile work;
	work.Create(NiVersion::getSF());
	auto cloned = work.CloneShape(shape, "Cloth", &source);
	REQUIRE(cloned);

	std::vector<std::string> clonedNames;
	work.GetShapeBoneList(cloned, clonedNames);
	CHECK(clonedNames == names);
	auto clonedSkin = work.GetHeader().GetBlock<BSSkinInstance>(cloned->SkinInstanceRef());
	REQUIRE(clonedSkin);
	CHECK(clonedSkin->boneRefs.GetSize() == names.size());
	for (const auto& boneRef : clonedSkin->boneRefs)
		CHECK(boneRef.IsEmpty());

	// A ref to the same-named node, as written by older versions, must not hide the other bones
	auto node = work.FindBlockByName<NiNode>("Cloth_A");
	REQUIRE(node);
	clonedSkin->boneRefs.SetBlockRef(0, work.GetBlockID(node));
	work.GetShapeBoneList(cloned, clonedNames);
	CHECK(clonedNames == names);

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&work, cloned, true));
	for (uint16_t i = 0; i < names.size(); ++i) {
		INFO(names[i]);
		std::unordered_map<uint16_t, float> weights;
		anim.GetWeights("Cloth", names[i], weights);
		CHECK(weights == std::unordered_map<uint16_t, float>{{i, 1.0f}});
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

TEST_CASE("Cloned shapes keep their weights after bones changed since loading", "[anim][clone]") {
	auto& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	const std::vector<std::string> bones{"Pauldron", "Pelvis", "Thigh"};
	for (auto& bn : bones)
		skeleton.AddStandardBone(bn).boneName = bn;
	skeleton.AddStandardBone("Calf").boneName = "Calf";

	// NiTriShape with NiSkinData, BSTriShape with vertex weights, and FO4's BSSkinInstance
	const NiVersion version = GENERATE(NiVersion::getSK(), NiVersion::getSSE(), NiVersion::getFO4());
	CAPTURE(version.GetVersionInfo());
	NifFile nif;
	nif.Create(version);
	const std::vector<Vector3> verts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	const std::vector<Triangle> tris{{0, 1, 2}};
	std::vector<int> ids;
	for (auto& bn : bones)
		ids.push_back(static_cast<int>(nif.GetBlockID(nif.AddNode(bn, MatTransform()))));
	nif.AddNode("Calf", MatTransform());

	auto armor = nif.CreateShapeFromData("Armor", &verts, &tris, nullptr);
	nif.CreateSkinning(armor);
	nif.SetShapeBoneIDList(armor, ids);
	// The separated pauldrons still use their bone, so its node stays in the NIF
	auto pauldrons = nif.CreateShapeFromData("Pauldrons", &verts, &tris, nullptr);
	nif.CreateSkinning(pauldrons);
	std::vector<int> pauldronIds{ids[0]};
	nif.SetShapeBoneIDList(pauldrons, pauldronIds);

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	std::unordered_map<uint16_t, float> pelvis{{0, 1.0f}};
	std::unordered_map<uint16_t, float> thigh{{1, 1.0f}};
	std::unordered_map<uint16_t, float> calf{{2, 1.0f}};
	anim.SetWeights("Armor", "Pelvis", pelvis);
	anim.SetWeights("Armor", "Thigh", thigh);

	std::vector<std::string> expectedBones;
	SECTION("A bone left the shape") {
		// CleanupBones after deleting vertices or saving
		REQUIRE(anim.RemoveShapeBone("Armor", "Pauldron"));
		expectedBones = {"Pelvis", "Thigh"};
	}
	SECTION("A bone was added to the shape") {
		REQUIRE(anim.AddShapeBone("Armor", "Calf"));
		anim.SetWeights("Armor", "Calf", calf);
		expectedBones = {"Pauldron", "Pelvis", "Thigh", "Calf"};
	}
	SECTION("The NIF has none of the bones yet") {
		// Skinning created in the editor, e.g. weights copied onto an imported mesh
		std::vector<int> none;
		nif.SetShapeBoneIDList(armor, none);
		expectedBones = bones;
	}

	// Separating or duplicating the shape
	REQUIRE(anim.CloneShape(&nif, armor, "Cuirass"));
	REQUIRE(nif.CloneShape(armor, "Cuirass"));
	CHECK(anim.shapeBones["Cuirass"] == expectedBones);

	// Export the way the project does, and read the file back
	NifFile output(nif);
	anim.WriteToNif(&output);
	for (auto& s : output.GetShapes())
		output.UpdateSkinPartitions(s);
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	REQUIRE(output.Save(stream) == 0);
	NifFile reloaded;
	REQUIRE(reloaded.Load(stream) == 0);

	// Both the separated shape and the one it came from
	for (const std::string shapeName : {"Cuirass", "Armor"}) {
		CAPTURE(shapeName);
		auto exported = reloaded.FindBlockByName<NiShape>(shapeName);
		REQUIRE(exported);
		std::vector<std::string> exportedBones;
		reloaded.GetShapeBoneList(exported, exportedBones);
		REQUIRE(exportedBones == expectedBones);

		auto exportedWeights = [&](const std::string& bone) {
			auto it = std::find(exportedBones.begin(), exportedBones.end(), bone);
			std::unordered_map<uint16_t, float> weights;
			reloaded.GetShapeBoneWeights(exported, static_cast<int>(it - exportedBones.begin()), weights);
			return weights;
		};
		CHECK(exportedWeights("Pelvis") == pelvis);
		CHECK(exportedWeights("Thigh") == thigh);
		if (std::find(expectedBones.begin(), expectedBones.end(), "Calf") != expectedBones.end())
			CHECK(exportedWeights("Calf") == calf);
		if (std::find(expectedBones.begin(), expectedBones.end(), "Pauldron") != expectedBones.end())
			CHECK(exportedWeights("Pauldron").empty());
	}

	skeleton.Clear();
}

TEST_CASE("Cloned Starfield shapes keep their weights after bones changed since loading", "[anim][clone][starfield]") {
	AnimSkeleton::getInstance().Clear();
	NifFile nif;
	nif.Create(NiVersion::getSF());

	// BSGeometry binds by the SkinAttach names, there are no bone nodes
	const std::vector<std::string> bones{"Pauldron", "Pelvis", "Thigh"};
	auto block = std::make_unique<BSGeometry>();
	auto armor = block.get();
	armor->name.get() = "Armor";
	armor->SetInternalGeomData(true);
	auto mesh = armor->AddMesh();
	mesh->meshData.version = 2;
	mesh->meshData.scale = 1.0f;
	mesh->meshData.nVertices = 3;
	mesh->meshData.vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	mesh->meshData.nWeightsPerVert = 1;
	mesh->meshData.skinWeights = {{{0, 65535}}, {{0, 65535}}, {{0, 65535}}};
	nif.GetRootNode()->childRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(block)));

	auto attach = std::make_unique<SkinAttach>();
	attach->bones.resize(bones.size());
	for (size_t i = 0; i < bones.size(); ++i)
		attach->bones[i].get() = bones[i];
	nif.AssignExtraData(armor, std::move(attach));
	auto data = std::make_unique<BSSkinBoneData>();
	data->nBones = static_cast<uint32_t>(bones.size());
	data->boneXforms.resize(bones.size());
	auto skin = std::make_unique<BSSkinInstance>();
	skin->dataRef.index = nif.GetHeader().AddBlock(std::move(data));
	skin->targetRef.index = nif.GetBlockID(nif.GetRootNode());
	for (size_t i = 0; i < bones.size(); ++i)
		skin->boneRefs.AddBlockRef(NIF_NPOS);
	armor->SkinInstanceRef()->index = nif.GetHeader().AddBlock(std::move(skin));

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	REQUIRE(anim.shapeBones["Armor"] == bones);
	std::unordered_map<uint16_t, float> none;
	std::unordered_map<uint16_t, float> pelvis{{0, 1.0f}};
	std::unordered_map<uint16_t, float> thigh{{1, 1.0f}};
	std::unordered_map<uint16_t, float> calf{{2, 1.0f}};
	anim.SetWeights("Armor", "Pauldron", none);
	anim.SetWeights("Armor", "Pelvis", pelvis);
	anim.SetWeights("Armor", "Thigh", thigh);

	std::vector<std::string> expectedBones;
	SECTION("A bone left the shape") {
		// Undoing a stroke that brought the bone
		REQUIRE(anim.RemoveShapeBone("Armor", "Pauldron"));
		expectedBones = {"Pelvis", "Thigh"};
	}
	SECTION("A bone was added to the shape") {
		REQUIRE(anim.AddShapeBone("Armor", "Calf", MatTransform()));
		anim.SetWeights("Armor", "Calf", calf);
		expectedBones = {"Pauldron", "Pelvis", "Thigh", "Calf"};
	}

	REQUIRE(anim.CloneShape(&nif, armor, "Cuirass"));
	REQUIRE(nif.CloneShape(armor, "Cuirass"));
	CHECK(anim.shapeBones["Cuirass"] == expectedBones);

	NifFile output(nif);
	CHECK(anim.WriteToNif(&output));
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	REQUIRE(output.Save(stream) == 0);
	NifFile reloaded;
	REQUIRE(reloaded.Load(stream) == 0);

	for (const std::string shapeName : {"Cuirass", "Armor"}) {
		CAPTURE(shapeName);
		auto exported = reloaded.FindBlockByName<BSGeometry>(shapeName);
		REQUIRE(exported);
		std::vector<std::string> exportedBones;
		reloaded.GetShapeBoneList(exported, exportedBones);
		REQUIRE(exportedBones == expectedBones);

		auto exportedWeights = [&](const std::string& bone) {
			auto it = std::find(exportedBones.begin(), exportedBones.end(), bone);
			std::unordered_map<uint16_t, float> weights;
			reloaded.GetShapeBoneWeights(exported, static_cast<int>(it - exportedBones.begin()), weights);
			return weights;
		};
		CHECK(exportedWeights("Pelvis") == pelvis);
		CHECK(exportedWeights("Thigh") == thigh);
		if (std::find(expectedBones.begin(), expectedBones.end(), "Calf") != expectedBones.end())
			CHECK(exportedWeights("Calf") == calf);
	}

	AnimSkeleton::getInstance().Clear();
}

TEST_CASE("Setting a shape's skin keeps the bone references and nodes in step", "[anim][clone]") {
	auto& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	const std::vector<std::string> bones{"Pauldron", "Pelvis", "Thigh"};
	for (auto& bn : bones)
		skeleton.AddStandardBone(bn).boneName = bn;

	const NiVersion version = GENERATE(NiVersion::getSK(), NiVersion::getSSE(), NiVersion::getFO4());
	CAPTURE(version.GetVersionInfo());
	NifFile nif;
	nif.Create(version);
	const std::vector<Vector3> verts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	const std::vector<Triangle> tris{{0, 1, 2}};
	std::vector<int> ids;
	for (auto& bn : bones)
		ids.push_back(static_cast<int>(nif.GetBlockID(nif.AddNode(bn, MatTransform()))));
	auto armor = nif.CreateShapeFromData("Armor", &verts, &tris, nullptr);
	nif.CreateSkinning(armor);
	nif.SetShapeBoneIDList(armor, ids);
	// Another shape keeps Thigh in use
	auto boots = nif.CreateShapeFromData("Boots", &verts, &tris, nullptr);
	nif.CreateSkinning(boots);
	std::vector<int> bootIds{ids[2]};
	nif.SetShapeBoneIDList(boots, bootIds);

	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));

	AnimSkin skin;
	skin.boneNames["Pelvis"] = 0;
	skin.boneWeights[0].weights = {{0, 1.0f}};
	anim.SetShapeSkin("Armor", {"Pelvis"}, skin);

	CHECK(anim.shapeBones["Armor"] == std::vector<std::string>{"Pelvis"});
	CHECK(anim.GetShapeBoneIndex("Armor", "Pelvis") == 0);
	CHECK(skeleton.GetBoneRefCount("Pauldron") == 0);
	CHECK(skeleton.GetBoneRefCount("Pelvis") == 1);
	CHECK(skeleton.GetBoneRefCount("Thigh") == 1);
	CHECK_FALSE(nif.FindBlockByName<NiNode>("Pauldron"));
	CHECK(nif.FindBlockByName<NiNode>("Pelvis"));
	CHECK(nif.FindBlockByName<NiNode>("Thigh"));

	skeleton.Clear();
}

namespace {
MatTransform ClothTestTransform(Vector3 translation, Vector3 rotation, float scale = 1.0f) {
	MatTransform t;
	t.translation = translation;
	t.rotation = RotVecToMat(rotation);
	t.scale = scale;
	return t;
}

const MatTransform clothSourceSpace = ClothTestTransform({0.3f, -0.2f, 0.1f}, {0, 0, 0.2f}, 1.1f);
const MatTransform clothTargetSpace = ClothTestTransform({-0.2f, 0.4f, 0.1f}, {0.2f, -0.1f, 0.3f}, 0.8f);
const MatTransform clothGlobalToBoneA = ClothTestTransform({0.2f, 0.3f, -0.8f}, {0.3f, 0.1f, -0.2f}, 0.9f);
const MatTransform clothGlobalToBoneB = ClothTestTransform({-0.4f, 0.5f, 0.7f}, {-0.1f, 0.4f, 0.2f}, 1.2f);
const MatTransform clothExistingBind = ClothTestTransform({0.7f, -0.6f, 0.2f}, {0.4f, 0.2f, 0.1f});

std::filesystem::path ClothTestDirectory() {
	const char* dir = std::getenv("BSOS_CLOTH_COPY_DIR");
	REQUIRE(dir);
	return std::filesystem::u8path(dir);
}

void AddClothTestShape(NifFile& nif, const std::string& name, const MatTransform& globalToSkin, bool reference, bool existing = false) {
	auto block = std::make_unique<BSGeometry>();
	auto shape = block.get();
	shape->name.get() = name;
	shape->SetInternalGeomData(true);
	auto meshSlot = shape->AddMesh();
	meshSlot->numVerts = 3;
	meshSlot->triSize = 3;
	meshSlot->flags = 64;
	auto& mesh = meshSlot->meshData;
	mesh.version = 2;
	mesh.scale = 1.0f;
	mesh.nVertices = 3;
	const std::vector<Vector3> vertices{{0, 0, 0}, {10, 0, 0}, {0, 10, 0}};
	mesh.Create(nif.GetHeader().GetVersion(), &vertices, nullptr, nullptr, nullptr);
	auto geometryTransform = globalToSkin;
	geometryTransform.translation *= sfHavokScale;
	for (auto& vertex : mesh.vertices)
		vertex = geometryTransform.ApplyTransform(vertex);
	mesh.nTriIndices = 3;
	mesh.tris = {{0, 1, 2}};
	mesh.nUV1 = 3;
	mesh.uvSets = {{{0, 0}, {1, 0}, {0, 1}}};
	mesh.nNormals = 3;
	mesh.normals.assign(3, globalToSkin.ApplyTransformToDir({0, 0, 1}));
	mesh.nWeightsPerVert = 4;
	mesh.skinWeights.resize(3, std::vector<BSGeometryMeshData::BoneWeight>(4));
	for (uint16_t v = 0; v < 3; ++v)
		mesh.skinWeights[v][0] = {static_cast<uint16_t>(reference ? (v + 1) % 3 : 0), 65535};
	nif.GetRootNode()->childRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(block)));
	nif.CreateSkinning(shape);
	std::vector<std::string> names{"COM"};
	if (reference || existing)
		names.push_back("Cloth_A");
	if (reference)
		names.push_back("Cloth_B");
	nif.SetShapeBoneList(shape, names);
	nif.SetShapeTransformSkinToBone(shape, 0, globalToSkin.InverseTransform());
	if (reference || existing)
		nif.SetShapeTransformSkinToBone(shape, 1, existing ? clothExistingBind : clothGlobalToBoneA.ComposeTransforms(globalToSkin.InverseTransform()));
	if (reference)
		nif.SetShapeTransformSkinToBone(shape, 2, clothGlobalToBoneB.ComposeTransforms(globalToSkin.InverseTransform()));
}
} // namespace

TEST_CASE("Changing Starfield skin coordinates preserves missing cloth bone binds", "[anim][starfield][cloth-copy]") {
	auto& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	skeleton.AddStandardBone("COM").boneName = "COM";
	NifFile nif;
	nif.Create(NiVersion::getSF());
	AddClothTestShape(nif, "Reference", clothSourceSpace, true);
	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	REQUIRE_FALSE(skeleton.GetBonePtr("Cloth_A"));
	REQUIRE_FALSE(skeleton.GetBonePtr("Cloth_B"));
	anim.ChangeGlobalToSkinTransform("Reference", clothTargetSpace);
	for (const auto& bone : {"COM", "Cloth_A", "Cloth_B"}) {
		INFO(bone);
		MatTransform actual;
		REQUIRE(anim.GetXFormSkinToBone("Reference", bone, actual));
		const auto globalToBone = bone == std::string("COM") ? MatTransform() : bone == std::string("Cloth_A") ? clothGlobalToBoneA : clothGlobalToBoneB;
		CHECK(actual.IsNearlyEqualTo(globalToBone.ComposeTransforms(clothTargetSpace.InverseTransform())));
	}
	skeleton.Clear();
}

TEST_CASE("Copied cloth binds survive deferred application without changing existing binds", "[anim][starfield][cloth-copy]") {
	auto& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	skeleton.AddStandardBone("COM").boneName = "COM";
	NifFile nif;
	nif.Create(NiVersion::getSF());
	AddClothTestShape(nif, "Reference", clothSourceSpace, true);
	AddClothTestShape(nif, "Target", clothTargetSpace, false);
	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	const auto expected = clothGlobalToBoneA.ComposeTransforms(clothTargetSpace.InverseTransform());

	SECTION("A captured bind remains valid after removing the reference and readding the bone") {
		UndoStateBoneWeights copied;
		copied.boneName = "Cloth_A";
		copied.skinToBone = anim.GetMissingBoneTransformForCopy("Reference", "Target", copied.boneName);
		REQUIRE(copied.skinToBone);
		anim.ClearShape("Reference");
		for (int application = 0; application < 2; ++application) {
			REQUIRE(anim.AddShapeBone("Target", copied.boneName, copied.skinToBone));
			MatTransform actual;
			REQUIRE(anim.GetXFormSkinToBone("Target", copied.boneName, actual));
			CHECK(actual.IsNearlyEqualTo(expected));
			// A subsequent copy must not overwrite the target's existing bind.
			CHECK_FALSE(anim.AddShapeBone("Target", copied.boneName, MatTransform()));
			REQUIRE(anim.GetXFormSkinToBone("Target", copied.boneName, actual));
			CHECK(actual.IsNearlyEqualTo(expected));
			REQUIRE(anim.RemoveShapeBone("Target", copied.boneName));
		}
	}
	SECTION("Known skeleton bones keep the normal recalculation path") {
		auto& bone = skeleton.AddStandardBone("Cloth_A");
		bone.boneName = "Cloth_A";
		bone.xformToGlobal = clothGlobalToBoneA.InverseTransform();
		anim.SetXFormSkinToBone("Reference", "Cloth_A", MatTransform());
		auto copied = anim.GetMissingBoneTransformForCopy("Reference", "Target", "Cloth_A");
		CHECK_FALSE(copied);
		REQUIRE(anim.AddShapeBone("Target", "Cloth_A", copied));
		MatTransform actual;
		REQUIRE(anim.GetXFormSkinToBone("Target", "Cloth_A", actual));
		CHECK(actual.IsNearlyEqualTo(expected));
	}
	SECTION("Missing source data is not replaced with a fabricated bind") {
		CHECK_FALSE(anim.GetMissingBoneTransformForCopy("Missing", "Target", "Cloth_A"));
		CHECK_FALSE(anim.GetMissingBoneTransformForCopy("Reference", "Missing", "Cloth_A"));
		CHECK_FALSE(anim.GetMissingBoneTransformForCopy("Reference", "Target", "Missing"));
		anim.shapeSkinning.at("Reference").boneWeights.erase(1);
		CHECK_FALSE(anim.GetMissingBoneTransformForCopy("Reference", "Target", "Cloth_A"));
		CHECK(anim.shapeSkinning.size() == 2);
		CHECK(anim.shapeSkinning.at("Reference").boneWeights.size() == 2);
	}
	SECTION("Other games do not use the Starfield fallback") {
		NifFile other;
		other.Create(NiVersion::getFO4());
		anim.SetRefNif(&other);
		CHECK_FALSE(anim.GetMissingBoneTransformForCopy("Reference", "Target", "Cloth_A"));
		anim.SetRefNif(&nif);
	}
	skeleton.Clear();
}

TEST_CASE("Starfield bones added without a bind take it from another shape", "[anim][starfield][cloth-copy]") {
	auto& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	skeleton.AddStandardBone("COM").boneName = "COM";
	NifFile nif;
	nif.Create(NiVersion::getSF());
	AddClothTestShape(nif, "Reference", clothSourceSpace, true);
	AddClothTestShape(nif, "Target", clothTargetSpace, false);
	AnimInfo anim;
	REQUIRE(anim.LoadFromNif(&nif));
	REQUIRE(anim.WriteToNif(&nif));

	SECTION("Painting a bone that another shape has uses that shape's bind") {
		REQUIRE(anim.AddShapeBone("Target", "Cloth_B"));
		MatTransform actual;
		REQUIRE(anim.GetXFormSkinToBone("Target", "Cloth_B", actual));
		CHECK(actual.IsNearlyEqualTo(clothGlobalToBoneB.ComposeTransforms(clothTargetSpace.InverseTransform())));
		CHECK(anim.WriteToNif(&nif));
	}
	SECTION("A bone no shape has a bind for is reported on export") {
		REQUIRE(anim.AddShapeBone("Target", "Cloth_New"));
		CHECK_FALSE(anim.WriteToNif(&nif));

		SECTION("The guess is not copied to other shapes") {
			REQUIRE(anim.AddShapeBone("Reference", "Cloth_New"));
			anim.SetXFormSkinToBone("Target", "Cloth_New", MatTransform());
			CHECK_FALSE(anim.WriteToNif(&nif));
		}
		SECTION("Changing skin coordinates keeps the bind a guess") {
			anim.ChangeGlobalToSkinTransform("Target", clothSourceSpace);
			CHECK_FALSE(anim.WriteToNif(&nif));
		}
		SECTION("Setting the bind resolves it") {
			anim.SetXFormSkinToBone("Target", "Cloth_New", MatTransform());
			CHECK(anim.WriteToNif(&nif));
		}
	}
	skeleton.Clear();
}

// These two hidden cases are driven by Run-ClothWeightCopy.ps1. The application,
// not the test, performs CopyBoneWeights/TransferWeights and exports the result.
TEST_CASE("Create cloth weight copy workflow fixture", "[.cloth-copy-setup]") {
	const auto dir = ClothTestDirectory();
	std::filesystem::create_directories(dir);
	NifFile skeleton;
	skeleton.Create(NiVersion::getSF());
	skeleton.GetRootNode()->name.get() = "Root";
	skeleton.AddNode("COM", MatTransform());
	REQUIRE(skeleton.Save(dir / "skeleton.nif") == 0);
	NifFile nif;
	nif.Create(NiVersion::getSF());
	AddClothTestShape(nif, "Reference", clothSourceSpace, true);
	AddClothTestShape(nif, "SameSpace", clothSourceSpace, false);
	AddClothTestShape(nif, "DifferentSpace", clothTargetSpace, false);
	AddClothTestShape(nif, "ExistingBind", clothTargetSpace, false, true);
	REQUIRE(nif.Save(dir / "input.nif") == 0);
	NifFile reloaded;
	REQUIRE(reloaded.Load(dir / "input.nif") == 0);
	for (const auto& name : {"Reference", "SameSpace", "DifferentSpace", "ExistingBind"}) {
		INFO(name);
		auto shape = reloaded.FindBlockByName<BSGeometry>(name);
		REQUIRE(shape);
		REQUIRE(shape->GetNumVertices() == 3);
		std::unordered_map<uint16_t, float> weights;
		reloaded.GetShapeBoneWeights(shape, name == std::string("Reference") ? 1 : 0, weights);
		REQUIRE(weights.at(0) == 1.0f);
	}
	AddClothTestShape(nif, "Unskinned", MatTransform(), false);
	auto unskinned = nif.FindBlockByName<BSGeometry>("Unskinned");
	nif.DeleteSkinning(unskinned);
	auto mesh = static_cast<BSGeometryMeshData*>(unskinned->GetGeomData());
	mesh->nWeightsPerVert = 0;
	mesh->skinWeights.clear();
	REQUIRE_FALSE(unskinned->IsSkinned());
	REQUIRE(nif.Save(dir / "input-unskinned.nif") == 0);
}

TEST_CASE("Verify cloth weight copy workflow export", "[.cloth-copy-verify]") {
	const auto dir = ClothTestDirectory();
	NifFile skeleton;
	REQUIRE(skeleton.Load(dir / "skeleton.nif") == 0);
	REQUIRE_FALSE(skeleton.FindBlockByName<NiNode>("Cloth_A"));
	REQUIRE_FALSE(skeleton.FindBlockByName<NiNode>("Cloth_B"));
	NifFile nif;
	REQUIRE(nif.Load(dir / "output.nif") == 0);
	NifFile input;
	REQUIRE(input.Load(dir / "workflow-input.nif") == 0);
	std::vector<std::string> shapes{"Reference", "SameSpace", "DifferentSpace", "ExistingBind"};
	if (input.FindBlockByName<BSGeometry>("Unskinned"))
		shapes.push_back("Unskinned");
	REQUIRE(nif.GetShapes().size() == shapes.size());
	for (const auto& name : shapes) {
		INFO(name);
		auto shape = nif.FindBlockByName<BSGeometry>(name);
		REQUIRE(shape);
		REQUIRE(shape->GetNumVertices() == 3);
		auto skin = nif.GetHeader().GetBlock<BSSkinInstance>(shape->SkinInstanceRef());
		REQUIRE(skin);
		for (const auto& boneRef : skin->boneRefs)
			CHECK(boneRef.IsEmpty());
		auto boneData = nif.GetHeader().GetBlock(skin->dataRef);
		REQUIRE(boneData);
		REQUIRE(boneData->boneXforms.size() == 3);
		std::vector<std::string> names;
		nif.GetShapeBoneList(shape, names);
		REQUIRE(names.size() == 3);
		const auto space = name == "Unskinned" ? MatTransform() : (name == "Reference" || name == "SameSpace") ? clothSourceSpace : clothTargetSpace;
		for (const std::string bone : {"COM", "Cloth_A", "Cloth_B"}) {
			INFO(bone);
			auto it = std::find(names.begin(), names.end(), bone);
			REQUIRE(it != names.end());
			const int index = static_cast<int>(it - names.begin());
			MatTransform actual;
			REQUIRE(nif.GetShapeTransformSkinToBone(shape, index, actual));
			MatTransform globalToBone;
			if (bone == "Cloth_A")
				globalToBone = clothGlobalToBoneA;
			if (bone == "Cloth_B")
				globalToBone = clothGlobalToBoneB;
			const auto expected = (name == "ExistingBind" && bone == "Cloth_A") ? clothExistingBind : globalToBone.ComposeTransforms(space.InverseTransform());
			CHECK(actual.IsNearlyEqualTo(expected));
			std::unordered_map<uint16_t, float> weights;
			nif.GetShapeBoneWeights(shape, index, weights);
			const uint16_t vertex = bone == "COM" ? 2 : bone == "Cloth_A" ? 0 : 1;
			CHECK(weights == std::unordered_map<uint16_t, float>{{vertex, 1.0f}});
			// The one weighted vertex gives a zero-radius bone-local bound, in meters.
			const auto& bounds = boneData->boneXforms[index].bounds;
			const auto& verts = *nif.GetVertsForShape(shape);
			CHECK(bounds.center.DistanceTo(expected.ApplyTransform(verts[vertex] / sfHavokScale)) < 0.0001f);
			CHECK(bounds.radius < 0.0001f);
		}
	}
}

TEST_CASE("Pose rotations turn model space normals in mesh coordinates", "[anim][pose]") {
	Mesh m;
	m.nVerts = 3;
	m.modelSpace = true;

	// 90 degrees around the NIF Z axis, a blend of that with no rotation, and a vertex without weights
	const Matrix3 turn(0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	const std::vector<Matrix3> rotations{turn, (turn + Matrix3()) * 0.5f, Matrix3()};
	m.SetPoseRotations(rotations);
	REQUIRE(m.poseRotations);

	auto turnInMesh = [&](int vertex, const Vector3& nifDir) {
		const glm::vec4& r = m.poseRotations[vertex];
		const glm::quat q(r.w, r.x, r.y, r.z);
		const Vector3 meshDir = Mesh::TransformDirNifToMesh(nifDir);
		const glm::vec3 turned = q * glm::vec3(meshDir.x, meshDir.y, meshDir.z);
		return Vector3(turned.x, turned.y, turned.z);
	};

	for (const Vector3& dir : {Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)}) {
		CHECK(turnInMesh(0, dir).IsNearlyEqualTo(Mesh::TransformDirNifToMesh(turn * dir)));
		CHECK(turnInMesh(2, dir).IsNearlyEqualTo(Mesh::TransformDirNifToMesh(dir)));
	}

	// The blend is made a proper rotation again, halfway around
	const Vector3 halfway = turnInMesh(1, Vector3(1.0f, 0.0f, 0.0f));
	CHECK(halfway.IsNearlyEqualTo(Mesh::TransformDirNifToMesh(Vector3(1.0f, 1.0f, 0.0f) / std::sqrt(2.0f))));
	for (int i = 0; i < m.nVerts; ++i) {
		CHECK(m.poseRotations[i].w >= 0.0f);
		CHECK(std::fabs(glm::length(m.poseRotations[i]) - 1.0f) < 0.0001f);
	}

	// Tangent space normals have no use for them, and neither do rotations for other vertices
	m.SetPoseRotations(std::vector<Matrix3>(2));
	CHECK(!m.poseRotations);

	m.SetPoseRotations(rotations);
	m.modelSpace = false;
	m.SetPoseRotations(rotations);
	CHECK(!m.poseRotations);
}

TEST_CASE("Mirroring a transform on X gives its left/right counterpart", "[anim][mirror]") {
	MatTransform t;
	t.translation = Vector3(1.0f, 2.0f, 3.0f);
	t.rotation = RotVecToMat(Vector3(0.3f, -0.5f, 0.7f));
	t.scale = 1.5f;

	MatTransform m = MirrorTransformX(t);
	CHECK(m.translation == Vector3(-1.0f, 2.0f, 3.0f));
	CHECK(m.scale == 1.5f);
	// Still a rotation, not a reflection
	CHECK(std::fabs(m.rotation.Determinant() - 1.0f) < 1e-4f);
	// Its Y and Z axes are the mirror images of the original's
	Vector3 y = t.rotation * Vector3(0.0f, 1.0f, 0.0f);
	Vector3 my = m.rotation * Vector3(0.0f, 1.0f, 0.0f);
	CHECK(my.IsNearlyEqualTo(Vector3(-y.x, y.y, y.z)));
	// Mirroring twice gives the original back
	MatTransform twice = MirrorTransformX(m);
	CHECK(twice.translation == t.translation);
	CHECK(twice.rotation == t.rotation);

	// Mirroring a parent and its child gives the mirrored child's global transform, so chains stay chains
	MatTransform child;
	child.translation = Vector3(-4.0f, 0.5f, 6.0f);
	child.rotation = RotVecToMat(Vector3(-0.2f, 0.9f, 0.1f));
	MatTransform mirroredGlobal = MirrorTransformX(t.ComposeTransforms(child));
	MatTransform composedMirrors = MirrorTransformX(t).ComposeTransforms(MirrorTransformX(child));
	CHECK(mirroredGlobal.IsNearlyEqualTo(composedMirrors));
}

TEST_CASE("Bones are sorted with parents before their children", "[anim][mirror]") {
	AnimBone root, a, b, c, other;
	a.parent = &root;
	b.parent = &a;
	c.parent = &b;
	other.parent = &root;

	std::vector<AnimBone*> sorted = SortBonesParentsFirst({&c, &a, &other, &b});
	REQUIRE(sorted.size() == 4);
	auto pos = [&](AnimBone* bone) { return std::find(sorted.begin(), sorted.end(), bone) - sorted.begin(); };
	CHECK(pos(&a) < pos(&b));
	CHECK(pos(&b) < pos(&c));
}
