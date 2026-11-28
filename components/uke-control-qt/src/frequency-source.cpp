// SPDX-License-Identifier: MIT
#include "frequency-source.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace uq {
namespace {
QString read(const QString &path,int limit=4096) {
    QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};
    const auto b=f.read(limit+1);return b.size()>limit?QString():QString::fromUtf8(b).trimmed();
}
QJsonArray words(const QString &text,bool numeric=false) {
    QJsonArray out;QSet<QString> seen;
    for(const auto &v:text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) {
        if(out.size()>=256)break;bool ok=false;const auto n=v.toULongLong(&ok);
        if(numeric&&(!ok||n==0||QString::number(n)!=v))continue;
        if(!seen.contains(v)){out<<v;seen.insert(v);}
    }
    return out;
}
bool machine() {
    QFile f("/sys/firmware/devicetree/base/compatible");if(!f.open(QIODevice::ReadOnly))return false;
    const auto b=f.read(4096);return b.split('\0').contains("xiaomi,uke")&&b.split('\0').contains("qcom,sm7675");
}
QString pathFor(const QString &kind,const QString &name) {
    if(kind=="cpu"&&QRegularExpression("^policy[0-9]{1,3}$").match(name).hasMatch())return "/sys/devices/system/cpu/cpufreq/"+name;
    if(kind=="gpu"&&QRegularExpression("^[a-zA-Z0-9,._-]{1,100}$").match(name).hasMatch()&&name!="."&&name!="..")return "/sys/class/devfreq/"+name;
    return {};
}
bool integer(const QJsonValue &v,qulonglong &out){bool ok=false;const auto s=v.toString();out=s.toULongLong(&ok);return ok&&QString::number(out)==s;}
}
QJsonObject frequencyDevice(const QString &kind,const QString &name) {
    const auto p=pathFor(kind,name);if(p.isEmpty())return {};
    const auto canonical=QFileInfo(p).canonicalFilePath();if(!canonical.startsWith("/sys/devices/"))return {};
    const bool cpu=kind=="cpu";
    const auto driver=cpu?read(p+"/scaling_driver"):QFileInfo(QFileInfo(p+"/device/driver").canonicalFilePath()).fileName();
    if(!cpu&&driver!="adreno")return {}; // UFS devfreq is not a GPU.
    QJsonObject d{{"id",kind+":"+name},{"kind",kind},{"name",name},{"path",canonical},{"driver",driver},
        {"minimum",read(p+(cpu?"/scaling_min_freq":"/min_freq"))},
        {"maximum",read(p+(cpu?"/scaling_max_freq":"/max_freq"))},
        {"governor",read(p+(cpu?"/scaling_governor":"/governor"))},
        {"frequencies",words(read(p+(cpu?"/scaling_available_frequencies":"/available_frequencies")),true)},
        {"governors",words(read(p+(cpu?"/scaling_available_governors":"/available_governors")))},
        {"frequency",read(p+(cpu?"/scaling_cur_freq":"/cur_freq"))},
        {"unit",cpu?"kHz":"Hz"},{"cpus",cpu?read(p+"/related_cpus"):QString()},
        {"hardware_minimum",cpu?read(p+"/cpuinfo_min_freq"):QString()},
        {"hardware_maximum",cpu?read(p+"/cpuinfo_max_freq"):QString()}};
    d["writable"]=machine()&&(!cpu||driver=="qcom-cpufreq-hw")&&!d["frequencies"].toArray().isEmpty();
    return d;
}
QJsonObject performanceSnapshot() {
    QJsonArray devices,temperatures;QJsonObject ticks;
    for(const auto &name:QDir("/sys/devices/system/cpu/cpufreq").entryList({"policy*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(devices.size()>=32)break;auto d=frequencyDevice("cpu",name);if(!d.isEmpty())devices<<d;
    }
    for(const auto &name:QDir("/sys/class/devfreq").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(devices.size()>=40)break;auto d=frequencyDevice("gpu",name);if(!d.isEmpty())devices<<d;
    }
    for(const auto &line:read("/proc/stat",65536).split('\n')) {
        const auto fields=line.simplified().split(' ');if(fields.size()<9||!QRegularExpression("^cpu[0-9]+$").match(fields[0]).hasMatch())continue;
        QJsonArray counters;bool valid=true;
        for(int i=1;i<=8;++i){bool ok=false;auto n=fields[i].toULongLong(&ok);if(!ok||n>9007199254740991ULL){valid=false;break;}counters<<double(n);}
        if(valid)ticks[fields[0]]=counters;
    }
    for(const auto &name:QDir("/sys/class/thermal").entryList({"thermal_zone*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(temperatures.size()>=128)break;const auto p="/sys/class/thermal/"+name;
        bool ok=false;const auto n=read(p+"/temp").toDouble(&ok);
        temperatures<<QJsonObject{{"id",name},{"label",read(p+"/type")},{"value",ok&&std::isfinite(n)&&n>=-273150&&n<=300000?QJsonValue(n/1000):QJsonValue()}};
    }
    return {{"devices",devices},{"ticks",ticks},{"temperatures",temperatures}};
}
QString frequencyRefusal(const QJsonObject &d,const QString &field,const QString &value) {
    if(d.isEmpty()||!d.value("writable").toBool())return "provider_not_supported";
    if(field!="minimum"&&field!="maximum"&&field!="governor")return "invalid_field";
    if(field=="governor")return value!="userspace"&&d.value("governors").toArray().contains(value)?QString():QString("governor_not_supported");
    qulonglong n=0,lo=0,hi=0;if(!integer(value,n)||!integer(d.value("minimum"),lo)||!integer(d.value("maximum"),hi))return "invalid_frequency";
    const auto frequencies=d.value("frequencies").toArray();
    if(n==0||!frequencies.contains(value))return "frequency_not_advertised";
    if(field=="minimum")lo=n;else hi=n;
    if(d.value("kind").toString()=="gpu") {
        qulonglong lowest=std::numeric_limits<qulonglong>::max(),highest=0;
        for(const auto &v:frequencies){qulonglong x=0;if(integer(v,x)){lowest=std::min(lowest,x);highest=std::max(highest,x);}}
        if(!lo)lo=lowest;if(!hi)hi=highest;
    } else {
        qulonglong hwLo=0,hwHi=0;
        if(!integer(d.value("hardware_minimum"),hwLo)||!integer(d.value("hardware_maximum"),hwHi)||n<hwLo||n>hwHi)return "outside_hardware_limits";
    }
    return lo<=hi?QString():QString("minimum_exceeds_maximum");
}
FrequencyResult changeFrequency(const QJsonObject &before,const QString &field,const QString &value,const QJsonObject &expected,
    const std::function<QJsonObject()> &read,const std::function<bool(const QString &,const QString &)> &write) {
    FrequencyResult r;r.after=before;
    for(const auto &key:{"minimum","maximum","governor"})if(before.value(key)!=expected.value(key)){r.error="settings_changed";return r;}
    r.error=frequencyRefusal(before,field,value);if(!r.error.isEmpty())return r;
    if(before.value(field).toString()==value)return r;
    // One attribute per request. No partly applied multi-attribute profile.
    const auto fresh=read();
    for(const auto &key:{"minimum","maximum","governor"})if(fresh.value(key)!=before.value(key)){r.after=fresh;r.error="settings_changed";return r;}
    if(fresh.value("path")!=before.value("path")||fresh.value("driver")!=before.value("driver")||!frequencyRefusal(fresh,field,value).isEmpty()){r.error="provider_changed";return r;}
    if(!write(field,value)){r.after=read();r.error="write_failed";return r;}
    r.after=read();r.applied=true;
    if(r.after.value(field).toString()!=value)r.error="readback_differs";
    return r;
}
}
