// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace uke::splash {
struct Output {
    unsigned width, height, width_mm, height_mm;
};
struct Density {
    double x, y; // Plymouth logical pixels per millimetre, after device scaling.
};
inline std::optional<Density> physical_density(unsigned width, unsigned height,
                                             unsigned scale, bool rotated,
                                             const std::vector<Output>& outputs) {
    if (!width || !height || !scale || scale > 16) return {};
    std::optional<Density> result;
    for (auto output : outputs) {
        if (rotated) {
            std::swap(output.width, output.height);
            std::swap(output.width_mm, output.height_mm);
        }
        // Device scale can round the logical viewport down by one pixel.
        if (output.width / scale != width || output.height / scale != height) continue;
        // Missing or contradictory EDID/DT dimensions must not claim centimetre accuracy.
        if (output.width_mm < 10 || output.height_mm < 10 ||
            output.width_mm > 10000 || output.height_mm > 10000) return {};
        const Density next{double(output.width) / scale / output.width_mm,
                           double(output.height) / scale / output.height_mm};
        if (result && (std::abs(result->x - next.x) > 0.001 ||
                       std::abs(result->y - next.y) > 0.001)) return {};
        result = next;
    }
    return result;
}
struct Layout {
    unsigned logo_w, logo_h, fedora_w, fedora_h, bar_w, bar_h, font_px;
    int logo_x, logo_y, bar_x, bar_y, fedora_x, fedora_y, message_y;
    bool physical;
};
inline Layout layout(unsigned width, unsigned height, std::optional<Density> density) {
    // Defaults preserve the established artwork's approximate physical size on Uke.
    // These are artwork dimensions, never hard-coded monitor/panel dimensions.
    const auto d = density.value_or(Density{96.0 / 25.4, 96.0 / 25.4});
    const auto pixels = [](double value) { return unsigned(std::max(1.0, std::round(value))); };
    Layout out{};
    out.physical = density.has_value();
    // Fit small displays safely; ordinary tablet/monitor sizes retain exact millimetres.
    const double fit = std::min({1.0, width / (50.0 * d.x), height / (65.0 * d.y)});
    out.logo_w = pixels(23 * d.x * fit); out.logo_h = pixels(23 * d.y * fit);
    out.fedora_w = pixels(32 * d.x * fit); out.fedora_h = pixels(32 * 120.0 / 427 * d.y * fit);
    out.bar_w = pixels(44 * d.x * fit); out.bar_h = pixels(1.2 * d.y * fit);
    out.font_px = pixels(3.5 * d.y * fit);
    out.logo_x = (int(width) - int(out.logo_w)) / 2;
    out.logo_y = int(height / 2) - int(pixels(3.5 * d.y * fit)) - int(out.logo_h / 2);
    out.bar_x = (int(width) - int(out.bar_w)) / 2;
    out.bar_y = int(height / 2) + int(pixels(14 * d.y * fit));
    out.fedora_x = (int(width) - int(out.fedora_w)) / 2;
    out.fedora_y = int(height) - int(out.fedora_h) - int(pixels(3 * d.y * fit));
    out.message_y = out.bar_y + int(out.bar_h) + int(pixels(2 * d.y * fit));
    return out;
}
} // namespace uke::splash
