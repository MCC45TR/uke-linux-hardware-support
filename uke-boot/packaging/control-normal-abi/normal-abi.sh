#!/usr/bin/bash
# Extend the already verified Control boot sources for the normal ABI.
set -Eeuo pipefail
[[ $# == 2 ]]
boot=$(realpath "$1") color=$(realpath "$2")
replace() {
    local path=$1 before=$2 after=$3 count
    count=$(grep -Fc -- "$before" "$path")
    [[ $count == 1 ]]
    BEFORE="$before" AFTER="$after" perl -0777 -i -pe 's/\Q$ENV{BEFORE}\E/$ENV{AFTER}/;' "$path"
}

for header in "$boot/src/unstable-release.hpp" "$color/unstable-release.hpp"; do
    replace "$header" 'namespace uke_release {' 'namespace uke_release {
inline constexpr std::string_view normal_publication_abi = "7.4.5-senemos";
inline bool normal_publication(std::string_view release) {
    return release == normal_publication_abi;
}'
done
replace "$boot/src/boot-status.cpp" 'if (release.size() < 128 && uke_release::unstable(release))' \
    'if (uke_release::normal_publication(release) || (release.size() < 128 && uke_release::unstable(release)))'
replace "$boot/src/o82-private-color.hpp" 'if (!uke_release::unstable(release))' \
    'if (!uke_release::unstable(release) && !uke_release::normal_publication(release))'
replace "$color/o82-stock-profile.hpp" 'token(release) && uke_release::unstable(release) && release != running' \
    'token(release) && (uke_release::unstable(release) || uke_release::normal_publication(release)) && release != running'
for test in "$boot/tests/o82-private-color.cpp" "$color/o82-stock-profile-contract.cpp"; do
    replace "$test" '"7.4.0-2-unstable", "8.0.0-1-unstable"' '"7.4.0-2-unstable", "8.0.0-1-unstable", "7.4.5-senemos"'
done

updater=$boot/src/kernel-update.cpp
replace "$updater" 'static constexpr char merged_layout_capability[]' 'static constexpr char normal_kernel_core[] = "senemos-uke-linux-kernel-mainline-core";
static constexpr char merged_layout_capability[]'
replace "$updater" 'const bool unstable_core = base+"-core"==merged_kernel_core &&' \
    'const auto core_name = base+"-core";
        const bool merged_core =
            (core_name==merged_kernel_core || (core_name==normal_kernel_core && release=="7.4.5-senemos")) &&'
replace "$updater" 'header.starts_with(std::string(merged_kernel_core)+' 'header.starts_with(core_name+'
replace "$updater" 'unstable_core?merged_kernel_core:expected' 'merged_core?core_name:expected'
sed -i 's/unstable_core/merged_core/g' "$updater"
replace "$updater" 'need(p.name==expected || p.merged_layout,"Alternate owner lacks merged kernel layout capability");' \
    'need(p.name==expected || p.merged_layout,"Alternate owner lacks merged kernel layout capability");
        need(release!="7.4.5-senemos" || p.merged_layout,"Normal publication requires exact merged ownership");'
replace "$updater" 'need(core.name==merged_kernel_core,"Unreviewed merged rollback kernel family");' \
    'need(core.name==merged_kernel_core || core.name==normal_kernel_core,"Unreviewed merged rollback kernel family");'

contract=$boot/tests/kernel-update.cpp
replace "$contract" '    reject([]{rollback_modules(Package{"other-core"' '
    {
        const std::string abi="7.4.5-senemos";
        const std::string core_name=normal_kernel_core;
        const std::string core_nevra=core_name+"-0:7.3.5-12.fc46.aarch64";
        const std::string owner=core_name+"\t0\t7.3.5\t12.fc46\taarch64\t8\n";
        std::map<std::string,std::map<fs::path,std::string>> groups;
        for (const std::string suffix:{"-boot","-core","-dtbs","-modules"}) {
            const auto path=root/("publication"+suffix);
            owners[path.string()]=owner;
            file_packages[path.string()]=core_nevra;
            dumps[core_nevra]+=path.string()+"\t"+hash+"\n";
            groups[suffix][path]=hash;
        }
        capabilities[core_nevra]="kernel-uname-r("+abi+")\n";
        reject([&]{installed_files(groups,abi,fake_rpm);});
        capabilities[core_nevra]+=std::string(merged_layout_capability)+"\n";
        const auto publication=installed_files(groups,abi,fake_rpm);
        assert(publication.merged_layout);
        variant_module_nevra=core_nevra;
        variant_module_identity=owner;
        rollback_modules(publication,fake_variant_rollback);
        reject([&]{installed_files(groups,abi+"-foreign",fake_rpm);});
    }
    reject([]{rollback_modules(Package{"other-core"'
