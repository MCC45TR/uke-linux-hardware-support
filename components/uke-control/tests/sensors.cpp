// SPDX-License-Identifier: MIT
#include "../src/sensors.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <systemd/sd-json.h>
#include <unistd.h>
namespace fs=std::filesystem;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
int main() {
    char path[]="/tmp/uke-sensor-view-XXXXXX";assert(mkdtemp(path));const fs::path root=path;
    put(root/"class/hwmon/hwmon0/name","fixture\033[2J");
    put(root/"class/hwmon/hwmon0/temp1_input","27500\n");put(root/"class/hwmon/hwmon0/temp1_label","ambient\n");
    put(root/"class/hwmon/hwmon0/in1_input","1800\n");put(root/"class/hwmon/hwmon0/temp1_max","999999\n");
    put(root/"bus/iio/devices/iio:device0/in_illuminance_raw","100\n");put(root/"bus/iio/devices/iio:device0/in_illuminance_scale","0.5\n");
    put(root/"bus/iio/devices/iio:device0/in_accel_x_raw","-2\n");put(root/"bus/iio/devices/iio:device0/in_accel_scale","0.25\n");put(root/"bus/iio/devices/iio:device0/in_accel_offset","1\n");
    put(root/"bus/iio/devices/iio:device0/in_proximity_raw","7\n");put(root/"bus/iio/devices/iio:device0/in_intensity_red_raw","42\n");
    put(root/"bus/iio/devices/iio:device0/in_distance_input","nan\n");put(root/"bus/iio/devices/iio:device0/in_voltage_raw","1e999\n");
    put(root/"bus/iio/devices/iio:device0/in_temp_input","1000\n");
    put(root/"class/backlight/panel/actual_brightness","200\n");put(root/"class/backlight/panel/max_brightness","1000\n");
    put(root/"class/thermal/thermal_zone0/temp","-5000\n");
    auto rows=uke::sensors::sysfs_snapshot(root);assert(rows.size()==12);
    bool light=false,accel=false,near=false,invalid=false;
    for(const auto& row:rows) {
        assert(row.label.find('\033')==std::string::npos);
        if(row.source.ends_with("in_illuminance_raw")){assert(row.value==50&&row.unit=="lux");light=true;}
        if(row.source.ends_with("in_accel_x_raw")){assert(row.value==-0.25&&row.unit=="m/s^2");accel=true;}
        if(row.source.ends_with("in_proximity_raw")){assert(row.value==7&&row.unit=="raw counts");near=true;}
        if(row.source.ends_with("in_distance_input")){assert(!row.value);invalid=true;}
        assert(!row.source.ends_with("temp1_max"));
    }
    assert(light&&accel&&near&&invalid);
    auto encoded=uke::sensors::json(rows);sd_json_variant* doc=nullptr;
    assert(sd_json_parse(encoded.c_str(),static_cast<sd_json_parse_flags_t>(0),&doc,nullptr,nullptr)>=0);sd_json_variant_unref(doc);
    auto missing=uke::sensors::sysfs_snapshot(root/"missing");assert(missing.empty());
    for(const auto& arg:{"-n0","-n-1","-n1.5","-n999999999999999999999","--bad"}) {
        char* args[]={const_cast<char*>(arg)};bool rejected=false;
        try{uke::sensors::command(1,args);}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    fs::remove_all(root);
}
