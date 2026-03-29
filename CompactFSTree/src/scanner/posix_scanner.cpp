#include "posix_scanner.h"

#include "core/node.h"

#if defined(__unix__) || defined(__APPLE__)
#  include <dirent.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <cerrno>
#  define USE_POSIX_API 1
#else
#  include <QDirIterator>
#  include <QFileInfo>
#endif

FSTraits PosixScanner::traits() const {
    return FSTraits{
        CaseSensitivity::Sensitive,
        NormForm::None,
        1,      // nanosecond resolution (ext4, btrfs, xfs)
        true    // inodes as file IDs
    };
}

uint64_t PosixScanner::get_mtime_ns(const QString& path) {
#ifdef USE_POSIX_API
    struct stat st{};
    if (::stat(path.toLocal8Bit().constData(), &st) != 0) return 0;
#  if defined(__APPLE__)
    return static_cast<uint64_t>(st.st_mtimespec.tv_sec)  * 1'000'000'000ULL
         + static_cast<uint64_t>(st.st_mtimespec.tv_nsec);
#  else
    return static_cast<uint64_t>(st.st_mtim.tv_sec)  * 1'000'000'000ULL
         + static_cast<uint64_t>(st.st_mtim.tv_nsec);
#  endif
#else
    QFileInfo fi(path);
    if (!fi.exists()) return 0;
    return static_cast<uint64_t>(fi.lastModified().toMSecsSinceEpoch()) * 1'000'000ULL;
#endif
}

std::vector<ScannedEntry> PosixScanner::scan_directory(const QString& path) {
    std::vector<ScannedEntry> entries;

#ifdef USE_POSIX_API
    const std::string path_str = path.toLocal8Bit().toStdString();
    DIR* dir = ::opendir(path_str.c_str());
    if (!dir) return entries;

    int dir_fd = ::dirfd(dir);

    struct dirent* ent;
    while ((ent = ::readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.' &&
            (ent->d_name[1] == '\0' ||
             (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
            continue;

        struct stat st{};
        if (::fstatat(dir_fd, ent->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
            continue;

        ScannedEntry e;
        e.name = ent->d_name;

        if (S_ISDIR(st.st_mode)) {
            e.flags     = NODE_TYPE_DIR;
            e.file_size = 0;
        } else if (S_ISLNK(st.st_mode)) {
            e.flags     = NODE_TYPE_SYMLINK;
            e.file_size = static_cast<uint64_t>(st.st_size);
        } else if (S_ISREG(st.st_mode)) {
            e.flags     = NODE_TYPE_FILE;
            e.file_size = static_cast<uint64_t>(st.st_size);
        } else {
            e.flags     = NODE_TYPE_OTHER;
            e.file_size = 0;
        }

        // Hidden on POSIX = dot-prefix
        if (ent->d_name[0] == '.') e.flags |= NODE_FLAG_HIDDEN;

#  if defined(__APPLE__)
        e.mtime_ns = static_cast<uint64_t>(st.st_mtimespec.tv_sec)  * 1'000'000'000ULL
                   + static_cast<uint64_t>(st.st_mtimespec.tv_nsec);
#  else
        e.mtime_ns = static_cast<uint64_t>(st.st_mtim.tv_sec)  * 1'000'000'000ULL
                   + static_cast<uint64_t>(st.st_mtim.tv_nsec);
#  endif

        entries.push_back(std::move(e));
    }

    ::closedir(dir);

#else
    // Windows fallback
    QDirIterator it(path,
                    QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        ScannedEntry e;
        e.name      = fi.fileName().toStdString();
        e.file_size = fi.isDir() ? 0 : static_cast<uint64_t>(fi.size());

        if (fi.isDir())        e.flags = NODE_TYPE_DIR;
        else if (fi.isSymLink()) e.flags = NODE_TYPE_SYMLINK;
        else                   e.flags = NODE_TYPE_FILE;

        if (fi.isHidden()) e.flags |= NODE_FLAG_HIDDEN;

        e.mtime_ns = get_mtime_ns(fi.absoluteFilePath());
        entries.push_back(std::move(e));
    }
#endif

    return entries;
}

bool PosixScanner::has_changed(const QString& path, uint64_t stored_mtime_ns) {
    // Exact nanosecond comparison.
    return get_mtime_ns(path) != stored_mtime_ns;
}

std::string PosixScanner::normalize_name(const std::string& raw_name) {
    return raw_name; // POSIX filesystems: no normalization
}
