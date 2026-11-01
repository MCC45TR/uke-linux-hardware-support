// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/dual-light.h"
#include <cassert>
#include <limits>
#include <initializer_list>
int main() {
    UkeDualLight state{};
    float value = -1;
    assert(!uke_dual_light_update(&state, 0, 0, &value));
    assert(value == -1);
    assert(uke_dual_light_update(&state, 1, 71.75f, &value) && value == 71.75f);
    assert(uke_dual_light_update(&state, 0, 95, &value) && value == 95);
    assert(uke_dual_light_update(&state, 0, 0, &value) && value == 71.75f);
    assert(uke_dual_light_update(&state, 1, 0, &value) && value == 0);
    for (float invalid : {-1.0f, std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()}) {
        assert(!uke_dual_light_update(&state, 1, invalid, &value));
        assert(value == 0 && state.lux[1] == 0);
    }
    assert(!uke_dual_light_update(&state, 2, 5, &value));
    state = {};
    assert(!uke_dual_light_update(&state, 1, 12, &value));
    assert(uke_dual_light_update(&state, 0, 4, &value) && value == 12);
}
