// SPDX-License-Identifier: MIT
#include "resource-source.hpp"
#include <QStorageInfo>
#include <zlib.h>
#include <cerrno>
#include <cmath>
#include <unistd.h>
namespace uq {
namespace {
struct Value {QString text,status;};
Value read(const QString &path,int limit=65536) {
    QFile f(path);errno=0;
    if(!f.open(QIODevice::ReadOnly))return {{},errno==ENODATA?"no-data":errno==EACCES||errno==EPERM?"permission-denied":QFileInfo::exists(path)?"read-failed":"not-exported"};
    const auto b=f.read(limit+1);
    if(f.error()!=QFileDevice::NoError)return {{},errno==ENODATA?"no-data":"read-failed"};
    if(b.size()>limit)return {{},"too-large"};
    return {QString::fromUtf8(b).trimmed(),b.isEmpty()?"empty":"available"};
}
QString text(const QString &p){return read(p).text;}
QJsonValue numeric(const QString &s,double scale=1){bool ok=false;const double n=s.toDouble(&ok);return ok&&std::isfinite(n)&&n>=0&&n<=9007199254740991.?QJsonValue(n/scale):QJsonValue();}
QJsonValue signedNumber(const QString &s,double scale,double minimum,double maximum){bool ok=false;const double n=s.toDouble(&ok)/scale;return ok&&std::isfinite(n)&&n>=minimum&&n<=maximum?QJsonValue(n):QJsonValue();}
QJsonObject fields(const QString &p,bool colon=false) {
    QJsonObject out;
    for(auto line:text(p).split('\n')) {if(colon)line.replace(':',' ');const auto w=line.simplified().split(' ');if(w.size()>=2)out[w[0]]=numeric(w[1]);}
    return out;
}
QJsonObject kernelConfig(const ResourceRoots &r) {
    // Read once per boot; never spawn a command or decompress on every sample.
    if(r.proc!="/proc")return fields(r.proc+"/uke-test-config");
    static const QJsonObject cached=[] {
        QJsonObject out;gzFile f=gzopen("/proc/config.gz","rb");if(!f)return out;
        QByteArray b;char block[8192];int n=0;
        while((n=gzread(f,block,sizeof(block)))>0&&b.size()<2*1024*1024)b.append(block,n);
        gzclose(f);if(n<0||b.size()>=2*1024*1024)return out;
        for(const auto &line:b.split('\n')) {if(!line.startsWith("CONFIG_"))continue;const auto i=line.indexOf('=');if(i>0)out[QString::fromLatin1(line.left(i))]=QString::fromLatin1(line.mid(i+1));}
        return out;
    }();return cached;
}
QString queue(const ResourceRoots &r) {
    QString name;
    if(r.sys!="/sys")name=text(r.proc+"/uke-test-root-device");
    else name=QFileInfo(QString::fromLocal8Bit(QStorageInfo::root().device())).fileName();
    if(!QRegularExpression("^[A-Za-z0-9_.!-]{1,80}$").match(name).hasMatch())return {};
    auto p=QFileInfo(r.sys+"/class/block/"+name).canonicalFilePath();
    if(p.isEmpty()||!p.startsWith(QFileInfo(r.sys).canonicalFilePath()+"/devices/"))return {};
    if(QFileInfo::exists(p+"/partition"))p=QFileInfo(p).dir().absolutePath();
    // A device mapper with several backing devices cannot be represented by
    // one scheduler. Refuse ambiguous mappings instead of guessing a queue.
    QSet<QString> seen;
    for(int depth=0;depth<8;++depth) {
        if(seen.contains(p))return {};seen.insert(p);
        const auto slaves=QDir(p+"/slaves").entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        if(slaves.isEmpty())return QFileInfo::exists(p+"/queue")?p:QString();
        if(slaves.size()!=1)return {};
        p=QFileInfo(p+"/slaves/"+slaves[0]).canonicalFilePath();
    }return {};
}
QJsonArray options(const QString &s){QJsonArray a;for(auto w:s.simplified().split(' ',Qt::SkipEmptyParts)){w.remove('[');w.remove(']');if(a.size()<128)a<<w;}return a;}
QString selected(const QString &s){auto m=QRegularExpression("\\[([^]]+)\\]").match(s);return m.hasMatch()?m.captured(1):s;}
struct SettingSpec {QString path,title,unit;int minimum=0,maximum=0;bool categorical=false;};
SettingSpec settingSpec(const QString &id,const ResourceRoots &r) {
    const QMap<QString,SettingSpec> fixed{
        {"memory.swappiness",{r.proc+"/sys/vm/swappiness","Swap preference","",0,200}},
        {"memory.thp",{r.sys+"/kernel/mm/transparent_hugepage/enabled","Transparent huge pages","",0,0,true}},
        {"memory.ksm",{r.sys+"/kernel/mm/ksm/run","Merge identical pages","",0,1}},
        {"memory.mglru",{r.sys+"/kernel/mm/lru_gen/enabled","Multi-generation LRU","",0,0,true}},
        {"cpu.idle-governor",{r.sys+"/devices/system/cpu/cpuidle/current_governor","CPU idle governor","",0,0,true}}
    };
    if(fixed.contains(id))return fixed[id];
    auto m=QRegularExpression("^cpu\\.(policy[0-9]{1,3})\\.(rate_limit_us|up_threshold|sampling_rate|sampling_down_factor|freq_step|down_threshold)$").match(id);
    if(m.hasMatch()) {
        const auto p=r.sys+"/devices/system/cpu/cpufreq/"+m.captured(1),gov=text(p+"/scaling_governor"),key=m.captured(2);
        if(!QStringList{"schedutil","ondemand","conservative"}.contains(gov))return {};
        if(gov=="schedutil"&&key!="rate_limit_us")return {};
        if(gov!="schedutil"&&key=="rate_limit_us")return {};
        if(gov=="ondemand"&&(key=="freq_step"||key=="down_threshold"))return {};
        const bool percent=key=="up_threshold"||key=="down_threshold"||key=="freq_step";
        return {p+"/"+gov+"/"+key,m.captured(1)+" / "+gov+" / "+key,percent?"%":key.endsWith("us")||key=="sampling_rate"?"µs":"",percent?1:0,percent?100:key=="sampling_down_factor"?100000:10000000};
    }
    if(id.startsWith("storage.")) {
        const auto p=queue(r);if(p.isEmpty())return {};const auto key=id.mid(8);
        const QMap<QString,SettingSpec> specs{
            {"scheduler",{p+"/queue/scheduler","I/O scheduler","",0,0,true}},
            {"read-ahead",{p+"/queue/read_ahead_kb","Read ahead","KiB",0,16384}},
            {"requests",{p+"/queue/nr_requests","Queue request limit","",4,4096}},
            {"read-expire",{p+"/queue/iosched/read_expire","Read expiry","ms",1,60000}},
            {"write-expire",{p+"/queue/iosched/write_expire","Write expiry","ms",1,60000}},
            {"fifo-batch",{p+"/queue/iosched/fifo_batch","FIFO batch","",1,128}},
            {"writes-starved",{p+"/queue/iosched/writes_starved","Writes starved","",1,64}}
        };return specs.value(key);
    }return {};
}
void row(QJsonArray &a,const QString &title,const QString &value,const QString &status="available") {if(a.size()<512)a<<QJsonObject{{"title",title},{"value",value},{"status",status}};}
void fileRow(QJsonArray &a,const QString &title,const QString &p){const auto v=read(p);row(a,title,v.text,v.status);}
void section(QJsonArray &a,const QString &title,const QJsonArray &rows){if(!rows.isEmpty())a<<QJsonObject{{"title",title},{"rows",rows}};}
void counter(QJsonObject &out,const QString &id,const QJsonValue &value,double scale=1){if(value.isDouble())out[id]=QJsonObject{{"value",value},{"scale",scale}};}
QString group(const QString &type) {
    const auto s=type.toLower();
    if(s.startsWith("cpuss")||QRegularExpression("^cpu-[0-9]").match(s).hasMatch())return "CPU die";
    if(s.startsWith("gpuss"))return "GPU die";
    if(s.contains("batt"))return "Battery";
    if(s.contains("conn")||s.contains("usb"))return "Connector";
    if(s.contains("charger")||s.contains("chg")||s.contains("pmic"))return "Power management";
    return "Board and peripherals";
}
}
QJsonObject resourceSetting(const QString &id,const ResourceRoots &r) {
    const auto spec=settingSpec(id,r);if(spec.path.isEmpty())return {};
    const auto v=read(spec.path);QJsonArray choices;
    if(spec.categorical) {
        if(id=="memory.mglru")choices=QJsonArray{"0","1","7"};
        else if(id=="cpu.idle-governor")choices=options(text(r.sys+"/devices/system/cpu/cpuidle/available_governors"));
        else choices=options(v.text);
    }
    const auto current=v.status!="available"?QString():id=="memory.mglru"?QString::number(v.text.toUInt(nullptr,0)):selected(v.text);
    return {{"id",id},{"title",spec.title},{"unit",spec.unit},{"value",current},{"path",QFileInfo(spec.path).canonicalFilePath()},
        {"status",v.status},{"minimum",spec.minimum},{"maximum",spec.maximum},{"choices",choices},
        {"writable",v.status=="available"&&bool(QFileInfo(spec.path).permissions() & QFileDevice::WriteOwner)},
        {"scope","Runtime only; not saved for the next boot"}};
}
QString resourceRefusal(const QJsonObject &s,const QString &value) {
    if(s.isEmpty()||s.value("status").toString()!="available")return "provider_unavailable";
    if(!s.value("choices").toArray().isEmpty())return s.value("choices").toArray().contains(value)?QString():QString("choice_not_advertised");
    bool ok=false;const auto n=value.toLongLong(&ok);
    if(!ok||QString::number(n)!=value||n<s.value("minimum").toInt()||n>s.value("maximum").toInt())return "outside_limits";
    if(s.value("id").toString().endsWith("read-ahead")&&n%4)return "alignment_required";
    return {};
}
QJsonObject resourceRates(const QJsonObject &before,const QJsonObject &after,double seconds) {
    QJsonObject out;
    for(auto i=after.begin();i!=after.end();++i) {
        const auto a=before.value(i.key()).toObject(),b=i.value().toObject();const auto x=a.value("value"),y=b.value("value");
        out[i.key()]=seconds>0&&seconds<=30&&x.isDouble()&&y.isDouble()&&y.toDouble()>=x.toDouble()&&a.value("scale")==b.value("scale")?QJsonValue((y.toDouble()-x.toDouble())*b.value("scale").toDouble(1)/seconds):QJsonValue();
    }return out;
}
QJsonObject resourceSnapshot(const QString &domain,const ResourceRoots &r) {
    const bool all=domain=="manager-overview"||domain=="diagnostics";
    QJsonArray sections,controls,charts;QJsonObject metrics,counters;QStringList ids;
    auto chart=[&](const QString &id,const QString &title,const QString &unit,double max=0){charts<<QJsonObject{{"id",id},{"title",title},{"unit",unit},{"maximum",max}};};
    auto psi=[&](const QString &kind){QJsonArray rows;const auto pressure=read(r.proc+"/pressure/"+kind);for(const auto &line:pressure.text.split('\n')) {
        const auto w=line.simplified().split(' ');if(w.size()<2)continue;
        for(const auto &f:w.mid(1)){const auto p=f.split('=');if(p.size()!=2)continue;
            if(p[0]=="avg10"){const auto id="psi/"+kind+"/"+w[0];metrics[id]=numeric(p[1]);row(rows,w[0]+" / last 10 seconds",p[1]+" %");if(w[0]=="some")chart(id,kind+" pressure","%",100);}
            if(p[0]=="total")counter(counters,"stall/"+kind+"/"+w[0],numeric(p[1]),.0001);
        }
    }if(rows.isEmpty())row(rows,"Pressure observations",{},pressure.status=="available"?"invalid-value":pressure.status);section(sections,kind+" pressure",rows);};
    if(all||domain=="memory") {
        const auto m=fields(r.proc+"/meminfo",true);QJsonArray rows;
        for(const auto &k:{"MemTotal","MemAvailable","Cached","Buffers","SReclaimable","Shmem","SwapTotal","SwapFree","Dirty","Writeback"})
            row(rows,k,m.value(k).isDouble()?QString::number(m.value(k).toDouble()/1024.,'f',1)+" MiB":QString(),m.value(k).isDouble()?"available":"not-exported");
        if(m.value("MemTotal").isDouble()&&m.value("MemAvailable").isDouble())metrics["memory/used"]=(m.value("MemTotal").toDouble()-m.value("MemAvailable").toDouble())/1024.;
        if(m.value("SwapTotal").isDouble()&&m.value("SwapFree").isDouble())metrics["memory/swap"]=(m.value("SwapTotal").toDouble()-m.value("SwapFree").toDouble())/1024.;
        chart("memory/used","Memory excluding available","MiB");chart("memory/swap","Swap used","MiB");section(sections,"Memory",rows);psi("memory");
        const auto vm=fields(r.proc+"/vmstat");
        for(const auto &k:{"pgfault","pgmajfault","pswpin","pswpout","pgscan_kswapd","pgscan_direct","pgsteal_kswapd","pgsteal_direct"})counter(counters,"vm/"+QString(k),vm.value(k));
        chart("vm/pgmajfault","Major faults","faults/s");chart("vm/pswpout","Swap out","pages/s");
        QJsonArray swaps;const auto lines=text(r.proc+"/swaps").split('\n');
        for(const auto &line:lines.mid(1)){const auto w=line.simplified().split(' ');if(w.size()>=5)row(swaps,QFileInfo(w[0]).fileName()+" / "+w[1],"size "+w[2]+" KiB; used "+w[3]+" KiB; priority "+w[4]);}
        section(sections,"Active swap backends",swaps);
        for(const auto &n:QDir(r.sys+"/block").entryList({"zram*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            QJsonArray z;const auto p=r.sys+"/block/"+n;
            fileRow(z,"Logical capacity (bytes)",p+"/disksize");fileRow(z,"Compression algorithms",p+"/comp_algorithm");
            const auto w=text(p+"/mm_stat").simplified().split(' ');
            if(w.size()>=3){row(z,"Original data",w[0]+" bytes");row(z,"Compressed data",w[1]+" bytes");row(z,"Actual RAM cost",w[2]+" bytes");}
            fileRow(z,"Backing device",p+"/backing_dev");section(sections,n,z);
        }
        ids<<"memory.swappiness"<<"memory.thp"<<"memory.ksm"<<"memory.mglru";
    }
    if(all||domain=="storage") {
        psi("io");QJsonArray rows;const auto p=queue(r);
        if(p.isEmpty())row(rows,"Root filesystem backing queue",{},"ambiguous-or-not-exported");
        else {
            row(rows,"Backing queue",QFileInfo(p).fileName());row(rows,"Scope","All partitions and applications using this queue");
            fileRow(rows,"Logical block size",p+"/queue/logical_block_size");fileRow(rows,"Physical block size",p+"/queue/physical_block_size");
            const auto s=text(p+"/stat").simplified().split(' ');
            if(s.size()>=11) {
                counter(counters,"io/read",numeric(s[2]),512./1048576);counter(counters,"io/write",numeric(s[6]),512./1048576);
                counter(counters,"io/read-ops",numeric(s[0]));counter(counters,"io/write-ops",numeric(s[4]));
                counter(counters,"io/read-ms",numeric(s[3]));counter(counters,"io/write-ms",numeric(s[7]));
                metrics["io/in-flight"]=numeric(s[8]);counter(counters,"io/busy",numeric(s[9]),.1);
            }
            chart("io/read","Storage reads","MiB/s");chart("io/write","Storage writes","MiB/s");
            chart("io/read-latency","Mean read completion time","ms");chart("io/write-latency","Mean write completion time","ms");
            ids<<"storage.scheduler"<<"storage.read-ahead"<<"storage.requests"<<"storage.read-expire"<<"storage.write-expire"<<"storage.fifo-batch"<<"storage.writes-starved";
        }section(sections,"Root filesystem I/O",rows);
        for(const auto &n:QDir(r.sys+"/class/devfreq").entryList({"*ufshc*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            QJsonArray u;const auto p=r.sys+"/class/devfreq/"+n;fileRow(u,"Clock (Hz)",p+"/cur_freq");fileRow(u,"Governor",p+"/governor");section(sections,"UFS controller",u);
        }
    }
    if(all||domain=="cpu") {
        psi("cpu");QJsonArray global;fileRow(global,"Idle governor",r.sys+"/devices/system/cpu/cpuidle/current_governor_ro");fileRow(global,"Available idle governors",r.sys+"/devices/system/cpu/cpuidle/available_governors");fileRow(global,"Energy-aware scheduling",r.proc+"/sys/kernel/sched_energy_aware");section(sections,"CPU scheduling",global);ids<<"cpu.idle-governor";
        for(const auto &n:QDir(r.sys+"/devices/system/cpu/cpufreq").entryList({"policy*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            QJsonArray rows;const auto p=r.sys+"/devices/system/cpu/cpufreq/"+n;
            fileRow(rows,"Related CPUs",p+"/related_cpus");fileRow(rows,"Transition count",p+"/stats/total_trans");
            const auto tis=read(p+"/stats/time_in_state");qulonglong sum=0;QList<QPair<QString,qulonglong>> states;
            for(const auto &line:tis.text.split('\n')){const auto w=line.simplified().split(' ');bool ok=false;if(w.size()!=2)continue;const auto t=w[1].toULongLong(&ok);if(ok){states<<qMakePair(w[0],t);sum+=t;}}
            for(const auto &s:states)row(rows,s.first+" kHz",QString::number(s.second*100./std::max<qulonglong>(1,sum),'f',1)+" % since statistics reset");
            if(states.isEmpty())row(rows,"Frequency residency",{},tis.status);
            section(sections,n+" frequency residency",rows);
            for(const auto &k:{"rate_limit_us","up_threshold","sampling_rate","sampling_down_factor","freq_step","down_threshold"}){const auto id="cpu."+n+"."+k;const auto spec=settingSpec(id,r);if(!spec.path.isEmpty()&&QFileInfo::exists(spec.path))ids<<id;}
        }
        QJsonArray idle;
        for(const auto &cpu:QDir(r.sys+"/devices/system/cpu").entryList({"cpu*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            if(!QRegularExpression("^cpu[0-9]+$").match(cpu).hasMatch())continue;
            for(const auto &st:QDir(r.sys+"/devices/system/cpu/"+cpu+"/cpuidle").entryList({"state*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
                const auto p=r.sys+"/devices/system/cpu/"+cpu+"/cpuidle/"+st;
                row(idle,cpu+" / "+text(p+"/name"),"entries "+text(p+"/usage")+"; residency "+text(p+"/time")+" µs; exit latency "+text(p+"/latency")+" µs; disabled "+text(p+"/disable"));
            }
        }section(sections,"CPU idle states",idle);
        QJsonArray sched;fileRow(sched,"Scheduler statistics",r.proc+"/sys/kernel/sched_schedstats");fileRow(sched,"Task delay accounting",r.proc+"/sys/kernel/task_delayacct");section(sections,"Scheduler accounting",sched);
    }
    if(all||domain=="gpu")for(const auto &n:QDir(r.sys+"/class/devfreq").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        const auto p=r.sys+"/class/devfreq/"+n;const auto driver=QFileInfo(QFileInfo(p+"/device/driver").canonicalFilePath()).fileName();if(driver!="adreno")continue;
        QJsonArray rows;fileRow(rows,"Runtime state",p+"/device/power/runtime_status");fileRow(rows,"Active time (ms)",p+"/device/power/runtime_active_time");fileRow(rows,"Suspended time (ms)",p+"/device/power/runtime_suspended_time");
        const auto stat=read(p+"/trans_stat");row(rows,"Frequency transition table",stat.text,stat.status);row(rows,"Load measurement","Frequency alone does not measure GPU utilization. Client counters appear only when DRM fdinfo exports them.");section(sections,"GPU runtime and residency",rows);
    }
    if(domain=="gpu") {
        QJsonArray clients;QSet<QString> seen;int scanned=0,processes=0;
        for(const auto &pid:QDir(r.proc).entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            if(!QRegularExpression("^[0-9]+$").match(pid).hasMatch()||QFileInfo(r.proc+"/"+pid).ownerId()!=getuid())continue;
            if(++processes>128||scanned>=2048)break;
            const auto base=r.proc+"/"+pid;int count=0;
            for(const auto &fd:QDir(base+"/fd").entryList(QDir::Files|QDir::System|QDir::NoDotAndDotDot)) {
                if(++count>128||++scanned>2048||clients.size()>=64)break;
                if(!QFileInfo(base+"/fd/"+fd).symLinkTarget().startsWith("/dev/dri/"))continue;
                const auto lines=read(base+"/fdinfo/"+fd,16384).text.split('\n');QMap<QString,QString> data;
                for(const auto &line:lines){const auto at=line.indexOf(':');if(at>0)data[line.left(at)]=line.mid(at+1).trimmed();}
                if(!data.contains("drm-client-id"))continue;const auto client=pid+"/"+data.value("drm-client-id");if(seen.contains(client))continue;seen.insert(client);
                const auto label=text(base+"/comm").left(80)+" ("+pid+")";QStringList values;
                for(auto i=data.begin();i!=data.end();++i) {
                    if(i.key().startsWith("drm-memory-")||i.key().startsWith("drm-resident-"))values<<i.key()+": "+i.value();
                    if(i.key().startsWith("drm-engine-")&&i.value().endsWith(" ns")) {
                        const auto key="drm/"+client+"/"+i.key();counter(counters,key,numeric(i.value().section(' ',0,0)),.0000001);
                        chart(key,label+" / "+i.key(),"% of one engine");values<<i.key()+": "+i.value();
                    }
                }
                if(!values.isEmpty())row(clients,label,values.join("\n"));
            }
        }
        if(clients.isEmpty())row(clients,"Application GPU counters","No readable DRM client counters for this user's running processes","not-exported");
        section(sections,"Application GPU activity",clients);
    }
    if(all||domain=="thermal") {
        QMap<QString,QJsonArray> groups;QSet<QString> seen,thermalProviders;QJsonArray thresholds;
        for(const auto &n:QDir(r.sys+"/class/thermal").entryList({"thermal_zone*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            const auto p=r.sys+"/class/thermal/"+n,type=text(p+"/type"),g=group(type);if(seen.contains(type))continue;seen.insert(type);
            thermalProviders.insert(QFileInfo(p).canonicalFilePath());
            const auto v=read(p+"/temp");const auto degrees=signedNumber(v.text,1000,-273.15,300);
            row(groups[g],type,degrees.isDouble()?QString::number(degrees.toDouble(),'f',1)+" °C":QString(),v.status);
            if(degrees.isDouble()) {const auto key="thermal/"+g;const auto old=metrics.value(key);if(!old.isDouble()||degrees.toDouble()>old.toDouble())metrics[key]=degrees;}
            for(const auto &trip:QDir(p).entryList({"trip_point_*_temp"},QDir::Files)) {const auto value=signedNumber(text(p+"/"+trip),1000,-273.15,300);if(value.isDouble())row(thresholds,type+" / "+text(p+"/"+QString(trip).replace("_temp","_type")),QString::number(value.toDouble(),'f',1)+" °C (informational)");}
        }
        for(const auto &n:QDir(r.sys+"/class/hwmon").entryList({"hwmon*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
            const auto p=r.sys+"/class/hwmon/"+n;
            if(thermalProviders.contains(QFileInfo(p+"/device").canonicalFilePath()))continue;
            for(const auto &input:QDir(p).entryList({"temp*_input"},QDir::Files)) {
                const auto label=text(p+"/"+QString(input).replace("_input","_label"));const auto provider=text(p+"/name");
                if(seen.contains(label)||seen.contains(provider))continue;
                const auto name=label.isEmpty()?provider+" / "+input:label;const auto v=read(p+"/"+input);const auto d=signedNumber(v.text,1000,-273.15,300);
                row(groups[group(name)],name,d.isDouble()?QString::number(d.toDouble(),'f',1)+" °C":QString(),v.status);
                seen.insert(name);if(d.isDouble()){const auto key="thermal/"+group(name);const auto old=metrics.value(key);if(!old.isDouble()||d.toDouble()>old.toDouble())metrics[key]=d;}
            }
        }
        for(auto it=groups.begin();it!=groups.end();++it){section(sections,it.key(),it.value());chart("thermal/"+it.key(),it.key()+" maximum","°C");}
        section(sections,"Kernel thermal thresholds",thresholds);
        QJsonArray cooling;
        for(const auto &n:QDir(r.sys+"/class/thermal").entryList({"cooling_device*"},QDir::Dirs|QDir::NoDotAndDotDot)) {const auto p=r.sys+"/class/thermal/"+n;row(cooling,text(p+"/type"),"state "+text(p+"/cur_state")+" / "+text(p+"/max_state")+" (policy levels, not a performance percentage)");}
        section(sections,"Cooling policy",cooling);QJsonArray sleep;
        fileRow(sleep,"Advertised suspend modes",r.sys+"/power/mem_sleep");row(sleep,"Qualification","An advertised mode does not prove physical suspend or wake reliability.");
        for(const auto &f:{"success","fail","last_failed_dev","last_failed_errno","last_failed_step","failed_freeze","failed_suspend","failed_resume"})fileRow(sleep,f,r.sys+"/power/suspend_stats/"+f);
        fileRow(sleep,"Last wake IRQ",r.sys+"/power/pm_wakeup_irq");fileRow(sleep,"Wakeup event count",r.sys+"/power/wakeup_count");section(sections,"Suspend and wake",sleep);
        for(const auto &n:QDir(r.sys+"/class/power_supply").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            const auto p=r.sys+"/class/power_supply/"+n;if(text(p+"/type")!="Battery")continue;
            QJsonArray b;for(const auto &key:{"status","power_now","current_now","voltage_now","temp","charge_control_start_threshold","charge_control_end_threshold"})fileRow(b,key,p+"/"+key);section(sections,"Battery power",b);
            metrics["battery/power"]=signedNumber(text(p+"/power_now"),1000000,-200,200);chart("battery/power","Battery reported power","W");
            metrics["battery/current"]=signedNumber(text(p+"/current_now"),1000,-30000,30000);chart("battery/current","Battery reported current","mA");
        }
    }
    if(all||domain=="profiles") {
        const auto config=kernelConfig(r);QJsonArray cap;
        const QList<QPair<QString,QString>> features{{"CONFIG_UCLAMP_TASK","Task utilization clamps"},{"CONFIG_UCLAMP_TASK_GROUP","Cgroup utilization clamps"},{"CONFIG_CFS_BANDWIDTH","CPU quotas"},{"CONFIG_ZRAM","Compressed RAM swap"},{"CONFIG_LRU_GEN","Multi-generation LRU"},{"CONFIG_DAMON","Data access monitoring"},{"CONFIG_BLK_DEV_THROTTLING","Per-cgroup I/O limits"},{"CONFIG_SCHEDSTATS","Scheduler statistics"},{"CONFIG_TASK_DELAY_ACCT","Task delay accounting"},{"CONFIG_THERMAL_STATISTICS","Cooling statistics"},{"CONFIG_CPU_IDLE_GOV_TEO","TEO idle governor"},{"CONFIG_SCHED_CLASS_EXT","sched_ext"},{"CONFIG_DEBUG_INFO_BTF","BPF type information"}};
        for(const auto &f:features){const auto v=config.value(f.first).toString();row(cap,f.second,v=="y"?"Compiled into this kernel":v=="m"?"Compiled as a module; runtime provider must also be present":config.isEmpty()?"Kernel configuration could not be read":"Not compiled into this kernel",v=="y"?"available":v=="m"?"module":"kernel-support-missing");}
        section(sections,"Kernel capabilities",cap);
        QJsonArray cg;fileRow(cg,"Cgroup controllers",r.sys+"/fs/cgroup/cgroup.controllers");row(cg,"Profile scope","Only applications owned by this desktop user; global GPU limits affect every application.");section(sections,"Application resource management",cg);
        if(all){QJsonArray scx;fileRow(scx,"External scheduler state",r.sys+"/kernel/sched_ext/state");row(scx,"Admission","An external scheduler requires a qualified BPF program and explicit activation. No scheduler is started automatically.");section(sections,"Experimental scheduler",scx);}
    }
    for(const auto &id:ids){auto s=resourceSetting(id,r);if(!s.isEmpty())controls<<s;}
    return {{"sections",sections},{"metrics",metrics},{"counters",counters},{"charts",charts},{"controls",controls}};
}
}
