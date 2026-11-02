#!/usr/bin/env bash
# COPR-only metadata correction over the installed Control source cohort.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
  --data-urlencode "projectname=$project" --data-urlencode packagename=uke-control \
  --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
  ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="2.8.1-6")' "$stage/builds.json" >/dev/null
done
curl -fLsS --proto '=https' --proto-redir '=https' --retry 2 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11240837/uke-control-2.8.1-5.fc46.src.rpm -o "$stage/parent.src.rpm"
printf '%s  %s\n' 86a9009506056373ec7f9cfbe4cfeeef189c154fdccd6267433a3f0c8dc6ee9e "$stage/parent.src.rpm" | sha256sum -c -
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-control-2.8.1-5.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}'
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
spec=$output/uke-control.spec
cp "$spec" "$stage/parent.spec"
printf '%s  %s\n' "$(sha256sum "$output/uke-control-2.8.1.tar.xz" | cut -d ' ' -f1)" "$output/uke-control-2.8.1.tar.xz" > "$stage/retained.sha256"
awk '
 BEGIN {header=1}
 /^Name:/ {package=$2}
 /^%package -n / {package=$3}
 /^%prep$/ {header=0}
 header && /^Release:/ {
  if ($2 !~ /^(5|6|17|19|35)[.]fc46$/) exit 1
  split($2,n,"."); $0="Release: " n[1]+1 ".fc46"; releases++
 }
 header {gsub(/2[.]8[.]1-5[.]fc46/,"2.8.1-6.fc46")}
 /^Recommends: uke-control-tui\(aarch-64\) = 2[.]8[.]1-2[.]fc46$/ {
  $0="Recommends: uke-control-tui(aarch-64) = 2.8.1-6.fc46"; tui++
 }
 {print}
 /^Source0:/ {print "Source1: control-metadata-overlay.json"; sources++}
 package=="uke-kde-plasma-meta" && /^Requires: glibc-all-langpacks$/ {
  print "Requires: hunspell-tr"; dictionaries++
 }
 END {if(releases!=12 || tui!=1 || sources!=1 || dictionaries!=1) exit 1}
' "$stage/parent.spec" > "$stage/next.spec"
# All build/check/install/lifecycle/file sections are retained exactly. Only
# package metadata and a source provenance receipt change; no KDE fork is built.
awk '/^%prep$/{body=1} body{print}' "$stage/parent.spec" > "$stage/body.before"
awk '/^%prep$/{body=1} body{print}' "$stage/next.spec" > "$stage/body.after"
cmp "$stage/body.before" "$stage/body.after"
mv "$stage/next.spec" "$spec"
test "$(grep -Fxc 'Requires: hunspell-tr' "$spec")" = 1
test "$(grep -Fxc 'Recommends: uke-control-tui(aarch-64) = 2.8.1-6.fc46' "$spec")" = 1
commit=$(git -C "$owner" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_copr_build:11240837,
 parent_srpm_sha256:"86a9009506056373ec7f9cfbe4cfeeef189c154fdccd6267433a3f0c8dc6ee9e",
 source_archive_unchanged:true,build_checks_install_and_scriptlets_unchanged:true,
 original_fedora_turkish_dictionary:true,matched_tui_recommendation:true,
 binary_releases_advanced:true,physical_acceptance:false}' > "$output/control-metadata-overlay.json"
sha256sum -c "$stage/retained.sha256"
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$spec") == 2.8.1-6.fc46 ]]
rpmspec --target aarch64 -q --qf '%{NAME}\t%{VERSION}-%{RELEASE}\n' "$spec" > "$stage/packages.tsv"
[[ $(wc -l < "$stage/packages.tsv") == 12 ]]
grep -Fxq $'uke-kde-plasma-meta\t1.0.0-36.fc46' "$stage/packages.tsv"
grep -Fxq $'uke-control-tui\t2.8.1-6.fc46' "$stage/packages.tsv"
