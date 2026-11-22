#!/usr/bin/env bash
set -Eeuo pipefail
export LC_ALL=C
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
mkdir -p .work out
stage=$(mktemp -d "$PWD/.work/rawhide.XXXXXXXX")
fedora=$(rpm --eval '%{fedora}')
[[ $fedora =~ ^[0-9]+$ ]]
printf '%%dist .fc%s\n' "$fedora" > "$HOME/.rpmmacros"
repo=(dnf5 --releasever=rawhide --repo=rawhide)
query() {
    local package=$1 format=$2
    "${repo[@]}" repoquery --arch=aarch64,noarch --latest-limit=1 \
        --queryformat="$format" "$package"
}
query mesa-dri-drivers '%{name}|%{epoch}|%{version}|%{release}|%{sourcerpm}\n' > "$stage/source"
mapfile -t rows < "$stage/source"
[[ ${#rows[@]} == 1 ]]
IFS='|' read -r binary epoch version fedora_release source <<< "${rows[0]}"
[[ $binary == mesa-dri-drivers && $epoch == 0 && $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ $fedora_release =~ ^[a-zA-Z0-9._+~^]+$ && $source == "mesa-$version-$fedora_release.src.rpm" ]]
if [[ $source == "$(jq -er .fedora_source state.json)" ]]; then
    build=$(jq -er '.build_id|select(type=="number" and .>0)' state.json)
    build_state=$(curl --fail --silent --show-error --proto '=https' --max-time 30 \
        "https://copr.fedorainfracloud.org/api_3/build/$build" | jq -er '.state')
    case "$build_state" in
        succeeded)
            runtime=$(jq -er '.runtime_id // 0' state.json)
            if ((runtime > 0)); then
                runtime_state=$(curl --fail --silent --show-error --proto '=https' --max-time 30 \
                    "https://copr.fedorainfracloud.org/api_3/build/$runtime" | jq -er '.state')
                case "$runtime_state" in
                    failed|canceled)
                        if [[ ${RETRY_FAILED:-false} != true ]]; then
                            echo "COPR runtime build $runtime ended in $runtime_state." >&2
                            exit 1
                        fi
                        ;;
                    *)
                        echo "Rawhide source already submitted: $source (COPR runtime $runtime, $runtime_state)"
                        exit 0
                        ;;
                esac
            fi
            bash scripts/consolidate.sh "$build" "$version" "$stage"
            exit 0
            ;;
        failed|canceled)
            if [[ ${RETRY_FAILED:-false} != true ]]; then
                echo "COPR build $build for $source ended in $build_state." >&2
                exit 1
            fi
            ;;
        *)
            echo "Rawhide source already submitted: $source (COPR $build, $build_state)"
            exit 0
            ;;
    esac
else
    old_evr=$(jq -er '.fedora_source|sub("^mesa-";"")|sub("\\.src\\.rpm$";"")' state.json)
    [[ $old_evr =~ ^[0-9A-Za-z._+~^-]+$ ]]
    [[ $(rpm --eval "%{lua: print(rpm.vercmp('$version-$fedora_release', '$old_evr'))}") == 1 ]]
fi

curl --fail --silent --show-error --proto '=https' --max-time 30 \
    'https://copr.fedorainfracloud.org/api_3/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&packagename=uke-mesa&limit=100' > "$stage/builds.json"
jq -e '.items|type=="array"' "$stage/builds.json" >/dev/null
if jq -e 'any(.items[]; .state|IN("pending","starting","importing","waiting","running","forked"))' "$stage/builds.json" >/dev/null; then
    echo 'COPR already has an active Mesa build; the next check will reconsider this source.'
    exit 0
