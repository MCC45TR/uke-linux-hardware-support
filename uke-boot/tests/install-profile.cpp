// SPDX-License-Identifier: MIT
#define UKE_INSTALL_PROFILE_TEST
#include "../src/install-profile.cpp"
#include <cassert>
#include <fstream>
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
    }
    fs::remove_all(root);
    std::cout << "Atomic profile state, stale-write recovery, masks and path-substitution refusals passed; regular files only.\n";
}
