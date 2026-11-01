#!/bin/bash
# Source-only contract checks; no upstream or target program executes.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "$0")/.."; pwd)
original=${1:?Original Fedora specification}
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
transform=$project/src/native-runtime/tailscale-dns-label.awk
release=$(jq -er .release "$project/manifests/tailscale-dns-label.json")
awk -v release="$release" -f "$transform" "$original" > "$stage/candidate.spec"
[[ $(grep -c '^%check$' "$stage/candidate.spec") == 1 ]]
grep -Fxq "Release:        $release%{?dist}" "$stage/candidate.spec"
grep -Fxq 'Requires: policycoreutils' "$stage/candidate.spec"
grep -Fq 'go test -mod=vendor -vet=off -timeout 120s' "$stage/candidate.spec"
grep -Fq 'published-missing-context' "$stage/candidate.spec"
for pattern in '^Version:' '^License:' '^Provides:' '^%systemd'; do
 cmp <(grep -E "$pattern" "$original") <(grep -E "$pattern" "$stage/candidate.spec")
done
cmp <(awk '/^%install$/{copy=1} /^# vendoring/{copy=0} copy' "$original") <(awk '/^%install$/{copy=1} /^# vendoring/{copy=0} copy' "$stage/candidate.spec")
cmp <(awk '/^%changelog$/{copy=1;next}copy' "$original") <(awk '/^%changelog$/{copy=1;next} copy{n++;if(n>4)print}' "$stage/candidate.spec")
for anchor in '^%autopatch -p1$' '^BuildRequires:[[:space:]]+systemd-rpm-macros$'; do
 awk -v anchor="$anchor" '$0 !~ anchor' "$original" > "$stage/missing.spec"
 if awk -v release="$release" -f "$transform" "$stage/missing.spec" > /dev/null; then exit 1; fi
done
awk '{print} /^Source10:/{print}' "$original" > "$stage/duplicate.spec"
if awk -v release="$release" -f "$transform" "$stage/duplicate.spec" > /dev/null; then exit 1; fi
printf 'Original versions/licenses/provides/lifecycle/install/history retained; focused checks and missing/duplicate source-anchor refusals pass.\n'
