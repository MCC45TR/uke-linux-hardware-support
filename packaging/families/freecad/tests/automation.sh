#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Synthetic HTTP/validation fixtures. No release publication or real COPR POST.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/bin" "$stage/base/packaging/families/freecad/"{host,tests}
base="$stage/base/packaging/families/freecad"
install -m644 "$root/"{Makefile,freecad.spec,source-lock.json} "$base/"
install -m644 "$root/host/"*.sh "$base/host/"
install -m644 "$root/tests/source-contracts.sh" "$base/tests/"
install -m644 "$root/../families.json" "$stage/base/packaging/families/"
cat >"$stage/bin/curl" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
url=; out=
while (($#)); do
  case "$1" in
    -o) out=$2; shift 2;;
    --data-binary) cp "${2#@}" "$FREECAD_FIXTURE/post.json"; shift 2;;
    --max-time|--max-filesize|--proto|--proto-redir|--netrc-file|-H) shift 2;;
    https:*) url=$1; shift;;
    -*) shift;;
    *) exit 99;;
  esac
done
case "$url" in
  https://api.github.com/repos/FreeCAD/FreeCAD/releases/latest) cp "$FREECAD_FIXTURE/release.json" "$out";;
  https://api.github.com/repos/FreeCAD/FreeCAD/git/ref/tags/*) cp "$FREECAD_FIXTURE/tag.json" "$out";;
  https://github.com/FreeCAD/FreeCAD/releases/download/*-SHA256.txt) cp "$FREECAD_FIXTURE/checksum" "$out";;
  https://github.com/FreeCAD/FreeCAD/releases/download/*.tar.gz) cp "$FREECAD_FIXTURE/archive.tar.gz" "$out";;
  https://copr.fedorainfracloud.org/api_3/project\?*) cp "$FREECAD_FIXTURE/project.json" "$out";;
  https://copr.fedorainfracloud.org/api_3/build/list\?*) cp "$FREECAD_FIXTURE/builds.json" "$out";;
  https://copr.fedorainfracloud.org/api_3/build/source-build-config/*) cp "$FREECAD_FIXTURE/source.json" "$out";;
  https://copr.fedorainfracloud.org/api_3/build/create/scm)
    printf 'POST\n' >>"$FREECAD_FIXTURE/posts"
    [[ ${FIXTURE_POST_FAIL:-0} == 0 ]] || exit 28
    printf '{"id":42}\n' >"$out";;
  *) printf 'Unexpected fixture URL: %s\n' "$url" >&2; exit 99;;
esac
MOCK
cat >"$stage/bin/make" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
[[ $1 == -C && $3 == srpm ]]
bash "$2/tests/source-contracts.sh"
printf 'source-validation\n' >>"$FREECAD_FIXTURE/checks"
MOCK
cat >"$stage/bin/rpmbuild" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
[[ $1 == -bp ]]
printf 'source-preparation\n' >>"$FREECAD_FIXTURE/checks"
[[ ${FIXTURE_PREP_FAIL:-0} == 0 ]]
MOCK
cat >"$stage/bin/dnf5" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
printf 'dependency-solver\n' >>"$FREECAD_FIXTURE/checks"
if [[ ${FIXTURE_SOLVER_FAIL:-0} == 0 ]]; then
  printf 'Transaction Summary:\n Installing: 1 package\nOperation aborted by the user.\n'
else
  printf 'Error: unavailable dependency\n'
fi
exit 1
MOCK
cat >"$stage/bin/git" <<'MOCK'
#!/usr/bin/env bash
set -Eeuo pipefail
case "$*" in
  'rev-parse HEAD') printf '%s\n' aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;;
  'ls-remote origin refs/heads/main') printf '%s\trefs/heads/main\n' aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;;
  'show -s --format=%ct '*) printf '1793700000\n';;
  'cat-file -e '*) exit 0;;
  'diff --quiet '*) [[ ${FIXTURE_SOURCE_CHANGED:-0} == 0 ]];;
  *) exit 99;;
