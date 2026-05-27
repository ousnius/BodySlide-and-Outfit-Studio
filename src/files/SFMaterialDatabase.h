/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SFResourceHash.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <deque>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

class SFMaterialDatabase {
public:
    SFMaterialDatabase() = default;

    bool Load(std::istream& input);
    bool Failed() const { return failed; }

    bool HasMaterial(const std::string& matPath) const;
    bool GetMaterialJSON(const std::string& matPath, std::string& jsonOutput);
    bool ComponentsRead() const { return allComponentsRead; }
    size_t GetIndexedComponentCount() const { return components.size(); }
    size_t GetDeserializedComponentCount() const { return componentJsonCache.size(); }

private:
    friend struct SFMaterialDatabaseTestAccess;

    struct Chunk {
        uint32_t sig = 0;
        uint32_t size = 0;
    };

    struct QueuedChunk {
        nlohmann::json* value = nullptr;
        bool isDiff = false;
    };

    struct QueuedCast {
        nlohmann::json* value = nullptr;
        uint32_t typeRef = 0;
    };

    struct ReaderState {
        std::istream* input = nullptr;
        uint32_t version = 0;
        uint32_t chunksRemaining = 0;
        std::deque<QueuedChunk> chunkQueue;
        std::deque<QueuedCast> userQueue;
    };

    struct ClassField {
        uint32_t nameOffset = 0;
        uint32_t typeId = 0;
        uint16_t dataOffset = 0;
        uint16_t dataSize = 0;
    };

    struct CDBClass {
        uint32_t nameOffset = 0;
        uint32_t typeId = 0;
        uint16_t flags = 0xFFFF;
        std::vector<ClassField> fields;

        explicit operator bool() const { return flags != 0xFFFF; }
        bool IsUser() const { return (flags & 4) != 0; }
        bool IsStruct() const { return (flags & 8) != 0; }
    };

    struct ObjectInfo {
        SFResourceID persistentID;
        uint32_t dbID = 0;
        uint32_t parentID = 0;
        SFResourceID parentPersistentID;
        bool hasData = false;
    };

    struct ComponentInfo {
        uint32_t objectID = 0;
        uint16_t index = 0;
        uint16_t type = 0;
    };

    struct EdgeInfo {
        uint32_t sourceID = 0;
        uint32_t targetID = 0;
        uint16_t index = 0;
        uint16_t type = 0;
    };

    struct ComponentTypeInfo {
        std::string className;
        uint16_t version = 0;
        bool isEmpty = false;
    };

    struct MapHeader {
        uint32_t keyType = 0;
        uint32_t valueType = 0;
        uint32_t size = 0;
    };

    struct ListHeader {
        uint32_t elementType = 0;
        uint32_t size = 0;
    };

    bool failed = true;

    std::vector<char> stringTable;
    std::vector<CDBClass> classes;

    bool optimized = false;
    std::vector<std::pair<uint16_t, ComponentTypeInfo>> componentTypes;
    std::vector<ObjectInfo> objects;
    std::vector<ComponentInfo> components;
    std::vector<EdgeInfo> edges;

    std::unordered_map<uint32_t, size_t> objectMap;
    std::unordered_map<uint32_t, std::vector<size_t>> componentMap;
    std::unordered_map<uint32_t, std::vector<size_t>> edgeMap;
    std::unordered_map<SFResourceID, uint32_t, SFResourceIDHash> resourceToDb;

    std::istream* componentStream = nullptr;
    std::streampos componentDataStart = std::streampos(-1);
    uint32_t readerVersion = 0;
    uint32_t readerChunksRemaining = 0;
    bool allComponentsRead = false;
    std::vector<std::streampos> componentPositions;
    std::vector<nlohmann::json> componentJsonCache;

    void Clear();

    bool ReadHeader(ReaderState& state);
    bool ReadDBIndex(ReaderState& state);
    bool ReadCompiledDB(ReaderState& state, const Chunk& objectChunk, std::streampos payloadStart);
    bool ReadFileIndex(ReaderState& state, const Chunk& objectChunk, std::streampos payloadStart);
    bool ReadChunk(ReaderState& state, Chunk& chunk);
    bool ReadMapHeader(ReaderState& state, const Chunk& chunk, MapHeader& map);
    bool ReadListHeader(ReaderState& state, const Chunk& chunk, ListHeader& list);
    bool ReadString(ReaderState& state, std::string& value);
    bool ReadResourceID(ReaderState& state, SFResourceID& id);
    bool ReadComponentTypeMap(ReaderState& state);
    bool ReadObjectList(ReaderState& state);
    bool ReadComponentList(ReaderState& state);
    bool ReadEdgeList(ReaderState& state);
    void BuildLookupIndexes();
    bool HasExactCountedPayload(const Chunk& chunk, uint32_t headerSize, uint32_t count, uint32_t itemSize) const;
    bool EnsureAllComponentsRead();
    bool ReadNextObject(ReaderState& state, nlohmann::json& value);
    bool ReadChunk(ReaderState& state, nlohmann::json& value);
    bool ReadType(ReaderState& state, nlohmann::json& value, uint32_t typeRef, bool isDiff, bool isCast = false);
    bool ReadList(ReaderState& state, nlohmann::json& value, bool isDiff);
    bool ReadMap(ReaderState& state, nlohmann::json& value, bool isDiff);
    bool ReadMapKeyString(ReaderState& state, uint32_t typeRef, std::string& value);

    const char* GetString(uint32_t offset) const;
    const char* GetBuiltinTypeName(uint32_t typeRef) const;
    const char* GetTypeName(uint32_t typeRef) const;
    const CDBClass* FindClass(uint32_t nameOffset) const;
    const CDBClass* FindClassByName(const char* name) const;
    bool ClassHasField(const CDBClass& cdbClass, const char* fieldName) const;
    bool ObjectInfoHasParentPersistentID() const;
};
