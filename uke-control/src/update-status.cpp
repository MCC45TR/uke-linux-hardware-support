// SPDX-License-Identifier: MIT
// Read-only observations of the existing Uke kernel-update/slot contracts.
// QTI flag meanings match uke-boot/src/slot-control.hpp; no writer is linked.
#include <json/json.h>
#include <libfdisk/libfdisk.h>
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>

namespace uke::update_status {
namespace fs = std::filesystem;
using J = Json::Value;
constexpr std::uint64_t capacity = 96ULL * 1024 * 1024;
constexpr std::uint64_t active = 1ULL << 50, successful = 1ULL << 54, unbootable = 1ULL << 55;
void need(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
struct Fd {
    int value;
    explicit Fd(int fd):value(fd) { need(fd >= 0, "Cannot open a required status source"); }
    ~Fd() { close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};
bool token(const std::string &text) {
    return !text.empty() && text.size() <= 128 && text.front() != '.' &&
        text.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._+-") == text.npos;
}
bool digest_value(const std::string &text) {
    return text.size() == 64 && text.find_first_not_of("0123456789abcdef") == text.npos;
}
std::string text(const fs::path &path, std::size_t limit = 8192) {
    Fd fd(open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    struct stat info{};
    need(!fstat(fd.value, &info) && S_ISREG(info.st_mode), "Status source is not a regular file");
    std::string result; std::array<char, 4096> buffer{};
    while (true) {
        const auto n = read(fd.value, buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR) continue;
        need(n >= 0, "Cannot read status source");
        if (!n) break;
        need(result.size() + std::size_t(n) <= limit, "Status source exceeds its size bound");
        result.append(buffer.data(), std::size_t(n));
    }
    return result;
}
std::string sha_final(EVP_MD_CTX *ctx) {
    std::array<unsigned char, 32> bytes{}; unsigned size = 0;
    need(EVP_DigestFinal_ex(ctx, bytes.data(), &size) == 1 && size == bytes.size(), "Cannot finish kernel identity check");
    std::string out; constexpr char digits[] = "0123456789abcdef";
    for (const auto b : bytes) { out += digits[b >> 4]; out += digits[b & 15]; }
    return out;
}
using Hash = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
Hash hash() {
    Hash ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    need(ctx && EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) == 1, "Cannot start kernel identity check");
    return ctx;
}
std::string hash_file(const fs::path &path) {
    Fd fd(open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    struct stat info{};
    need(!fstat(fd.value, &info) && S_ISREG(info.st_mode) && info.st_size > 64 &&
         std::uint64_t(info.st_size) < capacity, "Installed running kernel Image is unavailable");
    auto ctx = hash(); std::array<unsigned char, 65536> buffer{};
    std::uint64_t total = 0;
    while (true) {
        const auto n = read(fd.value, buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR) continue;
        need(n >= 0, "Cannot read installed running Image");
        if (!n) break;
        total += std::uint64_t(n); need(total < capacity, "Installed Image exceeds its bound");
        need(EVP_DigestUpdate(ctx.get(), buffer.data(), std::size_t(n)) == 1, "Cannot identify running Image");
    }
    return sha_final(ctx.get());
}
std::uint32_t le32(const unsigned char *p) {
    return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;
}
void read_at(int fd, std::uint64_t offset, unsigned char *out, std::size_t size) {
    need(offset <= capacity && size <= capacity - offset, "Boot read exceeds its bound");
    while (size) {
        const auto n = pread(fd, out, size, static_cast<off_t>(offset));
        if (n < 0 && errno == EINTR) continue;
        need(n > 0, "Incomplete boot partition read");
        offset += std::uint64_t(n); out += n; size -= std::size_t(n);
    }
}
struct Image { std::string release, kernel_hash, boot_hash; };
Image image(int fd) {
    std::array<unsigned char, 4096> header{}; read_at(fd, 0, header.data(), header.size());
    need(!memcmp(header.data(), "ANDROID!", 8) && le32(header.data()+40) == 4 &&
         le32(header.data()+20) == 1584 && !le32(header.data()+12) && !le32(header.data()+1580),
         "Unsupported boot image; a kernel-only Android v4 image is required");
    const std::uint64_t length = le32(header.data()+8);
    need(length >= 64 && length <= capacity - header.size(), "Invalid boot kernel size");
    auto kernel = hash(), whole = hash();
    need(EVP_DigestUpdate(whole.get(), header.data(), header.size()) == 1, "Cannot identify boot image");
    std::array<unsigned char, 65536> bytes{}; std::string tail; std::set<std::string> releases;
    for (std::uint64_t offset = header.size(); offset < capacity;) {
        const auto count = std::size_t(std::min<std::uint64_t>(bytes.size(), capacity-offset));
        read_at(fd, offset, bytes.data(), count);
        need(EVP_DigestUpdate(whole.get(), bytes.data(), count) == 1, "Cannot identify boot image");
        const auto kernel_count = offset < header.size()+length ?
            std::size_t(std::min<std::uint64_t>(count, header.size()+length-offset)) : 0;
        if (kernel_count) {
            if (offset == header.size()) need(bytes[0] == 'M' && bytes[1] == 'Z' &&
                le32(bytes.data()+56) == 0x644d5241, "Unsupported uncompressed ARM64 kernel Image");
            need(EVP_DigestUpdate(kernel.get(), bytes.data(), kernel_count) == 1, "Cannot identify kernel Image");
            const std::string scan = tail + std::string(reinterpret_cast<const char*>(bytes.data()), kernel_count);
            std::size_t pos = 0;
            while ((pos = scan.find("Linux version ", pos)) != scan.npos) {
                const auto start = pos + 14; auto end = start;
                while (end < scan.size() && scan[end] != ' ' && scan[end] != '\0' && scan[end] != '\n') ++end;
                if (end < scan.size()) {
                    const auto release = scan.substr(start, end-start);
                    if (token(release) && release.front() >= '0' && release.front() <= '9') releases.insert(release);
                }
                pos = start;
            }
            tail = scan.substr(scan.size() > 160 ? scan.size()-160 : 0);
        }
        offset += count;
    }
    need(releases.size() == 1, "Kernel release banner is missing or ambiguous");
    return {*releases.begin(), sha_final(kernel.get()), sha_final(whole.get())};
}
struct Slot { std::optional<Image> boot; std::optional<std::uint64_t> flags; };
struct Saved { int good = -1; std::string release, hash, phase, candidate, failed; bool baseline_approved=true; };
Saved saved(const std::string &input) {
    need(input.size() <= 8192, "Oversized kernel approval record");
    std::map<std::string, std::string> fields; std::istringstream lines(input); std::string line;
    while (std::getline(lines, line)) {
        const auto pos = line.find('=');
        need(pos != line.npos && fields.emplace(line.substr(0,pos),line.substr(pos+1)).second,
             "Malformed kernel approval record");
    }
    std::set<std::string> required{"format","good_slot","good_release","good_image","phase","candidate","candidate_image","failed","staged_boot_id"};
    const bool checkpoint=fields.contains("format") && fields.at("format")=="2";
    if (checkpoint) required.emplace("baseline_approved");
    need(fields.size() == required.size(), "Unsupported kernel approval record fields");
    for (const auto &key : required) need(fields.contains(key), "Missing kernel approval record field");
    need((fields.at("format") == "1" || (checkpoint && fields.at("baseline_approved")=="0")) &&
         (fields.at("good_slot") == "0" || fields.at("good_slot") == "1"), "Unsupported kernel approval record");
    Saved out; out.good = fields.at("good_slot") == "0" ? 0 : 1;
    out.baseline_approved=!checkpoint;
    out.release = fields.at("good_release"); out.hash = fields.at("good_image"); out.phase = fields.at("phase");
    need(token(out.release) && digest_value(out.hash), "Invalid accepted kernel identity");
    need(out.phase == "stable" || out.phase == "staging" || out.phase == "verified" ||
         out.phase == "trial" || out.phase == "current-slot-trial", "Unknown kernel update phase");
    need(out.baseline_approved || out.phase!="current-slot-trial", "Unapproved checkpoint cannot authorize an in-place trial");
    for (const auto *key : {"candidate","failed"}) need(fields.at(key) == "-" || token(fields.at(key)), "Invalid kernel release in approval record");
    out.candidate = fields.at("candidate") == "-" ? "" : fields.at("candidate");
    out.failed = fields.at("failed") == "-" ? "" : fields.at("failed");
    if (out.phase == "stable") need(out.candidate.empty() && fields.at("candidate_image") == "-" && fields.at("staged_boot_id") == "-", "Inconsistent stable approval record");
    else need(!out.candidate.empty() && digest_value(fields.at("candidate_image")) && fields.at("staged_boot_id").size() == 36, "Incomplete candidate approval record");
    return out;
}
std::optional<int> boot_slot(const std::string &input) {
    std::istringstream words(input); std::string word; std::optional<int> result;
    while (words >> word) if (word.starts_with("androidboot.slot_suffix=")) {
        need(!result && (word == "androidboot.slot_suffix=_a" || word == "androidboot.slot_suffix=_b"), "Malformed or duplicate bootloader slot");
        result = word.back() == 'a' ? 0 : 1;
    }
    return result;
}
std::array<std::uint64_t,2> gpt_flags(int fd) {
    std::unique_ptr<fdisk_context, decltype(&fdisk_unref_context)> ctx(fdisk_new_context(), fdisk_unref_context);
    need(bool(ctx), "Cannot inspect GPT slot metadata");
    bool warning = false;
    fdisk_set_ask(ctx.get(), [](fdisk_context*, fdisk_ask *ask, void *data) {
        if (fdisk_is_ask(ask, WARN) || fdisk_is_ask(ask, WARNX)) *static_cast<bool*>(data) = true;
        return fdisk_is_ask(ask, INFO) || fdisk_is_ask(ask, WARN) || fdisk_is_ask(ask, WARNX) ? 0 : -EINVAL;
    }, &warning);
    fdisk_disable_dialogs(ctx.get(), 1);
    need(!fdisk_assign_device_by_fd(ctx.get(), fd, "Uke boot metadata", 1) && fdisk_is_label(ctx.get(), GPT), "Cannot read Uke GPT");
    fdisk_table *raw = nullptr; need(!fdisk_get_partitions(ctx.get(), &raw), "Cannot read GPT partitions");
    std::unique_ptr<fdisk_table, decltype(&fdisk_unref_table)> table(raw, fdisk_unref_table);
    std::unique_ptr<fdisk_iter, decltype(&fdisk_free_iter)> iter(fdisk_new_iter(FDISK_ITER_FORWARD), fdisk_free_iter);
    need(table && iter, "Cannot allocate GPT status reader");
    std::array<std::uint64_t,2> flags{}; std::array<bool,2> found{}; fdisk_partition *part = nullptr;
    while (!fdisk_table_next_partition(table.get(), iter.get(), &part)) {
        const char *name = fdisk_partition_get_name(part); if (!name) continue;
        const std::string key = name; if (key != "boot_a" && key != "boot_b") continue;
        const unsigned index = key == "boot_a" ? 0 : 1;
        need(!found[index] && !fdisk_gpt_get_partition_attrs(ctx.get(), fdisk_partition_get_partno(part), &flags[index]), "Ambiguous GPT boot partition");
        found[index] = true;
    }
    need(found[0] && found[1] && !warning, "Both boot slots and warning-free GPT metadata are required");
    return flags;
}
J slot_name(std::optional<int> slot) { return slot ? J(*slot ? "b" : "a") : J(); }
J nullable(const std::string &value) { return value.empty() ? J() : J(value); }
J report(const std::array<Slot,2> &slots, const std::string &running, const std::string &running_hash,
         std::optional<int> declared, std::optional<int> next, const std::optional<Saved> &record, J issues) {
    J out; out["schema"] = "org.uke.control.update-status.v1"; out["available"] = true; out["read_only"] = true;
    out["running_kernel"] = running; std::optional<int> current, selected;
    if (declared && slots[*declared].boot && !running_hash.empty() &&
        slots[*declared].boot->kernel_hash == running_hash && slots[*declared].boot->release == running) current = declared;
    else if (!declared && !running_hash.empty()) {
        const bool a = slots[0].boot && slots[0].boot->kernel_hash == running_hash && slots[0].boot->release == running;
        const bool b = slots[1].boot && slots[1].boot->kernel_hash == running_hash && slots[1].boot->release == running;
        if (a != b) current = b ? 1 : 0;
    }
    if (!current) issues.append("The running slot cannot be proved from the bootloader and installed running Image.");
    if (slots[0].flags && slots[1].flags) {
        const bool a = (*slots[0].flags & active) != 0, b = (*slots[1].flags & active) != 0;
        if (a != b) selected = b ? 1 : 0;
    }
    const bool consistent = next && selected && *next == *selected;
    if (!consistent) issues.append("UFS and GPT do not establish one consistent next boot slot.");
    out["current_slot"] = slot_name(current); out["next_boot_slot"] = slot_name(next); out["gpt_selected_slot"] = slot_name(selected);
    out["current_slot_source"] = current ? (declared ? "bootloader_and_kernel_hash" : "unique_running_kernel_hash") : "unknown";
    out["pending_slot_switch"] = current && consistent ? J(*current != *next) : J();
    out["slots"] = J(Json::arrayValue);
    for (unsigned i = 0; i < 2; ++i) {
        J item; item["slot"] = i ? "b" : "a";
        item["kernel_release"] = slots[i].boot ? J(slots[i].boot->release) : J();
        item["kernel_source"] = slots[i].boot ? "boot_image_linux_banner" : "unavailable";
        item["matches_running_kernel"] = slots[i].boot && !running_hash.empty() ? J(slots[i].boot->kernel_hash == running_hash) : J();
        item["boot_successful"] = slots[i].flags ? J(bool(*slots[i].flags & successful)) : J();
        item["bootable"] = slots[i].flags ? J(!(*slots[i].flags & unbootable)) : J();
        item["selected_for_boot"] = slots[i].flags ? J(bool(*slots[i].flags & active)) : J();
        out["slots"].append(item);
    }
    J approval; approval["state"] = "unknown"; approval["latest_update_approved"] = J(); approval["running_kernel_approved"] = J();
    approval["accepted_kernel_verified"] = J(); approval["history_note"] = J();
    approval["accepted_kernel"] = J(); approval["accepted_slot"] = J(); approval["candidate_kernel"] = J();
    approval["last_failed_kernel"] = J(); approval["phase"] = J();
    if (record) {
        const auto &s = slots[record->good];
        const bool accepted = record->baseline_approved && s.boot && s.flags && s.boot->release == record->release &&
            s.boot->boot_hash == record->hash && (*s.flags & successful) && !(*s.flags & unbootable);
        approval["baseline_approved"]=record->baseline_approved;
        approval["baseline_kernel"]=record->release; approval["baseline_slot"]=slot_name(record->good);
        if (record->baseline_approved) {
            approval["accepted_kernel"] = record->release; approval["accepted_slot"] = slot_name(record->good);
        }
        approval["candidate_kernel"] = nullable(record->candidate); approval["last_failed_kernel"] = nullable(record->failed); approval["phase"] = record->phase;
        approval["accepted_kernel_verified"] = accepted;
        if (current) approval["running_kernel_approved"] = accepted && *current == record->good && running == record->release;
        if (record->phase != "stable") {
            approval["state"] = "awaiting_confirmation"; approval["latest_update_approved"] = false;
        } else if (!record->baseline_approved) {
            approval["state"]="development_checkpoint"; approval["latest_update_approved"]=false;
            if (!s.boot || s.boot->release!=record->release || s.boot->boot_hash!=record->hash)
                issues.append("The unapproved development checkpoint does not match its recorded boot image.");
        } else if (accepted) {
            approval["state"] = "accepted";
            if (record->failed.empty()) approval["latest_update_approved"] = true;
            else approval["history_note"] = "A previous failed candidate is retained without an ordered attempt history. Current kernel approval and the previous failure are reported separately.";
        } else issues.append("The accepted-kernel record does not match the boot image and successful-slot flags.");
    }
    out["kernel_approval"] = approval; out["issues"] = issues;
    out["complete"] = issues.empty() && slots[0].boot && slots[1].boot && record.has_value();
    return out;
}
std::map<std::string,std::string> policy_fields(const std::string &input) {
    std::map<std::string,std::string> result;
    std::istringstream lines(input); std::string line;
    need(input.size() <= 8192, "Oversized update policy");
    while (std::getline(lines,line)) {
        if (line.empty() || line.front() == '#') continue;
        const auto split=line.find('=');
        need(split != line.npos && split && split+1 < line.size() &&
             result.emplace(line.substr(0,split),line.substr(split+1)).second,
             "Malformed or duplicate update policy field");
    }
    return result;
}
bool automatic_policy(const std::string &input, bool userdata) {
    const auto fields=policy_fields(input);
    need(fields.contains("automatic") && (fields.at("automatic")=="0" || fields.at("automatic")=="1"),
         "Invalid automatic staging policy");
    need(fields.size()==1 || (userdata && fields.size()==2 && fields.contains("vrr_min_hz") &&
         (fields.at("vrr_min_hz")=="0" || fields.at("vrr_min_hz")=="30")), "Unknown automatic staging policy field");
    return fields.at("automatic")=="1";
}
std::string rollback_policy(const std::string &input) {
    for (const auto *mode : {"enforced","permissive","off"})
        if (input==std::string("mode=")+mode+'\n') return mode;
    throw std::runtime_error("Invalid rollback policy");
}
void observe_policies(J &out, const fs::path &configuration) {
    J policy; policy["automatic_global"]=J(); policy["automatic_userdata"]=J();
    policy["rollback_mode"]=J(); policy["automatic_fallback"]=J();
    for (const bool userdata : {false,true}) try {
        policy[userdata?"automatic_userdata":"automatic_global"]=automatic_policy(
            text(configuration/(userdata?"userdata-update.conf":"kernel-update.conf")),userdata);
    } catch (const std::exception &e) { out["issues"].append(std::string(userdata?"Userdata staging: ":"Global staging: ")+e.what()); }
    try {
        const auto mode=rollback_policy(text(configuration/"rollback.conf"));
        policy["rollback_mode"]=mode; policy["automatic_fallback"]=mode=="enforced";
    } catch (const std::exception &e) { out["issues"].append(std::string("Rollback: ")+e.what()); }
    policy["automatic_staging"]=policy["automatic_global"].isBool() && policy["automatic_userdata"].isBool()
        ? J(policy["automatic_global"].asBool() && policy["automatic_userdata"].asBool()) : J();
    out["update_policy"]=policy;
    if (!out["issues"].empty()) out["complete"]=false;
}
J inspect() {
    const auto compatible = text("/sys/firmware/devicetree/base/compatible");
    need(compatible.find(std::string("xiaomi,uke\0",11)) != compatible.npos &&
         compatible.find(std::string("qcom,sm7675\0",12)) != compatible.npos, "Update status requires Uke / SM7675");
    J issues(Json::arrayValue); std::array<Slot,2> slots{}; struct utsname current{};
    need(!uname(&current) && token(current.release), "Running kernel identity is unavailable");
    // A nonblocking shared lock reads the existing lock only. It never creates a
    // state directory/lock or repairs metadata. A changing update is retried.
    Fd lock(open("/var/lib/senemos/kernel-update/lock", O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    need(!flock(lock.value, LOCK_SH | LOCK_NB), "A kernel update is in progress; retry status when it finishes");
    std::array<dev_t,2> devices{};
    for (unsigned i = 0; i < 2; ++i) try {
        const auto path = fs::canonical(fs::path("/dev/disk/by-partlabel")/(i ? "boot_b" : "boot_a"));
        need(path.parent_path() == "/dev", "Unexpected boot partition path");
        Fd fd(open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC)); struct stat info{};
        need(!fstat(fd.value, &info) && S_ISBLK(info.st_mode), "Boot status requires a block partition");
        devices[i] = info.st_rdev; slots[i].boot = image(fd.value);
    } catch (const std::exception &e) { issues.append(std::string(i ? "Slot B: " : "Slot A: ")+e.what()); }
    try {
        need(devices[0] && devices[1] && devices[0] != devices[1], "Two distinct boot partition devices are required");
        std::array<fs::path,2> parents;
        for (unsigned i=0; i<2; ++i) parents[i] = fs::canonical(fs::path("/sys/dev/block")/
            (std::to_string(major(devices[i]))+":"+std::to_string(minor(devices[i])))).parent_path();
        need(parents[0] == parents[1], "Boot slots are on different devices");
        const auto disk = parents[0].filename().string();
        need(disk.starts_with("sd") && disk.size() <= 5 && disk.find_first_not_of("abcdefghijklmnopqrstuvwxyz") == disk.npos, "Unexpected UFS disk");
        Fd fd(open((fs::path("/dev")/disk).c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
        struct stat info{}; need(!fstat(fd.value,&info) && S_ISBLK(info.st_mode), "GPT source is not a block device");
        const auto flags = gpt_flags(fd.value); for (unsigned i=0;i<2;++i) slots[i].flags=flags[i];
    } catch (const std::exception &e) { issues.append(e.what()); }
    std::optional<int> next, declared; std::optional<Saved> record; std::string running_hash;
    bool boot_parameters_valid = true;
    try { auto raw=text("/sys/bus/platform/devices/1d84000.ufshc/attributes/boot_lun_enabled"); std::size_t n=0;
          const auto lun=std::stoul(raw,&n,0); need((lun==1 || lun==2) && raw.find_first_not_of(" \t\r\n",n)==raw.npos,"Unsupported UFS boot LUN"); next=int(lun)-1;
    } catch (const std::exception &e) { issues.append(e.what()); }
    try { declared=boot_slot(text("/proc/cmdline",65536)); } catch (const std::exception &e) { issues.append(e.what()); boot_parameters_valid = false; }
    try { running_hash=hash_file(fs::path("/usr/lib/modules")/current.release/"vmlinuz"); } catch (const std::exception &e) { issues.append(e.what()); }
    if (!boot_parameters_valid) running_hash.clear();
    try { record=saved(text("/var/lib/senemos/kernel-update/userdata.state")); } catch (const std::exception &e) { issues.append(e.what()); }
    auto result=report(slots,current.release,running_hash,declared,next,record,issues);
    observe_policies(result,"/etc/senemos");
    return result;
}
std::string display(const J &value) { return value.isNull() ? "Unknown" : value.isBool() ? (value.asBool() ? "Yes" : "No") : value.asString(); }
std::string upper_slot(const J &slot) { return slot.isNull() ? "Unknown" : slot.asString()=="a" ? "A" : "B"; }
unsigned terminal_width() {
    struct winsize size{};
    return !ioctl(STDOUT_FILENO,TIOCGWINSZ,&size) && size.ws_col ? size.ws_col : 80;
}
std::vector<std::string> wrap_field(std::string text, unsigned width) {
    std::vector<std::string> result;
    while (text.size()>width) {
        auto end=text.rfind(' ',width);
        if (end==text.npos || end==0) end=width;
        result.push_back(text.substr(0,end));
        text.erase(0,end+(text[end]==' '));
    }
    result.push_back(text); return result;
}
std::string columns(const std::vector<std::pair<std::string,std::string>> &fields, unsigned width) {
    width=std::clamp(width,20U,240U);
    const unsigned count=width>=120?3:width>=60?2:1, cell=(width-2*(count-1))/count;
    std::ostringstream out;
    for (std::size_t start=0;start<fields.size();start+=count) {
        std::array<std::vector<std::string>,3> wrapped; std::size_t height=0;
        const auto used=std::min<std::size_t>(count,fields.size()-start);
        for (std::size_t c=0;c<used;++c) {
            wrapped[c]=wrap_field(fields[start+c].first+": "+fields[start+c].second,cell);
            height=std::max(height,wrapped[c].size());
        }
        for (std::size_t row=0;row<height;++row) {
            for (std::size_t c=0;c<used;++c) {
                const auto line=row<wrapped[c].size()?wrapped[c][row]:std::string{};
                out<<line;
                if (c+1<used) out<<std::string(cell-line.size()+2,' ');
            }
            out<<'\n';
        }
    }
    return out.str();
}
void print(const J &value, bool json) {
    if (json) { Json::StreamWriterBuilder writer; writer["indentation"]=""; std::cout << Json::writeString(writer,value) << '\n'; return; }
    std::vector<std::pair<std::string,std::string>> fields{
        {"Running kernel",display(value["running_kernel"])},
        {"Current boot slot",upper_slot(value["current_slot"])},
        {"Next boot slot",upper_slot(value["next_boot_slot"])},
        {"Pending slot switch",display(value["pending_slot_switch"])},
        {"Automatic staging",display(value["update_policy"]["automatic_staging"])},
        {"Rollback mode",display(value["update_policy"]["rollback_mode"])},
        {"Global automatic policy",display(value["update_policy"]["automatic_global"])},
        {"Userdata automatic policy",display(value["update_policy"]["automatic_userdata"])},
        {"Automatic fallback policy",display(value["update_policy"]["automatic_fallback"])}};
    for (const auto &slot : value["slots"]) {
        const auto name="Slot "+upper_slot(slot["slot"]);
        fields.emplace_back(name+" kernel",display(slot["kernel_release"]));
        fields.emplace_back(name+" bootable",display(slot["bootable"]));
        fields.emplace_back(name+" bootloader successful",display(slot["boot_successful"]));
    }
    const auto &a=value["kernel_approval"];
    fields.emplace_back("Latest update approved",display(a["latest_update_approved"]));
    fields.emplace_back("Running kernel approved",display(a["running_kernel_approved"]));
    fields.emplace_back("Accepted image verified",display(a["accepted_kernel_verified"]));
    fields.emplace_back("Accepted kernel",display(a["accepted_kernel"])+" (slot "+upper_slot(a["accepted_slot"])+")");
    if (a["baseline_approved"].isBool() && !a["baseline_approved"].asBool())
        fields.emplace_back("Unapproved checkpoint",display(a["baseline_kernel"])+" (slot "+upper_slot(a["baseline_slot"])+")");
    fields.emplace_back("Kernel update phase",display(a["phase"]));
    fields.emplace_back("Pending candidate",a["phase"]=="stable" && a["candidate_kernel"].isNull()?"None":display(a["candidate_kernel"]));
    fields.emplace_back("Previous failed candidate",!a["phase"].isNull() && a["last_failed_kernel"].isNull()?"None recorded":display(a["last_failed_kernel"]));
    std::cout<<"Uke update status\n"<<columns(fields,terminal_width());
    if (!a["history_note"].isNull()) std::cout << "Approval history: " << a["history_note"].asString() << '\n';
    for (const auto &issue : value["issues"]) std::cout << "Status incomplete: " << issue.asString() << '\n';
}
} // namespace uke::update_status
#ifndef UKE_UPDATE_STATUS_TEST
int main(int argc, char **argv) {
    const bool json=argc==2 && std::string(argv[1])=="--json";
    try {
        uke::update_status::need(argc==1 || json,"Usage: update-status [--json]");
        uke::update_status::need(geteuid()==0,"Use uke-control update status from an active local session, or with sudo");
        uke::update_status::print(uke::update_status::inspect(),json);
        std::cout.flush(); return std::cout ? 0 : 1;
    } catch (const std::exception &e) {
        if (json) { Json::Value error; error["schema"]="org.uke.control.update-status.v1"; error["available"]=false; error["error"]=e.what(); uke::update_status::print(error,true); }
        else std::cerr << "uke-control update status: " << e.what() << '\n';
        return 1;
    }
}
#endif
