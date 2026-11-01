// SPDX-License-Identifier: MIT
#include "fixture.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
namespace c = uke::client;
void require(bool value, const char *reason) {
  if (!value)
    throw std::runtime_error(reason);
}
int main() {
  char pattern[] = "/tmp/uke-gtk-fixture-XXXXXX";
  auto *directory = mkdtemp(pattern);
  if (!directory)
    return 1;
  std::filesystem::path base = directory;
  try {
    auto write = [&](const std::string &bytes) {
      auto path = base / "input.json";
      std::ofstream(path) << bytes;
      return path.string();
    };
    auto rejects = [&](const std::string &bytes) {
      try {
        uke::gtkui::Fixture fixture(write(bytes));
        return false;
      } catch (const std::exception &) {
        return true;
      }
    };
    uke::gtkui::Fixture fixture(write(
        R"({"schema":"org.uke.control.tui-fixture.v1","results":{"status":{"data":{"unknown":null,"cycles":0,"current":-42,"supported":false},"stderr":"provider error","status":69,"delay_ms":100}}})"));
    auto sample = fixture.sample("status");
    require(sample.result.exit_status == 69, "error exit status lost");
    require(sample.result.error == "provider error", "stderr lost");
    require(sample.delay_ms == 100, "delay lost");
    require(sample.result.output.find("null") != std::string::npos,
            "null lost");
    require(sample.result.output.find("-42") != std::string::npos,
            "negative reading lost");
    require(sample.result.output.find("false") != std::string::npos,
            "false reading lost");
    require(fixture.sample("charge.status").result.exit_status == 69,
            "missing result must be unavailable");
    require(rejects(R"({"schema":"wrong","results":{}})"), "schema accepted");
    require(
        rejects(
            R"({"schema":"org.uke.control.tui-fixture.v1","results":{"arbitrary.command":{}}})"),
        "unknown action accepted");
    require(
        rejects(
            R"({"schema":"org.uke.control.tui-fixture.v1","results":{"status":{"delay_ms":30001}}})"),
        "unbounded delay accepted");
    require(
        rejects(
            R"({"schema":"org.uke.control.tui-fixture.v1","results":{"status":{"status":-1}}})"),
        "invalid status accepted");
    require(
        rejects(
            R"({"schema":"org.uke.control.tui-fixture.v1","results":{"status":{"stdout":false}}})"),
        "non-string output accepted");
    require(
        rejects(
            R"({"schema":"org.uke.control.tui-fixture.v1","results":{},"results":{}})"),
        "duplicate keys accepted");
    auto link = base / "link";
    std::filesystem::create_symlink(base / "input.json", link);
    try {
      uke::gtkui::Fixture invalid(link.string());
      throw std::runtime_error("symlink accepted");
    } catch (const std::exception &e) {
      require(std::string(e.what()) != "symlink accepted", "symlink accepted");
    }
    require(uke::frontend::required_confirmation(c::plan("update.apply", {})) ==
                "APPLY",
            "package confirmation missing");
    require(uke::frontend::required_confirmation(
                c::plan("resize.prepare", {})) == "resize.prepare",
            "maintenance confirmation missing");
    try {
      c::plan("flashlight.level", {{"percent", "101"}});
      throw std::runtime_error("invalid numeric request accepted");
    } catch (const std::invalid_argument &) {
    }
    std::filesystem::remove_all(base);
    std::cout << "GTK fixture and review safety checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::filesystem::remove_all(base);
    return 1;
  }
}
