// SPDX-License-Identifier: MIT
// Actual CLI readers with temporary files/pipes; no power-supply access.
#define main charge_control_cli_main
#include "../src/battery/control-main.cpp"
#undef main
#include <cassert>
#include <cstdio>

int main(int argc, char **argv) {
    assert(argc == 2);
    std::ostringstream release; auto *saved = std::cout.rdbuf(release.rdbuf());
    emit_release(false); emit_release(true);
    std::cout.rdbuf(saved);
    assert(release.str().find("\"owned_lease_revoke_completed\":false") != std::string::npos);
    assert(release.str().find("\"owned_lease_revoke_completed\":true") != std::string::npos);
    assert(release.str().find("\"firmware_setting_restore_performed\":false") != std::string::npos);
    assert(release.str().find("\"physical_charge_stopped\":null") != std::string::npos);
    const auto rejects = [](const auto &action, const std::string &expected) {
        bool rejected = false;
        try { action(); }
        catch (const std::runtime_error &e) {
            rejected = true;
            assert(e.what() == expected);
            assert(identifier(e.what()) == expected);
        }
        assert(rejected);
    };
    rejects([] { read_fd(-1, "uke_charge_settings"); },
        "control_metadata_read_failed_uke_charge_settings_errno_" + std::to_string(EBADF));
    int descriptors[2];
    assert(pipe2(descriptors, O_NONBLOCK | O_CLOEXEC) == 0);
    rejects([&] { read_fd(descriptors[0], "uke_charge_state"); },
        "control_metadata_read_failed_uke_charge_state_errno_" + std::to_string(EAGAIN));
    assert(write(descriptors[1], "1 70 80\n", 8) == 8);
    assert(close(descriptors[1]) == 0);
    assert(read_fd(descriptors[0], "uke_charge_settings") == "1 70 80\n");
    assert(close(descriptors[0]) == 0);
    FILE *file = std::tmpfile();
    assert(file);
    const std::string boundary(4096, 'x');
    assert(std::fwrite(boundary.data(), 1, boundary.size(), file) == boundary.size());
    std::rewind(file);
    assert(read_fd(fileno(file), "test") == boundary);
    assert(std::fseek(file, 0, SEEK_END) == 0 && std::fputc('x', file) == 'x');
    std::rewind(file);
    rejects([&] { read_fd(fileno(file), "test"); }, "control_metadata_too_large");
    assert(std::fclose(file) == 0);
    rejects([&] { read_file(fs::path(argv[1]) / "missing-private-name"); },
        "control_metadata_unavailable_errno_" + std::to_string(ENOENT));
    rejects([&] { read_file(argv[1]); },
        "control_metadata_read_failed_file_errno_" + std::to_string(EISDIR));
}
