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

// Lowercase with a leading dot, empty for "*" or nothing
std::string NormalizeExtension(std::string extension) {
	extension.erase(0, extension.find_first_not_of(" \t"));
	extension.erase(extension.find_last_not_of(" \t") + 1);
	if (extension.empty() || extension == "*" || extension == "*.*")
		return std::string();

	std::string result = NormalizePath(extension);
	if (result.rfind("*.", 0) == 0)
		result.erase(0, 1);
	if (result.front() != '.')
		result.insert(result.begin(), '.');
	return result;
}

bool EndsWith(const std::string& path, const std::string& suffix) {
	return path.size() >= suffix.size() && std::equal(suffix.begin(), suffix.end(), path.end() - suffix.size(), [](char a, char b) {
			   return a == std::tolower(static_cast<unsigned char>(b));
		   });
}

bool FilterNode(const ArchiveIndex::Node& in, ArchiveIndex::Node& out, const std::string& needle, const std::string& extension, size_t& matchCount) {
	for (auto& folder : in.folders) {
		ArchiveIndex::Node child;
		if (FilterNode(folder.second, child, needle, extension, matchCount))
			out.folders.emplace(folder.first, std::move(child));
	}

	for (auto& file : in.files) {
		if (EndsWith(file.second.path, extension) && file.second.path.find(needle) != std::string::npos) {
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
	extensionCounts.clear();
	fileCount = 0;
}

size_t ArchiveIndex::AddArchive(const std::string& path, const std::string& name) {
	archives.push_back({path, name});
	return archives.size() - 1;
}

bool ArchiveIndex::AddFile(const std::string& path, int64_t size, size_t archive) {
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
	extensionCounts[Extension(fileName)]++;
	fileCount++;
	return true;
}

bool ArchiveIndex::IsMeshFile(const std::string& path) {
	static const std::array<const char*, 3> meshExtensions = {".nif", ".bto", ".btr"};
	return std::any_of(meshExtensions.begin(), meshExtensions.end(), [&](const char* ext) { return EndsWith(path, ext); });
}

std::string ArchiveIndex::Extension(const std::string& path) {
	size_t dot = path.rfind('.');
	size_t sep = path.find_last_of("/\\");
	if (dot == std::string::npos || (sep != std::string::npos && dot < sep) || dot + 1 == path.size())
		return std::string();
	return NormalizePath(path.substr(dot));
}

bool ArchiveIndex::HasExtension(const std::string& path, const std::string& extension) {
	return EndsWith(path, NormalizeExtension(extension));
}

size_t ArchiveIndex::FileCount(const std::string& extension) const {
	std::string ext = NormalizeExtension(extension);
	if (ext.empty())
		return fileCount;

	auto it = extensionCounts.find(ext);
	return it != extensionCounts.end() ? it->second : 0;
}

ArchiveIndex::Node ArchiveIndex::Filter(const std::string& needle, const std::string& extension, size_t* matchCount) const {
	Node out;
	size_t count = 0;
	FilterNode(root, out, NormalizePath(needle), NormalizeExtension(extension), count);

	if (matchCount)
		*matchCount = count;

	return out;
}
