#pragma once

#include "fs_scanner.h"

// Scanner for APFS volumes (macOS).
// Normalises filenames from NFD to NFC and optionally folds case for
// case-insensitive volumes (the default for APFS).
class APFSScanner : public FSScanner {
public:
    // case_sensitive: pass true only for volumes created as case-sensitive.
    explicit APFSScanner(bool case_sensitive = false);

    FSTraits traits() const override;
    std::vector<ScannedEntry> scan_directory(const QString& path) override;
    bool has_changed(const QString& path, uint64_t stored_mtime_ns) override;
    std::string normalize_name(const std::string& raw_name) override;

private:
    bool case_sensitive_;

    static uint64_t get_mtime_ns(const QString& path);
};
