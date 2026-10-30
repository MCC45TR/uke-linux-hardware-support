#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
checker="$root/host/is-rawhide.sh"
printf 'ID=fedora\nVERSION_ID=46\nVERSION="46 (Rawhide Prerelease)"\nRELEASE_TYPE=development\nREDHAT_BUGZILLA_PRODUCT_VERSION=rawhide\n' > "$stage/valid"
bash "$checker" "$stage/valid" 46
for legacy in 'VERSION="Rawhide"' 'VERSION_ID=rawhide'; do
    printf 'ID=fedora\n%s\n' "$legacy" > "$stage/legacy"
    bash "$checker" "$stage/legacy" 46
done
for pattern in 'ID=fedora' 'RELEASE_TYPE=development' 'REDHAT_BUGZILLA_PRODUCT_VERSION=rawhide'; do
    grep -Fvx "$pattern" "$stage/valid" > "$stage/invalid"
    if bash "$checker" "$stage/invalid" 46; then exit 1; fi
done
if bash "$checker" "$stage/valid" 45; then exit 1; fi
printf 'ID=fedora\nVERSION_ID=46\nVERSION="46 (Workstation Edition)"\nRELEASE_TYPE=stable\n' > "$stage/invalid"
if bash "$checker" "$stage/invalid" 46; then exit 1; fi
printf '%s\n' 'PASS: legacy and explicit Rawhide prerelease workers; stable, incomplete and mismatched identities refused.'
