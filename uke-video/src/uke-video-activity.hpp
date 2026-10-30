// SPDX-License-Identifier: MIT
#pragma once
#include <memory>
namespace irisva {
class VideoActivity {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    VideoActivity(unsigned width, unsigned height);
    ~VideoActivity();
    void touch() noexcept;
};
}
