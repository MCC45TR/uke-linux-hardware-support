#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Project source generation only; never filter original Fedora dependencies.
set -Eeuo pipefail
[[ $# == 1 && $1 =~ ^[A-Za-z0-9+_.-]+$ ]]
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
reason=$(jq -er --arg name "$1" '
  if .schema != 1 or (.retired_source_families|type)!="array" then error("Invalid retirement policy") else
    [.retired_source_families[]|select(any(.names[]; . as $base|
      $name==$base or ($name|startswith($base+"-"))))|.replacement]|join(" ")
  end' "$root/packaging/families/consolidation/native-migration.json")
if [[ -n $reason ]]; then
    printf 'Retired project rebuild %s: %s\n' "$1" "$reason" >&2
    exit 2
fi
