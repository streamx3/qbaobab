#ifndef DRIVELISTMODEL_H
#define DRIVELISTMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <QString>
#include <QVector>

struct DriveEntry {
    enum Type { HomeFolder, Drive, UserFolder };

    Type type = UserFolder;
    QString displayName;
    QString path;
    qint64 totalBytes = 0;
    qint64 usedBytes = -1;   // -1 means not applicable (non-drive)
    QIcon icon;
};

class DriveListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        PathRole,
        DisplayNameRole,
        TotalBytesRole,
        UsedBytesRole,
        IconRole,
    };

    explicit DriveListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addUserFolder(const QString &path);

private:
    void populate();

    QVector<DriveEntry> m_entries;
};

#endif // DRIVELISTMODEL_H