esac
MOCK
chmod +x "$stage/bin/"*
fixture() {
  local name=$1 version=${2:-1.1.5}
  export FREECAD_FIXTURE="$stage/$name"
  mkdir -p "$FREECAD_FIXTURE"
  cp -a "$stage/base/." "$FREECAD_FIXTURE/repo/"
  family="$FREECAD_FIXTURE/repo/packaging/families/freecad"
  mkdir "$FREECAD_FIXTURE/archive"
  printf '# Synthetic source fixture\n' >"$FREECAD_FIXTURE/archive/CMakeLists.txt"
  printf 'Synthetic test fixture; not a published application.\n' >"$FREECAD_FIXTURE/archive/LICENSE"
  tar -czf "$FREECAD_FIXTURE/archive.tar.gz" -C "$FREECAD_FIXTURE/archive" CMakeLists.txt LICENSE
  hash=$(sha256sum "$FREECAD_FIXTURE/archive.tar.gz" | cut -d ' ' -f1)
  commit=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
  if [[ $version == 1.1.4 ]]; then
    hash=$(jq -r '.inputs[]|select(.filename=="freecad_source_1.1.4.tar.gz")|.sha256' "$family/source-lock.json")
    commit=$(jq -r .upstream_commit "$family/source-lock.json")
  fi
  printf '%s  freecad_source_%s.tar.gz\n' "$hash" "$version" >"$FREECAD_FIXTURE/checksum"
  jq -n --arg v "$version" --arg sha "$hash" '
    ("https://github.com/FreeCAD/FreeCAD/releases/download/"+$v+"/freecad_source_"+$v+".tar.gz") as $url |
    {id:1,tag_name:$v,draft:false,prerelease:false,published_at:"2026-11-03T00:00:00Z",
     html_url:("https://github.com/FreeCAD/FreeCAD/releases/tag/"+$v),
     assets:[{browser_download_url:$url,digest:("sha256:"+$sha)},
             {browser_download_url:($url+"-SHA256.txt")}]} ' >"$FREECAD_FIXTURE/release.json"
  jq -n --arg sha "$commit" '{object:{type:"commit",sha:$sha}}' >"$FREECAD_FIXTURE/tag.json"
  printf '{"items":[]}\n' >"$FREECAD_FIXTURE/builds.json"
  printf '{"ownername":"mcc45tr","name":"uke-linux-unstable","enable_net":false,"additional_repos":[],"chroot_repos":{"fedora-rawhide-aarch64":"fixture"}}\n' >"$FREECAD_FIXTURE/project.json"
  printf '{"source_type":"scm","source_dict":{"clone_url":"https://github.com/MCC45TR/uke-linux-hardware-support.git","subdirectory":"packaging/families/freecad","committish":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}}\n' >"$FREECAD_FIXTURE/source.json"
  (cd "$family"; sha256sum freecad.spec source-lock.json) >"$FREECAD_FIXTURE/original.sha256"
}
run_tracker() {
  PATH="$stage/bin:$PATH" bash "$family/host/track-stable.sh" >"$FREECAD_FIXTURE/log" 2>&1
}
unchanged() { (cd "$family"; sha256sum --check "$FREECAD_FIXTURE/original.sha256") >/dev/null; }
fixture unchanged 1.1.4
run_tracker; unchanged
fixture regression 1.1.3
if run_tracker; then echo 'Regression accepted' >&2; exit 1; fi
unchanged
fixture moved_tag 1.1.4
jq '.object.sha="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"' "$FREECAD_FIXTURE/tag.json" >"$FREECAD_FIXTURE/tmp"; mv "$FREECAD_FIXTURE/tmp" "$FREECAD_FIXTURE/tag.json"
if run_tracker; then echo 'Moved stable tag accepted' >&2; exit 1; fi
unchanged
fixture reissued 1.1.4
printf '%064d  freecad_source_1.1.4.tar.gz\n' 0 >"$FREECAD_FIXTURE/checksum"
jq '.assets[0].digest="sha256:0000000000000000000000000000000000000000000000000000000000000000"' "$FREECAD_FIXTURE/release.json" >"$FREECAD_FIXTURE/tmp"; mv "$FREECAD_FIXTURE/tmp" "$FREECAD_FIXTURE/release.json"
if run_tracker; then echo 'Reissued stable source accepted' >&2; exit 1; fi
unchanged
fixture prerelease
jq '.prerelease=true' "$FREECAD_FIXTURE/release.json" >"$FREECAD_FIXTURE/tmp"; mv "$FREECAD_FIXTURE/tmp" "$FREECAD_FIXTURE/release.json"
if run_tracker; then echo 'Prerelease accepted' >&2; exit 1; fi
unchanged
fixture checksum
printf '%064d  freecad_source_1.1.5.tar.gz\n' 0 >"$FREECAD_FIXTURE/checksum"
if run_tracker; then echo 'Mismatched checksum accepted' >&2; exit 1; fi
unchanged
fixture preparation
if FIXTURE_PREP_FAIL=1 run_tracker; then echo 'Failed source preparation accepted' >&2; exit 1; fi
unchanged
fixture dependencies
if FIXTURE_SOLVER_FAIL=1 run_tracker; then echo 'Failed dependency solution accepted' >&2; exit 1; fi
unchanged
fixture update
run_tracker
jq -e --arg sha "$hash" '.version=="1.1.5" and .upstream_commit=="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" and any(.inputs[];.sha256==$sha)' "$family/source-lock.json" >/dev/null
[[ $(wc -l <"$FREECAD_FIXTURE/checks") == 3 ]]
run_dispatch() {
  PATH="$stage/bin:$PATH" COPR_FREECAD_LOGIN=fixture_login COPR_FREECAD_TOKEN=fixture_token \
    GITHUB_STEP_SUMMARY='' bash "$family/host/request-unstable.sh" aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa >"$FREECAD_FIXTURE/log" 2>&1
}
build_fixture() {
  local state=$1 version=${2:-1:1.1.4-1.fc46}
  jq -n --arg state "$state" --arg version "$version" '{items:[{id:7,ownername:"mcc45tr",projectname:"uke-linux-unstable",state:$state,submitted_on:1793600000,source_package:{name:"freecad",version:$version}}]}' >"$FREECAD_FIXTURE/builds.json"
}
fixture active 1.1.4
build_fixture running
run_dispatch; [[ ! -f $FREECAD_FIXTURE/posts ]]
fixture published 1.1.4
build_fixture succeeded
run_dispatch; [[ ! -f $FREECAD_FIXTURE/posts ]]
fixture attempted 1.1.4
build_fixture failed
if run_dispatch; then echo 'Failed commit was retried' >&2; exit 1; fi
[[ ! -f $FREECAD_FIXTURE/posts ]]
fixture request 1.1.4
run_dispatch
jq -e '.projectname=="uke-linux-unstable" and .committish=="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" and .chroots==["fedora-rawhide-aarch64"] and .source_build_method=="make_srpm"' "$FREECAD_FIXTURE/post.json" >/dev/null
[[ $(wc -l <"$FREECAD_FIXTURE/posts") == 1 ]]
fixture unchanged_inputs 1.1.4
build_fixture failed
jq '.source_dict.committish="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"' "$FREECAD_FIXTURE/source.json" >"$FREECAD_FIXTURE/tmp"; mv "$FREECAD_FIXTURE/tmp" "$FREECAD_FIXTURE/source.json"
if run_dispatch; then echo 'Unchanged failed source inputs were retried' >&2; exit 1; fi
[[ ! -f $FREECAD_FIXTURE/posts ]]
FIXTURE_SOURCE_CHANGED=1 run_dispatch
[[ $(wc -l <"$FREECAD_FIXTURE/posts") == 1 ]]
fixture uncertain 1.1.4
if FIXTURE_POST_FAIL=1 run_dispatch; then echo 'Uncertain request reported success' >&2; exit 1; fi
[[ $(wc -l <"$FREECAD_FIXTURE/posts") == 1 ]]
printf '%s\n' 'Synthetic FreeCAD updater and dispatch fixtures passed; no external requests were made.'
