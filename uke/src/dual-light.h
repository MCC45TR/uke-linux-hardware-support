/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef UKE_DUAL_LIGHT_H
#define UKE_DUAL_LIGHT_H
#include <stdbool.h>
#include <math.h>

struct UkeDualLight {
    float lux[2];
    bool valid[2];
};

/* OEM main/assist selection uses the brighter valid sensor; KDE owns smoothing. */
static inline bool uke_dual_light_update(struct UkeDualLight *state, unsigned sensor,
                                        float lux, float *combined)
{
    if (sensor > 1 || !isfinite(lux) || lux < 0)
        return false;
    state->lux[sensor] = lux;
    state->valid[sensor] = true;
    if (!state->valid[0] || !state->valid[1])
        return false;
    *combined = fmaxf(state->lux[0], state->lux[1]);
    return true;
}
#endif
