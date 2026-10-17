// SPDX-License-Identifier: MIT
// One-shot metadata only; protocol: Qualcomm sensinghub 329551963bcc063f6a89f6cbca259c5b5c9c4e0b.
#include <algorithm>
#include <array>
#include <chrono>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <linux/qrtr.h>
#include <poll.h>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using Bytes = std::vector<std::uint8_t>;
using View = std::span<const std::uint8_t>;
static constexpr std::size_t limit = 16384;
static void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
static std::uint64_t little(View bytes) {
    require(bytes.size() <= 8, "Invalid integer width");
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) value |= std::uint64_t(bytes[i]) << (8 * i);
    return value;
}
static void put(Bytes &out, std::uint64_t value, unsigned width) {
    for (unsigned i = 0; i < width; ++i) out.push_back(value >> (8 * i));
}
static void length_field(Bytes &out, unsigned field, View data) {
    require(field < 16 && data.size() < 128, "Request field exceeds fixed discovery bounds");
    out.push_back((field << 3) | 2);
    out.push_back(data.size());
    out.insert(out.end(), data.begin(), data.end());
}
static std::uint64_t varint(View data, std::size_t &offset) {
    std::uint64_t result = 0;
    for (unsigned i = 0; i < 10; ++i) {
        require(offset < data.size(), "Truncated protobuf integer");
        auto byte = data[offset++];
        require(i != 9 || byte <= 1, "Overflowing protobuf integer");
        result |= std::uint64_t(byte & 127) << (7 * i);
        if (!(byte & 128)) return result;
    }
    throw std::runtime_error("Overflowing protobuf integer");
}
struct Field { unsigned number, wire; View data; };
static std::vector<Field> fields(View data) {
    require(data.size() <= limit, "Oversized protobuf message");
    std::vector<Field> result;
    std::size_t offset = 0;
    while (offset < data.size()) {
        auto key = varint(data, offset);
        require(key >= 8 && key <= 0xffffffff, "Invalid protobuf field tag");
        unsigned wire = key & 7;
        std::size_t start = offset, size;
        if (wire == 0) { varint(data, offset); size = offset - start; }
        else {
            require(wire == 1 || wire == 2 || wire == 5, "Unsupported protobuf wire type");
            size = wire == 2 ? varint(data, offset) : wire == 1 ? 8 : 4;
            start = offset;
            require(size <= data.size() - offset, "Truncated protobuf field");
            offset += size;
        }
        result.push_back({unsigned(key >> 3), wire, data.subspan(start, size)});
    }
    return result;
}
static View one(const std::vector<Field> &items, unsigned number, unsigned wire) {
    View result;
    bool found = false;
    for (auto &f : items) if (f.number == number) {
        require(!found && f.wire == wire, "Duplicate or wrongly typed protobuf field");
        found = true; result = f.data;
    }
    require(found, "Missing protobuf field");
    return result;
}
static bool datatype(std::string_view type) {
    static constexpr std::array allowed{"", "accel", "gyro", "mag", "ambient_light", "ambient_light_back", "ambient_light_back_strm", "proximity", "sar", "hall", "oem_hall", "flicker", "sensor_temperature", "rotv", "geomag_rv", "game_rv", "gravity", "linear_accel", "tilt", "motion_detect", "pedometer"};
    return std::find(allowed.begin(), allowed.end(), type) != allowed.end();
}
struct Uid {
    std::uint64_t low, high;
    bool operator==(const Uid &) const = default;
};
static constexpr Uid lookup_uid{0xababababababababULL, 0xababababababababULL};
static Bytes client_request(Uid sensor, unsigned message, View payload, bool batch_motion = false) {
    require(!batch_motion || message == 513, "Batching is restricted to sensor streams");
    Bytes standard;
    if (batch_motion) {
        const Bytes batch{8, 0xa0, 0x86, 6}; // 100,000 us; preserve the 64-packet bound while settling.
        length_field(standard, 1, batch);
    }
    length_field(standard, 2, payload);
    Bytes uid{0x09}; put(uid, sensor.low, 8);
    uid.push_back(0x11); put(uid, sensor.high, 8);
    Bytes client; length_field(client, 1, uid);
    client.push_back(0x15); put(client, message, 4);
    const Bytes suspend{0x08, 1, 0x10, 1}; // APSS, non-wakeup delivery.
    length_field(client, 3, suspend);
    length_field(client, 4, standard);
    Bytes body{1}; put(body, client.size() + 2, 2); put(body, client.size(), 2);
    body.insert(body.end(), client.begin(), client.end());
    Bytes packet{0}; put(packet, 1, 2); put(packet, 0x20, 2); put(packet, body.size(), 2);
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}
static Bytes request(std::string_view type, Uid sensor = lookup_uid, unsigned message = 512) {
    require((message == 512 && datatype(type) && sensor == lookup_uid) ||
            (message == 1 && type.empty()), "Unsupported metadata request");
    Bytes lookup;
    length_field(lookup, 1, View(reinterpret_cast<const std::uint8_t *>(type.data()), type.size()));
    lookup.insert(lookup.end(), {0x10, 0, 0x18, 0}); // No updates, all matching SUIDs.
    return client_request(sensor, message, message == 512 ? View(lookup) : View{});
}
struct Tlv { unsigned type; View data; };
static std::vector<Tlv> tlvs(View packet) {
    require(packet.size() >= 7 && packet.size() <= limit, "Invalid QMI packet size");
    require(little(packet.subspan(5, 2)) == packet.size() - 7, "QMI length mismatch");
    std::vector<Tlv> result;
    for (std::size_t p = 7; p < packet.size();) {
        require(packet.size() - p >= 3, "Truncated QMI TLV header");
        auto type = packet[p]; auto size = little(packet.subspan(p + 1, 2)); p += 3;
        require(size <= packet.size() - p, "Truncated QMI TLV value");
        result.push_back({type, packet.subspan(p, size)}); p += size;
    }
    return result;
}
static View tlv(const std::vector<Tlv> &items, unsigned type) {
    View result;
    bool found = false;
    for (auto &item : items) if (item.type == type) {
        require(!found, "Duplicate QMI TLV"); found = true; result = item.data;
    }
    require(found, "Missing QMI TLV"); return result;
}
static std::vector<Uid> suids(View event, std::string_view type) {
    auto root = fields(event);
    auto uid = fields(one(root, 1, 2));
    require(little(one(uid, 1, 1)) == 0xababababababababULL &&
            little(one(uid, 2, 1)) == 0xababababababababULL, "Foreign event sensor");
    std::vector<Uid> result;
    bool received = false;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid client event");
        auto e = fields(entry.data);
        auto id = little(one(e, 1, 5));
        one(e, 2, 1); // Require a complete event timestamp.
        auto payload = one(e, 3, 2);
        if (id == 130) throw std::runtime_error("SSC returned a sensor error event");
        if (id != 768) continue;
        auto lookup = fields(payload); auto name = one(lookup, 1, 2);
        require(std::string_view(reinterpret_cast<const char *>(name.data()), name.size()) == type, "Foreign lookup data type");
        received = true;
        for (auto &f : lookup) if (f.number == 2) {
            require(f.wire == 2, "Invalid discovered SUID");
            auto s = fields(f.data);
            Uid found{little(one(s, 1, 1)), little(one(s, 2, 1))};
            require(result.size() < 32 && std::find(result.begin(), result.end(), found) == result.end(), "Too many or duplicate discovered SUIDs");
            result.push_back(found);
        }
    }
    require(received, "No matching SUID discovery event");
    return result;
}
static std::string attributes(View event, Uid sensor, bool full = false) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == sensor, "Foreign attribute sensor");
    std::array<std::string,4> values{"not returned", "not returned", "not returned", "not returned"};
    std::array<bool,4> seen{};
    bool received = false;
    std::ostringstream raw;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid attribute event");
        auto e = fields(entry.data); auto id = little(one(e, 1, 5));
        one(e, 2, 1);
        auto payload = one(e, 3, 2);
        if (id == 130) throw std::runtime_error("SSC returned an attribute error");
        if (id != 128) continue;
        received = true;
        for (auto &attr : fields(payload)) if (attr.number == 1) {
            require(attr.wire == 2, "Invalid sensor attribute");
            auto a = fields(attr.data); auto raw_id = one(a, 1, 0);
            std::size_t p = 0; auto which = varint(raw_id, p);
            if (full) {
                raw << "\nattribute_id=" << std::dec << which << " protobuf_hex=";
                for (auto byte : one(a, 2, 2)) raw << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
            }
            if (which != 0 && which != 2 && which != 3 && which != 21) continue;
            unsigned slot = which == 0 ? 0 : which == 2 ? 1 : which == 3 ? 2 : 3;
            require(!seen[slot], "Duplicate metadata attribute"); seen[slot] = true;
            auto v = fields(one(a, 2, 2)); auto d = fields(one(v, 1, 2));
            if (slot < 2) {
                auto text = one(d, 2, 2);
                if (!text.empty() && text.back() == 0) text = text.first(text.size()-1);
                require(text.size() <= 128 && std::all_of(text.begin(), text.end(), [](auto c){ return c >= 32 && c < 127; }), "Invalid metadata string");
                values[slot] = std::string(reinterpret_cast<const char *>(text.data()), text.size());
            } else {
                auto boolean = one(d, 5, 0); p = 0; auto b = varint(boolean, p);
                require(b <= 1, "Invalid metadata boolean"); values[slot] = b ? "true" : "false";
            }
        }
    }
    require(received, "No matching attribute event");
    return "name=" + values[0] + "; type=" + values[1] + "; available=" + values[2] + "; physical=" + values[3] + raw.str();
}
static Bytes receive(int fd, sockaddr_qrtr &peer, std::chrono::steady_clock::time_point deadline) {
    for (;;) {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        require(remaining > 0, "SSC discovery timed out");
        pollfd watch{fd, POLLIN, 0};
        int ready = poll(&watch, 1, remaining);
        if (ready < 0 && errno == EINTR) continue;
        require(ready > 0 && !(watch.revents & (POLLERR | POLLHUP | POLLNVAL)), "SSC transport unavailable or timed out");
        Bytes bytes(limit);
        iovec iov{bytes.data(), bytes.size()};
        msghdr message{}; message.msg_name = &peer; message.msg_namelen = sizeof(peer);
        message.msg_iov = &iov; message.msg_iovlen = 1;
        auto length = recvmsg(fd, &message, MSG_DONTWAIT);
        if (length < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        require(length > 0 && !(message.msg_flags & MSG_TRUNC) && message.msg_namelen == sizeof(peer), "Invalid QRTR datagram");
        bytes.resize(length); return bytes;
    }
}
static void send(int fd, const sockaddr_qrtr &peer, View bytes) {
    require(sendto(fd, bytes.data(), bytes.size(), 0, reinterpret_cast<const sockaddr *>(&peer), sizeof(peer)) == ssize_t(bytes.size()), "QRTR send failed");
}
static Bytes rpc(int fd, const Bytes &packet, unsigned wanted_event = 0, bool settle_motion = false) {
    require(!settle_motion || wanted_event == 1025, "Settling is restricted to sensor data");
    sockaddr_qrtr local{};
    socklen_t size = sizeof(local);
    require(getsockname(fd, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR local address unavailable");
    local.sq_port = 0;
    require(bind(fd, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == 0, "QRTR bind failed");
    require(getsockname(fd, reinterpret_cast<sockaddr *>(&local), &size) == 0 && size == sizeof(local), "QRTR bound address unavailable");
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    sockaddr_qrtr control{AF_QIPCRTR, local.sq_node, QRTR_PORT_CTRL}, server{}, peer{};
    Bytes lookup; put(lookup, QRTR_TYPE_NEW_LOOKUP, 4); put(lookup, 400, 4);
    put(lookup, 1, 4); put(lookup, 0, 4); put(lookup, 0, 4);
    send(fd, control, lookup);
    bool found = false;
    for (unsigned i = 0; i < 64; ++i) {
        auto data = receive(fd, peer, deadline);
        if (peer.sq_port != QRTR_PORT_CTRL || peer.sq_node != local.sq_node) continue;
        require(data.size() == sizeof(qrtr_ctrl_pkt), "Invalid QRTR lookup reply");
        if (little(View(data).first(4)) != QRTR_TYPE_NEW_SERVER) continue;
        if (little(View(data).subspan(4, 4)) != 400 || little(View(data).subspan(8, 4)) != 1) continue;
        server = {AF_QIPCRTR, std::uint32_t(little(View(data).subspan(12, 4))), std::uint32_t(little(View(data).subspan(16, 4)))};
        require(server.sq_node && server.sq_port && server.sq_port != QRTR_PORT_CTRL, "SSC service not advertised");
        found = true; break;
    }
    require(found, "SSC service lookup exceeded packet limit");
    send(fd, server, packet);
    const auto settled = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool accepted = false; std::uint64_t client = 0;
    for (unsigned i = 0; i < 64; ++i) {
        auto data = receive(fd, peer, deadline);
        if (peer.sq_node != server.sq_node || peer.sq_port != server.sq_port) continue;
        auto items = tlvs(data); auto message = little(View(data).subspan(3, 2));
        if (data[0] == 2 && message == 0x20) {
            require(little(View(data).subspan(1, 2)) == 1 && !accepted, "Wrong QMI response transaction");
            auto result = tlv(items, 2);
            require(result.size() == 4 && little(result.first(2)) == 0 && little(result.last(2)) == 0, "SSC rejected discovery");
            auto id = tlv(items, 0x10); require(id.size() == 8, "Invalid SSC client identity");
            client = little(id);
            for (auto &t : items) if (t.type == 0x11) require(t.data.size() == 4 && little(t.data) == 0, "SSC protobuf request rejected");
            accepted = true;
        } else if (data[0] == 4 && message == 0x21) {
            require(accepted, "SSC event precedes acknowledged request");
            auto id = tlv(items, 1); require(id.size() == 8 && little(id) == client, "Foreign SSC client event");
            auto payload = tlv(items, 2);
            require(payload.size() >= 2 && little(payload.first(2)) == payload.size() - 2, "SSC payload length mismatch");
            auto event = payload.subspan(2);
            if (wanted_event) {
                bool matching = false;
                for (auto &entry : fields(event)) if (entry.number == 2) {
                    require(entry.wire == 2, "Invalid client event");
                    auto id = little(one(fields(entry.data), 1, 5));
                    require(id != 130, "SSC returned a sensor error event");
                    matching |= id == wanted_event;
                }
                if (!matching) continue;
                if (settle_motion && std::chrono::steady_clock::now() < settled) continue;
            }
            return Bytes(event.begin(), event.end());
        } else throw std::runtime_error("Unexpected SSC discovery message");
    }
    throw std::runtime_error("SSC discovery exceeded packet limit");
}
#ifndef UKE_SSC_TEST
int main(int argc, char **argv) {
    int fd = -1;
    try {
        require(argc == 2 || (argc == 3 && std::string_view(argv[2]) == "--attributes"), "Usage: uke-sensor-discovery all|DATATYPE [--attributes]");
        const bool full = argc == 3;
        std::string_view type = std::string_view(argv[1]) == "all" ? "" : argv[1];
        require(datatype(type), "Unsupported discovery data type");
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR socket unavailable");
        auto found = suids(rpc(fd, request(type)), type); close(fd); fd = -1;
        std::cout << argv[1] << ": " << found.size() << " SUID(s) in first discovery event; no samples requested\n";
        if (type.empty() || full) for (auto sensor : found) {
            fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
            require(fd >= 0, "QRTR metadata socket unavailable");
            std::cout << attributes(rpc(fd, request("", sensor, 1)), sensor, full) << '\n';
            close(fd); fd = -1;
        }
        return 0;
    } catch (const std::exception &e) {
        if (fd >= 0) close(fd);
        std::cerr << e.what() << '\n'; return 1;
    }
}
#endif
