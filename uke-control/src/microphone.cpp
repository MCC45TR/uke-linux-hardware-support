// SPDX-License-Identifier: MIT
#include "microphone.hpp"
#include "backend.hpp"
#include "cli.hpp"
#include "microphone-graph.hpp"
#include "microphone-engine.hpp"
#include <json/json.h>
#include <systemd/sd-login.h>
#include <array>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace uke::microphone {
namespace {
constexpr std::array<const char *, 4> ids = {"uke-amic1", "uke-amic3", "uke-amic4", "uke-amic5"};
constexpr auto setting = "uke.microphone.selection";
constexpr auto processing_setting = "uke.microphone.processing";
constexpr auto agc_setting = "uke.microphone.agc";
bool valid(std::string_view id) {
    if (id == "auto") return true;
    for (auto candidate : ids) if (id == candidate) return true;
    return false;
}
Json::Value parse(const std::string &text) {
    if (text.size() > 4 * 1024 * 1024) throw std::runtime_error("Microphone snapshot exceeds its size limit");
    Json::CharReaderBuilder builder;
    builder["rejectDupKeys"] = true;
    builder["failIfExtra"] = true;
    builder["stackLimit"] = 48;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value value; std::string error;
    if (!reader->parse(text.data(), text.data() + text.size(), &value, &error))
        throw std::runtime_error("Invalid microphone snapshot");
    return value;
}
std::string encode(const Json::Value &value) {
    Json::StreamWriterBuilder builder; builder["indentation"] = "";
    return Json::writeString(builder, value);
}
bool local_session() {
    if (geteuid() == 0 || geteuid() != getuid()) return false;
    char **sessions = nullptr;
    const int count = sd_uid_get_sessions(getuid(), 0, &sessions);
    int matches = 0;
    for (int i = 0; i < count; ++i) {
        char *type = nullptr;
        if (sd_session_is_active(sessions[i]) > 0 && sd_session_is_remote(sessions[i]) == 0 &&
            sd_session_get_type(sessions[i], &type) >= 0 && type && std::string_view(type) == "wayland") ++matches;
        std::free(type); std::free(sessions[i]);
    }
    std::free(sessions);
    return matches == 1;
}
bool array(const Json::Value &props) {
    if (props.get("media.class", "") != "Audio/Source" || props.get("device.api", "") != "alsa" ||
        props.get("audio.channels", 0).asUInt() != 4) return false;
    const auto components = props.get("alsa.components", "").asString();
    static const std::regex marker(R"(^mic:uke-4ch-v4 kernel:[0-9]+\.[0-9]+\.[0-9]+-[0-9]+(-unstable)?$)");
    if (!std::regex_match(components, marker) ||
        (props.get("alsa.card_name", "") != "POCO-Pad-X1" && props.get("alsa.long_card_name", "") != "Xiaomi Pad 7")) return false;
    const auto position = parse(props.get("audio.position", "[]").asString());
    if (!position.isArray() || position.size() != 4) return false;
    for (unsigned i = 0; i < 4; ++i) if (position[i] != "AUX" + std::to_string(i)) return false;
    return true;
}
Json::Value snapshot() {
    Json::Value result(Json::objectValue);
    result["schema_version"] = 1;
    result["selection"] = "auto";
    result["selection_scope"] = "new_implicit_mono_or_stereo_streams";
    result["local_session"] = local_session();
    result["supported"] = false;
    result["policy_available"] = false;
    result["processing_policy_available"] = false;
    result["processing_profile"] = "off";
    result["agc_requested"] = false;
    result["ready"] = false;
    result["reason"] = "no_qualified_v4_array";
    result["cpu_acceptance"] = "not measured";
    result["energy_acceptance"] = "not measured";
    result["orientation_calibrated"] = false;
    result["camera_calibrated"] = false;
    result["oem_dsp_algorithms"] = "not_observable_from_pipewire_definitions_are_not_runtime_evidence";
    result["capture_streams"] = Json::Value(Json::arrayValue);
    for (auto effect : {"aec", "ns", "hpf", "agc", "occlusion"}) {
        result["processing"][effect]["enabled"] = false;
        result["processing"][effect]["supported"] = false;
        result["processing"][effect]["reason"] = "no_verified_processing_path";
    }
    if (!result["local_session"].asBool()) { result["reason"] = "requires_one_active_local_wayland_session"; return result; }
    const auto dump = execute({"/usr/bin/pw-dump"}, 5, 4 * 1024 * 1024);
    if (dump.status) { result["reason"] = "pipewire_unavailable"; return result; }
    const auto objects = parse(dump.output);
    if (!objects.isArray() || objects.size() > 8192) throw std::runtime_error("Invalid PipeWire object list");
    unsigned arrays = 0;
    Json::Value internal(Json::arrayValue);
    for (const auto &object : objects) {
        const auto type = object.get("type", "").asString();
        if (type == "PipeWire:Interface:Node") {
            const auto &props = object["info"]["props"];
            if (array(props)) {
                ++arrays; result["source_id"] = object["id"];
                result["source_name"] = props["node.name"];
                result["components"] = props["alsa.components"];
                result["capture_state"] = object["info"]["state"];
            }
            if (graph::yes(props["uke.microphone.internal"])) internal.append(object);
        }
        if (type != "PipeWire:Interface:Metadata") continue;
        const auto name = object["props"].get("metadata.name", "").asString();
        for (const auto &entry : object["metadata"]) {
            if (entry.get("subject", 1).asUInt() != 0) continue;
            const auto key = entry.get("key", "").asString();
            if (name == "schema-sm-settings" && key == setting) result["policy_available"] = true;
            if (name == "schema-sm-settings" && key == processing_setting) result["processing_policy_available"] = true;
            if (name != "sm-settings") continue;
            if (key == setting && entry["value"].isString()) result["selection"] = entry["value"];
            if (key == processing_setting && entry["value"].isString()) result["processing_profile"] = entry["value"];
            if (key == agc_setting) result["agc_requested"] = graph::yes(entry["value"]);
        }
    }
    const bool supported = arrays == 1 && result["policy_available"].asBool();
    result["supported"] = supported;
    result["reason"] = arrays > 1 ? "ambiguous_array" : supported ? "qualified_policy" : arrays == 1 ? "policy_not_loaded" : "no_qualified_v4_array";
    const graph::Snapshot topology(objects);
    result["capture_streams"] = topology.streams(arrays == 1 ? result["source_id"].asUInt() : 0);
    const bool backend = access("/usr/lib64/spa-0.2/aec/libspa-aec-webrtc.so", R_OK) == 0;
    result["processing_backend_available"] = backend;
    result["processing_paths"] = Json::Value(Json::arrayValue);
    for (const auto &object : internal) {
        const auto &p = object["info"]["props"];
        if (supported && p.get("uke.microphone.id", "") == result["selection"] &&
            p.get("uke.microphone.stage", "") == "source" &&
            p.get("uke.microphone.processing", "") == "none") result["ready"] = true;
        if (p.get("uke.microphone.processing", "") != "webrtc" || p.get("uke.microphone.stage", "") != "source") continue;
        auto path = topology.source(object["id"].asUInt(), arrays == 1 ? result["source_id"].asUInt() : 0);
        result["processing_paths"].append(path);
    }
    for (auto effect : {"aec", "ns", "hpf", "agc"}) {
        const bool available = supported && backend && result["processing_policy_available"].asBool();
        auto &state = result["processing"][effect];
        state["supported"] = available;
        state["requested"] = available && (result["processing_profile"] == "calls" || result["processing_profile"] == "all") &&
            (std::string_view(effect) != "agc" || result["agc_requested"].asBool());
        state["enabled"] = false;
        for (const auto &path : result["processing_paths"]) {
            if (!path["physical_route_verified"].asBool() || path["state"] != "running") continue;
            const auto effects = "," + path["effects_configured"].asString() + ",";
            if (effects.find("," + std::string(effect) + ",") == std::string::npos) continue;
            if (std::string_view(effect) == "aec" && !path["aec_reference_running"].asBool()) continue;
            state["enabled"] = true;
        }
        state["reason"] = available ? "webrtc_graph_inspection_required_for_active_route" : "processing_policy_or_backend_unavailable";
        state["acoustic_acceptance"] = "not_measured";
    }
    if (result["selection"] == "auto") result["ready"] = supported;
    Json::Value microphones(Json::arrayValue);
    try {
        const auto layout = parse(read("/usr/share/senemos/uke/audio/microphone-layout.json", 32768));
        if (layout["microphones"].size() == 4) {
            bool matches = true;
            for (unsigned i = 0; i < 4; ++i)
                matches = matches && layout["microphones"][i]["id"] == ids[i] && layout["microphones"][i]["pcm_index"] == i;
            if (matches) microphones = layout["microphones"];
        }
    } catch (const std::exception &) { /* Status remains usable without the optional installed layout. */ }
    result["microphones"] = microphones;
    return result;
}
}
int command(int argc, char **argv, bool as_json) {
    if (argc == 1 && (std::string_view(argv[0]) == "status" || std::string_view(argv[0]) == "list" ||
                     std::string_view(argv[0]) == "streams")) {
        cli::print(encode(snapshot()), as_json); return 0;
    }
    if (argc == 2 && std::string_view(argv[0]) == "select") {
        const std::string selection = argv[1];
        if (!valid(selection)) throw cli::UsageError("expected auto|uke-amic1|uke-amic3|uke-amic4|uke-amic5");
        auto status = snapshot();
        if (!status["local_session"].asBool() || !status["policy_available"].asBool() ||
            (selection != "auto" && !status["supported"].asBool()))
            throw std::runtime_error("Microphone selection unavailable: " + status["reason"].asString());
        const auto applied = execute({"/usr/bin/wpctl", "settings", "--save", setting, encode(selection)}, 5, 65536);
        if (applied.status) throw std::runtime_error("WirePlumber refused the microphone preference");
        status = snapshot(); status["requested_selection"] = selection;
        status["applies_to"] = "new_streams_existing_recordings_are_preserved";
        cli::print(encode(status), as_json); return 0;
    }
    if (argc == 2 && std::string_view(argv[0]) == "processing" && std::string_view(argv[1]) == "status") {
        cli::print(encode(snapshot()), as_json); return 0;
    }
    if (argc == 2 && std::string_view(argv[0]) == "processing" && std::string_view(argv[1]) == "verify") {
        const auto verification = verify_engine();
        cli::print(encode(verification), as_json); return verification["runnable"].asBool() ? 0 : 1;
    }
    if (argc == 3 && std::string_view(argv[0]) == "processing" && std::string_view(argv[1]) == "select") {
        const std::string profile = argv[2];
        if (profile != "off" && profile != "calls" && profile != "all")
            throw cli::UsageError("expected processing select off|calls|all");
        auto status = snapshot();
        if (!status["local_session"].asBool() || !status["processing_policy_available"].asBool() ||
            (profile != "off" && !status["processing"]["aec"]["supported"].asBool()))
            throw std::runtime_error("Speech processing policy or WebRTC backend unavailable");
        if (profile != "off" && !verify_engine()["runnable"].asBool())
            throw std::runtime_error("WebRTC failed its synthetic runtime verification");
        const auto applied = execute({"/usr/bin/wpctl", "settings", "--save", processing_setting, encode(profile)}, 5, 65536);
        if (applied.status) throw std::runtime_error("WirePlumber refused the speech processing preference");
        status = snapshot(); status["applies_to"] = "new_streams_existing_recordings_are_preserved";
        cli::print(encode(status), as_json); return 0;
    }
    if (argc == 3 && std::string_view(argv[0]) == "processing" && std::string_view(argv[1]) == "agc") {
        const std::string choice = argv[2];
        if (choice != "off" && choice != "on") throw cli::UsageError("expected processing agc off|on");
        const auto status = snapshot();
        if (!status["local_session"].asBool() || !status["processing"]["agc"]["supported"].asBool())
            throw std::runtime_error("Speech processing policy or WebRTC backend unavailable");
        const auto applied = execute({"/usr/bin/wpctl", "settings", "--save", agc_setting, encode(choice == "on")}, 5, 65536);
        if (applied.status) throw std::runtime_error("WirePlumber refused the digital AGC preference");
        cli::print(encode(snapshot()), as_json); return 0;
    }
    if (argc > 0 && (std::string_view(argv[0]) == "processing" || std::string_view(argv[0]) == "profile"))
        throw std::runtime_error("Unsupported processing option; beamforming and automatic acoustic profiles remain unavailable");
    throw cli::UsageError("expected microphone status|list|streams|select ID|processing status|processing select off|calls|all|processing agc off|on");
}
}
