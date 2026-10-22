// SPDX-License-Identifier: MIT
// Host-only check against the distribution's real script parser; no display access.
#include <cassert>
#include <dlfcn.h>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 3);
    void *plugin = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!plugin) { std::cerr << dlerror() << '\n'; return 1; }
    auto parse = reinterpret_cast<void *(*)(const char *)>(dlsym(plugin, "script_parse_file"));
    auto parse_string = reinterpret_cast<void *(*)(const char *, const char *)>(dlsym(plugin, "script_parse_string"));
    auto release = reinterpret_cast<void (*)(void *)>(dlsym(plugin, "script_parse_op_free"));
    assert(parse && parse_string && release);
    assert(parse_string("fun {", "invalid-fixture") == nullptr);
    void *script = parse(argv[1]);
    assert(script);
    release(script);
    dlclose(plugin);
    std::cout << "PASS: distribution Plymouth parser accepts the theme and refuses malformed syntax.\n";
}
