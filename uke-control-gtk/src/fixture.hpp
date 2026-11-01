// SPDX-License-Identifier: MIT
#pragma once
#include "client-access.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <json/json.h>
#include <map>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace uke::gtkui {
struct Sample {
  client::Result result;
  unsigned delay_ms = 0;
};
class Fixture {
  std::map<std::string, Sample> samples_;

public:
  explicit Fixture(const std::string &path) {
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0)
      throw std::runtime_error("Cannot open fixture: " +
                               std::string(strerror(errno)));
    struct stat info{};
    if (fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 ||
        info.st_size > 2 * 1024 * 1024) {
      close(fd);
      throw std::runtime_error(
          "Fixture must be a regular file of at most 2 MiB");
    }
    std::string bytes;
    char buffer[8192];
    ssize_t count;
    while ((count = read(fd, buffer, sizeof buffer)) > 0) {
      bytes.append(buffer, count);
      if (bytes.size() > 2 * 1024 * 1024) {
        close(fd);
        throw std::runtime_error("Fixture grew beyond size limit");
      }
    }
    int saved = errno;
    close(fd);
    if (count < 0)
      throw std::runtime_error("Cannot read fixture: " +
                               std::string(strerror(saved)));
    Json::CharReaderBuilder builder;
    builder["rejectDupKeys"] = true;
    builder["failIfExtra"] = true;
    Json::Value root;
    std::string error;
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    if (!reader->parse(bytes.data(), bytes.data() + bytes.size(), &root,
                       &error) ||
        !root.isObject() ||
        root["schema"] != "org.uke.control.tui-fixture.v1" ||
        !root["results"].isObject())
      throw std::runtime_error("Invalid fixture schema or JSON");
    for (const auto &id : root["results"].getMemberNames()) {
      client::action(id);
      const auto &source = root["results"][id];
      if (!source.isObject() ||
          (source.isMember("stdout") && !source["stdout"].isString()) ||
          (source.isMember("stderr") && !source["stderr"].isString()) ||
          (source.isMember("status") &&
           (!source["status"].isUInt() || source["status"].asUInt() > 255)) ||
          (source.isMember("delay_ms") &&
           (!source["delay_ms"].isUInt() ||
            source["delay_ms"].asUInt() > 30000)))
        throw std::runtime_error("Invalid fixture result: " + id);
      Sample sample;
      sample.result.action_id = id;
      if (source.isMember("stdout"))
        sample.result.output = source["stdout"].asString();
      else if (source.isMember("data")) {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        sample.result.output = Json::writeString(writer, source["data"]);
      }
      sample.result.error = source.get("stderr", "").asString();
      sample.result.exit_status = source.get("status", 0).asInt();
      sample.delay_ms = source.get("delay_ms", 0).asUInt();
      if (sample.result.output.size() > 1024 * 1024 ||
          sample.result.error.size() > 256 * 1024)
        throw std::runtime_error("Fixture output exceeds capture limits");
      samples_.emplace(id, std::move(sample));
    }
  }
  Sample sample(const std::string &id) const {
    if (auto found = samples_.find(id); found != samples_.end())
      return found->second;
    Sample missing;
    missing.result.action_id = id;
    missing.result.exit_status = 69;
    missing.result.error = "No simulated observation is supplied for this "
                           "action. No command was executed.";
    return missing;
  }
};
} // namespace uke::gtkui
