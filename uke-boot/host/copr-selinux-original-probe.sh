#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 1 ]]
result=$(realpath -m "$1")
src=$(cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$result" "$work"/{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS}
fetch() {
    curl -qfsSL --retry 2 --max-time 120 "$2" -o "$work/SOURCES/$1"
    printf '%s  %s\n' "$3" "$work/SOURCES/$1" | sha256sum -c -
}
fetch fork.src.rpm https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11246701/selinux-policy-45.23-1.3.src.rpm 9bdc9e852519f7a681a21389cc8b93aecb1d423a6a18a1369d23a6c9376388e9
fetch fork.rpm https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/11246701-selinux-policy/selinux-policy-targeted-45.23-1.3.fc46.noarch.rpm 0c206b32e348b6c326ce35d3978b9fd79f724f3cb41db0b22ee9cb990c050650
fetch fedora.rpm https://dl.fedoraproject.org/pub/fedora/linux/development/rawhide/Everything/aarch64/os/Packages/s/selinux-policy-targeted-45.24-1.fc46.noarch.rpm 0aea7d66b16fbe9393f55353f169f08b45ac360b62c7896a08fe8be973f297c1
cp "$src/selinux-original-probe.sh" "$src/selinux-original-compose.awk" "$src/selinux-policy-canonical.cpp" "$src/test-selinux-canonical.sh" "$src/selinux-store-probe.sh" "$src/selinux-current-ssc.cil" "$work/SOURCES/"
curl -qfsSL --retry 2 --max-time 120 https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101/uke-control-2.17.0-14.fc46.aarch64.rpm -o "$work/control14.rpm"
printf '%s  %s\n' ea9ad71e6fb75bdf46fc8b09a31473388c3c592ece0594a3495b2772ed46ad00 "$work/control14.rpm" | sha256sum -c -
rpm2cpio "$work/control14.rpm" | cpio --quiet -i --to-stdout ./usr/share/selinux/packages/uke_control_rgb.pp > "$work/SOURCES/uke_control_rgb.pp"
printf '%s  %s\n' cabcbfc751800d44e0fa38f9144659f3ebb4f6c1234f05f599894b3775ef4c93 "$work/SOURCES/uke_control_rgb.pp" | sha256sum -c -
printf '%s  %s\n' e0eba7d41c59eeb0fbb37ecaba10b6a6461c7516add70386c5fdb487fc9be99f "$work/SOURCES/selinux-current-ssc.cil" | sha256sum -c -
cp "$src/../../uke-control/packaging/selinux/uke_control_rgb.te" "$src/../../uke-control/packaging/selinux/uke_control_rgb.fc" "$work/SOURCES/"
cp "$src/../../LICENSE" "$work/SOURCES/CONTROL-MIT-LICENSE"
cat > "$work/SPECS/probe.spec" <<'SPEC'
Name: uke-selinux-original-check
Version: 1.0
Release: 5%{?dist}
Summary: Offline original Fedora SELinux composition qualification
License: GPL-2.0-or-later AND MIT
BuildArch: noarch
Source0: selinux-original-probe.sh
Source1: selinux-original-compose.awk
Source2: fork.src.rpm
Source3: fork.rpm
Source4: fedora.rpm
Source5: selinux-policy-canonical.cpp
Source6: test-selinux-canonical.sh
Source7: selinux-store-probe.sh
Source8: uke_control_rgb.pp
Source9: selinux-current-ssc.cil
Source10: uke_control_rgb.te
Source11: uke_control_rgb.fc
Source12: CONTROL-MIT-LICENSE
BuildRequires: bash coreutils cpio rpm gawk gcc-c++ checkpolicy secilc setools-console policycoreutils bzip2

%description
Text-only qualification evidence. This package installs no policy and is not
a runtime dependency. Original Fedora return, DNF and device gates are separate.

%prep
%build
g++ -std=c++20 -O2 -Wall -Wextra -Werror -pedantic %{SOURCE5} -o policy-canonical
%install
mkdir -p %{buildroot}%{_datadir}/%{name}

%check
bash %{SOURCE6} "$PWD/policy-canonical"
bash %{SOURCE0} %{SOURCE2} %{SOURCE3} %{SOURCE4} %{SOURCE1} "$PWD/policy-canonical" %{buildroot}%{_datadir}/%{name}
UKE_POLICY_OFFLINE_FIXTURE=1 bash %{SOURCE7} %{SOURCE4} %{SOURCE3} %{SOURCE1} "$PWD/policy-canonical" %{SOURCE8} %{SOURCE9} %{buildroot}%{_datadir}/%{name}

%files
%{_datadir}/%{name}/
SPEC
cp "$work/SPECS/probe.spec" "$result/"
cp "$work"/SOURCES/* "$result/"
