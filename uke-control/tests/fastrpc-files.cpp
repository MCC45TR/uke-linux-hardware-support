// SPDX-License-Identifier: MIT
// Exercise unmodified Fedora apps_std file APIs. All ioctl syscalls are denied.
#include <apps_std.h>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void require(bool ok, const char *message) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
template <typename Function> Function symbol(void *library, const char *name) {
    void *entry = dlsym(library, name);
    require(entry != nullptr, name);
    return reinterpret_cast<Function>(entry);
}
void deny_ioctls() {
    const sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                 offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ioctl, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog program{static_cast<unsigned short>(std::size(rules)),
                       const_cast<sock_filter *>(rules)};
    require(prctl(PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) == 0,
            "cannot set fixture no-new-privileges");
    require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0,
            "cannot deny device ioctls");
    require(prctl(PR_GET_SECCOMP) == SECCOMP_MODE_FILTER,
            "syscall filter missing");
    errno = 0;
    require(ioctl(-1, 0) == -1 && errno == EPERM,
            "kernel ioctl refusal not active");
}
void put(const fs::path &path, const std::string &data) {
    std::ofstream stream(path, std::ios::binary);
    stream << data;
    require(stream.good(), "create public file vector");
}
} // namespace

int main(int argc, char **argv) {
    require(argc == 2 && getuid() != 0,
            "usage: non-root fixture EMPTY-DIRECTORY");
    const fs::path root = fs::absolute(argv[1]);
    require(fs::is_directory(root) && fs::is_empty(root),
            "fixture directory must be empty");
    require(!fs::exists("/dev/fastrpc-adsp"),
            "never run this fixture on a DSP host");
    deny_ioctls();
    fs::create_directories(root / "state/config");
    fs::create_directories(root / "state/registry");
    fs::create_directories(root / "view/vendor/etc/sensors");
    fs::create_directories(root / "view/odm/etc/sensors");
    fs::create_directories(root / "view/persist/sensors/registry");
    fs::create_directories(root / "view/mnt/vendor/persist/sensors/registry");
    fs::create_directory_symlink(root / "state/config",
                                root / "view/vendor/etc/sensors/config");
    fs::create_directory_symlink(root / "state/config",
                                root / "view/odm/etc/sensors/config");
    fs::create_directory_symlink(root / "state/registry",
                                root / "view/persist/sensors/registry/registry");
    fs::create_directory_symlink(root / "state/registry",
                        root / "view/mnt/vendor/persist/sensors/registry/registry");
    const std::string config = "public SSC config vector\n";
    put(root / "state/config/json.lst", config);
    require(chmod((root / "state/config/json.lst").c_str(), 0444) == 0,
            "set read-only config permission");

    void *library = dlopen("libadsprpc.so.1", RTLD_NOW | RTLD_LOCAL);
    require(library != nullptr, dlerror());
    const auto open_file = symbol<decltype(&apps_std_fopen)>(library, "apps_std_fopen");
    const auto close_file = symbol<decltype(&apps_std_fclose)>(library, "apps_std_fclose");
    const auto read_file = symbol<decltype(&apps_std_fread)>(library, "apps_std_fread");
    const auto write_file = symbol<decltype(&apps_std_fwrite)>(library, "apps_std_fwrite");
    const auto flush_file = symbol<decltype(&apps_std_fflush)>(library, "apps_std_fflush");
    const auto seek_file = symbol<decltype(&apps_std_fseek)>(library, "apps_std_fseek");
    const auto rename_file = symbol<decltype(&apps_std_frename)>(library, "apps_std_frename");
    const auto remove_file = symbol<decltype(&apps_std_fremove)>(library, "apps_std_fremove");
    const auto make_dir = symbol<decltype(&apps_std_mkdir)>(library, "apps_std_mkdir");

    const auto read_exact = [&](const fs::path &path, const std::string &expected) {
        apps_std_FILE stream = -1;
        require(open_file(path.c_str(), "rb", &stream) == 0, "original fopen failed");
        std::array<unsigned char, 128> data{};
        int count = -1, eof = -1;
        require(read_file(stream, data.data(), static_cast<int>(data.size()),
                          &count, &eof) == 0, "original fread failed");
        require(count == static_cast<int>(expected.size()) &&
                std::memcmp(data.data(), expected.data(), expected.size()) == 0,
                "original API file content changed");
        require(eof != 0, "original API short-read EOF missing");
        require(seek_file(stream, 0, APPS_STD_SEEK_SET) == 0, "original fseek failed");
        require(read_file(stream, data.data(), static_cast<int>(data.size()),
                          &count, &eof) == 0, "original reread failed");
        require(count == static_cast<int>(expected.size()), "reread length changed");
        require(close_file(stream) == 0, "original fclose failed");
    };
    read_exact(root / "view/vendor/etc/sensors/config/json.lst", config);
    read_exact(root / "view/odm/etc/sensors/config/json.lst", config);
    apps_std_FILE denied = -1;
    require(open_file((root / "view/vendor/etc/sensors/config/json.lst").c_str(),
                      "wb", &denied) != 0, "read-only config write was permitted");
    require(open_file((root / "missing/file").c_str(), "rb", &denied) != 0,
            "missing input was accepted");
    require(open_file(nullptr, "rb", &denied) != 0, "null filename was accepted");
    std::array<unsigned char, 8> invalid{};
    int count = 0, eof = 0;
    require(read_file(0x7fffffff, invalid.data(), static_cast<int>(invalid.size()),
                      &count, &eof) != 0, "invalid stream handle was accepted");

    const fs::path first = root / "view/persist/sensors/registry/registry/public-state";
    const fs::path second = root / "view/mnt/vendor/persist/sensors/registry/registry/public-state";
    const std::string registry = "public mutable registry\n";
    apps_std_FILE stream = -1;
    require(open_file(first.c_str(), "wb", &stream) == 0, "registry create failed");
    require(write_file(stream,
                       reinterpret_cast<const unsigned char *>(registry.data()),
                       static_cast<int>(registry.size()), &count, &eof) == 0,
            "original fwrite failed");
    require(count == static_cast<int>(registry.size()), "registry short write");
    require(flush_file(stream) == 0, "original fflush failed");
    require(close_file(stream) == 0, "registry close failed");
    read_exact(second, registry);
    require(open_file(second.c_str(), "ab", &stream) == 0, "registry append open");
    const unsigned char suffix[] = {'x', '\n'};
    require(write_file(stream, suffix, sizeof(suffix), &count, &eof) == 0 &&
            count == static_cast<int>(sizeof(suffix)), "registry append failed");
    require(close_file(stream) == 0, "append close failed");
    read_exact(first, registry + "x\n");
    const fs::path renamed = second.parent_path() / "renamed-state";
    require(rename_file(first.c_str(), renamed.c_str()) == 0,
            "original registry rename failed");
    require(!fs::exists(first), "rename left old registry entry");
    read_exact(renamed, registry + "x\n");
    require(remove_file(renamed.c_str()) == 0, "original remove failed");
    require(!fs::exists(renamed), "registry removal not visible");
    require(remove_file(renamed.c_str()) != 0, "missing removal accepted");
    const fs::path directory = root / "state/registry/new-directory";
    require(make_dir(directory.c_str(), 0700) == 0 && fs::is_directory(directory),
            "original mkdir failed");
    require(make_dir(directory.c_str(), 0700) != 0, "existing mkdir accepted");
    read_exact(root / "state/config/json.lst", config);
    dlclose(library);
    require(prctl(PR_GET_SECCOMP) == SECCOMP_MODE_FILTER, "ioctl filter lost");
    fs::remove_all(root);
    std::printf("Original Fedora apps_std file API:aliases,read/EOF/seek,registry write/append/flush/rename/remove,mkdir,readonly/missing/null/handle refusals passed with every ioctl denied (%u checks).\n", checks);
}
