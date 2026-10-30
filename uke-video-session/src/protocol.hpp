// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace uke::video {
inline constexpr char name[] = "org.uke.VideoSession1";
inline constexpr char path[] = "/org/uke/VideoSession1";
inline constexpr std::uint64_t uhd_pixels = 3840ULL * 2160;
inline constexpr std::uint64_t expiry_usec = 1500000;
inline constexpr std::uint64_t idle_exit_usec = 30000000;
}
