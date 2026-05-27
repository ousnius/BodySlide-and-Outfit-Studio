/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialDatabase.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {
constexpr uint32_t MakeSig(const char (&sig)[5]) {
    return static_cast<uint32_t>(sig[0])
        | (static_cast<uint32_t>(sig[1]) << 8)
        | (static_cast<uint32_t>(sig[2]) << 16)
        | (static_cast<uint32_t>(sig[3]) << 24);
}

constexpr uint32_t SigBETH = MakeSig("BETH");
constexpr uint32_t SigSTRT = MakeSig("STRT");
constexpr uint32_t SigTYPE = MakeSig("TYPE");
constexpr uint32_t SigCLAS = MakeSig("CLAS");
constexpr uint32_t SigOBJT = MakeSig("OBJT");
constexpr uint32_t SigUSER = MakeSig("USER");
constexpr uint32_t SigUSRD = MakeSig("USRD");
constexpr uint32_t SigDIFF = MakeSig("DIFF");
constexpr uint32_t SigMAPC = MakeSig("MAPC");
constexpr uint32_t SigLIST = MakeSig("LIST");
constexpr uint32_t TypeBuiltinMask = 0xFFFFFF00u;
constexpr uint32_t TypeNull = 0xFFFFFF01u;
constexpr uint32_t TypeString = 0xFFFFFF02u;
constexpr uint32_t TypeList = 0xFFFFFF03u;
constexpr uint32_t TypeMap = 0xFFFFFF04u;
constexpr uint32_t TypeRef = 0xFFFFFF05u;
constexpr uint32_t TypeInt8 = 0xFFFFFF08u;
constexpr uint32_t TypeUInt8 = 0xFFFFFF09u;
constexpr uint32_t TypeInt16 = 0xFFFFFF0Au;
constexpr uint32_t TypeUInt16 = 0xFFFFFF0Bu;
constexpr uint32_t TypeInt32 = 0xFFFFFF0Cu;
constexpr uint32_t TypeUInt32 = 0xFFFFFF0Du;
constexpr uint32_t TypeInt64 = 0xFFFFFF0Eu;
constexpr uint32_t TypeUInt64 = 0xFFFFFF0Fu;
constexpr uint32_t TypeBool = 0xFFFFFF10u;
constexpr uint32_t TypeFloat = 0xFFFFFF11u;
constexpr uint32_t TypeDouble = 0xFFFFFF12u;
constexpr uint32_t SFMatExtension = 0x0074616Du;
constexpr uint32_t CDBResourceIDSize = 12;
constexpr uint32_t CDBComponentTypeMapEntrySize = 5;
constexpr uint32_t CDBObjectInfoSize = 21;
constexpr uint32_t CDBObjectInfoWithParentPersistentIDSize = 33;
constexpr uint32_t CDBComponentInfoSize = 8;
constexpr uint32_t CDBEdgeInfoSize = 12;

constexpr std::array<const char*, 19> BuiltinTypeNames = {
    "Unk0",
    "<null>",
    "BSFixedString",
    "<collection>",
    "<collection>",
    "pointer",
    "Unk6",
    "Unk7",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t",
    "bool",
    "float",
    "double",
};

template <typename T>
bool ReadPod(std::istream& input, T& value) {
    input.read(reinterpret_cast<char*>(&value), sizeof(T));
    return input.good();
}

bool SkipBytes(std::istream& input, uint32_t size) {
    input.seekg(static_cast<std::streamoff>(size), std::ios::cur);
    return input.good();
}

bool SkipTo(std::istream& input, std::streampos target) {
    const std::streampos pos = input.tellg();
    if (pos == std::streampos(-1) || target == std::streampos(-1) || pos > target)
        return false;

    input.seekg(target);
    return input.good();
}

bool EqualsIgnoreCase(const char* lhs, const char* rhs) {
    if (!lhs || !rhs)
        return lhs == rhs;

    while (*lhs && *rhs) {
        const auto left = static_cast<unsigned char>(*lhs++);
        const auto right = static_cast<unsigned char>(*rhs++);
        if (std::tolower(left) != std::tolower(right))
            return false;
    }

    return *lhs == *rhs;
}

bool IsBuiltinTypeRef(uint32_t typeRef) {
    return (typeRef & TypeBuiltinMask) == TypeBuiltinMask;
}

bool IsChunkTypeRef(uint32_t typeRef) {
    return typeRef == TypeList || typeRef == TypeMap;
}

std::string FormatResourceID(const SFResourceID& id) {
    std::ostringstream ss;
    ss << "res:" << std::uppercase << std::hex << std::setfill('0')
       << std::setw(8) << id.dir << ':'
       << std::setw(8) << id.file << ':'
       << std::setw(8) << id.ext;
    return ss.str();
}
}

