// SPDX-License-Identifier: GPL-2.0-or-later
// Native equivalents of Dolphin 26.08.1 configuration migrations.
// Algorithm references: Jin Liu (2025), Felix Ernst (2026).
// Copyright (C) 2026 Senemos Maintainers
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <cstdio>

static bool readRegular(const QString &path, QByteArray &bytes)
{
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink() || info.size() > 8 * 1024 * 1024) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    bytes = file.readAll();
    return file.error() == QFileDevice::NoError;
}

int main(int argc, char **argv)
{
    const bool tab = QFileInfo(QString::fromLocal8Bit(argv[0])).fileName().contains("tab_key");
    QString xml = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                + QStringLiteral("/kxmlgui5/dolphin/dolphinui.rc");
    QString settings = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                     + QStringLiteral("/dolphinrc");
    if (argc == 5 && QByteArray(argv[1]) == "--file" && QByteArray(argv[3]) == "--settings") {
        xml = QString::fromLocal8Bit(argv[2]);
        settings = QString::fromLocal8Bit(argv[4]);
    } else if (argc != 1) {
        std::fputs("Usage: dolphin-migration [--file XML --settings CONFIG]\n", stderr);
        return 2;
    }
    if (tab) {
        if (!QFileInfo::exists(settings)) return 0;
        QByteArray config;
        if (!readRegular(settings, config)) return 1;
        bool enabled = false;
        for (QByteArray line : config.split('\n')) {
            line = line.trimmed();
            if (line == "UseTabForSwitchingSplitView=true") enabled = true;
        }
        if (!enabled) return 0;
    }
    if (!QFileInfo::exists(xml)) return 0;
    QByteArray original;
    if (!readRegular(xml, original)) return 1;
    QDomDocument document;
    if (!document.setContent(original)) return 1;
    bool changed = false;
    if (tab) {
        const auto groups = document.elementsByTagName(QStringLiteral("ActionProperties"));
        for (int i = 0; i < groups.size(); ++i) {
            auto group = groups.at(i).toElement();
            auto action = group.firstChildElement(QStringLiteral("Action"));
            while (!action.isNull() && action.attribute(QStringLiteral("name")) != "focus_inactive_split_view")
                action = action.nextSiblingElement(QStringLiteral("Action"));
            if (action.isNull()) {
                action = document.createElement(QStringLiteral("Action"));
                action.setAttribute(QStringLiteral("name"), QStringLiteral("focus_inactive_split_view"));
                group.appendChild(action);
                changed = true;
            }
            if (action.attribute(QStringLiteral("shortcut")) != "Ctrl+F3; Tab") {
                action.setAttribute(QStringLiteral("shortcut"), QStringLiteral("Ctrl+F3; Tab"));
                changed = true;
            }
        }
    } else {
        const auto bars = document.elementsByTagName(QStringLiteral("ToolBar"));
        for (int i = 0; i < bars.size(); ++i) {
            const auto actions = bars.at(i).toElement().elementsByTagName(QStringLiteral("Action"));
            for (int j = 0; j < actions.size(); ++j) {
                auto action = actions.at(j).toElement();
                if (action.attribute(QStringLiteral("name")) == "view_mode") {
                    action.setAttribute(QStringLiteral("name"), QStringLiteral("view_settings"));
                    changed = true;
                }
            }
        }
    }
    if (!changed) return 0;
    QSaveFile output(xml);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly) || !output.setPermissions(QFileInfo(xml).permissions())) return 1;
    const QByteArray result = document.toByteArray(2);
    return output.write(result) == result.size() && output.commit() ? 0 : 1;
}
