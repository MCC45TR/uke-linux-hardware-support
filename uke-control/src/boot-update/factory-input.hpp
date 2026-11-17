// SPDX-License-Identifier: MIT
#pragma once
// Include after the existing updater's protected descriptor/file helpers.
namespace uke_factory {
template<class Admit>
bool read(const fs::path &path, Admit admit, Bytes &out) {
    // A dangling symlink is present and invalid, rather than a missing source.
    if (!fs::exists(path) && !fs::is_symlink(path)) return false;
    auto data = ::file(path);
    need(admit(data), "Packaged OEM input is not already admitted");
    // Failed reads/admission leave the caller's previous data unchanged.
    out = std::move(data);
    return true;
}
}
