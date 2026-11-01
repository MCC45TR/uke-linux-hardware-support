#!/bin/bash
# Reconstruct one immutable historical archive; no upstream build is executed.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
package=uke-speaker-controls
bash "$root/packaging/ubuntu/prepare-archived-recipe.sh" "$package" "$work/view"
cd "$work"
git-build-recipe --safe --no-build --distribution tenacious "$work/view/$package.recipe" "$work/result"
archives=("$work/result/"*.orig.tar.xz)
[[ ${#archives[@]} == 1 ]]
[[ $(sha256sum "${archives[0]}" | cut -d ' ' -f1) == $(cat "$work/view/expected-orig.sha256") ]]
echo 'Exact historical orig archive recovered from saved tags and disposable local refs.'
