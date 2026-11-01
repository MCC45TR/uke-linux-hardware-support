// SPDX-License-Identifier: Apache-2.0
#define UKE_CURVE_TEST
#include "../src/oem-brightness-curve.cpp"
#include <cassert>
#include <limits>
template<class F> static void refuses(F operation) {
    bool refused = false;
    try { operation(); } catch (const std::exception &) { refused = true; }
    assert(refused);
}
int main() {
    const Spline straight({{0,0},{10,100},{20,200}});
    assert(straight.sample(-1) == 0 && straight.sample(30) == 200 && straight.sample(5) == 50);
    const Spline plateau({{0,1},{1,3},{2,3},{10,20}});
    assert(plateau.sample(1.5) == 3);
    double previous = 1;
    for (unsigned i = 0; i <= 1000; ++i) {
        const auto current = plateau.sample(i/100.0);
        assert(current >= previous && current <= 20); previous = current;
    }
    refuses([]{ Spline({{0,0},{0,1}}); });
    refuses([]{ Spline({{0,1},{1,0}}); });
    refuses([&]{ plateau.sample(std::numeric_limits<double>::quiet_NaN()); });
    const auto converted = curve({{2,.001709819},{600,.49975574},{800,1}}, {{0,3.5},{100,103},{100000,800}});
    assert(converted.front().second == 21.0/2047 && converted.back().second == 1);
    assert(std::abs(linear(converted,100)-(.001709819+(103-2)*(.49975574-.001709819)/598)*4095/2047) < 1e-12);
    refuses([]{ curve({{0,0},{1,2}}, {{0,0},{1,1}}); });
}
