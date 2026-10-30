// SPDX-License-Identifier: MIT
// Bounded SSC samples; no factory tests, calibration writes or persistent client.
#define UKE_SSC_TEST
#include "ssc-discovery.cpp"
#include <limits>
#include <ctime>
#include <charconv>
struct Sample { std::uint64_t timestamp, status; std::vector<float> values; };
static std::vector<Sample> samples(View event, Uid sensor, unsigned dimensions) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == sensor, "Foreign sample sensor");
    std::vector<Sample> result;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid sample event");
        auto e = fields(entry.data); auto id = little(one(e, 1, 5));
        require(id != 130, "SSC returned a sample error");
        if (id != 1025) continue;
        auto data = fields(one(e, 3, 2)); auto status = one(data, 2, 0);
        std::size_t p = 0;
        Sample sample{little(one(e, 2, 1)), varint(status, p), {}};
        require(sample.status <= 3 && sample.timestamp != 0, "Invalid sample metadata");
        require(result.size() < 32 && (result.empty() || sample.timestamp > result.back().timestamp), "Too many or unordered samples");
        for (auto &value : data) if (value.number == 1) {
            require((value.wire == 5 || value.wire == 2) && value.data.size() % 4 == 0, "Invalid sample float");
            for (std::size_t offset = 0; offset < value.data.size(); offset += 4) {
                auto number = std::bit_cast<float>(std::uint32_t(little(value.data.subspan(offset, 4))));
                require(std::isfinite(number) && sample.values.size() < (dimensions ? dimensions : 16), "Invalid sample value or dimension");
                sample.values.push_back(number);
            }
        }
        require(!sample.values.empty() && (!dimensions || sample.values.size() == dimensions), "Incomplete sample vector");
        result.push_back(sample);
    }
    require(!result.empty(), "No standard sensor samples");
    return result;
}
[[maybe_unused]] static std::uint64_t receipt_time() {
    timespec t{};
    require(clock_gettime(CLOCK_BOOTTIME, &t) == 0 && t.tv_sec >= 0, "BOOTTIME unavailable");
    return std::uint64_t(t.tv_sec)*1'000'000'000ULL + t.tv_nsec;
}
[[maybe_unused]] static void capture_metadata(View event, Uid selected, std::string_view type) {
    // sns_std_sensor attributes: exact type/API, available flag and streaming mode.
    auto root = fields(event), uid = fields(one(root,1,2));
    require((Uid{little(one(uid,1,1)),little(one(uid,2,1))} == selected), "Foreign capture metadata");
    std::array<bool,4> seen{};
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid capture metadata");
        const auto e = fields(entry.data);
        if (little(one(e,1,5)) != 128) continue;
        for (auto &attr : fields(one(e,3,2))) if (attr.number == 1) {
            require(attr.wire == 2, "Invalid capture attribute");
            auto a = fields(attr.data); std::size_t p = 0;
            const auto id = varint(one(a,1,0),p);
            if (id != 2 && id != 3 && id != 5 && id != 16) continue;
            const unsigned slot = id == 2 ? 0 : id == 3 ? 1 : id == 5 ? 2 : 3;
            require(!seen[slot], "Duplicate capture attribute"); seen[slot] = true;
            const auto value = fields(one(fields(one(a,2,2)),1,2));
            if (id == 2 || id == 5) {
                auto text = one(value,2,2);
                if (!text.empty() && text.back() == 0) text = text.first(text.size()-1);
                const std::string_view actual(reinterpret_cast<const char*>(text.data()),text.size());
                const auto api = type == "sensor_temperature" ? "sns_sensor_temperature.proto" :
                    type == "gyro" ? "sns_gyro.proto" : type == "accel" ? "sns_accel.proto" : "sns_ambient_light.proto";
                require(actual == (id == 2 ? type : std::string_view(api)), "Capture type/API mismatch");
            } else if (id == 3) {
                p=0; require(varint(one(value,5,0),p) == 1, "Capture sensor unavailable");
            } else require(little(one(value,4,1)) == 0, "Capture sensor is not streaming");
        }
    }
    require(std::all_of(seen.begin(),seen.end(),[](bool b){return b;}), "Incomplete capture metadata");
}
struct Capture {
    std::uint64_t first_ticks=0, last_ticks=0, first_ns=0, last_ns=0, min_dt=UINT64_MAX, max_dt=0;
    unsigned count=0, unreliable=0;
    std::vector<double> mean, m2, integral, previous;
    void accept(const Sample &s, std::uint64_t now) {
        require(count < 4096 && s.timestamp > last_ticks && (!count || now >= last_ns), "Unordered capture or sample limit");
        if (!count) {
            first_ticks=s.timestamp; first_ns=now;
            mean.resize(s.values.size()); m2.resize(s.values.size()); integral.resize(s.values.size());
        }
        require(s.values.size() == mean.size(), "Capture dimension changed");
        if (count) { min_dt=std::min(min_dt,s.timestamp-last_ticks); max_dt=std::max(max_dt,s.timestamp-last_ticks); }
        ++count; unreliable += s.status == 0;
        for (unsigned i=0;i<s.values.size();++i) {
            const double delta=s.values[i]-mean[i]; mean[i]+=delta/count;
            m2[i]+=delta*(s.values[i]-mean[i]);
            if (!previous.empty()) integral[i]+=(previous[i]+s.values[i])*0.5*double(s.timestamp-last_ticks);
        }
        previous.assign(s.values.begin(),s.values.end()); last_ticks=s.timestamp; last_ns=now;
    }
    void summary(std::string_view type, const char *units) const {
        require(count > 1 && last_ns > first_ns, "Insufficient capture interval");
        const double seconds=double(last_ns-first_ns)/1e9, ticks=double(last_ticks-first_ticks);
        std::cout << "{\"schema\":\"org.uke.sensors.capture.v1\",\"kind\":\"summary\",\"data_type\":\"" << type
            << "\",\"units\":\"" << units << "\",\"samples\":" << count << ",\"unreliable_samples\":" << unreliable
            << ",\"receipt_duration_s\":" << seconds << ",\"observed_rate_hz\":" << (count-1)/seconds
            << ",\"dsp_ticks_per_receipt_second_estimate\":" << ticks/seconds
            << ",\"min_period_ticks\":" << min_dt << ",\"max_period_ticks\":" << max_dt
            << ",\"dsp_clock_mapping\":\"unverified\",\"stationary_qualified\":false,\"calibration_applied\":false,\"mean\":[";
        for (unsigned i=0;i<mean.size();++i) { if(i)std::cout<<',';std::cout<<mean[i]; }
        std::cout << "],\"standard_deviation\":[";
        for (unsigned i=0;i<mean.size();++i) { if(i)std::cout<<',';std::cout<<std::sqrt(std::max(0.0,m2[i]/(count-1))); }
        std::cout << "],\"gyro_integrated_radians_receipt_clock_estimate\":";
        if (type != "gyro" || unreliable) std::cout << "null";
        else {
            std::cout<<'[';
            for (unsigned i=0;i<mean.size();++i) { if(i)std::cout<<',';std::cout<<integral[i]*seconds/ticks; }
            std::cout<<']';
        }
        std::cout << "}\n";
    }
};
[[maybe_unused]] static unsigned duration(std::string_view text) {
    unsigned value=0;
    const auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    require(result.ec == std::errc{} && result.ptr == text.data()+text.size() && value>=1 && value<=60,
            "Capture seconds must be 1..60");
    return value;
}
#ifndef UKE_SSC_SAMPLE_TEST
int main(int argc, char **argv) {
    (void)&attributes;
    int fd = -1;
    try {
        require(argc >= 2 && argc <= 4, "Usage: ssc-sample DATATYPE [SECONDS] [--json]");
        std::string_view type = argv[1];
        unsigned seconds=0; bool json=false;
        for (int i=2;i<argc;++i) {
            if (std::string_view(argv[i]) == "--json") { require(!json,"Duplicate JSON option"); json=true; }
            else { require(!seconds,"Duplicate duration"); seconds=duration(argv[i]); }
        }
        const bool xyz = type == "accel" || type == "gyro" || type == "mag";
        const bool cct = type == "cct_front";
        const bool flicker = type == "flicker";
        const bool temperature=type == "sensor_temperature", color_stream=type == "cct_front_strm";
        require(xyz || cct || flicker || temperature || color_stream || type == "ambient_light" || type == "ambient_light_back" || type == "ambient_light_back_strm", "Unsupported sample type");
        require(!seconds || type == "accel" || type == "gyro" || temperature || color_stream, "Bounded capture supports accel, gyro, sensor_temperature and cct_front_strm");
        if (flicker) std::cerr << "Flicker: keep the optical path open for a controlled-light test; raw units are unverified\n";
        if (cct) std::cerr << "CCT phase=metadata\n";
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR socket unavailable");
        auto found = suids(rpc(fd, request(type), 0, false, cct ? &std::cerr : nullptr), type); close(fd); fd = -1;
        require(found.size() == 1, "Exactly one sensor required for bounded sampling");
        if (seconds || temperature || color_stream) {
            fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
            require(fd >= 0,"QRTR attribute socket unavailable");
            capture_metadata(rpc(fd,request("",found[0],1),128),found[0],type);
            close(fd); fd=-1;
        }
        if (cct) std::cerr << "CCT phase=sample request_id=514 expected_event_id=1025\n";
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR sample socket unavailable");
        // The OEM rear stream advertises a minimum of 2 Hz; use its valid rate.
        const float hz=seconds && xyz ? 26.0f : color_stream ? 5.0f : type == "ambient_light_back_strm" ? 2.0f : 1.0f;
        Bytes rate{0x0d};put(rate,std::bit_cast<std::uint32_t>(hz),4);
        const bool streaming = xyz || temperature || color_stream || type == "ambient_light_back_strm";
        const char *units=temperature ? "degree_Celsius" : type == "gyro" ? "rad/s" : type == "accel" ? "m/s^2" : "unverified";
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        const auto emit=[&](const Sample &s, std::uint64_t received) {
            if (json) {
                std::cout << "{\"schema\":\"org.uke.sensors.capture.v1\",\"kind\":\"sample\",\"data_type\":\""<<type
                    <<"\",\"units\":\""<<units<<"\",\"timestamp_ticks\":"<<s.timestamp<<",\"status\":"<<s.status
                    <<",\"received_boottime_ns\":"<<received<<",\"dsp_clock_mapping\":\"unverified\",\"values\":[";
                for (unsigned i=0;i<s.values.size();++i){if(i)std::cout<<',';std::cout<<s.values[i];}
                std::cout<<"]}\n"<<std::flush;
            } else {
                std::cout<<type<<" timestamp_ticks="<<s.timestamp<<" status="<<s.status<<" units="<<units<<" data=";
                for(float value:s.values)std::cout<<value<<' ';
                std::cout<<'\n';
            }
        };
        if (seconds) {
            Capture capture;
            rpc(fd,client_request(found[0],513,rate),1025,false,nullptr,[&](View event) {
                const auto received=receipt_time();
                for (const auto &s:samples(event,found[0],xyz?3:temperature?1:0)) { capture.accept(s,received); emit(s,received); }
                return capture.last_ns-capture.first_ns < std::uint64_t(seconds)*1'000'000'000ULL;
            },seconds);
            close(fd);fd=-1;
            capture.summary(type,units);return 0;
        }
        auto event = rpc(fd, client_request(found[0], streaming ? 513 : 514, streaming ? View(rate) : View{}, xyz), 1025, streaming, cct ? &std::cerr : nullptr);
        close(fd); fd = -1; // QRTR disconnect removes this client's sensor requests.
        // shortcut: OEM rear-light channels remain raw until vendor channel units are verified.
        std::cout << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto &sample : samples(event, found[0], xyz ? 3 : temperature ? 1 : 0)) {
            if (json || temperature || color_stream) { emit(sample,receipt_time());continue; }
            std::cout << type << " timestamp_ticks=" << sample.timestamp << " status=" << sample.status;
            if (cct || flicker) std::cout << " units=unverified";
            std::cout << " data=";
            for (float number : sample.values) std::cout << number << ' ';
            std::cout << '\n';
            if (cct || flicker) break;
        }
        return 0;
    } catch (const std::exception &e) {
        if (fd >= 0) close(fd);
        std::cerr << e.what() << '\n'; return 1;
    }
}
#endif
