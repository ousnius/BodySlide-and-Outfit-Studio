#include "SFResourceHash.h"

#include <array>
#include <functional>

namespace {
constexpr std::array<unsigned char, 256> MakeLowerBackslashMap() {
    std::array<unsigned char, 256> result = {};
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<unsigned char>(i);

    for (unsigned char c = 'A'; c <= 'Z'; ++c)
        result[c] = static_cast<unsigned char>(c + 0x20);

    result[static_cast<unsigned char>('/')] = static_cast<unsigned char>('\\');
    return result;
}

constexpr std::array<uint32_t, 256> MakeCrcMap() {
    std::array<uint32_t, 256> result = {};
    for (uint32_t i = 0; i < result.size(); ++i) {
        uint32_t crc = i;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 1u) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);

        result[i] = crc;
    }

    return result;
}

uint32_t GetPackedExtension(std::string_view path, size_t dotPos) {
    if (dotPos == std::string_view::npos)
        return 0;

    uint32_t result = 0;
    constexpr auto lowerBackslashMap = MakeLowerBackslashMap();
    for (size_t i = 0; i < 4 && dotPos + 1 + i < path.size(); ++i) {
        const auto c = lowerBackslashMap[static_cast<unsigned char>(path[dotPos + 1 + i])];
        result |= static_cast<uint32_t>(c) << (8 * i);
    }

    return result;
}
}

size_t SFResourceIDHash::operator()(const SFResourceID& id) const noexcept {
    size_t value = std::hash<uint64_t>()((static_cast<uint64_t>(id.dir) << 32) | id.file);
    value ^= std::hash<uint32_t>()(id.ext) + 0x9E3779B9u + (value << 6) + (value >> 2);
    return value;
}

uint32_t SFGetCrc(std::string_view sv) {
    uint32_t result = 0;
    constexpr auto crcMap = MakeCrcMap();
    constexpr auto lowerBackslashMap = MakeLowerBackslashMap();

    for (const char c : sv) {
        const auto normalized = lowerBackslashMap[static_cast<unsigned char>(c)];
        result = crcMap[(normalized ^ result) & 0xFFu] ^ (result >> 8);
    }

    return result;
}

SFResourceID SFGetResourceIdFromPath(const std::string& path) {
    const size_t slashPos = path.find_last_of("/\\");
    const size_t dotPos = path.find_last_of('.');
    const bool hasExtension = dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos);
    const size_t nameEnd = hasExtension ? dotPos : path.size();

    SFResourceID result;
    if (slashPos != std::string::npos) {
        result.dir = SFGetCrc(std::string_view(path.data(), slashPos));
        result.file = SFGetCrc(std::string_view(path.data() + slashPos + 1, nameEnd - slashPos - 1));
    }
    else {
        result.file = SFGetCrc(std::string_view(path.data(), nameEnd));
    }

    result.ext = GetPackedExtension(path, hasExtension ? dotPos : std::string_view::npos);
    return result;
}
