#!/bin/bash
# Read actual built DEBs and supplied archive metadata. No installation,
# downloads, maintainer scripts or host APT state are used.
set -euo pipefail
export LC_ALL=C
indexes=$(realpath "${1:?index directory required}")
debs=$(realpath "${2:?directory of checksum-verified DEBs required}")
out=${3:?new report directory required}
[[ ! -e $out ]]
mkdir -p "$out"
out=$(realpath "$out")
[[ $out != *'"'* && $out != *$'\n'* ]]
mkdir -p "$out/lists/partial" "$out/cache/archives/partial" "$out/empty/apt.conf.d" "$out/empty/sources.list.d" "$out/empty/preferences.d"
: > "$out/empty/apt.conf"
: > "$out/status"
dpkg-scanpackages --multiversion "$debs" /dev/null > "$out/built-Packages" 2> "$out/scan.log"
# This union is a solver fixture, never an authenticated archive or install
# source. Every added provider comes from an actual supplied DEB.
cat "$indexes/main-Packages" "$out/built-Packages" > "$out/lists/ports.ubuntu.com_ubuntu-ports_dists_tenacious_main_binary-arm64_Packages"
cp "$indexes/universe-Packages" "$out/lists/ports.ubuntu.com_ubuntu-ports_dists_tenacious_universe_binary-arm64_Packages"
echo 'deb https://ports.ubuntu.com/ubuntu-ports tenacious main universe' > "$out/sources.list"
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
sha256sum "$indexes/main-Packages" "$indexes/universe-Packages" "$debs/"*.deb > "$out/input-sha256.txt"
printf 'package\tversion\tclosure\tpython_in_plan\n' > "$out/results.tsv"
failed=0
for deb in "$debs/"*.deb; do
 package=$(dpkg-deb -f "$deb" Package)
 version=$(dpkg-deb -f "$deb" Version)
 status=0
 /usr/bin/apt-get --simulate --no-install-recommends satisfy "$package (= $version)" > "$out/$package.log" 2>&1 || status=$?
 if ((status==0)); then closure=resolved; else closure=unresolved; failed=$((failed+1)); fi
 python=not-assessed
 if ((status==0)); then
  python=absent
  if grep -Eq '^Inst (python[0-9.-]*|libpython[0-9.-]*)(:| )' "$out/$package.log"; then python=present; fi
 fi
 printf '%s\t%s\t%s\t%s\n' "$package" "$version" "$closure" "$python" >> "$out/results.tsv"
done
[[ ! -s $out/status && -z $(find "$out/cache/archives" -type f -name '*.deb' -print -quit) ]]
cat "$out/results.tsv"
printf 'Simulation only: %s unresolved binary dependency closures. No archive trust or runtime behavior is established.\n' "$failed"
((failed==0))
