#ifndef UTILS_H
#define UTILS_H

#include <QString>

inline QString formatSize(qint64 bytes)
{
    if (bytes <= 0)
        return QStringLiteral("0");
    if (bytes < 1024)
        return QStringLiteral("<1 KB");
    if (bytes < 1024LL * 1024)
        return QString("%1 KB").arg(qreal(bytes) / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024)
        return QString("%1 MB").arg(qreal(bytes) / (1024.0 * 1024.0), 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024 * 1024)
        return QString("%1 GB").arg(qreal(bytes) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024 * 1024 * 1024)
        return QString("%1 TB").arg(qreal(bytes) / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 PB").arg(qreal(bytes) / (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

#endif // UTILS_H
