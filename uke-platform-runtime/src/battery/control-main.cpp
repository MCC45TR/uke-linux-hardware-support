// SPDX-License-Identifier: MIT
#include "control.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <thread>
#include <time.h>
#include <unistd.h>

namespace {
using namespace uke::battery::control;
namespace fs = std::filesystem;
volatile sig_atomic_t stopping = 0;
void stop(int) { stopping = 1; }
[[noreturn]] void fail(const char *reason) { throw std::runtime_error(reason); }
uint64_t now() {
    timespec t{}; if (clock_gettime(CLOCK_BOOTTIME, &t)) fail("control_clock_unavailable");
    return uint64_t(t.tv_sec) * 1000000000ULL + uint64_t(t.tv_nsec);
}
std::string read_fd(int fd) {
    std::string data; char buffer[4097];
    for (;;) {
        const auto n = read(fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 || (n > 0 && data.size() + size_t(n) > 4096)) fail("control_metadata_read_failed");
        if (!n) break;
        data.append(buffer, size_t(n));
    }
    return data;
}
std::string read_file(const fs::path &path) {
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) fail("control_metadata_unavailable");
    try { auto data = read_fd(fd); close(fd); return data; }
    catch (...) { close(fd); throw; }
}
fs::path provider(const fs::path &supply) {
    std::error_code error;
    auto node = fs::canonical(supply / "device", error);
    for (int i = 0; !error && i != 5 && node != node.root_path(); ++i) {
        auto driver = fs::canonical(node / "driver", error);
        if (!error && driver.filename() == "pmic_glink_power_supply") return node;
        error.clear(); node = node.parent_path();
    }
    return {};
}
class Sysfs final : public Io {
    int directory_ = -1;
public:
    Sysfs() {
        const fs::path root("/sys/class/power_supply");
        const auto bat = provider(root / "qcom-battmgr-bat"), usb = provider(root / "qcom-battmgr-usb");
        if (bat.empty() || usb != bat || bat.string().rfind("/sys/devices/", 0) != 0) fail("control_provider_unverified");
        const auto compatible = read_file("/sys/firmware/devicetree/base/compatible");
        bool uke = false;
        for (size_t at = 0; at < compatible.size();) {
            auto end = compatible.find('\0', at);
            if (compatible.substr(at, end == std::string::npos ? end : end - at) == "xiaomi,uke") uke = true;
            if (end == std::string::npos) break;
            at = end + 1;
        }
        if (!uke) fail("control_device_mismatch");
        directory_ = open(bat.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (directory_ < 0) fail("control_provider_unavailable");
    }
    ~Sysfs() override { if (directory_ >= 0) close(directory_); }
    std::string read_attribute(const char *name) {
        const int fd = openat(directory_, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) fail("control_backend_unavailable");
        try { auto data = read_fd(fd); close(fd); return data; }
        catch (...) { close(fd); throw; }
    }
    State state() override { return parse_state(read_attribute("uke_charge_state")); }
    void write(const std::string &name, const std::string &value) override {
        if (name != "uke_charge_lease" && name != "uke_charge_thresholds" && name != "uke_charge_refresh" &&
            name != "uke_charge_current_profile" && name != "uke_charge_current_ua")
            fail("control_attribute_forbidden");
        const int fd = openat(directory_, name.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) fail("control_attribute_not_writable");
        // Never retry a failed/interrupted/short sysfs store: delivery may be ambiguous.
        const auto n = ::write(fd, value.data(), value.size());
        const int saved_errno = errno; close(fd);
        if (n != static_cast<ssize_t>(value.size())) {
            errno = saved_errno;
            fail("control_store_failed_no_retry");
        }
    }
};
class Lock {
    int fd_ = -1;
public:
    explicit Lock(const char *name = "owner.lock") {
        if (mkdir("/run/uke-charge-control", 0700) && errno != EEXIST) fail("control_lock_directory_failed");
        const int dir = open("/run/uke-charge-control", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        struct stat st{};
        if (dir < 0 || fstat(dir, &st) || st.st_uid != 0 || (st.st_mode & 0022)) {
            if (dir >= 0) close(dir);
            fail("control_lock_directory_untrusted");
        }
        fd_ = openat(dir, name, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600); close(dir);
        if (fd_ < 0 || fstat(fd_, &st) || !S_ISREG(st.st_mode) || st.st_uid != 0 || (st.st_mode & 0077)) {
            if (fd_ >= 0) close(fd_);
            fail("control_lock_file_untrusted");
        }
        if (flock(fd_, LOCK_EX | LOCK_NB)) { close(fd_); fd_ = -1; fail("control_operation_busy"); }
    }
    ~Lock() { if (fd_ >= 0) close(fd_); }
};
Context context(Sysfs &io) {
    Context c; c.state = io.state(); c.boottime_ns = now(); c.provider_verified = true;
    const auto transport = io.read_attribute("uke_battmgr_state");
    std::istringstream in(transport);
    unsigned version, generation, firmware, source, up, desynced, suspended, removing;
    int error; uint32_t raw; uint64_t receipt, instance;
    if (!(in >> version >> generation >> firmware >> source >> up >> desynced >> suspended >> removing
        >> error >> raw >> receipt >> instance) || version != 1 || up != 1 || desynced || suspended || removing ||
        !(in >> std::ws).eof() || !instance || instance != c.state.instance || firmware != c.state.firmware ||
        source != c.state.source || generation != c.state.generation) fail("control_transport_not_admitted");
    utsname kernel{}; if (uname(&kernel)) fail("control_kernel_identity_unavailable");
    c.kernel_release = kernel.release; c.boot_id = read_file("/proc/sys/kernel/random/boot_id");
    while (!c.boot_id.empty() && (c.boot_id.back() == '\n' || c.boot_id.back() == '\r')) c.boot_id.pop_back();
    return c;
}
void emit(const State &s, const Profile &p, const char *operation, bool applied) {
    std::cout << "{\"schema_version\":1,\"kind\":\"uke_charge_control\",\"operation\":\"" << operation
        << "\",\"backend_compiled\":" << (s.compiled ? "true" : "false")
        << ",\"control_profile_qualified\":" << (p.qualified ? "true" : "false")
        << ",\"configuration_readback_verified\":" << (s.verified ? "true" : "false")
        << ",\"reported_feature_enabled\":" << (s.enabled ? "true" : "false")
        << ",\"reported_start_percent\":" << (s.verified ? std::to_string(s.start) : "null")
        << ",\"reported_end_percent\":" << (s.verified ? std::to_string(s.end) : "null")
        << ",\"start_percent\":" << (s.verified ? std::to_string(s.enabled ? s.start : 0) : "null")
        << ",\"end_percent\":" << (s.verified ? std::to_string(s.enabled ? s.end : 100) : "null")
        << ",\"verified_boottime_ns\":" << s.verified_ns << ",\"applied\":" << (applied ? "true" : "false")
        << ",\"current_backend_compiled\":" << (s.current_compiled ? "true" : "false")
        << ",\"current_readback_verified\":" << (s.current_verified ? "true" : "false")
        << ",\"reported_constant_charge_current_ua\":" << (s.current_verified ? std::to_string(s.current_ua) : "null")
        << ",\"measurement_valid\":" << (s.measurement_valid ? "true" : "false")
        << ",\"battery_temperature_deciC\":" << (s.measurement_valid ? std::to_string(s.temperature_deciC) : "null")
        << ",\"battery_voltage_uv\":" << (s.measurement_valid ? std::to_string(s.voltage_uv) : "null")
        << ",\"battery_current_ua\":" << (s.measurement_valid ? std::to_string(s.measured_current_ua) : "null")
        << ",\"measurement_boottime_ns\":" << s.measurement_ns
        << ",\"physical_charge_stopped\":null,\"current_control_qualified\":" << (p.current_control ? "true" : "false")
        << ",\"voltage_writer\":false,\"cv_owner\":\"firmware\"}\n" << std::flush;
}
uint32_t percent(const char *value) {
    uint32_t result; const std::string text(value);
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result > 100)
        fail("control_percentage_invalid");
    return result;
}
}
int main(int argc, char **argv) {
    std::unique_ptr<Sysfs> io;
    std::unique_ptr<Session> session;
    try {
        if (argc == 2 && std::string(argv[1]) == "capabilities") {
            std::cout << "{\"schema_version\":1,\"kind\":\"uke_charge_control_capabilities\",\"threshold_transport_implemented\":true,\"current_transport_implemented\":true,\"automatic_policy_implemented\":true,\"qualification_required\":true,\"active_qualification\":null,\"voltage_writer\":false,\"physical_charge_stopped\":null}\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "uke-charge-control capabilities | status\n"
                << "uke-charge-control verify | cap80 | band80-90 | threshold-off | pair START END | serve\n"
                << "uke-charge-control slow | automatic | profile-restore\n"
                << "Actions require root, a reviewed control profile and matching per-boot evidence.\n"
                << "Positive FCC only; CV remains firmware-owned. No PD negotiation or electrical cutoff.\n";
            return 0;
        }
        if (argc < 2) fail("control_command_required");
        const std::string command(argv[1]);
        if ((command == "pair" && argc != 4) || (command != "pair" && argc != 2)) fail("control_arguments_invalid");
        if (command != "status" && command != "verify" && command != "cap80" && command != "band80-90" &&
            command != "pair" && command != "serve" && command != "threshold-off" &&
            command != "slow" && command != "automatic" && command != "profile-restore") fail("control_command_unsupported");
        io = std::make_unique<Sysfs>();
        if (command == "status") { emit(io->state(), {}, "metadata_only", false); return 0; }
        if (geteuid() != 0) fail("control_root_required");
        const auto initial = context(*io);
        const auto profile = resolve(initial);
        if (!profile.qualified) throw std::runtime_error(profile.reason);
        if (command == "serve" && !profile.standard_clients) fail("control_standard_clients_not_admitted");
        const bool current_action = command == "slow" || command == "automatic" || command == "profile-restore";
        if (current_action && !profile.current_control) fail("control_current_unqualified");
        // One automatic/current policy writer; threshold clients still serialize
        // through the per-operation lock and the kernel request mutex.
        std::unique_ptr<Lock> policy_lock;
        if (current_action) policy_lock = std::make_unique<Lock>("policy.lock");
        session = std::make_unique<Session>(*io, profile, initial.state);
        if (command == "serve" || command == "automatic") {
            signal(SIGTERM, stop); signal(SIGINT, stop);
            Automatic automatic;
            while (!stopping) {
                {
                    Lock lock;
                    const auto current = resolve(context(*io));
                    if (!current.qualified || current.evidence != profile.evidence || current.expires_ns != profile.expires_ns)
                        fail("control_qualification_changed");
                    auto result = session->refresh(now());
                    bool applied = false;
                    if (command == "automatic") {
                        const auto proposal = automatic.propose(profile, result, now());
                        if (proposal != result.current_ua) {
                            result = session->current(proposal, now()); applied = true;
                        }
                    }
                    emit(result, profile, command == "automatic" ? "automatic_current" : "verified_refresh", applied);
                }
                const auto seconds = std::chrono::seconds(profile.lease_ns / 3000000000ULL);
                const auto until = std::chrono::steady_clock::now() + seconds;
                while (!stopping && std::chrono::steady_clock::now() < until)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            Lock lock;
            State result;
            if (command == "verify") result = session->refresh(now());
            else if (current_action) {
                const auto observed = session->refresh(now());
                const auto ceiling = current_ceiling(profile, observed, now());
                const auto requested = command == "slow" ? std::min({profile.slow_ua, ceiling, observed.current_ua}) : profile.restore_ua;
                if (requested > ceiling) fail("control_restore_exceeds_current_band");
                result = session->current(requested, now());
            }
            else {
                const auto start = command == "pair" ? percent(argv[2]) : command == "cap80" ? profile.cap_resume_percent : command == "threshold-off" ? 0 : 80;
                const auto end = command == "pair" ? percent(argv[3]) : command == "cap80" ? 80 : command == "threshold-off" ? 100 : 90;
                result = session->thresholds(start, end, now());
            }
            emit(result, profile, command == "verify" ? "verified_refresh" : current_action ?
                (command == "slow" ? "slow_current" : "profile_current_restore") : "threshold_apply", command != "verify");
        }
        session->revoke(); session.reset(); return 0;
    } catch (const std::exception &e) {
        const bool uncertain = session && session->write_attempted();
        std::string reason(e.what());
        for (auto &ch : reason) if (!(ch >= 'a' && ch <= 'z') && !(ch >= 'A' && ch <= 'Z') &&
            !(ch >= '0' && ch <= '9') && ch != '_' && ch != '-') ch = '_';
        session.reset();
        std::cout << "{\"schema_version\":1,\"kind\":\"uke_charge_control\",\"status\":\"failed\",\"reason\":\""
            << reason << "\",\"applied\":" << (uncertain ? "null" : "false")
            << ",\"write_delivery_may_be_uncertain\":" << (uncertain ? "true" : "false")
            << ",\"physical_charge_stopped\":null}\n";
        return uncertain ? 1 : 69;
    }
}
