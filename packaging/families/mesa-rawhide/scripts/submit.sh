#!/usr/bin/env bash
set -Eeuo pipefail
cd -- "$(dirname -- "$0")/.."
[[ -s out/submission.json ]] || exit 0
[[ ${GITHUB_REPOSITORY:?} == MCC45TR/uke-linux-hardware-support ]]
[[ ${GITHUB_REF:?} == refs/heads/main ]]
git config --global --add safe.directory "${GITHUB_WORKSPACE:?}"
state_url="https://api.github.com/repos/$GITHUB_REPOSITORY/contents/packaging/families/mesa-rawhide/state.json"
github() {
    curl --fail --silent --show-error --proto '=https' --max-time 45 \
        -H "Authorization: Bearer ${GH_TOKEN:?GitHub workflow token is missing}" \
        -H 'Accept: application/vnd.github+json' -H 'X-GitHub-Api-Version: 2022-11-28' "$@"
}
state_sha=$(github "$state_url?ref=main" | jq -er .sha)
[[ $state_sha == "$(git hash-object state.json)" ]]
srpm=$(jq -er .srpm out/submission.json)
[[ $srpm =~ ^out/uke-mesa-[0-9.]+-[0-9]+\.fc[0-9]+\.src\.rpm$ && -s $srpm ]]
case $(jq -er .kind out/submission.json) in
    source) project=mcc45tr/uke-linux-unstable:custom:mesa-rawhide; field=build_id ;;
    runtime) project=mcc45tr/uke-linux-unstable; field=runtime_id ;;
    *) exit 1 ;;
esac
umask 077
config=$(mktemp)
trap 'unlink "$config"' EXIT
printf '%s\n' "${COPR_CONFIG:?COPR repository secret is missing}" > "$config"
unset COPR_CONFIG
copr-cli --config "$config" build --nowait --chroot fedora-rawhide-aarch64 \
    "$project" "$srpm" | tee out/copr-submission.log
build=$(sed -n 's/^Created builds: \([0-9][0-9]*\)$/\1/p' out/copr-submission.log)
[[ $build =~ ^[0-9]+$ ]]
jq --arg field "$field" --argjson id "$build" 'del(.srpm,.kind)|.[$field]=$id' out/submission.json > out/next-state.json
jq -n --arg sha "$state_sha" --arg content "$(base64 -w0 out/next-state.json)" \
    --arg message "Track Fedora Mesa source submitted to COPR $build" \
    '{branch:"main",sha:$sha,content:$content,message:$message}' > out/github-state.json
github --request PUT --json @out/github-state.json "$state_url" | jq -er '.commit.sha'
