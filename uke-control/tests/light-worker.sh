#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Isrc tests/torch.cpp -Lbuild -l:libuke-control.so.0 $(pkg-config --cflags --libs libsystemd libselinux) -o build/test-torch
LD_LIBRARY_PATH=build build/test-torch
