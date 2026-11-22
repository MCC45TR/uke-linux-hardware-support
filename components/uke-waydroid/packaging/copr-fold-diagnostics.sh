#!/usr/bin/bash
# COPR source worker. No native compilation, policy reload or device mutation.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
plan=$owner/uke-waydroid/packaging/diagnostics-fold.json
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/integration" "$stage/diagnostics"
for part in integration diagnostics; do
    url=$(jq -er ".$part.srpm_url" "$plan")
    hash=$(jq -er ".$part.srpm_sha256" "$plan")
    curl -qfsSL --retry 2 --max-time 120 --proto '=https' --proto-redir '=https' "$url" -o "$stage/$part.src.rpm"
    printf '%s  %s\n' "$hash" "$stage/$part.src.rpm" | sha256sum -c --strict -
    (cd "$stage/$part"; rpm2cpio "../$part.src.rpm" | cpio -idm --quiet --no-absolute-filenames)
    spec=$stage/$part/uke-waydroid-$part.spec
    archive=$stage/$part/uke-waydroid-$part-0.1.0.tar.xz
    printf '%s  %s\n' "$(jq -er ".$part.spec_sha256" "$plan")" "$spec" | sha256sum -c --strict -
    printf '%s  %s\n' "$(jq -er ".$part.archive_sha256" "$plan")" "$archive" | sha256sum -c --strict -
    install -m0644 "$archive" "$output/"
    install -m0644 "$spec" "$output/$part-parent.recipe"
done
node "$owner/uke-waydroid/packaging/fold-diagnostics-spec.mjs" \
    "$output/integration-parent.recipe" "$output/diagnostics-parent.recipe" "$output/uke-waydroid-integration.spec"
install -m0644 "$plan" "$output/diagnostics-fold.json"
printf 'Waydroid fold source: %s; both original source archives and recipes retained.\n' "$(git -C "$owner" rev-parse HEAD)"
sha256sum "$output/uke-waydroid-integration.spec" "$output/"*.tar.xz
