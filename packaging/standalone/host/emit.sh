#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Reuse a reviewed source recipe, retaining its complete build and check phases.
set -Eeuo pipefail
component=${1:?component}; family=${2:?legacy source group}; stage=${3:?stage}
output=${4:?output}; target=${5:?architecture}; dist=${6:?dist tag}
part="$stage/parts/$component"
[[ -s $part/preamble && -s $part/identity ]]
name=$(awk 'NR==1 && $1=="%package" && $2=="-n" {print $3}' "$part/preamble")
version=$(awk '$1=="Version:" {print $2;exit}' "$part/preamble")
[[ $name =~ ^uke-[A-Za-z0-9+_.-]+$ && $version =~ ^[A-Za-z0-9._+~^-]+$ ]]
[[ $name == "$(jq -er --arg input "$component" '.packages[]|select(.input==$input)|.name' packaging/standalone/packages.json)" ]]
IFS=$'\t' read -r _ _ _ cwd builddir buildroot <"$part/identity"
legacy_version=$(jq -er --arg f "$family" '.families[]|select(.name==$f)|.version' packaging/families/families.json)
source="$stage/$family-$legacy_version/inputs/$component"
bundle="$stage/$name-$version"
mkdir -p "$bundle/inputs/$component"
cp -a "$source/." "$bundle/inputs/$component/"
cp LICENSE "$bundle/"
cp "$stage/$family-$legacy_version/INPUT-SRPMS.tsv" "$bundle/"
printf '%s\n' 'Independent source build. Package checks and tablet acceptance are separate.' >"$bundle/README"

# Retire empty selection RPMs through explicit Provides/Obsoletes transitions.
# Keep installation policy in Core metadata, without coupling native libraries.
: >"$part/compat"
mapfile -t owned < <(jq -r --arg input "$component" '.packages[]|select(.input==$input)|.replaces[]' packaging/standalone/packages.json)
while IFS= read -r legacy; do
    [[ -n $legacy ]] || continue
    profile=$(jq -ce --arg f "$legacy" '.families[]|select(.name==$f)' packaging/families/families.json)
    evr=$(jq -r '.version+"-"+((.release//1)|tostring)' <<<"$profile")$dist
    printf 'Provides: %s = %s\nObsoletes: %s <= %s\n' "$legacy" "$evr" "$legacy" "$evr" >>"$part/compat"
