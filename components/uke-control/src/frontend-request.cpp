// SPDX-License-Identifier: MIT
// Ordinary-user terminal companion. Authentication stays in
// client::run_terminal; there is no setuid mode, arbitrary executable argument
// or shell expression.
#include "client.hpp"
#include <csignal>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unistd.h>
namespace {
void pause_terminal() {
  std::cout << "Press Enter to close this terminal." << std::flush;
  std::string line;
  std::getline(std::cin, line);
}
struct Interrupts {
  using Handler = void (*)(int);
  Handler interrupt = std::signal(SIGINT, SIG_IGN),
          quit = std::signal(SIGQUIT, SIG_IGN);
  ~Interrupts() {
    std::signal(SIGINT, interrupt);
    std::signal(SIGQUIT, quit);
  }
};
} // namespace
int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--help") {
    std::cout << "Uke Control graphical terminal companion\n"
                 "Usage: frontend-request ACTION_ID [KEY=VALUE ...]\n"
                 "Revalidates a catalog action; existing authentication and "
                 "backend approvals apply.\n";
    return 0;
  }
  bool interactive = false;
  try {
    if (argc < 2 || argc > 34)
      throw std::invalid_argument(
          "Select one catalog action with bounded typed parameters.");
    uke::client::Values values;
    for (int i = 2; i < argc; ++i) {
      const std::string entry = argv[i];
      const auto equal = entry.find('=');
      if (equal == entry.npos || !equal ||
          !values.emplace(entry.substr(0, equal), entry.substr(equal + 1))
               .second)
        throw std::invalid_argument(
            "Parameters must be distinct KEY=VALUE arguments.");
    }
    const auto request = uke::client::plan(argv[1], values);
    if (request.presentation != uke::client::Presentation::terminal)
      throw std::invalid_argument(
          "Use the read-only query interface for captured observations.");
    if (geteuid() == 0)
      throw std::runtime_error(
          "Run Uke Control as your normal session user; authentication applies "
          "only to the selected backend action.");
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
      throw std::runtime_error(
          "This request requires an interactive terminal.");
    interactive = true;
    const auto &action = uke::client::action(request.action_id);
    std::cout
        << "\n"
        << action.title.en << "\n"
        << uke::client::display_command(request.argv) << "\n\n"
        << "Existing authentication and backend confirmations still apply.\n"
        << std::flush;
    // Invoking this ordinary-user companion requests this single allowlisted
    // action, just as invoking the CLI does. A GUI has already reviewed it;
    // the backend remains the authority for privilege/admission/maintenance.
    const auto result = [&] {
      Interrupts ignore;
      return uke::client::run_terminal(request.action_id, request.values, true);
    }();
    std::cout
        << "\n"
        << (result.exit_status == 0 ? "Command completed."
                                    : "Command did not complete.")
        << " Exit status: " << result.exit_status << "\n"
        << "Inspect current device state before making another request.\n";
    pause_terminal();
    return result.exit_status;
  } catch (const std::invalid_argument &error) {
    std::cerr << "Uke Control: " << error.what() << '\n';
    if (interactive)
      pause_terminal();
    return 64;
  } catch (const std::exception &error) {
    std::cerr << "Uke Control: " << error.what() << '\n';
    if (interactive)
      pause_terminal();
    return 1;
  }
}
