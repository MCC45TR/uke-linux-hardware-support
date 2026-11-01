// SPDX-License-Identifier: MIT
// Exercise the failing QML creation against the staged module without playback.
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <cstdio>
#include <memory>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (argc != 2 || !QFileInfo::exists(QDir(argv[1]).filePath("MultiSoundEffect.qml"))) return 1;
    QQmlEngine engine;
    for (const bool available : {false, true}) {
        const auto type = available ? "MultiSoundEffect" : "MissingSoundAdmissionType";
        QQmlComponent component(&engine);
        component.setData(QString(R"QML(
import QtQuick
import QtQuick.VirtualKeyboard
Item {
    property QtObject probe: null
    property string creationError: ""
    Component.onCompleted: {
        try { probe = Qt.createQmlObject('import QtQuick; import QtQuick.VirtualKeyboard; %1 {}', this) }
        catch (error) { creationError = String(error) }
    }
})QML").arg(type).toUtf8(), QUrl::fromLocalFile(QDir(argv[1]).absoluteFilePath("sound-admission.qml")));
        std::unique_ptr<QObject> object(component.create());
        if (!object) { for (const auto &error : component.errors()) qWarning("%s", qPrintable(error.toString())); return 1; }
        auto *probe = object->property("probe").value<QObject *>();
        const auto error = object->property("creationError").toString();
        if (available ? (!probe || !error.isEmpty()) : (probe || !error.contains("not a type"))) {
            qWarning("Unexpected QML creation for %s: %s", type, qPrintable(error)); return 1;
        }
    }
    std::puts("Packaged MultiSoundEffect dynamic creation and missing-type control passed; offscreen, no play call or physical audio test");
}
