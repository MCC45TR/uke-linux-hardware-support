// SPDX-License-Identifier: MIT
#pragma once
#include "usb-port.hpp"
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace uke::usb {
template<class T, void (*Release)(T*)> using DrmPtr = std::unique_ptr<T, decltype(Release)>;
struct DrmFd {
    int value;
    explicit DrmFd(const char* path) : value(open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK)) {}
    ~DrmFd() { if (value >= 0) close(value); }
    DrmFd(const DrmFd&) = delete;
    DrmFd& operator=(const DrmFd&) = delete;
};
inline std::string kms_observation() {
    std::string result = "[";
    std::error_code error;
    unsigned cards = 0;
    for (std::filesystem::directory_iterator it("/dev/dri", error), end;
         !error && it != end; it.increment(error)) {
        const auto name = it->path().filename().string();
        if (name.size() < 5 || name.compare(0, 4, "card") ||
            name.find_first_not_of("0123456789", 4) != std::string::npos) continue;
        if (++cards > 16) break;
        DrmFd fd(it->path().c_str());
        if (fd.value < 0) continue;
        DrmPtr<drmVersion, drmFreeVersion> version(drmGetVersion(fd.value), drmFreeVersion);
        if (!version || !version->name || version->name_len != 3 ||
            std::string(version->name, 3) != "msm") continue;
        DrmPtr<drmModeRes, drmModeFreeResources> resources(drmModeGetResources(fd.value), drmModeFreeResources);
        if (!resources || resources->count_connectors < 0 || resources->count_connectors > 128) continue;
        std::string connectors = "[";
        std::set<uint32_t> active_crtcs;
        for (int i = 0; i < resources->count_connectors; ++i) {
            // Current reads do not force hotplug, EDID/AUX probes or a modeset.
            DrmPtr<drmModeConnector, drmModeFreeConnector> c(
                drmModeGetConnectorCurrent(fd.value, resources->connectors[i]), drmModeFreeConnector);
            if (!c) continue;
            const bool dp = c->connector_type == DRM_MODE_CONNECTOR_DisplayPort;
            const bool dsi = c->connector_type == DRM_MODE_CONNECTOR_DSI;
            if (!dp && !dsi) continue;
            std::optional<uint64_t> max_bpc;
            bool edid_cached = false;
            if (c->count_props < 0 || c->count_props > 256 || c->count_modes < 0 || c->count_modes > 512) continue;
            for (int p = 0; p < c->count_props; ++p) {
                DrmPtr<drmModePropertyRes, drmModeFreeProperty> property(
                    drmModeGetProperty(fd.value, c->props[p]), drmModeFreeProperty);
                if (!property) continue;
                if (std::string(property->name) == "max bpc") max_bpc = c->prop_values[p];
                if (std::string(property->name) == "EDID") edid_cached = c->prop_values[p] != 0;
            }
            uint32_t crtc_id = 0;
            if (c->encoder_id) {
                DrmPtr<drmModeEncoder, drmModeFreeEncoder> encoder(
                    drmModeGetEncoder(fd.value, c->encoder_id), drmModeFreeEncoder);
                if (encoder) crtc_id = encoder->crtc_id;
            }
            DrmPtr<drmModeCrtc, drmModeFreeCrtc> crtc(
                crtc_id ? drmModeGetCrtc(fd.value, crtc_id) : nullptr, drmModeFreeCrtc);
            const bool active = c->connection == DRM_MODE_CONNECTED && crtc && crtc->mode_valid;
            if (active) active_crtcs.insert(crtc_id);
            if (connectors.size() > 1) connectors += ',';
            connectors += "{\"connector_id\":" + std::to_string(c->connector_id);
            connectors += ",\"kind\":\"" + std::string(dp ? "DisplayPort" : "DSI") + "\"";
            connectors += ",\"connected\":" + std::string(c->connection == DRM_MODE_CONNECTED ? "true" : "false");
            connectors += ",\"edid_cached\":" + std::string(edid_cached ? "true" : "false");
            connectors += ",\"requested_max_bpc\":" + (max_bpc ? std::to_string(*max_bpc) : "null");
            connectors += ",\"wire_bpc\":null,\"crtc_id\":" + std::to_string(crtc_id);
            connectors += ",\"active_mode\":";
            if (active) {
                const auto& m = crtc->mode;
                connectors += "{\"width\":" + std::to_string(m.hdisplay) + ",\"height\":" + std::to_string(m.vdisplay);
                connectors += ",\"pixel_clock_khz\":" + std::to_string(m.clock);
                connectors += ",\"x\":" + std::to_string(crtc->x) + ",\"y\":" + std::to_string(crtc->y) + '}';
            } else connectors += "null";
            connectors += ",\"cached_modes\":[";
            for (int mode = 0; mode < c->count_modes; ++mode) {
                const auto& m = c->modes[mode];
                if (mode) connectors += ',';
                connectors += "{\"width\":" + std::to_string(m.hdisplay) + ",\"height\":" + std::to_string(m.vdisplay);
                connectors += ",\"pixel_clock_khz\":" + std::to_string(m.clock);
                connectors += ",\"htotal\":" + std::to_string(m.htotal) + ",\"vtotal\":" + std::to_string(m.vtotal);
                connectors += ",\"interlaced\":" + std::string(m.flags & DRM_MODE_FLAG_INTERLACE ? "true" : "false");
                connectors += ",\"preferred\":" + std::string(m.type & DRM_MODE_TYPE_PREFERRED ? "true" : "false");
                connectors += ",\"hbr2_rgb10_dp2_fits\":" +
                    std::string(choose_dp_lanes({m.hdisplay, m.vdisplay, m.clock, 10, edid_cached, true,
                        bool(m.flags & DRM_MODE_FLAG_INTERLACE)}, 5400, true, false, false).lanes == 2 ? "true" : "false");
                connectors += '}';
            }
            connectors += "]}";
        }
        if (result.size() > 1) result += ',';
        result += "{\"card_index\":" + name.substr(4) + ",\"driver\":\"msm\",\"connectors\":" + connectors + ']';
        result += ",\"distinct_active_crtcs\":" + std::to_string(active_crtcs.size());
        result += ",\"extended_desktop_verified\":null,\"snapshot_atomic\":false}";
    }
    return result + ']';
}
}
