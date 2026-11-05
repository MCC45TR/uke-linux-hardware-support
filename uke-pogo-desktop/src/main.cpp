// SPDX-License-Identifier: MIT
#include "state.hpp"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QFile>
#include <QTimer>
#include <iostream>
#include <linux/magic.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <unistd.h>
using namespace uke_pogo_desktop;
static constexpr auto upower="org.freedesktop.UPower";
static constexpr auto upower_path="/org/freedesktop/UPower";
static constexpr auto backlight_path="/org/freedesktop/UPower/KbdBacklight";
static constexpr auto backlight_interface="org.freedesktop.UPower.KbdBacklight";
static constexpr auto powerdevil="org.kde.Solid.PowerManagement";
static constexpr auto action_path="/org/kde/Solid/PowerManagement/Actions/KeyboardBrightnessControl";
static constexpr auto action_interface="org.kde.Solid.PowerManagement.Actions.KeyboardBrightnessControl";
static std::string read_sysfs(const fs::path &path) {
    const int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return {};
    struct statfs info{};char bytes[1024];
    const auto size=fstatfs(fd,&info)==0&&info.f_type==SYSFS_MAGIC?read(fd,bytes,sizeof(bytes)):-1;
    close(fd);
    return size>0&&size<ssize_t(sizeof(bytes))?std::string(bytes,size):std::string{};
}
static QDBusMessage call(const QDBusConnection &bus,const char *name,const char *path,
                         const char *interface,const char *method,const QVariantList &arguments={}) {
    auto request=QDBusMessage::createMethodCall(QString::fromLatin1(name),QString::fromLatin1(path),
        QString::fromLatin1(interface),QString::fromLatin1(method));
    request.setAutoStartService(false);request.setArguments(arguments);
    return bus.call(request,QDBus::Block,1500);
}
static std::optional<int> integer(const QDBusMessage &reply) {
    if(reply.type()!=QDBusMessage::ReplyMessage||reply.arguments().size()!=1||
       reply.arguments()[0].metaType().id()!=QMetaType::Int)return {};
    const int value=reply.arguments()[0].toInt();
    if(value<0||value>10000)return {};
    return value;
}
class Monitor:public QObject {
    Q_OBJECT
    QDBusConnection system_=QDBusConnection::systemBus(),session_=QDBusConnection::sessionBus();
    QTimer settle_;
    Refresh refresh_;
    unsigned attempts_=0;
    QString own_native_;
    bool has_object(bool pogo) {
        const auto reply=call(system_,upower,upower_path,upower,"EnumerateKbdBacklights");
        if(reply.type()!=QDBusMessage::ReplyMessage||reply.arguments().size()!=1)return false;
        const auto objects=qdbus_cast<QList<QDBusObjectPath>>(reply.arguments()[0]);
        if(objects.size()>32)return false;
        bool own=false;
        for(const auto &object:objects) {
            const auto native=call(system_,upower,object.path().toUtf8().constData(),"org.freedesktop.DBus.Properties",
                "Get",{QString::fromLatin1(backlight_interface),QStringLiteral("NativePath")});
            if(native.type()!=QDBusMessage::ReplyMessage||native.arguments().size()!=1)continue;
            const auto path=qdbus_cast<QDBusVariant>(native.arguments()[0]).variant().toString();
            if(path.isEmpty())continue;
            // Keep the admitted native path across removal until UPower drops it.
            if(!own_native_.isEmpty()&&path==own_native_)own=true;
        }
        return own==pogo;
    }
    Observation observe() {
        Observation state;state.pogo=present("/sys",read_sysfs);
        if(state.pogo) {
            std::error_code error;
            const auto path=fs::canonical(fs::path("/sys/class/leds")/led_name,error);
            if(!error)own_native_=QString::fromStdString(path.string());
        }
        const auto max=integer(call(system_,upower,backlight_path,backlight_interface,"GetMaxBrightness"));
        state.upower_ready=max&&has_object(state.pogo)&&(!state.pogo||*max>=100);
        state.upower_max=max.value_or(0);
        const auto reply=call(session_,powerdevil,"/org/kde/Solid/PowerManagement",powerdevil,
            "isActionSupported",{QStringLiteral("KeyboardBrightnessControl")});
        if(reply.type()==QDBusMessage::ReplyMessage&&reply.arguments().size()==1&&
           reply.arguments()[0].metaType().id()==QMetaType::Bool) {
            state.powerdevil_running=true;state.powerdevil_supported=reply.arguments()[0].toBool();
            if(state.powerdevil_supported) {
                const auto maximum=integer(call(session_,powerdevil,action_path,action_interface,"keyboardBrightnessMax"));
                if(!maximum)state.powerdevil_running=false;
                else state.powerdevil_max=*maximum;
            }
        }
        return state;
    }
public:
    Monitor() {
        settle_.setInterval(500);
        connect(&settle_,&QTimer::timeout,this,&Monitor::check);
        for(const char *signal:{"DeviceAdded","DeviceRemoved"})
            system_.connect(QString::fromLatin1(upower),QString::fromLatin1(upower_path),QString::fromLatin1(upower),
                QString::fromLatin1(signal),this,SLOT(changed(QDBusObjectPath)));
        auto *upower_watcher=new QDBusServiceWatcher(QString::fromLatin1(upower),system_,QDBusServiceWatcher::WatchForOwnerChange,this);
        auto *powerdevil_watcher=new QDBusServiceWatcher(QString::fromLatin1(powerdevil),session_,QDBusServiceWatcher::WatchForOwnerChange,this);
        for(auto *watcher:{upower_watcher,powerdevil_watcher})
            connect(watcher,&QDBusServiceWatcher::serviceOwnerChanged,this,
                [this](const QString &,const QString &,const QString &){wake();});
    }
    void wake() {attempts_=0;settle_.start();}
    int inspect() {
        const auto state=observe();
        std::cout<<"pogo_backlight="<<state.pogo<<" upower_ready="<<state.upower_ready
                 <<" upower_max="<<state.upower_max<<" powerdevil_running="<<state.powerdevil_running
                 <<" powerdevil_supported="<<state.powerdevil_supported<<'\n';
        return 0;
    }
public slots:
    void changed(const QDBusObjectPath &path) {
        if(path.path().startsWith(QLatin1String("/org/freedesktop/UPower/KbdBacklight/")))wake();
    }
    void check() {
        const auto state=observe();
        if(refresh_.decide(state)) {
            const auto reply=call(session_,"org.freedesktop.systemd1","/org/freedesktop/systemd1",
                "org.freedesktop.systemd1.Manager","TryRestartUnit",
                {QStringLiteral("plasma-powerdevil.service"),QStringLiteral("replace")});
            std::cout<<(reply.type()==QDBusMessage::ReplyMessage?
                "Pogo backlight changed; existing PowerDevil service re-detect requested\n":
                "PowerDevil re-detect request failed; no hardware operation attempted\n")<<std::flush;
        }
        if(++attempts_>=20)settle_.stop();
    }
};
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(argc!=2)return 2;
    const std::string_view mode=argv[1];
    const bool attached_check=mode=="--bridge-attached";
    if(mode!="--check"&&mode!="--monitor"&&!attached_check)return 2;
    const auto identity=read_sysfs("/sys/firmware/devicetree/base/compatible");
    if(!compatible(identity,"xiaomi,uke")||!compatible(identity,"qcom,sm7675"))return attached_check?1:0;
    if(attached_check)return present("/sys",read_sysfs)?0:1;
    const auto desktops=qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(':');
    if(!desktops.contains(QStringLiteral("KDE")))return 0;
    Monitor monitor;
    if(std::string_view(argv[1])=="--check")return monitor.inspect();
    monitor.wake();return app.exec();
}
#include "main.moc"
