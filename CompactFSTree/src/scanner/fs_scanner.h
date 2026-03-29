#pragma once

#include "core/fs_traits.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QString>

struct ScannedEntry {
    std::string name;       // filename only, not full path
    uint16_t    flags;      // same encoding as Node::flags
    uint64_t    file_size;
    uint64_t    mtime_ns;   // nanoseconds since Unix epoch
};

class FSScanner {
public:
    virtual ~FSScanner() = default;

    // Return filesystem traits (case sensitivity, normalization, etc.).
    virtual FSTraits traits() const = 0;

    // Scan one directory level. Returns immediate children only.
    virtual std::vector<ScannedEntry> scan_directory(const QString& path) = 0;

    // Returns true if the directory may have changed since stored_mtime_ns.
    // Implementations must account for FS-specific mtime resolution.
    virtual bool has_changed(const QString& path, uint64_t stored_mtime_ns) = 0;

    // Normalize a filename according to FS rules (case folding, NFC, etc.).
    // Called once per entry before it enters the builder.
    virtual std::string normalize_name(const std::string& raw_name) = 0;

    // Factory: detect FS type from mount point and return the right scanner.
    static std::unique_ptr<FSScanner> create(const QString& root_path);
};
