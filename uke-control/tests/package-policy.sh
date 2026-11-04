#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR-only signed repositories and an isolated RPM database; no chroot or RPM execution.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
fixture=$(mktemp -d)
trap 'find "$fixture" -depth -delete' EXIT
mkdir -p "$fixture"/{installed,target,foreign,conflict,bad-manager,unsigned,top/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS},root}
export GNUPGHOME="$fixture/gnupg"
mkdir -m700 "$GNUPGHOME"
gpg --batch --passphrase '' --quick-generate-key 'Package fixture <fixture@example.invalid>' rsa2048 sign 1d >/dev/null 2>&1
fingerprint=$(gpg --batch --with-colons --list-keys 2>/dev/null | awk -F: '$1=="fpr"{print $10;exit}')
gpg --batch --armor --export "$fingerprint" > "$fixture/key.asc"
build() {
    local name=$1 version=$2 destination=$3 extra=${4-}
    cat > "$fixture/top/SPECS/fixture.spec" <<EOF
Name: $name
Version: $version
Release: 1.fc46
Summary: Isolated signed package-policy fixture
License: MIT
Vendor: Fixture-$destination
BuildArch: noarch
AutoReqProv: no
$extra
%description
Solver input only; no installed programs or maintainer scripts.
%install
mkdir -p %{buildroot}/usr/share/uke-policy-fixture/$name
echo fixture > %{buildroot}/usr/share/uke-policy-fixture/$name/value
%files
/usr/share/uke-policy-fixture/$name/value
EOF
    rpmbuild -bb --define "_topdir $fixture/top" "$fixture/top/SPECS/fixture.spec" > "$fixture/build.log" 2>&1 || { cat "$fixture/build.log"; return 1; }
    local package="$fixture/top/RPMS/noarch/$name-$version-1.fc46.noarch.rpm"
    if [[ $destination != unsigned ]]; then
        rpmsign --define "_openpgp_sign_id $fingerprint" --define '_openpgp_sign gpg' --addsign "$package" > "$fixture/sign.log" 2>&1 || { cat "$fixture/sign.log"; return 1; }
    fi
    cp "$package" "$fixture/$destination/"
}
build uke-control 3 installed 'Provides: uke-control(channel-api) = 1'
build uke-shared 2 installed
build uke-extra 1 installed
build uke-app 1 installed
build senemos-uke-linux-kernel-mainline 2 installed
build senemos-uke-linux-kernel-mainline-core 2 installed $'Provides: installonlypkg(kernel)\nProvides: kernel-uname-r(2.0-fixture)'
build uke-control 2 target 'Provides: uke-control(channel-api) = 1'
build uke-shared 1 target
build uke-app 2 target
build senemos-uke-linux-kernel-mainline 1 target
build uke-extra 2 foreign
cp "$fixture/target/"*.rpm "$fixture/conflict/"
build uke-app 3 conflict 'Requires: uke-extra >= 2'
cp "$fixture/target/"*.rpm "$fixture/bad-manager/"
build uke-control 4 bad-manager
cp "$fixture/target/"*.rpm "$fixture/unsigned/"
build uke-app 2 unsigned
for repository in target foreign conflict bad-manager unsigned; do createrepo_c --quiet "$fixture/$repository"; done
dbpath="$fixture/root$(rpm --eval '%{_dbpath}')"
mkdir -p "$dbpath"
rpm --dbpath "$dbpath" --initdb
rpmkeys --dbpath "$dbpath" --import "$fixture/key.asc"
# Database-only fixture input: never install these files into the worker's root.
rpm --dbpath "$dbpath" --justdb --noscripts --notriggers -i "$fixture/installed/"*.rpm
rpm --dbpath "$dbpath" -qa | sort > "$fixture/database.before"
for scenario in channel-set update locked-kernel exact-sync; do build/test-package-policy "$fixture" "$fixture/target" "$scenario"; done
build/test-package-policy "$fixture" "$fixture/conflict" dependency-conflict
build/test-package-policy "$fixture" "$fixture/bad-manager" incompatible-manager
build/test-package-policy "$fixture" "$fixture/unsigned" unsigned
rpm --dbpath "$dbpath" -qa | sort > "$fixture/database.after"
cmp "$fixture/database.before" "$fixture/database.after"
printf '%s\n' 'Signed channel/update retention, kernel exclusions, strict synchronization, dependency and signature refusals passed; RPM database unchanged.'
