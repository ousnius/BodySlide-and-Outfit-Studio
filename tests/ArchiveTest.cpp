#include "FSEngine/FSBSA.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// The corpus in tests/data/archives holds archives packed by the official tools
// (Archive.exe for BSA, Archive2 for BA2) next to the loose files they were
// packed from, so every extracted file can be checked against its source.
namespace {
using Bytes = std::vector<uint8_t>;

fs::path CorpusDir() {
	return fs::u8path(BSOS_TEST_DATA_DIR) / "archives";
}

Bytes ReadFile(const fs::path& path) {
	std::ifstream file(path, std::ios::binary);
	return Bytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Without "git lfs pull" the corpus is only a set of pointer files
bool IsLfsPointer(const fs::path& path) {
	static const std::string magic = "version https://git-lfs";
	Bytes head = ReadFile(path);
	return head.size() >= magic.size() && std::memcmp(head.data(), magic.data(), magic.size()) == 0;
}

void SkipWithoutCorpus() {
	if (!fs::exists(CorpusDir() / "BSA" / "0x69_DDS.bsa") || IsLfsPointer(CorpusDir() / "BSA" / "0x69_DDS.bsa"))
		SKIP("Archive test corpus is missing, run \"git lfs pull\"");
}

// A source file's path the way the archives store it: relative to the corpus,
// '/' separated, ASCII lowercased and non-ASCII characters as Windows-1252
// bytes (the corpus only uses the Latin-1 range of it).
std::optional<std::string> ArchiveName(const fs::path& relative) {
	std::string name;
	for (char32_t c : relative.generic_u32string()) {
		if (c > 0xFF)
			return std::nullopt;
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		name += static_cast<char>(c);
	}
	return name;
}

// Archive name -> loose source file
const std::map<std::string, fs::path>& SourceFiles() {
	static const std::map<std::string, fs::path> files = [] {
		std::map<std::string, fs::path> result;
		for (auto& entry : fs::recursive_directory_iterator(CorpusDir())) {
			if (!entry.is_regular_file())
				continue;

			fs::path relative = fs::relative(entry.path(), CorpusDir());
			std::string top = relative.begin()->u8string();
			if (top == "BSA" || top == "BA2")
				continue;

			if (auto name = ArchiveName(relative))
				result[*name] = entry.path();
		}
		return result;
	}();
	return files;
}

Bytes Extract(BSA& archive, const std::string& path) {
	wxMemoryBuffer buffer;
	if (!archive.fileContents(path, buffer))
		return {};

	auto data = static_cast<const uint8_t*>(buffer.GetData());
	return Bytes(data, data + buffer.GetDataLen());
}

std::vector<std::string> ListFiles(const BSA& archive) {
	std::vector<std::string> files;
	archive.forEachFile([&](const std::string& path, wxInt64) { files.push_back(path); });
	return files;
}

// What a DDS header describes, so headers written by different tools compare
struct DdsInfo {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mips = 0;
	uint32_t format = 0; // DXGI_FORMAT
	bool cube = false;
	size_t dataOffset = 0;
};

uint32_t U32(const Bytes& data, size_t offset) {
	uint32_t value = 0;
	std::memcpy(&value, data.data() + offset, 4);
	return value;
}

std::optional<DdsInfo> ParseDds(const Bytes& data) {
	if (data.size() < 128 || std::memcmp(data.data(), "DDS ", 4) != 0)
		return std::nullopt;

	DdsInfo info;
	info.height = U32(data, 12);
	info.width = U32(data, 16);
	info.mips = std::max(U32(data, 28), 1u);
	info.cube = (U32(data, 112) & 0x200) != 0; // DDSCAPS2_CUBEMAP
	info.dataOffset = 128;

	uint32_t pfFlags = U32(data, 80);
	std::string fourCC(reinterpret_cast<const char*>(data.data()) + 84, 4);
	if ((pfFlags & 0x4) && fourCC == "DX10") {
		if (data.size() < 148)
			return std::nullopt;
		info.format = U32(data, 128);
		info.cube |= (U32(data, 136) & 0x4) != 0; // RESOURCE_MISC_TEXTURECUBE
		info.dataOffset = 148;
	}
	else if (pfFlags & 0x4) {
		static const std::map<std::string, uint32_t> fourCCs = {
			{"DXT1", 71}, {"DXT3", 74}, {"DXT5", 77}, {"BC4U", 80}, {"ATI1", 80}, {"BC5U", 83}, {"ATI2", 83}};
		auto it = fourCCs.find(fourCC);
		if (it == fourCCs.end())
			return std::nullopt;
		info.format = it->second;
	}
	else if (U32(data, 88) == 32) {
		uint32_t red = U32(data, 92), alpha = U32(data, 104);
		if (red == 0xFF && alpha == 0xFF000000)
			info.format = 28; // R8G8B8A8_UNORM
		else if (red == 0xFF0000 && alpha == 0xFF000000)
			info.format = 87; // B8G8R8A8_UNORM
		else
			return std::nullopt;
	}
	else
		return std::nullopt;

	return info;
}

void RequireSameDds(const Bytes& extracted, const Bytes& source, const std::string& path) {
	INFO(path);
	auto got = ParseDds(extracted);
	auto want = ParseDds(source);
	REQUIRE(want);
	REQUIRE(got);
	CHECK(got->width == want->width);
	CHECK(got->height == want->height);
	CHECK(got->mips == want->mips);
	CHECK(got->format == want->format);
	CHECK(got->cube == want->cube);

	Bytes gotData(extracted.begin() + got->dataOffset, extracted.end());
	Bytes wantData(source.begin() + want->dataOffset, source.end());
	CHECK(gotData.size() == wantData.size());
	CHECK(gotData == wantData);
}

// Checks that every file the archive lists extracts to its loose source
void RequireExtractsSources(BSA& archive, size_t expectedFiles, bool rebuildsDdsHeaders) {
	auto files = ListFiles(archive);
	CHECK(files.size() == expectedFiles);
	CHECK(archive.fileCount() == expectedFiles);

	for (auto& path : files) {
		INFO(path);
		auto source = SourceFiles().find(path);
		REQUIRE(source != SourceFiles().end());
		REQUIRE(archive.hasFile(path));

		Bytes extracted = Extract(archive, path);
		Bytes expected = ReadFile(source->second);
		REQUIRE(!extracted.empty());

		if (rebuildsDdsHeaders)
			RequireSameDds(extracted, expected, path);
		else
			CHECK(extracted == expected);
	}
}

std::unique_ptr<BSA> OpenArchive(const std::string& relative) {
	auto path = CorpusDir() / fs::u8path(relative);
	auto archive = std::make_unique<BSA>(path.u8string());
	INFO(relative);
	REQUIRE(BSA::canOpen(path.u8string()));
	REQUIRE(archive->open());
	return archive;
}

struct ArchiveCase {
	const char* file;
	size_t files;
};
}

TEST_CASE("BSA archives extract to their source files", "[Archive][BSA]") {
	SkipWithoutCorpus();

	// 0x68: Fallout 3, New Vegas, Skyrim LE (zlib). 0x69: Skyrim SE (LZ4).
	auto [file, files] = GENERATE(values<ArchiveCase>({
		{"BSA/0x68_ASCII.bsa", 4},
		{"BSA/0x68_ASCII_EmbedFileNames.bsa", 4},
		{"BSA/0x68_DDS.bsa", 4},
		{"BSA/0x68_DDS_EmbedFileNames.bsa", 4},
		{"BSA/0x68_DDS_NoComp.bsa", 4},
		{"BSA/0x68_DDS_XBOX360.bsa", 4},
		{"BSA/0x68_Paths.bsa", 25},
		{"BSA/0x68_Paths_EmbedFileNames.bsa", 25},
		{"BSA/0x68_WAV_TXT.bsa", 2},
		{"BSA/0x69_ASCII.bsa", 4},
		{"BSA/0x69_ASCII_EmbedFileNames.bsa", 4},
		{"BSA/0x69_DDS.bsa", 4},
		{"BSA/0x69_DDS_EmbedFileNames.bsa", 4},
		{"BSA/0x69_DDS_NoComp.bsa", 4},
		{"BSA/0x69_DDS_XBOX360.bsa", 4},
		{"BSA/0x69_DDS_XBOX360_XMem.bsa", 4},
		{"BSA/0x69_Paths.bsa", 25},
		{"BSA/0x69_Paths_EmbedFileNames.bsa", 25},
		{"BSA/0x69_WAV_TXT.bsa", 2},
	}));

	INFO(file);
	auto archive = OpenArchive(file);
	RequireExtractsSources(*archive, files, false);
}

TEST_CASE("BSA archives with XMem compression are listed but not extracted", "[Archive][BSA]") {
	SkipWithoutCorpus();

	// XMem (Xbox 360 LZX) isn't supported. Its data must not come back as garbage.
	auto archive = OpenArchive("BSA/0x68_DDS_XBOX360_XMem.bsa");
	auto files = ListFiles(*archive);
	CHECK(files.size() == 4);

	for (auto& path : files) {
		INFO(path);
		CHECK(archive->hasFile(path));
		CHECK(Extract(*archive, path).empty());
	}
}

TEST_CASE("BA2 general archives extract to their source files", "[Archive][BA2]") {
	SkipWithoutCorpus();

	auto [file, files] = GENERATE(values<ArchiveCase>({
		{"BA2/GNRL.ba2", 14},
		{"BA2/GNRL_ASCII.ba2", 4},
		{"BA2/GNRL_NoComp.ba2", 14},
		{"BA2/GNRL_Paths.ba2", 5},
		// Version 8 (Fallout 4 next-gen update), same layout as version 1
		{"BA2/GNRL_v8.ba2", 6},
	}));

	INFO(file);
	auto archive = OpenArchive(file);
	RequireExtractsSources(*archive, files, false);
}

TEST_CASE("BA2 texture archives extract to their source textures", "[Archive][BA2]") {
	SkipWithoutCorpus();

	// DX10 archives store the texture data without its DDS header, which is
	// rebuilt on extraction
	auto [file, files] = GENERATE(values<ArchiveCase>({
		{"BA2/DDS.ba2", 23},
		{"BA2/DDS_NoComp.ba2", 23},
		{"BA2/DDS_XBOX.ba2", 23},
	}));

	INFO(file);
	auto archive = OpenArchive(file);
	RequireExtractsSources(*archive, files, true);
}

TEST_CASE("BA2 archives without a name table find files by their hashes", "[Archive][BA2]") {
	SkipWithoutCorpus();

	// Same files as GNRL.ba2, packed without the name table. The game looks
	// files up by path hashes, so lookups by name still work.
	auto named = OpenArchive("BA2/GNRL.ba2");
	auto archive = OpenArchive("BA2/GNRL_NoString.ba2");
	CHECK(archive->fileCount() == 14);
	CHECK(ListFiles(*archive).empty());

	auto files = ListFiles(*named);
	REQUIRE(files.size() == 14);
	for (auto& path : files) {
		INFO(path);
		CHECK(archive->hasFile(path));
		CHECK(Extract(*archive, path) == ReadFile(SourceFiles().at(path)));
	}

	CHECK_FALSE(archive->hasFile("dds/missing.dds"));
	CHECK_FALSE(archive->hasFile("txt/text.dds"));
}

TEST_CASE("Archive lookups ignore case and slash direction", "[Archive]") {
	SkipWithoutCorpus();

	auto bsa = OpenArchive("BSA/0x69_WAV_TXT.bsa");
	CHECK(bsa->hasFile("TXT\\Text.TXT"));
	CHECK(bsa->hasFile("txt/text.txt"));
	CHECK_FALSE(bsa->hasFile("txt/missing.txt"));

	auto ba2 = OpenArchive("BA2/GNRL.ba2");
	CHECK(ba2->hasFile("DDS\\BC1_16x16.DDS"));
	CHECK(ba2->hasFile("dds/bc1_16x16.dds"));
	CHECK_FALSE(ba2->hasFile("dds/missing.dds"));
}

TEST_CASE("Only supported archive versions can be opened", "[Archive]") {
	SkipWithoutCorpus();

	CHECK_FALSE(BSA::canOpen((CorpusDir() / "DDS" / "BC1_4x4.dds").u8string()));

	auto path = fs::temp_directory_path() / "bsos_archive_test_version.ba2";
	for (uint32_t version : {1u, 2u, 3u, 7u, 8u, 4u, 99u}) {
		{
			std::ofstream file(path, std::ios::binary);
			file.write("BTDX", 4);
			file.write(reinterpret_cast<const char*>(&version), 4);
			file.write("GNRL", 4);
		}

		INFO(version);
		bool supported = version == 1 || version == 2 || version == 3 || version == 7 || version == 8;
		CHECK(BSA::canOpen(path.u8string()) == supported);
	}
	fs::remove(path);
}
