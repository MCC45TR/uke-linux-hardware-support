// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: view
    required property var report
    readonly property var approval: report.kernel_approval || ({})
    spacing: Kirigami.Units.smallSpacing
    function value(text) { return text === null || text === undefined || text === "" ? "Unknown" : String(text) }
    function slot(text) { return text === "a" ? "A" : text === "b" ? "B" : "Unknown" }
    function yesNo(flag) { return flag === true ? "Yes" : flag === false ? "No" : "Unknown" }
    FormCard.FormHeader { title: "Current boot & next selection" }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText {
            objectName: "bootCurrent"
            text: "Running now"
            description: "Slot " + view.slot(view.report.current_slot) + " · " + view.value(view.report.running_kernel)
            icon.name: themeIcons.names["system-run"]
        }
        SafeText {
            text: "Selected for the next boot"
            description: "Slot " + view.slot(view.report.next_boot_slot)
            icon.name: themeIcons.names["system-reboot"]
        }
        SafeText {
            objectName: "bootPending"
            text: "Pending slot change"
            description: view.yesNo(view.report.pending_slot_switch)
        }
    }
    FormCard.FormHeader { title: "Kernels stored in A & B" }
    Kirigami.CardsLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 58
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        maximumColumns: 2
        minimumColumnWidth: Kirigami.Units.gridUnit * 18
        maximumColumnWidth: Kirigami.Units.gridUnit * 28
        Repeater {
            model: view.report.slots || []
            delegate: Kirigami.AbstractCard {
                required property var modelData
                Layout.fillWidth: true
                Layout.fillHeight: true
                showClickFeedback: false
                focusPolicy: Qt.NoFocus
                Accessible.role: Accessible.Grouping
                Accessible.name: "Slot " + view.slot(modelData.slot)
                contentItem: ColumnLayout {
                    spacing: 0
                    Kirigami.Heading { level: 4; text: "Slot " + view.slot(modelData.slot); Layout.leftMargin: Kirigami.Units.largeSpacing }
                    SafeText { text: "Stored kernel"; description: view.value(modelData.kernel_release); icon.name: themeIcons.names["cpu"] }
                    SafeText { text: "Bootable"; description: view.yesNo(modelData.bootable) }
                    SafeText { text: "Bootloader successful flag"; description: view.yesNo(modelData.boot_successful) }
                    SafeText { text: "Matches the running kernel Image"; description: view.yesNo(modelData.matches_running_kernel) }
                    SafeText {
                        visible: Boolean(modelData.kernel_release && view.approval.last_failed_kernel === modelData.kernel_release)
                        text: "Recorded previous failure"
                        description: "This release is listed as the failed candidate."
                        icon.name: themeIcons.names["dialog-warning"]
                    }
                }
            }
        }
    }
    FormCard.FormHeader { title: "Kernel approval" }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText {
            objectName: "bootApproval"
            text: "Running kernel approval"
            description: view.approval.running_kernel_approved === true ? "Accepted" : view.approval.running_kernel_approved === false ? "Not accepted" : "Unknown"
            icon.name: themeIcons.names[view.approval.running_kernel_approved === true ? "dialog-ok" : "dialog-information"]
        }
        SafeText {
            text: "Recorded accepted kernel"
            description: view.value(view.approval.accepted_kernel) + " · Slot " + view.slot(view.approval.accepted_slot)
        }
        SafeText { text: "Accepted record matches the stored image"; description: view.yesNo(view.approval.accepted_kernel_verified) }
        SafeText { text: "Update phase"; description: view.value(view.approval.phase).replace(/-/g, " ") }
        SafeText {
            text: "Pending candidate kernel"
            description: view.approval.candidate_kernel === null ? "No candidate recorded" : view.value(view.approval.candidate_kernel)
        }
        SafeText { text: "Latest recorded attempt approved"; description: view.yesNo(view.approval.latest_update_approved) }
        SafeText {
            visible: Boolean(view.approval.last_failed_kernel)
            text: "Previous failed candidate (history)"
            description: view.value(view.approval.last_failed_kernel)
            icon.name: themeIcons.names["dialog-warning"]
        }
        FormCard.FormSectionText {
            visible: Boolean(view.approval.history_note)
            text: view.approval.history_note || ""
            textFormat: Text.PlainText
        }
    }
    Repeater {
        model: view.report.issues || []
        delegate: FormCard.FormCard {
            required property var modelData
            Layout.fillWidth: true
            SafeText {
                text: "Some information could not be verified"
                description: modelData
                icon.name: themeIcons.names["dialog-warning"]
            }
        }
    }
}
