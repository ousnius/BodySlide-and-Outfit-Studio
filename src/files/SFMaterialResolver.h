/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SFLayeredMaterial.h"
#include "SFMaterialDatabase.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Finds Starfield materials the way the game would: a loose .mat in the data folder wins over one in
// an archive, and both over the material databases (materials/*.cdb) the archives carry. Shared by
// Outfit Studio and the BodySlide preview so that both resolve a shape's material the same way.
class SFMaterialResolver {
public:
    // Reads the material a shape's shader names. Texture paths come out normalized to
    // "textures/..." relative to the data folder. Returns false when no source has a readable
    // material with any texture in it.
    bool Resolve(const std::string& shaderName, const std::string& dataPath, SFLayeredMaterial& material);

    // Drops the loaded material databases, so that the next lookup reads them from the archives again
    void Clear();

    // "materials/....mat" for whatever form a shader stores its material path in
    static std::string NormalizeMaterialPath(std::string matFile, const std::string& extension);
    // "textures/..." for whatever form a material or NIF stores a texture path in
    static std::string NormalizeTexturePath(std::string textureFile);

private:
    struct Database {
        // The database reads its components from this stream lazily, so it has to stay around
        std::unique_ptr<std::istringstream> stream;
        std::unique_ptr<SFMaterialDatabase> db;
    };

    std::vector<Database> databases;
    bool databasesLoaded = false;

    void LoadDatabases();
    bool GetMaterialJSON(const std::string& matPath, std::string& jsonOutput);
};
