#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host CI only. Submit one exact source commit to the unstable AArch64 chroot.
set -Eeuo pipefail
commit=${1:?Pass the validated full Git commit}
[[ $commit =~ ^[a-f0-9]{40}$ ]]
root=$(cd -- "$(dirname -- "$0")/../../../.."; pwd)
cd "$root"
[[ $(git rev-parse HEAD) == "$commit" ]]
family=packaging/families/freecad
bash "$family/tests/source-contracts.sh"
version=$(jq -er '(.epoch|tostring)+":"+.version+"-"+(.release|tostring)' "$family/source-lock.json")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
api=https://copr.fedorainfracloud.org/api_3
get() { curl --fail --silent --show-error --proto '=https' --max-time 45 "$1" -o "$2"; }
get "$api/project?ownername=mcc45tr&projectname=uke-linux-unstable" "$stage/project.json"
jq -e '.ownername=="mcc45tr" and .name=="uke-linux-unstable" and .enable_net==false and .additional_repos==[] and (.chroot_repos|has("fedora-rawhide-aarch64"))' "$stage/project.json" >/dev/null
get "$api/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&packagename=freecad&limit=100" "$stage/named.json"
get "$api/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&limit=100" "$stage/recent.json"
jq -s '{items:([.[].items[]]|unique_by(.id)|sort_by(.id)|reverse)}' "$stage/named.json" "$stage/recent.json" >"$stage/builds.json"
jq -e 'all(.items[]; .ownername=="mcc45tr" and .projectname=="uke-linux-unstable")' "$stage/builds.json" >/dev/null
if jq -e --arg v "$version" 'any(.items[]; .source_package.name=="freecad" and (.state|IN("succeeded","forked")) and ((.source_package.version==$v) or (.source_package.version|startswith($v+"."))))' "$stage/builds.json" >/dev/null; then
  echo 'This FreeCAD release is already published; no duplicate requested.'; exit 0
fi
mapfile -t ids < <(jq -r '.items[]|select(.source_package.name=="freecad" or .source_package.name==null)|.id' "$stage/builds.json")
for id in "${ids[@]}"; do
  [[ $id =~ ^[0-9]+$ ]]
  get "$api/build/source-build-config/$id" "$stage/source.json"
  if ! jq -e '.source_type=="scm" and .source_dict.clone_url=="https://github.com/MCC45TR/uke-linux-hardware-support.git" and .source_dict.subdirectory=="packaging/families/freecad"' "$stage/source.json" >/dev/null; then continue; fi
  state=$(jq -er --argjson id "$id" '.items[]|select(.id==$id)|.state' "$stage/builds.json")
  case "$state" in running|pending|starting|importing|waiting)
    printf 'FreeCAD build %s is active; no duplicate requested.\n' "$id"; exit 0;;
  esac
  if [[ $(jq -r .source_dict.committish "$stage/source.json") == "$commit" ]]; then
    printf 'Commit already attempted as build %s (%s); inspect it before retrying.\n' "$id" "$state" >&2; exit 1
  fi
done
# A full recent page may hide older source failures without package metadata.
if [[ $(jq '.items|length' "$stage/recent.json") == 100 ]]; then
  oldest=$(jq '[.items[].submitted_on]|min' "$stage/recent.json")
  [[ $(git show -s --format=%ct "$commit") -ge $oldest ]] || {
    echo 'Build history before this commit is truncated; inspect COPR before submitting.' >&2; exit 1;
  }
fi
[[ $(git ls-remote origin refs/heads/main | cut -f1) == "$commit" ]] || {
  echo 'Main advanced after validation; its next scheduled run must submit it.'; exit 0;
}
login=${COPR_FREECAD_LOGIN:?Configure the existing COPR API login in GitHub Actions}
token=${COPR_FREECAD_TOKEN:?Configure the existing COPR API token in GitHub Actions}
[[ $login =~ ^[a-zA-Z0-9._~-]+$ && $token =~ ^[a-zA-Z0-9._~-]+$ ]]
umask 077
printf 'machine copr.fedorainfracloud.org login %s password %s\n' "$login" "$token" >"$stage/auth.netrc"
jq -n --arg commit "$commit" '{ownername:"mcc45tr",projectname:"uke-linux-unstable",clone_url:"https://github.com/MCC45TR/uke-linux-hardware-support.git",committish:$commit,subdirectory:"packaging/families/freecad",spec:"freecad.spec",scm_type:"git",source_build_method:"make_srpm",chroots:["fedora-rawhide-aarch64"]}' >"$stage/request.json"
# One POST only. A timeout may have allocated a build; inspect before retrying.
if ! curl --fail --silent --show-error --proto '=https' --max-time 45 \
    --netrc-file "$stage/auth.netrc" -H 'Content-Type: application/json' \
    --data-binary @"$stage/request.json" "$api/build/create/scm" -o "$stage/receipt.json"; then
  echo 'COPR submission outcome is uncertain; inspect the project before another request.' >&2; exit 1
fi
id=$(jq -er '.id|select(type=="number")' "$stage/receipt.json")
printf 'Requested pinned FreeCAD commit %s as unstable build %s.\n' "$commit" "$id"
if [[ -n ${GITHUB_STEP_SUMMARY:-} ]]; then
  printf 'FreeCAD source commit %s: [COPR build %s](https://copr.fedorainfracloud.org/coprs/build/%s/). Native build and device results remain separate.\n' "$commit" "$id" "$id" >>"$GITHUB_STEP_SUMMARY"
fi
