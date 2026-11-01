#!/bin/bash
# Offline APT simulation against supplied Tenacious indexes. No downloads,
# installation, package scripts, host status database or configured host hooks.
set -euo pipefail
export LC_ALL=C
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
indexes=$(realpath "${1:?Directory containing main-Packages and universe-Packages}")
out=${2:?New report directory required}
[[ ! -e $out ]]
mkdir -p "$out"
out=$(realpath "$out")
[[ $out != *'"'* && $out != *$'\n'* ]]
mkdir -p "$out/lists/partial" "$out/cache/archives/partial" "$out/empty/apt.conf.d" "$out/empty/sources.list.d" "$out/empty/preferences.d"
: > "$out/empty/apt.conf"
: > "$out/status"
for section in main universe; do
 cp "$indexes/$section-Packages" "$out/lists/ports.ubuntu.com_ubuntu-ports_dists_tenacious_${section}_binary-arm64_Packages"
done
printf '%s\n' 'deb https://ports.ubuntu.com/ubuntu-ports tenacious main universe' > "$out/sources.list"
cat > "$out/apt.conf" <<EOF
Dir::Etc "$out/empty";
Dir::Etc::main "apt.conf";
Dir::Etc::parts "apt.conf.d";
Dir::Etc::sourcelist "$out/sources.list";
Dir::Etc::sourceparts "sources.list.d";
Dir::State::lists "$out/lists";
Dir::State::status "$out/status";
Dir::Cache "$out/cache";
Dir::Log "$out";
APT::Architecture "arm64";
APT::Architectures { "arm64"; };
APT::Install-Recommends "false";
APT::Get::Simulate "true";
EOF
export APT_CONFIG=$out/apt.conf
sha256sum "$indexes/main-Packages" "$indexes/universe-Packages" > "$out/index-sha256.txt"
: > "$out/results.tsv"
failed=0
for control in "$root/packaging/ubuntu/recipes/"*/debian/control; do
 package=$(sed -n 's/^Source: //p' "$control")
 dependencies=$(sed -n 's/^Build-Depends: //p' "$control")
 [[ -n $dependencies && $dependencies != *$'\n'* ]]
 status=0
 /usr/bin/apt-get --simulate --no-install-recommends satisfy "$dependencies" > "$out/$package.log" 2>&1 || status=$?
 if [[ $status == 0 ]]; then result=resolved; else result=unresolved; failed=$((failed+1)); fi
 printf '%s\t%s\t%s\n' "$package" "$result" "$status" >> "$out/results.tsv"
done
[[ ! -s $out/status && -z $(find "$out/cache/archives" -type f -name '*.deb' -print -quit) ]]
printf 'Metadata-only ARM64 build dependency simulation: %s unresolved source recipes. No authenticated download, installation, target chroot or native build was performed.\n' "$failed"
cat "$out/results.tsv"
((failed==0))
