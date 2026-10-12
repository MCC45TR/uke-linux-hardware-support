// Host-only regression fixture, compiled against the patched Mesa function.
#include <llvm/ADT/StringMap.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/TargetParser/Host.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Exercise the AArch64 branch even when checking the source on an x86 host.
#define DETECT_ARCH_ARM 0
#define DETECT_ARCH_AARCH64 1
#include "mesa-host-features.inc"

int main()
{
    const auto expected = llvm::sys::getHostCPUFeatures();
    std::vector<std::string> actual;
    lp_build_fill_mattrs(actual);
    if (expected.empty() || actual.size() != expected.size()) {
        std::cerr << "AArch64 host feature map missing or incomplete\n";
        return 1;
    }
    for (const auto &feature : expected) {
        const std::string attribute = (feature.second ? "+" : "-") + feature.first().str();
        if (std::count(actual.begin(), actual.end(), attribute) != 1) {
            std::cerr << "Host feature not preserved: " << attribute << '\n';
            return 1;
        }
    }
    std::cout << "All " << actual.size() << " host feature states preserved\n";
}