void SFMaterialDatabase::Clear() {
    stringTable.clear();
    classes.clear();
    optimized = false;
    componentTypes.clear();
    objects.clear();
    components.clear();
    edges.clear();
    objectMap.clear();
    componentMap.clear();
    edgeMap.clear();
    resourceToDb.clear();
    componentStream = nullptr;
    componentDataStart = std::streampos(-1);
    readerVersion = 0;
    readerChunksRemaining = 0;
    allComponentsRead = false;
    componentPositions.clear();
    componentJsonCache.clear();
}

bool SFMaterialDatabase::Load(std::istream& input) {
    failed = true;
    Clear();

    if (!input)
        return false;

    ReaderState state;
    state.input = &input;

    if (!ReadHeader(state))
        return false;

    if (!ReadDBIndex(state))
        return false;

    componentDataStart = input.tellg();
    if (componentDataStart == std::streampos(-1))
        return false;

    componentStream = &input;
    readerVersion = state.version;
    readerChunksRemaining = state.chunksRemaining;

    BuildLookupIndexes();
    failed = false;
    return true;
}

bool SFMaterialDatabase::HasMaterial(const std::string& matPath) const {
    if (failed)
        return false;

    const SFResourceID id = SFGetResourceIdFromPath(matPath);
    return resourceToDb.find(id) != resourceToDb.end();
}

bool SFMaterialDatabase::GetMaterialJSON(const std::string& matPath, std::string& jsonOutput) {
    jsonOutput.clear();

    if (failed || !HasMaterial(matPath))
        return false;

    if (!EnsureAllComponentsRead())
        return false;

    return false;
}

bool SFMaterialDatabase::ReadHeader(ReaderState& state) {
    Chunk root;
    if (!ReadPod(*state.input, root.sig) || !ReadPod(*state.input, root.size))
        return false;

    if (root.sig != SigBETH || root.size < 8)
        return false;

    uint32_t chunkCount = 0;
    if (!ReadPod(*state.input, state.version) || !ReadPod(*state.input, chunkCount))
        return false;

    state.chunksRemaining = chunkCount > 0 ? chunkCount - 1 : 0;
    if (root.size > 8 && !SkipBytes(*state.input, root.size - 8))
        return false;

    Chunk stringChunk;
    if (!ReadChunk(state, stringChunk) || stringChunk.sig != SigSTRT)
        return false;

    stringTable.resize(static_cast<size_t>(stringChunk.size) + 1, '\0');
    if (stringChunk.size != 0) {
        state.input->read(stringTable.data(), stringChunk.size);
        if (!state.input->good())
            return false;
    }
    stringTable[stringChunk.size] = '\0';

    Chunk typeChunk;
    if (!ReadChunk(state, typeChunk) || typeChunk.sig != SigTYPE || typeChunk.size < 4)
        return false;

    uint32_t typeCount = 0;
    if (!ReadPod(*state.input, typeCount))
        return false;

    if (typeChunk.size > 4 && !SkipBytes(*state.input, typeChunk.size - 4))
        return false;

    classes.reserve(typeCount);
    for (uint32_t i = 0; i < typeCount; ++i) {
        Chunk classChunk;
        if (!ReadChunk(state, classChunk) || classChunk.sig != SigCLAS || classChunk.size < 12)
            return false;

        CDBClass cdbClass;
        uint16_t fieldCount = 0;
        if (!ReadPod(*state.input, cdbClass.nameOffset)
            || !ReadPod(*state.input, cdbClass.typeId)
            || !ReadPod(*state.input, cdbClass.flags)
            || !ReadPod(*state.input, fieldCount)) {
            return false;
        }

        const uint32_t expectedSize = 12u + static_cast<uint32_t>(fieldCount) * 12u;
        if (classChunk.size < expectedSize)
            return false;

        cdbClass.fields.reserve(fieldCount);
        for (uint16_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            ClassField field;
            if (!ReadPod(*state.input, field.nameOffset)
                || !ReadPod(*state.input, field.typeId)
                || !ReadPod(*state.input, field.dataOffset)
                || !ReadPod(*state.input, field.dataSize)) {
                return false;
            }

            cdbClass.fields.emplace_back(field);
        }

        if (classChunk.size > expectedSize && !SkipBytes(*state.input, classChunk.size - expectedSize))
            return false;

        classes.emplace_back(std::move(cdbClass));
    }

    return true;
}

