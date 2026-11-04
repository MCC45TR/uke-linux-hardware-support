// SPDX-License-Identifier: MIT
#include "nfc-reader.hpp"
#include "model-data.hpp"
#include <QNdefNfcTextRecord>
#include <QNdefNfcUriRecord>
#include <KLocalizedString>
namespace uq {
QVariantList ndefRows(const QNdefMessage &message) {
    QVariantList rows;int bytes=0;
    for(const auto &record:message) {
        bytes+=record.payload().size();
        if(rows.size()>=16||bytes>16384||record.payload().size()>4096) {
            rows<<QVariantMap{{"title",i18n("Read limit")},{"value",i18n("Additional or oversized records were omitted")}};break;
        }
        QString text,title;
        if(record.isRecordType<QNdefNfcTextRecord>()) {
            const auto payload=record.payload();
            if(payload.isEmpty()||1+(quint8(payload[0])&63)>payload.size())continue;
            title=i18n("Text");text=QNdefNfcTextRecord(record).text();
        } else if(record.isRecordType<QNdefNfcUriRecord>()) {
            title=i18n("Address (display only)");text=QNdefNfcUriRecord(record).uri().toString();
        } else {title=i18n("Other NDEF record");text=i18n("Type %1 · %2 bytes",safe(record.type().left(64).toStdString()),record.payload().size());}
        rows<<QVariantMap{{"title",title},{"value",safe(text.left(2048).toStdString())}};
    }
    return rows;
}
NfcReader::NfcReader(bool simulation,QObject *parent,bool restricted):QObject(parent),simulation_(simulation),restricted_(restricted) {
    timeout_.setSingleShot(true);timeout_.setInterval(30000);
    connect(&timeout_,&QTimer::timeout,this,[this]{complete(i18n("Reading timed out. Bring a supported tag closer and try again."));});
    report_={{"busy",false},{"supported",false},{"message",i18n("Check for a connected NFC reader before scanning.")}};
}
NfcReader::~NfcReader(){release();if(service_){service_->disconnect(this);service_->kill();service_->waitForFinished(1000);}}
void NfcReader::release(){
    scanning_=false;timeout_.stop();
    if(target_){QObject::disconnect(target_,nullptr,this,nullptr);target_->disconnect();target_->deleteLater();target_=nullptr;}
    if(manager_)manager_->stopTargetDetection();
}
void NfcReader::complete(const QString &message){release();report_["busy"]=false;report_["message"]=message;emit changed();}
void NfcReader::inspect(){
    if(simulation_){report_["message"]=i18n("Simulation does not access NFC hardware.");emit changed();return;}
    if(scanning_||service_)return;
    if(manager_){manager_->deleteLater();manager_=nullptr;}
    if(!manager_) {
        manager_=new QNearFieldManager(this);
        connect(manager_,&QNearFieldManager::targetDetected,this,[this](QNearFieldTarget *target){
            if(!scanning_||target_){target->deleteLater();return;}
            target_=target;
            report_["tagType"]=int(target->type());
            if(!(target->accessMethods()&QNearFieldTarget::NdefAccess)){complete(i18n("A tag was detected, but this reader cannot read its NDEF records."));return;}
            connect(target,&QNearFieldTarget::ndefMessageRead,this,[this](const QNdefMessage &message){
                report_["records"]=ndefRows(message);complete(message.isEmpty()?i18n("The tag returned an empty NDEF message."):i18n("NDEF records read. Content is displayed locally and is not opened automatically."));
            });
            connect(target,&QNearFieldTarget::error,this,[this](QNearFieldTarget::Error,const QNearFieldTarget::RequestId &){complete(i18n("The reader could not read this tag. Check its format and position."));});
            report_["message"]=i18n("Tag detected; reading NDEF records…");emit changed();
            if(!target->readNdefMessages().isValid())complete(i18n("The reader refused the NDEF read request."));
        });
        connect(manager_,&QNearFieldManager::targetLost,this,[this](QNearFieldTarget *target){if(scanning_&&target_==target)complete(i18n("The tag moved out of range before the read finished."));});
    }
    const bool supported=manager_->isSupported(QNearFieldTarget::NdefAccess);
    report_["supported"]=supported;report_["busy"]=false;
    report_["message"]=supported?i18n("A compatible NFC reader is available."):i18n("No compatible NFC reader is available. The built-in FM19511 is a tag, not a reader. Connect a supported USB NFC reader for NDEF reading.");emit changed();
}
void NfcReader::startService(){
    if(simulation_||restricted_||scanning_||service_)return;
    service_=new QProcess(this);auto *p=service_;
    auto done=[this,p](bool success){
        if(service_!=p)return;service_=nullptr;p->deleteLater();report_["busy"]=false;report_["startingService"]=false;
        if(success)inspect();else {report_["message"]=i18n("The NFC reader service could not be started. Check administrator authorization and the neard service.");emit changed();}
    };
    connect(p,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit&&code==0);});
    connect(p,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(false);});
    report_["busy"]=true;report_["startingService"]=true;report_["message"]=i18n("Starting the distribution NFC reader service…");emit changed();
    p->start("/usr/bin/systemctl",{"start","neard.service"});
    QTimer::singleShot(30000,p,[p]{if(p->state()!=QProcess::NotRunning)p->kill();});
}
void NfcReader::start(){
    if(scanning_)return;inspect();if(simulation_||!report_.value("supported").toBool())return;
    report_.remove("records");report_.remove("tagType");scanning_=true;
    if(!manager_->startTargetDetection(QNearFieldTarget::NdefAccess)){complete(i18n("Could not start NFC detection. Check the reader service and access permissions."));return;}
    report_["busy"]=true;report_["message"]=i18n("Hold an NDEF tag near the connected reader. Reading stops after 30 seconds.");timeout_.start();emit changed();
}
void NfcReader::stop(){complete(i18n("NFC reading stopped."));}
void NfcReader::clear(){release();report_.remove("records");report_.remove("tagType");report_["busy"]=false;report_["message"]=i18n("Read content cleared.");emit changed();}
}
