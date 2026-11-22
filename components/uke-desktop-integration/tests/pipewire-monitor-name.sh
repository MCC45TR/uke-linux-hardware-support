#!/usr/bin/env bash
# Exercise the actual Pulse route collector and sink selection with synthetic route pods.
set -euo pipefail
juo_source=$(realpath "${1:?PipeWire source directory required}")
juo_tmp=$(mktemp -d)
trap 'rm -rf "$juo_tmp"' EXIT
juo_pulse=$juo_source/src/modules/module-protocol-pulse
cat > "$juo_tmp/check.c" <<'C'
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <alloca.h>
#include <spa/param/route.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/utils/list.h>
#include <spa/utils/dict.h>
#include <spa/utils/string.h>
#define PW_KEY_DEVICE_PRODUCT_NAME "device.product.name"
#define MAX_ALLOCA_SIZE 65536
struct pw_device_info { const struct spa_dict *props; };
struct pw_manager_object { struct spa_list param_list; void *info; };
struct pw_manager_param { struct spa_list link; uint32_t id; struct spa_pod *param; };
struct card_info { uint32_t active_profile, n_ports; };
struct device_info { uint32_t direction, device, active_port; };
static uint32_t port_type_value(const char *value) { return spa_streq(value, "hdmi") ? 11 : 0; }
C
awk '/^struct port_info \{/{copy=1} copy{print} copy && /^};/{exit}' "$juo_pulse/collect.h" >> "$juo_tmp/check.c"
awk '/^static bool array_contains\(/{copy=1} copy{print} copy && /^}/{exit}' "$juo_pulse/collect.c" >> "$juo_tmp/check.c"
awk '/^uint32_t collect_port_info\(/{copy=1} copy{print} copy && /^}/{exit}' "$juo_pulse/collect.c" >> "$juo_tmp/check.c"
cat >> "$juo_tmp/check.c" <<'C'
static int select_description(struct pw_manager_object *card, struct card_info card_info,
                              struct device_info dev_info, const char **description) {
    const char *desc = *description;
    struct port_info *port_info, *pi;
    const char *active_port_name = NULL;
    uint32_t n_ports, n;
C
awk '/^static int fill_sink_info\(/{sink=1} sink && /^\tif \(card_info.n_ports >/{copy=1} copy && /^\tflags =/{exit} copy{print}' "$juo_pulse/pulse-server.c" >> "$juo_tmp/check.c"
cat >> "$juo_tmp/check.c" <<'C'
    *description = desc;
    return 0;
}
static const char *sink_description(struct pw_manager_object *card, struct card_info ci,
                                  struct device_info di, const char *desc) {
    assert(select_description(card, ci, di, &desc) == 0);
    return desc;
}
static struct spa_pod *route(void *buffer, size_t size, const char *product,
                            uint32_t index, uint32_t device) {
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, size);
    struct spa_pod_frame object, info;
    uint32_t profile = 1;
    spa_pod_builder_push_object(&b, &object, SPA_TYPE_OBJECT_ParamRoute, SPA_PARAM_EnumRoute);
    spa_pod_builder_add(&b,
        SPA_PARAM_ROUTE_index, SPA_POD_Int(index),
        SPA_PARAM_ROUTE_direction, SPA_POD_Id(SPA_DIRECTION_OUTPUT),
        SPA_PARAM_ROUTE_name, SPA_POD_String(index ? "DP2" : "DP1"),
        SPA_PARAM_ROUTE_description, SPA_POD_String("DisplayPort monitor playback"),
        SPA_PARAM_ROUTE_devices, SPA_POD_Array(sizeof(device), SPA_TYPE_Int, 1, &device),
        SPA_PARAM_ROUTE_profiles, SPA_POD_Array(sizeof(profile), SPA_TYPE_Int, 1, &profile), 0);
    spa_pod_builder_prop(&b, SPA_PARAM_ROUTE_info, 0);
    spa_pod_builder_push_struct(&b, &info);
    spa_pod_builder_int(&b, product ? 3 : 2);
    spa_pod_builder_string(&b, "port.type");
    spa_pod_builder_string(&b, "hdmi");
    spa_pod_builder_string(&b, "port.availability-group");
    spa_pod_builder_string(&b, "DP0");
    if (product) {
        spa_pod_builder_string(&b, PW_KEY_DEVICE_PRODUCT_NAME);
        spa_pod_builder_string(&b, product);
    }
    spa_pod_builder_pop(&b, &info);
    return spa_pod_builder_pop(&b, &object);
}
int main(void) {
    struct spa_dict_item identity = { "api.alsa.card.longname", "POCO-Pad-X1-HDMI" };
    struct spa_dict props = SPA_DICT_INIT(&identity, 1);
    struct pw_device_info info = { &props };
    struct pw_manager_object card = { .info = &info };
    struct card_info ci = { .active_profile = 1, .n_ports = 2 };
    struct device_info di = { .direction = SPA_DIRECTION_OUTPUT, .device = 0, .active_port = 0 };
    struct pw_manager_param p = { .id = SPA_PARAM_EnumRoute }, other = p;
    uint8_t buffer[2048], buffer2[2048];
    struct port_info ports[2];
    const char *active;
    const char *models[] = { "VA3209-QHD", "Other DP monitor", "", NULL };
    spa_list_init(&card.param_list);
    spa_list_append(&card.param_list, &p.link);
    for (unsigned i = 0; i < 4; i++) {
        p.param = route(buffer, sizeof(buffer), models[i], 0, 0);
        assert(collect_port_info(&card, &ci, &di, ports, &active) == 1);
        assert(spa_streq(active, "DP1") && ports[0].type == 11);
        assert(spa_streq(ports[0].availability_group, "DP0"));
        const char *expected = i < 2 ? models[i] : "DisplayPort monitor playback";
        assert(spa_streq(ports[0].description, expected));
        assert(spa_streq(sink_description(&card, ci, di, "Fallback"), i < 2 ? models[i] : "Fallback"));
    }
    p.param = route(buffer, sizeof(buffer), "VA3209-QHD", 0, 0);
    identity.value = "Unrelated sound card";
    assert(collect_port_info(&card, &ci, &di, ports, &active) == 1);
    assert(spa_streq(ports[0].description, "DisplayPort monitor playback"));
    assert(spa_streq(sink_description(&card, ci, di, "Custom name"), "Custom name"));
    identity.value = "POCO-Pad-X1-HDMI";
    other.param = route(buffer2, sizeof(buffer2), "Wrong device", 1, 1);
    spa_list_append(&card.param_list, &other.link);
    assert(collect_port_info(&card, &ci, &di, ports, &active) == 1);
    assert(spa_streq(sink_description(&card, ci, di, "Fallback"), "VA3209-QHD"));
    di.device = 1; di.active_port = 1;
    assert(spa_streq(sink_description(&card, ci, di, "Fallback"), "Wrong device"));
    ci.active_profile = 2;
    assert(collect_port_info(&card, &ci, &di, ports, &active) == 0);
    assert(spa_streq(sink_description(&card, ci, di, "Fallback"), "Fallback"));
    puts("Actual Pulse route and sink monitor-name checks passed");
}
C
${CC:-cc} -std=gnu11 -Wall -Wextra -Werror -Wno-unused-variable \
    -I "$juo_source/spa/include" "$juo_tmp/check.c" -o "$juo_tmp/check"
"$juo_tmp/check"
