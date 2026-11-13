// SPDX-License-Identifier: MIT
#include "../src/motion-sample.hpp"
#include <cassert>
#include <iostream>
#ifdef NDEBUG
#error Motion acceptance assertions must remain enabled
#endif

using namespace uke::motion;
namespace {
Mount identity() {return *Mount::parse("1,0,0;0,1,0;0,0,1\n");}
void put(std::span<std::byte> data,std::int64_t value) {
    const auto raw=std::bit_cast<std::uint64_t>(value);
    for(std::size_t i=0;i<data.size();++i)data[i]=std::byte((raw>>(i*8))&255);
}
Packet angle(double degrees,std::uint64_t time,double yaw=0) {
    const auto tilt=degrees*std::numbers::pi/180.0,rotation=yaw*std::numbers::pi/180.0;
    return {{9.81*std::sin(tilt)*std::cos(rotation),9.81*std::sin(tilt)*std::sin(rotation),9.81*std::cos(tilt)},time};
}
void frame() {
    assert(!Mount::parse(""));assert(!Mount::parse("1,0,0;0,1,0;0,0,NaN"));
    assert(!Mount::parse("2,0,0;0,1,0;0,0,1"));assert(!Mount::parse("1,0,0;1,0,0;0,0,1"));
    assert(!Mount::parse("1,0,0;0,1,0"));assert(!Mount::parse("1,0,0;0,1,0;0,0,1;0"));
    const auto transform=Mount::parse("0,1,0;-1,0,0;0,0,1");assert(transform);
    assert((transform->apply({1,2,3})==Vector{2,-1,3}));
    Samples missing;assert(!missing.begin({},100));assert(!missing.observe(angle(0,101),101));assert(!missing.current(101).known);
}
void binary() {
    const auto signed_type=ScanType::parse(" be:s12/16>>4\n"),unsigned_type=ScanType::parse("be:u12/16>>4");assert(signed_type&&unsigned_type);
    const std::array negative{std::byte(0xff),std::byte(0xf0)};
    assert(signed_type->read(negative)==-1);assert(unsigned_type->read(negative)==4095);
    assert(!ScanType::parse("le:s0/64>>0"));assert(!ScanType::parse("le:s64/64>>1"));
    assert(!ScanType::parse("le:s32/32X3>>0"));assert(!ScanType::parse("le:f32/32>>0"));
    assert(!ScanType::parse("le:s64/63>>0"));assert(!ScanType::parse("le:s64/64>>999999999999999999999"));
    assert(!ScanType{}.read(negative));
    const auto type=*ScanType::parse("le:s64/64>>0");
    std::vector<Channel> channels;
    for(auto name:{"in_gravity_x","in_gravity_y","in_gravity_z","in_accel_linear_x","in_accel_linear_y","in_accel_linear_z","in_timestamp"})
        channels.push_back({name,unsigned(channels.size()),type});
    const auto layout=Layout::make(channels);assert(layout&&layout->bytes==56);
    std::array<std::byte,56> packet{};put(std::span(packet).subspan(16,8),9810000);put(std::span(packet).subspan(24,8),5000000);
    put(std::span(packet).subspan(48,8),1001000);
    const auto decoded=layout->read(packet,{0.000001,0.000001,0.000001});assert(decoded);
    assert(std::abs(decoded->gravity[2]-9.81)<1e-9&&decoded->gravity[0]==0&&decoded->gravity[1]==0&&decoded->timestamp_usec==1001);
    assert(!layout->read(std::span(packet).first(55),{1,1,1}));assert(!layout->read(packet,{1,1,0}));
    put(std::span(packet).subspan(48,8),-1);assert(!layout->read(packet,{1,1,1}));
    auto bad=channels;bad[1].index=bad[0].index;assert(!Layout::make(bad));
    bad=channels;bad[1].name=bad[0].name;assert(!Layout::make(bad));
    bad=channels;bad.pop_back();assert(!Layout::make(bad));
    bad=channels;bad[3].name="in_temperature";assert(!Layout::make(bad));
    bad=channels;bad.back().type=*ScanType::parse("le:s32/32>>0");assert(!Layout::make(bad));
}
void freshness() {
    Samples samples;assert(samples.begin(identity(),100));
    assert(!samples.observe(angle(0,99),100));assert(!samples.current(100).known);
    assert(samples.observe(angle(0,101),101));assert(samples.current(101).known);
    assert(!samples.observe(angle(1,102),102));assert(samples.current(102).timestamp_usec==102);
    assert(!samples.expire(102+maximum_sample_age_usec-1));
    assert(samples.expire(102+maximum_sample_age_usec));assert(!samples.current(102+maximum_sample_age_usec).known);
    assert(samples.observe(angle(1,2000100),2000100));
    assert(samples.observe(angle(2,2000200),2000199));assert(!samples.current(2000199).known);
    assert(!samples.observe(angle(1,2000100),2000200));assert(!samples.current(2000200).known);
    assert(samples.observe(angle(2,2000101),2000201));assert(samples.current(2000201).known);
    assert(samples.observe({{0,0,0},2000102},2000202));assert(!samples.current(2000202).known);
    assert(samples.observe(angle(0,2000103),2000203));
    assert(samples.observe({{0,0,std::numeric_limits<double>::quiet_NaN()},2000104},2000204));assert(!samples.current(2000204).known);
    assert(samples.begin(identity(),3000000));assert(!samples.observe(angle(0,2000105),3000001));
    assert(samples.observe(angle(0,3000001),3000001));
    assert(samples.observe(angle(0,3000002),3000002+maximum_sample_age_usec));assert(!samples.current(3000002+maximum_sample_age_usec).known);
}
void edges() {
    Samples samples;assert(samples.begin(identity(),100));
    assert(samples.observe(angle(0,101),101));assert(!samples.observe(angle(30,102),102));
    assert(samples.observe(angle(41,103),103));assert(!samples.observe(angle(44,104),104));
    assert(samples.observe(angle(46,105),105));assert(samples.observe(angle(43,106),106));
    assert(samples.observe(angle(39,107),107));assert(samples.observe(angle(180,108),108));
    assert(samples.current(108).known&&std::abs(samples.current(108).inclination-180)<1e-8);
    std::uint64_t time=109;
    for(double degrees:{0.0,15.0,39.0,41.0,44.0,46.0,90.0,135.0,180.0})for(double yaw:{0.0,90.0,180.0,270.0}){
        samples.observe(angle(degrees,time,yaw),time);assert(samples.current(time).known);
        assert(std::abs(samples.current(time).inclination-degrees)<1e-6);++time;
    }
}
}
int main() {frame();binary();freshness();edges();std::cout<<"Motion frame, scalar layout, freshness/generation and edge tests passed\n";}
