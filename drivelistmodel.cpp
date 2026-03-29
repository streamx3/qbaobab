#include "drivelistmodel.h"

#include <QFileIconProvider>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStorageInfo>

DriveListModel::DriveListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    populate();
}

int DriveListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DriveListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const DriveEntry &e = m_entries.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:   return e.displayName;
    case TypeRole:          return int(e.type);
    case PathRole:          return e.path;
    case TotalBytesRole:    return e.totalBytes;
    case UsedBytesRole:     return e.usedBytes;
    case IconRole:          return e.icon;
    default:                return {};
    }
}

void DriveListModel::populate()
{
    QFileIconProvider iconProvider;

    // 1. Home folder
    {
        DriveEntry home;
        home.type = DriveEntry::HomeFolder;
        home.path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        home.displayName = tr("Home Folder");
        home.icon = iconProvider.icon(QFileInfo(home.path));

        QStorageInfo si(home.path);
        if (si.isValid()) {
            home.totalBytes = si.bytesTotal();
        }
        home.usedBytes = -1; // not a drive

        m_entries.append(home);
    }

    // 2. Mounted drives
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &si : volumes) {
        if (!si.isReady() || !si.isValid())
            continue;

#ifdef Q_OS_LINUX
        // Skip virtual filesystems on Linux
        const QByteArray fsType = si.fileSystemType();
        static const QList<QByteArray> virtualFs = {
            "tmpfs", "proc", "sysfs", "devtmpfs", "squashfs",
            "cgroup", "cgroup2", "overlay", "devpts", "mqueue",
            "hugetlbfs", "debugfs", "tracefs", "securityfs",
            "pstore", "binfmt_misc", "fusectl", "configfs",
            "efivarfs", "autofs", "fuse.portal"
        };
        if (virtualFs.contains(fsType))
            continue;
#endif

        DriveEntry drive;
        drive.type = DriveEntry::Drive;
        drive.path = si.rootPath();
        drive.totalBytes = si.bytesTotal();
        drive.usedBytes = si.bytesTotal() - si.bytesAvailable();
        drive.icon = iconProvider.icon(QFileInfo(drive.path));

        // Build display name
        QString name = si.displayName();
        if (name.isEmpty())
            name = si.rootPath();

#ifdef Q_OS_WIN
        // On Windows, show "Name (C:)" style
        QString rootPath = si.rootPath();          // e.g. "C:/"
        QString letter = rootPath.left(2);         // e.g. "C:"
        if (name == rootPath || name == letter)
            name = tr("Local Disk");
        drive.displayName = QString("%1 (%2)").arg(name, letter);
#else
        drive.displayName = name;
#endif

        m_entries.append(drive);
    }
}

void DriveListModel::addUserFolder(const QString &path)
{
    QFileIconProvider iconProvider;

    DriveEntry folder;
    folder.type = DriveEntry::UserFolder;
    folder.path = path;
    folder.displayName = QFileInfo(path).fileName();
    if (folder.displayName.isEmpty())
        folder.displayName = path;
    folder.icon = iconProvider.icon(QFileInfo(path));

    QStorageInfo si(path);
    if (si.isValid())
        folder.totalBytes = si.bytesTotal();
    folder.usedBytes = -1;

    beginInsertRows({}, m_entries.size(), m_entries.size());
    m_entries.append(folder);
    endInsertRows();
}
