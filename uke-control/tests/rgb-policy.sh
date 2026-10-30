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
grep -Fxq 'User=uke-rgb' "$root/packaging/uke-control-flashlight.service"
grep -Fxq 'CapabilityBoundingSet=' "$root/packaging/uke-control-flashlight.service"
grep -Fxq 'DeviceAllow=/dev/uke-torch rw' "$root/packaging/uke-control-flashlight.service"
for permission in read write open ioctl; do
    grep -Eq "^\\(allow uke_rgb_t uke_torch_device_t \\(chr_file \\([^)]*${permission}([ )])" "$cil"
done
if grep -Eq '^\(allow uke_rgb_t sysfs_t \(file \([^)]*write([ )])' "$cil"; then
    echo 'LED broker must not write generic sysfs attributes' >&2; exit 1
fi
! grep -Eq 'uke_torch_brightness_t|uke_torch_runtime_t' "$cil"
for name in rgb torch; do
    grep -Fq '<allow_active>auth_admin</allow_active>' "$root/packaging/org.senemos.uke.$name.policy"
    ! grep -Eq 'auth_.*_keep|<allow_active>yes</allow_active>' "$root/packaging/org.senemos.uke.$name.policy"
done
grep -Eq '^\(allowx uke_rgb_t uke_torch_device_t \(ioctl chr_file' "$cil"
grep -Eq '^\(allowx uke_rgb_t uke_rgb_device_t \(ioctl chr_file' "$cil"
grep -Fq '<associate own="org.senemos.Uke.Torch" context="system_u:object_r:uke_rgb_t:s0"/>' "$root/packaging/org.senemos.Uke.Torch.conf"
grep -Fq '<annotate key="org.freedesktop.policykit.owner">unix-user:uke-rgb</annotate>' "$root/packaging/org.senemos.uke.torch.policy"
grep -Fq '<associate own="org.senemos.Uke.Rgb" context="system_u:object_r:uke_rgb_t:s0"/>' "$root/packaging/org.senemos.Uke.Rgb.conf" || {
    echo 'RGB bus name lacks its admitted SELinux target context' >&2
    exit 1
}
grep -Eq '^\(allow uke_rgb_t self \(dbus \([^)]*acquire_svc([ )])' "$cil"
grep -Fq '<annotate key="org.freedesktop.policykit.owner">unix-user:uke-rgb</annotate>' "$root/packaging/org.senemos.uke.rgb.policy" || {
    echo 'RGB mechanism lacks ownership of its Polkit authorization query' >&2
    exit 1
}
# The host parse fixture uses the build UID without creating a service account.
for name in Rgb Torch; do
    sed "s/policy user=\"uke-rgb\"/policy user=\"$(id -u)\"/" "$root/packaging/org.senemos.Uke.$name.conf" > "$stage/$name.conf"
done
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
  <include>$stage/Rgb.conf</include>
  <include>$stage/Torch.conf</include>
</busconfig>
EOF
dbus-run-session --config-file="$stage/bus.conf" -- /usr/bin/true
printf '%s\n' 'Compiled RGB transition/enforcement/name permissions and parsed bus configuration: PASS (host only)'
