// SPDX-License-Identifier: GPL-3.0-or-later
// Native client on a private bus; no kernel device or authentication agent.
#include <QCoreApplication>
#include <QDBusContext>
#include <QElapsedTimer>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <cassert>
#include <limits>
#include "../src/light-client.cpp"

class FakeLight : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.senemos.Uke.Torch1")
public:
    QStringList senders;
    QList<bool> interactive;
    QList<unsigned> values;
public slots:
    QString Status(){record();return "status";}
    QString SetBrightness(uint value){record();values.append(value);return "changed";}
    QString SetEnabled(bool value){record();values.append(value?100:0);return "changed";}
private:
    void record(){senders.append(message().service());interactive.append(message().isInteractiveAuthorizationAllowed());}
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    auto provider=QDBusConnection::connectToBus(QDBusConnection::SystemBus,"fake-light-provider");
    assert(provider.isConnected());FakeLight light;
    assert(provider.registerService("org.senemos.Uke.Torch"));
    assert(provider.registerObject("/org/senemos/Uke/Torch",&light,QDBusConnection::ExportAllSlots));
    LightClient client;unsigned completed=0;
    QObject::connect(&client,&LightClient::completed,[&](const QString&,const QString& output,const QString& error){assert(error.isEmpty() && !output.isEmpty());++completed;});
    auto wait=[&](unsigned target){QElapsedTimer timer;timer.start();while(completed<target && timer.elapsed()<3000)QCoreApplication::processEvents();assert(completed==target);};
    assert(!client.request("host") && !client.request("flashlightSet",{QString("50")}));
    for(double value:{0.,101.,1.5,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
        assert(!client.request("flashlightSet",{value}));
    assert(!client.request("flashlightSet",{1,2}) && !client.request("flashlightOn",{1}));
    assert(light.senders.isEmpty());
    assert(client.request("lights"));wait(1);assert(!light.interactive.back());const auto sender=light.senders.back();
    for(unsigned value:{1U,99U,100U}) {
        const auto target=completed+1;assert(client.request("flashlightSet",{value}));wait(target);
        assert(light.senders.back()==sender && light.interactive.back() && light.values.back()==value);
    }
    QQmlEngine engine;engine.rootContext()->setContextProperty("nativeClient",&client);
    QQmlExpression expression(engine.rootContext(),nullptr,"nativeClient.request('flashlightSet', [50])");
    auto target=completed+1;assert(expression.evaluate().toBool() && !expression.hasError());wait(target);
    assert(light.values.back()==50 && light.senders.back()==sender);
    target=completed+1;assert(client.request("flashlightOff"));wait(target);assert(light.values.back()==0 && light.senders.back()==sender);
    client.close();target=completed+1;assert(client.request("lights"));wait(target);assert(light.senders.back()!=sender && !light.interactive.back());
    LightClient other;unsigned otherCompleted=0;
    QObject::connect(&other,&LightClient::completed,[&](const QString&,const QString&,const QString& error){assert(error.isEmpty());++otherCompleted;});
    const auto restarted=light.senders.back();assert(other.request("lights"));
    QElapsedTimer timer;timer.start();while(!otherCompleted && timer.elapsed()<3000)QCoreApplication::processEvents();
    assert(otherCompleted==1 && light.senders.back()!=restarted);
    qInfo("Native QML light client: fixed typed requests, interaction flags, persistent unique peer and reconnect isolation passed; private bus only");
}
#include "light-client-test.moc"
