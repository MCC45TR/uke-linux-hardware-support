%global debug_package %{nil}
%global __os_install_post %{nil}
%global _build_id_links none

Name: uke-orangefox-recovery
Version: 12.0~alpha1.20260930
Release: 1.1%{?dist}
Summary: Verified OrangeFox Uke recovery images for explicit device evaluation
License: GPL-2.0-only AND GPL-3.0-or-later AND Apache-2.0 AND MIT AND BSD-2-Clause
URL: https://github.com/MCC45TR/uke-linux-hardware-support/tree/main/uke-orangefox-packaging
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: aarch64
BuildRequires: bash tar gzip coreutils jq
Provides: uke-recovery-image
# No installer, boot hook or firmware/runtime dependency is attached.

%description
DNF-managed, checksum-verified image delivery from the pinned OrangeFox Uke
GitHub release, retaining its declared release channel. Installs images and manifests as data without flashing,
unlocking, running an installer or changing boot selection. Published firmware
scope is Global OS3.0.303.0; physical boot and rollback are not accepted.
The source snapshots and exact upstream revisions are listed in SOURCE-OFFER.

%prep
%setup -q
sha256sum -c PACKAGE-SHA256SUMS
jq -e '.device == "uke" and .validation.no_python_payload == true and .validation.payload_privacy == true and .validation.physical_device == false' ARTIFACT-MANIFEST.json

%build
# Images were built at the exact published source checkpoint; this package
# verifies and delivers that release. It does not claim a fresh recovery build.

%install
destination=%{buildroot}%{_datadir}/senemos/recovery/uke/%{version}
mkdir -p "$destination"
install -m644 OrangeFox-uke-recovery.img OrangeFox-uke-fastboot-boot.img \
    OrangeFox-uke-flashable.zip ARTIFACT-MANIFEST.json release-lock.json \
    PACKAGE-SHA256SUMS PAYLOAD-FILES.tsv ORANGEFOX-SOURCE-PINS.xml \
    STOCK-KERNEL-SOURCE.json RECOVERY-AVB.txt TEMPORARY-BOOT-AVB.txt "$destination/"
# Source/license records describe the complete artifact; no tablet script runs.
install -m644 DELIVERY.md SOURCE-OFFER.md INSTALL.md LICENSE "$destination/"
cp -r licenses "$destination/"

%check
test "$(head -c8 OrangeFox-uke-recovery.img)" = 'ANDROID!'
test "$(head -c8 OrangeFox-uke-fastboot-boot.img)" = 'ANDROID!'
sha256sum -c PACKAGE-SHA256SUMS

%files
%license LICENSE
%license licenses/*.txt
%doc DELIVERY.md SOURCE-OFFER.md
%{_datadir}/senemos/recovery/uke/%{version}/

%changelog
* Sun Oct 04 2026 Senemos Maintainers <MCC45TR@users.noreply.github.com> - 12.0~alpha1.20260930-1
- Deliver the pinned Uke alpha images and evidence without device writes.
