#!/usr/bin/env bash
# COPR/host-only: never compile or create a fixture on the physical Uke root.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$stage/content/firmware" build/tests
printf 'fixture\0payload' > "$stage/content/firmware/vpu30_2v.mbn"
truncate -s 8M "$stage/ext4.img"
/usr/sbin/mke2fs -q -F -t ext4 -d "$stage/content" "$stage/ext4.img"
${CXX:-c++} -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-function tests/oem-video.cpp \
    -o build/tests/oem-video $(pkg-config --cflags --libs openssl libsystemd fdisk zlib)
build/tests/oem-video "$stage/ext4.img"
