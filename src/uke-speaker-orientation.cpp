// SPDX-License-Identifier: MIT
#include <alsa/asoundlib.h>
#include <systemd/sd-login.h>
#include <wayland-client.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <list>
#include <optional>
#include <poll.h>
#include <string>
#include <unistd.h>

static volatile sig_atomic_t stopping;
static void stop(int) { stopping = 1; }
static constexpr std::array<const char *, 4> controls = {
    "TopLeft Speaker Rotation", "TopRight Speaker Rotation",
    "BottomLeft Speaker Rotation", "BottomRight Speaker Rotation"
};
static constexpr std::array<const char *, 4> angles = {"0", "90", "180", "270"};

static bool target_card(const char *driver, const char *name) {
    return !std::strcmp(driver, "sm7675") && !std::strcmp(name, "POCO-Pad-X1");
}

static int open_card(snd_ctl_t **result) {
    snd_ctl_t *found = nullptr;
    snd_ctl_card_info_t *info;
    snd_ctl_card_info_alloca(&info);
    int card = -1, ret;
    while ((ret = snd_card_next(&card)) >= 0 && card >= 0) {
        char device[32];
        std::snprintf(device, sizeof device, "hw:%d", card);
        snd_ctl_t *candidate = nullptr;
        if (snd_ctl_open(&candidate, device, SND_CTL_NONBLOCK) < 0) continue;
        if (snd_ctl_card_info(candidate, info) < 0 ||
            !target_card(snd_ctl_card_info_get_driver(info), snd_ctl_card_info_get_name(info))) {
            snd_ctl_close(candidate);
            continue;
        }
        if (found) {
            snd_ctl_close(found);
            snd_ctl_close(candidate);
            return -EEXIST;
        }
        found = candidate;
    }
    if (ret < 0 || !found) {
        if (found) snd_ctl_close(found);
        return ret < 0 ? ret : -ENODEV;
    }
    if ((ret = snd_ctl_subscribe_events(found, 1)) < 0) {
        snd_ctl_close(found);
        return ret;
    }
    *result = found;
    return 0;
}

static int synchronize(snd_ctl_t *ctl, unsigned turn) {
    if (turn >= angles.size()) return -EINVAL;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *value;
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&value);
    // Validate the complete ABI before changing any amplifier.
    for (const char *name : controls) {
        snd_ctl_elem_info_clear(info);
        snd_ctl_elem_info_set_interface(info, SND_CTL_ELEM_IFACE_MIXER);
        snd_ctl_elem_info_set_name(info, name);
        int ret = snd_ctl_elem_info(ctl, info);
        if (ret < 0) return ret;
        if (snd_ctl_elem_info_get_type(info) != SND_CTL_ELEM_TYPE_ENUMERATED ||
            snd_ctl_elem_info_get_count(info) != 1 || snd_ctl_elem_info_get_items(info) != 4)
            return -EPROTO;
        for (unsigned item = 0; item < angles.size(); ++item) {
            snd_ctl_elem_info_set_item(info, item);
            if ((ret = snd_ctl_elem_info(ctl, info)) < 0) return ret;
            if (std::strcmp(snd_ctl_elem_info_get_item_name(info), angles[item])) return -EPROTO;
        }
    }
    for (const char *name : controls) {
        snd_ctl_elem_value_clear(value);
        snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
        snd_ctl_elem_value_set_name(value, name);
        int ret = snd_ctl_elem_read(ctl, value);
        if (ret < 0) return ret;
        if (snd_ctl_elem_value_get_enumerated(value, 0) == turn) continue;
        snd_ctl_elem_value_set_enumerated(value, 0, turn);
        if ((ret = snd_ctl_elem_write(ctl, value)) < 0) return ret;
        if ((ret = snd_ctl_elem_read(ctl, value)) < 0) return ret;
        if (snd_ctl_elem_value_get_enumerated(value, 0) != turn) return -EIO;
    }
    return 0;
}

