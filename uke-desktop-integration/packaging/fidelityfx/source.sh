#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Run only in the COPR source builder; return flat spec/source inputs to COPR.
set -Eeuo pipefail
out=$(realpath -m "${1:?COPR result directory}")
component=$(cd -- "$(dirname -- "$0")/../.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$out" "$stage/rpmbuild/SOURCES" "$stage/rpmbuild/SPECS"
acquire() {
    local url=$1 revision=$2 name=$3
    shift 3
    git -c core.hooksPath=/dev/null init -q "$stage/$name"
    git -C "$stage/$name" remote add origin "$url"
    git -C "$stage/$name" -c core.hooksPath=/dev/null fetch -q --depth=1 --filter=blob:none origin "$revision"
    [[ $(git -C "$stage/$name" rev-parse FETCH_HEAD) == "$revision" ]]
    git -C "$stage/$name" archive --format=tar --prefix="$name/" FETCH_HEAD "$@" |
        gzip -n > "$stage/rpmbuild/SOURCES/$name.tar.gz"
}
acquire https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git \
    c6efa6bf7f2027b3ec94f28578bb5965eabb9e55 fidelityfx-1.1.4 \
    LICENSE.txt sdk/CMakeLists.txt sdk/toolchain.cmake sdk/include sdk/src
acquire https://github.com/DethRaid/FidelityFX-SDK-Linux.git \
    5a86dac1e1fdea3dd9f132947c30e7fc08205e9c fidelityfx-native-compiler \
    LICENSE.txt sdk/tools/ffx_shader_compiler/src \
    sdk/tools/ffx_shader_compiler/libs/MD5 \
    sdk/tools/ffx_shader_compiler/libs/SPIRV-Reflect \
    sdk/tools/ffx_shader_compiler/libs/tiny-process-library
(cd "$stage/rpmbuild/SOURCES" && sha256sum -c - <<'DIGESTS'
93b69cfc82960a7982c46ff16d3d52c5f2056fda5befcf751b83d65b40e9d8a2  fidelityfx-1.1.4.tar.gz
d7611f22601cae1ffb7e5352629be12c5ff4c1905785277017714d7f098511b0  fidelityfx-native-compiler.tar.gz
DIGESTS
)
mkdir -p "$stage/uke-fidelityfx-1.1.4"
cp -a "$component/src/fsr" "$stage/uke-fidelityfx-1.1.4/src"
cp -a "$component/patches/fsr" "$stage/uke-fidelityfx-1.1.4/patches"
cp -a "$component/packaging/fidelityfx/gpu-test.sh" "$stage/uke-fidelityfx-1.1.4/"
cp -a "$component/docs/FIDELITYFX.md" "$stage/uke-fidelityfx-1.1.4/"
cp -a "$component/manifests/fsr-research-20261029.json" "$stage/uke-fidelityfx-1.1.4/"
tar -C "$stage" --sort=name --mtime=@1793232000 --owner=0 --group=0 --numeric-owner \
    -cf - uke-fidelityfx-1.1.4 | gzip -n > "$stage/rpmbuild/SOURCES/uke-fidelityfx-1.1.4.tar.gz"
cp "$component/packaging/fidelityfx/uke-fidelityfx.spec" "$stage/rpmbuild/SPECS/"
(cd "$stage/rpmbuild/SOURCES" && sha256sum ./*.tar.gz > source-sha256.txt)
fedora=$(rpm --eval '%{fedora}')
[[ $fedora =~ ^[0-9]+$ ]]
# COPR's outer SRPM phase does not inherit the custom worker's RPM macros.
sed "s/%{?dist}/.fc$fedora/g" "$stage/rpmbuild/SPECS/uke-fidelityfx.spec" > "$out/uke-fidelityfx.spec"
cp -- "$stage/rpmbuild/SOURCES/"* "$out/"
