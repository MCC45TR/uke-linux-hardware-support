#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR custom source worker only; this script is never installed on the tablet.
set -Eeuo pipefail
owner=$(cd -- "$(dirname -- "$0")/../.."; pwd)
output=$(realpath -m "${1:?COPR result directory}")
profile="$owner/packaging/ddcutil/source.json"
version=$(jq -er .version "$profile")
release=$(jq -er .release "$profile")
nvr=$(jq -er .source_nvr "$profile")
[[ $version == 3.0.2 && $release == 2.uke1 && $nvr == ddcutil-3.0.2-1.fc46 ]]
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
cache="$owner/referances/ddcutil/$nvr.src.rpm"
mkdir -p "$(dirname "$cache")" "$output"
if [[ ! -s $cache ]]; then
    url=$(jq -er .source_url "$profile")
    [[ $url == https://kojipkgs.fedoraproject.org/packages/ddcutil/3.0.2/1.fc46/src/ddcutil-3.0.2-1.fc46.src.rpm ]]
    curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' --max-time 120 "$url" -o "$cache.part"
    mv "$cache.part" "$cache"
fi
printf '%s  %s\n' "$(jq -er .sha256 "$profile")" "$cache" | sha256sum -c -
stage=$(mktemp -d)
trap 'rm -r --one-file-system -- "$stage"' EXIT
rpm2cpio "$cache" >"$stage/source.cpio"
printf '%s  %s\n' "$(jq -er .source_payload_sha256 "$profile")" "$stage/source.cpio" | sha256sum -c -
cpio -t --quiet <"$stage/source.cpio" >"$stage/files"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/files"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames <"$stage/source.cpio")
[[ $(awk '/^Name:/ {print $2;exit}' "$output/ddcutil.spec") == ddcutil ]]
[[ $(awk '/^Version:/ {print $2;exit}' "$output/ddcutil.spec") == "$version" ]]
[[ $(grep -c '^%setup -q$' "$output/ddcutil.spec") == 1 ]]
! grep -q '^%check' "$output/ddcutil.spec"
cp "$owner/packaging/ddcutil/0001-first-line-read.patch" "$output/"
cp "$owner/tests/ddcutil-first-line.sh" "$output/"
awk -v release="$release.fc46" '
  /^Release:/ {print "Release:    " release;next}
  /^Source0:/ {print;print "Patch9990: 0001-first-line-read.patch\nSource9990: ddcutil-first-line.sh";next}
  /^BuildRequires:[[:space:]]+gcc$/ {print;print "BuildRequires: gcc-c++ libasan libubsan patch";next}
  /^%setup -q$/ {print "%autosetup -p1";next}
  !checked && /^%files/ {
    print "%check\nulimit -c 0"
    print "bash %{SOURCE9990} src/util/file_util.c"
    print "baseline=$(mktemp -d)"
    print "trap '\''rm -r --one-file-system -- \"$baseline\"'\'' EXIT"
    print "mkdir -p \"$baseline/src/util\""
    print "cp src/util/file_util.c \"$baseline/src/util/\""
    print "(cd \"$baseline\"; patch --batch --reverse -p1 < %{PATCH9990})"
    print "if bash %{SOURCE9990} \"$baseline/src/util/file_util.c\" >\"$baseline/old.log\" 2>&1; then"
    print "    echo '\''The original reader unexpectedly passed the regression fixture'\'' >&2; exit 1"
    print "else"
    print "    result=$?; test \"$result\" = 134 || { cat \"$baseline/old.log\"; exit 1; }"
    print "fi"
    print "echo '\''PASS: original reader fails; patched reader passes; no monitor access'\''\n"
    checked=1
  }
  /^%changelog/ {
    print
    print "* Thu Oct 29 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 3.0.2-2.uke1"
    print "- Preserve failed and unterminated first-line reads; add native regression checks.\n"
    next
  }
  {print}
' "$output/ddcutil.spec" >"$stage/ddcutil.spec"
mv "$stage/ddcutil.spec" "$output/ddcutil.spec"
rpmspec --target aarch64 -q --srpm --qf '%{NAME} %{VERSION}-%{RELEASE}\n' "$output/ddcutil.spec"
