#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
audit=$root/packaging/families/host/native-migration-audit.sh
guard=$root/packaging/families/host/check-source-retirement.sh
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
trap 'printf "Native migration fixture failed at line %s: %s\n" "$LINENO" "$BASH_COMMAND" >&2; tail -n 4 "$stage/"*.err >&2' ERR
cat > "$stage/copr.json" <<'JSON'
{"items":[{"id":1,"name":"uke-upower"},{"id":2,"name":"uke-battery-observer"}],"meta":{"complete":true,"resource":"package","ownername":"mcc45tr","projectname":"uke-linux-unstable"}}
JSON
printf '%s\n' \
  $'uke-upower\t1.91.5-20.fc46.aarch64\tuke-upower-1.91.5-20.fc46.src.rpm\tFedora Copr' \
  $'uke-upower-libs\t1.91.5-20.fc46.aarch64\tuke-upower-1.91.5-20.fc46.src.rpm\tFedora Copr' \
  $'uke-offline-charge\t1.4.1-7.fc46.aarch64\tuke-battery-observer-1.4.1-7.fc46.src.rpm\tFedora Copr' \
  $'qt6-qtvirtualkeyboard\t6.12.0-1.3.fc46.aarch64\tqt6-qtvirtualkeyboard-6.12.0-1.3.fc46.src.rpm\tFedora Project' \
  $'ddcutil\t3.0.2-2.uke1.fc46.aarch64\tddcutil-3.0.2-2.uke1.fc46.src.rpm\tFedora Copr' \
  $'pipewire\t1.6.11-1.fc46.aarch64\tpipewire-1.6.11-1.fc46.src.rpm\tFedora Project' \
  > "$stage/rpms.tsv"
bash "$audit" audit "$stage/copr.json" "$stage/rpms.tsv" "$stage/result.json"
jq -e '.coverage_complete and .producer_count==2 and .installed_uke_name_count==3 and
  ([.components[].installed[]]|length)==5 and
  any(.components[]; .id=="virtual-keyboard" and (.installed|length)==1 and (.live_producers|length)==0) and
  any(.components[]; .id=="display-ddc" and (.installed|length)==1) and
  any(.components[]; .id=="battery-services" and .installed[0].name=="uke-offline-charge" and .retirement_eligible==false) and
  all(.components[]; .retirement_eligible==false) and all(.acceptance[]; .==false)' "$stage/result.json" >/dev/null
# An unprefixed fork and a new producer must not disappear from the audit.
printf '%s\n' $'ordinary-name\t1-1.fc46.aarch64\tordinary-name-1-1.fc46.src.rpm\tFedora Copr' >> "$stage/rpms.tsv"
jq '.items += [{"id":3,"name":"unreviewed-producer"}]' "$stage/copr.json" > "$stage/unknown.json"
status=0
bash "$audit" audit "$stage/unknown.json" "$stage/rpms.tsv" "$stage/unknown-result.json" 2>"$stage/unknown.err" || status=$?
[[ $status == 2 ]]
jq -e '.coverage_complete==false and .unknown_producers==["unreviewed-producer"] and .unknown_packages[0].name=="ordinary-name"' "$stage/unknown-result.json" >/dev/null
# Pagination, identity and malformed-input failures must not create a report.
for mutation in '.meta.complete=false' '.meta.projectname="other"' '.items += [.items[0]]' '.items[0].id=-1'; do
    jq "$mutation" "$stage/copr.json" > "$stage/bad.json"
    if bash "$audit" audit "$stage/bad.json" "$stage/rpms.tsv" "$stage/invalid-result.json" 2>"$stage/bad.err"; then exit 1; fi
    [[ ! -e $stage/invalid-result.json ]]
done
printf '%s\n' $'missing-fields\t1-1.fc46' > "$stage/bad.tsv"
if bash "$audit" audit "$stage/copr.json" "$stage/bad.tsv" "$stage/invalid-result.json" 2>"$stage/bad.err"; then exit 1; fi
[[ ! -e $stage/invalid-result.json ]]
# Rebuilding retired binaries is refused before source download/build tools.
for key in pipewire uke-pipewire-devel libwacom uke-libwacom-data gstreamer1 uke-gstreamer1-devel at-spi2-core uke-atk libstdcxx-uke-runtime uke-libstdcxx-runtime; do
    status=0
    bash "$guard" "$key" 2>"$stage/guard.err" || status=$?
    [[ $status == 2 ]]
    grep -Fq 'Retired project rebuild' "$stage/guard.err"
done
for key in uke-control upower iio-sensor-proxy mesa uke-firmware; do bash "$guard" "$key"; done
for entry in standalone compose; do
    status=0
    SOURCE_PACKAGE=uke-libwacom bash "$root/packaging/families/host/$entry.sh" uke-libwacom "$stage/out" 2>"$stage/entry.err" || status=$?
    [[ $status == 2 ]]
    grep -Fq 'Retired project rebuild' "$stage/entry.err"
done
status=0
bash "$root/uke-desktop-packaging/src/native-runtime/prepare-source.sh" libstdcxx-uke-runtime "$stage/out" "$root/uke-desktop-packaging" 2>"$stage/direct.err" || status=$?
[[ $status == 2 && ! -e $stage/out ]]
# An umbrella must not revive the same fork from a pinned archived SRPM.
# All build/download tools are stubs, so this fixture cannot compile locally.
mkdir "$stage/bin"
cat > "$stage/bin/rpm" <<'SH'
#!/usr/bin/bash
case "$*" in
    '--eval %{fedora}') echo 46;;
    '--eval %{_host_cpu}') echo aarch64;;
    *) exit 99;;
esac
SH
for command in make curl rpmbuild; do
    printf '#!/usr/bin/bash\nprintf unexpected-tool > "%s"\nexit 99\n' "$stage/unexpected" > "$stage/bin/$command"
done
chmod +x "$stage/bin/"*
status=0
PATH="$stage/bin:$PATH" FEDORA_TARGET=rawhide bash "$root/packaging/families/host/compose.sh" uke-sound "$stage/umbrella-out" 2>"$stage/umbrella.err" || status=$?
[[ $status == 2 && ! -e $stage/unexpected && ! -e $stage/umbrella-out ]]
grep -Fq 'Retired project rebuild' "$stage/umbrella.err"
echo 'PASS: original-name and split-source coverage, fail-closed inventory and retired-source entry points.'
