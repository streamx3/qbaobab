#pragma once

#include <cstdint>

enum class CaseSensitivity : uint8_t {
    Sensitive,
    Insensitive
};

enum class NormForm : uint8_t {
    None,
    NFC,
    NFD
};

struct FSTraits {
    CaseSensitivity case_sensitivity;
    NormForm        name_normalization;
    uint64_t        mtime_resolution_ns; // e.g. 2'000'000'000 for FAT32, 1 for ext4
    bool            has_file_ids;
};
