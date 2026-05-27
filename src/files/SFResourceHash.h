#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct SFResourceID {
    uint32_t dir = 0;
    uint32_t file = 0;
    uint32_t ext = 0;

    bool operator==(const SFResourceID& rhs) const {
        return dir == rhs.dir && file == rhs.file && ext == rhs.ext;
    }
};

struct SFResourceIDHash {
    size_t operator()(const SFResourceID& id) const noexcept;
};

uint32_t SFGetCrc(std::string_view sv);
SFResourceID SFGetResourceIdFromPath(const std::string& path);
