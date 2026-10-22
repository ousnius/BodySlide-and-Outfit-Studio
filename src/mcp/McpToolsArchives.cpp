/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/ArchiveIndex.h"
#include "../files/GameDataStream.h"
#include "../program/OutfitStudio.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <wx/string.h>

#include <algorithm>
#include <cctype>

using nlohmann::json;
using nifly::NiShape;

namespace {

// The meshes of the loaded archives, as the Browse Archives dialog lists them. Built on first use and
// again when the loaded archives change.
class ArchiveCache {
public:
	const ArchiveIndex& Get(bool refresh) {
		std::vector<std::string> paths;
		for (FSArchiveFile* archive : FSManager::archiveList())
			if (archive)
				paths.push_back(archive->path());

		if (refresh || !built || paths != archivePaths) {
			index.Clear();
			for (FSArchiveFile* archive : FSManager::archiveList()) {
				if (!archive)
					continue;

				size_t archiveId = index.AddArchive(archive->path(), archive->name());
				archive->forEachFile([&](const std::string& path, wxInt64 size) { index.AddFile(path, size, archiveId); });
			}

			archivePaths = std::move(paths);
			built = true;
		}

		if (index.Archives().empty())
			throw McpToolError("No archives are loaded. Check the game data path and the archive list in the settings.");

		return index;
	}

private:
	ArchiveIndex index;
	std::vector<std::string> archivePaths;
	bool built = false;
};

// Archive paths are lowercase with '/' separators and no leading or trailing separator
std::string NormalizePath(std::string path) {
	std::replace(path.begin(), path.end(), '\\', '/');
	std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	while (!path.empty() && path.front() == '/')
		path.erase(path.begin());
	while (!path.empty() && path.back() == '/')
		path.pop_back();
	return path;
}

std::string ToLower(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

// Which files of the index a call means: below a folder, with optional path and archive filters
struct FileSelection {
	std::string folder;
	std::string filter;
	std::string pattern;
	std::string archive;

	explicit FileSelection(const json& args)
		: folder(NormalizePath(McpArgs::String(args, "folder", "")))
		, filter(NormalizePath(McpArgs::String(args, "filter", "")))
		, pattern(ToLower(McpArgs::String(args, "pattern", "")))
		, archive(ToLower(McpArgs::String(args, "archive", ""))) {
		std::replace(pattern.begin(), pattern.end(), '\\', '/');
	}

	bool HasFilters() const { return !filter.empty() || !pattern.empty() || !archive.empty(); }

	bool Matches(const ArchiveIndex& index, const ArchiveIndex::Entry& file) const {
		if (!filter.empty() && file.path.find(filter) == std::string::npos)
			return false;
		if (!pattern.empty() && !wxMatchWild(wxString::FromUTF8(pattern), wxString::FromUTF8(file.path), false))
			return false;
		if (!archive.empty()) {
			if (file.archive >= index.Archives().size())
				return false;
			if (ToLower(index.Archives()[file.archive].name).find(archive) == std::string::npos)
				return false;
		}
		return true;
	}
};

const ArchiveIndex::Node* FindFolder(const ArchiveIndex& index, const std::string& folder) {
	const ArchiveIndex::Node* node = &index.Root();
	size_t start = 0;
	while (start < folder.size()) {
		size_t sep = folder.find('/', start);
		if (sep == std::string::npos)
			sep = folder.size();

		auto it = node->folders.find(folder.substr(start, sep - start));
		if (it == node->folders.end())
			return nullptr;

		node = &it->second;
		start = sep + 1;
	}
	return node;
}

const ArchiveIndex::Node& RequireFolder(const ArchiveIndex& index, const std::string& folder) {
	const ArchiveIndex::Node* node = FindFolder(index, folder);
	if (!node)
		throw McpToolError("The archives have no folder '" + folder + "'. browse_archives lists the folders.");
	return *node;
}

void CollectFiles(const ArchiveIndex& index, const ArchiveIndex::Node& node, const FileSelection& selection, std::vector<const ArchiveIndex::Entry*>& out) {
	for (auto& file : node.files)
		if (selection.Matches(index, file.second))
			out.push_back(&file.second);

	for (auto& folder : node.folders)
		CollectFiles(index, folder.second, selection, out);
}

size_t CountFiles(const ArchiveIndex& index, const ArchiveIndex::Node& node, const FileSelection& selection) {
	size_t count = 0;
	for (auto& file : node.files)
		if (selection.Matches(index, file.second))
			count++;

	for (auto& folder : node.folders)
		count += CountFiles(index, folder.second, selection);
	return count;
}

void CountByArchive(const ArchiveIndex::Node& node, std::vector<size_t>& counts) {
	for (auto& file : node.files)
		if (file.second.archive < counts.size())
			counts[file.second.archive]++;

	for (auto& folder : node.folders)
		CountByArchive(folder.second, counts);
}

json FileJson(const ArchiveIndex& index, const ArchiveIndex::Entry& file) {
	return {
		{"path", file.path},
		{"size", file.size},
		{"archive", file.archive < index.Archives().size() ? index.Archives()[file.archive].name : std::string()},
	};
}

// The file at an archive path, if the index lists it
const ArchiveIndex::Entry* FindFile(const ArchiveIndex& index, const std::string& path) {
	size_t sep = path.rfind('/');
	const ArchiveIndex::Node* node = sep == std::string::npos ? &index.Root() : FindFolder(index, path.substr(0, sep));
	if (!node)
		return nullptr;

	auto it = node->files.find(sep == std::string::npos ? path : path.substr(sep + 1));
	return it != node->files.end() ? &it->second : nullptr;
}

const json SelectionProps = {
	{"folder", McpProp("string", "Archive folder, e.g. \"meshes/clothes/monk\". Case and slash direction don't matter. Default: the top.")},
	{"filter", McpProp("string", "Only files whose path contains this text, like the search box of the dialog")},
	{"pattern", McpProp("string", "Only files whose full path matches this wildcard pattern (* and ?), e.g. \"*/clothes/monk/*_1.nif\"")},
	{"archive", McpProp("string", "Only files from archives whose file name contains this text")},
};

} // namespace

void AddArchiveTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	auto cache = std::make_shared<ArchiveCache>();

