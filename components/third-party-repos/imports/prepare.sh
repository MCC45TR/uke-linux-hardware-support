#!/usr/bin/env bash
set -Eeuo pipefail

name=${1:?application name required}
out=${2:?result directory required}
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

case "$name" in
    bruno)
        repo=usebruno/bruno
        version=4.3.0
        asset=bruno_4.3.0_aarch64_linux.rpm
        sha=d226ca796dd39babd92fb13d91591bb578459f477fd20230d6cb019d54ab13e5
        license=MIT
        license_file=license.md
        license_sha=8891070a847e5047bf77d38d88d7dfbab1beab41e37c802b9f5b23f2bbb9c7be
        summary='Bruno API client for Linux AArch64'
        app_dir=/opt/Bruno
        alternatives=1
        ;;
    trilium)
        repo=TriliumNext/Trilium
        version=0.107.0
        asset=TriliumNotes-v0.107.0-linux-arm64.rpm
        sha=ec86a940e34b17b8a3148101e95bdda8679aaf5322cb4a9c74746a8ee7c903be
        license=AGPL-3.0-or-later
        license_file=LICENSE
        license_sha=76a97c878c9c7a8321bb395c2b44d3fe2f8d81314d219b20138ed0e2dddd5182
        summary='Trilium Notes for Linux AArch64'
        app_dir=/usr/lib/trilium
        alternatives=0
        ;;
    ferdium)
        repo=ferdium/ferdium-app
        version=7.2.3
        asset=Ferdium-linux-7.2.3-aarch64.rpm
        sha=a750d3db8f18886aa0ed646dcf9f05ef2bd7f424189cbec085ca558bd4648df9
        license=Apache-2.0
        license_file=LICENSE.md
        license_sha=d8cce85684984a168ed6d7a4513b7a27b3471a8f0d1efabc1f58faf556c73ac0
        summary='Ferdium workspace for Linux AArch64'
        app_dir=/opt/Ferdium
        alternatives=1
        ;;
    *)
        printf 'Unsupported application %s\n' "$name" >&2
        exit 2
        ;;
esac

mkdir -p -- "$out"
cd -- "$out"
upstream="https://github.com/$repo"
asset_url="$upstream/releases/download/v$version/$asset"
license_url="https://raw.githubusercontent.com/$repo/v$version/$license_file"
curl --proto '=https' --tlsv1.2 -fsSL --retry 2 --max-time 220 --max-filesize 170000000 "$asset_url" -o "$asset"
curl --proto '=https' --tlsv1.2 -fsSL --retry 2 --max-time 30 --max-filesize 80000 "$license_url" -o "$license_file"
printf '%s  %s\n' "$sha" "$asset" "$license_sha" "$license_file" | sha256sum -c --strict
test "$(rpm -qp --qf '%{NAME} %{VERSION} %{ARCH}' "$asset")" = "$name $version aarch64"

requirements=$(rpm -qpR "$asset" | awk '!/^rpmlib\(/ {print "Requires:       " $0}')
files=$(rpm -qpl "$asset" | awk '/^\/usr\/share\// {print}')
[[ -n $requirements && -n $files && $requirements != *%* && $files != *%* ]]
{
    printf '%%global app_name %s\n' "$name"
    printf '%%global app_version %s\n' "$version"
    printf '%%global app_summary %s\n' "$summary"
    printf '%%global app_license %s\n' "$license"
    printf '%%global app_upstream %s\n' "$upstream"
    printf '%%global app_asset %s\n' "$asset_url"
    printf '%%global app_digest %s\n' "$sha"
    printf '%%global app_license_url %s\n' "$license_url"
    printf '%%global app_license_digest %s\n' "$license_sha"
    printf '%%global app_dir %s\n' "$app_dir"
    printf '%%global app_alternatives %s\n' "$alternatives"
    awk -v requires="$requirements" -v files="$files" '
        /^@APP_REQUIRES@$/ { print requires; next }
        /^@APP_FILES@$/ { print files; next }
        { print }
    ' "$here/electron-app.spec.in"
} > "$name.spec"
