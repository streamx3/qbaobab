#include "ntfs_scanner.h"

#include "core/node.h"

#include <QDirIterator>
#include <QFileInfo>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

static uint64_t filetime_to_unix_ns(FILETIME ft) {
    // FILETIME: 100-ns intervals since 1601-01-01 UTC
    // Unix epoch: 1970-01-01 UTC
    // Difference: 116444736000000000 x 100-ns ticks
    constexpr uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
    uint64_t ticks = (static_cast<uint64_t>(ft.dwHighDateTime) << 32)
                   | static_cast<uint64_t>(ft.dwLowDateTime);
    if (ticks < EPOCH_DIFF_100NS) return 0;
    return (ticks - EPOCH_DIFF_100NS) * 100; // convert to nanoseconds
}
#endif

// ---------------------------------------------------------------------------

FSTraits NTFSScanner::traits() const {
    return FSTraits{
        CaseSensitivity::Insensitive,
        NormForm::None,
        100,   // 100 ns NTFS resolution
        true   // NTFS has file IDs
    };
}

uint64_t NTFSScanner::get_mtime_ns(const QString& path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(reinterpret_cast<LPCWSTR>(path.utf16()),
                              GetFileExInfoStandard, &info)) {
        return filetime_to_unix_ns(info.ftLastWriteTime);
    }
    return 0;
#else
    QFileInfo fi(path);
    if (!fi.exists()) return 0;
    return static_cast<uint64_t>(fi.lastModified().toMSecsSinceEpoch()) * 1'000'000ULL;
#endif
}

std::vector<ScannedEntry> NTFSScanner::scan_directory(const QString& path) {
    std::vector<ScannedEntry> entries;

    QDirIterator it(path,
                    QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        ScannedEntry e;
        e.name      = fi.fileName().toStdString();
        e.file_size = fi.isDir() ? 0 : static_cast<uint64_t>(fi.size());

        if (fi.isDir())
            e.flags = NODE_TYPE_DIR;
        else if (fi.isSymLink())
            e.flags = NODE_TYPE_SYMLINK;
        else
            e.flags = NODE_TYPE_FILE;

        if (fi.isHidden())
            e.flags |= NODE_FLAG_HIDDEN;

        e.mtime_ns = get_mtime_ns(fi.absoluteFilePath());
        entries.push_back(std::move(e));
    }

    return entries;
}

bool NTFSScanner::has_changed(const QString& path, uint64_t stored_mtime_ns) {
    uint64_t current = get_mtime_ns(path);
    // NTFS resolution is 100 ns; snap both to the nearest 100 ns boundary.
    constexpr uint64_t RES = 100;
    return (current / RES) != (stored_mtime_ns / RES);
}

std::string NTFSScanner::normalize_name(const std::string& raw_name) {
    // Names are stored as-is; case-insensitive comparison handled at query time.
    return raw_name;
}
