/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ArchiveIndex.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace {
std::string NormalizePath(std::string path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return path;
}

bool FilterNode(const ArchiveIndex::Node& in, ArchiveIndex::Node& out, const std::string& needle, size_t& matchCount) {
	for (auto& folder : in.folders) {
		ArchiveIndex::Node child;
		if (FilterNode(folder.second, child, needle, matchCount))
			out.folders.emplace(folder.first, std::move(child));
	}

	for (auto& file : in.files) {
		if (file.second.path.find(needle) != std::string::npos) {
			out.files.emplace(file.first, file.second);
			matchCount++;
		}
	}

	return !out.Empty();
}
}

void ArchiveIndex::Clear() {
	root = Node();
	archives.clear();
	fileCount = 0;
}

size_t ArchiveIndex::AddArchive(const std::string& path, const std::string& name) {
	archives.push_back({path, name});
	return archives.size() - 1;
}

bool ArchiveIndex::AddFile(const std::string& path, int64_t size, size_t archive) {
	if (!IsMeshFile(path))
		return false;

	std::string fullPath = NormalizePath(path);

	Node* node = &root;
	size_t start = 0;
	for (;;) {
		size_t sep = fullPath.find('/', start);
		if (sep == std::string::npos)
			break;

		if (sep > start)
			node = &node->folders[fullPath.substr(start, sep - start)];

		start = sep + 1;
	}

	std::string fileName = fullPath.substr(start);
	if (fileName.empty() || node->files.count(fileName))
		return false;

	node->files.emplace(fileName, Entry{fullPath, size, archive});
	fileCount++;
	return true;
}

bool ArchiveIndex::IsMeshFile(const std::string& path) {
	static const std::array<const char*, 3> meshExtensions = {".nif", ".bto", ".btr"};

	for (const char* ext : meshExtensions) {
		size_t extLen = std::char_traits<char>::length(ext);
		if (path.size() < extLen)
			continue;

		bool match = std::equal(path.end() - extLen, path.end(), ext, [](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == b;
		});
		if (match)
			return true;
	}

	return false;
}

ArchiveIndex::Node ArchiveIndex::Filter(const std::string& needle, size_t* matchCount) const {
	Node out;
	size_t count = 0;
	FilterNode(root, out, NormalizePath(needle), count);

	if (matchCount)
		*matchCount = count;

	return out;
}