fi
release=$(jq -r --arg v "$version" '
    [.items[].source_package.version // "" | select(startswith($v+"-")) |
        ltrimstr($v+"-") | split(".")[0] | select(test("^[0-9]+$")) | tonumber]
    | (max // 0) + 1' "$stage/builds.json")
[[ $release =~ ^[1-9][0-9]*$ ]]

mkdir "$stage/signed" "$stage/rpmdb"
dnf5 --releasever=rawhide --repo=rawhide-source download --source --destdir="$stage/signed" "mesa-$version-$fedora_release"
archive="$stage/signed/$source"
[[ -s $archive && $(rpm -qp --qf '%{NAME}|%{VERSION}|%{RELEASE}|%{SOURCEPACKAGE}' "$archive") == "mesa|$version|$fedora_release|1" ]]
rpm --dbpath "$stage/rpmdb" --initdb
[[ $fedora_release =~ \.fc([0-9]+)(\.|$) ]]
source_dist=${BASH_REMATCH[1]}
for number in $(printf '%s\n' "$source_dist" "$fedora" "$((fedora+1))" | sort -u); do
    key="/usr/share/pki/rpm-gpg/RPM-GPG-KEY-fedora-$number-primary"
    if [[ -s $key ]]; then rpmkeys --dbpath "$stage/rpmdb" --import "$key"; fi
done
rpmkeys --dbpath "$stage/rpmdb" --checksig --verbose "$archive" > "$stage/signature"
grep -Eqi '(signature|key fingerprint).*(OK|: OK)' "$stage/signature"
if grep -Eqi 'NOKEY|NOT OK|BAD|NOTTRUSTED' "$stage/signature"; then exit 1; fi
hash=$(sha256sum "$archive" | cut -d' ' -f1)
payload=$(rpm2cpio "$archive" | sha256sum | cut -d' ' -f1)

commit=$(<support-commit)
[[ $commit =~ ^[0-9a-f]{40}$ ]]
support="$stage/support"
git init --quiet "$support"
git -C "$support" remote add origin https://github.com/MCC45TR/uke-linux-hardware-support.git
git -C "$support" fetch --quiet --depth=1 origin "$commit"
git -C "$support" checkout --quiet --detach FETCH_HEAD
[[ $(git -C "$support" rev-parse HEAD) == "$commit" ]]
component="$support/components/uke-desktop-packaging"
manifest="$component/manifests/native-runtime.json"
cp patches/*.patch "$component/packaging/mesa/"
mkdir -p "$component/references/fedora-srpms"
mv "$archive" "$component/references/fedora-srpms/$source"
jq --arg v "$version" --arg r "$fedora_release" --arg rev "$release" --arg hash "$hash" --arg payload "$payload" '
    .packages |= map(if .name=="mesa" then .version=$v | .release=$r | .native_release=$rev |
        .sha256=$hash | .signed_mirror_sha256=$hash | .source_payload_sha256=$payload else . end)
' "$manifest" > "$stage/native-runtime.json"
mv "$stage/native-runtime.json" "$manifest"
jq --arg v "$version" --arg r "$release.fc$fedora" '
    .native_versions.mesa=$v | .release_overrides.mesa=$r
' "$support/packaging/families/families.json" > "$stage/families.json"
mv "$stage/families.json" "$support/packaging/families/families.json"
# Fedora's official Rawhide container uses development release metadata.
# Accept that identity in the two pinned recipe entry points.
for recipe in "$support"/packaging/families/host/{standalone,compose}.sh; do
    [[ $(grep -Fc ' /etc/os-release || ' "$recipe") == 1 ]]
    sed -i 's@ /etc/os-release || @ /etc/os-release || { grep -Fxq ID=fedora /etc/os-release \&\& grep -Fxq RELEASE_TYPE=development /etc/os-release; } || @' "$recipe"
done
# Refresh the exact pins already used by the reviewed source adapter.
jq -r '(.build_tools+(.packages[]|select(.name=="mesa")|.host_tools // {}))|keys[]' "$manifest" > "$stage/tools"
while IFS= read -r package; do
    evr=$(query "$package" '%{version}-%{release}')
    [[ $evr =~ ^[0-9a-zA-Z._+~^-]+$ ]]
    jq --arg tool "$package" --arg evr "$evr" '
        if .build_tools|has($tool) then .build_tools[$tool]=$evr
        else .packages |= map(if .name=="mesa" then .host_tools[$tool]=$evr else . end) end
    ' "$manifest" > "$stage/native-runtime.json"
    mv "$stage/native-runtime.json" "$manifest"
done < "$stage/tools"

bash "$support/packaging/families/host/standalone.sh" mesa "$PWD/out"
srpm="out/uke-mesa-$version-$release.fc$fedora.src.rpm"
[[ -s $srpm && $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$srpm") == "uke-mesa-$version-$release.fc$fedora" ]]
jq -n --arg source "$source" --arg version "$version" --arg path "$srpm" --arg sha "$hash" \
    '{fedora_source:$source,version:$version,srpm:$path,fedora_srpm_sha256:$sha,kind:"source"}' > out/submission.json
