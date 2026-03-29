#include "fat_scanner.h"

#include "core/node.h"

#include <QDirIterator>
#include <QFileInfo>

#include <cctype>

FATScanner::FATScanner(uint64_t mtime_resolution_ns)
    : mtime_resolution_ns_(mtime_resolution_ns)
{}

FSTraits FATScanner::traits() const {
    return FSTraits{
        CaseSensitivity::Insensitive,
        NormForm::None,
        mtime_resolution_ns_,
        false   // FAT has no stable file IDs
    };
}

uint64_t FATScanner::get_mtime_ns(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists()) return 0;
    return static_cast<uint64_t>(fi.lastModified().toMSecsSinceEpoch()) * 1'000'000ULL;
}

std::vector<ScannedEntry> FATScanner::scan_directory(const QString& path) {
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

bool FATScanner::has_changed(const QString& path, uint64_t stored_mtime_ns) {
    uint64_t current = get_mtime_ns(path);
    uint64_t res     = mtime_resolution_ns_;
    // Truncate both timestamps to resolution boundary before comparing.
    return (current / res) != (stored_mtime_ns / res);
}

std::string FATScanner::normalize_name(const std::string& raw_name) {
    // FAT names are case-insensitive; fold to lowercase so the string pool
    // stores a canonical form. ASCII tolower is correct for FAT short names;
    // long names use Unicode but we keep it simple here.
    std::string result = raw_name;
    for (char& c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}
