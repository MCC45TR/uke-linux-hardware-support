#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR source worker only. Retain Fedora's existing policy package family.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
here=$(cd -- "$(dirname -- "$0")" && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
    curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
        --data-urlencode "projectname=$project" --data-urlencode packagename=selinux-policy \
        --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
    jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
        ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="45.23-1.3")' "$stage/builds.json" >/dev/null
done
base=https://kojipkgs.fedoraproject.org/packages/selinux-policy/45.23/1.fc46
curl -fLsS --proto '=https' --proto-redir '=https' --retry 2 --max-time 180 \
    "$base/src/selinux-policy-45.23-1.fc46.src.rpm" -o "$stage/parent.src.rpm"
printf '%s  %s\n' e550a17157200bbd9550a7f5f26f0a0e9056151ec918b6c26cc8dfeefcb65c61 \
    "$stage/parent.src.rpm" | sha256sum -c -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == selinux-policy-45.23-1.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}'
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
cp "$output/selinux-policy.spec" "$stage/parent.spec"
test "$(grep -Fxc '%global commit 8a22f4474f77c7cb32cd6d18e3da75eaef8696de' "$stage/parent.spec")" = 1
! grep -Eq '^%check([[:space:]]|$)' "$stage/parent.spec"
! grep -Eq '^(Patch|Source)1000[123]:' "$stage/parent.spec"

# Koji's original build output is unsigned. The public baseline payload and
# policy digests below are bound to Fedora's signed, installed RPM header.
# Never substitute the live policy store or upload locally added modules.
curl -fLsS --proto '=https' --proto-redir '=https' --retry 2 --max-time 180 \
    "$base/noarch/selinux-policy-targeted-45.23-1.fc46.noarch.rpm" -o "$stage/baseline.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/baseline.rpm") == selinux-policy-targeted-45.23-1.fc46 ]]
[[ $(rpm -qp --qf '%{PAYLOADSHA256}' "$stage/baseline.rpm") == cb24cb042a652c20afd334e80e660d3a916b41e74caf60921403fdbf7e27370b ]]
rpm2cpio "$stage/baseline.rpm" | cpio -i --to-stdout --quiet \
    './etc/selinux/targeted/policy/policy.35' > "$output/fedora-targeted-baseline.35"
printf '%s  %s\n' 63ce943b9b060e81b714501cce086bc509f0b9f6457e2fb468824aa35482538c \
    "$output/fedora-targeted-baseline.35" | sha256sum -c -
cp "$here/selinux-kernel-compat.patch" "$here/check-selinux-kernel-compat.sh" "$output/"
awk '
    /^Release: 1%\{\?dist\}$/ {print "Release: 1.3%{?dist}"; releases++; next}
    /^Source40:/ {
        print
        print "Patch10001: selinux-kernel-compat.patch"
        print "Source10001: check-selinux-kernel-compat.sh"
        print "Source10002: fedora-targeted-baseline.35"
        print "Source10003: selinux-compat-receipt.json"
        print "BuildRequires: setools-console patch coreutils bash"
        sources++; next
    }
    /^%install$/ {install=1}
    install && !checks && /^%post([[:space:]]|$)/ {
        print "%check"
        print "bash %{SOURCE10001} %{buildroot} %{SOURCE10002}"
        print ""
        checks++
    }
    {print}
    END {if(releases!=1 || sources!=1 || checks!=1) exit 1}
' "$stage/parent.spec" > "$output/selinux-policy.spec"
# Reverse only the declared metadata and new check hook, then compare the
# complete original recipe, including early sandbox/devel lifecycle sections.
awk '
    /^Release: 1[.]3%\{\?dist\}$/ {print "Release: 1%{?dist}"; releases++; next}
    /^(Patch10001|Source1000[123]):/ {metadata++; next}
    /^BuildRequires: setools-console patch coreutils bash$/ {metadata++; next}
    /^%check$/ {
        if((getline)<=0 || $0!="bash %{SOURCE10001} %{buildroot} %{SOURCE10002}") exit 1
        if((getline)<=0 || $0!="") exit 1
        checks++; next
    }
    {print}
    END {if(releases!=1 || metadata!=5 || checks!=1) exit 1}
' "$output/selinux-policy.spec" > "$stage/reconstructed.spec"
cmp "$stage/parent.spec" "$stage/reconstructed.spec"
commit=$(git -C "$here/../.." rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,
    original_fedora_srpm:"selinux-policy-45.23-1.fc46",
    parent_srpm_sha256:"e550a17157200bbd9550a7f5f26f0a0e9056151ec918b6c26cc8dfeefcb65c61",
    fedora_policy_commit:"8a22f4474f77c7cb32cd6d18e3da75eaef8696de",
    baseline_policy_sha256:"63ce943b9b060e81b714501cce086bc509f0b9f6457e2fb468824aa35482538c",
    original_sources_and_lifecycle_retained:true,policy_capabilities_unchanged:true,
    original_unknown_permission_setting_unchanged:true,physical_acceptance:false}' \
    > "$output/selinux-compat-receipt.json"
[[ $(rpmspec --target aarch64 --define "_sourcedir $output" -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/selinux-policy.spec") == 45.23-1.3.fc46 ]]
