#pragma once

#include "fs_scanner.h"

// Scanner for FAT32 and exFAT volumes.
// Constructed with the appropriate mtime resolution:
//   FAT32 : 2'000'000'000 ns  (2 s)
//   exFAT : 10'000'000 ns     (10 ms)
class FATScanner : public FSScanner {
public:
    explicit FATScanner(uint64_t mtime_resolution_ns);

    FSTraits traits() const override;
    std::vector<ScannedEntry> scan_directory(const QString& path) override;
    bool has_changed(const QString& path, uint64_t stored_mtime_ns) override;
    std::string normalize_name(const std::string& raw_name) override;

private:
    uint64_t mtime_resolution_ns_;

    static uint64_t get_mtime_ns(const QString& path);
};
