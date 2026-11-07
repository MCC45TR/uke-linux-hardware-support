// SPDX-License-Identifier: MIT
#include "unstable-release.hpp"
#include <array>
#include <cassert>
#include <iostream>
int main() {
    unsigned cases = 0;
    for (const auto version : {"7.3.1", "7.3.3", "7.4.0", "8.0.0", "12.34.56"})
        for (const auto revision : {"1", "2", "3", "34", "12217", "4294967295"}) {
            const std::string release = std::string(version) + '-' + revision + "-unstable";
            assert(uke_release::unstable(release)); ++cases;
        }
    for (const auto release : {"7.3.1", "7.3.3-1", "7.3.3-0-unstable", "7.3.3-01-unstable",
        "7.2.9-1-unstable", "7.3.0-1-unstable", "6.19.9-1-unstable", "7.03.3-1-unstable",
        "07.3.3-1-unstable", "7.3.03-1-unstable", "7.3-1-unstable", "7.3.3.1-1-unstable",
        "7.3.3--unstable", "7.3.3-+1-unstable", "7.3.3-1x-unstable", "7.3.3-4294967296-unstable",
        "4294967296.3.3-1-unstable", "7.3.3-1/2-unstable", "7.3.3-1-unstable\n",
        "7.3.3-1-unstable-extra", "../7.3.3-1-unstable", "7.3.3-1-kvm"}) {
        assert(!uke_release::unstable(release)); ++cases;
    }
    std::cout << "Canonical unstable release parser: " << cases << " cases; new versions and contiguous revisions, malformed/overflow/old-family refusals passed\n";
}
