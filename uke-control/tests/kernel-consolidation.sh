#!/usr/bin/env bash
# Execute emitted lifecycle fragments with isolated state and fake providers.
set -Eeuo pipefail
spec=$(realpath "${1:?Actual generated consolidated spec}")
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin"
rpmspec -P "$spec" > "$work/expanded"
# Relative doc/license operands copy source documents. Absolute operands instead
# name files inside BUILDROOT, and are inappropriate for these retained inputs.
if grep -Eq '^%(doc|license)[[:space:]]+/' "$work/expanded";then
    printf 'Source documents must use relative staged paths\n' >&2;exit 1
fi
grep -Fx '%doc KERNEL-CONSOLIDATION.md' "$work/expanded"
grep -Fx '%license BOOT-RUNTIME-LICENSE.GPL-3.0' "$work/expanded"
grep -Fx '%license BOOT-RUNTIME-THP-LICENSE' "$work/expanded"
for phase in SAVE RESTORE;do
 awk -v phase="$phase" '
  $0=="# UKE_CONTROL_TRANSFER_"phase"_BEGIN" {copy=1;found++;next}
  $0=="# UKE_CONTROL_TRANSFER_"phase"_END" {copy=0;done++;next}
  copy {print}
  END{if(found!=1||done!=1)exit 1}' "$work/expanded" > "$work/$phase.raw"
 sed "s|/run/uke-control-package-migration|$work/state|g" "$work/$phase.raw" > "$work/$phase"
 bash -n "$work/$phase"
done
cat > "$work/bin/rpm" <<'RPM'
#!/usr/bin/bash
[[ ${UKE_TRANSFER_TEST_OLD:-yes} == yes ]]
RPM
cat > "$work/bin/systemctl" <<'SYSTEMCTL'
#!/usr/bin/bash
set -Eeuo pipefail
printf '%s\n' "$*" >> "$UKE_TRANSFER_TEST_LOG"
case ${1:-} in
 is-enabled)
  case $2 in
   uke-touchscreen.service) printf 'disabled\n';exit 1;;
   uke-userdata-accept.service) printf 'enabled\n';;
   uke-root-grow.service) printf 'masked\n';exit 1;;
   uke-rtc-sync.service) printf 'static\n';;
   uke-rtc-sync.timer) printf 'enabled-runtime\n';;
   *) exit 93;;
  esac;;
 is-active)
  case $2 in uke-userdata-accept.service|uke-rtc-sync.timer) printf 'active\n';;*) printf 'inactive\n';exit 3;;esac;;
 daemon-reload|enable|--no-block) ;;
 *) exit 94;;
esac
SYSTEMCTL
chmod 700 "$work/bin/"*
export PATH="$work/bin:$PATH" UKE_TRANSFER_TEST_LOG=$work/calls UKE_TRANSFER_TEST_OLD=yes
bash -e "$work/SAVE"
[[ $(stat -c %a "$work/state/units") == 600 ]]
[[ $(wc -l < "$work/state/units") == 5 ]]
awk -F '\t' 'NF!=3{bad=1}END{exit bad}' "$work/state/units"
cp "$work/state/units" "$work/saved"
if bash -e "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
cmp "$work/state/units" "$work/saved"
: > "$work/calls"
bash -e "$work/RESTORE"
[[ ! -e $work/state ]]
grep -Fx 'enable uke-userdata-accept.service' "$work/calls"
grep -Fx 'enable --runtime uke-rtc-sync.timer' "$work/calls"
grep -Fx -- '--no-block start uke-userdata-accept.service' "$work/calls"
grep -Fx -- '--no-block start uke-rtc-sync.timer' "$work/calls"
! grep -Eq '(enable|start).*uke-(touchscreen|root-grow|rtc-sync[.]service)' "$work/calls"
bash -e "$work/RESTORE"
export UKE_TRANSFER_TEST_OLD=no
bash -e "$work/SAVE"
[[ ! -e $work/state ]]
export UKE_TRANSFER_TEST_OLD=yes
mkdir "$work/other"
ln -s "$work/other" "$work/state"
if bash -e "$work/SAVE" > "$work/refusal" 2>&1;then exit 1;fi
[[ ! -e $work/other/units ]]
rm "$work/state"
mkdir "$work/state"
printf 'other.service\tenabled\tactive\n' > "$work/state/units"
if bash -e "$work/RESTORE" > "$work/refusal" 2>&1;then exit 1;fi
[[ -f $work/state/units ]]
printf 'Control kernel lifecycle: actual emitted save/restore, disabled/masked/runtime/active retention, idempotence, stale/symlink/foreign refusal passed.\n'
