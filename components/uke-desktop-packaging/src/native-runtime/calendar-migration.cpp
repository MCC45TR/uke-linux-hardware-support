// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
// Native implementation of the KDE calendar plugin-ID migration.
// Algorithm reference: plasma-workspace 6.7.91, Nicolas Fella (2024).
// Copyright (C) 2026 Senemos Maintainers
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <cstdio>

int main(int argc, char **argv)
{
    QString path;
    if (argc == 3 && QByteArray(argv[1]) == "--file") {
        path = QString::fromLocal8Bit(argv[2]);
    } else if (argc == 1) {
        path = QStandardPaths::locate(QStandardPaths::ConfigLocation,
                                    QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc"));
        if (path.isEmpty()) return 0;
    } else {
        std::fputs("Usage: calendar-migration [--file CONFIG]\n", stderr);
        return 2;
    }
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink() || info.size() > 8 * 1024 * 1024) return 1;
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return 1;
    const QByteArray original = input.readAll();
    if (input.error() != QFileDevice::NoError) return 1;
    input.close();
    QList<QByteArray> lines = original.split('\n');
    const QByteArray prefix("enabledCalendarPlugins=");
    for (QByteArray &line : lines) {
        if (!line.startsWith(prefix)) continue;
        const bool crlf = line.endsWith('\r');
        QByteArray value = line.mid(prefix.size());
        if (crlf) value.chop(1);
        QList<QByteArray> plugins = value.split(',');
        for (QByteArray &plugin : plugins) {
            if (plugin.startsWith('/') && plugin.endsWith(".so")) {
                const qsizetype slash = plugin.lastIndexOf('/');
                plugin = plugin.mid(slash + 1, plugin.size() - slash - 4);
            }
        }
        QByteArray updated;
        for (qsizetype i = 0; i < plugins.size(); ++i) {
            if (i) updated += ',';
            updated += plugins[i];
        }
        line = prefix + updated;
        if (crlf) line += '\r';
    }
    QByteArray result;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (i) result += '\n';
        result += lines[i];
    }
    if (result == original) return 0;
    QSaveFile output(path);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) return 1;
    if (!output.setPermissions(info.permissions())) return 1;
    if (output.write(result) != result.size() || !output.commit()) return 1;
    return 0;
}
