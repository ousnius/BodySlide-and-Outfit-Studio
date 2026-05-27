#include "../src/files/SFMaterialDatabase.h"
#include "../src/files/SFMaterialFile.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr uint32_t MakeSig(const char (&sig)[5]) {
    return static_cast<uint32_t>(sig[0])
        | (static_cast<uint32_t>(sig[1]) << 8)
        | (static_cast<uint32_t>(sig[2]) << 16)
        | (static_cast<uint32_t>(sig[3]) << 24);
}

void WriteU8(std::string& out, uint8_t value) {
    out.push_back(static_cast<char>(value));
}

void WriteU16(std::string& out, uint16_t value) {
    out.push_back(static_cast<char>(value & 0xFFu));
    out.push_back(static_cast<char>((value >> 8) & 0xFFu));
}

void WriteU32(std::string& out, uint32_t value) {
    out.push_back(static_cast<char>(value & 0xFFu));
    out.push_back(static_cast<char>((value >> 8) & 0xFFu));
    out.push_back(static_cast<char>((value >> 16) & 0xFFu));
    out.push_back(static_cast<char>((value >> 24) & 0xFFu));
}

void WriteString(std::string& out, const std::string& value) {
    WriteU16(out, static_cast<uint16_t>(value.size() + 1));
    out.append(value);
    out.push_back('\0');
}

void WriteChunk(std::string& out, const char (&sig)[5], const std::string& payload) {
    WriteU32(out, MakeSig(sig));
    WriteU32(out, static_cast<uint32_t>(payload.size()));
    out.append(payload);
}

uint32_t AddString(std::string& table, const std::string& value) {
    const uint32_t offset = static_cast<uint32_t>(table.size());
    table.append(value);
    table.push_back('\0');
    return offset;
}

std::string MakeClassChunk(uint32_t nameOffset) {
    std::string payload;
    WriteU32(payload, nameOffset);
    WriteU32(payload, 0);
    WriteU16(payload, 0);
    WriteU16(payload, 0);
    return payload;
}

void WriteResourceID(std::string& out, const SFResourceID& id) {
    WriteU32(out, id.file);
    WriteU32(out, id.ext);
    WriteU32(out, id.dir);
}

struct ClassFieldSpec {
    uint32_t nameOffset = 0;
    uint32_t typeId = 0;
};

std::string MakeSyntheticCDB(bool includeParentPersistentID) {
    std::string stringTable;
    const uint32_t compiledDBType = AddString(stringTable, "BSMaterial::Internal::CompiledDB");
    const uint32_t fileIndexType = AddString(stringTable, "BSComponentDB2::DBFileIndex");

    std::vector<std::string> chunks;
    std::string stringTableChunk = stringTable;
    chunks.emplace_back();
    WriteChunk(chunks.back(), "STRT", stringTableChunk);

    std::string typePayload;
    WriteU32(typePayload, 2);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "TYPE", typePayload);

    chunks.emplace_back();
    WriteChunk(chunks.back(), "CLAS", MakeClassChunk(compiledDBType));
    chunks.emplace_back();
    WriteChunk(chunks.back(), "CLAS", MakeClassChunk(fileIndexType));

    std::string compiledObject;
    WriteU32(compiledObject, compiledDBType);
    WriteString(compiledObject, "synthetic");
    chunks.emplace_back();
    WriteChunk(chunks.back(), "OBJT", compiledObject);

    std::string emptyMap;
    WriteU32(emptyMap, 0);
    WriteU32(emptyMap, 0);
    WriteU32(emptyMap, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "MAPC", emptyMap);

    std::string emptyList;
    WriteU32(emptyList, 0);
    WriteU32(emptyList, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);

    std::string fileIndexObject;
    WriteU32(fileIndexObject, fileIndexType);
    WriteU8(fileIndexObject, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "OBJT", fileIndexObject);

    chunks.emplace_back();
    WriteChunk(chunks.back(), "MAPC", emptyMap);

    const std::string materialPath = "materials/actors/human/naked_body/female/naked_f_body.mat";
    const SFResourceID materialID = SFGetResourceIdFromPath(materialPath);
    std::string objectList;
    WriteU32(objectList, 0);
    WriteU32(objectList, 1);
    WriteResourceID(objectList, materialID);
    WriteU32(objectList, 42);
    WriteU32(objectList, 0);
    if (includeParentPersistentID)
        WriteResourceID(objectList, SFResourceID{});
    WriteU8(objectList, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", objectList);

    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);

    std::string cdb;
    WriteU32(cdb, MakeSig("BETH"));
    WriteU32(cdb, 8);
    WriteU32(cdb, 1);
    WriteU32(cdb, static_cast<uint32_t>(chunks.size() + 1));
    for (const auto& chunk : chunks)
        cdb.append(chunk);

    return cdb;
}

