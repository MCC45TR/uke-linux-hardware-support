// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace uke::motion {
inline constexpr auto bus_name="org.senemos.Uke.Motion1";
inline constexpr auto object_path="/org/senemos/Uke/Motion1";
inline constexpr auto interface="org.senemos.Uke.Motion1";
inline constexpr std::uint64_t maximum_sample_age_usec=1500000;
using Vector=std::array<double,3>;

inline std::string_view trim(std::string_view text) {
    const auto first=text.find_first_not_of(" \t\r\n");
    if(first==text.npos)return {};
    return text.substr(first,text.find_last_not_of(" \t\r\n")-first+1);
}
template<class T> std::optional<T> number(std::string_view text) {
    text=trim(text);if(text.empty())return {};T value{};
    const auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    if(result.ec!=std::errc()||result.ptr!=text.data()+text.size())return {};
    return value;
}

// Only a kernel-admitted gravity mount may construct this transform. A rigid
// body identifier, another sensor's matrix or an orientation string is not one.
struct Mount {
    std::array<Vector,3> rows{};
    static std::optional<Mount> parse(std::string_view text) {
        if(text.size()>512)return {};
        Mount mount;
        for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){
            const char separator=column<2?',':';';
            const auto end=(row==2&&column==2)?text.size():text.find(separator);
            if(end==text.npos)return {};
            const auto value=number<double>(text.substr(0,end));
            if(!value||!std::isfinite(*value))return {};
            mount.rows[row][column]=*value;
            text.remove_prefix(end);
            if(!text.empty())text.remove_prefix(1);
        }
        if(!trim(text).empty())return {};
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
            double dot=0;for(unsigned axis=0;axis<3;++axis)dot+=mount.rows[i][axis]*mount.rows[j][axis];
            if(std::abs(dot-(i==j?1.0:0.0))>0.0001)return {};
        }
        return mount;
    }
    Vector apply(const Vector& vector)const {
        Vector result{};
        for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)result[row]+=rows[row][column]*vector[column];
        return result;
    }
};

struct ScanType {
    unsigned bits=0,storage=0,shift=0;
    bool big_endian=false,is_signed=false;
    bool valid()const {return bits&&bits<=64&&shift<64&&bits+shift<=storage&&(storage==8||storage==16||storage==32||storage==64);}
    static std::optional<ScanType> parse(std::string_view text) {
        text=trim(text);if(text.size()>48||text.size()<7)return {};
        ScanType type;
        if(text.starts_with("be:"))type.big_endian=true;
        else if(!text.starts_with("le:"))return {};
        text.remove_prefix(3);
        if(text.front()=='s')type.is_signed=true;
        else if(text.front()!='u')return {};
        text.remove_prefix(1);
        const auto slash=text.find('/');if(slash==text.npos)return {};
        const auto bits=number<unsigned>(text.substr(0,slash));text.remove_prefix(slash+1);
        const auto separator=text.find(">>");
        const auto storage=number<unsigned>(text.substr(0,separator));
        const auto shift=separator==text.npos?std::optional<unsigned>(0):number<unsigned>(text.substr(separator+2));
        if(!bits||!storage||!shift||!*bits||*bits>64||*shift>63||*bits+*shift>*storage||
           (*storage!=8&&*storage!=16&&*storage!=32&&*storage!=64))return {};
        type.bits=*bits;type.storage=*storage;type.shift=*shift;return type;
    }
    std::optional<std::int64_t> read(std::span<const std::byte> data)const {
        if(!valid())return {};
        const auto bytes=storage/8;if(data.size()!=bytes)return {};
        std::uint64_t value=0;
        for(unsigned i=0;i<bytes;++i){
            const unsigned position=big_endian?bytes-1-i:i;
            value|=std::uint64_t(std::to_integer<unsigned char>(data[i]))<<(position*8);
        }
        value>>=shift;
        const auto mask=bits==64?std::numeric_limits<std::uint64_t>::max():(std::uint64_t(1)<<bits)-1;
        value&=mask;
        if(is_signed&&bits<64&&(value&(std::uint64_t(1)<<(bits-1))))value|=~mask;
        if(!is_signed&&value>std::uint64_t(std::numeric_limits<std::int64_t>::max()))return {};
        return std::bit_cast<std::int64_t>(value);
    }
};