done < <(printf '%s\n' "${owned[@]}")
if [[ $component == uke-core-meta ]]; then
    jq -r '.families[].requires[]|"Requires: "+gsub(" = ";" >= ")' packaging/families/families.json | LC_ALL=C sort -u | while IFS= read -r dependency; do
        token=${dependency#Requires: }; token=${token%% *}
        if ! awk -v token="$token" '$1=="%package" && $3==token {found=1} END {exit !found}' "$part/preamble"; then
            printf '%s\n' "$dependency" >>"$part/compat"
        fi
    done
    jq -r '.families[].suggests[]|"Suggests: "+gsub(" = ";" >= ")' packaging/families/families.json | LC_ALL=C sort -u >>"$part/compat"
fi

spec="$output/SPECS/$name.spec"
{
    printf '%%global debug_package %%{nil}\n'
    if [[ $component == uke-firmware || $component == uke-speaker-firmware ]]; then
        printf '%%global _build_id_links none\n%%global _binaries_in_noarch_packages_terminate_build 0\n'
    fi
    awk -v archive="$name-$version.tar.xz" -v target="$target" -v compat="$part/compat" '
      NR==1 {print "Name: " $3;next}
      !extra && /^%description/ {
        print "URL: https://github.com/MCC45TR/uke-linux-hardware-support"
        print "Source0: " archive;print "ExclusiveArch: " target
        while((getline line < compat)>0)print line
        close(compat);extra=1
      }
      {print}
    ' "$part/preamble"
    printf '\n%%prep\n%%setup -q\n'
    base="%{_builddir}/$name-$version"
    for phase in prep generate_buildrequires build install check; do
        [[ $phase != generate_buildrequires || -s $part/$phase ]] || continue
        [[ $phase == prep ]] || printf '\n%%%s\n' "$phase"
        if [[ $component == uke-firmware && $phase == check ]]; then
            printf 'mv "%s/firmware-unprocessed" "%%{buildroot}/usr/lib/firmware"\n' "$base"
        fi
        if [[ -s $part/$phase ]]; then
            printf '(\n'
            if [[ $phase == prep ]]; then
                printf 'mkdir -p "%s/work/%s"\n' "$base" "$component"
            else
                printf 'cd "%s/work/%s/%s"\n' "$base" "$component" "$cwd"
            fi
            awk -v base="$base" -v key="$component" -v source="$source" -v builddir="$builddir" -v buildroot="$buildroot" '
              function replace(s,old,new, p,r){r="";while((p=index(s,old))){r=r substr(s,1,p-1) new;s=substr(s,p+length(old))}return r s}
              {gsub(/%/,"%%");s=replace($0,buildroot,"%{buildroot}");s=replace(s,builddir,base "/work/" key);s=replace(s,source,base "/inputs/" key);print s}
            ' "$part/$phase"
            printf ')\n'
        fi
        if [[ $component == uke-firmware && $phase == install ]]; then
            printf 'mv "%%{buildroot}/usr/lib/firmware" "%s/firmware-unprocessed"\n' "$base"
        fi
    done
    awk -v prefix="work/$component/$cwd/" -v base="$base" -v key="$component" -v source="$source" '
      function replace(s,old,new, p,r){r="";while((p=index(s,old))){r=r substr(s,1,p-1) new;s=substr(s,p+length(old))}return r s}
      /^%files/ {files=1;for(i=2;i<=NF;i++)if($i=="-f")$(i+1)=prefix $(i+1);print;next}
      /^%/ && !/^%(license|doc|dir|ghost|config|attr|defattr|exclude|verify|caps)/ {files=0;print;next}
      files && /^%(license|doc) / {for(i=2;i<=NF;i++)if($i!~/^\//)$i=prefix $i;print;next}
      files && /^%config / {sub(/^%config /,"%config(noreplace) ");print;next}
      {if(!files)gsub(/%/,"%%");print replace($0,source,base "/inputs/" key)}
    ' "$part/tail"
    while IFS=$'\t' read -r old new unit; do
        if ! awk -v name="$new" '$1=="%package" && $3==name {found=1} END {exit !found}' "$part/preamble"; then continue; fi
        for phase in save restore; do
            section=pre; [[ $phase != restore ]] || section=posttrans
            printf '\n%%%s -n %s\nold=%s; new=%s; unit=%s; phase=%s\n' "$section" "$new" "$old" "$new" "$unit" "$phase"
            sed 's/%/%%/g' packaging/families/host/service-rename.sh
        done
    done < <(jq -r --arg f "$family" '.[]|select(.family==$f)|[.old,.new,.unit]|@tsv' packaging/families/migrations.json)
    printf '\n%%changelog\n* Thu Oct 29 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com>\n- Build this component independently in the Rawhide unstable channel.\n'
} >"$spec"
touch "$part/buildrequires"
cp "$part/buildrequires" "$part/buildrequires.saved"
awk 'NR==FNR {requirements=requirements $0 "\n";next} !inserted && /^%description/ {printf "%s",requirements;inserted=1} {print}' "$part/buildrequires.saved" "$spec" >"$spec.new"
mv "$spec.new" "$spec"
bash packaging/families/host/check-cohort.sh "$spec" "$target"
(cd "$bundle"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | LC_ALL=C sort -z | xargs -0 sha256sum >SOURCE-SHA256SUMS)
tar --sort=name --mtime='2026-10-29 00:00:00Z' --owner=0 --group=0 --numeric-owner -cJf "$output/SOURCES/$name-$version.tar.xz" -C "$stage" "$name-$version"
rpmspec --parse --target "$target" --define "dist $dist" "$spec" >"$output/parsed.spec"
rpmbuild -bs --nodeps --target "$target" --define "dist $dist" --define "_topdir $output" "$spec"
