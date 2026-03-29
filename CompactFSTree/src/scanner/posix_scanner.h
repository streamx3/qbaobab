#pragma once

#include "fs_scanner.h"

// Default scanner for ext3, ext4, BtrFS, XFS and other POSIX filesystems.
// On POSIX platforms uses opendir/readdir/fstatat for efficiency.
// On Windows falls back to QDirIterator.
class PosixScanner : public FSScanner {
public:
    FSTraits traits() const override;
    std::vector<ScannedEntry> scan_directory(const QString& path) override;
    bool has_changed(const QString& path, uint64_t stored_mtime_ns) override;
    std::string normalize_name(const std::string& raw_name) override;

private:
    static uint64_t get_mtime_ns(const QString& path);
};
