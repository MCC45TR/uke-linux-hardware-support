#!/bin/bash
# Recover an immutable historical recipe in a disposable local repository.
# No remote branch, credential, compiler, signing or upload action is involved.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
package=${1:?Usage: prepare-archived-recipe.sh PACKAGE NEW_OUTPUT_DIRECTORY}
output=${2:?new output directory required}
[[ $package =~ ^[a-z0-9][a-z0-9+.-]+$ && ! -e $output ]]
row=$(awk -F '\t' -v p="$package" '$1==p{print}' "$root/packaging/ubuntu/recipe-source-manifest.tsv")
[[ -n $row && $row != *$'\n'* ]]
IFS=$'\t' read -r name source_commit orig_sha metadata_commit <<< "$row"
for hash in "$source_commit" "$metadata_commit"; do [[ $hash =~ ^[0-9a-f]{40}$ ]]; done
[[ $orig_sha =~ ^[0-9a-f]{64}$ ]]
archive=refs/tags/archive/main-only-20261101
[[ $(git -C "$root" rev-parse "$archive/launchpad/$package") == "$source_commit" ]]
git -C "$root" merge-base --is-ancestor "$metadata_commit" "$archive/pristine-tar"
mkdir -p "$output"
output=$(realpath "$output")
git init -q --bare "$output/hub.git"
git -C "$output/hub.git" fetch -q --no-tags "$root" "$source_commit:refs/heads/launchpad/$package" "$metadata_commit:refs/heads/pristine-tar"
git -C "$output/hub.git" symbolic-ref HEAD "refs/heads/launchpad/$package"
printf '# git-build-recipe format 0.4 deb-version {debversion}+git{revtime}\n%s launchpad/%s\n' "$output/hub.git" "$package" > "$output/$package.recipe"
printf '%s\n' "$orig_sha" > "$output/expected-orig.sha256"
printf 'Historical pinned source view prepared locally for %s; not a canonical main daily recipe.\n' "$package"
