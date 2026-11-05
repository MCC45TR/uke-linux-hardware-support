// SPDX-License-Identifier: MIT
// Exercise the production firmware gate and the retained private-color boundary.
#include "../src/managed-firmware.hpp"
#include "../src/o82-private-color.hpp"
#include <iostream>
#include <vector>

namespace {
unsigned checks = 0;
void check(bool value, const char *reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
template<class Operation> void rejected(Operation operation) {
    bool failed = false;
    try { operation(); } catch (const std::exception &) { failed = true; }
    check(failed, "invalid firmware admission was accepted");
}
void scenario(bool managed, bool dry, bool fail_prepare, bool fail_verify, bool fail_color) {
    const std::string target = managed ? "7.3.1-12205-unstable" : "7.3.1-10000-unstable";
    std::map<std::string, std::string> manifest{{"kernel_release", target}};
    if (managed) manifest["o82_private_color"] = "5";
    std::vector<std::string> events;
    unsigned image_writes = 0, selection_writes = 0, state_writes = 0;
    bool failed = false;
    const auto prepare = [&](const std::string &release) {
        check(release == target && managed && !dry, "wrong prepare target or dry mutation");
        events.push_back("firmware-prepare");
        if (fail_prepare) throw std::runtime_error("injected firmware preparation failure");
    };
    const auto verify = [&](const std::string &release) {
        check(release == target && managed, "wrong verification target");
        events.push_back("firmware-verify");
        if (fail_verify) throw std::runtime_error("injected firmware verification failure");
    };
    const auto activate = [&] {
        uke_color::with_prepared(manifest, "7.3.1-12204-unstable", dry,
            [&](const std::string &release) {
                check(release == target, "wrong retained private-color target");
                events.push_back("color-prepare");
                if (fail_color) throw std::runtime_error("injected private-color failure");
            }, [&] {
                events.push_back("activate");
                if (!dry) { ++image_writes; ++selection_writes; ++state_writes; }
            });
    };
    try {
#ifdef UKE_MANAGED_OLD_BOUNDARY
        // This is exactly the original 1.8.95 production boundary: only color.
        (void)prepare; (void)verify;
        activate();
#else
        uke_firmware::with_prepared(manifest, dry, prepare, verify, activate);
#endif
    } catch (const std::exception &) { failed = true; }
    const bool preparation_refused = managed && !dry && fail_prepare;
    const bool verification_refused = managed && !preparation_refused && fail_verify;
    const bool color_refused = managed && !dry && !preparation_refused && !verification_refused && fail_color;
    if (preparation_refused)
        check(!image_writes && !selection_writes && !state_writes,
              "firmware preparation failure reached activation");
    if (verification_refused)
        check(!image_writes && !selection_writes && !state_writes,
              "firmware verification failure reached activation");
    std::vector<std::string> expected;
    if (managed && !dry) expected.push_back("firmware-prepare");
    if (managed && !preparation_refused) expected.push_back("firmware-verify");
    if (managed && !dry && !preparation_refused && !verification_refused) expected.push_back("color-prepare");
    if (!preparation_refused && !verification_refused && !color_refused) expected.push_back("activate");
    check(events == expected, "firmware/color/activation order differs");
    check(failed == (preparation_refused || verification_refused || color_refused), "failure propagation differs");
    if (dry) check(!image_writes && !selection_writes && !state_writes, "dry check changed boot state");
}
void contract() {
    // Required first negative: original 1.8.95 stages despite failed preparation.
    scenario(true, false, true, false, false);
    scenario(true, false, false, true, false);
    for (bool managed : {false, true}) for (bool dry : {false, true})
        for (bool preparation : {false, true}) for (bool verification : {false, true})
            for (bool color : {false, true}) scenario(managed, dry, preparation, verification, color);
    for (const char *action : {"auto", "transaction", "apply", "apply-current", "replace-trial", "select-slot", "boot"})
        check(!uke_firmware::dry_action(action), "mutating action was classified read-only");
    for (const char *action : {"check", "check-auto", "check-current", "check-trial", "check-select-slot",
                              "accept", "fallback", "restore-current", "check-restore-current"})
        check(uke_firmware::dry_action(action), "verification/recovery action would prepare firmware");
    for (const char *action : {"", "AUTO", "applyx", "transaction\n", "../auto"})
        rejected([&] { (void)uke_firmware::dry_action(action); });
    for (const char *target : {"7.3.1-11003-unstable", "7.3.1-12204-unstable", "7.3.2-1-unstable", "8.0.0-1-unstable"})
        check(uke_firmware::required({{"kernel_release", target}}), "managed or future cohort escaped verification");
    for (const char *target : {"7.3.1-1500", "7.2.9-20-unstable", "7.3.0-99999-unstable", "7.3.1-11002-unstable"})
        check(!uke_firmware::required({{"kernel_release", target}}), "legacy recovery gained a new provider dependency");
    for (const char *target : {"-unstable", "7.3-1-unstable", "7.3.1.0-1-unstable", "07.3.1-1-unstable",
                              "7.03.1-1-unstable", "7.3.1-0-unstable", "7.3.1-01-unstable", "7.3.1-+1-unstable",
                              "7.3.1-4294967296-unstable", "7.3.1-1/2-unstable", "7.3.1--unstable"})
        rejected([&] { (void)uke_firmware::required({{"kernel_release", target}}); });
    check(!uke_firmware::required({}), "accepted recovery acquired a new provider dependency");
    unsigned called = 0;
    check(uke_firmware::with_prepared({}, false, [&](const auto &) { ++called; },
        [&](const auto &) { ++called; }, [] { return 37; }) == 37 && !called, "legacy return value changed");
    rejected([&] {
        uke_firmware::with_prepared({{"kernel_release", "8.0.0-1-unstable"}}, false,
            [](const auto &) { throw std::runtime_error("provider rejects unsupported future ABI"); },
            [&](const auto &) { ++called; }, [&] { ++called; });
    });
    check(!called, "unsupported provider proceeded to verification or activation");
}
}
int main() {
    try {
        contract();
        std::cout << "Actual firmware/private-color gate: " << checks
                  << " checks; preparation/verification failures prevent image/selection/state callbacks, dry and legacy recovery remain unchanged; synthetic providers only\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
