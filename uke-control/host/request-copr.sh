#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host CI only. Run after validation; never ship or invoke on the tablet.
set -Eeuo pipefail
before=${UKE_CONTROL_BEFORE:?Missing previous commit}
commit=${UKE_CONTROL_COMMIT:?Missing validated commit}
[[ $before =~ ^[a-f0-9]{40}$ && $commit =~ ^[a-f0-9]{40}$ ]]
[[ $(git rev-parse HEAD) == "$commit" ]]
spec=uke-control/packaging/uke-control.spec
empty=$(git hash-object -t tree /dev/null)
base=$before
[[ $before != 0000000000000000000000000000000000000000 ]] || base=$empty
if git diff --quiet "$base" "$commit" -- uke-control .copr packaging/families/host; then
    printf '%s\n' 'No Uke Control source change; no build requested.'
    exit 0
fi
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
git show "$commit:$spec" >"$stage/new.spec"
version=$(rpmspec --undefine dist -q --srpm --qf '%{VERSION}-%{RELEASE}' "$stage/new.spec")
[[ $version =~ ^[a-zA-Z0-9._+~^-]+$ ]]
if git cat-file -e "$commit:packaging/families/families.json" 2>/dev/null; then
    git show "$commit:packaging/families/families.json" >"$stage/families.json"
    family_version=$(jq -er '.families[]|select(.name=="uke-core")|.version+"-"+((.release // 1)|tostring)' "$stage/families.json")
    [[ $version == "$family_version" ]] || { printf '%s\n' 'Control and its source family must have matching version/release.' >&2; exit 1; }
fi
old=
if git cat-file -e "$base:$spec" 2>/dev/null; then
    git show "$base:$spec" >"$stage/old.spec"
    old=$(rpmspec --undefine dist -q --srpm --qf '%{VERSION}-%{RELEASE}' "$stage/old.spec")
fi
if [[ $version == "$old" ]]; then
    if ! git diff --quiet "$base" "$commit" -- uke-control/src uke-control/shared uke-control/packaging uke-control/Makefile .copr packaging/families/host; then
        printf '%s\n' 'Package inputs changed without a version/release bump; refusing a same-NEVRA rebuild.' >&2
        exit 1
    fi
    printf '%s\n' 'Only documentation or tests changed; no new package release.'
    exit 0
fi
if [[ -n $old ]]; then
    [[ $old =~ ^[a-zA-Z0-9._+~^-]+$ ]]
    comparison=$(rpm --eval "%{lua: print(rpm.vercmp('$version', '$old'))}")
    [[ $comparison == 1 ]] || { printf '%s\n' 'Published package revisions must increase, including source reverts.' >&2; exit 1; }
fi
git fetch --quiet origin main
paths=(uke-control .copr)
if [[ -e $stage/families.json ]]; then paths+=(packaging/families); fi
for path in "${paths[@]}"; do
    if [[ $(git rev-parse "$commit:$path") != "$(git rev-parse "origin/main:$path")" ]]; then
        printf '%s\n' 'The package source advanced after this validation; let its newer validation submit it.'
        exit 0
    fi
done
url='https://copr.fedorainfracloud.org/api_3/build/list?ownername=mcc45tr&projectname=uke-linux-test&packagename=uke-core&limit=100'
curl --fail --silent --show-error --max-time 30 "$url" >"$stage/builds.json"
jq -e '.items|type=="array"' "$stage/builds.json" >/dev/null
if jq -e --arg version "$version" '
    any(.items[];
        (.state as $s | ["running","pending","starting","importing","waiting"] | index($s)) != null
        or ((.state == "succeeded" or .state == "forked") and
            ((.source_package.version // "") == $version or
             ((.source_package.version // "") | startswith($version + ".")))))
    ' "$stage/builds.json" >/dev/null; then
    printf '%s\n' 'An app build is active or this release is already published; no duplicate requested.'
    exit 0
fi
hook=${COPR_SUPPORT_BUILD_HOOK_BASE:?Missing existing project build hook}
[[ $hook == https://copr.fedorainfracloud.org/webhooks/custom/* ]]
# A failed POST may already have allocated a build. Never retry it automatically.
if ! response=$(curl --fail --silent --show-error --proto '=https' --max-time 45 -X POST "${hook%/}/uke-core/"); then
    printf '%s\n' 'Build request outcome is unknown; inspect COPR before retrying validation.' >&2
    exit 1
fi
[[ $response =~ ^[0-9]+$ ]] || { printf '%s\n' 'Unexpected build receipt; inspect COPR before retrying.' >&2; exit 1; }
printf 'Requested uke-core %s as COPR build %s after validation.\n' "$version" "$response"
