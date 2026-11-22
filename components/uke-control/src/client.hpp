// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace uke::client {
inline constexpr unsigned api_version=1;
enum class Language { english, turkish };
struct Text {
    std::string en,tr;
    const std::string& get(Language language) const {return language==Language::turkish?tr:en;}
};
enum class Effect { read, local_file, device, packages, maintenance, publish };
enum class Access { user, active_session, root };
enum class Presentation { capture, terminal };
enum class Kind { choice, integer, path, absolute_path, identifier, interface, usb_device, input_device, partition_uuid, color, swap_size, repository };
struct Choice {std::string value;Text title,description;};
struct Parameter {
    std::string id;
    Text title,description;
    Kind kind=Kind::identifier;
    std::string initial;
    std::vector<Choice> choices;
    std::uint64_t minimum=0,maximum=0;
    bool optional=false;
};
struct Category {std::string id;Text title,description;};
struct Action {
    std::string id,category;
    Text title,description;
    std::vector<std::string> arguments;
    std::vector<Parameter> parameters;
    Effect effect=Effect::read;
    Access access=Access::user;
    Presentation presentation=Presentation::capture;
    unsigned timeout_seconds=20;
    std::string inspect,preview;
    bool tests_package=false;
};
using Values=std::map<std::string,std::string>;
struct Plan {
    std::string action_id;
    Values values;
    std::vector<std::string> argv;
    Effect effect;
    Access access;
    Presentation presentation;
    unsigned timeout_seconds;
    bool confirmation_required;
};
const std::vector<Category>& categories();
const std::vector<Action>& actions();
const Action& action(std::string_view id);
Values defaults(const Action& action);
Plan plan(std::string_view id,const Values& values);
void validate_value(const Parameter& parameter,const std::string& value);
std::vector<std::string> search(std::string_view query,std::string_view category={},Language language=Language::turkish);
std::string catalog_json();
std::string describe_json(std::string_view id);
std::string plan_json(const Plan& plan);
std::string effect_name(Effect effect);
std::string access_name(Access access);
Text effect_description(Effect effect);
std::string display_command(const std::vector<std::string>& argv);
std::string format_output(const std::string& output,Language language,bool raw=false);

struct Result {
    std::string action_id,output,error;
    int exit_status=-1;
    bool running=false,cancelled=false,timed_out=false,truncated=false;
    std::uint64_t elapsed_ms=0;
};
// Owns only an allowlisted read query. GUI event loops may call poll(); no UI dependency.
// Single-thread owner: poll/cancel/result and destruction use the same event loop.
// Do not independently reap the owned child with another SIGCHLD handler.
class Query {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    Query(std::string_view action,const Values& values={});
    ~Query();
    Query(const Query&)=delete;
    Query& operator=(const Query&)=delete;
    void poll();
    void cancel();
    const Result& result() const;
};
// Foreground terminal handoff; writes require explicit approval, and are never timed out.
Result run_terminal(std::string_view action,const Values& values,bool approved);
std::string receipt_json(const Result& result,const Values& values);
void save_receipt(const std::string& new_file,const Result& result,const Values& values);
}
