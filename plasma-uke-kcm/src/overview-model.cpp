// SPDX-License-Identifier: MIT
#include "overview-model.hpp"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <memory>

namespace {
const QStringList domains{QStringLiteral("battery"),QStringLiteral("charge"),QStringLiteral("display"),QStringLiteral("usb"),QStringLiteral("desktop"),QStringLiteral("hardware"),QStringLiteral("sensors"),QStringLiteral("audio"),QStringLiteral("camera"),QStringLiteral("nfc"),QStringLiteral("sleep"),QStringLiteral("storage"),QStringLiteral("updates"),QStringLiteral("recovery"),QStringLiteral("workload")};
constexpr qsizetype maximum=1024*1024;
QVariantList metrics(const QString& domain,const QJsonObject& data) {
    QVariantList rows;
    const auto add=[&](const QString& id,QJsonValue value,QString unit=QString(),QString validity=QString()) {
        if(value.isDouble()&&unit==QStringLiteral("uV")){value=value.toDouble()/1000000;unit=QStringLiteral("V");}
        if(value.isDouble()&&unit==QStringLiteral("uA_positive_into_battery")){value=value.toDouble()/1000000;unit=QStringLiteral("A");}
        if(value.isDouble()&&unit==QStringLiteral("deciC")){value=value.toDouble()/10;unit=QStringLiteral("°C");}
        if(unit==QStringLiteral("cycles_semantics_unqualified"))unit=QStringLiteral("cycles");
        if(unit==QStringLiteral("percent"))unit=QStringLiteral("%");
        rows.append(QVariantMap{{QStringLiteral("id"),id},{QStringLiteral("known"),!value.isUndefined()&&!value.isNull()&&!value.isObject()&&!value.isArray()},
            {QStringLiteral("value"),value.toVariant()},{QStringLiteral("unit"),unit},{QStringLiteral("validity"),validity}});
    };
    const auto field=[&](const QString& id,const QJsonObject& object) {add(id,object.value(QStringLiteral("value")),object.value(QStringLiteral("unit")).toString(),object.value(QStringLiteral("validity")).toString());};
    const auto count=[&](const QString& id,const QJsonValue& value){add(id,value.isArray()?QJsonValue(value.toArray().size()):QJsonValue());};
    if(domain==QStringLiteral("battery")) {
        const auto readings=data.value(QStringLiteral("readings")).toObject();
        for(const auto* name:{"battery.status","battery.capacity","battery.voltage_now","battery.current_now","battery.temp","usb.online"})field(QString::fromLatin1(name),readings.value(QString::fromLatin1(name)).toObject());
        field(QStringLiteral("battery.cycle_count"),data.value(QStringLiteral("battery.cycle_count")).toObject());
        add(QStringLiteral("provider_verified"),data.value(QStringLiteral("provider_verified")));
    } else if(domain==QStringLiteral("charge")) {
        add(QStringLiteral("profile_qualified"),data.value(QStringLiteral("profile_qualified")));
        add(QStringLiteral("automatic_admission"),data.value(QStringLiteral("automatic_admission_ready")));
    } else if(domain==QStringLiteral("display")) {
        const auto session=data.value(QStringLiteral("session")).toObject();
        add(QStringLiteral("rotation"),session.value(QStringLiteral("rotation")));
        add(QStringLiteral("automatic_brightness"),session.value(QStringLiteral("automatic_brightness")));
    } else if(domain==QStringLiteral("usb")) {
        for(const auto& raw:data.value(QStringLiteral("typec")).toArray())add(QStringLiteral("usb_role"),raw.toObject().value(QStringLiteral("data_role")));
        add(QStringLiteral("cdc_policy"),data.value(QStringLiteral("cdc_saved_policy")));
    } else if(domain==QStringLiteral("sensors")) {
        count(QStringLiteral("sensor_channels"),data.value(QStringLiteral("readings")));
    } else if(domain==QStringLiteral("audio")) {
        count(QStringLiteral("alsa_nodes"),data.value(QStringLiteral("devices")).toObject().value(QStringLiteral("alsa")).toObject().value(QStringLiteral("devices")));
        const auto receipt=data.value(QStringLiteral("session")).toObject().value(QStringLiteral("exit_status"));
        add(QStringLiteral("audio_session"),receipt.isDouble()?QJsonValue(receipt.toInt()==0):QJsonValue());
    } else if(domain==QStringLiteral("camera")) {
        count(QStringLiteral("video_nodes"),data.value(QStringLiteral("video")).toObject().value(QStringLiteral("devices")));
        count(QStringLiteral("media_nodes"),data.value(QStringLiteral("media")).toObject().value(QStringLiteral("devices")));
    } else if(domain==QStringLiteral("nfc")) {
        add(QStringLiteral("nfc_available"),data.value(QStringLiteral("available")));add(QStringLiteral("nfc_powered"),data.value(QStringLiteral("powered")));
    } else if(domain==QStringLiteral("sleep")) {
        add(QStringLiteral("sleep_modes"),data.value(QStringLiteral("selected_modes")));
        count(QStringLiteral("wake_sources"),data.value(QStringLiteral("wake_permissions")).toObject().value(QStringLiteral("sources")));
    } else if(domain==QStringLiteral("storage")||domain==QStringLiteral("workload")) {
        add(QStringLiteral("root_capacity_bytes"),data.value(QStringLiteral("root_capacity_bytes")),QStringLiteral("bytes"));
        add(QStringLiteral("root_available_bytes"),data.value(QStringLiteral("root_available_bytes")),QStringLiteral("bytes"));
    } else if(domain==QStringLiteral("updates")) {
        const auto channel=data.value(QStringLiteral("channel_info")).toObject();
        add(QStringLiteral("selected_channel"),channel.value(QStringLiteral("selected")));
        add(QStringLiteral("package_pending"),channel.value(QStringLiteral("pending")));
    } else if(domain==QStringLiteral("recovery")) {
        add(QStringLiteral("layout_pending"),data.value(QStringLiteral("layout_prepared")));
        add(QStringLiteral("resize_pending"),data.value(QStringLiteral("resize_prepared")));
    }
    return rows;
}
}
OverviewModel::OverviewModel(QObject* parent):QObject(parent) {
    for(const auto& domain:domains)cards_.append(QVariantMap{{QStringLiteral("domain"),domain},{QStringLiteral("state"),QStringLiteral("unknown")}});
}
OverviewModel::~OverviewModel(){for(auto* process:running_){process->disconnect(this);process->kill();process->waitForFinished(1000);}}
int OverviewModel::index(const QString& domain) const{return int(domains.indexOf(domain));}
void OverviewModel::update(const QString& domain,const QString& state,const QJsonObject& result) {
    const auto row=index(domain);if(row<0)return;
    auto card=cards_[row].toMap();card[QStringLiteral("state")]=state;
    if(!result.isEmpty()) {
        card[QStringLiteral("snapshot")]=result.toVariantMap();
        card[QStringLiteral("metrics")]=metrics(domain,result.value(QStringLiteral("data")).toObject());
        card[QStringLiteral("details")]=QString::fromUtf8(QJsonDocument(result.value(QStringLiteral("data")).toObject()).toJson(QJsonDocument::Indented));
        card[QStringLiteral("observed")]=QDateTime::currentDateTime().toString(Qt::ISODate);
    }
    cards_[row]=card;Q_EMIT changed();
}
void OverviewModel::refresh() {
    if(fixture_)return;
    for(const auto& domain:domains) {
        const auto state=cards_[index(domain)].toMap().value(QStringLiteral("state")).toString();
        if(state==QStringLiteral("loading")||queue_.contains(domain))continue;
        queue_.append(domain);update(domain,QStringLiteral("loading"));
    }
    pump();
}
void OverviewModel::pump() {
    while(!fixture_&&running_.size()<3&&!queue_.isEmpty()) {
        const auto domain=queue_.takeFirst();auto* process=new QProcess(this);running_.insert(process);
        auto output=std::make_shared<QByteArray>();auto oversized=std::make_shared<bool>(false);
        auto* timer=new QTimer(process);timer->setSingleShot(true);
        connect(timer,&QTimer::timeout,process,[process]{process->setProperty("timed_out",true);process->kill();});
        connect(process,&QProcess::readyReadStandardOutput,this,[process,output,oversized]{
            const auto chunk=process->readAllStandardOutput();
            if(chunk.size()>maximum-output->size()){*oversized=true;process->kill();}else output->append(chunk);
        });
        connect(process,&QProcess::readyReadStandardError,process,[process]{process->readAllStandardError();});
        const auto finish=[this,process,domain,output,oversized] {
            if(process->property("finished").toBool())return;
            process->setProperty("finished",true);
            const auto tail=process->readAllStandardOutput();
            if(tail.size()>maximum-output->size())*oversized=true;else output->append(tail);
            QString state=QStringLiteral("error");QJsonObject result;
            if(!*oversized&&!process->property("timed_out").toBool()&&process->exitStatus()==QProcess::NormalExit&&process->exitCode()==0) {
                QJsonParseError error{};const auto document=QJsonDocument::fromJson(*output,&error);
                const auto root=document.object();const auto list=root.value(QStringLiteral("components")).toArray();
                if(error.error==QJsonParseError::NoError&&root.value(QStringLiteral("schema"))==QStringLiteral("org.uke.control.overview.v1")&&list.size()==1) {
                    result=list[0].toObject();const auto reported=result.value(QStringLiteral("availability")).toString();
                    if(result.value(QStringLiteral("component"))==domain&&QStringList{QStringLiteral("available"),QStringLiteral("unsupported"),QStringLiteral("unknown"),QStringLiteral("denied"),QStringLiteral("not_qualified"),QStringLiteral("error")}.contains(reported))state=reported;
                    else result={};
                }
            }
            auto previous=cards_[index(domain)].toMap();
            if(state==QStringLiteral("error")&&previous.contains(QStringLiteral("snapshot")))state=QStringLiteral("stale");
            update(domain,state,result);running_.remove(process);process->deleteLater();pump();Q_EMIT changed();
        };
        connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[finish](int,QProcess::ExitStatus){finish();});
        connect(process,&QProcess::errorOccurred,this,[finish](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish();});
        process->setProgram(QStringLiteral("/usr/bin/uke-control"));
        process->setArguments({QStringLiteral("--json"),QStringLiteral("overview"),domain});
        process->start();timer->start(15000);
    }
}
bool OverviewModel::setFixture(const QJsonObject& overview) {
    if(busy()||overview.value(QStringLiteral("schema"))!=QStringLiteral("org.uke.control.overview.v1"))return false;
    const auto components=overview.value(QStringLiteral("components"));if(!components.isArray())return false;
    QSet<QString> seen;
    for(const auto& raw:components.toArray()) {
        const auto component=raw.toObject();const auto domain=component.value(QStringLiteral("component")).toString();
        const auto state=component.value(QStringLiteral("availability")).toString();
        if(index(domain)<0||seen.contains(domain)||!QStringList{QStringLiteral("available"),QStringLiteral("unsupported"),QStringLiteral("unknown"),QStringLiteral("denied"),QStringLiteral("not_qualified"),QStringLiteral("stale"),QStringLiteral("error")}.contains(state))return false;
        seen.insert(domain);
    }
    fixture_=true;
    for(const auto& raw:components.toArray()) {
        const auto component=raw.toObject();update(component.value(QStringLiteral("component")).toString(),component.value(QStringLiteral("availability")).toString(),component);
    }
    return true;
}
void OverviewModel::completeFixture(const QString& domain,const QString& state,const QJsonObject& result) {
    if(fixture_)update(domain,state,result);
}
