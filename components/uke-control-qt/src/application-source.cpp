// SPDX-License-Identifier: MIT
#include "application-source.hpp"
#include <unistd.h>
#include <cmath>
namespace uq {
namespace {
QByteArray run(const QStringList &args,int &code,int timeout=1500) {
    QProcess p;p.start("/usr/bin/systemctl",QStringList{"--user"}+args);QByteArray out;QElapsedTimer t;t.start();
    while(p.state()!=QProcess::NotRunning&&t.elapsed()<timeout){p.waitForFinished(50);out+=p.readAllStandardOutput();if(out.size()>262144)break;}
    if(p.state()!=QProcess::NotRunning){p.kill();p.waitForFinished(500);code=70;return {};}
    out+=p.readAllStandardOutput();code=p.exitStatus()==QProcess::NormalExit?p.exitCode():70;return out.size()<=262144?out:QByteArray();
}
bool allowed(const QString &unit){return QRegularExpression("^app-[A-Za-z0-9_.@\\\\:-]{1,180}\\.(scope|service)$").match(unit).hasMatch()&&!unit.contains("..")&&!unit.contains('/');}
QMap<QString,QString> properties(const QString &unit) {
    if(!allowed(unit))return {};int code=0;
    const auto b=run({"show",unit,"--property=Id,Description,ActiveState,InvocationID,ControlGroup,CPUWeight,CPUQuotaPerSecUSec,MemoryCurrent,MemoryHigh"},code);
    QMap<QString,QString> out;if(code)return out;
    for(const auto &line:b.split('\n')){const auto i=line.indexOf('=');if(i>0)out[QString::fromUtf8(line.left(i))]=QString::fromUtf8(line.mid(i+1));}
    const auto cg=out.value("ControlGroup"),prefix="/user.slice/user-"+QString::number(getuid())+".slice/user@"+QString::number(getuid())+".service/";
    if(out.value("Id")!=unit||out.value("ActiveState")!="active"||!cg.startsWith(prefix)||!QRegularExpression("^[a-f0-9]{32}$").match(out.value("InvocationID")).hasMatch())return {};
    return out;
}
QString fingerprint(const QMap<QString,QString> &p){QByteArray b;for(const auto &k:{"InvocationID","ControlGroup","CPUWeight","MemoryHigh","CPUQuotaPerSecUSec"})b+=p.value(k).toUtf8()+'\0';return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex());}
bool exported(const QString &cg,const QString &name){return QFileInfo::exists("/sys/fs/cgroup"+cg+"/"+name);}
QString cgroupText(const QString &cg,const QString &name){QFile file("/sys/fs/cgroup"+cg+"/"+name);return file.open(QIODevice::ReadOnly)?QString::fromUtf8(file.read(4096)).trimmed():QString();}
}
QJsonObject applicationScope(const QString &unit) {
    const auto p=properties(unit);if(p.isEmpty())return {};
    QJsonValue cpuUsage;for(const auto &line:cgroupText(p.value("ControlGroup"),"cpu.stat").split('\n')){const auto w=line.simplified().split(' ');if(w.size()==2&&w[0]=="usage_usec"){bool ok=false;const auto n=w[1].toDouble(&ok);if(ok&&std::isfinite(n)&&n>=0)cpuUsage=n;}}
    return {{"unit",unit},{"title",p.value("Description").left(200)},{"invocation",p.value("InvocationID")},{"fingerprint",fingerprint(p)},
        {"weight",p.value("CPUWeight")},{"memory",p.value("MemoryCurrent")},{"memoryHigh",p.value("MemoryHigh")},{"quota",p.value("CPUQuotaPerSecUSec")},
        {"quotaRaw",cgroupText(p.value("ControlGroup"),"cpu.max")},
        {"cpuUsage",cpuUsage},
        {"quotaSupported",exported(p.value("ControlGroup"),"cpu.max")},{"uclampSupported",exported(p.value("ControlGroup"),"cpu.uclamp.max")},
        {"ioLimitSupported",exported(p.value("ControlGroup"),"io.max")}};
}
QJsonArray applicationScopes() {
    QElapsedTimer deadline;deadline.start();int code=0;const auto output=run({"list-units","--state=active","--no-legend","--plain","app-*.scope","app-*.service"},code);QJsonArray out;if(code)return out;
    for(const auto &line:QString::fromUtf8(output).split('\n')) {if(out.size()>=32||deadline.elapsed()>1500)break;const auto name=line.simplified().section(' ',0,0);if(!allowed(name))continue;const auto item=applicationScope(name);if(!item.isEmpty())out<<item;}return out;
}
QString lastApplicationPreset() {
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/uke-control-qt/last-resource-preset.json");
    if(!file.open(QIODevice::ReadOnly)||file.size()>4096)return "normal";
    const auto d=QJsonDocument::fromJson(file.read(4096)).object();const auto preset=d.value("preset").toString();
    return d.value("automatic").isBool()&&!d.value("automatic").toBool()&&QStringList{"normal","priority","background","limited"}.contains(preset)?preset:QString("normal");
}
QJsonObject applyApplicationPreset(const QString &unit,const QString &invocation,const QString &expected,const QString &preset) {
    QJsonObject result{{"schema","org.uke.qt.application-control.v1"},{"persistent",false}};
    if(geteuid()==0||!QStringList{"normal","priority","background","limited"}.contains(preset)){result["error"]="invalid_request";return result;}
    const auto p=properties(unit);if(p.isEmpty()||p.value("InvocationID")!=invocation||fingerprint(p)!=expected){result["error"]="settings_changed";return result;}
    const bool quota=exported(p.value("ControlGroup"),"cpu.max");
    const auto weight=preset=="priority"?"200":preset=="background"?"25":"100";
    const auto memory=preset=="limited"?"1073741824":"infinity";
    QStringList args{"set-property","--runtime",unit,"CPUWeight="+QString(weight),"MemoryHigh="+QString(memory)};
    if(quota)args<<"CPUQuota="+QString(preset=="limited"?"50%":"infinity");
    int code=0;run(args,code,4000);const auto after=applicationScope(unit);result["after"]=after;
    const auto q=after.value("quotaRaw").toString().simplified().split(' ');
    bool quotaReadback=!quota;
    if(quota&&q.size()==2){bool a=false,b=false;const auto amount=q[0].toULongLong(&a),period=q[1].toULongLong(&b);quotaReadback=preset=="limited"?a&&b&&period>0&&amount*2==period:q[0]=="max";}
    const bool readback=after.value("invocation").toString()==invocation&&after.value("weight").toString()==weight&&after.value("memoryHigh").toString()==memory&&quotaReadback;
    result["error"]=code?"request_failed":!readback?"readback_differs":"";
    result["quotaSupported"]=quota;result["preset"]=preset;
    // Store a reviewed preset as a local record, never an automatic rule for
    // random transient unit names. The next invocation requires a fresh review.
    if(code==0&&readback) {
        const auto dir=QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/uke-control-qt";
        if(QDir().mkpath(dir)) {QSaveFile f(dir+"/last-resource-preset.json");if(f.open(QIODevice::WriteOnly)){f.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);f.write(QJsonDocument(QJsonObject{{"preset",preset},{"automatic",false}}).toJson());f.commit();}}
    }
    return result;
}
}