struct Channel {std::string name;unsigned index;ScanType type;std::size_t offset=0;};
struct Packet {Vector gravity;std::uint64_t timestamp_usec;};
struct Layout {
    std::vector<Channel> channels;
    std::size_t bytes=0;
    static std::optional<Layout> make(std::vector<Channel> channels) {
        if(channels.size()<4||channels.size()>7)return {};
        std::sort(channels.begin(),channels.end(),[](const auto& a,const auto& b){return a.index<b.index;});
        const std::array<std::string_view,7> names{"in_gravity_x","in_gravity_y","in_gravity_z","in_accel_linear_x","in_accel_linear_y","in_accel_linear_z","in_timestamp"};
        std::size_t size=0,alignment=1;unsigned seen=0;
        for(std::size_t i=0;i<channels.size();++i){
            auto& channel=channels[i];
            const auto found=std::find(names.begin(),names.end(),channel.name);
            if(found==names.end()||channel.index>31||(i&&channels[i-1].index==channel.index))return {};
            const auto bit=1U<<unsigned(found-names.begin());if(seen&bit)return {};seen|=bit;
            const auto width=channel.type.storage/8;
            if(!channel.type.valid())return {};
            if(channel.name=="in_timestamp"&&(channel.type.bits!=64||channel.type.storage!=64||channel.type.shift))return {};
            size=(size+width-1)/width*width;channel.offset=size;size+=width;alignment=std::max(alignment,std::size_t(width));
        }
        if((seen&0x47)!=0x47)return {};
        return Layout{std::move(channels),(size+alignment-1)/alignment*alignment};
    }
    std::optional<Packet> read(std::span<const std::byte> data,const Vector& scale)const {
        if(data.size()!=bytes)return {};
        Packet packet{};
        for(const auto& channel:channels){
            const auto value=channel.type.read(data.subspan(channel.offset,channel.type.storage/8));if(!value)return {};
            if(channel.name=="in_timestamp"){
                if(*value<=0)return {};
                packet.timestamp_usec=std::uint64_t(*value)/1000;
            }else for(unsigned axis=0;axis<3;++axis)if(channel.name==std::array{"in_gravity_x","in_gravity_y","in_gravity_z"}[axis]){
                if(!std::isfinite(scale[axis])||scale[axis]<=0)return {};
                packet.gravity[axis]=double(*value)*scale[axis];
            }
        }
        return packet;
    }
};

struct Snapshot {bool known=false;double inclination=0;std::uint64_t timestamp_usec=0;};
class Samples {
public:
    bool begin(std::optional<Mount> mount,std::uint64_t epoch_usec) {
        m_mount=mount;m_epoch=epoch_usec;m_last_timestamp=0;m_sample={};return mount.has_value()&&epoch_usec;
    }
    Snapshot current(std::uint64_t now)const {
        if(!m_sample.known||now<m_sample.timestamp_usec||now-m_sample.timestamp_usec>=maximum_sample_age_usec)return {};
        return m_sample;
    }
    bool observe(const Packet& packet,std::uint64_t now) {
        const auto previous=current(now);
        if(!m_mount||!m_epoch||packet.timestamp_usec<m_epoch||packet.timestamp_usec>now||
           now-packet.timestamp_usec>=maximum_sample_age_usec)return unknown();
        if(packet.timestamp_usec<=m_last_timestamp)return false;
        m_last_timestamp=packet.timestamp_usec;
        const auto vector=m_mount->apply(packet.gravity);
        const double length=std::hypot(vector[0],vector[1],vector[2]);
        if(!std::isfinite(length)||length<7||length>13)return unknown();
        const double angle=std::acos(std::clamp(vector[2]/length,-1.0,1.0))*180.0/std::numbers::pi;
        if(!std::isfinite(angle))return unknown();
        m_sample={true,angle,packet.timestamp_usec};
        return !previous.known||band(previous.inclination)!=band(angle);
    }
    bool expire(std::uint64_t now) {return m_sample.known&&!current(now).known?unknown():false;}
    bool unknown() {const bool changed=m_sample.known;m_sample={};return changed;}
private:
    static int band(double angle) {return angle<40?0:angle<=45?1:2;}
    std::optional<Mount> m_mount;
    std::uint64_t m_epoch=0,m_last_timestamp=0;
    Snapshot m_sample;
};
}