std::string MakeClassChunk(uint32_t nameOffset, uint16_t flags, const std::vector<ClassFieldSpec>& fields) {
    std::string payload;
    WriteU32(payload, nameOffset);
    WriteU32(payload, 0);
    WriteU16(payload, flags);
    WriteU16(payload, static_cast<uint16_t>(fields.size()));

    for (const ClassFieldSpec& field : fields) {
        WriteU32(payload, field.nameOffset);
        WriteU32(payload, field.typeId);
        WriteU16(payload, 0);
        WriteU16(payload, 0);
    }

    return payload;
}

std::string MakeQueuedComponentCDB() {
    constexpr uint32_t TypeString = 0xFFFFFF02u;
    constexpr uint32_t TypeList = 0xFFFFFF03u;
    constexpr uint32_t TypeMap = 0xFFFFFF04u;
    constexpr uint32_t TypeUInt32 = 0xFFFFFF0Du;
    constexpr uint32_t TypeBool = 0xFFFFFF10u;

    std::string stringTable;
    const uint32_t compiledDBType = AddString(stringTable, "BSMaterial::Internal::CompiledDB");
    const uint32_t fileIndexType = AddString(stringTable, "BSComponentDB2::DBFileIndex");
    const uint32_t componentType = AddString(stringTable, "TestQueuedComponent");
    const uint32_t enabledField = AddString(stringTable, "Enabled");
    const uint32_t firstListField = AddString(stringTable, "FirstList");
    const uint32_t secondMapField = AddString(stringTable, "SecondMap");

    std::vector<std::string> chunks;
    chunks.emplace_back();
    WriteChunk(chunks.back(), "STRT", stringTable);

    std::string typePayload;
    WriteU32(typePayload, 3);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "TYPE", typePayload);

    chunks.emplace_back();
    WriteChunk(chunks.back(), "CLAS", MakeClassChunk(compiledDBType));
    chunks.emplace_back();
    WriteChunk(chunks.back(), "CLAS", MakeClassChunk(fileIndexType));
    chunks.emplace_back();
    WriteChunk(chunks.back(), "CLAS", MakeClassChunk(componentType, 0, {
        { enabledField, TypeBool },
        { firstListField, TypeList },
        { secondMapField, TypeMap },
    }));

    std::string compiledObject;
    WriteU32(compiledObject, compiledDBType);
    WriteString(compiledObject, "synthetic");
    chunks.emplace_back();
    WriteChunk(chunks.back(), "OBJT", compiledObject);

    std::string emptyMap;
    WriteU32(emptyMap, 0);
    WriteU32(emptyMap, 0);
    WriteU32(emptyMap, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "MAPC", emptyMap);

    std::string emptyList;
    WriteU32(emptyList, 0);
    WriteU32(emptyList, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);

    std::string fileIndexObject;
    WriteU32(fileIndexObject, fileIndexType);
    WriteU8(fileIndexObject, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "OBJT", fileIndexObject);

    const uint16_t componentTypeIndex = 7;
    std::string componentTypeMap;
    WriteU32(componentTypeMap, 0);
    WriteU32(componentTypeMap, 0);
    WriteU32(componentTypeMap, 1);
    WriteU16(componentTypeMap, componentTypeIndex);
    WriteU16(componentTypeMap, 1);
    WriteU8(componentTypeMap, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "MAPC", componentTypeMap);

    std::string componentClassRef;
    WriteU32(componentClassRef, 0);
    WriteU32(componentClassRef, TypeString);
    WriteString(componentClassRef, "TestQueuedComponent");
    WriteU32(componentClassRef, 0);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "USER", componentClassRef);

    const std::string materialPath = "materials/actors/human/naked_body/female/naked_f_body.mat";
    const SFResourceID materialID = SFGetResourceIdFromPath(materialPath);
    std::string objectList;
    WriteU32(objectList, 0);
    WriteU32(objectList, 1);
    WriteResourceID(objectList, materialID);
    WriteU32(objectList, 42);
    WriteU32(objectList, 0);
    WriteU8(objectList, 1);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", objectList);

    std::string componentList;
    WriteU32(componentList, 0);
    WriteU32(componentList, 1);
    WriteU32(componentList, 42);
    WriteU16(componentList, 0);
    WriteU16(componentList, componentTypeIndex);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", componentList);

    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", emptyList);

    std::string componentObject;
    WriteU32(componentObject, componentType);
    WriteU8(componentObject, 1);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "OBJT", componentObject);

    std::string firstList;
    WriteU32(firstList, TypeUInt32);
    WriteU32(firstList, 1);
    WriteU32(firstList, 111);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "LIST", firstList);

    std::string secondMap;
    WriteU32(secondMap, TypeString);
    WriteU32(secondMap, TypeUInt32);
    WriteU32(secondMap, 1);
    WriteString(secondMap, "second");
    WriteU32(secondMap, 222);
    chunks.emplace_back();
    WriteChunk(chunks.back(), "MAPC", secondMap);

    std::string cdb;
    WriteU32(cdb, MakeSig("BETH"));
    WriteU32(cdb, 8);
    WriteU32(cdb, 1);
    WriteU32(cdb, static_cast<uint32_t>(chunks.size() + 1));
    for (const auto& chunk : chunks)
        cdb.append(chunk);

    return cdb;
}

