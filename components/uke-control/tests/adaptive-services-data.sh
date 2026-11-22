#!/usr/bin/bash
# Source/archive/transaction fixtures only. No compiler or device access.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1")
input=$(realpath "$2")
output=$(realpath -m "$3")
here=$(cd -- "$(dirname -- "$0")" && pwd)
packaging=$here/../packaging
[[ ! -e $output && ! -L $output ]]
mkdir "$output"
cp "$parent" "$output/uke-control.spec"
node "$packaging/fold-adaptive-spec.mjs" "$output/uke-control.spec" "$packaging/adaptive-services.json" "$packaging/adaptive-unit-state.sh"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$packaging/adaptive-services.json")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
bash "$packaging/prepare-adaptive-sources.sh" "$input" "$output/qualified-source" "$packaging/adaptive-services.json"
bash "$here/adaptive-unit-state.sh" --data-only "$output/uke-control.spec"
if bash "$packaging/prepare-adaptive-sources.sh" "$input" "$output/qualified-source" "$packaging/adaptive-services.json" > "$output/repeated-output.log" 2>&1;then exit 1;fi
cp "$parent" "$output/altered-parent.spec"
printf '\n# altered parent\n' >> "$output/altered-parent.spec"
if node "$packaging/fold-adaptive-spec.mjs" "$output/altered-parent.spec" "$packaging/adaptive-services.json" "$packaging/adaptive-unit-state.sh" > "$output/altered-parent.log" 2>&1;then exit 1;fi
for alteration in archive runtime dependency;do
 case $alteration in
  archive) expression='.archives.power=("0"*64)';;
  runtime) expression='.runtime_files[0].sha256=("0"*64)';;
  dependency) expression='.dependency_scope.filter_path=".*"';;
 esac
 jq "$expression" "$packaging/adaptive-services.json" > "$output/$alteration.json"
 if [[ $alteration == dependency ]];then
  cp "$parent" "$output/dependency.spec"
  if node "$packaging/fold-adaptive-spec.mjs" "$output/dependency.spec" "$output/dependency.json" "$packaging/adaptive-unit-state.sh" > "$output/dependency.log" 2>&1;then exit 1;fi
 else
  if bash "$packaging/prepare-adaptive-sources.sh" "$input" "$output/refused-$alteration" "$output/$alteration.json" > "$output/$alteration.log" 2>&1;then exit 1;fi
 fi
done
printf 'Adaptive data checks:complete parent inverse,26 source files,19 conditional ELF capabilities,transaction-state fixtures and five altered/repeated-input refusals pass;no native build or physical claim.\n'
