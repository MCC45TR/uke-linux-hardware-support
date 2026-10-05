#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
for tool in dtc fdtget jq; do command -v "$tool" >/dev/null; done
mkdir -p build/tests
fixture=$(mktemp -d build/tests/dtb.XXXXXX)
trap 'rm -rf "$fixture"' EXIT
base=$fixture/root/usr/lib/modules/7.2.9-senemos-uke
mkdir -p "$base/dtb/qcom"
printf 'Host-only synthetic kernel bytes\n' > "$base/vmlinuz"
printf '%s\n' CONFIG_EFI=y CONFIG_EFI_STUB=y CONFIG_SCSI_UFS_QCOM=m CONFIG_PHY_QCOM_QMP_UFS=m > "$base/config"
cat > "$fixture/source.dts" <<'DTS'
/dts-v1/;
/ {
    compatible = "xiaomi,uke", "qcom,sm7675";
    #address-cells = <2>;
    #size-cells = <2>;
    memory@80000000 { device_type = "memory"; reg = <0 0x80000000 0 0x100000>; };
    bus {
        ufs { compatible = "qcom,fixture-ufshc"; firmware-name = "qcom/fixture.bin"; };
    };
};
DTS
compile() { dtc -I dts -O dtb -o "$base/dtb/qcom/sm7675-xiaomi-uke.dtb" "$fixture/source.dts"; }
compile
bash src/audit-kernel-inputs.sh "$fixture/root" "$fixture/accepted.json"
jq -e '.static_memory_node and .enabled_ufs_node and (.requests|length)==1 and .hardware_tested==false and .uefi_firmware_accepted==false' "$fixture/accepted.json" >/dev/null
sed -i 's/^    bus {/    bus { status = "disabled";/' "$fixture/source.dts"
compile
if bash src/audit-kernel-inputs.sh "$fixture/root" "$fixture/rejected.json"; then exit 1; else test "$?" -eq 2; fi
jq -e '.enabled_ufs_node==false and (.requests|length)==0' "$fixture/rejected.json" >/dev/null
sed -i 's/xiaomi,uke/xiaomi,nabu/' "$fixture/source.dts"
compile
if bash src/audit-kernel-inputs.sh "$fixture/root" "$fixture/foreign.json"; then exit 1; else test "$?" -eq 1; fi
printf '%s\n' 'DTB fixtures passed; disabled parent nodes and foreign identity rejected; no boot acceptance inferred'
