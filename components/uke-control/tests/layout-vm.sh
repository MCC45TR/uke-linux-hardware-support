#!/bin/bash
# Host-only: only disposable images enter QEMU. No host block devices or network.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
image=localhost/uke-layout-xfs-sdk:20261025
scenarios=("$@")
if (( $# == 0 )); then scenarios=(ext4 f2fs btrfs xfs btrfs-fail-create btrfs-fail-merge shutdown shutdown-tamper); fi
for scenario in "${scenarios[@]}"; do
  case "$scenario" in ext4|f2fs|btrfs|xfs|btrfs-fail-create|btrfs-fail-merge|shutdown|shutdown-tamper) :;; *) exit 2;; esac
done
mkdir -p build/layout-vm
podman run --rm --network=none -v "$PWD:/src:z" -w /src "$image" -c '
set -Eeuo pipefail
make -j3 build/layout-manager build/test-layout build/test-layout-vm build/test-layout-shutdown-vm build/test-layout-payload
install -Dm755 build/layout-manager /usr/libexec/uke-control/layout-manager
parent=$(mktemp -d build/layout-vm/payload-parent-XXXXXX)
build/test-layout-payload "$parent/payload"
kernel=$(basename /usr/lib/modules/*)
cp /usr/lib/modules/"$kernel"/vmlinuz build/layout-vm/vmlinuz
mkdir -p build/layout-vm/include build/layout-vm/empty
install -Dm755 build/test-layout-shutdown-vm build/layout-vm/include/usr/libexec/uke-control/layout-manager
install -Dm755 "$parent/payload/tree/var/lib/dracut/hooks/shutdown/90-uke-layout.sh" build/layout-vm/include/var/lib/dracut/hooks/shutdown/90-uke-layout.sh
cat > build/layout-vm/include/init <<"INIT"
#!/bin/sh
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mkdir -p /mnt/userdata /dev/mapper
for module in virtio_pci virtio_scsi sd_mod dm_mod ext4 f2fs btrfs xfs; do modprobe "$module" 2>/dev/null || :; done
udevadm trigger --subsystem-match=block
udevadm settle
scenario=ext4
for word in $(cat /proc/cmdline); do case "$word" in uke_test_fs=*) scenario=${word#*=};; esac; done
case "$scenario" in
 shutdown*)
    if /usr/libexec/uke-control/layout-manager --setup; then exec /shutdown reboot; fi
    echo UKE_LAYOUT_SHUTDOWN_SETUP_FAILED
    ;;
 *) /src/build/test-layout-vm "$scenario"; echo UKE_LAYOUT_VM_EXIT="$?";;
esac
sync
poweroff -f
INIT
chmod 755 build/layout-vm/include/init
install_tools="/usr/bin/mkfs.ext4 /usr/bin/e2fsck /usr/bin/resize2fs /usr/bin/mkfs.f2fs /usr/bin/fsck.f2fs /usr/bin/resize.f2fs /usr/bin/mkfs.btrfs /usr/bin/btrfs /usr/bin/mkfs.xfs /usr/bin/xfs_repair /usr/bin/xfs_growfs /usr/bin/mkfs.vfat /usr/bin/fsck.fat /usr/bin/mkntfs /usr/bin/ntfsfix /usr/bin/mkfs.exfat /usr/bin/fsck.exfat /usr/bin/mkswap /usr/bin/blkid /usr/bin/dmsetup /usr/bin/wipefs /usr/bin/udevadm /usr/bin/mount /usr/bin/umount /usr/bin/modprobe /usr/bin/cat /usr/bin/mkdir /usr/bin/poweroff /usr/bin/sync /src/build/test-layout-vm /src/build/test-layout-shutdown-vm"
dracut --force --conf /dev/null --confdir build/layout-vm/empty --no-hostonly --no-hostonly-cmdline --no-hostonly-default-device --no-early-microcode --modules "base shutdown kernel-modules" --add-drivers "virtio_pci virtio_scsi sd_mod dm_mod ext4 f2fs btrfs xfs" --install "$install_tools" --include build/layout-vm/include / build/layout-vm/initramfs.img "$kernel"
for scenario in "$@"; do
  disk=build/layout-vm/"$scenario".raw
  test ! -e "$disk"
  build/test-layout --create "$disk"
  extra=()
  case "$scenario" in shutdown*)
    journal=build/layout-vm/"$scenario"-journal.raw
    test ! -e "$journal"
    truncate -s 512M "$journal"
    extra=(-drive "file=$journal,format=raw,if=none,id=journal,cache=none" -device "scsi-hd,bus=scsi.0,scsi-id=1,drive=journal,serial=UKE-JOURNAL-VM,wwn=0x5000000000000002,logical_block_size=4096,physical_block_size=4096")
    ;;
  esac
  timeout 1800 qemu-system-x86_64 -machine q35,accel=tcg -m 2048 -smp 4 -nodefaults -no-reboot -display none -serial stdio -kernel build/layout-vm/vmlinuz -initrd build/layout-vm/initramfs.img -append "console=ttyS0 panic=-1 uke_test_fs=$scenario" -drive "file=$disk,format=raw,if=none,id=testdisk,cache=none" -device virtio-scsi-pci,id=scsi -device scsi-hd,bus=scsi.0,scsi-id=0,drive=testdisk,serial=UKE-LAYOUT-VM,wwn=0x5000000000000001,logical_block_size=4096,physical_block_size=4096 "${extra[@]}" -nic none > build/layout-vm/"$scenario".log 2>&1
  grep -F "UKE_LAYOUT_VM_PASS $scenario " build/layout-vm/"$scenario".log
  if [[ $scenario != shutdown* ]]; then grep -F "UKE_LAYOUT_VM_EXIT=0" build/layout-vm/"$scenario".log; fi
  rm -- "$disk"
  case "$scenario" in shutdown*) rm -- "$journal";; esac
done
' layout-vm "${scenarios[@]}"
