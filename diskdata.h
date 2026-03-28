#ifndef DISKDATA_H
#define DISKDATA_H

#include <QString>
#include <QVector>

struct DiskItem {
    QString name;
    qint64 size = 0;
    QVector<DiskItem> children;

    bool isFolder() const { return !children.isEmpty(); }
    qint64 totalSize() const;

    static DiskItem fromJsonFile(const QString &path);
};

#endif // DISKDATA_H
