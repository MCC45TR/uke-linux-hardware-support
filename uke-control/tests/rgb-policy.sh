#!/usr/bin/env bash
# Check the compiled permission required by the actual hardened service.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
policy=${1:-$root/build/selinux/uke_control_rgb.pp}
stage=$(mktemp -d /tmp/uke-rgb-policy.XXXXXX)
trap 'rm -rf -- "$stage"' EXIT
cil=$stage/policy.cil
grep -Fxq 'NoNewPrivileges=yes' "$root/packaging/uke-control-rgb.service"
/usr/libexec/selinux/hll/pp < "$policy" > "$cil"
grep -Eq '^\(allow init_t uke_rgb_t \(process2 \([^)]*nnp_transition([ )])' "$cil" || {
    echo 'Compiled RGB policy lacks the systemd NoNewPrivileges transition' >&2
    exit 1
}
for permission in read open; do
    grep -Eq "^\\(allow uke_rgb_t security_t \\(file \\([^)]*${permission}([ )])" "$cil" || {
        echo 'Compiled RGB policy cannot read the enforcing state' >&2
        exit 1
    }
done
grep -Fq '<associate own="org.senemos.Uke.Rgb" context="system_u:object_r:uke_rgb_t:s0"/>' "$root/packaging/org.senemos.Uke.Rgb.conf" || {
    echo 'RGB bus name lacks its admitted SELinux target context' >&2
    exit 1
}
grep -Eq '^\(allow uke_rgb_t self \(dbus \([^)]*acquire_svc([ )])' "$cil"
# The host parse fixture uses the build UID without creating a service account.
sed "s/policy user=\"uke-rgb\"/policy user=\"$(id -u)\"/" "$root/packaging/org.senemos.Uke.Rgb.conf" > "$stage/service.conf"
cat > "$stage/bus.conf" <<EOF
<busconfig>
  <type>session</type>
  <listen>unix:tmpdir=/tmp</listen>
  <policy context="default">
    <allow user="*"/>
    <allow own="*"/>
    <allow send_destination="*"/>
    <allow receive_sender="*"/>
  </policy>
  <include>$stage/service.conf</include>
</busconfig>
EOF
dbus-run-session --config-file="$stage/bus.conf" -- /usr/bin/true
printf '%s\n' 'Compiled RGB transition/enforcement/name permissions and parsed bus configuration: PASS (host only)'
