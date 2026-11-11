#!/usr/bin/bash
# Existing text receipt family; actual Control217 helper, not a sparse checkout.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
output=$(realpath -m "$1")
contract=$owner/packaging/charge-wait-source.json
[[ ! -e $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --max-time 90 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/parent.rpm") == uke-control-2.17.0-1 ]]
mkdir "$work/parent" "$work/source" "$output"
rpm2cpio "$work/parent.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames)
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$work/parent/uke-control.spec" | sha256sum -c --strict --quiet -
archive=$work/parent/uke-control-charge.tar.xz
printf '%s  %s\n' "$(jq -er .parent.runtime_archive_sha256 "$contract")" "$archive" | sha256sum -c --strict --quiet -
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
tar -xJf "$archive" -C "$work/source"
(cd "$work/source";sha256sum -c --strict --quiet SHA256SUMS)
runtime=$work/source/runtime
file=src/battery/control-main.cpp
printf '%s  %s\n' "$(jq -er '.source_unit.before_sha256' "$contract")" "$runtime/$file" | sha256sum -c --strict --quiet -
cp -a "$runtime" "$work/before-runtime"
git -c init.defaultBranch=senemos -C "$runtime" init -q
git -C "$runtime" apply --check --whitespace=error "$owner/packaging/charge-wait.patch"
git -C "$runtime" apply --whitespace=error "$owner/packaging/charge-wait.patch"
printf '%s  %s\n' "$(jq -er '.source_unit.after_sha256' "$contract")" "$runtime/$file" | sha256sum -c --strict --quiet -
# Inverse every original runtime byte before adding the test fixture.
cp -a "$runtime" "$work/inverse-runtime"
git -C "$work/inverse-runtime" apply -R --whitespace=error "$owner/packaging/charge-wait.patch"
find "$work/inverse-runtime/.git" -depth -delete
diff -qr "$work/before-runtime" "$work/inverse-runtime"
install -m644 "$workspace/uke-platform-runtime/tests/charge-wait.cpp" "$runtime/tests/charge-wait.cpp"
cp -a "$runtime" "$work/charge-wait"
find "$work/charge-wait/.git" -depth -delete
install -Dm644 "$owner/packaging/check-charge-wait-native.sh" "$work/charge-wait/packaging/check-charge-wait-native.sh"
install -Dm644 "$owner/docs/CHARGE-WAIT.md" "$work/charge-wait/docs/CHARGE-WAIT.md"
commit=$(git -C "$workspace" rev-parse HEAD);tree=$(git -C "$workspace" rev-parse HEAD^{tree})
jq --arg commit "$commit" --arg tree "$tree" '.+{source_commit:$commit,source_tree:$tree}' "$contract" > "$work/charge-wait/source-qualification.json"
(cd "$work/charge-wait";find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
tar --sort=name --mtime=@1794393000 --owner=0 --group=0 --numeric-owner -cJf "$output/charge-wait.tar.xz" -C "$work" charge-wait
release=$(jq -er .source_check_release "$contract")
{
 cat <<'SPEC'
%global debug_package %{nil}
Name: uke-kernel-migration-check
Version: 7.3.3
SPEC
 printf 'Release: %s%%{?dist}\n' "$release"
 cat <<'SPEC'
Summary: Native Control interruptible charge-wait qualification
License: MIT
URL: https://github.com/MCC45TR/uke-linux-hardware-support
Source0: charge-wait.tar.xz
ExclusiveArch: aarch64
BuildRequires: gcc-c++ clang llvm make compiler-rt libasan libubsan nodejs
BuildRequires: pkgconfig(libsystemd) >= 257
BuildRequires: pkgconfig(libcrypto) jq

%description
Text-only actual helper wait/source qualification over Control217.
No runtime, hardware operation, guard retirement or electrical acceptance.

%prep
%setup -q -n charge-wait
sha256sum -c --strict --quiet SOURCE-SHA256SUMS

%build
bash packaging/check-charge-wait-native.sh "$PWD"

%install
install -Dm644 source-qualification.json %{buildroot}%{_docdir}/%{name}/control-wait-source-qualification.json
install -Dm644 SOURCE-SHA256SUMS %{buildroot}%{_docdir}/%{name}/source-assets.sha256
install -Dm644 src/battery/control-main.cpp %{buildroot}%{_docdir}/%{name}/control-main.cpp
install -Dm644 LICENSE %{buildroot}%{_licensedir}/%{name}/LICENSE

%files
%{_docdir}/%{name}
%{_licensedir}/%{name}

%changelog
SPEC
 printf '* Wed Nov 11 2026 Uke contributors - 7.3.3-%s\n- Qualify one interruptible deadline instead of short polling ticks.\n- Preserve the protection interval and actual Control217 hardware behavior.\n' "$release"
} > "$output/uke-kernel-migration-check.spec"
printf 'Control wait source prepared: exact Control217 helper,whole runtime inverse,one changed source unit and existing receipt family; no device operation.\n'