bool SFMaterialDatabase::ReadDBIndex(ReaderState& state) {
    bool readCompiledDB = false;
    bool readFileIndex = false;

    for (int i = 0; i < 2; ++i) {
        Chunk objectChunk;
        if (!ReadChunk(state, objectChunk) || objectChunk.sig != SigOBJT || objectChunk.size < 4)
            return false;

        uint32_t typeRef = 0;
        if (!ReadPod(*state.input, typeRef))
            return false;

        const char* typeName = GetTypeName(typeRef);
        const std::streampos payloadStart = state.input->tellg();
        if (payloadStart == std::streampos(-1))
            return false;

        if (EqualsIgnoreCase(typeName, "BSMaterial::Internal::CompiledDB")) {
            if (readCompiledDB || !ReadCompiledDB(state, objectChunk, payloadStart))
                return false;
            readCompiledDB = true;
        }
        else if (EqualsIgnoreCase(typeName, "BSComponentDB2::DBFileIndex")) {
            if (readFileIndex || !ReadFileIndex(state, objectChunk, payloadStart))
                return false;
            readFileIndex = true;
        }
        else {
            return false;
        }
    }

    return readCompiledDB && readFileIndex;
}

bool SFMaterialDatabase::ReadCompiledDB(ReaderState& state, const Chunk& objectChunk, std::streampos payloadStart) {
    std::string buildVersion;
    if (!ReadString(state, buildVersion))
        return false;

    const std::streampos objectEnd = payloadStart + static_cast<std::streamoff>(objectChunk.size - 4);
    if (!SkipTo(*state.input, objectEnd))
        return false;

    Chunk hashMapChunk;
    if (!ReadChunk(state, hashMapChunk) || hashMapChunk.sig != SigMAPC)
        return false;
    if (!SkipBytes(*state.input, hashMapChunk.size))
        return false;

    Chunk collisionsChunk;
    if (!ReadChunk(state, collisionsChunk) || collisionsChunk.sig != SigLIST)
        return false;
    if (!SkipBytes(*state.input, collisionsChunk.size))
        return false;

    Chunk circularChunk;
    if (!ReadChunk(state, circularChunk) || circularChunk.sig != SigLIST)
        return false;
    return SkipBytes(*state.input, circularChunk.size);
}

bool SFMaterialDatabase::ReadFileIndex(ReaderState& state, const Chunk& objectChunk, std::streampos payloadStart) {
    uint8_t optimizedByte = 0;
    if (!ReadPod(*state.input, optimizedByte))
        return false;

    optimized = optimizedByte != 0;

    const std::streampos objectEnd = payloadStart + static_cast<std::streamoff>(objectChunk.size - 4);
    if (!SkipTo(*state.input, objectEnd))
        return false;

    return ReadComponentTypeMap(state)
        && ReadObjectList(state)
        && ReadComponentList(state)
        && ReadEdgeList(state);
}

bool SFMaterialDatabase::ReadChunk(ReaderState& state, Chunk& chunk) {
    if (!ReadPod(*state.input, chunk.sig) || !ReadPod(*state.input, chunk.size))
        return false;

    if (state.chunksRemaining > 0)
        --state.chunksRemaining;

    return true;
}

bool SFMaterialDatabase::ReadMapHeader(ReaderState& state, const Chunk& chunk, MapHeader& map) {
    if (chunk.sig != SigMAPC || chunk.size < 12)
        return false;

    return ReadPod(*state.input, map.keyType)
        && ReadPod(*state.input, map.valueType)
        && ReadPod(*state.input, map.size);
}