struct App;
struct Output {
    App *app;
    uint32_t id;
    wl_output *object;
    std::string name;
    int pending_transform = -1, pending_width = 0, pending_height = 0;
    int transform = -1, width = 0, height = 0;
};
struct App {
    std::list<Output> outputs;
    std::optional<unsigned> desired;
    unsigned zero = 0;
    bool dirty = true;
    void refresh() {
        std::optional<unsigned> selected;
        unsigned matches = 0;
        for (const auto &output : outputs) {
            if (output.name != "DSI-1" || output.width != 3200 || output.height != 2136 ||
                output.transform < 0 || output.transform > 3) continue;
            selected = (static_cast<unsigned>(output.transform) + zero) % 4;
            ++matches;
        }
        if (matches != 1) selected.reset();
        if (desired != selected) { desired = selected; dirty = true; }
    }
};
static void geometry(void *data, wl_output *, int32_t, int32_t, int32_t, int32_t,
                     int32_t, const char *, const char *, int32_t transform) {
    static_cast<Output *>(data)->pending_transform = transform;
}
static void mode(void *data, wl_output *, uint32_t flags, int32_t width, int32_t height, int32_t) {
    if (!(flags & WL_OUTPUT_MODE_CURRENT)) return;
    auto &output = *static_cast<Output *>(data);
    output.pending_width = width; output.pending_height = height;
}
static void done(void *data, wl_output *) {
    auto &output = *static_cast<Output *>(data);
    output.transform = output.pending_transform;
    output.width = output.pending_width; output.height = output.pending_height;
    output.app->refresh();
}
static void scale(void *, wl_output *, int32_t) {}
static void name(void *data, wl_output *, const char *value) {
    if (strnlen(value, 64) < 64) static_cast<Output *>(data)->name = value;
}
static void description(void *, wl_output *, const char *) {}
static const wl_output_listener output_listener = {geometry, mode, done, scale, name, description};
static void global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    auto &app = *static_cast<App *>(data);
    if (std::strcmp(interface, "wl_output") || version < 4 || app.outputs.size() >= 32) return;
    auto *object = static_cast<wl_output *>(wl_registry_bind(registry, id, &wl_output_interface, 4));
    if (!object) return;
    app.outputs.push_back(Output{&app, id, object, {}});
    wl_output_add_listener(object, &output_listener, &app.outputs.back());
}
static void removed(void *data, wl_registry *, uint32_t id) {
    auto &app = *static_cast<App *>(data);
    for (auto it = app.outputs.begin(); it != app.outputs.end(); ++it) {
        if (it->id != id) continue;
        if (it->object) wl_output_release(it->object);
        app.outputs.erase(it); app.refresh(); break;
    }
}
static const wl_registry_listener registry_listener = {global, removed};

static bool local_wayland_session(const char *session) {
    if (!session || !*session) return false;
    uid_t uid;
    char *type = nullptr;
    bool local = sd_session_get_uid(session, &uid) >= 0 && uid == getuid() &&
        sd_session_is_remote(session) == 0 &&
        sd_session_get_type(session, &type) >= 0 && !std::strcmp(type, "wayland");
    std::free(type);
    return local;
}

static bool active_session(const char *session) {
    return local_wayland_session(session) && sd_session_is_active(session) == 1;
}

static char *resolve_session() {
    char *session = nullptr;
    if (sd_pid_get_session(0, &session) >= 0) return session;
    if (const char *inherited = std::getenv("XDG_SESSION_ID")) return strdup(inherited);
    // User-manager autostarts may have neither a login cgroup nor an inherited ID.
    char **sessions = nullptr;
    if (sd_uid_get_sessions(getuid(), 0, &sessions) < 0) return nullptr;
    unsigned matches = 0;
    if (sessions) {
        for (unsigned i = 0; sessions[i]; ++i) {
            if (local_wayland_session(sessions[i])) {
                if (++matches == 1) session = strdup(sessions[i]);
            }
            std::free(sessions[i]);
        }
    }
    std::free(sessions);
    if (matches != 1) { std::free(session); session = nullptr; }
    return session;
}

static int read_reference(const char *path, unsigned &zero) {
    zero = 0;
    errno = 0;
    std::ifstream config(path);
    if (!config) return errno == ENOENT ? 0 : -EIO;
    unsigned degrees;
    if (!(config >> degrees) || degrees > 270 || degrees % 90) return -EINVAL;
    config >> std::ws;
    if (!config.eof()) return -EINVAL;
    zero = degrees / 90;
    return 0;
}

