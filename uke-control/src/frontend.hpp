// SPDX-License-Identifier: MIT
#pragma once
#include "client.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <stdexcept>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

extern char **environ;
namespace uke::frontend {
struct LaunchResult {
  bool launched = false;
  std::string error;
};
inline std::string required_confirmation(const client::Plan &plan) {
  if (plan.effect == client::Effect::maintenance)
    return plan.action_id;
  if (plan.effect == client::Effect::packages)
    return "APPLY";
  if (plan.effect == client::Effect::publish)
    return plan.values.at("repository");
  return {};
}
// A GUI hands a reviewed, revalidated action to an original distribution
// terminal. Authentication and interactive backend safeguards still happen
// there. Launch success says nothing about command completion or hardware
// application.
inline LaunchResult launch_terminal(std::string_view action,
                                    const client::Values &values,
                                    bool approved) {
  try {
    if (geteuid() == 0)
      throw std::runtime_error(
          "Run the graphical application as your normal session user.");
    const auto request = client::plan(action, values);
    if (!approved)
      throw std::runtime_error(
          "Review and explicitly approve this request first.");
    if (request.presentation != client::Presentation::terminal)
      throw std::runtime_error(
          "Use the read-only query interface for captured observations.");
    std::vector<std::string> arguments;
    if (access("/usr/bin/gnome-terminal", X_OK) == 0)
      arguments = {"/usr/bin/gnome-terminal", "--wait", "--"};
    else if (access("/usr/bin/konsole", X_OK) == 0)
      arguments = {"/usr/bin/konsole", "--separate", "-e"};
    else
      throw std::runtime_error("Install gnome-terminal or konsole to open the "
                               "existing authenticated terminal workflow.");
    constexpr const char *helper = "/usr/libexec/uke-control/frontend-request";
    if (access(helper, X_OK) != 0)
      throw std::runtime_error(
          "Install the matching Uke Control frontend-request helper to keep "
          "terminal results visible.");
    arguments.push_back(helper);
    arguments.push_back(request.action_id);
    for (const auto &[key, value] : request.values)
      arguments.push_back(key + "=" + value);
    // posix_spawn is safe in multithreaded GTK/Qt processes. Do not run C++
    // allocation or toolkit code in a manually forked child. This helper owns
    // only its terminal launcher child, independently of client::Query.
    std::vector<char *> raw;
    for (auto &argument : arguments)
      raw.push_back(argument.data());
    raw.push_back(nullptr);
    posix_spawn_file_actions_t files;
    int error = posix_spawn_file_actions_init(&files);
    pid_t child = -1;
    if (error)
      throw std::runtime_error(strerror(error));
    error =
        posix_spawn_file_actions_addopen(&files, 0, "/dev/null", O_RDONLY, 0);
    if (!error)
      error =
          posix_spawn_file_actions_addopen(&files, 1, "/dev/null", O_WRONLY, 0);
    if (!error)
      error =
          posix_spawn_file_actions_addopen(&files, 2, "/dev/null", O_WRONLY, 0);
    if (!error)
      error = posix_spawn_file_actions_addclosefrom_np(&files, 3);
    if (!error)
      error = posix_spawn(&child, raw[0], &files, nullptr, raw.data(), environ);
    posix_spawn_file_actions_destroy(&files);
    if (error)
      throw std::runtime_error(std::string("Cannot open terminal: ") +
                               strerror(error));
    // The terminal/backend own the interactive operation. A GUI close does
    // not terminate it, and opening a window is never reported as success.
    try {
      std::thread([child] {
        int status;
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
        }
      }).detach();
    } catch (...) {
      return {true, "Terminal opened; launcher tracking unavailable. Inspect "
                    "the terminal before any further request."};
    }
    return {true, {}};
  } catch (const std::exception &error) {
    return {false, error.what()};
  }
}
} // namespace uke::frontend
