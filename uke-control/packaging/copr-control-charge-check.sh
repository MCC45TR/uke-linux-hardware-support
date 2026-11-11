#!/usr/bin/bash
# COPR-only text receipt; no new runtime package or charging operation.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
output=$(realpath -m "$1")
contract=$owner/packaging/charge-bridge-source.json
[[ ! -e $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$work/charge-bridge/uke-control" "$work/charge-bridge/uke-platform-runtime" "$output"
for component in uke-control uke-platform-runtime;do
 for path in Makefile LICENSE src tests shared packaging manifests;do [[ ! -e $workspace/$component/$path ]] || cp -a "$workspace/$component/$path" "$work/charge-bridge/$component/";done
done
commit=$(git -C "$workspace" rev-parse HEAD)
tree=$(git -C "$workspace" rev-parse HEAD^{tree})
jq --arg commit "$commit" --arg tree "$tree" '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/charge-bridge/source-qualification.json"
(cd "$work/charge-bridge";find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
tar --sort=name --mtime=@1794376800 --owner=0 --group=0 --numeric-owner -cJf "$output/charge-bridge.tar.xz" -C "$work" charge-bridge
release=$(jq -er .source_check_release "$contract")
{
 cat <<'SPEC'
%global debug_package %{nil}
Name: uke-kernel-migration-check
Version: 7.3.3
SPEC
 printf 'Release: %s%%{?dist}\n' "$release"
 cat <<'SPEC'
Summary: Native Control fixed charge bridge qualification
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: charge-bridge.tar.xz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ clang llvm make compiler-rt libasan libubsan
BuildRequires: pkgconfig(libsystemd) >= 257
BuildRequires: pkgconfig(libcrypto) pkgconfig(libdrm) pkgconfig(libselinux)
BuildRequires: pkgconfig(wayland-client) wayland-protocols-devel wayland-devel
BuildRequires: pkgconfig(jsoncpp) pkgconfig(rpm) libdnf5-devel xfsprogs-devel
BuildRequires: pkgconfig(blkid) pkgconfig(fdisk) jq

%description
Native source receipt for the fixed online cap bridge and no-retry errno
reporting. No runtime, device operation or electrical qualification.

%prep
%setup -q -n charge-bridge
sha256sum -c --strict --quiet SOURCE-SHA256SUMS

%build
bash uke-control/packaging/check-charge-bridge-native.sh "$PWD"

%install
install -Dm644 source-qualification.json %{buildroot}%{_docdir}/%{name}/control-charge-source-qualification.json
install -Dm644 SOURCE-SHA256SUMS %{buildroot}%{_docdir}/%{name}/source-assets.sha256
install -Dm644 uke-control/LICENSE %{buildroot}%{_licensedir}/%{name}/LICENSE

%files
%{_docdir}/%{name}
%{_licensedir}/%{name}

%changelog
SPEC
 printf '* Wed Nov 11 2026 Uke contributors - 7.3.3-%s\n- Qualify the existing Control charge bridge and preserved errno without device access.\n' "$release"
} > "$output/uke-kernel-migration-check.spec"
printf 'Control charge source prepared as a text-only existing receipt family; no runtime or service added.\n'
