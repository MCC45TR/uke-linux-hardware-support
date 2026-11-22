// SPDX-License-Identifier: Apache-2.0
// Host-only data conversion. Monotone Hermite interpolation follows Android
// Spline.java (android-16.0.0_r1, Copyright 2013 The Android Open Source Project).
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

using Points = std::vector<std::pair<double, double>>;
static void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
static void validate(const Points &points) {
    require(points.size() >= 2 && points.size() <= 4096, "Invalid OEM point count");
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto [x, y] = points[i];
        require(std::isfinite(x) && std::isfinite(y) && x >= 0 && y >= 0, "Invalid OEM point");
        require(i == 0 || (x > points[i-1].first && y >= points[i-1].second), "Non-monotone OEM curve");
    }
}
static double linear(const Points &points, double x) {
    if (x <= points.front().first) return points.front().second;
    const auto upper = std::upper_bound(points.begin(), points.end(), x,
        [](double value, const auto &point) { return value < point.first; });
    if (upper == points.end()) return points.back().second;
    const auto lower = std::prev(upper);
    return std::lerp(lower->second, upper->second, (x - lower->first) / (upper->first - lower->first));
}
class Spline {
    Points points;
    std::vector<double> slopes;
public:
    explicit Spline(Points input): points(std::move(input)), slopes(points.size()) {
        validate(points);
        std::vector<double> secants(points.size()-1);
        for (std::size_t i = 0; i < secants.size(); ++i)
            secants[i] = (points[i+1].second-points[i].second)/(points[i+1].first-points[i].first);
        slopes.front() = secants.front(); slopes.back() = secants.back();
        for (std::size_t i = 1; i+1 < slopes.size(); ++i) slopes[i] = (secants[i-1]+secants[i])/2;
        for (std::size_t i = 0; i < secants.size(); ++i) {
            if (secants[i] == 0) slopes[i] = slopes[i+1] = 0;
            else {
                const auto length = std::hypot(slopes[i]/secants[i], slopes[i+1]/secants[i]);
                if (length > 3) { slopes[i] *= 3/length; slopes[i+1] *= 3/length; }
            }
        }
    }
    double sample(double x) const {
        require(std::isfinite(x), "Non-finite lux");
        if (x <= points.front().first) return points.front().second;
        if (x >= points.back().first) return points.back().second;
        const auto upper = std::upper_bound(points.begin(), points.end(), x,
            [](double value, const auto &point) { return value < point.first; });
        const auto i = std::size_t(upper - points.begin() - 1);
        const auto h = points[i+1].first-points[i].first, t = (x-points[i].first)/h;
        const auto result = (points[i].second*(1+2*t)+h*slopes[i]*t)*(1-t)*(1-t)
             + (points[i+1].second*(3-2*t)+h*slopes[i+1]*(t-1))*t*t;
        return std::clamp(result, points[i].second, points[i+1].second);
    }
};
static Points curve(const Points &nits_map, const Points &lux_nits) {
    validate(nits_map); validate(lux_nits);
    require(nits_map.back().second <= 1 && lux_nits.back().first <= 100000, "OEM range exceeds Uke contract");
    const Spline spline(lux_nits);
    const auto brightness = [&](double lux) {
        // OEM Android uses 0..4095; native normal mode retains exactly 0..2047.
        // The kernel already owns the paired KTZ8866 LUT. Do not apply it twice.
        return std::clamp(linear(nits_map, spline.sample(lux))*4095/2047, 21.0/2047, 1.0);
    };
    Points result{{lux_nits.front().first, brightness(lux_nits.front().first)}};
    const auto segment = [&](auto &&self, double x, double y, unsigned depth) -> void {
        require(depth <= 20 && result.size() < 4096, "OEM curve exceeds approximation bounds");
        const auto first = brightness(x), last = brightness(y);
        bool split = false;
        for (double fraction : {0.25, 0.5, 0.75}) {
            const auto at = std::lerp(x, y, fraction);
            split |= std::abs(brightness(at)-std::lerp(first,last,fraction)) > 0.1/2047;
        }
        if (split) {
            self(self, x, (x+y)/2, depth+1);
            self(self, (x+y)/2, y, depth+1);
        } else result.emplace_back(y, last);
    };
    for (std::size_t i = 1; i < lux_nits.size(); ++i)
        segment(segment, lux_nits[i-1].first, lux_nits[i].first, 0);
    validate(result);
    double error = 0;
    for (unsigned step = 0; step <= unsigned(lux_nits.back().first*10); ++step)
        error = std::max(error, std::abs(brightness(step/10.0)-linear(result,step/10.0))*2047);
    require(error <= 0.25, "KDE approximation differs by more than a quarter backlight step");
    std::cerr << "OEM curve: " << result.size() << " points; maximum 0.1-lux-grid error "
              << error << " native backlight steps; existing normal-mode ceiling retained\n";
    return result;
}
#ifndef UKE_CURVE_TEST
int main() {
    try {
        const auto read = [] {
            unsigned count;
            require(bool(std::cin >> count) && count >= 2 && count <= 4096, "Invalid OEM input count");
            Points result(count);
            for (auto &[x,y] : result) require(bool(std::cin >> x >> y), "Truncated OEM input");
            validate(result); return result;
        };
        const auto nits_map = read(), lux_nits = read();
        std::cin >> std::ws;
        require(std::cin.eof(), "Trailing OEM input");
        const auto output = curve(nits_map, lux_nits);
        std::cout << std::setprecision(17) << '[';
        for (std::size_t i = 0; i < output.size(); ++i)
            std::cout << (i ? ",\n" : "\n") << '[' << output[i].first << ',' << output[i].second << ']';
        std::cout << "\n]\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
#endif
