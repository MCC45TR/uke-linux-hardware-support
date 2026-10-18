// SPDX-License-Identifier: MIT
#pragma once
#include "model.hpp"
#include <systemd/sd-json.h>
#include <initializer_list>
#include <memory>
#include <sys/types.h>

namespace uke::battery::record {
using Json = std::unique_ptr<sd_json_variant, decltype(&sd_json_variant_unref)>;
using V = sd_json_variant;
[[noreturn]] void reject(const char *reason);
V *field(V *v, const char *key);
std::string string(V *v);
std::string digest(V *v);
bool boolean(V *v);
uint64_t integer(V *v);
Number quantity(V *v, bool allow_zero);
void keys(V *v, std::initializer_list<const char *> allowed);
Json read_json(const std::string &path, uid_t trusted_uid, std::string *bytes = nullptr);
}
