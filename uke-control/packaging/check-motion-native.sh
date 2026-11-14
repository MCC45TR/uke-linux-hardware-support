#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR source-worker qualification only. Never runs on the tablet.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 2 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $EUID != 0 && ! -e /etc/senemos/kernel-update.conf ]]
policy=$(realpath -e -- "$1")
output=$(realpath -e -- "$2")
ulimit -c 0
node tests/motion-policy-permissions.mjs --self-test
make -f packaging/motion.mk check
make -f packaging/motion.mk CXX=clang++ motion_out=build/motion-clang CXXFLAGS='-O1 -g -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all' LDFLAGS='-fsanitize=address,undefined' check
# Reconstruct exactly the pre19 broker, not a simplified replacement.
node <<'JS'
const fs=require('node:fs'),crypto=require('node:crypto'),assert=require('node:assert/strict');
let source=fs.readFileSync('src/motion-broker.cpp','utf8');
for(const line of ['    bool removed_pending=false;\n','        removed_pending=true;\n']){
 assert.equal(source.split(line).length,2);source=source.replace(line,'');
}
const start=source.indexOf('    const bool removed_lease=m_leases.erase(sender)>0;');
const end=source.indexOf('\n}\nvoid Broker::unused()',start);assert(start>0&&end>start);
source=source.slice(0,start)+'    const bool last=m_leases.erase(sender)&&m_leases.empty();\n    if(last)m_source->stop();\n    unused();'+source.slice(end);
assert.equal(crypto.createHash('sha256').update(source).digest('hex'),'0229659526ccfff336c6b5568989ccd8f1128e9dabd2560ea812892c78c9bf0a');
fs.writeFileSync('build/motion/before19-broker.cpp',source);
JS
g++ -std=c++20 -O2 -g -UNDEBUG -Wall -Wextra -Werror -Isrc $(pkg-config --cflags libsystemd jsoncpp) tests/motion-bus.cpp build/motion/before19-broker.cpp $(pkg-config --libs libsystemd jsoncpp) -o build/motion/before19-bus
if timeout 30s dbus-run-session -- build/motion/before19-bus > build/motion/before19-refusal.txt 2>&1;then
 echo 'Pre19 unrelated-peer loss was accepted' >&2;exit 1
fi
grep -Fq 'Unrelated peer loss must preserve activation grace' build/motion/before19-refusal.txt
printf 'Exact pre19 broker negative: unrelated peer loss ends activation grace (refused).\n'
UKE_POLICY_OFFLINE_FIXTURE=1 bash tests/motion-policy.sh "$policy" build/motion/selinux/uke_control_motion.cil packaging/selinux/motion-policy.sh "$output/policy-proof.txt"
cp LICENSE "$output/LICENSE"
printf '%s\n' 'Both six-group GCC/Clang sanitizer suites pass, including activation grace and owned release. Exact pre19 broker is refused. Linked original-policy permissions and missing-init semantic negative pass; disabled preferences and ownership checks retained. No actual systemd, ARM service or sensor admission claimed.' > "$output/native-proof.txt"
