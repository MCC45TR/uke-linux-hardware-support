#!/usr/bin/env bash
# Static/package and unprivileged refusal tests; never activate policy or mount disks.
set -euo pipefail
cd "$(dirname "$0")/.."
policy=packaging/org.senemos.uke.chroot.policy
xmllint --noout "$policy"
[[ $(xmllint --xpath 'string(/policyconfig/action/@id)' "$policy") == org.senemos.uke.chroot.manage ]]
[[ $(xmllint --xpath 'string(/policyconfig/action/defaults/allow_any)' "$policy") == no ]]
[[ $(xmllint --xpath 'string(/policyconfig/action/defaults/allow_inactive)' "$policy") == no ]]
[[ $(xmllint --xpath 'string(/policyconfig/action/defaults/allow_active)' "$policy") == auth_admin ]]
[[ $(xmllint --xpath 'string(/policyconfig/action/annotate)' "$policy") == /usr/libexec/uke-control/chroot-manager ]]
! grep -Eq 'allow_gui|auth_admin_keep|<allow_.*>yes|NOPASSWD' "$policy"
grep -Fq '%{_libexecdir}/uke-control/chroot-manager' packaging/uke-control.spec
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
chmod 755 "$work"
install -m755 build/chroot-manager "$work/manager"
"$work/manager" --help | grep -F 'NOT a security boundary'
if [[ $(id -u) == 0 ]]; then runner=(setpriv --reuid=65534 --regid=65534 --clear-groups); else runner=(); fi
set +e
"${runner[@]}" "$work/manager" scan /dev/never-open-this > "$work/out" 2> "$work/error"
status=$?
set -e
[[ $status != 0 && ! -s $work/out ]]
grep -F 'Administrator authorization is required' "$work/error"
for action in 'shell /bin/sh' 'enter /dev/no token --read-write' 'scan /dev/no --shell'; do
    read -r -a arguments <<< "$action"
    if "$work/manager" "${arguments[@]}" > "$work/out" 2> "$work/error"; then exit 1; fi
    grep -q 'Invalid chroot command' "$work/error"
done
printf '%s\n' 'Chroot policy XML/package boundary, active-local auth_admin, unprivileged pre-device refusal and arbitrary-command/write-mode rejection passed; policy not activated'
