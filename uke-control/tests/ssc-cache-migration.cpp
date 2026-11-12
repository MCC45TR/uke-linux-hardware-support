// SPDX-License-Identifier: GPL-3.0-or-later
// Public synthetic inputs only. Native execution belongs in COPR.
#define UKE_SSC_CACHE_TEST
#include "../src/ssc-cache.cpp"
#include <cassert>

static void write(const fs::path &path, const char *value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path) << value;
}
static std::string read(const fs::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
template<class Function> static void rejects(Function function) {
    bool rejected=false;
    try { function(); } catch (const std::exception &) { rejected=true; }
    assert(rejected);
}
static void fixture(const fs::path &root) {
    fs::create_directories(root/"run/uke-ssc/fedora");
    fs::permissions(root/"run/uke-ssc",fs::perms::owner_all);
    fs::permissions(root/"run/uke-ssc/fedora",fs::perms::owner_all);
    write(root/"mnt/vendor/persist/sensors/registry/registry/calibration","public factory vector");
    write(root/"mnt/vendor/persist/sensors/registry/sns_reg_version","1");
    write(root/"usr/share/uke-sensors/odm/json.lst","public config");
    write(root/"usr/share/uke-sensors/sns_reg.conf","public profile");
    write(root/"usr/lib/firmware/qcom/uke/sensors/sns_tppe.so","public DSP fixture");
    for (const char *name : {"hw_platform","platform_subtype","platform_subtype_id","platform_version","soc_id","revision"})
        write(root/"sys/devices/soc0"/name,"public SoC field");
}
int main(int argc,char **argv) {
    assert(argc==2);
    const fs::path root=argv[1], legacy=root/"run/uke-ssc", base=legacy/"fedora";
    fixture(root);
    // A new installation without an old generation still seeds read-only input.
    seed(root,base);
    assert(read(base/"state/sensors/persist/registry/calibration")=="public factory vector");
    fs::remove_all(base/"state");
    seed(root,legacy);
    write(legacy/"state/sensors/persist/registry/calibration","public accepted RAM change");
    write(legacy/"state/sensors/persist/runtime-only","public parent-level state");
    const int reader=open((legacy/"registry.lock").c_str(),O_RDONLY|O_CLOEXEC);
    assert(reader>=0 && flock(reader,LOCK_SH|LOCK_NB)==0);
    rejects([&]{seed(root,base);});
    assert(!fs::exists(base/"state"));
    assert(flock(reader,LOCK_UN)==0);close(reader);
    fs::permissions(legacy/"state",fs::perms::group_read,fs::perm_options::add);
    rejects([&]{seed(root,base);});
    fs::permissions(legacy/"state",fs::perms::owner_all);
    fs::rename(legacy/"state/sensors/persist/sns_reg_version",legacy/"state/sensors/persist/saved-version");
    rejects([&]{seed(root,base);});
    fs::rename(legacy/"state/sensors/persist/saved-version",legacy/"state/sensors/persist/sns_reg_version");
    fs::create_symlink("/etc/passwd",legacy/"state/sensors/persist/registry/foreign");
    rejects([&]{seed(root,base);});
    assert(!fs::exists(base/"state"));
    fs::remove(legacy/"state/sensors/persist/registry/foreign");
    seed(root,base);
    assert(read(base/"state/sensors/persist/registry/calibration")=="public accepted RAM change");
    assert(read(base/"state/sensors/persist/runtime-only")=="public parent-level state");
    assert(same_input(base/"state",legacy/"state"));
    assert(read(root/"mnt/vendor/persist/sensors/registry/registry/calibration")=="public factory vector");
    assert(fs::status(base/"state/sensors/persist/registry/calibration").permissions()==
           (fs::perms::owner_read|fs::perms::owner_write));
    // Once imported, the new generation owns its independent runtime changes.
    write(base/"state/sensors/persist/registry/calibration","public new RAM change");
    write(legacy/"state/sensors/persist/registry/calibration","public later old change");
    seed(root,base);
    assert(read(base/"state/sensors/persist/registry/calibration")=="public new RAM change");
    write(root/"usr/share/uke-sensors/odm/json.lst","public replacement config");
    seed(root,base);
    assert(read(base/"state/sensors/persist/registry/calibration")=="public new RAM change");
    assert(read(base/"state/sensors/persist/runtime-only")=="public parent-level state");
    assert(same_input(root/"usr/share/uke-sensors/odm",base/"state/sensors/config"));
    assert(read(legacy/"state/sensors/persist/registry/calibration")=="public later old change");
    // A qualified downgrade retires the new mount. A later upgrade must import
    // the accepted provider's latest RAM state rather than reuse stale new RAM.
    fs::remove_all(base/"state");
    write(legacy/"state/sensors/persist/registry/calibration","public rollback session change");
    seed(root,base);
    assert(read(base/"state/sensors/persist/registry/calibration")=="public rollback session change");
    assert(read(base/"state/sensors/persist/runtime-only")=="public parent-level state");
    assert(read(root/"mnt/vendor/persist/sensors/registry/registry/calibration")=="public factory vector");
    std::cout<<"SSC noswap-generation migration:accepted RAM,parent metadata,old-reader/private-mode/incomplete/symlink refusals,factory unchanged,independent warm refresh,and post-rollback reimport passed\n";
}
