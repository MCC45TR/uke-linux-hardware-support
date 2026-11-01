// SPDX-License-Identifier: MIT
#include "frontend.hpp"
#include <cassert>
#include <iostream>
int main() {
  namespace client = uke::client;
  namespace frontend = uke::frontend;
  assert(frontend::required_confirmation(client::plan("resize.prepare", {})) ==
         "resize.prepare");
  assert(frontend::required_confirmation(client::plan("update.apply", {})) ==
         "APPLY");
  assert(frontend::required_confirmation(client::plan(
             "report.publish", {{"directory", "/tmp/report"},
                                {"repository", "example/support"}})) ==
         "example/support");
  assert(frontend::required_confirmation(
             client::plan("charge.mode", {{"mode", "firmware"}}))
             .empty());
  // These refusals must never spawn a terminal, sudo or a backend command.
  const auto unapproved =
      frontend::launch_terminal("charge.mode", {{"mode", "firmware"}}, false);
  assert(!unapproved.launched && !unapproved.error.empty());
  const auto captured = frontend::launch_terminal("battery.status", {}, true);
  assert(!captured.launched && !captured.error.empty());
  const auto unknown = frontend::launch_terminal("arbitrary.command", {}, true);
  assert(!unknown.launched && !unknown.error.empty());
  const auto invalid = frontend::launch_terminal(
      "charge.mode", {{"mode", "firmware; /bin/false"}}, true);
  assert(!invalid.launched && !invalid.error.empty());
  std::cout << "Native frontend review phrases and no-execution refusal paths "
               "passed\n";
}
