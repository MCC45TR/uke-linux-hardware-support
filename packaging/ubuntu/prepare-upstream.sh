#!/bin/bash
# Sourced by prepare-source.sh after common sources have been staged.
upstream=$(awk -F '\t' -v p="$pkg" '$1==p' "$root/packaging/ubuntu/upstreams.tsv")
if [[ -n $upstream ]]; then
 IFS=$'\t' read -r unused archive url hash patchset <<< "$upstream"
 [[ -r $root/$archive ]] || { echo "Missing pinned source: $archive; fetch $url and verify $hash" >&2; exit 2; }
 printf '%s  %s\n' "$hash" "$root/$archive" | sha256sum -c -
 # Reject absolute paths and traversal before extracting the upstream archive.
 tar -tf "$root/$archive" > "$out/archive-members"
 if grep -Eq '(^/|(^|/)\.\.(/|$))' "$out/archive-members"; then exit 2; fi
 # The exact hash is reviewed; upstream code is read but never executed here.
 mv "$stage" "$out/common-source"
 mkdir "$stage"
 tar -xf "$root/$archive" -C "$stage" --strip-components=1
 mv "$out/common-source" "$stage/uke"
 # Preserve any existing upstream Debian recipe as review evidence, not active packaging.
 if [[ -d $stage/debian ]]; then mv "$stage/debian" "$stage/uke/upstream-debian"; fi
 printf '%s\n' "Upstream: $url" "SHA256: $hash" > "$stage/UPSTREAM-ORIGIN"
fi
