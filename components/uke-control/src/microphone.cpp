// SPDX-License-Identifier: MIT
#include "microphone.hpp"
#include "backend.hpp"
#include "cli.hpp"
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
    static const std::regex marker(R"(^mic:uke-4ch-v4 kernel:[0-9]+\.[0-9]+\.[0-9]+-[0-9]+$)");
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
    result["ready"] = false;
    result["reason"] = "no_qualified_v4_array";
    result["cpu_acceptance"] = "not measured";
    result["energy_acceptance"] = "not measured";
    result["orientation_calibrated"] = false;
    result["camera_calibrated"] = false;
    for (auto effect : {"aec", "ns", "agc", "occlusion"}) {
        result["processing"][effect]["enabled"] = false;
        result["processing"][effect]["supported"] = false;
        result["processing"][effect]["reason"] = "load_and_acoustic_acceptance_missing";
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
            if (props.get("uke.microphone.internal", "") == "true") internal.append(props);
        }
        if (type != "PipeWire:Interface:Metadata") continue;
        const auto name = object["props"].get("metadata.name", "").asString();
        for (const auto &entry : object["metadata"]) {
            if (entry.get("subject", 1).asUInt() != 0 || entry.get("key", "") != setting) continue;
            if (name == "schema-sm-settings") result["policy_available"] = true;
            if (name == "sm-settings" && entry["value"].isString()) result["selection"] = entry["value"];
        }
    }
    const bool supported = arrays == 1 && result["policy_available"].asBool();
    result["supported"] = supported;
    result["reason"] = arrays > 1 ? "ambiguous_array" : supported ? "qualified_policy" : arrays == 1 ? "policy_not_loaded" : "no_qualified_v4_array";
    for (const auto &props : internal) if (supported && props.get("uke.microphone.id", "") == result["selection"]) result["ready"] = true;
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
    if (argc == 1 && (std::string_view(argv[0]) == "status" || std::string_view(argv[0]) == "list")) {
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
    if (argc > 0 && (std::string_view(argv[0]) == "processing" || std::string_view(argv[0]) == "profile"))
        throw std::runtime_error("Unsupported: processing and automatic profiles require separate calibration and load acceptance");
    throw cli::UsageError("expected microphone status|list|select ID|processing status");
}
}