bool SFMaterialDatabase::ReadListHeader(ReaderState& state, const Chunk& chunk, ListHeader& list) {
    if (chunk.sig != SigLIST || chunk.size < 8)
        return false;

    return ReadPod(*state.input, list.elementType)
        && ReadPod(*state.input, list.size);
}

bool SFMaterialDatabase::ReadString(ReaderState& state, std::string& value) {
    uint16_t length = 0;
    if (!ReadPod(*state.input, length))
        return false;

    value.clear();
    if (length == 0)
        return true;

    std::string bytes(length, '\0');
    state.input->read(bytes.data(), bytes.size());
    if (!state.input->good())
        return false;

    if (!bytes.empty() && bytes.back() == '\0')
        bytes.pop_back();

    value = std::move(bytes);
    return true;
}

bool SFMaterialDatabase::ReadResourceID(ReaderState& state, SFResourceID& id) {
    uint32_t file = 0;
    uint32_t ext = 0;
    uint32_t dir = 0;
    if (!ReadPod(*state.input, file) || !ReadPod(*state.input, ext) || !ReadPod(*state.input, dir))
        return false;

    id.dir = dir;
    id.file = file;
    id.ext = ext;
    return true;
}

bool SFMaterialDatabase::ReadComponentTypeMap(ReaderState& state) {
    Chunk mapChunk;
    if (!ReadChunk(state, mapChunk))
        return false;

    const std::streampos payloadStart = state.input->tellg();
    if (payloadStart == std::streampos(-1))
        return false;

    MapHeader map;
    if (!ReadMapHeader(state, mapChunk, map))
        return false;

    if (!HasExactCountedPayload(mapChunk, 12, map.size, CDBComponentTypeMapEntrySize))
        return false;

    componentTypes.reserve(map.size);
    for (uint32_t i = 0; i < map.size; ++i) {
        uint16_t key = 0;
        ComponentTypeInfo info;
        uint8_t isEmpty = 0;
        if (!ReadPod(*state.input, key)
            || !ReadPod(*state.input, info.version)
            || !ReadPod(*state.input, isEmpty)) {
            return false;
        }

        info.isEmpty = isEmpty != 0;
        componentTypes.emplace_back(key, std::move(info));
    }

    const std::streampos mapEnd = payloadStart + static_cast<std::streamoff>(mapChunk.size);
    if (!SkipTo(*state.input, mapEnd))
        return false;

    for (auto& type : componentTypes) {
        Chunk userChunk;
        if (!ReadChunk(state, userChunk) || (userChunk.sig != SigUSER && userChunk.sig != SigUSRD))
            return false;

        const std::streampos userPayloadStart = state.input->tellg();
        if (userPayloadStart == std::streampos(-1))
            return false;

        uint32_t targetType = 0;
        uint32_t castedType = 0;
        uint32_t userValue = 0;
        if (!ReadPod(*state.input, targetType) || !ReadPod(*state.input, castedType))
            return false;

        (void)targetType;
        (void)castedType;

        if (!ReadString(state, type.second.className))
            return false;

        if (!ReadPod(*state.input, userValue))
            return false;

        (void)userValue;

        const std::streampos userEnd = userPayloadStart + static_cast<std::streamoff>(userChunk.size);
        if (!SkipTo(*state.input, userEnd))
            return false;
    }

    return true;
}

bool SFMaterialDatabase::ReadObjectList(ReaderState& state) {
    Chunk listChunk;
    if (!ReadChunk(state, listChunk))
        return false;

    const std::streampos payloadStart = state.input->tellg();
    if (payloadStart == std::streampos(-1))
        return false;

    ListHeader list;
    if (!ReadListHeader(state, listChunk, list))
        return false;

    const bool hasParentPersistentID = ObjectInfoHasParentPersistentID();
    const uint32_t objectInfoSize = hasParentPersistentID
        ? CDBObjectInfoWithParentPersistentIDSize
        : CDBObjectInfoSize;
    if (!HasExactCountedPayload(listChunk, 8, list.size, objectInfoSize))
        return false;

    objects.reserve(list.size);
    for (uint32_t i = 0; i < list.size; ++i) {
        ObjectInfo object;
        uint8_t hasData = 0;
        if (!ReadResourceID(state, object.persistentID)
            || !ReadPod(*state.input, object.dbID)
            || !ReadPod(*state.input, object.parentID)) {
            return false;
        }

        if (hasParentPersistentID && !ReadResourceID(state, object.parentPersistentID))
            return false;

        if (!ReadPod(*state.input, hasData))
            return false;

        object.hasData = hasData != 0;
        objects.emplace_back(object);
    }

    const std::streampos listEnd = payloadStart + static_cast<std::streamoff>(listChunk.size);
    return SkipTo(*state.input, listEnd);
}

