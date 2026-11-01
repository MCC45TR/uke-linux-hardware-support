#!/bin/bash
# Validate this one attributed shared preset; never infer rights for other blobs.
set -euo pipefail
source=${1:?Extracted speaker source directory required}
cd "$source"
jq -e '.source_path=="/odm/firmware/fs19xx.fsm" and .target_path=="qcom/uke/audio/fs19xx.fsm" and .license=="Apache-2.0" and .notice_content_id=="3b83ef96387f14655fc854ddc3c6bd57" and .unit_calibration_included==false and .hardware_tested==false and .bytes==4828' provenance.json >/dev/null
printf '%s  %s\n' f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b fs19xx.fsm 71aef2c9cb076992714858bd3861590e7329c82098501c19a7075b00c9540727 odm-NOTICE.xml.gz 8c6db340475136df3c1201d458fa5755698eace76e510471ecc9d857d6083dac LICENSE.Apache-2.0 | sha256sum -c -
[[ $(stat -c %s fs19xx.fsm) == 4828 ]]
# The verified original notice must contain the exact filename/content-ID mapping.
gzip -dc odm-NOTICE.xml.gz | awk '/<file-name / && /contentId="3b83ef96387f14655fc854ddc3c6bd57"/ && />\/odm\/firmware\/fs19xx[.]fsm<\/file-name>/ {found=1} END {exit !found}'
printf '%s\n' 'PASS: exact speaker preset, original notice mapping and license identity; no hardware qualification'
