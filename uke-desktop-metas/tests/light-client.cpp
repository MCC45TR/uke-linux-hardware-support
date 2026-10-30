// SPDX-License-Identifier: GPL-3.0-or-later
// Native Qt client and an independent sd-bus provider on a private bus.
#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QElapsedTimer>
#include <QEvent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <systemd/sd-bus.h>
#include <atomic>
#include <chrono>
#include <cassert>
#include <limits>
#include <mutex>
#include <thread>
#include "../src/light-client.cpp"
namespace {
std::atomic<bool> running{true},ready{false},holdNext{false},holding{false},replyLate{false};
std::atomic<unsigned> lastValue{0},calls{0};
std::atomic<unsigned> lastRgb{0};
std::atomic<bool> interactive{false};
std::mutex senderLock;
std::string sender;
sd_bus_message* held=nullptr; // Provider thread only.
std::string lastSender(){std::lock_guard lock(senderLock);return sender;}
int request(sd_bus_message* m,void*,sd_bus_error*) {
    {std::lock_guard lock(senderLock);sender=sd_bus_message_get_sender(m);}
    interactive=sd_bus_message_get_allow_interactive_authorization(m)>0;++calls;
    const std::string method=sd_bus_message_get_member(m);
    if(method=="SetBrightness") {unsigned value=0;assert(sd_bus_message_read(m,"u",&value)>0);lastValue=value;}
    else if(method=="SetEnabled") {int value=0;assert(sd_bus_message_read(m,"b",&value)>0);lastValue=value?100:0;}
    else if(method=="SetRgb") {
        unsigned r=0,g=0,b=0,percent=0;assert(sd_bus_message_read(m,"uuuu",&r,&g,&b,&percent)>0);
        assert(r<=255 && g<=255 && b<=255 && percent<=100);lastRgb=(r<<16)|(g<<8)|b;lastValue=percent;
    }
    else if(method=="Automatic" || method=="TestColors") {}
    else assert(method=="Status");
    if(holdNext.exchange(false)){assert(!held);held=sd_bus_message_ref(m);holding=true;return 1;}
    return sd_bus_reply_method_return(m,"s",method=="Status"?"status":"changed");
}
const sd_bus_vtable methods[]={SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Status","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetBrightness","u","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetRgb","uuuu","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Automatic","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("TestColors","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetEnabled","b","s",request,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
void server() {
    sd_bus* bus=nullptr;assert(sd_bus_open_system(&bus)>=0);
    assert(sd_bus_add_object_vtable(bus,nullptr,"/org/senemos/Uke/Torch","org.senemos.Uke.Torch1",methods,nullptr)>=0);
    assert(sd_bus_request_name(bus,"org.senemos.Uke.Torch",0)>=0);
    assert(sd_bus_add_object_vtable(bus,nullptr,"/org/senemos/Uke/Rgb","org.senemos.Uke.Rgb1",methods,nullptr)>=0);
    assert(sd_bus_request_name(bus,"org.senemos.Uke.Rgb",0)>=0);ready=true;
    while(running) {
        int r;do{r=sd_bus_process(bus,nullptr);assert(r>=0);}while(r>0);
        if(held && replyLate.exchange(false)){(void)sd_bus_reply_method_return(held,"s","late");sd_bus_message_unref(held);held=nullptr;holding=false;}
        assert(sd_bus_wait(bus,1000)>=0);
    }
    sd_bus_message_unref(held);sd_bus_unref(bus);
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);std::thread provider(server);
    for(unsigned i=0;i<3000 && !ready;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(ready);
    LightClient client;unsigned completed=0;bool expectCancellation=false;
    QObject::connect(&client,&LightClient::completed,[&](const QString&,const QString& output,const QString& error){
        if(expectCancellation)assert(error=="Request cancelled");else assert(error.isEmpty() && !output.isEmpty());++completed;
    });
    auto process=[] {QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);};
    auto wait=[&](unsigned target){QElapsedTimer timer;timer.start();while(completed<target && timer.elapsed()<3000)process();assert(completed==target);};
    assert(!client.request("host") && !client.request("flashlightSet",{QString("50")}));
    for(double value:{0.,101.,1.5,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
        assert(!client.request("flashlightSet",{value}));
    assert(!client.request("flashlightSet",{1,2}) && !client.request("flashlightOn",{1}) && calls==0);
    assert(client.request("lights"));wait(1);assert(!interactive);const auto firstSender=lastSender();
    for(unsigned value:{1U,47U,50U,51U,52U,99U,100U}) {
        const auto target=completed+1;assert(client.request("flashlightSet",{value}));wait(target);
        assert(lastSender()==firstSender && interactive && lastValue==value);
    }
    QQmlEngine engine;engine.rootContext()->setContextProperty("nativeClient",&client);
    QQmlExpression expression(engine.rootContext(),nullptr,"nativeClient.request('flashlightSet', [50])");
    auto target=completed+1;assert(expression.evaluate().toBool() && !expression.hasError());wait(target);
    assert(lastValue==50 && lastSender()==firstSender);
    for(const QVariantList& values: {QVariantList{256,0,0,100},QVariantList{0,-1,0,100},QVariantList{0,0,0,101},
            QVariantList{0,0,1.5,100},QVariantList{0,0,true,100},QVariantList{QString("255"),0,0,100},QVariantList{0,0,0}})
        assert(!client.request("rgbSet",values));
    QQmlExpression rgbExpression(engine.rootContext(),nullptr,"nativeClient.request('rgbSet', [255, 255, 255, 52])");
    target=completed+1;assert(rgbExpression.evaluate().toBool() && !rgbExpression.hasError());wait(target);
    assert(lastRgb==0xffffff && lastValue==52 && interactive && lastSender()==firstSender);
    target=completed+1;assert(client.request("rgbAuto"));wait(target);assert(interactive && lastSender()==firstSender);
    target=completed+1;assert(client.request("flashlightOff"));wait(target);assert(lastValue==0 && lastSender()==firstSender);
    client.close();target=completed+1;assert(client.request("lights"));wait(target);assert(lastSender()!=firstSender && !interactive);
    const auto reopened=lastSender();LightClient other;unsigned otherCompleted=0;
    QObject::connect(&other,&LightClient::completed,[&](const QString&,const QString&,const QString& error){assert(error.isEmpty());++otherCompleted;});
    assert(other.request("lights"));QElapsedTimer timer;timer.start();while(!otherCompleted && timer.elapsed()<3000)process();
    assert(otherCompleted==1 && lastSender()!=reopened);
    // A pending authorization reply cannot keep a closed popup's connection alive.
    holdNext=true;assert(client.request("flashlightOn"));timer.restart();while(!holding && timer.elapsed()<3000)process();assert(holding);
    const auto departing=lastSender();expectCancellation=true;target=completed+1;client.close();wait(target);expectCancellation=false;
    auto observer=QDBusConnection::connectToBus(QDBusConnection::SystemBus,"peer-observer");
    timer.restart();while(observer.interface()->isServiceRegistered(QString::fromStdString(departing)).value() && timer.elapsed()<3000)process();
    assert(!observer.interface()->isServiceRegistered(QString::fromStdString(departing)).value());
    replyLate=true;timer.restart();while(holding && timer.elapsed()<3000)process();assert(!holding);
    for(unsigned i=0;i<10;++i)process();
    assert(completed==target);
    expectCancellation=true;const auto before=completed;
    for(unsigned i=0;i<8;++i)assert(client.request("lights"));
    assert(!client.request("lights"));
    client.close();assert(completed==before+8);expectCancellation=false;
    running=false;provider.join();
    qInfo("Native QML light client: fixed typed requests, QML conversion, interaction flags, persistent unique peers, bounded pending calls and popup-close revocation passed; private bus only");
}
