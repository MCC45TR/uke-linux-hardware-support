// SPDX-License-Identifier: GPL-3.0-or-later
// Persistent authenticated IPC only; no device, sysfs or privileged operations.
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QQmlExtensionPlugin>
#include <QtQml/qqml.h>
#include <QUuid>
#include <QVariantList>
#include <QMap>
#include <memory>
#include <cmath>

class LightClient : public QObject {
    Q_OBJECT
    std::unique_ptr<QDBusConnection> bus;
    QString connectionName;
    quint64 generation=0;
    QMap<QDBusPendingCallWatcher*,QString> pending;
public:
    explicit LightClient(QObject* parent=nullptr):QObject(parent){}
    ~LightClient() override {close();}
    Q_INVOKABLE void close() {
        ++generation;
        const auto cancelled=pending;pending.clear();
        for(auto* watcher:cancelled.keys()){watcher->disconnect(this);delete watcher;}
        if(bus){QDBusConnection::disconnectFromBus(connectionName);bus.reset();}
        for(const auto& action:cancelled)emit completed(action,{},QStringLiteral("Request cancelled"));
    }
    Q_INVOKABLE bool request(const QString& action,const QVariantList& values={}) {
        if(pending.size()>=8)return false;
        const bool rgb=action=="rgb" || action=="rgbTest";
        const bool query=action=="rgb" || action=="lights";
        QString method;
        QList<QVariant> arguments;
        if(query)method="Status";
        else if(action=="rgbTest")method="TestColors";
        else if(action=="flashlightOn" || action=="flashlightOff") {
            method="SetEnabled";arguments.append(action=="flashlightOn");
        } else if(action=="flashlightSet") {
            if(values.size()!=1)return false;
            const int type=values.front().metaType().id();
            if(type!=QMetaType::Double && type!=QMetaType::Int && type!=QMetaType::UInt)return false;
            bool ok=false;const double value=values.front().toDouble(&ok);
            if(!ok || !std::isfinite(value) || value<1 || value>100 || double(unsigned(value))!=value)return false;
            method="SetBrightness";arguments.append(QVariant::fromValue(unsigned(value)));
        } else return false;
        if(action!="flashlightSet" && !values.isEmpty())return false;
        if(!bus) {
            connectionName="uke-lights-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
            bus=std::make_unique<QDBusConnection>(QDBusConnection::connectToBus(QDBusConnection::SystemBus,connectionName));
        }
        if(!bus->isConnected()){close();return false;}
        auto message=QDBusMessage::createMethodCall(rgb?"org.senemos.Uke.Rgb":"org.senemos.Uke.Torch",
            rgb?"/org/senemos/Uke/Rgb":"/org/senemos/Uke/Torch",
            rgb?"org.senemos.Uke.Rgb1":"org.senemos.Uke.Torch1",method);
        message.setArguments(arguments);message.setInteractiveAuthorizationAllowed(!query);
        const auto requestGeneration=generation;
        auto* watcher=new QDBusPendingCallWatcher(bus->asyncCall(message,query?3000:130000),this);
        pending.insert(watcher,action);
        connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,action,requestGeneration](QDBusPendingCallWatcher* finished){
            if(requestGeneration!=generation)return;
            pending.remove(finished);
            const QDBusPendingReply<QString> reply=*finished;
            finished->deleteLater();
            emit completed(action,reply.isError()?QString():reply.value(),reply.isError()?reply.error().message():QString());
        });
        return true;
    }
signals:
    void completed(const QString& action,const QString& output,const QString& error);
};

class UkeLightPlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char* uri) override {qmlRegisterType<LightClient>(uri,1,0,"LightClient");}
};
#include "light-client.moc"
