// Host-only PTY fixture; no USB device, bridge or tablet is opened.
#include <array>
#include <cstdlib>
#include <iostream>
#include <pty.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
static int execute(const char* device, const char* extra) {
    const auto child = fork();
    if (child == 0) {
        execl("build/tests/uke-esp32-cdc", "uke-esp32-cdc", "--device", device, extra, static_cast<char*>(nullptr));
        _exit(99);
    }
    int status = 0;
    if (child < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status)) return 99;
    return WEXITSTATUS(status);
}
int main() {
    int master = -1, slave = -1;
    std::array<char, 128> name {};
    if (openpty(&master, &slave, name.data(), nullptr, nullptr)) return 1;
    if (execute(name.data(), "--check") != 0) return 1;
    termios attributes {};
    if (tcgetattr(slave, &attributes) || cfgetispeed(&attributes) != B115200 ||
        cfgetospeed(&attributes) != B115200 || (attributes.c_lflag & (ICANON | ECHO)) ||
        (attributes.c_oflag & OPOST) || (attributes.c_cflag & CRTSCTS)) return 1;
    const std::array<unsigned char, 4> bytes {0, '\r', '\n', 0xff};
    std::array<unsigned char, 4> output {};
    if (write(slave, bytes.data(), bytes.size()) != 4 || read(master, output.data(), output.size()) != 4 || output != bytes) return 1;
    close(slave); close(master);
    if (execute("/missing-host-fixture", "--check") != 2) return 1;
    if (execute("/dev/null", "--check") != 2) return 1;
    if (execute("/dev/ttyACM../tty2", nullptr) != 1) return 1;
    std::cout << "Native PTY fixtures passed: raw 115200 framing and byte preservation; missing/non-TTY/invalid paths rejected; USB untested\n";
}
