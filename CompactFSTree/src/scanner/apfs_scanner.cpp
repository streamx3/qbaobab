#include "apfs_scanner.h"

#include "core/node.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#if defined(__APPLE__)
#  include <sys/stat.h>
#  define USE_POSIX_STAT 1
#endif

#include <cctype>

APFSScanner::APFSScanner(bool case_sensitive)
    : case_sensitive_(case_sensitive)
{}

FSTraits APFSScanner::traits() const {
    return FSTraits{
        case_sensitive_ ? CaseSensitivity::Sensitive : CaseSensitivity::Insensitive,
        NormForm::NFC,   // names normalised to NFC at scan time
        1,               // nanosecond mtime resolution
        true             // APFS has stable file IDs
    };
}

uint64_t APFSScanner::get_mtime_ns(const QString& path) {
#ifdef USE_POSIX_STAT
    struct stat st{};
    if (::stat(path.toLocal8Bit().constData(), &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_mtimespec.tv_sec)  * 1'000'000'000ULL
         + static_cast<uint64_t>(st.st_mtimespec.tv_nsec);
#else
    QFileInfo fi(path);
    if (!fi.exists()) return 0;
    return static_cast<uint64_t>(fi.lastModified().toMSecsSinceEpoch()) * 1'000'000ULL;
#endif
}

std::vector<ScannedEntry> APFSScanner::scan_directory(const QString& path) {
    std::vector<ScannedEntry> entries;

    QDirIterator it(path,
                    QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        ScannedEntry e;
        // normalize_name handles NFD→NFC conversion.
        e.name      = normalize_name(fi.fileName().toStdString());
        e.file_size = fi.isDir() ? 0 : static_cast<uint64_t>(fi.size());

        if (fi.isDir())          e.flags = NODE_TYPE_DIR;
        else if (fi.isSymLink()) e.flags = NODE_TYPE_SYMLINK;
        else                     e.flags = NODE_TYPE_FILE;

        if (fi.isHidden()) e.flags |= NODE_FLAG_HIDDEN;

        e.mtime_ns = get_mtime_ns(fi.absoluteFilePath());
        entries.push_back(std::move(e));
    }

    return entries;
}

bool APFSScanner::has_changed(const QString& path, uint64_t stored_mtime_ns) {
    return get_mtime_ns(path) != stored_mtime_ns;
}

std::string APFSScanner::normalize_name(const std::string& raw_name) {
    // NFD → NFC via Qt (scan-time only; acceptable conversion cost).
    QString qs = QString::fromStdString(raw_name)
                     .normalized(QString::NormalizationForm_C);

    if (!case_sensitive_) {
        qs = qs.toLower();
    }

    return qs.toStdString();
}
