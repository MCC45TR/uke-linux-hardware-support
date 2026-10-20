// SPDX-License-Identifier: MIT
#pragma once
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace uke_install {
inline void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
enum class Profile { linux, userdata };
inline const char *name(Profile value) { return value == Profile::linux ? "linux" : "userdata"; }
struct RootMount { unsigned major, minor; std::string filesystem; };
inline unsigned decimal(std::string_view value) {
    unsigned result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    require(!value.empty() && parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size(), "Invalid root device number");
    return result;
}
inline RootMount root_mount(const std::string &text) {
    require(!text.empty() && text.size() <= 1024 * 1024, "Missing or oversized mount table");
    std::istringstream input(text);
    std::string line;
    RootMount result{};
    bool found = false;
    while (std::getline(input, line)) {
        require(line.size() <= 8192, "Oversized mount row");
        std::istringstream row(line);
        std::string id, parent, device, root, mount, options, field, filesystem;
        require(bool(row >> id >> parent >> device >> root >> mount >> options), "Incomplete mount row");
        if (mount != "/") continue;
        require(!found && root == "/", "Ambiguous or partial root mount");
        // ProtectSystem=strict exposes the physical root read-only to the classifier.
        const auto flags = "," + options + ",";
        require((flags.find(",rw,") != std::string::npos) != (flags.find(",ro,") != std::string::npos), "Invalid root mount access mode");
        while (row >> field && field != "-") {}
        require(field == "-" && bool(row >> filesystem) && (filesystem == "ext4" || filesystem == "f2fs"), "Physical ext4 or F2FS root required");
        const auto colon = device.find(':');
        require(colon != std::string::npos && device.find(':', colon + 1) == std::string::npos, "Invalid root device");
        result = {decimal(std::string_view(device).substr(0, colon)), decimal(std::string_view(device).substr(colon + 1)), filesystem};
        require(result.major != 0, "Virtual root refused");
        found = true;
    }
    require(found, "Root mount not found");
    return result;
}
inline Profile classify(const std::string &uevent, const std::string &cmdline, bool physical_ufs) {
    require(physical_ufs, "Measured Uke physical UFS root required");
    require(uevent.size() <= 8192 && cmdline.size() <= 8192, "Oversized root observation");
    std::istringstream rows(uevent), arguments(cmdline);
    std::string row, partname, selector;
    bool named = false;
    while (std::getline(rows, row)) {
        if (row.rfind("PARTNAME=", 0) != 0) continue;
        require(!named, "Duplicate partition name");
        named = true;
        partname = row.substr(9);
    }
    require(partname == "linux" || partname == "userdata", "Unsupported root partition");
    while (arguments >> row) {
        if (row.rfind("root=", 0) != 0) continue;
        require(selector.empty(), "Multiple root selectors");
        selector = row;
    }
    require(selector == "root=PARTLABEL=" + partname, "Mounted root and boot selector differ");
    return partname == "linux" ? Profile::linux : Profile::userdata;
}
inline bool release_token(const std::string &value) {
    return !value.empty() && value.size() <= 127 && value.front() != '.' &&
        value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._+-") == std::string::npos;
}
enum class Slot { a, b };
inline Slot opposite(Slot value) {
    require(value == Slot::a || value == Slot::b, "Invalid boot slot");
    return value == Slot::a ? Slot::b : Slot::a;
}
enum class Phase { stable, staging, verified, trial };
struct Update {
    Profile profile;
    Slot good;
    std::string good_release;
    Phase phase = Phase::stable;
    std::string candidate;
    std::string failed;
};
inline void userdata(const Update &state) {
    require(state.profile == Profile::userdata, "Alternating updates disabled for Linux root");
    require(release_token(state.good_release), "Invalid accepted kernel release");
    (void)opposite(state.good);
    if (state.phase != Phase::stable) require(release_token(state.candidate) && state.candidate != state.good_release, "Invalid candidate release");
}
inline Slot stage(Update &state, Slot running, const std::string &running_release, const std::string &candidate) {
    userdata(state);
    require(release_token(state.good_release) && release_token(running_release) && release_token(candidate), "Invalid kernel release");
    require(state.phase == Phase::stable, "A kernel transaction is already outstanding");
    require(running == state.good && running_release == state.good_release, "Running slot does not match the accepted baseline");
    require(candidate != running_release && candidate != state.failed, "Same or previously failed release refused");
    state.candidate = candidate;
    state.phase = Phase::staging;
    return opposite(running);
}
inline void verified(Update &state) {
    userdata(state);
    require(state.phase == Phase::staging, "Only a staging transaction can be verified");
    state.phase = Phase::verified;
}
inline Slot arm(Update &state) {
    userdata(state);
    require(state.phase == Phase::verified, "Verify the inactive image before selecting it");
    state.phase = Phase::trial;
    return opposite(state.good);
}
inline void accept(Update &state, Slot running, const std::string &release, bool healthy) {
    userdata(state);
    require(state.phase == Phase::trial && running == opposite(state.good) && release == state.candidate && healthy,
            "Only the matching healthy trial may be accepted");
    state.good = running;
    state.good_release = state.candidate;
    state.candidate.clear();
    state.phase = Phase::stable;
}
inline void returned(Update &state, Slot running, const std::string &release) {
    userdata(state);
    require(state.phase == Phase::trial && running == state.good && release == state.good_release,
            "Fallback must match the accepted baseline");
    state.failed = state.candidate;
    state.candidate.clear();
    state.phase = Phase::stable;
}
} // namespace uke_install
