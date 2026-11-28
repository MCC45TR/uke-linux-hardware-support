// SPDX-License-Identifier: MIT
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace uq {
inline QString wetTouchFilterPath()
{
    const QDir driver(QStringLiteral("/sys/bus/spi/drivers/nt36532-uke-touch"));
    QString selected;
    for (const auto &entry : driver.entryInfoList({QStringLiteral("spi*")}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        const auto device = entry.canonicalFilePath();
        if (!device.startsWith(QStringLiteral("/sys/devices/")) ||
            QFileInfo(device + QStringLiteral("/driver")).canonicalFilePath() != driver.path())
            continue;
        const auto attribute = device + QStringLiteral("/wet_touch_filter");
        if (!QFileInfo(attribute).isFile())
            continue;
        if (!selected.isEmpty())
            return {};
        selected = attribute;
    }
    return selected;
}

inline QByteArray wetTouchFilterValue(const QString &path)
{
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const auto value = file.read(8).trimmed();
    return value == "0" || value == "1" ? value : QByteArray();
}
}
