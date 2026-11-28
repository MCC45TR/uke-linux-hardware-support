// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: view
    required property var report
    readonly property var approval: report.kernel_approval || ({})
    spacing: Kirigami.Units.mediumSpacing

    function value(text) {
        return text === null || text === undefined || text === "" ? i18n("Unknown") : String(text)
    }
    function slot(name) {
        return name === "a" || name === "b" ? i18n("Slot %1", name.toUpperCase()) : i18n("Slot unknown")
    }
    function yesNo(flag) {
        return flag === true ? i18n("Yes") : flag === false ? i18n("No") : i18n("Unknown")
    }
    function fact(title, value, primary = false, detail = false) {
        return {title, value, primary, detail}
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: view.report.stale || false
        type: Kirigami.MessageType.Warning
        text: view.report.stamp ? i18n("Showing the last available reading from %1.", view.report.stamp)
                                : i18n("Showing the last available reading.")
    }

    InformationGrid {
        sections: [
            {
                key: "runningBoot",
                title: i18n("Running now"),
                rows: [
                    view.fact(view.value(view.report.running_kernel), view.slot(view.report.current_slot), true),
                    view.fact(i18n("Running kernel approval"), view.approval.running_kernel_approved === true
                        ? i18n("Accepted") : view.approval.running_kernel_approved === false ? i18n("Not accepted") : i18n("Unknown"))
                ]
            },
            {
                key: "nextBoot",
                title: i18n("Next boot"),
                rows: [
                    view.fact(view.report.pending_slot_switch === true ? i18n("A slot change is pending")
                        : view.report.pending_slot_switch === false ? i18n("No slot change is pending") : i18n("Pending change unknown"),
                              view.slot(view.report.next_boot_slot), true),
                    view.fact(i18n("Pending candidate kernel"), view.approval.candidate_kernel === null
                        ? i18n("No candidate recorded") : view.value(view.approval.candidate_kernel))
                ]
            }
        ]
    }

    Kirigami.Heading {
        visible: (view.report.slots || []).length > 0
        Layout.fillWidth: true
        Layout.maximumWidth: slotGrid.width
        Layout.alignment: Qt.AlignHCenter
        level: 3
        text: i18n("Stored kernels")
    }
    InformationGrid {
        id: slotGrid
        sections: (view.report.slots || []).map(entry => {
            let rows = [
                view.fact(i18n("Stored kernel"), view.value(entry.kernel_release)),
                view.fact(i18n("Bootable"), view.yesNo(entry.bootable)),
                view.fact(i18n("Bootloader successful flag"), view.yesNo(entry.boot_successful), false, true),
                view.fact(i18n("Matches the running kernel Image"), view.yesNo(entry.matches_running_kernel), false, true)
            ]
            if (entry.kernel_release && view.approval.last_failed_kernel === entry.kernel_release)
                rows.splice(1, 0, view.fact(i18n("Recorded previous failure"), i18n("This release is listed as the failed candidate.")))
            return {key: "slot-" + entry.slot, title: view.slot(entry.slot), rows}
        })
    }

    InformationGrid {
        Layout.maximumWidth: Kirigami.Units.gridUnit * 48
        sections: [{
            key: "kernelApproval",
            title: i18n("Kernel approval record"),
            note: view.approval.history_note || "",
            rows: [
                view.fact(i18n("Recorded accepted kernel"), view.value(view.approval.accepted_kernel) + " · " + view.slot(view.approval.accepted_slot)),
                view.fact(i18n("Accepted record matches the stored image"), view.yesNo(view.approval.accepted_kernel_verified)),
                view.fact(i18n("Update phase"), view.value(view.approval.phase).replace(/-/g, " ")),
                view.fact(i18n("Latest recorded attempt approved"), view.yesNo(view.approval.latest_update_approved))
            ].concat(view.approval.last_failed_kernel
                ? [view.fact(i18n("Previous failed candidate (history)"), view.value(view.approval.last_failed_kernel))] : [])
        }].concat((view.report.issues || []).length ? [{
            key: "bootIssues",
            title: i18n("Some information could not be verified"),
            failed: true,
            message: view.report.issues.map(issue => String(issue)).join("\n")
        }] : [])
    }
}
