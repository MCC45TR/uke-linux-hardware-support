#!/usr/bin/env bash
set -Eeuo pipefail

build=$1 version=$2 stage=$3
copr=https://copr.fedorainfracloud.org/api_3
download=https://download.copr.fedorainfracloud.org/results/mcc45tr
runtime_project=uke-linux-unstable
build_info=$(curl -fsS --proto '=https' --max-time 30 "$copr/build/$build")
build_project=$(jq -er '.project_dirname' <<< "$build_info")
case "$build_project" in
    "$runtime_project"|"$runtime_project:custom:mesa-rawhide") ;;
    *) exit 1 ;;
esac
jq -e --arg version "$version" --argjson id "$build" '
    .id==$id and .state=="succeeded" and .ownername=="mcc45tr" and
    .source_package.name=="uke-mesa" and (.source_package.version|startswith($version+"-"))
' <<< "$build_info" >/dev/null
source_evr=$(jq -er '.source_package.version' <<< "$build_info")
[[ $source_evr =~ ^[0-9.]+-([0-9]+)\.fc([0-9]+)$ ]]
runtime_release=${BASH_REMATCH[1]}
fedora=${BASH_REMATCH[2]}

# The first source-only trial was submitted to the runtime repository.
# Keep its NVR unique when publishing the combined package.
if [[ $build_project == "$runtime_project" || $(jq -r '.runtime_id // 0' state.json) != 0 ]]; then
    runtime_release=$((runtime_release+1))
    releases=$(curl -fsS --proto '=https' --max-time 30 \
        "$copr/build/list?ownername=mcc45tr&projectname=$runtime_project&packagename=uke-mesa&limit=100")
    latest=$(jq -r --arg v "$version" --arg project "$runtime_project" '
        [.items[] | select(.project_dirname==$project) | .source_package.version // "" |
         select(startswith($v+"-")) | ltrimstr($v+"-") | split(".")[0] |
         select(test("^[0-9]+$")) | tonumber] | max // 0
    ' <<< "$releases")
    if ((latest >= runtime_release)); then runtime_release=$((latest+1)); fi
fi

input=$stage/bundle-inputs
mkdir -p "$input/keydb"
fetch() {
    curl --fail --location --silent --show-error --proto '=https' --proto-redir '=https' \
        --retry 2 --max-time 80 --max-filesize 100000000 -o "$input/$1" "$2"
}
fetch copr.asc "$download/$runtime_project/pubkey.gpg"
build_dir="$download/$build_project/fedora-rawhide-aarch64/$build-uke-mesa"
for child in dri-drivers filesystem libEGL libGL libOpenCL libTeflon libgbm vulkan-drivers \
    libEGL-devel libGL-devel libgbm-devel libOpenCL-devel; do
    name=uke-mesa-$child-$source_evr.aarch64.rpm
    fetch "$name" "$build_dir/$name"
done
builtins=uke-mesa-libclc22-spirv-0.2.0-7.fc46.aarch64.rpm
fetch "$builtins" "https://packages.redhat.com/api/pulp-content/public-copr/mcc45tr/$runtime_project/fedora-rawhide-aarch64/Packages/u/$builtins"
printf '%s  %s\n' 8f9e1287f4cca65d0abfc75c076698937ae019f1234de747fd36d732551a4abc "$input/$builtins" |
    sha256sum --check

release=$runtime_release.fc$fedora
bash scripts/render-bundle.sh "$input" "$version" "$release" "$stage/rpm"
srpm=out/uke-mesa-$version-$release.src.rpm
install -m644 "$stage/rpm/SRPMS/${srpm##*/}" "$srpm"
jq --arg srpm "$srpm" '. + {srpm:$srpm,kind:"runtime"}' state.json > out/submission.json
