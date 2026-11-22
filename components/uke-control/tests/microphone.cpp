// SPDX-License-Identifier: MIT
#include "../src/microphone.cpp"
#include <cassert>
#include <iostream>
namespace uke {
Result execute(const std::vector<std::string>&, unsigned, size_t) { throw std::runtime_error("Unexpected process execution in fixture"); }
std::string read(const std::string&, size_t) { throw std::runtime_error("Unexpected file read in fixture"); }
namespace cli { void print(const std::string&, bool, std::string_view) {} }
}
int main() {
    using namespace uke::microphone;
    for (auto id : ids) assert(valid(id));
    assert(valid("auto") && !valid("uke-amic2") && !valid("../../device"));
    Json::Value props;
    props["media.class"] = "Audio/Source"; props["device.api"] = "alsa";
    props["audio.channels"] = 4;
    props["alsa.card_name"] = "POCO-Pad-X1";
    props["alsa.components"] = "mic:uke-4ch-v4 kernel:7.3.1-106";
    props["audio.position"] = "[\"AUX0\",\"AUX1\",\"AUX2\",\"AUX3\"]";
    assert(array(props));
    props["alsa.components"] = "mic:uke-4ch-v3"; assert(!array(props));
    props["alsa.components"] = "mic:uke-4ch-v4 kernel:7.3.1-106";
    props["audio.position"] = "[\"AUX1\",\"AUX0\",\"AUX2\",\"AUX3\"]"; assert(!array(props));
    props["device.api"] = "bluez5"; assert(!array(props));
    for (const auto &text : {"{", "{\"x\":1,\"x\":2}", "{} trailing"}) {
        bool refused = false; try { parse(text); } catch (const std::exception&) { refused = true; }
        assert(refused);
    }
    assert(parse(encode("uke-amic1")).asString() == "uke-amic1");
    char processing[] = "processing", aec[] = "aec", on[] = "on";
    char *args[] = {processing, aec, on};
    bool refused = false; try { command(3, args, true); } catch (const std::runtime_error&) { refused = true; }
    assert(refused);
    std::cout << "Microphone JSON boundaries, identity, channel order and unsupported-effect refusal passed\n";
}
