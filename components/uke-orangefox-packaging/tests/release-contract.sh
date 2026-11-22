# Host-only source contract tests; never installed on the tablet.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
cp -r "$project/Makefile" "$project/manifests" "$project/packaging" "$tmp/"
make -s -C "$tmp" validate
for change in '.assets[0].name="../escape.img"' '.assets[0].sha256="invalid"' '.assets |= .[:-1]' '.source_snapshots[0].url="https://untrusted.example/source.tar.gz"'; do
    jq "$change" "$project/manifests/release-lock.json" > "$tmp/manifests/release-lock.json"
    if make -s -C "$tmp" validate > "$tmp/rejected.log" 2>&1; then
        printf 'Unsafe release contract accepted: %s\n' "$change" >&2
        exit 1
    fi
done
cp "$project/manifests/release-lock.json" "$tmp/manifests/"
printf '[{"draft":false,"prerelease":true,"tag_name":"R12.0-uke-modded-v003"}]\n' > "$tmp/previews.json"
before=$(sha256sum "$tmp/manifests/release-lock.json")
make -s -C "$tmp" track-stable releases_file="$tmp/previews.json"
test "$(sha256sum "$tmp/manifests/release-lock.json")" = "$before"
printf '%s\n' 'Valid preview pin, unsafe source rejection and stable-channel isolation passed.'
