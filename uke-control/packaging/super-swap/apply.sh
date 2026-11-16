#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 2 ]]
control=$(realpath "$1");overlay=$(realpath "$2")
[[ -f $control/src/backend.cpp && ! -e $overlay/legacy && ! -e $control/tests/super-swap.sh ]]
# The retained signed CDC23 check still runs on its exact prepared sources.
# Production and the new four-mode checks use the patched current tree.
cp -a --reflink=auto "$control" "$overlay/legacy"
before=$(awk '/^static std::set<std::string> partition_candidates\(\)/{p=1} /^static bool own_file\(/{p=0} p' "$control/src/backend.cpp" | sha256sum | cut -d ' ' -f1)
[[ $before == 14b2f0df0a20875ce61c01381bd5b3c727d6ccf4588a6543a6cfd0d8571fcbe9 ]]
printf '%s  %s\n' 4b2adc37bb657405e7ce915ecdb9754ca08a4831678c8b9203becf3f5ed33250 "$control/src/setup-cdc.cpp" | sha256sum -c --strict --quiet -
(cd "$control";patch -p2 --fuzz=0 --forward --dry-run -i "$overlay/consumer.patch";patch -p2 --fuzz=0 --forward -i "$overlay/consumer.patch")
(cd "$control";sha256sum -c --strict --quiet "$overlay/post.sha256")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for file in src/backend.cpp src/setup-cdc.cpp tests/setup-cdc.cpp;do
    install -Dm644 "$control/$file" "$work/$file"
done
(cd "$work";patch -p2 --fuzz=0 --reverse -i "$overlay/consumer.patch")
for file in src/backend.cpp src/setup-cdc.cpp tests/setup-cdc.cpp;do cmp "$overlay/legacy/$file" "$work/$file";done
install -Dm644 "$overlay/tests/super-swap.sh" "$control/tests/super-swap.sh"
grep -Fq 'std::string target=prepared_partition();' "$control/src/backend.cpp"
grep -Fq '"--reboot-argument=bootloader","reboot"' "$control/src/setup-cdc.cpp"
if grep -Fq '"/usr/bin/uke-reboot"' "$control/src/setup-cdc.cpp";then exit 1;fi
echo 'Control25 exact swap preimage, original CDC source and whole-file inverse preserve every other prepared byte.'
