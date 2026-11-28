// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: panel
    required property var settings
    readonly property var report: settings.bootReport
    readonly property var approval: report.kernel_approval || ({})
    readonly property var choices: settings.actionChoices("boot.set-slot", "slot")
    readonly property string acceptedSlot: approval.accepted_kernel_verified === true ? approval.accepted_slot || "" : ""
    readonly property bool canSelect: !backend.readOnly && settings.topic.canApply !== false &&
                                      report.complete === true && !report.stale && report.pending_slot_switch === false
    minimumTileWidth: Kirigami.Units.gridUnit * 24
    function reviewSlot(value) {
        if (canSelect && backend.openAction("boot.set-slot")) {
            backend.setDraft("slot", value)
            backend.submit()
        }
    }

    SelectionCard {
        heading: i18n("Next boot slot")
        current: panel.report.next_boot_slot || ""
        choices: panel.choices.filter(choice => choice.value !== "next").map(choice => ({
            title: choice.title, value: choice.value,
            enabled: (panel.report.slots || []).some(slot => slot.slot === choice.value && slot.bootable === true)
        }))
        canSelect: panel.canSelect
        help: i18n("Changes take effect on the next restart.")
        reviewLabel: i18n("Review next boot slot")
        onReviewRequested: value => panel.reviewSlot(value)
    }
    SettingsSection {
        heading: i18n("Boot fallback")
        SafeText {
            text: i18n("Automatic recovery during a kernel trial")
            description: i18n("The boot service manages fallback to the accepted kernel. The current provider has no manual fallback mode switch.")
        }
        FormCard.FormButtonDelegate {
            visible: panel.acceptedSlot === "a" || panel.acceptedSlot === "b"
            enabled: panel.canSelect && panel.acceptedSlot !== panel.report.next_boot_slot
            text: i18n("Use accepted slot %1 on next boot", panel.acceptedSlot.toUpperCase())
            onClicked: panel.reviewSlot(panel.acceptedSlot)
        }
    }
}
