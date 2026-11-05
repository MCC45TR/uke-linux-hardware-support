// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
inline QJsonObject managerDevice(const QString &kind="cpu") {
    const bool cpu=kind=="cpu";
    return {{"id",cpu?"cpu:policy0":"gpu:gpu0"},{"kind",kind},{"name",cpu?"policy0":"gpu0"},{"path",cpu?"/sys/devices/system/cpu/cpufreq/policy0":"/sys/devices/platform/gpu/devfreq/gpu0"},
      {"driver",cpu?"qcom-cpufreq-hw":"adreno"},{"minimum",cpu?"100000":"100000000"},{"maximum",cpu?"300000":"300000000"},{"frequency",cpu?"200000":"200000000"},
      {"hardware_minimum",cpu?"100000":""},{"hardware_maximum",cpu?"300000":""},{"governor","schedutil"},
      {"frequencies",cpu?QJsonArray{"100000","200000","300000"}:QJsonArray{"100000000","200000000","300000000"}},
      {"governors",QJsonArray{"schedutil","performance","userspace"}},{"cpus",cpu?"0 1":""},{"writable",true}};
}
inline QJsonObject managerSample() {
    return {{"devices",QJsonArray{managerDevice(),managerDevice("gpu")}},{"ticks",QJsonObject{{"cpu0",QJsonArray{100,0,50,800,50,0,0,0}},{"cpu1",QJsonArray{100,0,50,800,50,0,0,0}}}},
      {"temperatures",QJsonArray{QJsonObject{{"label","cpuss-0"},{"value",42}},QJsonObject{{"label","gpuss-0"},{"value",38}}}},
      {"resources",QJsonObject{
        {"sections",QJsonArray{QJsonObject{{"title","Memory pressure"},{"rows",QJsonArray{QJsonObject{{"title","Some stalls"},{"value","2.5 %"},{"status","available"}},QJsonObject{{"title","Optional provider"},{"value",""},{"status","no-data"}}}}}}},
        {"metrics",QJsonObject{{"memory/used",2048},{"psi/memory/some",2.5}}},
        {"charts",QJsonArray{QJsonObject{{"id","memory/used"},{"title","Memory used"},{"unit","MiB"},{"maximum",8192}},QJsonObject{{"id","psi/memory/some"},{"title","Memory pressure"},{"unit","%"},{"maximum",100}}}},
        {"controls",QJsonArray{
            QJsonObject{{"id","memory.swappiness"},{"title","Swap preference"},{"value","60"},{"unit",""},{"path","/proc/sys/vm/swappiness"},{"status","available"},{"writable",true},{"minimum",0},{"maximum",200},{"choices",QJsonArray{}}},
            QJsonObject{{"id","memory.thp"},{"title","Transparent huge pages"},{"value","madvise"},{"unit",""},{"path","/sys/kernel/mm/transparent_hugepage/enabled"},{"status","available"},{"writable",true},{"choices",QJsonArray{"always","madvise","never"}}},
            QJsonObject{{"id","memory.mglru"},{"title","Multi-generation LRU"},{"value",""},{"unit",""},{"path",""},{"status","not-exported"},{"writable",false},{"choices",QJsonArray{}}}}}}},
      {"applications",QJsonArray{QJsonObject{{"unit","app-example.scope"},{"title","Example application"},{"invocation",QString(32,'a')},{"fingerprint",QString(64,'b')},{"weight","100"},{"memoryHigh","infinity"},{"quotaSupported",false},{"uclampSupported",false},{"ioLimitSupported",false}}}}};
}
inline QJsonObject managerSensors() {
    return {{"readings",QJsonArray{QJsonObject{{"source","class/thermal/thermal_zone0/temp"},{"label","cpuss-0"},{"unit","C"},{"status","read (sample age unknown)"},{"value",42}},
      QJsonObject{{"source","SensorProxy/LightLevel"},{"label","Light level"},{"unit","lux"},{"status","cached (sample age unknown)"},{"value",120}},
      QJsonObject{{"source","SensorProxy/CompassHeading"},{"label","Compass heading"},{"unit","degrees"},{"status","unavailable"},{"value",QJsonValue()}}}}};
}
