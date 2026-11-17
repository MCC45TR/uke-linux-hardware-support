// SPDX-License-Identifier: GPL-2.0-or-later
// COPR only: original Fedora libraries, real artwork, production geometry and callbacks.
#include "../src/plymouth-uke/plugin.cpp"
#include <cassert>
#include <limits>
#include <fstream>

int main(int argc, char** argv) {
    assert(argc == 3);
    using namespace uke::splash;
    const std::vector<Output> monitors{{3200,2136,237,158}, {3840,2160,952,536}};
    for (const auto& monitor : monitors) for (unsigned scale : {1u, 2u, 3u}) {
        const auto density = physical_density(monitor.width/scale, monitor.height/scale, scale, false, monitors);
        assert(density);
        auto p = layout(monitor.width/scale, monitor.height/scale, density);
        assert(p.physical);
        assert(std::abs(p.logo_w / density->x - 23) <= 0.51 / density->x);
        assert(std::abs(p.logo_h / density->y - 23) <= 0.51 / density->y);
        assert(std::abs(p.bar_w / density->x - 44) <= 0.51 / density->x);
        auto rotated = physical_density(monitor.height/scale, monitor.width/scale, scale, true, monitors);
        assert(rotated && std::abs(rotated->x-density->y)<0.00001);
        auto portrait = layout(monitor.height/scale, monitor.width/scale, rotated);
        assert(std::abs(portrait.logo_w/rotated->x-23)<=0.51/rotated->x);
    }
    assert(!physical_density(3200,2136,0,false,monitors));
    assert(!physical_density(3200,2136,1,false,{{3200,2136,0,0}}));
    assert(!physical_density(3200,2136,1,false,{{3200,2136,237,158},{3200,2136,952,635}}));
    assert(physical_density(3200,2136,1,false,{{3200,2136,237,158},{3200,2136,237,158}}));
    assert(!physical_density(800,600,1,false,monitors));
    const auto small=layout(320,200,std::nullopt);
    assert(!small.physical && small.logo_x>=0 && small.logo_y>=0 && small.fedora_y>=0);

    std::ofstream config(argv[2]);
    config << "[uke-physical]\nImageDir=" << argv[1] << '\n'; config.close();
    auto* key=ply_key_file_new(argv[2]); assert(ply_key_file_load(key));
    auto* api=ply_boot_splash_plugin_get_interface(); auto* plugin=api->create_plugin(key); assert(plugin);
    auto* loop=ply_event_loop_new();
    for (auto mode : {PLY_BOOT_SPLASH_MODE_BOOT_UP, PLY_BOOT_SPLASH_MODE_SHUTDOWN,
                     PLY_BOOT_SPLASH_MODE_REBOOT, PLY_BOOT_SPLASH_MODE_UPDATES,
                     PLY_BOOT_SPLASH_MODE_SYSTEM_UPGRADE, PLY_BOOT_SPLASH_MODE_FIRMWARE_UPGRADE}) {
        // No display exists in the builder; the engine can choose its normal fallback.
        assert(!api->show_splash_screen(plugin,loop,nullptr,mode));
        api->on_boot_progress(plugin,1.0,0.25);
        if (!updating(plugin) && !stopping(plugin)) assert(plugin->progress==0.25);
        api->system_update(plugin,73); assert(plugin->progress==0.73);
        api->system_update(plugin,150); assert(plugin->progress==1);
        api->system_update(plugin,-1); assert(plugin->progress==0);
        api->display_message(plugin,"Offline update"); assert(plugin->message=="Offline update");
        api->hide_message(plugin,"unrelated"); assert(!plugin->message.empty());
        api->hide_message(plugin,"Offline update"); assert(plugin->message.empty());
        api->display_prompt(plugin,"Unlock", "secret-ş", true);
        assert(plugin->prompt.find("secret")==std::string::npos);
        assert(plugin->prompt=="Unlock  ••••••••");
        api->display_question(plugin,"Question", "answer"); assert(plugin->prompt=="Question  answer");
        api->display_normal(plugin); assert(plugin->prompt.empty());
        api->on_boot_output(plugin,"diagnostic log",14); assert(plugin->message.empty());
        auto* trigger=ply_trigger_new(nullptr);
        api->become_idle(plugin,trigger); assert(plugin->idle && !plugin->timer);
        ply_trigger_free(trigger); // This fixture uses no auto-free address.
        api->hide_splash_screen(plugin,loop); assert(!plugin->visible && !plugin->timer && !plugin->loop);
    }
    api->destroy_plugin(plugin); ply_event_loop_free(loop); ply_key_file_free(key);
    auto* label=uke_label_new(); assert(label); uke_label_set_font(label,"Cantarell 20px");
    uke_label_set_text(label,"Offline update 73%");
    assert(uke_label_get_width(label)>1 && uke_label_get_height(label)>1); uke_label_free(label);
    std::puts("PASS: physical centimetres, rotation, device scaling, ambiguity, original artwork/text, update/password callbacks and timer cleanup");
}
