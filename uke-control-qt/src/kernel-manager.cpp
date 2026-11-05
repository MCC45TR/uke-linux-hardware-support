// SPDX-License-Identifier: MIT
#include "kernel-manager.hpp"
#include "frequency-source.hpp"
#include <QtConcurrent>
#include <KLocalizedString>
#include <QGuiApplication>
#include <cmath>
#include <algorithm>
#include <limits>
namespace uq {
namespace {
QVariant number(const QJsonValue &v,double scale=1) {
    bool ok=false;const auto n=v.isDouble()?v.toDouble():v.toString().toDouble(&ok);
    if(v.isDouble())ok=true;return ok&&std::isfinite(n)&&n>=0?QVariant(n/scale):QVariant();
}
QString reason(const QString &error) {
    const QMap<QString,QString> reasons{{"settings_changed",i18n("Settings changed elsewhere. Refresh and review the request again.")},
      {"minimum_exceeds_maximum",i18n("The minimum frequency must not exceed the maximum.")},
      {"frequency_not_advertised",i18n("Choose a frequency advertised by this driver.")},
      {"governor_not_supported",i18n("Choose a supported automatic governor.")},
      {"outside_hardware_limits",i18n("The requested frequency is outside the hardware limits.")},
      {"readback_differs",i18n("The driver returned a different value. The kernel may have limited the request; refresh to inspect it.")},
      {"write_failed",i18n("The driver refused the change. Current settings remain visible.")}};
    return reasons.value(error,i18n("This provider cannot accept that request. Refresh its current state."));
}
QVariantMap configuration(const QJsonObject &d){auto copy=d;copy.remove("frequency");return copy.toVariantMap();}
}
QVariantMap cpuLoads(const QJsonObject &before,const QJsonObject &after) {
    QVariantMap out;
    for(auto it=after.begin();it!=after.end();++it) {
        const auto a=before.value(it.key()).toArray(),b=it.value().toArray();
        if(a.size()!=8||b.size()!=8){out[it.key()]=QVariant();continue;}
        double total=0,idle=0;bool valid=true;
        for(int i=0;i<8;++i){const auto delta=b[i].toDouble()-a[i].toDouble();if(!a[i].isDouble()||!b[i].isDouble()||delta<0||!std::isfinite(delta)){valid=false;break;}total+=delta;if(i==3||i==4)idle+=delta;}
        out[it.key()]=valid&&total>0?QVariant(100*(total-idle)/total):QVariant();
    }
    return out;
}
KernelManager::KernelManager(Fixture f,QObject *parent,bool restricted):QObject(parent),fixture_(std::move(f)),restricted_(restricted) {
    clock_.start();timer_.setInterval(2000);deadline_.setSingleShot(true);queryPoll_.setInterval(40);
    connect(&timer_,&QTimer::timeout,this,&KernelManager::refresh);
    connect(&deadline_,&QTimer::timeout,this,[this]{pause();message_=i18n("Monitoring paused after ten minutes. Start again to continue.");emit changed();});
    connect(&worker_,&QFutureWatcher<QJsonObject>::finished,this,[this]{if(workerGeneration_==generation_)accept(worker_.result());if(requested_)QTimer::singleShot(0,this,&KernelManager::refresh);emit changed();});
    connect(&queryPoll_,&QTimer::timeout,this,[this]{
        if(!query_)return;query_->poll();const auto r=query_->result();if(r.running)return;
        queryPoll_.stop();query_.reset();
        if(r.exit_status||r.cancelled||r.timed_out||r.truncated){failure(i18n("Sensor reading failed. Previous values may be stale."));return;}
        QJsonParseError e;const auto doc=QJsonDocument::fromJson(QByteArray::fromStdString(r.output),&e);
        if(e.error!=QJsonParseError::NoError||!doc.isObject()){failure(i18n("Sensor reading failed. Previous values may be stale."));return;}acceptSensors(doc.object());
    });
    connect(qGuiApp,&QGuiApplication::applicationStateChanged,this,[this](Qt::ApplicationState state){if(state!=Qt::ApplicationActive&&active_){pause();message_=i18n("Monitoring paused while the application is inactive.");emit changed();}});
}
KernelManager::~KernelManager(){pause();if(write_){write_->disconnect(this);write_->kill();connect(write_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),write_,&QObject::deleteLater);}}
void KernelManager::open(const QString &mode) {
    if(mode!="cpu"&&mode!="gpu"&&mode!="sensors")return;
    if(mode_!=mode){pause();++generation_;dismissReview();mode_=mode;outcome_.clear();message_.clear();history_.clear();previousTicks_={};state_={};devices_.clear();sensors_.clear();selectedSensor_.clear();interval_=mode=="sensors"?5:2;emit devicesChanged();}
    requested_=true;
    if(mode=="sensors"){interval_=5;timer_.setInterval(5000);refresh();}else start();
}
void KernelManager::start(){if(mode_.isEmpty()||write_)return;active_=true;timer_.start(interval_*1000);deadline_.start(600000);refresh();emit changed();}
void KernelManager::pause(){active_=false;requested_=false;timer_.stop();deadline_.stop();queryPoll_.stop();if(query_){query_->cancel();query_.reset();}for(auto it=history_.begin();it!=history_.end();++it)add(it.key(),QVariant());emit changed();}
void KernelManager::close(){pause();++generation_;mode_.clear();dismissReview();}
void KernelManager::setInterval(int seconds){if(seconds!=1&&seconds!=2&&seconds!=5)return;interval_=seconds;if(active_)timer_.start(seconds*1000);emit changed();}
void KernelManager::refresh() {
    if(mode_.isEmpty()||busy())return;
    requested_=false;
    if(fixture_.enabled) {
        const auto d=fixture_.results.value(mode_=="sensors"?"sensors.once":"manager.sample").toObject().value("data").toObject();
        if(mode_=="sensors")acceptSensors(d);else accept(d);return;
    }
    if(mode_=="sensors") {try{query_=std::make_unique<Query>("sensors.once");queryPoll_.start();}catch(const std::exception &){failure(i18n("The sensor provider could not be started."));}emit changed();return;}
    workerGeneration_=generation_;worker_.setFuture(QtConcurrent::run([]{return performanceSnapshot();}));emit changed();
}
void KernelManager::add(const QString &id,const QVariant &value){auto &v=history_[id];const double time=clock_.elapsed()/1000.;if(!v.isEmpty()&&time<=v.last().time)v.last().value=value;else v.append({time,value});if(v.size()>600)v.remove(0,v.size()-600);}
void KernelManager::failure(const QString &message){message_=message;for(auto it=history_.begin();it!=history_.end();++it)add(it.key(),QVariant());++revision_;emit changed();}
void KernelManager::accept(const QJsonObject &snapshot) {
    const auto loads=cpuLoads(previousTicks_,snapshot.value("ticks").toObject());previousTicks_=snapshot.value("ticks").toObject();
    QVariantList metadata,cores;QVariantMap values;
    for(auto it=loads.begin();it!=loads.end();++it)cores<<QVariantMap{{"id",it.key()},{"load",it.value()}};
    for(const auto &v:snapshot.value("devices").toArray()) {
        const auto d=v.toObject();const auto id=d.value("id").toString();if(id.isEmpty())continue;
        metadata<<configuration(d);auto frequency=number(d.value("frequency"),d.value("kind").toString()=="cpu"?1000:1000000);
        QVariant load;double sum=0;int count=0;
        for(const auto &cpu:d.value("cpus").toString().split(' ',Qt::SkipEmptyParts)){const auto n=loads.value("cpu"+cpu);if(n.isValid()){sum+=n.toDouble();++count;}}
        if(count)load=sum/count;
        values[id]=QVariantMap{{"frequency",frequency},{"load",load}};add(id+"/frequency",frequency);if(d.value("kind").toString()=="cpu")add(id+"/load",load);
    }
    QVariant cpuTemp,gpuTemp;
    for(const auto &v:snapshot.value("temperatures").toArray()) {
        const auto t=v.toObject();if(!t.value("value").isDouble())continue;const auto label=t.value("label").toString();const auto n=t.value("value").toDouble();
        if(label.startsWith("cpu")&&(!cpuTemp.isValid()||n>cpuTemp.toDouble()))cpuTemp=n;
        if(label.startsWith("gpu")&&(!gpuTemp.isValid()||n>gpuTemp.toDouble()))gpuTemp=n;
    }
    add("cpuTemperature",cpuTemp);add("gpuTemperature",gpuTemp);
    state_={{"values",values},{"cores",cores},{"cpuTemperature",cpuTemp},{"gpuTemperature",gpuTemp}};
    if(metadata!=devices_){devices_=metadata;emit devicesChanged();}
    message_=metadata.isEmpty()?i18n("No CPU/GPU frequency provider is exported by this kernel."):QString();++revision_;emit changed();
}
void KernelManager::acceptSensors(const QJsonObject &snapshot) {
    QVariantList catalog;QVariantMap values;int count=0;
    for(const auto &v:snapshot.value("readings").toArray()) {
        if(++count>512)break;const auto d=v.toObject();const auto id=d.value("source").toString();if(id.isEmpty()||id.size()>256)continue;
        const auto status=d.value("status").toString();
        const bool valid=!status.startsWith("unavailable")&&d.value("value").isDouble()&&std::isfinite(d.value("value").toDouble());
        QString group=id.startsWith("SensorProxy/")?"desktop":id.startsWith("bus/iio/")?"iio":id.startsWith("class/thermal/")?"thermal":id.startsWith("class/hwmon/")?"hwmon":"other";
        const QMap<QString,QString> labels{{"HasAccelerometer",i18n("Accelerometer available")},{"AccelerometerOrientation",i18n("Orientation")},{"AccelerometerTilt",i18n("Tilt")},{"HasAmbientLight",i18n("Ambient light available")},{"LightLevel",i18n("Ambient light")},{"HasProximity",i18n("Proximity sensor available")},{"ProximityNear",i18n("Object nearby")},{"HasCompass",i18n("Compass available")},{"CompassHeading",i18n("Compass heading")}};
        const auto label=d.value("label").toString();
        catalog<<QVariantMap{{"id",id},{"title",labels.value(label,safe(label.left(160).toStdString()))},{"unit",safe(d.value("unit").toString().left(40).toStdString())},{"group",group}};
        QString statusText=status.startsWith("cached")?i18n("Cached value; sample age unknown"):status.startsWith("read")?i18n("Read from the kernel; sample age unknown"):status.startsWith("unavailable")?i18n("Unavailable; check the provider and access permissions"):safe(status.left(200).toStdString());
        auto text=safe(d.value("text").toString().left(256).toStdString());
        if(text=="true")text=i18n("Yes");else if(text=="false")text=i18n("No");else if(text=="normal")text=i18n("Normal");
        values[id]=QVariantMap{{"value",valid?d.value("value").toVariant():QVariant()},{"text",text},{"status",statusText}};
    }
    if(catalog!=sensors_){sensors_=catalog;emit devicesChanged();}
    state_={{"sensorValues",values}};
    if(!selectedSensor_.isEmpty())add("sensor:"+selectedSensor_,values.value(selectedSensor_).toMap().value("value"));
    message_=catalog.isEmpty()?i18n("No sensor readings were returned by the provider."):QString();++revision_;emit changed();
}
void KernelManager::selectSensor(const QString &id){if(selectedSensor_==id)return;history_.remove("sensor:"+selectedSensor_);selectedSensor_=id;++revision_;emit changed();}
QString KernelManager::format(const QVariant &value,const QString &unit)const{bool ok=false;const auto n=value.toDouble(&ok);return value.isValid()&&ok&&std::isfinite(n)?QLocale().toString(n,'f',unit=="MHz"?1:unit=="%"?0:2)+(unit.isEmpty()?QString():" "+unit):i18n("Not reported");}
QString KernelManager::current(const QString &id,const QString &field)const{for(const auto &v:devices_){const auto d=v.toMap();if(d.value("id").toString()==id)return d.value(field).toString();}return {};}
QVariantList KernelManager::choices(const QString &id,const QString &field)const {
    QVariantList out;for(const auto &v:devices_) {const auto d=v.toMap();if(d.value("id").toString()!=id)continue;
        const bool cpu=d.value("kind").toString()=="cpu";const auto list=d.value(field=="governor"?"governors":"frequencies").toList();
        for(const auto &n:list){const auto s=n.toString();if(field=="governor"&&s=="userspace")continue;out<<QVariantMap{{"title",field=="governor"?s:format(s.toDouble()/(cpu?1000:1000000),"MHz")},{"value",s}};}
    }return out;
}
QVariantMap KernelManager::plot(const QString &metric,int seconds,double maximum)const {
    seconds=std::clamp(seconds,30,600);const auto end=clock_.elapsed()/1000.,start=std::max(0.,end-seconds);
    QVariantList segments,segment;double minimum=std::numeric_limits<double>::max(),high=-minimum,previous=-1;int points=0;
    auto flush=[&]{if(!segment.isEmpty())segments<<QVariant(segment);segment.clear();};
    for(const auto &p:history_.value(metric)) {
        if(p.time<start)continue;
        if(!p.value.isValid()||!std::isfinite(p.value.toDouble())){flush();previous=p.time;continue;}
        if(previous>=0&&p.time-previous>std::max(3.,interval_*2.5))flush();previous=p.time;
        const auto n=p.value.toDouble();minimum=std::min(minimum,n);high=std::max(high,n);++points;
        segment<<QVariantMap{{"time",p.time},{"value",n}};
    }
    flush();if(!points){minimum=0;high=1;}
    if(maximum>0){minimum=0;high=maximum;}else{const auto pad=std::max(1.,(high-minimum)*.15);minimum-=pad;high+=pad;}
    return {{"segments",segments},{"start",start},{"end",std::max(start+1.,end)},{"minimum",minimum},{"maximum",high},{"points",points},{"available",points>=2}};
}
void KernelManager::dismissReview(){review_.clear();emit reviewChanged();}
bool KernelManager::prepare(const QString &id,const QString &field,const QString &value) {
    if(write_)return false;outcome_.clear();QVariantMap d;
    for(const auto &v:devices_)if(v.toMap().value("id").toString()==id)d=v.toMap();
    const auto error=frequencyRefusal(QJsonObject::fromVariantMap(d),field,value);
    if(!error.isEmpty()){message_=reason(error);emit changed();return false;}
    const auto display=[&](const QString &v){return field=="governor"?v:v=="0"?i18n("Driver default"):format(v.toDouble()/(d.value("kind").toString()=="cpu"?1000:1000000),"MHz");};
    review_={{"id",id},{"kind",d.value("kind")},{"name",d.value("name")},{"field",field},{"value",value},{"before",display(d.value(field).toString())},{"after",display(value)},
       {"minimum",d.value("minimum")},{"maximum",d.value("maximum")},{"governor",d.value("governor")},
       {"token",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"blocked",fixture_.enabled||restricted_}};emit reviewChanged();return true;
}
bool KernelManager::apply(const QString &token) {
    if(write_||review_.isEmpty()||token!=review_.value("token").toString()||restricted_||fixture_.enabled)return false;
    for(const auto &key:{"minimum","maximum","governor"})if(current(review_.value("id").toString(),key)!=review_.value(key).toString()){message_=reason("settings_changed");dismissReview();emit changed();return false;}
    pause();auto *p=new QProcess(this);write_=p;writeOutput_.clear();
    connect(p,&QProcess::readyReadStandardOutput,this,[this,p]{writeOutput_+=p->readAllStandardOutput();if(writeOutput_.size()>32768)p->kill();});
    const auto requestMode=mode_;
    auto done=[this,p,requestMode](int code){
        if(write_!=p)return;writeOutput_+=p->readAllStandardOutput();const auto d=QJsonDocument::fromJson(writeOutput_.left(32768)).object();
        write_=nullptr;p->deleteLater();dismissReview();
        if(mode_==requestMode)outcome_=code==0&&d.value("schema").toString()=="org.uke.qt.frequency-control.v1"?i18n("The driver confirmed the setting. It is not saved for the next boot."):d.value("error").isString()&&!d.value("error").toString().isEmpty()?reason(d.value("error").toString()):i18n("The request was not completed. Check authorization and refresh current settings.");
        refresh();emit changed();
    };
    connect(p,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit?code:70);});
    connect(p,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(69);});
    QStringList args{"/usr/libexec/uke-control-qt/frequency-control",review_.value("kind").toString(),review_.value("name").toString(),review_.value("field").toString(),review_.value("minimum").toString(),review_.value("maximum").toString(),review_.value("governor").toString(),review_.value("value").toString()};
    p->start("/usr/bin/pkexec",args);QTimer::singleShot(60000,p,[p]{if(p->state()!=QProcess::NotRunning)p->kill();});emit changed();return true;
}
}
