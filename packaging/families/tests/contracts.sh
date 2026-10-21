#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
jq -e '.schema==1 and ([.families[].name]|(length==9) and (unique|length==9))' packaging/families/families.json >/dev/null
jq -e 'all(.[]; (.sha256|test("^[a-f0-9]{64}$")) and (.url|startswith("https://download.copr.fedorainfracloud.org/results/mcc45tr/")))' packaging/families/source-inputs.json >/dev/null
jq -e 'any(.[]; .name=="material-decoration" and .family=="uke-extras")' packaging/families/source-inputs.json >/dev/null
jq -e '.rename["material-decoration"]=="material-decoration" and (.exceptions|length)==4' packaging/families/families.json >/dev/null
for script in packaging/families/host/*.sh; do bash -n "$script"; done
for name in uke-core uke-hw uke-power uke-sensors uke-extras uke-additional uke-metas uke-camera uke-sound; do
    spec="packaging/families/build/$name/SPECS/$name.spec"
    [[ -f $spec ]] || continue
    rpmspec --target aarch64 -q --qf '%{NAME}\n' "$spec" >"$spec.names"
    if grep -Ev '^(uke-[A-Za-z0-9+_.-]+|material-decoration)$' "$spec.names"; then exit 1; fi
    [[ $(rpmspec --target aarch64 -q --srpm --qf '%{NAME}' "$spec") == "$name" ]]
    ! grep -q '/__UKE_' "$spec"
    ! grep -qE '^(Requires|Recommends):.*(python|pypy)' "$spec"
done
hw_spec=packaging/families/build/uke-hw/SPECS/uke-hw.spec
if [[ -f $hw_spec ]]; then
    for name in adsp-firmware wpss-firmware wlan-board wlan-regdb; do
        grep -Fxq "Obsoletes: uke-owner-$name <= 3.0.304.0-1.owner.fc46" "$hw_spec"
    done
fi
[[ ! -e uke-desktop-packaging/packaging/fastfetch ]]
jq -e '(.modules[]|select(type=="object" and .type=="cpu")|.format)=="{vendor} {name} ({cores-online} cores) @ {freq-max}" and all(.modules[]; if type=="string" then (test("^(bios|board|host|physicalmemory|command)$")|not) else (.type|test("^(bios|board|host|physicalmemory|command)$")|not) end)' uke-desktop-packaging/src/fastfetch/config.jsonc >/dev/null
grep -Fx 'Version: 1.0.23' uke-control/packaging/uke-control.spec
grep -Fx 'Version: 1.0.15' uke-desktop-integration/packaging/uke-adaptive-display.spec
grep -Fx 'Name: uke-plymonth' uke-desktop-packaging/packaging/plymouth-uke/plymouth-uke.spec
if [[ -e packaging/families/build/uke-additional/SPECS/uke-additional.spec ]]; then
    grep -Fx 'BuildRequires: llvm-devel = 23.1.3-1.fc46' packaging/families/build/uke-additional/SPECS/uke-additional.spec
fi
printf '%s\n' 'PASS: family/source identity, prefix, pinned inputs, native versions and no Fastfetch fork.'
