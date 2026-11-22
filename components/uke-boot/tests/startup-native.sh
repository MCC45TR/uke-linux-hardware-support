#!/usr/bin/env bash
# Host-only native fixtures use private sockets and never request a host reboot.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
# shellcheck disable=SC2046
c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -O2 -UNDEBUG -pthread \
    tests/boot-guard.cpp -o build/tests/boot-guard-test $(pkg-config --cflags --libs libsystemd wayland-client wayland-server)
# shellcheck disable=SC2046
c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -O2 -UNDEBUG -pthread \
    tests/reboot.cpp -o build/tests/reboot-test $(pkg-config --cflags --libs libsystemd)
timeout 15 build/tests/boot-guard-test
timeout 15 build/tests/reboot-test
