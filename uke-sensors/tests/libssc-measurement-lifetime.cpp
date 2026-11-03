// SPDX-License-Identifier: GPL-3.0-or-later
#include <glib-object.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

extern "C" {
GObject *fixture_measurement_sensor_new(void);
guint fixture_measurement_queue(GObject *, guint sequence, guint mode);
}

struct Observation {
    unsigned finalized = 0;
    unsigned calls = 0;
    unsigned expected_sequence = 0;
};

static void finalized(gpointer data, GObject *) {
    ++static_cast<Observation *>(data)->finalized;
}

static gboolean measured(GSignalInvocationHint *, guint count,
                         const GValue *values, gpointer data) {
    auto &seen = *static_cast<Observation *>(data);
    assert(seen.finalized == 0);
#if MEASUREMENT_KIND == 3
    assert(count == 2);
    assert(g_value_get_boolean(&values[1]) == (seen.expected_sequence % 2 != 0));
#elif MEASUREMENT_KIND == 2 || MEASUREMENT_KIND == 5
    assert(count == 2);
    const auto expected = MEASUREMENT_KIND == 2 ? 1.0f : 0.0f;
    assert(std::abs(g_value_get_float(&values[1]) - expected) < 0.0001f);
#else
    assert(count == 4);
    for(unsigned i = 1; i < count; ++i)
        assert(std::abs(g_value_get_float(&values[i]) - static_cast<float>(i)) < 0.0001f);
#endif
    ++seen.calls;
    ++seen.expected_sequence;
    return TRUE;
}

static void drain() {
    unsigned iterations = 0;
    while(g_main_context_iteration(nullptr, FALSE))
        assert(++iterations < 128);
}

static void pending(bool cancel_all, unsigned queued, unsigned cancel_first, guint mode = 0) {
    assert(!g_main_context_pending(nullptr));
    Observation seen;
    auto *sensor = fixture_measurement_sensor_new();
    g_object_weak_ref(sensor, finalized, &seen);
    const auto signal = g_signal_lookup("measurement", G_OBJECT_TYPE(sensor));
    assert(signal != 0);
    const auto hook = g_signal_add_emission_hook(signal, 0, measured, &seen, nullptr);
    assert(hook != 0);
    std::array<guint, 4> ids{};
    assert(queued > 0 && queued <= ids.size());
    for(unsigned i = 0; i < queued; ++i) {
        ids[i] = fixture_measurement_queue(sensor, i, mode);
        assert(ids[i] != 0);
    }
    assert(seen.calls == 0);
    g_object_unref(sensor);
    assert(seen.finalized == 0 && "Queued measurement must retain its sensor");
    const auto removed = cancel_all ? queued : cancel_first;
    assert(removed <= queued);
    for(unsigned i = 0; i < removed; ++i) {
        assert(g_source_remove(ids[i]));
        assert(seen.finalized == (i + 1 == queued ? 1U : 0U));
    }
    seen.expected_sequence = removed;
    drain();
    assert(seen.calls == queued - removed);
    assert(seen.finalized == 1);
    g_signal_remove_emission_hook(signal, hook);
    assert(!g_main_context_pending(nullptr));
}

static void ignored(guint mode) {
    Observation seen;
    auto *sensor = fixture_measurement_sensor_new();
    g_object_weak_ref(sensor, finalized, &seen);
    if(mode != 0)
        assert(fixture_measurement_queue(sensor, 0, mode) == 0);
    g_object_unref(sensor);
    assert(seen.finalized == 1);
    assert(!g_main_context_pending(nullptr));
}

int main() {
    // The first assertion is also the exact old-source negative control.
    pending(false, 1, 0);
    pending(true, 1, 0);
    pending(false, 4, 0);
    pending(false, 4, 3);
    pending(true, 4, 0);
    ignored(0); // No hidden object reference in the fixture.
    ignored(1); // A foreign UID never schedules a measurement.
    ignored(2); // An unrelated event never schedules a measurement.
#if MEASUREMENT_KIND == 3
    pending(false, 1, 0, 3);
    pending(true, 1, 0, 3);
    pending(false, 4, 0, 3);
    pending(false, 4, 3, 3);
    pending(true, 4, 0, 3);
#endif
    std::printf("Actual measurement class%d: pending ownership, values, dispatch/cancel/mixed queues and finalization PASS\n", MEASUREMENT_KIND);
}
