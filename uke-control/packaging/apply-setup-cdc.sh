#!/usr/bin/bash
# Source/text overlay only. Preserve every prepared cumulative Control input.
set -Eeuo pipefail
[[ $# == 2 ]]
root=$(realpath "$1");overlay=$(realpath "$2")
[[ $root != / && -f $root/src/main.cpp && -f $root/Makefile && ! -L $root/src/main.cpp && ! -e $root/src/setup-cdc.cpp ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ $(grep -Fxc '#include "backend.hpp"' "$root/src/main.cpp") == 1 ]]
[[ $(grep -Fc 'std::string command=argv[1];' "$root/src/main.cpp") == 1 ]]
! grep -Fq 'setup-cdc' "$root/src/main.cpp"
mkdir "$work/before" "$work/next"
cp "$root/src/main.cpp" "$work/before/main.cpp"
cp "$root/Makefile" "$work/before/Makefile"
! grep -Fq UKE_SETUP_CDC_DEP "$root/Makefile"
awk '
 $0=="#include \"backend.hpp\""{print;print "#include \"setup-cdc.hpp\"";next}
 {print}
 index($0,"std::string command=argv[1];"){print "        if(command==\"setup-cdc\")return uke::setup_cdc::command(argc-2,argv+2);"}
' "$work/before/main.cpp" > "$work/next/main.cpp"
awk '
 $0=="#include \"setup-cdc.hpp\"" || $0=="        if(command==\"setup-cdc\")return uke::setup_cdc::command(argc-2,argv+2);"{next}
 {print}
' "$work/next/main.cpp" | cmp - "$work/before/main.cpp"
mapfile -d '' candidates < <(find "$root" -maxdepth 3 -type f \( -name Makefile -o -name '*.mk' \) -print0)
[[ ${#candidates[@]} -gt 0 && ${#candidates[@]} -lt 256 ]]
changed=0
for file in "${candidates[@]}";do
 grep -Fq src/main.cpp "$file" || continue
 [[ ! -L $file ]]
 relative=${file#"$root/"}
 (cd "$root";cp --parents "$relative" "$work/before")
 mkdir -p "$work/next/$(dirname "$relative")"
 sed 's|src/main.cpp|src/main.cpp src/setup-cdc.cpp|g' "$file" > "$work/next/$relative"
 sed 's|src/main.cpp src/setup-cdc.cpp|src/main.cpp|g' "$work/next/$relative" | cmp - "$file"
 install -m644 "$work/next/$relative" "$file"
 changed=$((changed+1))
done
[[ $changed -gt 0 ]]
install -m644 "$work/next/main.cpp" "$root/src/main.cpp"
for file in src/setup-cdc.cpp src/setup-cdc.hpp tests/setup-cdc.cpp tests/setup-cdc.sh;do
 [[ -f $overlay/$file && ! -L $overlay/$file ]]
 install -m644 "$overlay/$file" "$root/$file"
done
printf '# UKE_SETUP_CDC_DEP_BEGIN\nbuild/uke-control: src/setup-cdc.cpp src/setup-cdc.hpp\n# UKE_SETUP_CDC_DEP_END\n' >> "$root/Makefile"
while IFS= read -r -d '' file;do
 relative=${file#"$work/before/"}
 awk '
  $0=="# UKE_SETUP_CDC_DEP_BEGIN"{skip=1;next}
  $0=="# UKE_SETUP_CDC_DEP_END"{skip=0;next}
  !skip{print}
 ' "$root/$relative" | sed 's|src/main.cpp src/setup-cdc.cpp|src/main.cpp|g' | cmp - "$file"
done < <(find "$work/before" -type f ! -name main.cpp -print0)
printf 'Prepared Control main route and %s Makefile fragments retain their complete inverses; setup code stays in the existing executable.\n' "$changed"
