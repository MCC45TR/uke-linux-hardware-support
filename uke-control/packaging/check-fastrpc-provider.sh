#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR %check only; no target-device compilation or real DSP/calibration.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 3 ]]
overlay=$(realpath "$1")
parent=$(realpath "$2")
buildroot=$(realpath "$3")
work=$overlay/native-check
[[ ! -e $work ]]
mkdir -p "$work"/{src,tests,units,fixtures}
install -m644 "$overlay/src/ssc-cache.cpp" "$work/src/ssc-cache.cpp"
install -m644 "$parent/inputs/ssc-cache-test.cpp" "$work/tests/cache-parent.cpp"
install -m644 "$overlay/tests/ssc-cache-migration.cpp" "$work/tests/cache-migration.cpp"
g++ -std=c++20 -O2 -Wall -Wextra -Werror -UNDEBUG "$work/tests/cache-parent.cpp" -o "$work/cache-parent"
g++ -std=c++20 -O2 -Wall -Wextra -Werror -UNDEBUG "$work/tests/cache-migration.cpp" -o "$work/cache-migration"
"$work/cache-parent" "$work/fixtures/parent"
"$work/cache-migration" "$work/fixtures/migration"
cp "$overlay/src/fastrpc/"* "$work/units/"
# Only the uninstalled cache binary's executable path is relocated for offline
# parsing. The installed path and all dependency/namespace settings are sealed.
node --input-type=module - "$work/units/uke-ssc-cache.service" "$overlay/uke-ssc-cache" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';
const [path,binary]=process.argv.slice(2);const data=fs.readFileSync(path,'utf8');
assert.equal(data.split('ExecStart=/usr/libexec/uke-ssc-cache\n').length,2);
fs.writeFileSync(path,data.replace('ExecStart=/usr/libexec/uke-ssc-cache\n',()=>`ExecStart=${binary}\n`));
JS
# v262 creates its fixed system runtime prefix before --root is applied.
# Redirect only that public path API in this build-only process. Keep system
# scope, original unit bytes/settings and the original systemd binary.
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared \
 "$overlay/tests/systemd-verify-runtime.cpp" $(pkg-config --cflags libsystemd) -ldl -o "$work/libverify-runtime.so"
mkdir -m700 "$work/verify-runtime"
g++ -std=c++17 -O2 -Wall -Wextra -Werror "$overlay/tests/systemd-verify-runtime-probe.cpp" \
 $(pkg-config --cflags --libs libsystemd) -o "$work/runtime-probe"
readelf --wide --dyn-syms /usr/bin/systemd-analyze | grep -E 'UND sd_path_lookup@' > "$work/analyzer-lookup-import.txt"
[[ -s $work/analyzer-lookup-import.txt ]]
original_state_path=$("$work/runtime-probe" state)
[[ $(UKE_VERIFY_RUNTIME="$work/verify-runtime" LD_PRELOAD="$work/libverify-runtime.so" "$work/runtime-probe" runtime) == "$work/verify-runtime" ]]
[[ $(UKE_VERIFY_RUNTIME="$work/verify-runtime" LD_PRELOAD="$work/libverify-runtime.so" "$work/runtime-probe" state) == "$original_state_path" ]]
if env --unset=UKE_VERIFY_RUNTIME LD_PRELOAD="$work/libverify-runtime.so" "$work/runtime-probe" runtime > "$work/missing-runtime.log" 2>&1;then exit 1;fi
UKE_VERIFY_RUNTIME="$work/verify-runtime" LD_PRELOAD="$work/libverify-runtime.so" \
 systemd-analyze --generators=no --man=no verify "$work/units/"*
[[ ! -e $buildroot/usr/libexec/uke-hexagonrpcd ]]
[[ -x $buildroot/usr/libexec/uke-ssc-cache ]]
[[ $(find "$buildroot/usr/lib/systemd/system" -maxdepth 1 -name 'run-uke*.mount' -type f | wc -l) == 1 ]]
node --input-type=module - "$buildroot/usr/share/uke-control/fastrpc-root" <<'JS'
import fs from 'node:fs';import assert from 'node:assert/strict';const root=process.argv[2];let files=0,links=0;
const expected=new Map([['bin','usr/bin'],['lib','usr/lib'],['lib64','usr/lib64'],['mnt/vendor/persist','/persist'],['system/vendor','/vendor']]);
function walk(path,relative=''){for(const n of fs.readdirSync(path)){const p=path+'/'+n,r=relative?relative+'/'+n:n,s=fs.lstatSync(p);if(s.isSymbolicLink()){assert.equal(fs.readlinkSync(p),expected.get(r));expected.delete(r);links++;}else if(s.isDirectory()){assert.equal(s.mode&0o777,0o700);walk(p,r);}else{assert(s.isFile());assert.equal(s.size,0);assert.equal(s.mode&0o777,0o600);files++;}}}walk(root);assert.equal(files,8);assert.equal(links,5);assert.equal(expected.size,0);
console.log('Original Fedora provider:static empty root,private aliases,staged unit parsing and absent legacy runtime ELF passed.');
JS
