// SPDX-License-Identifier: GPL-3.0-or-later
#include <gio/gio.h>
extern "C" {
#include <libssc-sensor-light.h>
}
#include <cassert>
int main() {
    const char *types[]{nullptr, "", "factory_test", "ambient_light_back_raw"};
    for (const char *type : types) {
        GError *error = nullptr;
        assert(!ssc_sensor_light_new_for_type_sync(type, nullptr, &error));
        assert(g_error_matches(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT));
        g_clear_error(&error);
    }
}
