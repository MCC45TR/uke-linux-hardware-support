// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
FormCard.FormCardPage {
    id: page
    objectName: "waydroidPage"
    required property var appWindow
    readonly property var installer: backend.waydroid
    property string selectedEdition: "GAPPS"
    title: i18n("Android applications")
    Component.onCompleted: installer.open()
    Component.onDestruction: installer.close()
    Connections {
        target: installer
        function onChanged() {
            if (installer.state.variant === "GAPPS" || installer.state.variant === "VANILLA") page.selectedEdition = installer.state.variant
        }
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    FormCard.FormHeader { title: i18n("Waydroid") }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("Android applications on your desktop")
            description: i18n("Install Waydroid automatically, then open Android and run its applications in separate desktop windows.")
        }
        FormCard.FormComboBoxDelegate {
            id: edition
            objectName: "waydroidEdition"
            text: i18n("Android edition")
            model: [i18n("GAPPS — Google Play included"), i18n("Vanilla — without Google services")]
            enabled: !installer.busy && !installer.state.initialized && !installer.state.variant && !installer.blocked
            currentIndex: page.selectedEdition === "VANILLA" ? 1 : 0
            onActivated: index => page.selectedEdition = index === 1 ? "VANILLA" : "GAPPS"
        }
        FormCard.FormTextDelegate {
            visible: !!installer.state.initialized
            text: i18n("Installed edition")
            description: installer.state.variant === "GAPPS" ? i18n("GAPPS — Google Play included") :
                         installer.state.variant === "VANILLA" ? i18n("Vanilla — without Google services") : i18n("Existing Android installation")
        }
        FormCard.FormTextDelegate {
            text: installer.phaseText
            description: installer.busy ? i18n("You can close Uke Control. Installation will continue and its progress will be available when you return.") :
                         installer.state.ready ? i18n("Installation is complete. Open Waydroid to get started.") :
                         i18n("Required packages and official Android images are downloaded and configured for you. Existing Android data is kept.")
        }
        Controls.ProgressBar {
            objectName: "waydroidProgress"
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: installer.busy
            indeterminate: installer.authorizing || installer.state.progress === undefined || installer.state.progress < 0
            from: 0
            to: 100
            value: installer.state.progress === undefined ? 0 : installer.state.progress
        }
        FormCard.FormButtonDelegate {
            objectName: "installWaydroid"
            visible: !installer.state.ready
            text: installer.state.phase === "failed" ? i18n("Try installation again") : i18n("Install Waydroid")
            icon.name: "system-software-install"
            enabled: !installer.blocked && !installer.busy
            onClicked: installer.install(page.selectedEdition)
        }
        FormCard.FormButtonDelegate {
            objectName: "launchWaydroid"
            visible: !!installer.state.ready
            text: i18n("Open Waydroid")
            icon.name: "media-playback-start"
            enabled: !installer.blocked && !installer.busy && !!installer.state.kernel_ready
            onClicked: installer.launch()
        }
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: installer.state.kernel_ready === false
        type: Kirigami.MessageType.Warning
        text: i18n("Update the system and restart it before installing Waydroid.")
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: installer.message.length > 0
        type: Kirigami.MessageType.Warning
        text: installer.message
    }
    FormCard.FormHeader {
        visible: !!installer.state.ready && installer.state.variant === "GAPPS"
        title: i18n("Google Play")
    }
    FormCard.FormCard {
        visible: !!installer.state.ready && installer.state.variant === "GAPPS"
        FormCard.FormTextDelegate {
            text: i18n("Your Google account")
            description: i18n("Sign in inside Android. If Google asks you to register this device, use the registration identifier below. This step uses your own Google account.")
        }
        FormCard.FormButtonDelegate {
            objectName: "readWaydroidGoogleId"
            text: i18n("Read Google registration identifier")
            icon.name: "dialog-information"
            enabled: !installer.blocked && !installer.busy
            onClicked: installer.readGoogleId()
        }
        FormCard.FormTextDelegate {
            visible: installer.googleId.length > 0
            text: i18n("Android / GSF identifier")
        }
        Controls.TextField {
            objectName: "waydroidGoogleId"
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: installer.googleId.length > 0
            readOnly: true
            selectByMouse: true
            text: installer.googleId
            Accessible.name: i18n("Android / GSF identifier")
        }
        FormCard.FormButtonDelegate {
            objectName: "openWaydroidGoogleRegistration"
            text: i18n("Open Google's registration page")
            icon.name: "internet-web-browser"
            enabled: !installer.blocked && installer.googleId.length > 0
            onClicked: installer.openGoogleRegistration()
        }
    }
}
