// SPDX-License-Identifier: GPL-3.0-or-later
// Real GObject metadata/getter test. Construction opens no sensor connection.
#include <gio/gio.h>
#include <cassert>
#include <cstdint>
#include "libssc-sensor.h"

int main()
{
    auto *klass = G_OBJECT_CLASS(g_type_class_ref(SSC_TYPE_SENSOR));
    auto *property = g_object_class_find_property(klass, SSC_SENSOR_SAMPLE_RATE);
    assert(property && G_PARAM_SPEC_VALUE_TYPE(property) == G_TYPE_FLOAT);
    assert(G_IS_PARAM_SPEC_FLOAT(property));
    assert((property->flags & G_PARAM_READABLE) && !(property->flags & G_PARAM_WRITABLE));
    auto *definition = G_PARAM_SPEC_FLOAT(property);
    assert(definition->minimum == 0.0f && definition->maximum == G_MAXFLOAT);
    assert(definition->default_value == 0.0f);
    const auto *default_value = g_param_spec_get_default_value(property);
    assert(G_VALUE_HOLDS_FLOAT(default_value) && g_value_get_float(default_value) == 0.0f);

    auto *sensor = G_OBJECT(g_object_new(SSC_TYPE_SENSOR, nullptr));
    assert(sensor);
    struct {
        gfloat rate;
        std::uint32_t guard;
    } result = {-1.0f, 0xa513c79bu};
    g_object_get(sensor, SSC_SENSOR_SAMPLE_RATE, &result.rate, nullptr);
    assert(result.rate == 0.0f && result.guard == 0xa513c79bu);

    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_FLOAT);
    g_object_get_property(sensor, SSC_SENSOR_SAMPLE_RATE, &value);
    assert(G_VALUE_HOLDS_FLOAT(&value) && g_value_get_float(&value) == 0.0f);
    g_value_unset(&value);
    g_object_unref(sensor);
    g_type_class_unref(klass);
    g_print("Actual sample-rate GObject metadata, read-only float/default and both getter APIs pass; no sensor opened\n");
}