int main() {
    const char *session_type = std::getenv("XDG_SESSION_TYPE");
    if (!session_type || std::strcmp(session_type, "wayland") || !std::getenv("WAYLAND_DISPLAY")) return 0;
    App app;
    if (read_reference("/etc/uke-speaker-orientation-zero", app.zero) < 0) {
        std::fputs("uke-speaker-orientation: invalid or unreadable display reference\n", stderr);
        return 1;
    }
    char *session = resolve_session();
    if (!session) {
        std::fputs("uke-speaker-orientation: no identifiable local session\n", stderr);
        return 1;
    }
    sd_login_monitor *monitor = nullptr;
    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    snd_ctl_t *ctl = nullptr;
    snd_ctl_event_t *event;
    snd_ctl_event_alloca(&event);
    int result = 1;
    if (sd_login_monitor_new("session", &monitor) < 0 || !(display = wl_display_connect(nullptr))) goto cleanup;
    registry = wl_display_get_registry(display);
    if (!registry) goto cleanup;
    wl_registry_add_listener(registry, &registry_listener, &app);
    if (wl_display_roundtrip(display) < 0 || wl_display_roundtrip(display) < 0) goto cleanup;
    std::signal(SIGTERM, stop); std::signal(SIGINT, stop);
    {
        auto retry = std::chrono::steady_clock::now();
        int previous_error = 0;
        while (!stopping) {
            bool active = active_session(session);
            if ((!active || !app.desired) && ctl) { snd_ctl_close(ctl); ctl = nullptr; }
            auto now = std::chrono::steady_clock::now();
            if (active && app.desired && app.dirty && now >= retry) {
                int ret = ctl ? 0 : open_card(&ctl);
                if (!ret) ret = synchronize(ctl, *app.desired);
                if (ret < 0) {
                    if (ret != previous_error)
                        std::fprintf(stderr, "uke-speaker-orientation: routing unavailable: %s\n", snd_strerror(ret));
                    if (ctl) { snd_ctl_close(ctl); ctl = nullptr; }
                    retry = now + std::chrono::seconds(1);
                } else app.dirty = false;
                previous_error = ret;
            }
            while (wl_display_prepare_read(display) != 0)
                if (wl_display_dispatch_pending(display) < 0) goto cleanup;
            int flush = wl_display_flush(display);
            if (flush < 0 && errno != EAGAIN) { wl_display_cancel_read(display); goto cleanup; }
            pollfd fds[3] = {{wl_display_get_fd(display), static_cast<short>(POLLIN | (flush < 0 ? POLLOUT : 0)), 0},
                             {sd_login_monitor_get_fd(monitor), POLLIN, 0}, {-1, POLLIN, 0}};
            if (ctl && snd_ctl_poll_descriptors(ctl, &fds[2], 1) != 1) {
                snd_ctl_close(ctl); ctl = nullptr; app.dirty = true;
            }
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(retry - now).count();
            int timeout = active && app.desired && app.dirty ?
                static_cast<int>(remaining > 0 ? remaining + 1 : 0) : -1;
            int polled = poll(fds, 3, timeout);
            if (polled < 0) {
                wl_display_cancel_read(display);
                if (errno == EINTR) continue;
                goto cleanup;
            }
            if (fds[0].revents & POLLIN) {
                if (wl_display_read_events(display) < 0) goto cleanup;
            } else wl_display_cancel_read(display);
            if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) goto cleanup;
            if (fds[1].revents) { sd_login_monitor_flush(monitor); app.dirty = true; }
            if (fds[2].revents && ctl) {
                while (snd_ctl_read(ctl, event) >= 0) {}
                app.dirty = true;
                if (fds[2].revents & (POLLERR | POLLHUP | POLLNVAL)) { snd_ctl_close(ctl); ctl = nullptr; }
            }
            if (wl_display_dispatch_pending(display) < 0) goto cleanup;
        }
        result = 0;
    }
cleanup:
    if (ctl) snd_ctl_close(ctl);
    for (auto &output : app.outputs) wl_output_release(output.object);
    if (registry) wl_registry_destroy(registry);
    if (display) wl_display_disconnect(display);
    if (monitor) sd_login_monitor_unref(monitor);
    std::free(session);
    return result;
}