void Require(bool condition, const char* message) {
    INFO(message);
    REQUIRE(condition);
}
}

struct SFMaterialDatabaseTestAccess {
    static const nlohmann::json& ComponentJson(const SFMaterialDatabase& db, size_t index) {
        return db.componentJsonCache.at(index);
    }
};

TEST_CASE("Starfield material databases load synthetic indexes", "[SFMaterialDatabase]") {
    const std::string syntheticCDB = MakeSyntheticCDB(false);
    std::istringstream syntheticInput(syntheticCDB, std::ios::binary);
    SFMaterialDatabase syntheticDb;
    Require(syntheticDb.Load(syntheticInput), "Synthetic CDB header+index should parse");
    Require(!syntheticDb.Failed(), "Synthetic CDB should not fail after successful load");
    Require(syntheticDb.HasMaterial("materials/actors/human/naked_body/female/naked_f_body.mat"),
            "Synthetic CDB should contain naked_f_body.mat");
    Require(!syntheticDb.HasMaterial("materials/nonexistent/fake_material.mat"),
            "Synthetic CDB should not find nonexistent material");

    const std::string unverifiedObjectInfoCDB = MakeSyntheticCDB(true);
    std::istringstream unverifiedObjectInfoInput(unverifiedObjectInfoCDB, std::ios::binary);
    SFMaterialDatabase unverifiedObjectInfoDb;
    Require(!unverifiedObjectInfoDb.Load(unverifiedObjectInfoInput),
            "33-byte ObjectInfo entries should require schema support");

    const std::string queuedComponentCDB = MakeQueuedComponentCDB();
    std::istringstream queuedComponentInput(queuedComponentCDB, std::ios::binary);
    SFMaterialDatabase queuedComponentDb;
    Require(queuedComponentDb.Load(queuedComponentInput), "Queued component CDB should parse");
    std::string queuedJsonOutput;
    (void)queuedComponentDb.GetMaterialJSON("materials/actors/human/naked_body/female/naked_f_body.mat", queuedJsonOutput);
    Require(queuedComponentDb.ComponentsRead(), "Queued component CDB should complete component read-all");
    Require(queuedComponentDb.GetIndexedComponentCount() == 1, "Queued component CDB should index one component");
    Require(queuedComponentDb.GetDeserializedComponentCount() == queuedComponentDb.GetIndexedComponentCount(),
            "Queued component CDB should deserialize every indexed component");
    const nlohmann::json& queuedComponentJson = SFMaterialDatabaseTestAccess::ComponentJson(queuedComponentDb, 0);
    Require(queuedComponentJson["Data"]["Enabled"] == "true",
            "Bool fields should deserialize to SFME-style true/false strings");
    Require(queuedComponentJson["Data"]["FirstList"]["Data"][0] == "111",
            "First deferred LIST field should receive first serialized LIST chunk");
    Require(queuedComponentJson["Data"]["SecondMap"]["Data"][0]["Data"]["Key"] == "second",
            "Second deferred MAP field should receive second serialized MAP chunk key");
    Require(queuedComponentJson["Data"]["SecondMap"]["Data"][0]["Data"]["Value"] == "222",
            "Second deferred MAP field should receive second serialized MAP chunk value");
    }

    TEST_CASE("Starfield material databases parse optional real CDB fixtures", "[SFMaterialDatabase][fixture]") {
    const char* fixturePath = std::getenv("SF_MATERIAL_CDB_FIXTURE");
    if (!fixturePath || !fixturePath[0]) {
        SKIP("SF_MATERIAL_CDB_FIXTURE is not set");
    }

    std::ifstream input(fixturePath, std::ios::binary);
    Require(input.good(), "CDB fixture file should open");

    SFMaterialDatabase db;
    const bool loaded = db.Load(input);
    Require(loaded, "CDB header+index should parse successfully");
    Require(!db.Failed(), "CDB should not be in failed state after successful load");

    Require(db.HasMaterial("materials/actors/human/naked_body/female/naked_f_body.mat"),
            "CDB should contain naked_f_body.mat");
    Require(!db.HasMaterial("materials/nonexistent/fake_material.mat"),
            "CDB should not find nonexistent material");

    Require(!db.ComponentsRead(), "CDB fixture should not deserialize components before lookup");
    Require(db.GetIndexedComponentCount() > 0, "CDB fixture should index components");
    std::string jsonOutput;
    Require(db.GetMaterialJSON("materials/actors/human/naked_body/female/naked_f_body.mat", jsonOutput),
            "GetMaterialJSON should resolve naked_f_body.mat from CDB");
    Require(!jsonOutput.empty(), "CDB material JSON output should not be empty");
    Require(db.ComponentsRead(), "GetMaterialJSON should complete CDB component read-all");
    Require(db.GetDeserializedComponentCount() == db.GetIndexedComponentCount(),
            "GetMaterialJSON should deserialize every indexed CDB component");

    std::istringstream jsonStream(jsonOutput);
    SFMaterialFile sfMat(jsonStream);
    Require(!sfMat.Failed(), "SFMaterialFile should parse CDB-produced JSON");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::Color) == "Textures/Actors/human/Naked_Body/NakedBodyF_sk3_color.dds",
            "CDB material should resolve naked body color texture");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::Normal) == "Textures/Actors/human/Naked_Body/nakedbodyf_normal.dds",
            "CDB material should resolve naked body normal texture");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::Roughness) == "Textures/Actors/human/Naked_Body/nakedbodyf_rough.dds",
            "CDB material should resolve naked body roughness texture");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::AmbientOcclusion) == "Textures/Actors/human/Naked_Body/NakedBodyF_ao.dds",
            "CDB material should resolve naked body AO texture");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::Color).find("mask") == std::string::npos,
            "CDB material color texture must not be a blender mask");
    Require(sfMat.GetTexture(SFMaterialTextureSlot::Normal).find("faces") == std::string::npos,
            "CDB material normal texture must not be a face detail normal");
}
