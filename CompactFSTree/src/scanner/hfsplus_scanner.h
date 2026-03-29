#pragma once

#include "fs_scanner.h"

// Scanner for HFS+ volumes (older macOS / external drives).
// Case-insensitive, NFD-normalised names, 1-second mtime resolution.
class HFSPlusScanner : public FSScanner {
public:
    FSTraits traits() const override;
    std::vector<ScannedEntry> scan_directory(const QString& path) override;
    bool has_changed(const QString& path, uint64_t stored_mtime_ns) override;
    std::string normalize_name(const std::string& raw_name) override;

private:
    static uint64_t get_mtime_ns(const QString& path);
};