bool SFMaterialDatabase::ReadComponentList(ReaderState& state) {
    Chunk listChunk;
    if (!ReadChunk(state, listChunk))
        return false;

    const std::streampos payloadStart = state.input->tellg();
    if (payloadStart == std::streampos(-1))
        return false;

    ListHeader list;
    if (!ReadListHeader(state, listChunk, list))
        return false;

    if (!HasExactCountedPayload(listChunk, 8, list.size, CDBComponentInfoSize))
        return false;

    components.reserve(list.size);
    for (uint32_t i = 0; i < list.size; ++i) {
        ComponentInfo component;
        if (!ReadPod(*state.input, component.objectID)
            || !ReadPod(*state.input, component.index)
            || !ReadPod(*state.input, component.type)) {
            return false;
        }

        components.emplace_back(component);
    }

    const std::streampos listEnd = payloadStart + static_cast<std::streamoff>(listChunk.size);
    return SkipTo(*state.input, listEnd);
}

bool SFMaterialDatabase::ReadEdgeList(ReaderState& state) {
    Chunk listChunk;
    if (!ReadChunk(state, listChunk))
        return false;

    const std::streampos payloadStart = state.input->tellg();
    if (payloadStart == std::streampos(-1))
        return false;

    ListHeader list;
    if (!ReadListHeader(state, listChunk, list))
        return false;

    if (!HasExactCountedPayload(listChunk, 8, list.size, CDBEdgeInfoSize))
        return false;

    edges.reserve(list.size);
    for (uint32_t i = 0; i < list.size; ++i) {
        EdgeInfo edge;
        if (!ReadPod(*state.input, edge.sourceID)
            || !ReadPod(*state.input, edge.targetID)
            || !ReadPod(*state.input, edge.index)
            || !ReadPod(*state.input, edge.type)) {
            return false;
        }

        edges.emplace_back(edge);
    }

    const std::streampos listEnd = payloadStart + static_cast<std::streamoff>(listChunk.size);
    return SkipTo(*state.input, listEnd);
}

void SFMaterialDatabase::BuildLookupIndexes() {
    objectMap.reserve(objects.size());
    resourceToDb.reserve(objects.size());

    for (size_t i = 0; i < objects.size(); ++i) {
        const ObjectInfo& object = objects[i];
        objectMap.emplace(object.dbID, i);

        if (object.persistentID.ext == SFMatExtension)
            resourceToDb.emplace(object.persistentID, object.dbID);
    }

    componentMap.reserve(components.size());
    for (size_t i = 0; i < components.size(); ++i)
        componentMap[components[i].objectID].emplace_back(i);

    edgeMap.reserve(edges.size());
    for (size_t i = 0; i < edges.size(); ++i)
        edgeMap[edges[i].sourceID].emplace_back(i);
}

bool SFMaterialDatabase::HasExactCountedPayload(const Chunk& chunk, uint32_t headerSize, uint32_t count, uint32_t itemSize) const {
    if (chunk.size < headerSize)
        return false;

    const uint64_t expectedSize = static_cast<uint64_t>(headerSize)
        + static_cast<uint64_t>(count) * static_cast<uint64_t>(itemSize);
    return expectedSize == chunk.size;
}

