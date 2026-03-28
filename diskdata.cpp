#include "diskdata.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

static DiskItem parseItem(const QJsonObject &obj)
{
    DiskItem item;
    item.name = obj.value("name").toString();

    if (obj.contains("items")) {
        const QJsonArray arr = obj.value("items").toArray();
        for (const QJsonValue &v : arr)
            item.children.append(parseItem(v.toObject()));
    } else {
        item.size = obj.value("size").toInteger();
    }

    return item;
}

qint64 DiskItem::totalSize() const
{
    if (!isFolder())
        return size;

    qint64 sum = 0;
    for (const DiskItem &child : children)
        sum += child.totalSize();
    return sum;
}

DiskItem DiskItem::fromJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return parseItem(doc.object());
}
