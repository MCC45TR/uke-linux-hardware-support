// SPDX-License-Identifier: MIT
#define main factory_application_main
#include "../src/wifi-factory.cpp"
#undef main
#include <cassert>
#include <filesystem>
#include <fstream>

static void record_fixtures() {
    namespace fs = std::filesystem;
    char temporary[] = "/tmp/uke-wifi-record.XXXXXX";
    const auto created = mkdtemp(temporary);
    assert(created);
    const fs::path base(created), persist = base / "persist", wlan = persist / "wlan";
    const fs::path path = wlan / "wlan_mac.bin", outside = base / "outside";
    fs::create_directories(wlan);
    fs::create_directory(outside);
    const int root = open(persist.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    assert(root >= 0);
    struct stat metadata{};
    assert(fstat(root, &metadata) == 0);
    const auto save = [](const fs::path &file, const std::string &data) {
        std::ofstream output(file, std::ios::binary);
        output << data;
        output.close();
        assert(output.good());
    };
    const auto rejects = [&](dev_t device) {
        bool rejected = false;
        try { (void)factory_record(root, device); }
        catch (const std::exception &) { rejected = true; }
        assert(rejected);
    };
    const std::string valid = "wlan0=001122334455\n";
    save(path, valid);
    assert(factory_record(root, metadata.st_dev) == valid);
    rejects(metadata.st_dev ^ dev_t(1));
    save(path, valid + std::string(1024 - valid.size(), '#'));
    assert(factory_record(root, metadata.st_dev).size() == 1024);
    save(path, std::string(1025, 'x')); rejects(metadata.st_dev);
    save(path, ""); rejects(metadata.st_dev);
    fs::remove(path);
    fs::create_directory(path); rejects(metadata.st_dev);
    fs::remove(path);
    assert(mkfifo(path.c_str(), 0600) == 0); rejects(metadata.st_dev);
    fs::remove(path);
    save(outside / "wlan_mac.bin", valid);
    fs::create_symlink(outside / "wlan_mac.bin", path); rejects(metadata.st_dev);
    fs::remove(path);
    fs::remove(wlan);
    fs::create_directory_symlink(outside, wlan); rejects(metadata.st_dev);
    fs::remove(wlan);
    fs::create_directory(wlan); rejects(metadata.st_dev);
    fs::remove(wlan); rejects(metadata.st_dev);
    close(root);
    fs::remove_all(base);
}

int main() {
    assert(factory_mac("wlan0=001122aAbBcC\n") == "00:11:22:AA:BB:CC");
    assert(factory_mac("wlan1=001122334456\r\nwlan0=001122334455\r\n") == "00:11:22:33:44:55");
    for (const auto* invalid : {"", "wlan0=000000000000", "wlan0=FFFFFFFFFFFF",
            "wlan0=011122334455", "wlan0=00112233445", "wlan0=00112233445G",
            "wlan0=001122334455\nwlan0=001122334456", "wlan0=\nwlan0=001122334455",
            "wlan0=001122334455;evil"}) {
        bool rejected = false;
        try { (void)factory_mac(invalid); } catch (const std::exception&) { rejected = true; }
        assert(rejected);
    }
    bool oversized = false;
    try { (void)factory_mac(std::string(1025, 'x')); } catch (const std::exception&) { oversized = true; }
    assert(oversized);
    record_fixtures();
    std::puts("Factory WLAN parser and descriptor-relative record fixtures passed");
}