bool SFMaterialDatabase::EnsureAllComponentsRead() {
    if (allComponentsRead)
        return true;

    if (!componentStream || componentDataStart == std::streampos(-1))
        return false;

    componentStream->clear();
    componentStream->seekg(componentDataStart);
    if (!componentStream->good())
        return false;

    ReaderState state;
    state.input = componentStream;
    state.version = readerVersion;
    state.chunksRemaining = readerChunksRemaining;

    componentPositions.clear();
    componentJsonCache.clear();
    componentPositions.reserve(components.size());
    componentJsonCache.reserve(components.size());

    for (size_t i = 0; i < components.size(); ++i) {
        const std::streampos componentPos = componentStream->tellg();
        if (componentPos == std::streampos(-1))
            return false;

        componentPositions.emplace_back(componentPos);
        componentJsonCache.emplace_back(nlohmann::json::object());
        if (!ReadNextObject(state, componentJsonCache.back())) {
            componentPositions.clear();
            componentJsonCache.clear();
            return false;
        }
    }

    readerChunksRemaining = state.chunksRemaining;
    allComponentsRead = true;
    return true;
}

bool SFMaterialDatabase::ReadNextObject(ReaderState& state, nlohmann::json& value) {
    if (!ReadChunk(state, value))
        return false;

    while (!state.chunkQueue.empty() || !state.userQueue.empty()) {
        if (!ReadChunk(state, value))
            return false;
    }

    return true;
}

bool SFMaterialDatabase::ReadChunk(ReaderState& state, nlohmann::json& value) {
    Chunk chunk;
    if (!ReadChunk(state, chunk))
        return false;

    const std::streampos payloadStart = state.input->tellg();
    if (payloadStart == std::streampos(-1))
        return false;

    const std::streampos payloadEnd = payloadStart + static_cast<std::streamoff>(chunk.size);
    const bool isDiff = chunk.sig == SigDIFF || chunk.sig == SigUSRD;
    bool readValue = false;

    switch (chunk.sig) {
    case SigOBJT:
    case SigDIFF:
    {
        if (chunk.size < 4)
            return false;

        uint32_t typeRef = 0;
        readValue = ReadPod(*state.input, typeRef)
            && ReadType(state, value, typeRef, isDiff);
        break;
    }
    case SigUSER:
    case SigUSRD:
    {
        if (chunk.size < 12 || state.userQueue.empty())
            return false;

        uint32_t targetType = 0;
        uint32_t castedType = 0;
        if (!ReadPod(*state.input, targetType) || !ReadPod(*state.input, castedType))
            return false;

        QueuedCast cast = state.userQueue.front();
        state.userQueue.pop_front();

        uint32_t userValue = 0;
        readValue = cast.value
            && ReadType(state, *cast.value, castedType, isDiff, true)
            && ReadPod(*state.input, userValue);

        (void)targetType;
        (void)userValue;
        break;
    }
    case SigLIST:
    case SigMAPC:
    {
        if (state.chunkQueue.empty())
            return false;

        QueuedChunk queuedChunk = state.chunkQueue.front();
        state.chunkQueue.pop_front();
        if (!queuedChunk.value)
            return false;

        readValue = chunk.sig == SigLIST
            ? ReadList(state, *queuedChunk.value, queuedChunk.isDiff)
            : ReadMap(state, *queuedChunk.value, queuedChunk.isDiff);
        break;
    }
    default:
        return false;
    }

    return readValue && SkipTo(*state.input, payloadEnd);
}

