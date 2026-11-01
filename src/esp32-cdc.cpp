// SPDX-License-Identifier: MIT
// Native real-root journal forwarding for a configured USB-host CDC port.
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: uke-esp32-cdc --device CDC_TTY [--check]\n"
                     "Forward the current boot journal to an explicitly configured ESP32-S3 CDC port.\n"
                     "--check configures a TTY only; it does not identify USB hardware or run a journal stream.\n";
        return 0;
    }
    if ((argc != 3 && argc != 4) || std::string(argv[1]) != "--device" ||
        (argc == 4 && std::string(argv[3]) != "--check")) return 1;
    const bool check = argc == 4;
    const std::string path = argv[2];
    if (!check && (path.rfind("/dev/ttyACM", 0) != 0 || path.size() <= 11 ||
                   path.size() > 16 || path.find_first_not_of("0123456789", 11) != std::string::npos)) {
        std::cerr << "Configure an explicit ttyACM port; USB identity remains a hardware validation\n";
        return 1;
    }
    const int tty = open(path.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    struct stat statbuf {};
    if (tty < 0 || fstat(tty, &statbuf) || !S_ISCHR(statbuf.st_mode)) {
        if (tty >= 0) close(tty);
        std::cerr << "Configured CDC character device is unavailable\n";
        return 2;
    }
    termios settings {};
    if (ioctl(tty, TIOCEXCL) || tcgetattr(tty, &settings)) {
        close(tty);
        std::cerr << "CDC TTY ownership or attributes are unavailable\n";
        return 2;
    }
    cfmakeraw(&settings);
    settings.c_cflag |= CLOCAL | CREAD;
    settings.c_cflag &= ~CRTSCTS;
    if (cfsetispeed(&settings, B115200) || cfsetospeed(&settings, B115200) ||
        tcsetattr(tty, TCSANOW, &settings)) {
        close(tty);
        return 2;
    }
    int modem = TIOCM_DTR | TIOCM_RTS;
    // PTYs used by host fixtures lack modem ioctls. Real cdc_acm implements them.
    if (ioctl(tty, TIOCMBIS, &modem) && !(check && errno == ENOTTY)) {
        close(tty);
        std::cerr << "CDC DTR/RTS assertion failed\n";
        return 2;
    }
    const int flags = fcntl(tty, F_GETFL);
    if (flags < 0 || fcntl(tty, F_SETFL, flags & ~O_NONBLOCK)) { close(tty); return 2; }
    if (check) {
        close(tty);
        std::cout << "TTY configuration passed; USB identification and enumeration untested\n";
        return 0;
    }
    if (dup2(tty, STDOUT_FILENO) < 0 || dup2(tty, STDERR_FILENO) < 0) { close(tty); return 2; }
    close(tty);
    setenv("TERM", "dumb", 1);
    execl("/usr/bin/journalctl", "journalctl", "--boot", "--follow", "--output=short-monotonic", "--no-pager", static_cast<char*>(nullptr));
    return 2;
}
