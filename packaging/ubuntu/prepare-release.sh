#!/bin/bash
# Extract shared released source only, never RPM binaries or spec commands.
release=$(awk -F '\t' -v p="$pkg" '$1==p' "$root/packaging/ubuntu/release-sources.tsv")
if [[ -n $release ]]; then
 IFS=$'\t' read -r unused archive url hash member source_hash <<< "$release"
 printf '%s  %s\n' "$hash" "$root/$archive" | sha256sum -c -
 command -v bsdtar >/dev/null || { echo 'Install official libarchive-tools for source RPM extraction' >&2; exit 2; }
 # Select one exact source member: never extract RPM paths or run its spec.
 bsdtar -xOf "$root/$archive" "$member" > "$out/released-source.tar.xz"
 printf '%s  %s\n' "$source_hash" "$out/released-source.tar.xz" | sha256sum -c -
 tar -tf "$out/released-source.tar.xz" > "$out/released-members"
 if grep -Eq '(^/|(^|/)\.\.(/|$))' "$out/released-members"; then exit 2; fi
 mv "$stage" "$out/git-source-reference"
 mkdir "$stage"
 tar -xf "$out/released-source.tar.xz" -C "$stage" --strip-components=1
 printf '%s\n' "Source-RPM: $url" "Source-RPM-SHA256: $hash" "Source-member: $member" "Source-SHA256: $source_hash" > "$stage/RELEASE-ORIGIN"
fi
while IFS=$'\t' read -r package archive url hash member destination source_hash; do
 [[ $package == "$pkg" ]] || continue
 printf '%s  %s\n' "$hash" "$root/$archive" | sha256sum -c -
 [[ $destination != /* && $destination != *..* && $member != */* ]]
 command -v bsdtar >/dev/null || { echo 'Install official libarchive-tools' >&2; exit 2; }
 bsdtar -xOf "$root/$archive" "$member" > "$out/released-member"
 printf '%s  %s\n' "$source_hash" "$out/released-member" | sha256sum -c -
 [[ -d $(dirname "$stage/$destination") ]]
 cp "$out/released-member" "$stage/$destination"
 printf '%s\n' "Source-RPM: $url" "Source-RPM-SHA256: $hash" "Member: $member -> $destination" "SHA256: $source_hash" >> "$stage/RELEASE-ORIGIN"
done < "$root/packaging/ubuntu/release-overrides.tsv"
