// SPDX-License-Identifier: MIT
#pragma once
#include <json/json.h>
#include <spa/support/plugin.h>
#include <spa/interfaces/audio/aec.h>
#include <spa/param/audio/raw.h>
#include <dlfcn.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace uke::microphone {
// This uses synthetic memory buffers only. It never opens an audio device.
inline Json::Value verify_engine() {
    Json::Value result(Json::objectValue);
    result["engine"] = "webrtc";
    result["input"] = "synthetic_memory_only";
    result["acoustic_acceptance"] = "not_measured";
    result["runnable"] = false;
    void *library = dlopen("/usr/lib64/spa-0.2/aec/libspa-aec-webrtc.so", RTLD_NOW | RTLD_LOCAL);
    if (!library) { result["reason"] = "backend_load_failed"; return result; }
    struct Library { void *value; ~Library() { dlclose(value); } } loaded{library};
    using Enumerate = int (*)(const spa_handle_factory **, uint32_t *);
    const auto enumerate = reinterpret_cast<Enumerate>(dlsym(library, SPA_HANDLE_FACTORY_ENUM_FUNC_NAME));
    if (!enumerate) { result["reason"] = "backend_factory_missing"; return result; }
    const spa_handle_factory *factory = nullptr;
    uint32_t index = 0;
    if (enumerate(&factory, &index) <= 0 || !factory) { result["reason"] = "backend_factory_missing"; return result; }
    const auto bytes = spa_handle_factory_get_size(factory, nullptr);
    if (!bytes || bytes > 16 * 1024 * 1024) { result["reason"] = "invalid_backend_size"; return result; }
    std::unique_ptr<void, decltype(&std::free)> storage(std::calloc(1, bytes), std::free);
    if (!storage) throw std::runtime_error("Speech verification memory unavailable");
    auto *handle = static_cast<spa_handle *>(storage.get());
    if (spa_handle_factory_init(factory, handle, nullptr, nullptr, 0) < 0) {
        result["reason"] = "backend_initialization_failed"; return result;
    }
    struct Handle { spa_handle *value; ~Handle() { spa_handle_clear(value); } } initialized{handle};
    spa_audio_aec *aec = nullptr;
    if (spa_handle_get_interface(handle, SPA_TYPE_INTERFACE_AUDIO_AEC, reinterpret_cast<void **>(&aec)) < 0 || !aec ||
        !aec->name || std::string_view(aec->name) != "webrtc") {
        result["reason"] = "aec_interface_missing"; return result;
    }
    spa_dict_item options[] = {
        {"webrtc.high_pass_filter", "true"}, {"webrtc.noise_suppression", "true"},
        {"webrtc.gain_control", "false"},
    };
    spa_dict args = SPA_DICT_INIT_ARRAY(options);
    spa_audio_info_raw capture{}, output{}, reference{};
    capture.format = output.format = reference.format = SPA_AUDIO_FORMAT_F32P;
    capture.rate = output.rate = reference.rate = 48000;
    capture.channels = output.channels = 1;
    reference.channels = 2;
    capture.position[0] = output.position[0] = SPA_AUDIO_CHANNEL_MONO;
    reference.position[0] = SPA_AUDIO_CHANNEL_FL; reference.position[1] = SPA_AUDIO_CHANNEL_FR;
    if (spa_audio_aec_init2(aec, &args, &capture, &output, &reference) < 0) {
        result["reason"] = "aec_configuration_failed"; return result;
    }
    if (capture.channels != 1 || output.channels != 1 || reference.channels != 2 ||
        capture.rate != 48000 || output.rate != 48000 || reference.rate != 48000) {
        result["reason"] = "backend_changed_the_buffer_contract"; return result;
    }
    std::array<float, 480> input{}, left{}, right{}, processed{}, unexpected_output{};
    const float *microphone[] = {input.data()}, *render[] = {left.data(), right.data()};
    // Two bounded buffers also contain a backend with incompatible output semantics.
    float *destination[] = {processed.data(), unexpected_output.data()};
    double input_energy = 0, output_energy = 0;
    for (unsigned block = 0; block < 100; ++block) {
        // A fixed DC offset exercises the enabled high-pass filter deterministically.
        input.fill(0.05f); processed.fill(NAN); unexpected_output.fill(NAN);
        if (spa_audio_aec_run(aec, microphone, render, destination, input.size()) < 0) {
            result["reason"] = "aec_processing_failed"; return result;
        }
        for (auto value : unexpected_output) if (!std::isnan(value)) {
            result["reason"] = "backend_output_contract_mismatch"; return result;
        }
        for (auto value : processed) {
            if (!std::isfinite(value) || std::abs(value) > 1.0f) {
                result["reason"] = "invalid_backend_output"; return result;
            }
            if (block >= 80) output_energy += double(value) * value;
        }
        if (block >= 80) input_energy += input.size() * 0.05 * 0.05;
    }
    if (output_energy >= input_energy * 0.1) { result["reason"] = "high_pass_response_missing"; return result; }
    result["runnable"] = true;
    result["processed_frames"] = 48000;
    result["reason"] = "backend_processed_finite_synthetic_frames_and_removed_dc";
    return result;
}
} // namespace uke::microphone
