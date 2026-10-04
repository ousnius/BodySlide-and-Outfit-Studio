/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GameDataStream.h"

#include "../utils/ConfigurationManager.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"
#include "NifFile.hpp"

#include <fstream>
#include <regex>
#include <sstream>

#include <wx/buffer.h>

extern ConfigurationManager Config;

namespace GameDataStream {
std::unique_ptr<std::istream> OpenLoose(const std::string& fullPath) {
	if (!PlatformUtil::FileExists(fullPath))
		return nullptr;

	auto fs = std::make_unique<std::fstream>();
	PlatformUtil::OpenFileStream(*fs, fullPath, std::ios::in | std::ios::binary);
	if (!fs->fail())
		return fs;

	return nullptr;
}

std::unique_ptr<std::istream> OpenArchive(const std::string& relPath) {
	for (FSArchiveFile* archive : FSManager::archiveList()) {
		if (!archive || !archive->hasFile(relPath))
			continue;

		wxMemoryBuffer outData;
		archive->fileContents(relPath, outData);
		if (outData.IsEmpty())
			continue;

		auto contentStream = std::make_unique<std::istringstream>(
			std::string(static_cast<char*>(outData.GetData()), outData.GetDataLen()), std::istringstream::binary);
		if (!contentStream->fail())
			return contentStream;
	}

	return nullptr;
}

std::unique_ptr<std::istream> OpenPhysicsXml(const std::string& xmlPath, const std::string& nifFilePath) {
	// The paths are relative to the game data folder, but the game's resource
	// system also accepts a leading "Data\" prefix ("Data\meshes\...") and mods
	// in the wild use both spellings, so strip it before resolving.
	std::string relPath = std::regex_replace(xmlPath, std::regex("\\\\+"), "/");
	relPath = std::regex_replace(relPath, std::regex("^/+"), "");
	if (ToLower(relPath).rfind("data/", 0) == 0)
		relPath = relPath.substr(5);
	if (relPath.empty())
		return nullptr;

	// 1) Loose file in GameDataPath
	if (auto stream = OpenLoose(Config["GameDataPath"] + relPath))
		return stream;

	// 2) Relative to the NIF the link came from: its "meshes" anchor points at
	// the mod's data root, where SKSE/Plugins/... lives for loose mod folders
	if (!nifFilePath.empty()) {
		std::string nifDir = std::regex_replace(nifFilePath, std::regex("\\\\+"), "/");
		std::string nifDirLower = ToLower(nifDir);

		auto meshesPos = nifDirLower.rfind("/meshes/");
		if (meshesPos != std::string::npos) {
			if (auto stream = OpenLoose(nifDir.substr(0, meshesPos + 1) + relPath))
				return stream;
		}

		auto lastSlash = nifDir.rfind('/');
		if (lastSlash != std::string::npos) {
			// Also try directly beside the NIF (both the full relative path and
			// just the file name), for standalone test setups
			if (auto stream = OpenLoose(nifDir.substr(0, lastSlash + 1) + relPath))
				return stream;

			auto fileNamePos = relPath.rfind('/');
			if (fileNamePos != std::string::npos) {
				if (auto stream = OpenLoose(nifDir.substr(0, lastSlash + 1) + relPath.substr(fileNamePos + 1)))
					return stream;
			}
		}
	}

	// 3) Search in archives
	return OpenArchive(relPath);
}

std::unique_ptr<std::istream> OpenExternalGeometry(const std::string& path, const std::string& dataPath, const std::string& nifFilePath) {
	// Normalize path: replace backslashes, extract relative geometries path, ensure prefix and suffix
	std::string meshPath = std::regex_replace(path, std::regex("\\\\+"), "/");
	meshPath = std::regex_replace(meshPath, std::regex("^(.*?)/geometries/", std::regex_constants::icase), "");
	meshPath = std::regex_replace(meshPath, std::regex("^/+"), "");
	meshPath = std::regex_replace(meshPath, std::regex("^(?!^geometries/)", std::regex_constants::icase), "geometries/");

	if (meshPath.size() < 5 || meshPath.compare(meshPath.size() - 5, 5, ".mesh") != 0)
		meshPath += ".mesh";

	// 1) Loose file in the data folder
	if (auto stream = OpenLoose(dataPath + meshPath))
		return stream;

	// 2) Beside the meshes folder (or NIF directory) of the loading NIF
	if (!nifFilePath.empty()) {
		std::string nifDir = std::regex_replace(nifFilePath, std::regex("\\\\+"), "/");
		std::string nifDirLower = ToLower(nifDir);

		auto meshesPos = nifDirLower.rfind("/meshes/");
		if (meshesPos != std::string::npos) {
			if (auto stream = OpenLoose(nifDir.substr(0, meshesPos + 1) + meshPath))
				return stream;
		}
		else {
			auto lastSlash = nifDir.rfind('/');
			if (lastSlash != std::string::npos) {
				if (auto stream = OpenLoose(nifDir.substr(0, lastSlash + 1) + meshPath))
					return stream;
			}
		}
	}

	// 3) Search in archives
	return OpenArchive(meshPath);
}

std::vector<std::string> LoadExternalGeometry(nifly::NifFile& nif, const std::string& dataPath, const std::string& nifFilePath) {
	std::vector<std::string> missing;

	for (auto& shape : nif.GetShapes()) {
		uint8_t meshIndex = 0;
		for (auto meshPath : nif.GetExternalGeometryPathRefs(shape)) {
			auto meshStream = OpenExternalGeometry(meshPath.get(), dataPath, nifFilePath);
			if (!meshStream) {
				missing.push_back(meshPath.get());
				continue;
			}

			nif.LoadExternalShapeData(shape, *meshStream, meshIndex);
			meshIndex++;
		}
	}

	return missing;
}
}
