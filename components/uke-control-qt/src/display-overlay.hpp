// SPDX-License-Identifier: MIT
#pragma once
#include <QString>

namespace uq {
inline constexpr auto displayOverlayBusName = "org.uke.ControlQt.DisplayOverlay";
inline constexpr auto displayOverlayBusPath = "/Overlay";
inline constexpr auto displayOverlayBusInterface = "org.uke.ControlQt.DisplayOverlay1";
int runDisplayOverlay();
} // namespace uq
