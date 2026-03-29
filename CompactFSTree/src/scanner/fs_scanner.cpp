#include "fs_scanner.h"

#include "ntfs_scanner.h"
#include "fat_scanner.h"
#include "posix_scanner.h"
#include "apfs_scanner.h"
#include "hfsplus_scanner.h"

#include <QStorageInfo>
#include <QtGlobal>

std::unique_ptr<FSScanner> FSScanner::create(const QString& root_path) {
    QString fsType = QStorageInfo(root_path).fileSystemType().toLower();

    // Linux / generic POSIX
    if (fsType == "ext2" || fsType == "ext3" || fsType == "ext4" ||
        fsType == "btrfs" || fsType == "xfs")
        return std::make_unique<PosixScanner>();

    // FAT32
    if (fsType == "vfat" || fsType == "msdos" || fsType == "fat32")
        return std::make_unique<FATScanner>(2'000'000'000ULL);

    // exFAT
    if (fsType == "exfat")
        return std::make_unique<FATScanner>(10'000'000ULL);

    // APFS (macOS) — default to case-insensitive
    if (fsType == "apfs")
        return std::make_unique<APFSScanner>(false);

    // HFS+
    if (fsType == "hfs" || fsType == "hfs+")
        return std::make_unique<HFSPlusScanner>();

    // NTFS (Windows native, Linux ntfs3 driver, FUSE ntfs-3g)
    if (fsType == "ntfs" || fsType == "ntfs3" || fsType == "fuseblk")
        return std::make_unique<NTFSScanner>();

    // Unknown filesystem: warn and fall back to NTFS scanner on Windows,
    // PosixScanner elsewhere.
    qWarning("FSScanner::create: unknown filesystem type \"%s\" for \"%s\"; "
             "using fallback scanner.",
             qPrintable(fsType), qPrintable(root_path));

#ifdef _WIN32
    return std::make_unique<NTFSScanner>();
#else
    return std::make_unique<PosixScanner>();
#endif
}
