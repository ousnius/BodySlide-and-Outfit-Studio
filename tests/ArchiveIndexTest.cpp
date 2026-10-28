#include "../src/components/ArchiveIndex.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Archive index builds a folder tree of mesh files", "[ArchiveIndex]") {
	ArchiveIndex index;
	size_t meshes0 = index.AddArchive("D:/Data/Skyrim - Meshes0.bsa", "Skyrim - Meshes0.bsa");

	REQUIRE(index.AddFile("meshes/actors/character/character assets/femalebody_1.nif", 28000, meshes0));
	REQUIRE(index.AddFile("Meshes\\Actors\\Character\\Character Assets\\FemaleHands_1.NIF", 71000, meshes0));
	REQUIRE(index.AddFile("meshes/terrain/tamriel/tamriel.4.0.0.btr", 100, meshes0));
	REQUIRE(index.AddFile("meshes/terrain/tamriel/objects/tamriel.4.0.0.bto", 100, meshes0));
	REQUIRE(index.FileCount() == 4);

	const auto& root = index.Root();
	REQUIRE(root.files.empty());
	REQUIRE(root.folders.size() == 1);

	const auto& assets = root.folders.at("meshes").folders.at("actors").folders.at("character").folders.at("character assets");
	REQUIRE(assets.files.size() == 2);

	const auto& hands = assets.files.at("femalehands_1.nif");
	REQUIRE(hands.path == "meshes/actors/character/character assets/femalehands_1.nif");
	REQUIRE(hands.size == 71000);
	REQUIRE(hands.archive == meshes0);
}

TEST_CASE("Archive index tells mesh files apart", "[ArchiveIndex]") {
	REQUIRE(ArchiveIndex::IsMeshFile("meshes/foo.nif"));
	REQUIRE(ArchiveIndex::IsMeshFile("meshes/FOO.BTR"));
	REQUIRE(ArchiveIndex::IsMeshFile("meshes/foo.bto"));
	REQUIRE_FALSE(ArchiveIndex::IsMeshFile("textures/foo.dds"));
	REQUIRE_FALSE(ArchiveIndex::IsMeshFile("meshes/foo.hkx"));
	REQUIRE_FALSE(ArchiveIndex::IsMeshFile("geometries/0123/4567.mesh"));
	REQUIRE_FALSE(ArchiveIndex::IsMeshFile("nif"));
}

TEST_CASE("Archive index lists every file type and counts extensions", "[ArchiveIndex]") {
	ArchiveIndex index;
	size_t archive = index.AddArchive("a.bsa", "a.bsa");
	REQUIRE(index.AddFile("textures/cubemaps/Chrome_E.DDS", 1, archive));
	REQUIRE(index.AddFile("textures/foo.dds", 1, archive));
	REQUIRE(index.AddFile("meshes/foo.nif", 1, archive));
	REQUIRE(index.AddFile("meshes/readme", 1, archive));
	REQUIRE(index.FileCount() == 4);

	REQUIRE(index.ExtensionCounts().at(".dds") == 2);
	REQUIRE(index.ExtensionCounts().at(".nif") == 1);
	REQUIRE(index.ExtensionCounts().at("") == 1);
	REQUIRE(index.FileCount(".dds") == 2);
	REQUIRE(index.FileCount("DDS") == 2);
	REQUIRE(index.FileCount("*.dds") == 2);
	REQUIRE(index.FileCount(" .dds ") == 2);
	REQUIRE(index.FileCount(".hkx") == 0);
	REQUIRE(index.FileCount("*") == 4);
	REQUIRE(index.FileCount("") == 4);

	REQUIRE(ArchiveIndex::Extension("meshes/foo.NIF") == ".nif");
	REQUIRE(ArchiveIndex::Extension("meshes.d/readme").empty());
	REQUIRE(ArchiveIndex::HasExtension("textures/foo.DDS", ".dds"));
	REQUIRE(ArchiveIndex::HasExtension("textures/foo.dds", "*"));
	REQUIRE_FALSE(ArchiveIndex::HasExtension("textures/foo.dds", ".nif"));
	// The extension is matched whole, not as any end of the name
	REQUIRE_FALSE(ArchiveIndex::HasExtension("textures/foodds", "dds"));
}

TEST_CASE("Archive index keeps the first archive on duplicate paths", "[ArchiveIndex]") {
	ArchiveIndex index;
	size_t first = index.AddArchive("a.ba2", "a.ba2");
	size_t second = index.AddArchive("b.ba2", "b.ba2");

	REQUIRE(index.AddFile("meshes/foo.nif", 10, first));
	REQUIRE_FALSE(index.AddFile("meshes/foo.nif", 20, second));
	REQUIRE(index.FileCount() == 1);

	const auto& entry = index.Root().folders.at("meshes").files.at("foo.nif");
	REQUIRE(entry.archive == first);
	REQUIRE(entry.size == 10);
}

TEST_CASE("Archive index filter keeps matches and their folders", "[ArchiveIndex]") {
	ArchiveIndex index;
	size_t archive = index.AddArchive("a.bsa", "a.bsa");
	index.AddFile("meshes/actors/character/character assets/femalebody_1.nif", 1, archive);
	index.AddFile("meshes/actors/character/character assets/malebody_1.nif", 1, archive);
	index.AddFile("meshes/armor/iron/cuirass_1.nif", 1, archive);

	index.AddFile("textures/actors/character/female/femalebody_1.dds", 1, archive);

	size_t matches = 0;
	auto filtered = index.Filter("FemaleBody", ".nif", &matches);
	REQUIRE(matches == 1);
	REQUIRE(filtered.folders.size() == 1);

	const auto& meshes = filtered.folders.at("meshes");
	REQUIRE(meshes.folders.count("armor") == 0);

	const auto& assets = meshes.folders.at("actors").folders.at("character").folders.at("character assets");
	REQUIRE(assets.files.size() == 1);
	REQUIRE(assets.files.count("femalebody_1.nif") == 1);

	// Folder names match too, since the full path is searched
	index.Filter("armor/iron", ".nif", &matches);
	REQUIRE(matches == 1);

	REQUIRE(index.Filter("nothing", ".nif", &matches).Empty());
	REQUIRE(matches == 0);

	// Other extensions, all files, and the extension alone without a search text
	filtered = index.Filter("FemaleBody", ".dds", &matches);
	REQUIRE(matches == 1);
	REQUIRE(filtered.folders.count("textures") == 1);
	REQUIRE(filtered.folders.count("meshes") == 0);

	index.Filter("FemaleBody", "*", &matches);
	REQUIRE(matches == 2);

	index.Filter("", ".nif", &matches);
	REQUIRE(matches == 3);
}
