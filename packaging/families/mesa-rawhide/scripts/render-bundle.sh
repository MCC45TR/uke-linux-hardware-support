#!/usr/bin/env bash
set -Eeuo pipefail
export LC_ALL=C

# Preserve the installed runtime package and its versioned Mesa capabilities.
input=$1 version=$2 release=$3 top=$4
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ && $release =~ ^[0-9]+\.fc[0-9]+$ ]]
mkdir -p "$top"/{SOURCES,SPECS}
source_dir=$top/SOURCES
spec=$top/SPECS/uke-mesa.spec
runtime=(dri-drivers filesystem libEGL libGL libOpenCL libTeflon libgbm vulkan-drivers libclc22-spirv)
development=(libEGL-devel libGL-devel libgbm-devel libOpenCL-devel)
key=$input/copr.asc
fingerprint=2204a661ea630a5303500085538797e72a3e4eca
rpmkeys --dbpath "$input/keydb" --import "$key"
rpmkeys --dbpath "$input/keydb" --list | awk '{ print tolower($1) }' | grep -Fx "$fingerprint"
cp "$key" "$source_dir/"

cat > "$spec" <<EOF
%global debug_package %{nil}
%global _build_id_links none
%global __os_install_post %{nil}
Name: uke-mesa
Version: $version
Release: $release
Summary: Mesa runtime for Uke
License: MIT AND BSD-3-Clause AND SGI-B-2.0 AND (Apache-2.0 WITH LLVM-exception)
ExclusiveArch: aarch64
AutoReqProv: no
BuildRequires: rpm cpio coreutils bash
EOF

declare -A file
all=()
index=0
for child in "${runtime[@]}" "${development[@]}"; do
    shopt -s nullglob
    packages=("$input/uke-mesa-$child-"[0-9]*.aarch64.rpm)
    shopt -u nullglob
    [[ ${#packages[@]} == 1 ]]
    rpm_path=${packages[0]}
    filename=${rpm_path##*/}
    queries=(rpm --dbpath "$input/keydb" -qp)
    rpmkeys --dbpath "$input/keydb" --checksig -v "$rpm_path" > "$input/signature"
    grep -Fq "signature, key fingerprint: $fingerprint: OK" "$input/signature"
    for query in scripts triggers filetriggers; do
        [[ -z $("${queries[@]}" "--$query" "$rpm_path") ]]
    done
    [[ $("${queries[@]}" --qf '%{NAME}|%{ARCH}' "$rpm_path") == "uke-mesa-$child|aarch64" ]]
    install -m644 "$rpm_path" "$source_dir/$filename"
    file[$child]=$source_dir/$filename
    all+=("$filename")
    printf 'Source%d: %s\n' "$index" "$filename" >> "$spec"
    index=$((index+1))
done
for name in copr.asc signed-inputs.sha256 payload.tsv; do
    printf 'Source%d: %s\n' "$index" "$name" >> "$spec"
    index=$((index+1))
done
(cd "$source_dir"; sha256sum -- "${all[@]}") > "$source_dir/signed-inputs.sha256"

emit_dependencies() {
    local header=$1 child line
    shift
    for child; do
        rpm --dbpath "$input/keydb" -qp "--$header" "${file[$child]}" |
            awk -v tag="$header" 'NF && !/^rpmlib\(/ { print toupper(substr(tag,1,1)) substr(tag,2) ": " $0 }'
    done
}
emit_metadata() {
    local header
    for header in requires provides conflicts obsoletes recommends suggests supplements enhances; do
        emit_dependencies "$header" "$@" | sort -u
    done
}

emit_metadata "${runtime[@]}" >> "$spec"
for child in "${runtime[@]}"; do
    rpm --dbpath "$input/keydb" -qp --qf 'Obsoletes: %{NAME} <= %{VERSION}-%{RELEASE}\n' \
        "${file[$child]}" >> "$spec"
done
cat >> "$spec" <<'EOF'

%description
Mesa drivers, runtime libraries and the LLVM 22 SPIR-V builtins used on Uke.
EOF

for child in "${development[@]}"; do
    printf '\n%%package -n uke-mesa-%s\nSummary: Uke Mesa %s files\nAutoReqProv: no\n' "$child" "$child" >> "$spec"
    emit_metadata "$child" >> "$spec"
    printf '\n%%description -n uke-mesa-%s\nDevelopment files from the same Mesa build.\n' "$child" >> "$spec"
done

{
    cat <<'EOF'

%prep
test "$(uname -m)" = aarch64
work=%{_builddir}/validated-rpms
mkdir -p "$work"
rpmkeys --dbpath "$work" --import %{_sourcedir}/copr.asc
(cd %{_sourcedir}; sha256sum --check --strict signed-inputs.sha256)

%install
set -o pipefail
work=%{_builddir}/validated-rpms
mkdir -p %{buildroot}
cd %{buildroot}
EOF
    for filename in "${all[@]}"; do
        printf 'rpmkeys --dbpath "$work" --checksig %%{_sourcedir}/%s\n' "$filename"
    done
    for filename in "${all[@]}"; do
        printf 'rpm2cpio %%{_sourcedir}/%s | cpio -idm --quiet --no-absolute-filenames --no-preserve-owner\n' "$filename"
    done
    cat <<'EOF'

%check
cd %{buildroot}
while IFS='|' read -r path mode flags digest link owner group; do
    test "$(stat -c '%%A' ".$path")" = "$mode"
    case "$mode" in
        l*) test "$(readlink ".$path")" = "$link" ;;
        -*) printf '%%s  %%s\n' "$digest" ".$path" | sha256sum --check --strict --quiet ;;
        d*) test -d ".$path" ;;
        *) exit 1 ;;
    esac
done < %{_sourcedir}/payload.tsv
EOF
} >> "$spec"

inventory() {
    local child
    for child; do
        rpm --dbpath "$input/keydb" -qp --qf \
            '[%{FILENAMES}|%{FILEMODES:perms}|%{FILEFLAGS:fflags}|%{FILEDIGESTS}|%{FILELINKTOS}|%{FILEUSERNAME}|%{FILEGROUPNAME}\n]' \
            "${file[$child]}"
    done | sort -u
}
inventory "${runtime[@]}" "${development[@]}" > "$source_dir/payload.tsv"
# Reject ambiguous paths and metadata that this repackaging does not copy.
awk -F '|' '
    NF != 7 || $1 !~ /^\/[[:alnum:]_+.\/-]+$/ || $1 ~ /(^|\/)\.\.(\/|$)/ ||
    $2 !~ /^[ld-]/ || $3 !~ /^[dl]*$/ || $6 != "root" || $7 != "root" { bad=1; print }
    paths[$1]++ { bad=1; print }
    END { exit bad }
' "$source_dir/payload.tsv"

emit_files() {
    inventory "$@" | awk -F '|' '{
        if ($3 ~ /d/) printf "%%doc ";
        if ($3 ~ /l/) printf "%%license ";
        if ($2 ~ /^d/) printf "%%dir ";
        print $1
    }'
}
{
    printf '\n%%files\n'
    emit_files "${runtime[@]}"
    for child in "${development[@]}"; do
        printf '\n%%files -n uke-mesa-%s\n' "$child"
        emit_files "$child"
    done
} >> "$spec"
rpmbuild -bs --target aarch64 --define "_topdir $top" "$spec"
