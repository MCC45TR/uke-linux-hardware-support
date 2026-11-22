// Diagnostic only: compare socket creation under SSH and the service's confinement.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    if (FILE* context = std::fopen("/proc/self/attr/current", "r")) {
        char label[256]{};
        if (std::fgets(label, sizeof(label), context)) std::printf("context=%s\n", label);
        std::fclose(context);
    }
    const int fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        std::printf("AF_QIPCRTR socket: errno=%d (%s)\n", errno, std::strerror(errno));
        return 1;
    }
    std::puts("AF_QIPCRTR socket: available");
    close(fd);
}
