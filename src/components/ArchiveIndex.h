/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Merged folder tree of the mesh files in the loaded BSA/BA2 archives, used by
// the archive browser. Paths are stored lowercase with '/' separators, the way
// the archives index them.
class ArchiveIndex {
public:
	struct Archive {
		std::string path; // Full path of the archive file
		std::string name; // File name of the archive
	};

	struct Entry {
		std::string path; // Full path inside the archive, e.g. "meshes/actors/foo.nif"
		int64_t size = 0;
		size_t archive = 0; // Index into Archives()
	};

	struct Node {
		std::map<std::string, Node> folders;
		std::map<std::string, Entry> files;

		bool Empty() const { return folders.empty() && files.empty(); }
	};

	void Clear();

	// Registers an archive and returns its index for AddFile.
	size_t AddArchive(const std::string& path, const std::string& name);

	// Adds a file if it's a mesh and no earlier archive holds the same path,
	// matching the order archive lookups resolve in. Returns whether it was added.
	bool AddFile(const std::string& path, int64_t size, size_t archive);

	// Whether the file is one of the mesh types the browser lists.
	static bool IsMeshFile(const std::string& path);

	const Node& Root() const { return root; }
	const std::vector<Archive>& Archives() const { return archives; }
	size_t FileCount() const { return fileCount; }

	// Copy of the tree with only the files whose path contains "needle"
	// (case-insensitive) and the folders leading to them.
	Node Filter(const std::string& needle, size_t* matchCount = nullptr) const;

private:
	Node root;
	std::vector<Archive> archives;
	size_t fileCount = 0;
};
