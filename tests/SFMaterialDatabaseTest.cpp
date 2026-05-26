#include "../src/files/SFMaterialDatabase.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
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

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}
}

int main() {
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

    const char* fixturePath = std::getenv("SF_MATERIAL_CDB_FIXTURE");
    if (!fixturePath || !fixturePath[0]) {
        std::cout << "SFMaterialDatabase synthetic tests PASSED" << std::endl;
        std::cout << "SF_MATERIAL_CDB_FIXTURE not set, skipping real CDB fixture tests" << std::endl;
        return 0;
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

    std::cout << "SFMaterialDatabase header tests PASSED" << std::endl;
    return 0;
}
