// SPDX-License-Identifier: GPL-2.0-or-later
// A theme plugin for the unmodified Fedora Plymouth engine. No private ABI or modesetting.
#include "physical-layout.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
extern "C" {
#include <ply-boot-splash-plugin.h>
#include <ply-image.h>
#include "label-bridge.h"
#include <ply-renderer.h>
}

namespace uke_splash {
struct ImageDeleter { void operator()(ply_image_t* image) const { if (image) ply_image_free(image); } };
using Image = std::unique_ptr<ply_image_t, ImageDeleter>;
Image load(const std::string& directory, const char* name) {
    Image image(ply_image_new((directory + "/" + name).c_str()));
    if (!image || !ply_image_load(image.get())) return {};
    return image;
}
struct View {
    ply_pixel_display_t* display{};
    uke_label_t* label{uke_label_new()};
    uke::splash::Layout position{};
    Image logo, fedora, track, cap, center;
    ~View() {
        ply_pixel_display_set_draw_handler(display, nullptr, nullptr);
        if (label) { uke_label_hide(label); uke_label_free(label); }
    }
};
std::vector<uke::splash::Output> outputs(ply_renderer_t* renderer) {
    std::vector<uke::splash::Output> result;
    const char* name = ply_renderer_get_device_name(renderer);
    if (!name || ply_renderer_get_type(renderer) != PLY_RENDERER_TYPE_DRM) return result;
    const int fd = open(name, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return result;
    drmModeRes* resources = drmModeGetResources(fd);
    if (resources && resources->count_connectors <= 128) {
        for (int i = 0; i < resources->count_connectors; ++i) {
            // Cached DRM metadata only: never force EDID detection or acquire DRM master.
            drmModeConnector* connector = drmModeGetConnectorCurrent(fd, resources->connectors[i]);
            if (!connector) continue;
            if (connector->connection == DRM_MODE_CONNECTED) {
                drmModeEncoder* encoder = connector->encoder_id ? drmModeGetEncoder(fd, connector->encoder_id) : nullptr;
                drmModeCrtc* crtc = encoder && encoder->crtc_id ? drmModeGetCrtc(fd, encoder->crtc_id) : nullptr;
                if (crtc && crtc->mode_valid) {
                    result.push_back({crtc->mode.hdisplay, crtc->mode.vdisplay, connector->mmWidth, connector->mmHeight});
                } else if (connector->count_modes <= 256) {
                    // Plymouth can publish its display before applying the selected mode.
                    // The exact display size selects an advertised mode, with ambiguity refused below.
                    for (int j = 0; j < connector->count_modes; ++j)
                        result.push_back({connector->modes[j].hdisplay, connector->modes[j].vdisplay,
                                          connector->mmWidth, connector->mmHeight});
                }
                if (crtc) drmModeFreeCrtc(crtc);
                if (encoder) drmModeFreeEncoder(encoder);
            }
            drmModeFreeConnector(connector);
        }
    }
    if (resources) drmModeFreeResources(resources);
    close(fd);
    return result;
}
}

struct _ply_boot_splash_plugin {
    std::vector<std::unique_ptr<uke_splash::View>> views;
    uke_splash::Image logo, fedora, track, cap, center;
    ply_event_loop_t* loop{};
    ply_boot_splash_mode_t mode{PLY_BOOT_SPLASH_MODE_BOOT_UP};
    std::string message, prompt;
    double progress{}, phase{};
    bool visible{}, timer{}, idle{};
};

namespace uke_splash {
using Plugin = ply_boot_splash_plugin_t;
bool updating(const Plugin* plugin) {
    return plugin->mode == PLY_BOOT_SPLASH_MODE_UPDATES || plugin->mode == PLY_BOOT_SPLASH_MODE_SYSTEM_UPGRADE ||
           plugin->mode == PLY_BOOT_SPLASH_MODE_FIRMWARE_UPGRADE || plugin->mode == PLY_BOOT_SPLASH_MODE_SYSTEM_RESET;
}
bool stopping(const Plugin* plugin) {
    return plugin->mode == PLY_BOOT_SPLASH_MODE_SHUTDOWN || plugin->mode == PLY_BOOT_SPLASH_MODE_REBOOT;
}
void draw(void* data, ply_pixel_buffer_t* buffer, int x, int y, int width, int height,
          ply_pixel_display_t* display) {
    auto* plugin = static_cast<Plugin*>(data);
    const auto it = std::find_if(plugin->views.begin(), plugin->views.end(),
                                 [display](const auto& view) { return view->display == display; });
    if (it == plugin->views.end() || !plugin->visible) return;
    auto& view = **it;
    const auto& p = view.position;
    ply_rectangle_t clip{x, y, static_cast<unsigned long>(width), static_cast<unsigned long>(height)};
    ply_pixel_buffer_fill_with_hex_color(buffer, &clip, 0xff000000);
    if (view.logo) ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.logo.get()), p.logo_x, p.logo_y);
    if (view.fedora) ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.fedora.get()), p.fedora_x, p.fedora_y);
    ply_rectangle_t bar{p.bar_x, p.bar_y, p.bar_w, p.bar_h};
    if (view.track) ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.track.get()), p.bar_x, p.bar_y);
    bar.width = static_cast<unsigned long>(std::round(p.bar_w * plugin->progress));
    if (bar.width && view.cap && view.center) {
        ply_pixel_buffer_push_clip_area(buffer, &bar);
        const auto cap_width = ply_image_get_width(view.cap.get());
        ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.cap.get()), p.bar_x, p.bar_y);
        if (bar.width >= static_cast<unsigned long>(cap_width)) {
            ply_rectangle_t middle{p.bar_x + cap_width/2, p.bar_y, bar.width - cap_width, p.bar_h};
            ply_pixel_buffer_push_clip_area(buffer, &middle);
            ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.center.get()), p.bar_x, p.bar_y);
            ply_pixel_buffer_pop_clip_area(buffer);
            ply_pixel_buffer_fill_with_buffer(buffer, ply_image_get_buffer(view.cap.get()), p.bar_x + bar.width - cap_width, p.bar_y);
        }
        ply_pixel_buffer_pop_clip_area(buffer);
    }
    if (view.label) uke_label_draw_area(view.label, buffer, x, y, width, height);
}
void redraw(Plugin* plugin, bool full) {
    if (!plugin->visible) return;
    for (auto& view : plugin->views) {
        if (full) ply_pixel_display_draw_area(view->display, 0, 0,
                        ply_pixel_display_get_width(view->display), ply_pixel_display_get_height(view->display));
        else {
            const auto& p = view->position;
            ply_pixel_display_draw_area(view->display, p.bar_x, p.bar_y, p.bar_w, p.bar_h);
        }
    }
}
void label(Plugin* plugin) {
    std::string text = plugin->prompt.empty() ? plugin->message : plugin->prompt;
    if (text.empty() && updating(plugin)) text = std::to_string(int(plugin->progress * 100)) + "%";
    for (auto& view : plugin->views) {
        if (!view->label) continue;
        uke_label_hide(view->label);
        uke_label_set_text(view->label, text.c_str());
        uke_label_center(view->label);
        uke_label_set_width(view->label, ply_pixel_display_get_width(view->display) * 4 / 5);
        const auto font = "Cantarell " + std::to_string(view->position.font_px) + "px";
        uke_label_set_font(view->label, font.c_str());
        uke_label_set_color(view->label, 0.82f, 0.82f, 0.82f, 1.0f);
        if (!text.empty() && plugin->visible)
            uke_label_show(view->label, view->display, ply_pixel_display_get_width(view->display) / 10,
                           view->position.message_y);
    }
    redraw(plugin, true);
}
void configure(Plugin* plugin) {
    for (auto& view : plugin->views) {
        auto* renderer = ply_pixel_display_get_renderer(view->display);
        auto* buffer = ply_renderer_get_buffer_for_head(renderer, ply_pixel_display_get_renderer_head(view->display));
        const auto rotation = ply_pixel_buffer_get_device_rotation(buffer);
        const bool rotated = rotation == PLY_PIXEL_BUFFER_ROTATE_CLOCKWISE || rotation == PLY_PIXEL_BUFFER_ROTATE_COUNTER_CLOCKWISE;
        const auto density = uke::splash::physical_density(ply_pixel_display_get_width(view->display),
            ply_pixel_display_get_height(view->display), ply_pixel_display_get_device_scale(view->display), rotated, outputs(renderer));
        view->position = uke::splash::layout(ply_pixel_display_get_width(view->display),
                                             ply_pixel_display_get_height(view->display), density);
        const auto& p = view->position;
        if (plugin->logo) view->logo.reset(ply_image_resize(plugin->logo.get(), p.logo_w, p.logo_h));
        if (plugin->fedora) view->fedora.reset(ply_image_resize(plugin->fedora.get(), p.fedora_w, p.fedora_h));
        if (plugin->track) view->track.reset(ply_image_resize(plugin->track.get(), p.bar_w, p.bar_h));
        const unsigned cap_width = std::max(1u, unsigned(std::round(p.logo_w * 1.2 / 23)));
        if (plugin->cap) view->cap.reset(ply_image_resize(plugin->cap.get(), cap_width, p.bar_h));
        if (plugin->center) view->center.reset(ply_image_resize(plugin->center.get(), p.bar_w, p.bar_h));
    }
    label(plugin);
}
void tick(void* data, ply_event_loop_t* loop) {
    auto* plugin = static_cast<Plugin*>(data);
    plugin->timer = false;
    if (!plugin->visible || plugin->idle || !stopping(plugin)) return;
    plugin->phase += 0.25;
    plugin->progress = (std::sin(plugin->phase) + 1) / 2;
    redraw(plugin, false); // Only the progress rectangle is damaged at 8 Hz during shutdown.
    plugin->timer = true;
    ply_event_loop_watch_for_timeout(loop, 0.125, tick, plugin);
}
void cancel_timer(Plugin* plugin) {
    if (plugin->loop && plugin->timer) ply_event_loop_stop_watching_for_timeout(plugin->loop, tick, plugin);
    plugin->timer = false;
}
void loop_exit(void* data, int, ply_event_loop_t*) {
    auto* plugin = static_cast<Plugin*>(data);
    plugin->loop = nullptr; plugin->timer = false; plugin->visible = false;
}
Plugin* create(ply_key_file_t* key) {
    auto* plugin = new (std::nothrow) Plugin;
    if (!plugin) return nullptr;
    char* configured = key ? ply_key_file_get_value(key, "uke-physical", "ImageDir") : nullptr;
    const std::string directory = configured ? configured : "/usr/share/plymouth/themes/senemos-uke";
    free(configured);
    plugin->logo = load(directory, "xiaomi-logo.png"); plugin->fedora = load(directory, "fedora-logo.png");
    plugin->track = load(directory, "progress-track.png"); plugin->cap = load(directory, "progress-cap.png");
    plugin->center = load(directory, "progress-center.png");
    if (!plugin->logo || !plugin->fedora || !plugin->track || !plugin->cap || !plugin->center) { delete plugin; return nullptr; }
    return plugin;
}
void hide(Plugin* plugin, ply_event_loop_t*) {
    cancel_timer(plugin); plugin->visible = false;
    for (auto& view : plugin->views) if (view->label) uke_label_hide(view->label);
    if (plugin->loop) ply_event_loop_stop_watching_for_exit(plugin->loop, loop_exit, plugin);
    plugin->loop = nullptr;
}
void destroy(Plugin* plugin) { hide(plugin, plugin->loop); delete plugin; }
void add(Plugin* plugin, ply_pixel_display_t* display) {
    if (std::any_of(plugin->views.begin(), plugin->views.end(), [display](const auto& v) { return v->display == display; })) return;
    auto view = std::make_unique<View>(); view->display = display;
    plugin->views.push_back(std::move(view));
    ply_pixel_display_set_draw_handler(display, draw, plugin);
    configure(plugin); // Re-evaluate all heads on hotplug; no periodic connector polling.
}
void remove(Plugin* plugin, ply_pixel_display_t* display) {
    plugin->views.erase(std::remove_if(plugin->views.begin(), plugin->views.end(),
        [display](const auto& v) { return v->display == display; }), plugin->views.end());
    configure(plugin);
}
bool show(Plugin* plugin, ply_event_loop_t* loop, ply_buffer_t*, ply_boot_splash_mode_t mode) {
    if (plugin->visible) hide(plugin, plugin->loop);
    plugin->mode = mode; plugin->loop = loop; plugin->visible = true; plugin->idle = false;
    ply_event_loop_watch_for_exit(loop, loop_exit, plugin);
    configure(plugin);
    if (stopping(plugin)) tick(plugin, loop);
    return !plugin->views.empty();
}
void progress(Plugin* plugin, double, double fraction) {
    if (updating(plugin) || stopping(plugin)) return;
    plugin->progress = std::isfinite(fraction) ? std::clamp(fraction, 0.0, 1.0) : 0;
    redraw(plugin, false);
}
void update(Plugin* plugin, int percent) {
    plugin->progress = std::clamp(percent, 0, 100) / 100.0;
    label(plugin);
}
void message(Plugin* plugin, const char* text) { plugin->message = text ? text : ""; label(plugin); }
void hide_message(Plugin* plugin, const char* text) {
    if (!text || plugin->message == text) { plugin->message.clear(); label(plugin); }
}
void normal(Plugin* plugin) { plugin->prompt.clear(); label(plugin); }
void password(Plugin* plugin, const char* text, int bullets) {
    plugin->prompt = text ? text : ""; plugin->prompt += "  ";
    for (int i = 0; i < std::clamp(bullets, 0, 256); ++i) plugin->prompt += "•";
    label(plugin);
}
void question(Plugin* plugin, const char* text, const char* entry) {
    plugin->prompt = std::string(text ? text : "") + "  " + (entry ? entry : ""); label(plugin);
}
void prompt(Plugin* plugin, const char* text, const char* entry, bool secret) {
    if (secret) {
        // Count UTF-8 codepoints, never display or retain the secret.
        int bullets = 0;
        if (entry) for (const unsigned char* p = reinterpret_cast<const unsigned char*>(entry); *p; ++p)
            if ((*p & 0xc0) != 0x80) ++bullets;
        password(plugin, text, bullets);
    } else question(plugin, text, entry);
}
void idle(Plugin* plugin, ply_trigger_t* trigger) {
    plugin->idle = true; cancel_timer(plugin); ply_trigger_pull(trigger, nullptr);
}
void status(Plugin* plugin, const char* text) { if (updating(plugin)) message(plugin, text); }
void boot_output(Plugin*, const char*, size_t) {} // The journal remains the diagnostic log.
}

extern "C" __attribute__((visibility("default")))
ply_boot_splash_plugin_interface_t* ply_boot_splash_plugin_get_interface() {
    using namespace uke_splash;
    static ply_boot_splash_plugin_interface_t interface{};
    interface.create_plugin = create; interface.destroy_plugin = destroy;
    interface.add_pixel_display = add; interface.remove_pixel_display = remove;
    interface.show_splash_screen = show; interface.hide_splash_screen = hide;
    interface.on_boot_progress = progress; interface.system_update = update;
    interface.update_status = status; interface.on_boot_output = boot_output;
    interface.display_message = message; interface.hide_message = hide_message;
    interface.display_normal = normal; interface.display_password = password;
    interface.display_question = question; interface.display_prompt = prompt;
    interface.become_idle = idle;
    return &interface;
}