bool SFMaterialDatabase::ReadType(ReaderState& state, nlohmann::json& value, uint32_t typeRef, bool isDiff, bool isCast) {
    switch (typeRef) {
    case TypeNull:
        value = nullptr;
        return true;
    case TypeString:
    {
        std::string stringValue;
        if (!ReadString(state, stringValue))
            return false;

        value = std::move(stringValue);
        return true;
    }
    case TypeList:
    case TypeMap:
        return false;
    case TypeRef:
    {
        uint32_t referencedType = 0;
        if (!ReadPod(*state.input, referencedType))
            return false;

        if (IsBuiltinTypeRef(referencedType)) {
            if (referencedType != TypeNull)
                return false;

            value = nullptr;
            return true;
        }

        const CDBClass* referencedClass = FindClass(referencedType);
        if (!referencedClass)
            return false;

        value["Type"] = "<ref>";
        nlohmann::json& dataValue = value["Data"];
        if (referencedClass->IsUser()) {
            state.userQueue.push_back({ &dataValue, referencedType });
            return true;
        }

        return ReadType(state, dataValue, referencedType, isDiff);
    }
    case TypeInt8:
    {
        int8_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt8:
    {
        uint8_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt16:
    {
        int16_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt16:
    {
        uint16_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt32:
    {
        int32_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt32:
    {
        uint32_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt64:
    {
        int64_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt64:
    {
        uint64_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeBool:
    {
        uint8_t boolValue = 0;
        if (!ReadPod(*state.input, boolValue))
            return false;

        value = boolValue != 0 ? "true" : "false";
        return true;
    }
    case TypeFloat:
    {
        float numericValue = 0.0f;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeDouble:
    {
        double numericValue = 0.0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    default:
        break;
    }

    const char* typeName = GetString(typeRef);
    if (EqualsIgnoreCase(typeName, "BSComponentDB2::ID")) {
        uint32_t id = 0;
        if (!isDiff) {
            if (!ReadPod(*state.input, id))
                return false;
        }
        else {
            uint16_t fieldPadBegin = 0;
            uint16_t fieldPadEnd = 0;
            if (!ReadPod(*state.input, fieldPadBegin)
                || !ReadPod(*state.input, id)
                || !ReadPod(*state.input, fieldPadEnd)) {
                return false;
            }
        }

        value = id != 0 ? std::to_string(id) : "";
        return true;
    }

    const CDBClass* cdbClass = FindClass(typeRef);
    if (!cdbClass)
        return false;

    if (!isCast && cdbClass->IsUser()) {
        state.userQueue.push_back({ &value, typeRef });
        return true;
    }

    value["Type"] = GetString(cdbClass->nameOffset);
    nlohmann::json& dataValue = value["Data"];
    dataValue = nlohmann::json::object();

    if (!isDiff) {
        for (const ClassField& field : cdbClass->fields) {
            nlohmann::json& fieldValue = dataValue[GetString(field.nameOffset)];
            if (IsChunkTypeRef(field.typeId))
                state.chunkQueue.push_back({ &fieldValue, isDiff });
            else if (!ReadType(state, fieldValue, field.typeId, isDiff))
                return false;
        }

        return true;
    }

    uint16_t fieldIndex = 0;
    if (!ReadPod(*state.input, fieldIndex))
        return false;

    while (fieldIndex != 0xFFFFu) {
        if (fieldIndex >= cdbClass->fields.size())
            return false;

        const ClassField& field = cdbClass->fields[fieldIndex];
        nlohmann::json& fieldValue = dataValue[GetString(field.nameOffset)];
        if (IsChunkTypeRef(field.typeId))
            state.chunkQueue.push_back({ &fieldValue, isDiff });
        else if (!ReadType(state, fieldValue, field.typeId, isDiff))
            return false;

        if (!ReadPod(*state.input, fieldIndex))
            return false;
    }

    return true;
}

bool SFMaterialDatabase::ReadList(ReaderState& state, nlohmann::json& value, bool isDiff) {
    ListHeader list;
    if (!ReadPod(*state.input, list.elementType) || !ReadPod(*state.input, list.size))
        return false;

    value["Type"] = "<collection>";
    nlohmann::json& dataValue = value["Data"];
    dataValue = nlohmann::json::array();

    if (list.size == 0)
        return true;

    value["ElementType"] = GetTypeName(list.elementType);
    dataValue.get_ref<nlohmann::json::array_t&>().reserve(list.size);
    for (uint32_t i = 0; i < list.size; ++i) {
        nlohmann::json& itemValue = dataValue.emplace_back();
        if (!ReadType(state, itemValue, list.elementType, isDiff))
            return false;
    }

    return true;
}

bool SFMaterialDatabase::ReadMap(ReaderState& state, nlohmann::json& value, bool isDiff) {
    MapHeader map;
    if (!ReadPod(*state.input, map.keyType)
        || !ReadPod(*state.input, map.valueType)
        || !ReadPod(*state.input, map.size)) {
        return false;
    }

    value["Type"] = "<collection>";
    value["ElementType"] = "StdMapType::Pair";
    nlohmann::json& dataValue = value["Data"];
    dataValue = nlohmann::json::array();

    if (map.size == 0)
        return true;

    dataValue.get_ref<nlohmann::json::array_t&>().reserve(map.size);
    for (uint32_t i = 0; i < map.size; ++i) {
        std::string key;
        if (!ReadMapKeyString(state, map.keyType, key))
            return false;

        nlohmann::json& pairValue = dataValue.emplace_back();
        pairValue["Type"] = "StdMapType::Pair";
        nlohmann::json& pairData = pairValue["Data"];
        pairData["Key"] = std::move(key);
        if (!ReadType(state, pairData["Value"], map.valueType, isDiff))
            return false;
    }

    return true;
}

bool SFMaterialDatabase::ReadMapKeyString(ReaderState& state, uint32_t typeRef, std::string& value) {
    switch (typeRef) {
    case TypeString:
        return ReadString(state, value);
    case TypeInt8:
    {
        int8_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt8:
    {
        uint8_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt16:
    {
        int16_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt16:
    {
        uint16_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt32:
    {
        int32_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt32:
    {
        uint32_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeInt64:
    {
        int64_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeUInt64:
    {
        uint64_t numericValue = 0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeBool:
    {
        uint8_t boolValue = 0;
        if (!ReadPod(*state.input, boolValue))
            return false;

        value = boolValue != 0 ? "true" : "false";
        return true;
    }
    case TypeFloat:
    {
        float numericValue = 0.0f;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    case TypeDouble:
    {
        double numericValue = 0.0;
        if (!ReadPod(*state.input, numericValue))
            return false;

        value = std::to_string(numericValue);
        return true;
    }
    default:
        break;
    }

    if (EqualsIgnoreCase(GetTypeName(typeRef), "BSResource::ID")) {
        SFResourceID id;
        if (!ReadResourceID(state, id))
            return false;

        value = FormatResourceID(id);
        return true;
    }

    return false;
}

const char* SFMaterialDatabase::GetString(uint32_t offset) const {
    if (offset >= stringTable.size())
        return "";

    const auto begin = stringTable.begin() + static_cast<std::ptrdiff_t>(offset);
    if (std::find(begin, stringTable.end(), '\0') == stringTable.end())
        return "";

    return stringTable.data() + offset;
}

const char* SFMaterialDatabase::GetBuiltinTypeName(uint32_t typeRef) const {
    if ((typeRef & TypeBuiltinMask) != TypeBuiltinMask)
        return nullptr;

    const uint32_t index = typeRef & 0xFFu;
    if (index >= BuiltinTypeNames.size())
        return "";

    return BuiltinTypeNames[index];
}

const char* SFMaterialDatabase::GetTypeName(uint32_t typeRef) const {
    if (const char* builtinName = GetBuiltinTypeName(typeRef))
        return builtinName;

    if (const CDBClass* cdbClass = FindClass(typeRef))
        return GetString(cdbClass->nameOffset);

    return GetString(typeRef);
}

const SFMaterialDatabase::CDBClass* SFMaterialDatabase::FindClass(uint32_t nameOffset) const {
    const auto it = std::find_if(classes.begin(), classes.end(), [nameOffset](const CDBClass& cdbClass) {
        return cdbClass.nameOffset == nameOffset;
    });

    return it != classes.end() ? &*it : nullptr;
}

const SFMaterialDatabase::CDBClass* SFMaterialDatabase::FindClassByName(const char* name) const {
    const auto it = std::find_if(classes.begin(), classes.end(), [this, name](const CDBClass& cdbClass) {
        return EqualsIgnoreCase(GetString(cdbClass.nameOffset), name);
    });

    return it != classes.end() ? &*it : nullptr;
}

bool SFMaterialDatabase::ClassHasField(const CDBClass& cdbClass, const char* fieldName) const {
    return std::find_if(cdbClass.fields.begin(), cdbClass.fields.end(), [this, fieldName](const ClassField& field) {
        return EqualsIgnoreCase(GetString(field.nameOffset), fieldName);
    }) != cdbClass.fields.end();
}

bool SFMaterialDatabase::ObjectInfoHasParentPersistentID() const {
    const CDBClass* objectInfoClass = FindClassByName("BSComponentDB2::DBFileIndex::ObjectInfo");
    return objectInfoClass && ClassHasField(*objectInfoClass, "ParentPersistentID");
}
