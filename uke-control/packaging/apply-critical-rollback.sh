#!/usr/bin/bash
# Apply only after the retained2.11 overlay, before native compilation.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
boot=$(realpath "${1:?Prepared boot runtime}")
status=$(realpath "${2:?Actual Control update-status source}")
overlay=$(cd -- "$(dirname -- "$0")" && pwd)
[[ -f $boot/src/boot-health-report.hpp && ! -e $boot/src/critical-health.hpp ]]
(cd "$boot";printf '%s\n' \
 'e25295b369821c5cadd87b2c485c80139b7b43dd3956941a09adac9def5f35f1  src/kernel-update.cpp' \
 '441ef1dc07b058ecde4df5d60741c448da4548370541d81412a5491aad255dab  src/userdata-update.hpp' \
 'ffded2f28652458cb7f400ad159d115326d720f51b888cc3ed20442096845a8e  src/seamless-policy.hpp' | sha256sum -c --strict --quiet -)
git -C "$boot" apply --check "$overlay/critical-rollback.patch"
git -C "$boot" apply "$overlay/critical-rollback.patch"
install -m644 "$overlay/critical-health.hpp" "$overlay/critical-runtime.hpp" "$overlay/boot-health-report.hpp" "$boot/src/"
install -m644 "$overlay/critical-health.cpp" "$overlay/critical-health.sh" "$overlay/boot-health-report.cpp" "$boot/tests/"
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
awk '
 $0=="        policy[\"rollback_mode\"]=mode; policy[\"automatic_fallback\"]=mode==\"enforced\";" {
  print "        policy[\"rollback_mode\"]=mode; policy[\"automatic_fallback\"]=mode!=\"off\";"
  print "        policy[\"critical_fallback\"]=mode!=\"off\";"
  print "        policy[\"ordinary_failure_fallback\"]=mode==\"enforced\";";found++;next
 }
 {print}
 END{if(found!=1)exit 1}
' "$status" > "$work/status.cpp"
install -m644 "$work/status.cpp" "$status"
git -C "$boot" apply --reverse --check "$overlay/critical-rollback.patch"
printf 'Critical rollback overlay applied: exact boot/state identity, checkpoint/module/firmware/power and existing metadata transaction remain required; no runtime action executed.\n'
