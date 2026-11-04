# UKE-QT-UX-20261104: organize native Qt pages around tasks and observation context

Date: 4 November 2026.
Environment: Fedora 46 Rawhide aarch64 on POCO Pad X1 (xiaomi,uke), running
kernel 7.3.1-1500. Native compilation and RPM builds take place in COPR.

## Source finding and design

Qt release 1.0.0-8 exposed seven broad areas and a flat advanced catalog. Sound,
cameras and sensors shared one page, while inspection, options and preview
results used the same generic sequence. A result did not identify the draft
options it belonged to after edits.

The Qt-local navigation model now defines nine task areas with captions,
guidance and curated groups. Overview uses original Kirigami AbstractCard and
CardsLayout controls. Sound/cameras and sensors/input are separate, as are
diagnostics/files and guarded recovery. The complete installed SDK catalog
remains available through a separate searchable page with category/type filters.
English and Turkish metadata participate in multiword searches. No GTK or
desktop theme package is changed. The separate CLI/boot-owner changes supply
the new status and guarded slot-selection catalog actions.

Guides separate direct reads from numbered inspection, preference and review
stages. Field help and ranges appear alongside inputs. Draft reset restores SDK
defaults and invalidates review without discarding an unrelated status reading.
Observations retain their read origin and validated options, appear beside the
relevant stage, and become visibly out of date after relevant form edits.
Refresh validates the matching current draft before reading again; its existing
read-only/captured-operation guard remains enforced. Raw keys and output remain
available, while ordinary results gain readable field labels without inventing
units or measurements.

The native page stack retains catalog search/filter state across guide visits.
Programmatic transitions protect a newly prepared guide from intermediate page
notifications; ordinary Back, area changes and close still cancel owned reads.
Review preserves frozen typed requests, separate consent, exact confirmation
tokens, fixture/read-only refusal and the original terminal authentication path.

The original distribution Kirigami/FormCard controls provide cards, spacing,
palette, drawer, selection dialogs and review surfaces. No KDE package is forked,
and no stylesheet imitates its controls. Design references are the official
[CardsLayout documentation](https://api.kde.org/qml-org-kde-kirigami-cardslayout.html)
and [FormCard module](https://api.kde.org/org-kde-kirigamiaddons-formcard-qmlmodule.html).

## Validation boundary

Source tests cover curated action IDs, complete/nonduplicated catalog coverage,
category/type/multiword filters, retained search state, draft reset, stale
observations and refresh validation, alongside the previous all-guide and theme
checks. Build, DNF installation, screenshots and tablet results will be recorded
after they are observed. Fixture navigation is not physical provider acceptance.

COPR build [11256192](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11256192/)
produced Qt 1.1.0-1 and passed all four native CTest suites. DNF installed the
application and optional test RPM on the tablet. Installed Wayland checks passed
15 widget, 11 appearance and 10 Kirigami cases, including four opt-in live
read-only provider checks. Fixture screenshots covered all nine areas, the
drawer, wide overview, catalog filtering, update/channel guides and review.
Inspection found an overlong overview action label; the next release uses the
short category caption and allows native FormCard labels to wrap.

Release 1.1.0-2 adds a schema-checked kernel/slot report with separate current,
next-selection, A/B-image and approval sections. Historical failure uncertainty
is preserved. Guarded selection and preview share the Software updates area;
the frontend keeps its existing terminal review path. New fixture checks cover
the report and selection guide, and the opt-in installed suite additionally
requires complete real A/B status. Validation of that release remains pending.
