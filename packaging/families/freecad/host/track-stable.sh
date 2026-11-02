#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host only. Update source pins only after provenance, preparation and SAT checks.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
cd "$root"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
fetch() {
  curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' \
    --max-time 300 --max-filesize 1073741824 "$1" -o "$2"
}
bash tests/source-contracts.sh
fetch https://api.github.com/repos/FreeCAD/FreeCAD/releases/latest "$stage/release.json"
jq -e '.draft==false and .prerelease==false and (.tag_name|test("^[0-9]+[.][0-9]+[.][0-9]+$")) and (.id|type=="number")' "$stage/release.json" >/dev/null
version=$(jq -er .tag_name "$stage/release.json")
jq -e --arg version "$version" '.html_url==("https://github.com/FreeCAD/FreeCAD/releases/tag/"+$version)' "$stage/release.json" >/dev/null
old=$(jq -er .version source-lock.json)
comparison=$(rpm --eval "%{lua: print(rpm.vercmp('$version', '$old'))}")
[[ $comparison != -1 ]] || { echo 'Refusing a stable-version regression.' >&2; exit 1; }
archive="freecad_source_$version.tar.gz"
url="https://github.com/FreeCAD/FreeCAD/releases/download/$version/$archive"
checksum_url="$url-SHA256.txt"
jq -e --arg url "$url" --arg checksum "$checksum_url" '
  ([.assets[]|select(.browser_download_url==$url)]|length)==1 and
  ([.assets[]|select(.browser_download_url==$checksum)]|length)==1
' "$stage/release.json" >/dev/null
digest=$(jq -er --arg url "$url" '.assets[]|select(.browser_download_url==$url)|.digest|select(test("^sha256:[a-f0-9]{64}$"))|sub("^sha256:";"")' "$stage/release.json")
fetch "$checksum_url" "$stage/upstream.sha256"
expected=$(awk -v name="$archive" 'NF==2 {gsub(/\r/, "", $2); sub(/^\*/, "", $2); if ($2==name) print $1}' "$stage/upstream.sha256")
[[ $expected =~ ^[a-f0-9]{64}$ && $expected == "$digest" ]] || { echo 'Published source checksums disagree.' >&2; exit 1; }
fetch "https://api.github.com/repos/FreeCAD/FreeCAD/git/ref/tags/$version" "$stage/tag.json"
for _ in 1 2 3; do
  kind=$(jq -er .object.type "$stage/tag.json")
  commit=$(jq -er '.object.sha|select(test("^[a-f0-9]{40}$"))' "$stage/tag.json")
  [[ $kind != commit ]] || break
  [[ $kind == tag ]] || { echo 'Unsupported release-tag object.' >&2; exit 1; }
  fetch "https://api.github.com/repos/FreeCAD/FreeCAD/git/tags/$commit" "$stage/tag.json.next"
  mv "$stage/tag.json.next" "$stage/tag.json"
done
[[ $kind == commit ]]
if [[ $comparison == 0 ]]; then
  [[ $commit == "$(jq -er .upstream_commit source-lock.json)" ]] || {
    echo 'The stable tag moved without a version change; manual review required.' >&2; exit 1;
  }
  [[ $expected == "$(jq -er --arg url "$url" '.inputs[]|select(.url==$url)|.sha256' source-lock.json)" ]] || {
    echo 'Source was reissued at an unchanged version; manual review required.' >&2; exit 1;
  }
  printf 'Latest stable FreeCAD %s is unchanged; verified tag and both published checksums.\n' "$version"
  exit 0
fi
fetch "$url" "$stage/$archive"
printf '%s  %s\n' "$expected" "$stage/$archive" | sha256sum --check -
tar -tzf "$stage/$archive" >"$stage/archive-files"
if grep -Eq '(^/|(^|/)[.][.](/|$))' "$stage/archive-files"; then
  echo 'Refusing an archive with unsafe paths.' >&2; exit 1
fi
grep -Fx CMakeLists.txt "$stage/archive-files" >/dev/null
grep -Fx LICENSE "$stage/archive-files" >/dev/null
published=$(jq -er '.published_at|select(test("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$"))' "$stage/release.json")
epoch=$(date -u -d "$published" +%s)
candidate="$stage/family"
mkdir -p "$candidate/tests" "$candidate/referances"
install -m644 Makefile freecad.spec source-lock.json "$candidate/"
install -m644 tests/source-contracts.sh "$candidate/tests/"
install -m644 ../families.json "$stage/families.json"
install -m644 "$stage/$archive" "$candidate/referances/$archive"
sed -i -E "s/^Version:.*/Version:        $version/; s/^Release:.*/Release:        1%{?dist}/" "$candidate/freecad.spec"
day=$(LC_ALL=C date -u -d "$published" '+%a %b %d %Y')
awk -v day="$day" -v version="$version" '
  { print }
  $0=="%changelog" {
    print "* " day " github-actions[bot] <41898282+github-actions[bot]@users.noreply.github.com> - 1:" version "-1"
    print "- Update to the verified upstream stable release.\n"
  }
' "$candidate/freecad.spec" >"$stage/new.spec"
mv "$stage/new.spec" "$candidate/freecad.spec"
jq --arg version "$version" --arg commit "$commit" --arg archive "$archive" \
   --arg url "$url" --arg checksum_url "$checksum_url" --arg sha "$expected" \
   --argjson epoch "$epoch" --slurpfile upstream "$stage/release.json" '
  .version=$version | .release=1 | .upstream_commit=$commit | .source_date_epoch=$epoch |
  .upstream_release=$upstream[0].html_url |
  .release_id=$upstream[0].id |
  .inputs |= map(if (.filename|startswith("freecad_source_")) then
    .filename=$archive | .url=$url | .sha256=$sha | .upstream_checksum_url=$checksum_url
    else . end)
' source-lock.json >"$candidate/source-lock.json"
make -C "$candidate" srpm outdir="$stage/srpm"
rpmbuild -bp --nodeps --target aarch64 --define "_topdir $stage/srpm/rpmbuild" "$candidate/freecad.spec"
mapfile -t requires < <(rpmspec --target aarch64 --query --buildrequires "$candidate/freecad.spec")
[[ ${#requires[@]} -gt 0 ]]
if LC_ALL=C dnf5 --forcearch=aarch64 --installroot="$stage/closure" --use-host-config \
    --releasever=rawhide --disable-repo='*' --enable-repo=rawhide \
    --setopt=install_weak_deps=False --assumeno install "${requires[@]}" >"$stage/solve.log" 2>&1; then
  echo 'Unexpected empty dependency transaction.' >&2; exit 1
else
  status=$?
fi
if [[ $status == 1 ]] && grep -Fx 'Transaction Summary:' "$stage/solve.log" >/dev/null &&
    grep -Fx 'Operation aborted by the user.' "$stage/solve.log" >/dev/null; then
  :
else
  cat "$stage/solve.log" >&2; echo 'Rawhide AArch64 dependency resolution failed.' >&2; exit 1
fi
# Source checks above run on isolated copies. Nothing is published on a failure.
install -m644 "$candidate/freecad.spec" freecad.spec
install -m644 "$candidate/source-lock.json" source-lock.json
bash tests/source-contracts.sh
printf 'Verified stable source update: FreeCAD %s, upstream commit %s.\n' "$version" "$commit"
