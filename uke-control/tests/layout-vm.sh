#!/bin/bash
# Host-only, disposable QEMU disks. No physical block devices or network are passed.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
image=localhost/uke-layout-vm-sdk:20261025
mkdir -p build/layout-vm
podman run --rm --network=none -v "$PWD:/src:z" -w /src "$image" -c '
set -Eeuo pipefail
make -B -j2 build/test-layout build/test-layout-vm
kernel=$(basename /usr/lib/modules/*)
cp /usr/lib/modules/"$kernel"/vmlinuz build/layout-vm/vmlinuz
mkdir -p build/layout-vm/include build/layout-vm/empty
cat > build/layout-vm/include/init <<"INIT"
#!/bin/sh
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mkdir -p /mnt/userdata /dev/mapper
for module in virtio_pci virtio_scsi sd_mod dm_mod ext4 f2fs btrfs; do modprobe "$module" 2>/dev/null || :; done
udevadm trigger --subsystem-match=block
udevadm settle
fs=ext4
for word in $(cat /proc/cmdline); do case "$word" in uke_test_fs=*) fs=${word#*=};; esac; done
/src/build/test-layout-vm "$fs"
result=$?
echo UKE_LAYOUT_VM_EXIT="$result"
sync
poweroff -f
INIT
chmod 755 build/layout-vm/include/init
install_tools="/usr/bin/mkfs.ext4 /usr/bin/e2fsck /usr/bin/resize2fs /usr/bin/mkfs.f2fs /usr/bin/fsck.f2fs /usr/bin/resize.f2fs /usr/bin/mkfs.btrfs /usr/bin/btrfs /usr/bin/mkfs.vfat /usr/bin/fsck.fat /usr/bin/dmsetup /usr/bin/wipefs /usr/bin/udevadm /usr/bin/mount /usr/bin/umount /usr/bin/modprobe /usr/bin/cat /usr/bin/mkdir /usr/bin/poweroff /usr/bin/sync /src/build/test-layout-vm"
dracut --force --conf /dev/null --confdir build/layout-vm/empty --no-hostonly --no-hostonly-cmdline --no-hostonly-default-device --no-early-microcode --modules "base kernel-modules" --add-drivers "virtio_pci virtio_scsi sd_mod dm_mod ext4 f2fs btrfs" --install "$install_tools" --include build/layout-vm/include / build/layout-vm/initramfs.img "$kernel"
for type in ext4 f2fs btrfs; do
  disk=build/layout-vm/"$type".raw
  test ! -e "$disk"
  build/test-layout --create "$disk"
  timeout 1800 qemu-system-x86_64 -machine q35,accel=tcg -m 2048 -smp 4 -nodefaults -no-reboot -display none -serial stdio -kernel build/layout-vm/vmlinuz -initrd build/layout-vm/initramfs.img -append "console=ttyS0 panic=-1 uke_test_fs=$type" -drive "file=$disk,format=raw,if=none,id=testdisk,cache=none" -device virtio-scsi-pci,id=scsi -device scsi-hd,bus=scsi.0,drive=testdisk,serial=UKE-LAYOUT-VM,wwn=0x5000000000000001,logical_block_size=4096,physical_block_size=4096 -nic none > build/layout-vm/"$type".log 2>&1
  grep -F "UKE_LAYOUT_VM_PASS $type" build/layout-vm/"$type".log
  grep -F "UKE_LAYOUT_VM_EXIT=0" build/layout-vm/"$type".log
  rm -- "$disk"
done
'
