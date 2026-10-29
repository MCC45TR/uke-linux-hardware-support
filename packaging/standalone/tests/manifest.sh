#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
manifest=packaging/standalone/packages.json
jq -e '.schema==1 and .copr=="mcc45tr/uke-linux-unstable" and .chroots==["fedora-rawhide-aarch64"] and
  (.packages|length)==30 and ([.packages[].name]|unique|length)==30 and
  ([.packages[].input]|unique|length)==30 and
  all(.packages[]; (.name|test("^uke-[A-Za-z0-9+_.-]+$")) and (.input|test("^[A-Za-z0-9+_.-]+$")) and
    (.release|type)=="number" and .release>0 and .release==(.release|floor))' "$manifest" >/dev/null
cmp <(jq -r '.packages[].replaces[]' "$manifest" | LC_ALL=C sort) \
    <(jq -r '.families[].name' packaging/families/families.json | LC_ALL=C sort)
while IFS=$'\t' read -r name group; do
    [[ -s packaging/standalone/$name/Makefile ]]
    jq -e --arg group "$group" 'any(.families[];.name==$group)' packaging/families/families.json >/dev/null
    ! jq -e --arg name "$name" 'any(.families[];.name==$name)' packaging/families/families.json >/dev/null
done < <(jq -r '.packages[]|[.name,.group]|@tsv' "$manifest")
for script in packaging/standalone/{host,tests}/*.sh; do bash -n "$script"; done
printf '%s\n' 'PASS: thirty independent source entries, Rawhide AArch64 only, and one owner per retired selection.'
