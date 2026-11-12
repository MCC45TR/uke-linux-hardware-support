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
cp "$src/selinux-original-probe.sh" "$src/selinux-original-compose.awk" "$work/SOURCES/"
cat > "$work/SPECS/probe.spec" <<'SPEC'
Name: uke-selinux-original-check
Version: 1.0
Release: 2%{?dist}
Summary: Offline original Fedora SELinux composition qualification
License: GPL-2.0-or-later
BuildArch: noarch
Source0: selinux-original-probe.sh
Source1: selinux-original-compose.awk
Source2: fork.src.rpm
Source3: fork.rpm
Source4: fedora.rpm
BuildRequires: bash coreutils cpio rpm gawk checkpolicy secilc setools-console

%description
Text-only qualification evidence. This package installs no policy and is not
a runtime dependency. Original Fedora return, DNF and device gates are separate.

%prep
%build
%install
mkdir -p %{buildroot}%{_datadir}/%{name}

%check
bash %{SOURCE0} %{SOURCE2} %{SOURCE3} %{SOURCE4} %{SOURCE1} %{buildroot}%{_datadir}/%{name}

%files
%{_datadir}/%{name}/
SPEC
cp "$work/SPECS/probe.spec" "$result/"
cp "$work"/SOURCES/* "$result/"
