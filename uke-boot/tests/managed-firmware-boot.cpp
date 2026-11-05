// SPDX-License-Identifier: MIT
// Exercise the actual updater boot flow with synthetic firmware/state callbacks.
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include <functional>

namespace {
unsigned checks = 0;
void check(bool value, const char *reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
void scenario(bool managed, bool enabled, bool changed, unsigned failure) {
    const fs::path running = fs::path("/installed") /
        (managed ? "7.3.1-12204-unstable" : "7.3.1-1500");
    const fs::path next = fs::path("/installed") /
        (managed ? "7.3.1-12205-unstable" : "7.3.1-1600");
    std::vector<std::string> events;
    unsigned accepted = 0, applied = 0, state_writes = 0;
    bool failed = false;
    const auto prepare = [&](const std::string &release) {
        check(release == running.filename().string(), "wrong boot firmware target");
        events.push_back("prepare");
        if (failure == 1) throw std::runtime_error("injected boot prepare failure");
    };
    const auto verify = [&](const std::string &release) {
        check(release == running.filename().string(), "wrong boot verification target");
        events.push_back("verify");
        if (failure == 2) throw std::runtime_error("injected boot verify failure");
    };
    const auto accept = [&](const fs::path &path, const Bytes &bytes) {
        check(path == running && bytes == Bytes{1}, "boot acceptance identity changed");
        events.push_back("accept"); ++accepted; ++state_writes;
    };
    try {
        boot_userdata(running,
            [&](const fs::path &path) {
                check(path == running || path == next, "unexpected boot validation target");
                events.push_back(path == running ? "validate-running" : "validate-next");
                return Bytes{static_cast<unsigned char>(path == running ? 1 : 2)};
            },
#ifdef UKE_MANAGED_UNGATED_BOOT
            // Actual 1.8.96 boot acceptance callback, before the missing gate.
            accept,
#else
            uke_firmware::prepared_accept(false, prepare, verify, accept),
#endif
            [&] { events.push_back("policy"); return enabled; },
            [&] { events.push_back("select"); return changed ? next : running; },
            [&](const fs::path &path, const Bytes &bytes) {
                check(path == (changed ? next : running) &&
                      bytes == Bytes{static_cast<unsigned char>(changed ? 2 : 1)},
                      "boot inspection identity changed");
                events.push_back("inspect");
            },
            [&](const fs::path &path, const Bytes &bytes) {
                check(path == next && bytes == Bytes{2}, "boot application identity changed");
                events.push_back("apply"); ++applied; ++state_writes;
            });
    } catch (const std::exception &) { failed = true; }
    if (managed && failure == 1)
        check(!accepted && !applied && !state_writes,
              "boot firmware preparation failure reached acceptance");
    if (managed && failure == 2)
        check(!accepted && !applied && !state_writes,
              "boot firmware verification failure reached acceptance");
    std::vector<std::string> expected{"validate-running"};
    if (managed) expected.push_back("prepare");
    if (managed && failure != 1) expected.push_back("verify");
    const bool refused = managed && failure;
    if (!refused) {
        expected.push_back("accept"); expected.push_back("policy");
        if (enabled) {
            expected.push_back("select");
            if (changed) expected.push_back("validate-next");
            expected.push_back("inspect");
            if (changed) expected.push_back("apply");
        }
    }
    check(events == expected, "actual boot firmware/acceptance order differs");
    check(failed == refused, "actual boot failure propagation differs");
    check(accepted == (refused ? 0u : 1u) &&
          applied == (!refused && enabled && changed ? 1u : 0u), "boot callbacks differ");
#ifdef UKE_MANAGED_UNGATED_BOOT
    (void)prepare; (void)verify;
#endif
}
void baseline() {
    const fs::path current = "/installed/7.3.1-12204-unstable";
    for (bool dry : {false, true}) for (unsigned failure : {0u, 1u, 2u}) {
        unsigned prepared = 0, verified = 0, accepted = 0;
        bool failed = false;
        try {
            uke_firmware::prepared_accept(dry,
                [&](const std::string &) { ++prepared; if (failure == 1) throw std::runtime_error("prepare"); },
                [&](const std::string &) { ++verified; if (failure == 2) throw std::runtime_error("verify"); },
                [&](const fs::path &path, const Bytes &bytes) {
                    check(path == current && bytes == Bytes{3}, "baseline acceptance identity changed");
                    ++accepted;
                })(current, Bytes{3});
        } catch (const std::exception &) { failed = true; }
        const bool refused = (!dry && failure == 1) || failure == 2;
        check(prepared == (dry ? 0u : 1u) && verified == (!dry && failure == 1 ? 0u : 1u),
              "baseline firmware check order differs");
        check(failed == refused && accepted == (refused ? 0u : 1u), "baseline failure reached acceptance");
    }
}
}
int main() {
    try {
        scenario(true, true, false, 1);
        scenario(true, true, false, 2);
        for (bool managed : {false, true}) for (bool enabled : {false, true})
            for (bool changed : {false, true}) for (unsigned failure : {0u, 1u, 2u})
                scenario(managed, enabled, changed, failure);
        baseline();
        std::cout << "Actual boot_userdata and managed baseline: " << checks
                  << " checks; firmware failure prevents acceptance/state callbacks, same/changed selections and legacy recovery covered; synthetic providers only\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
