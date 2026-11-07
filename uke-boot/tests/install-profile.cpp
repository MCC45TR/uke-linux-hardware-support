// SPDX-License-Identifier: MIT
#define UKE_INSTALL_PROFILE_TEST
#include "../src/install-profile.cpp"
#include <cassert>
#include <fstream>
static unsigned grow_calls = 0;
static void simulated_grow() { ++grow_calls; }
template<class F> static void refuse(F action) {
    bool refused = false;
    try { action(); } catch (const std::runtime_error &) { refused = true; }
    assert(refused);
}
int main() {
    char pattern[] = "/tmp/uke-install-profile.XXXXXX";
    const auto temporary = mkdtemp(pattern);
    assert(temporary);
    const fs::path root = temporary;
    {
        ProfileFd dir(open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
        trusted(dir.value, true);
        unsigned set_calls = 0; timespec applied{};
        const ClockSample synchronized{{1793650000, 123456789}, true};
        const auto setter = [&](timespec value){++set_calls; applied=value; return 0;};
        assert(!save_clock_floor(dir.value, {{1793650000, 0}, false}));
        assert(!fs::exists(root / "clock-floor"));
        assert(!restore_clock_floor(dir.value, {{0, 0}, false}, setter));
        assert(save_clock_floor(dir.value, synchronized));
        assert(fs::file_size(root / "clock-floor") == 0);
        assert(!save_clock_floor(dir.value, synchronized));
        assert(!save_clock_floor(dir.value, {{1793649000, 0}, true}));
        assert(restore_clock_floor(dir.value, {{0, 0}, false}, setter));
        assert(set_calls == 1 && applied.tv_sec == synchronized.now.tv_sec && applied.tv_nsec == synchronized.now.tv_nsec);
        assert(!restore_clock_floor(dir.value, {{1793650100, 0}, false}, setter) && set_calls == 1);
        refuse([&]{restore_clock_floor(dir.value, {{0, 0}, false}, [](timespec){return -1;});});
        refuse([&]{save_clock_floor(dir.value, {{1, 0}, true});});
        refuse([&]{save_clock_floor(dir.value, {{4102444800, 0}, true});});
        refuse([&]{save_clock_floor(dir.value, {{1793650100, 1000000000}, true});});
        fs::remove(root / "clock-floor");
        std::ofstream(root / "outside-clock") << "preserved\n";
        fs::create_symlink(root / "outside-clock", root / "clock-floor");
        refuse([&]{save_clock_floor(dir.value, synchronized);});
        fs::remove(root / "clock-floor");
        fs::create_hard_link(root / "outside-clock", root / "clock-floor");
        refuse([&]{read_clock_floor(dir.value);});
        fs::remove(root / "clock-floor");
        std::ofstream(root / "clock-floor") << "unexpected\n";
        refuse([&]{read_clock_floor(dir.value);});
        assert(observation(root / "outside-clock", 32) == "preserved\n");
        fs::remove(root / "clock-floor"); fs::remove(root / "outside-clock");
        std::cout << "Synchronized-only zero-byte clock floor, monotonic preservation, restore/error propagation and unsafe-state refusal passed; injected clocks, no RTC or system-clock changes.\n";
        assert(!repair_persist_context(dir.value));
        const auto fstab = root / "fstab";
        const std::string legacy = "PARTLABEL=persist /mnt/vendor/persist ext4 ro,noload,nodev,nosuid,noexec,nofail,x-systemd.device-timeout=10s 0 0";
        const std::string context = "context=system_u:object_r:var_lib_t:s0,";
        const auto content = "# preserved comment\nUUID=example / ext4 defaults 0 1\n" + legacy + "\n";
        std::ofstream(fstab) << content;
        assert(chmod(fstab.c_str(), 0640) == 0);
        assert(setxattr(fstab.c_str(), "user.uke-fixture", "preserved", 9, 0) == 0);
        assert(repair_persist_context(dir.value));
        auto expected = content; expected.insert(expected.find("noexec,") + 7, context);
        assert(observation(fstab, 4096) == expected && !repair_persist_context(dir.value));
        struct stat fstab_stat{}; assert(stat(fstab.c_str(), &fstab_stat) == 0);
        assert((fstab_stat.st_mode & 07777) == 0640 && fstab_stat.st_uid == geteuid());
        char attribute[16]{};
        assert(getxattr(fstab.c_str(), "user.uke-fixture", attribute, sizeof(attribute)) == 9 &&
               std::string(attribute, 9) == "preserved");
        for (const auto &custom : {legacy + ",custom\n", "# " + legacy + "\n",
                                  legacy + "\n" + legacy + "\n", legacy + "\nUUID=other /mnt/vendor/persist ext4 ro 0 0\n"}) {
            std::ofstream(fstab) << custom;
            assert(!repair_persist_context(dir.value) && observation(fstab, 4096) == custom);
        }
        std::ofstream(fstab) << content;
        {
            ProfileFd busy(open(fstab.c_str(), O_RDONLY | O_CLOEXEC));
            assert(flock(busy.value, LOCK_EX | LOCK_NB) == 0);
            refuse([&] { repair_persist_context(dir.value); });
        }
        refuse([&] { repair_persist_context(dir.value, [&] { std::ofstream(fstab) << "administrator update\n"; }); });
        assert(observation(fstab, 4096) == "administrator update\n");
        for (const auto &entry : fs::directory_iterator(root))
            assert(entry.path().filename().string().find(".uke-fstab-") != 0);
        fs::remove(fstab); std::ofstream(root / "other-fstab") << content;
        fs::create_symlink(root / "other-fstab", fstab);
        refuse([&] { repair_persist_context(dir.value); });
        fs::remove(fstab); fs::create_hard_link(root / "other-fstab", fstab);
        refuse([&] { repair_persist_context(dir.value); });
        assert(observation(root / "other-fstab", 4096) == content);
        fs::remove(fstab); fs::remove(root / "other-fstab");
        std::cout << "Exact legacy persist mount-view migration, metadata/xattr preservation, custom/duplicate/busy/changed/symlink/hardlink refusal passed; no mount or OEM xattr operation.\n";
        assert(!repair_default_clock_config(dir.value));
        const auto clock_file = root / "adjtime";
        std::ofstream(clock_file) << "0.0 0 0.0\n0\n";
        assert(chmod(clock_file.c_str(), 0640) == 0);
        struct stat clock_before{}, clock_after{};
        assert(stat(clock_file.c_str(), &clock_before) == 0);
        assert(repair_default_clock_config(dir.value));
        assert(observation(clock_file, 32) == "0.0 0 0.0\n0\nUTC\n");
        assert(!repair_default_clock_config(dir.value));
        assert(stat(clock_file.c_str(), &clock_after) == 0);
        assert(clock_before.st_ino == clock_after.st_ino && clock_before.st_mode == clock_after.st_mode);
        std::ofstream(clock_file) << "0.0 0 0.0\n0\n";
        {
            ProfileFd locked(open(clock_file.c_str(), O_RDWR | O_CLOEXEC));
            assert(flock(locked.value, LOCK_EX | LOCK_NB) == 0);
            refuse([&] { repair_default_clock_config(dir.value); });
            assert(observation(clock_file, 32) == "0.0 0 0.0\n0\n");
        }
        for (const auto *value : {"0.0 0 0.0\n0\nLOCAL\n", "1.5 123 0\n123\nUTC\n", "1.5 123 0\n0\n", "custom clock\n"}) {
            std::ofstream(clock_file) << value;
            assert(!repair_default_clock_config(dir.value));
            assert(observation(clock_file, 64) == value);
        }
        fs::remove(clock_file);fs::create_symlink("outside", clock_file);
        std::ofstream(root / "outside") << "0.0 0 0.0\n0\n";
        refuse([&] { repair_default_clock_config(dir.value); });
        assert(observation(root / "outside", 32) == "0.0 0 0.0\n0\n");
        fs::remove(clock_file);fs::create_hard_link(root / "outside", clock_file);
        refuse([&] { repair_default_clock_config(dir.value); });
        assert(observation(root / "outside", 32) == "0.0 0 0.0\n0\n");
        fs::remove(clock_file);
        std::ofstream(clock_file) << "0.0 0 0.0\n0\n";
        assert(chmod(clock_file.c_str(), 0666) == 0);
        refuse([&] { repair_default_clock_config(dir.value); });
        assert(observation(clock_file, 32) == "0.0 0 0.0\n0\n");
        fs::remove(clock_file);
        assert(mkfifo(clock_file.c_str(), 0600) == 0);
        refuse([&] { repair_default_clock_config(dir.value); });
        fs::remove(clock_file);
        std::cout << "Default UTC completion, idempotence, inode/mode retention and custom/unsafe clock-file preservation passed; no RTC access.\n";
        for (const auto *type : {"ext4", "xfs", "btrfs", "f2fs"}) {
            grow_calls = 0;
            grow_once(dir.value, type, simulated_grow);
            grow_once(dir.value, type, simulated_grow);
            assert(grow_calls == (std::string(type) == "f2fs" ? 0 : 1));
            refuse([&] { grow_once(dir.value, "invalid", simulated_grow); });
            assert(unlinkat(dir.value, "online-grow-complete", 0) == 0);
        }
        refuse([&] { grow_once(dir.value, "ext4", +[] { throw std::runtime_error("simulated resize failure"); }); });
        assert(!fs::exists(root / "online-grow-complete"));
        fs::create_symlink("absent", root / "online-grow-complete");
        refuse([&] { grow_once(dir.value, "ext4", simulated_grow); });
        fs::remove(root / "online-grow-complete");
        save_value(dir.value, "format=1\nroot=linux\n");
        assert(stored_profile(dir.value) == uke_install::Profile::linux);
        save_value(dir.value, "format=1\nroot=userdata\n");
        assert(stored_profile(dir.value) == uke_install::Profile::userdata);
        save_value(dir.value, "format=1\nroot=unknown\n");
        refuse([&] { stored_profile(dir.value); });
        save_value(dir.value, "format=1\nroot=linux\nextra=userdata\n");
        refuse([&] { stored_profile(dir.value); });
        save_value(dir.value, std::string(129, 'x'));
        refuse([&] { stored_profile(dir.value); });
        assert(unlinkat(dir.value, "profile", 0) == 0);
        std::ofstream(root / "outside") << "preserved\n";
        assert(symlinkat("outside", dir.value, "profile.tmp") == 0);
        refuse([&] { save_value(dir.value, "format=1\nroot=linux\n"); });
        assert(observation(root / "outside", 32) == "preserved\n");
        assert(unlinkat(dir.value, "profile.tmp", 0) == 0);
        assert(linkat(dir.value, "outside", dir.value, "profile.tmp", 0) == 0);
        refuse([&] { save_value(dir.value, "format=1\nroot=linux\n"); });
        assert(observation(root / "outside", 32) == "preserved\n");
        assert(unlinkat(dir.value, "profile.tmp", 0) == 0);
        std::ofstream(root / "profile.tmp") << "interrupted prior write\n";
        assert(chmod((root / "profile.tmp").c_str(), 0600) == 0);
        save_value(dir.value, "format=1\nroot=linux\n");
        assert(stored_profile(dir.value) == uke_install::Profile::linux);
        assert(chmod((root / "profile").c_str(), 0666) == 0);
        refuse([&] { stored_profile(dir.value); });
        assert(chmod((root / "profile").c_str(), 0600) == 0);
        const auto descriptors = [] {
            size_t count = 0;
            for (const auto &entry : fs::directory_iterator("/proc/self/fd")) { (void)entry; ++count; }
            return count;
        };
        const auto before = descriptors();
        for (unsigned i = 0; i < 10000; ++i) assert(stored_profile(dir.value) == uke_install::Profile::linux);
        assert(descriptors() == before);
        mask_unit(dir.value, "uke-install-profile.service");
        mask_unit(dir.value, "uke-install-profile.service");
        assert(fs::read_symlink(root / "uke-install-profile.service") == "/dev/null");
        std::ofstream(root / "custom.service") << "preserved unit\n";
        refuse([&] { mask_unit(dir.value, "custom.service"); });
        assert(observation(root / "custom.service", 32) == "preserved unit\n");
        std::ofstream(root / "too-large") << std::string(33, 'x');
        refuse([&] { observation(root / "too-large", 32); });
        assert(journal_nocow_unavailable(F2FS_SUPER_MAGIC));
        assert(journal_nocow_unavailable(EXT4_SUPER_MAGIC));
        assert(journal_nocow_unavailable(XFS_SUPER_MAGIC));
        assert(!journal_nocow_unavailable(BTRFS_SUPER_MAGIC));
        assert(!journal_nocow_unavailable(0));
        journal_policy(dir.value, false);
        assert(!fs::exists(root / "tmpfiles.d"));
        const auto override = root / "tmpfiles.d/journal-nocow.conf";
        journal_policy(dir.value, true);journal_policy(dir.value, true);
        assert(fs::read_symlink(override) == journal_override);
        journal_policy(dir.value, false);assert(!fs::is_symlink(override));
        std::ofstream(override) << "administrator rule\n";
        journal_policy(dir.value, true);journal_policy(dir.value, false);
        assert(observation(override, 64) == "administrator rule\n");
        fs::remove(override);fs::create_symlink("/dev/null", override);
        journal_policy(dir.value, true);journal_policy(dir.value, false);
        assert(fs::read_symlink(override) == "/dev/null");
        fs::remove_all(root / "tmpfiles.d");fs::create_directory(root / "outside-dir");
        fs::create_directory_symlink(root / "outside-dir", root / "tmpfiles.d");
        refuse([&] { journal_policy(dir.value, true); });
        assert(fs::is_empty(root / "outside-dir"));
    }
    fs::remove_all(root);
    std::cout << "One-time growth state, resize failure, atomic profile state, stale-write recovery, masks and path-substitution refusals passed; regular files only.\n";
}
