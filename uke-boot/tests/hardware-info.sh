#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -O2 src/hardware-info.cpp -o build/tests/uke-hardware-info
fixture=$(mktemp -d build/tests/hardware.XXXXXX)
trap 'rm -rf "$fixture"' EXIT
dt=$fixture/sys/firmware/devicetree/base
mkdir -p "$dt/chosen" "$fixture/proc" "$fixture/etc" "$fixture/usr/bin"
cp build/tests/uke-hardware-info "$fixture/usr/bin/"
tool=(build/tests/uke-hardware-info --root "$fixture")
refuse() {
    local expected=$1
    shift
    if "${tool[@]}" "$@" > "$fixture/out" 2> "$fixture/error"; then exit 1; else test "$?" -eq "$expected"; fi
    test ! -s "$fixture/out"
    ! grep -Fq 'TEST-FACTORY-SERIAL' "$fixture/error"
}
refuse 2
printf 'xiaomi,uke-impostor\0qcom,sm7675\0' > "$dt/compatible"
refuse 2
printf 'xiaomi,uke\0qcom,sm7675\0' > "$dt/compatible"
printf 'Xiaomi Pad 7\0' > "$dt/model"
"${tool[@]}" > "$fixture/out"
jq -e '.processor=="Qualcomm Snapdragon 7+ Gen 3" and .graphics_processor=="Qualcomm Adreno 732" and .manufacturer=="Xiaomi Communications Co., Ltd." and .product_name=="Xiaomi Pad 7" and .system_version=="Rev:1.1" and .system_version_source=="requested-presentation-label" and .oem_hwversion==null and .serial_number==null and (.hardware_acceptance_granted|not)' "$fixture/out" >/dev/null
cat > "$fixture/proc/bootconfig" <<'EOF'
androidboot.product.hardware.sku = "ukepgl"
androidboot.hwversion = "11.19.8"
androidboot.serialno = "TEST-FACTORY-SERIAL"
EOF
"${tool[@]}" > "$fixture/out"
jq -e '.product_name=="POCO Pad X1" and .product_detection=="bootloader-sku" and .oem_hwversion=="11.19.8" and .serial_number==null' "$fixture/out" >/dev/null
! grep -Fq 'TEST-FACTORY-SERIAL' "$fixture/out"
"${tool[@]}" --show-serial > "$fixture/out"
jq -e '.serial_number=="TEST-FACTORY-SERIAL" and .serial_source=="bootloader-serialno"' "$fixture/out" >/dev/null
printf 'TEST-FACTORY-SERIAL\0' > "$dt/serial-number"
"${tool[@]}" --show-serial >/dev/null
printf 'DIFFERENT-FACTORY-SERIAL\0' > "$dt/serial-number"
refuse 1 --show-serial
rm "$dt/serial-number"
"${tool[@]}" --machine-info > "$fixture/out"
! grep -Eq 'SERIAL|11\.19\.8' "$fixture/out"
refuse 1 --machine-info --show-serial
# Preserve unrelated settings, restrictive permissions and idempotence.
printf 'PRETTY_HOSTNAME="Lab tablet"\nCHASSIS=tablet\nHARDWARE_MODEL=old' > "$fixture/etc/machine-info"
chmod 600 "$fixture/etc/machine-info"
sh src/uke-hardware-identity.sh "$fixture"
grep -Fxq 'PRETTY_HOSTNAME="Lab tablet"' "$fixture/etc/machine-info"
grep -Fxq 'HARDWARE_MODEL="POCO Pad X1"' "$fixture/etc/machine-info"
test "$(stat -c %a "$fixture/etc/machine-info")" = 600
cp "$fixture/etc/machine-info" "$fixture/before"
sh src/uke-hardware-identity.sh "$fixture"
cmp "$fixture/before" "$fixture/etc/machine-info"
printf 'androidboot.product.hardware.sku=foreign\n' > "$fixture/proc/cmdline"
if sh src/uke-hardware-identity.sh "$fixture" 2>/dev/null; then exit 1; fi
cmp "$fixture/before" "$fixture/etc/machine-info"
refuse 1
rm "$fixture/proc/bootconfig"
printf 'androidboot.product.hardware.sku="ukepgl" androidboot.serialno=TEST-FACTORY-SERIAL\n' > "$fixture/proc/cmdline"
"${tool[@]}" --show-serial > "$fixture/out"
jq -e '.product_name=="POCO Pad X1" and .serial_number=="TEST-FACTORY-SERIAL"' "$fixture/out" >/dev/null
printf 'androidboot.product.hardware.sku=ukepgl androidboot.serialno=TEST-FACTORY-SERIAL\0' > "$dt/chosen/bootargs"
rm "$fixture/proc/cmdline"
"${tool[@]}" --show-serial > "$fixture/out"
jq -e '.product_name=="POCO Pad X1" and .serial_number=="TEST-FACTORY-SERIAL"' "$fixture/out" >/dev/null
printf 'androidboot.product.hardware.sku = "ukepgl"\nandroidboot.product.hardware.sku = "other"\n' > "$fixture/proc/bootconfig"
refuse 1
printf 'androidboot.serialno = "bad\\\"value"\n' > "$fixture/proc/bootconfig"
refuse 1 --show-serial
printf 'androidboot.product.hardware.sku = "unterminated\n' > "$fixture/proc/bootconfig"
refuse 1
rm "$fixture/proc/bootconfig" "$dt/chosen/bootargs"
printf 'TEST-FACTORY-SERIAL\0' > "$dt/serial-number"
"${tool[@]}" --show-serial > "$fixture/out"
jq -e '.serial_number=="TEST-FACTORY-SERIAL" and .serial_source=="device-tree-serial-number"' "$fixture/out" >/dev/null
printf '000000\0' > "$dt/serial-number"
"${tool[@]}" --show-serial | jq -e '.serial_number==null' >/dev/null
printf 'unterminated' > "$dt/serial-number"
refuse 1 --show-serial
rm "$dt/serial-number"
printf 'POCO Pad X1\0' > "$dt/model"
"${tool[@]}" | jq -e '.product_name=="POCO Pad X1" and .product_detection=="device-tree-model"' >/dev/null
printf 'androidboot.product.hardware.sku = "ukepgl"\n' > "$fixture/proc/bootconfig"
head -c 65537 /dev/zero >> "$fixture/proc/bootconfig"
refuse 1
rm "$fixture/proc/bootconfig" "$fixture/etc/machine-info"
ln -s "$fixture/before" "$fixture/etc/machine-info"
if sh src/uke-hardware-identity.sh "$fixture" 2>/dev/null; then exit 1; fi
test -L "$fixture/etc/machine-info"
rm "$fixture/etc/machine-info"
ln "$fixture/before" "$fixture/etc/machine-info"
if sh src/uke-hardware-identity.sh "$fixture" 2>/dev/null; then exit 1; fi
rm "$fixture/etc/machine-info"
sh src/uke-hardware-identity.sh "$fixture"
test "$(stat -c %a "$fixture/etc/machine-info")" = 644
printf 'xiaomi,nabu\0qcom,sm8150\0' > "$dt/compatible"
cp "$fixture/etc/machine-info" "$fixture/before-foreign"
if sh src/uke-hardware-identity.sh "$fixture" 2>/dev/null; then exit 1; fi
cmp "$fixture/before-foreign" "$fixture/etc/machine-info"
! find "$fixture/etc" -name '.uke-machine-info.*' | grep -q .
printf '%s\n' 'Uke identity fixtures passed: SKU, serial opt-in, conflict refusal and atomic machine-info preservation'