	McpTool browse;
	browse.name = "browse_archives";
	browse.description =
		"Browses the meshes (.nif, .bto, .btr) in the game's loaded BSA/BA2 archives, like File > Browse Archives. "
		"Archives are merged into one folder tree, and a path in several archives is listed from the first, the one "
		"the game would use. Without filters, lists the subfolders (with their mesh counts) and the files of 'folder'. "
		"With filter, pattern or archive, or recursive: true, lists the matching files anywhere below 'folder'. "
		"Load files with load_from_archive.";
	json browseProps = SelectionProps;
	browseProps["recursive"] = McpProp("boolean", "List the files of all subfolders too. Default false, true when a filter is given.");
	browseProps["limit"] = McpProp("integer", "Most files to list. Default 200.");
	browseProps["refresh"] = McpProp("boolean", "Read the archives again, e.g. after the archive list in the settings changed");
	browse.inputSchema = McpSchema(browseProps);
	browse.readOnly = true;
	browse.handler = [cache](const json& args) {
		const ArchiveIndex& index = cache->Get(McpArgs::Bool(args, "refresh", false));
		FileSelection selection(args);
		const ArchiveIndex::Node& node = RequireFolder(index, selection.folder);
		bool recursive = McpArgs::Bool(args, "recursive", selection.HasFilters());
		int limit = std::max(1, McpArgs::Int(args, "limit", 200));

		McpToolResult result;
		json& data = result.data;

		// The archives with meshes, at the top only, where the agent starts
		data["meshCount"] = index.FileCount();
		if (selection.folder.empty() && !selection.HasFilters()) {
			std::vector<size_t> counts(index.Archives().size());
			CountByArchive(index.Root(), counts);

			json archives = json::array();
			for (size_t i = 0; i < counts.size(); i++)
				if (counts[i] > 0)
					archives.push_back({{"name", index.Archives()[i].name}, {"meshes", counts[i]}});
			data["archives"] = archives;
		}
		data["folder"] = selection.folder;

		json folders = json::array();
		for (auto& folder : node.folders) {
			size_t count = CountFiles(index, folder.second, selection);
			if (count > 0)
				folders.push_back({{"name", folder.first}, {"files", count}});
		}
		data["folders"] = folders;

		std::vector<const ArchiveIndex::Entry*> files;
		if (recursive) {
			CollectFiles(index, node, selection, files);
		}
		else {
			for (auto& file : node.files)
				if (selection.Matches(index, file.second))
					files.push_back(&file.second);
		}

		json fileList = json::array();
		for (size_t i = 0; i < files.size() && i < static_cast<size_t>(limit); i++)
			fileList.push_back(FileJson(index, *files[i]));

		data["files"] = fileList;
		data["fileCount"] = files.size();
		data["truncated"] = files.size() > static_cast<size_t>(limit);
		return result;
	};
	mcp.AddTool(std::move(browse));

