#!/bin/bash
# Source-stage existing data delivery, not a new Android or Ubuntu image build.
cache=$root/uke-orangefox-packaging/referances/releases/R12.0-uke-modded-v003
mkdir "$stage/release"
while IFS=$'\t' read -r name hash bytes; do
 [[ $name =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]
 printf '%s  %s\n' "$hash" "$cache/$name" | sha256sum -c -
 [[ $(stat -c %s "$cache/$name") == "$bytes" ]]
 cp "$cache/$name" "$stage/release/"
done < <(jq -r '.assets[]|[.name,.sha256,.bytes]|@tsv' "$stage/manifests/release-lock.json")
cp "$stage/manifests/release-lock.json" "$stage/release/"
cp "$stage/docs/DELIVERY.md" "$stage/docs/SOURCE-OFFER.md" "$stage/LICENSE" "$stage/release/"
cp -a "$stage/licenses" "$stage/release/"
jq -e '.device=="uke" and .firmware_profile=="global-os3.0.303.0" and .validation.no_python_payload==true and .validation.payload_privacy==true and .validation.physical_device==false' "$stage/release/ARTIFACT-MANIFEST.json" >/dev/null
[[ $(head -c8 "$stage/release/orangefox-r12-uke-modded-v003.img") == 'ANDROID!' ]]
(cd "$stage/release"; find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/PACKAGE-SHA256SUMS"
mv "$stage/PACKAGE-SHA256SUMS" "$stage/release/"
