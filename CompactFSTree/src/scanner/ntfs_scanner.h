#pragma once

#include "fs_scanner.h"

// NTFS scanner. Uses QDirIterator for directory listing.
// On Windows, uses GetFileAttributesExW for 100-ns mtime precision.
// On other platforms, falls back to QFileInfo with millisecond precision.
class NTFSScanner : public FSScanner {
public:
    FSTraits traits() const override;
    std::vector<ScannedEntry> scan_directory(const QString& path) override;
    bool has_changed(const QString& path, uint64_t stored_mtime_ns) override;
    std::string normalize_name(const std::string& raw_name) override;

private:
    // Returns mtime in nanoseconds since Unix epoch for a given path.
    static uint64_t get_mtime_ns(const QString& path);
};
