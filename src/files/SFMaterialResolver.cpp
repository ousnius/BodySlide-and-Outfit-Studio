/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialResolver.h"

#include "SFMaterialFile.h"
#include "../utils/StringStuff.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <fstream>
#include <regex>
#include <set>

namespace {
bool StartsWithInsensitive(const std::string& path, const std::string& prefix) {
    if (path.length() < prefix.length())
        return false;

    return StringsEqualNInsens(path.c_str(), prefix.c_str(), static_cast<int>(prefix.length()));
}

bool HasExtensionInsensitive(const std::string& path, const std::string& extension) {
    if (extension.empty())
        return true;

    if (path.length() < extension.length())
        return false;

    return StringsEqualInsens(path.substr(path.length() - extension.length()).c_str(), extension.c_str());
}

bool ReadArchiveFile(const std::string& filePath, wxMemoryBuffer& data) {
    for (FSArchiveFile* archive : FSManager::archiveList()) {
        if (!archive || !archive->hasFile(filePath))
            continue;

        wxMemoryBuffer outData;
        archive->fileContents(filePath, outData);
        if (!outData.IsEmpty()) {
            data = std::move(outData);
            return true;
        }
    }

    return false;
}

bool ReadMaterial(std::istream& input, SFLayeredMaterial& material) {
    SFMaterialFile file(input);
    if (file.Failed())
        return false;

    material = file.GetLayeredMaterial();
    return true;
}
}

std::string SFMaterialResolver::NormalizeMaterialPath(std::string matFile, const std::string& extension) {
    matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");
    matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "materials/");
    matFile = std::regex_replace(matFile, std::regex("^/+"), "");

    if (!StartsWithInsensitive(matFile, "materials/"))
        matFile = "materials/" + matFile;

    if (!HasExtensionInsensitive(matFile, extension))
        matFile += extension;

    return matFile;
}

std::string SFMaterialResolver::NormalizeTexturePath(std::string textureFile) {
    textureFile = std::regex_replace(textureFile, std::regex("\\\\+"), "/");
    textureFile = std::regex_replace(textureFile, std::regex("^(.*?)/textures/", std::regex_constants::icase), "");
    textureFile = std::regex_replace(textureFile, std::regex("^/+"), "");

    if (!StartsWithInsensitive(textureFile, "textures/"))
        textureFile = "textures/" + textureFile;

    return textureFile;
}

bool SFMaterialResolver::Resolve(const std::string& shaderName, const std::string& dataPath, SFLayeredMaterial& material) {
    if (shaderName.empty())
        return false;

    const std::string matFile = NormalizeMaterialPath(shaderName, ".mat");
    bool found = false;

    std::ifstream looseInput(dataPath + matFile);
    if (looseInput)
        found = ReadMaterial(looseInput, material);

    if (!found) {
        wxMemoryBuffer data;
        if (ReadArchiveFile(matFile, data)) {
            std::istringstream archiveInput(std::string(static_cast<const char*>(data.GetData()), data.GetDataLen()), std::ios::binary);
            found = ReadMaterial(archiveInput, material);
        }
    }

    if (!found) {
        std::string materialJson;
        if (GetMaterialJSON(matFile, materialJson)) {
            std::istringstream databaseInput(materialJson);
            found = ReadMaterial(databaseInput, material);
        }
    }

    if (!found)
        return false;

    material.ResolveTexturePaths(NormalizeTexturePath);
    return true;
}

void SFMaterialResolver::Clear() {
    databases.clear();
    databasesLoaded = false;
}

void SFMaterialResolver::LoadDatabases() {
    databasesLoaded = true;

    std::set<std::string> seen;
    for (FSArchiveFile* archive : FSManager::archiveList()) {
        if (!archive)
            continue;

        std::vector<std::string> cdbPaths;
        archive->findFilesBySuffix("materials/", ".cdb", cdbPaths);

        for (const auto& cdbPath : cdbPaths) {
            if (!seen.insert(cdbPath).second)
                continue;

            wxMemoryBuffer data;
            archive->fileContents(cdbPath, data);
            if (data.IsEmpty())
                continue;

            Database database;
            database.stream = std::make_unique<std::istringstream>(std::string(static_cast<const char*>(data.GetData()), data.GetDataLen()), std::ios::in | std::ios::binary);
            database.db = std::make_unique<SFMaterialDatabase>();

            if (database.db->Load(*database.stream) && !database.db->Failed())
                databases.push_back(std::move(database));
        }
    }
}

bool SFMaterialResolver::GetMaterialJSON(const std::string& matPath, std::string& jsonOutput) {
    if (!databasesLoaded)
        LoadDatabases();

    for (auto& database : databases)
        if (database.db->GetMaterialJSON(matPath, jsonOutput))
            return true;

    return false;
}