	McpTool load;
	load.name = "load_from_archive";
	load.description =
		"Loads meshes from the game's archives into the project, like the Import and Replace Outfit buttons of "
		"File > Browse Archives. Name the files with 'files' (archive paths from browse_archives), or select them "
		"with folder, filter, pattern and archive (all files below the folder that match). mode \"import\" (the "
		"default) adds the shapes to the outfit; \"replace\" replaces the outfit shapes with the first file and "
		"imports the others. The reference stays.";
	json loadProps = SelectionProps;
	loadProps["files"] = McpStringList("Archive paths of the files, e.g. \"meshes/clothes/monk/monkrobes_1.nif\"");
	loadProps["mode"] = json{{"type", "string"}, {"enum", {"import", "replace"}}, {"description", "import (default) or replace"}};
	loadProps["maxFiles"] = McpProp("integer", "Refuse when more files than this match, so a broad selection doesn't load half the game. Default 10.");
	loadProps["discardChanges"] = McpProp("boolean", "Replace mode: drop unsaved changes of the outfit. Default false, which refuses when there are any.");
	load.inputSchema = McpSchema(loadProps);
	load.handler = [&mcp, frame, cache](const json& args) {
		const ArchiveIndex& index = cache->Get(false);
		FileSelection selection(args);

		std::vector<const ArchiveIndex::Entry*> files;
		if (McpArgs::Has(args, "files")) {
			for (auto& path : McpArgs::Strings(args, "files")) {
				const ArchiveIndex::Entry* file = FindFile(index, NormalizePath(path));
				if (!file)
					throw McpToolError("The archives have no mesh '" + path + "'. browse_archives lists them.");
				files.push_back(file);
			}
		}
		else {
			if (selection.folder.empty() && !selection.HasFilters())
				throw McpToolError("Name the files with 'files', or select them with folder, filter, pattern or archive.");
			CollectFiles(index, RequireFolder(index, selection.folder), selection, files);
		}

		if (files.empty())
			throw McpToolError("No archive mesh matches. browse_archives with the same arguments shows what's there.");

		int maxFiles = McpArgs::Int(args, "maxFiles", 10);
		if (files.size() > static_cast<size_t>(std::max(1, maxFiles)))
			throw McpToolError(std::to_string(files.size()) + " files match, more than maxFiles (" + std::to_string(maxFiles)
							   + "). Narrow the selection, or raise maxFiles.");

		std::string mode = McpArgs::String(args, "mode", "import");
		if (mode != "import" && mode != "replace")
			throw McpToolError("mode has to be \"import\" or \"replace\".");

		bool replace = mode == "replace";
		if (replace && frame->HasPendingChanges()) {
			if (!McpArgs::Bool(args, "discardChanges", false))
				throw McpToolError("The project has unsaved changes. Save it with save_project first, or pass discardChanges: true to drop them.");
			frame->SetPendingChanges(false);
		}

		OutfitProject& project = mcp.Project();

		McpToolResult result;
		json loaded = json::array();
		for (const ArchiveIndex::Entry* file : files) {
			// Copies, the cache may be rebuilt while loading
			std::string path = file->path;
			std::string archivePath = index.Archives()[file->archive].path;
			std::string archiveName = index.Archives()[file->archive].name;

			// The window shows a message box when it can't read the file, the agent gets an error instead
			if (!GameDataStream::OpenArchive(path, archivePath))
				throw McpToolError("Could not read '" + path + "' from archive '" + archivePath + "'.");

			// Replacing removes the outfit shapes, so then every outfit shape after is from this file
			std::vector<std::string> before = replace ? std::vector<std::string>() : project.GetShapeOrder();
			bool ok = frame->LoadMeshFromArchive(path, archivePath, !replace);
			replace = false;

			NiShape* baseShape = project.GetBaseShape();
			json shapes = json::array();
			for (auto& name : project.GetShapeOrder())
				if ((!baseShape || name != baseShape->name.get()) && std::find(before.begin(), before.end(), name) == before.end())
					shapes.push_back(name);

			loaded.push_back({{"path", path}, {"archive", archiveName}, {"loaded", ok}, {"shapes", shapes}});
			if (!ok) {
				result.isError = true;
				break;
			}
		}

		NiShape* baseShape = project.GetBaseShape();
		json outfitShapes = json::array();
		for (auto& name : project.GetShapeOrder())
			if (!baseShape || name != baseShape->name.get())
				outfitShapes.push_back(name);

		result.data["loaded"] = loaded;
		result.data["outfitShapes"] = outfitShapes;
		return result;
	};
	mcp.AddTool(std::move(load));
}
