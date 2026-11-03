// SPDX-License-Identifier: MIT
#pragma once
#include <json/json.h>
#include <array>
#include <charconv>
#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <vector>

namespace uke::microphone::graph {
inline unsigned number(const Json::Value &value) {
    if (value.isUInt()) return value.asUInt();
    if (value.isString()) {
        const auto text = value.asString(); unsigned result = 0;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
        if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()) return result;
    }
    return 0;
}
inline const Json::Value &props(const Json::Value &object) { return object["info"]["props"]; }
inline std::string text(const Json::Value &value) {
    if (!value.isString()) return {};
    auto result = value.asString();
    if (result.size() > 160) return {};
    for (auto &byte : result) if (static_cast<unsigned char>(byte) < 32) byte = ' ';
    return result;
}
inline bool yes(const Json::Value &value) {
    return (value.isBool() && value.asBool()) || (value.isString() && value.asString() == "true");
}
struct Snapshot {
    std::map<unsigned, const Json::Value *> nodes, ports;
    std::vector<const Json::Value *> links;
    explicit Snapshot(const Json::Value &objects) {
        if (!objects.isArray() || objects.size() > 8192) throw std::runtime_error("Invalid audio graph snapshot");
        std::set<unsigned> ids;
        for (const auto &object : objects) {
            const auto id = number(object["id"]);
            if (!object["id"].isUInt() || !ids.insert(id).second) throw std::runtime_error("Ambiguous audio graph object identity");
            const auto type = text(object["type"]);
            if (type == "PipeWire:Interface:Node") nodes.emplace(id, &object);
            if (type == "PipeWire:Interface:Port") ports.emplace(id, &object);
            if (type == "PipeWire:Interface:Link") links.push_back(&object);
        }
    }
    const Json::Value *node(unsigned id) const {
        const auto found = nodes.find(id); return found == nodes.end() ? nullptr : found->second;
    }
    bool valid_link(const Json::Value &link) const {
        const auto &info = link["info"];
        const auto source = ports.find(number(info["output-port-id"]));
        const auto target = ports.find(number(info["input-port-id"]));
        return source != ports.end() && target != ports.end() &&
            number(props(*source->second)["node.id"]) == number(info["output-node-id"]) &&
            number(props(*target->second)["node.id"]) == number(info["input-node-id"]);
    }
    std::vector<const Json::Value *> incoming(unsigned id) const {
        std::vector<const Json::Value *> result;
        for (const auto *link : links)
            if (number((*link)["info"]["input-node-id"]) == id && valid_link(*link)) result.push_back(link);
        return result;
    }
    std::set<std::string> raw_channels(unsigned id, unsigned raw) const {
        std::set<std::string> result;
        for (const auto *link : incoming(id)) {
            const auto &info = (*link)["info"];
            if (number(info["output-node-id"]) != raw) continue;
            const auto port = ports.find(number(info["output-port-id"]));
            auto channel = text(props(*port->second)["audio.channel"]);
            if (channel == "AUX0" || channel == "AUX1" || channel == "AUX2" || channel == "AUX3")
                result.insert(channel);
        }
        return result;
    }
    Json::Value source(unsigned id, unsigned raw) const {
        Json::Value result(Json::objectValue);
        const auto *object = node(id);
        if (!object) { result["kind"] = "unknown"; return result; }
        const auto &p = props(*object);
        result["node_id"] = id;
        result["node_name"] = text(p["node.name"]);
        result["state"] = (*object)["info"]["state"];
        if (id == raw && raw) { result["kind"] = "raw_array"; return result; }
        const auto path = text(p["uke.microphone.path"]);
        if (path.empty() || !yes(p["uke.microphone.internal"])) {
            result["kind"] = "external_or_unverified_source"; return result;
        }
        result["kind"] = text(p["uke.microphone.processing"]) == "webrtc" ? "speech" : "selected_channel";
        result["claimed_microphone"] = text(p["uke.microphone.id"]);
        unsigned capture = 0, reference = 0;
        bool ambiguous = false;
        for (const auto &[candidate_id, candidate] : nodes) {
            const auto &q = props(*candidate);
            if (text(q["uke.microphone.path"]) != path) continue;
            const auto stage = text(q["uke.microphone.stage"]);
            if (stage == "capture") { if (capture) ambiguous = true; capture = candidate_id; }
            if (stage == "reference") { if (reference) ambiguous = true; reference = candidate_id; }
        }
        const auto channels = !ambiguous && capture ? raw_channels(capture, raw) : std::set<std::string>{};
        Json::Value physical(Json::arrayValue);
        static constexpr std::array<const char *, 4> identities{"uke-amic1", "uke-amic3", "uke-amic4", "uke-amic5"};
        for (const auto &channel : channels) physical.append(identities[size_t(channel.back() - '0')]);
        result["physical_microphones"] = physical;
        const auto capture_links = capture ? incoming(capture) : std::vector<const Json::Value *>{};
        const auto expected = text(p["uke.microphone.id"]);
        result["physical_route_verified"] = !ambiguous && channels.size() == 1 && capture_links.size() == 1 &&
            physical.size() == 1 && physical[0] == expected;
        result["aec_reference_linked"] = false;
        result["aec_reference_running"] = false;
        if (reference && !ambiguous) for (const auto *link : incoming(reference)) {
            const auto *sink = node(number((*link)["info"]["output-node-id"]));
            if (!sink || text(props(*sink)["media.class"]) != "Audio/Sink") continue;
            result["aec_reference_linked"] = true;
            if (text((*link)["info"]["state"]) == "active") result["aec_reference_running"] = true;
        }
        result["engine_requested"] = text(p["uke.microphone.processing"]);
        result["effects_configured"] = text(p["uke.microphone.effects"]);
        result["acoustic_acceptance"] = "not_measured";
        return result;
    }
    Json::Value streams(unsigned raw) const {
        Json::Value result(Json::arrayValue);
        for (const auto &[id, object] : nodes) {
            const auto &p = props(*object);
            if (text(p["media.class"]) != "Stream/Input/Audio" || yes(p["uke.microphone.internal"])) continue;
            Json::Value stream(Json::objectValue), routes(Json::arrayValue);
            stream["node_id"] = id;
            stream["application"] = text(p["application.name"]);
            stream["state"] = (*object)["info"]["state"];
            stream["requested_channels"] = number(p["audio.channels"]);
            stream["application_internal_processing"] = "not_observable";
            const auto links_to_stream = incoming(id);
            for (const auto *link : links_to_stream) {
                const auto &info = (*link)["info"];
                auto route = source(number(info["output-node-id"]), raw);
                const auto source_port = ports.at(number(info["output-port-id"]));
                const auto input_port = ports.at(number(info["input-port-id"]));
                route["source_channel"] = text(props(*source_port)["audio.channel"]);
                route["application_channel"] = text(props(*input_port)["audio.channel"]);
                route["link_state"] = info["state"];
                if (number(info["output-node-id"]) == raw && raw) {
                    auto channel = route["source_channel"].asString();
                    if (channel.size() == 4 && channel.starts_with("AUX") && channel[3] >= '0' && channel[3] <= '3') {
                        static constexpr const char *ids[] = {"uke-amic1", "uke-amic3", "uke-amic4", "uke-amic5"};
                        route["physical_microphones"].append(ids[channel[3] - '0']);
                        route["physical_route_verified"] = true;
                    }
                }
                routes.append(route);
            }
            stream["routes"] = routes;
            stream["route_status"] = routes.empty() ? "not_linked_or_not_observable" : "linked";
            result.append(stream);
        }
        return result;
    }
};
} // namespace uke::microphone::graph
