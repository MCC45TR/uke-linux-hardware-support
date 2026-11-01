#!/bin/bash
# Disposable server userspace qualification only. No Uke kernel/native helper,
# boot producer, filesystem image writer or device command is executed.
set -euo pipefail
export LC_ALL=C DEBIAN_FRONTEND=noninteractive
[[ $(uname -m) == aarch64 && $(dpkg --print-architecture) == arm64 ]]
source /etc/os-release
[[ $VERSION_CODENAME == tenacious && $VERSION_ID == 27.04 ]]
mkdir -p /evidence /usr/share/keyrings
# Minimal target policy: omit the exact optional interpreter diagnostics seen
# in the first qualification. Keep distro ELF libraries and boot/service files.
# The terminal audit still refuses any other Python package or script payload.
cat > /etc/dpkg/dpkg.cfg.d/99-uke-minimal-no-python <<'EOF'
path-exclude=/usr/share/apport/package-hooks/source_shadow.py
path-exclude=/usr/share/apport/package-hooks/udev.py
path-exclude=/usr/share/apport/package-hooks/systemd.py
path-exclude=/usr/share/gcc/python/*
path-exclude=/usr/share/gdb/auto-load/usr/lib/*/libstdc++.so.*-gdb.py
EOF
install -m644 /verified-key/uke-linux-unstable.gpg /usr/share/keyrings/uke-linux-unstable.gpg
printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d
chmod 755 /usr/sbin/policy-rc.d
cat > /etc/apt/sources.list.d/uke-linux-unstable.sources <<'EOF'
Types: deb
URIs: https://ppa.launchpadcontent.net/mcs45tr/uke-linux-unstable/ubuntu
Suites: tenacious
Components: main
Architectures: arm64
Signed-By: /usr/share/keyrings/uke-linux-unstable.gpg
EOF
apt-get update
apt-get install -y --no-install-recommends systemd-sysv udev dbus kmod \
 ca-certificates apt ubuntu-keyring bash coreutils util-linux e2fsprogs f2fs-tools \
 iproute2 iputils-ping policycoreutils selinux-policy-default selinux-utils \
 'uke-speaker-firmware=3.0.304.0-0ubuntu1~tenacious1+git202611011251~ubuntu27.04.1'
dpkg-query -W -f='${binary:Package}\t${Version}\t${Architecture}\n' > /evidence/packages.tsv
if cut -f1 /evidence/packages.tsv | grep -E '^(python|libpython)'; then
 echo 'Python runtime is not admitted in the target package set' >&2; exit 1
fi
if find /usr /etc -type f \( -name '*.py' -o -name '*.pyc' -o -name '*.pyo' \) -print | grep .; then
 echo 'Python script payload is not admitted in the target tree' >&2; exit 1
fi
[[ -x /usr/lib/systemd/systemd && -x /usr/sbin/load_policy ]]
policies=(/etc/selinux/default/policy/policy.*)
[[ ${#policies[@]} == 1 && -s ${policies[0]} ]]
sha256sum "${policies[0]}" > /evidence/selinux-policy.sha256
grep -E '^(SELINUX|SELINUXTYPE)=' /etc/selinux/config > /evidence/selinux-config.txt
printf '%s  %s\n' f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b /usr/lib/firmware/qcom/uke/audio/fs19xx.fsm | sha256sum -c -
apt-cache policy uke-speaker-firmware > /evidence/firmware-apt-policy.txt
cp /var/lib/apt/lists/*InRelease /evidence/
cp /etc/os-release /evidence/os-release
cp /etc/dpkg/dpkg.cfg.d/99-uke-minimal-no-python /evidence/target-file-policy.txt
printf '%s\n' 'Native ARM64 Tenacious userspace installation passed with authenticated Ubuntu and PPA indexes.' 'No Python packages or script payloads were found.' 'SELinux policy is installed; enforcing boot, labels and root handoff remain untested.' 'This test creates no tablet image, kernel, initramfs or boot bundle.' > /evidence/qualification.txt
(cd /evidence; find . -maxdepth 1 -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
