#!/bin/bash
# Read-only byte identity check against an explicitly reviewed receipt.
set -euo pipefail
[[ $# == 4 ]]
receipt=$1; cohort=$(realpath "$2"); evidence=$(realpath "$3"); early=$4
[[ -f $receipt && ! -L $receipt && -f $early && ! -L $early ]]
early=$(realpath "$early")
jq -e '
 .schema=="uke-verified-native-artifact-v1" and .architecture=="arm64" and
 .source_commit=="fcbdae4b757732f0f0e3162c610f19abae290832" and
 .producer_run==41331215146 and .kernel_release=="7.3.1-1006u1" and
 .original_zip_sha256=="3494352ab845bd345b6191ec5c049780eaa1550b2b3b1b994560844fe84741a9" and
 .version=="7.3.1-1006ubuntu1~tenacious1" and
 .artifact_only==true and .root_image_admitted==false and .redistribution_admitted==false and
 (.files|length==14) and ([.files[]|.scope+"/"+.path]|unique|length)==(.files|length) and
 ([.files[]|.scope+"/"+.path]|sort)==([
  "cohort/Image","cohort/.config","cohort/module-signing.x509","cohort/modules.tar.xz",
  "cohort/sm7675-xiaomi-uke.dtb","cohort/linux.img","cohort/userdata.img",
  "cohort/boot-manifest","cohort/boot-header.json","cohort/modules.sha256",
  "evidence/payload-provenance.tsv","evidence/build-debs.tsv","evidence/base-links.tsv",
  "early/early-modules.list"]|sort) and
 all(.files[]; (.scope|IN("cohort","evidence","early")) and
  (.path|test("^[A-Za-z0-9_./-]+$")) and (.path|startswith("/")|not) and
  (.path|contains("..")|not) and (.sha256|test("^[0-9a-f]{64}$")) and
  (.size|type=="number" and .>=0))
' "$receipt" >/dev/null
while IFS=$'\t' read -r scope path hash size; do
 case $scope in cohort) file=$cohort/$path;; evidence) file=$evidence/$path;; early) [[ $path == early-modules.list ]]; file=$early;; esac
 [[ -f $file && ! -L $file && $(stat -c %s "$file") == "$size" ]]
 printf '%s  %s\n' "$hash" "$file" | sha256sum -c -
done < <(jq -r '.files[]|[.scope,.path,.sha256,.size]|@tsv' "$receipt")
