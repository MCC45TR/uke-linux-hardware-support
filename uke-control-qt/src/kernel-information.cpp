// SPDX-License-Identifier: MIT
// Fixed, read-only kernel observations. No arbitrary paths, commands or writes.
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace {
QJsonValue attribute(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QJsonValue();
    const auto data=f.read(4097);
    if (data.size()>4096) return QJsonValue();
    auto text=QString::fromUtf8(data).trimmed();text.remove(QChar(0));
    return text.isEmpty()?QJsonValue():QJsonValue(text);
}
QJsonObject fields(const QString &path,const QStringList &names) {
    QJsonObject o;for(const auto &name:names)o[name]=attribute(path+'/'+name);return o;
}
QString driver(const QString &path) {return QFileInfo(QFileInfo(path+"/driver").canonicalFilePath()).fileName();}
QJsonObject command(const QString &argument) {
    QProcess p;QProcessEnvironment environment;environment.insert("PATH","/usr/sbin:/usr/bin");environment.insert("LANG","C.UTF-8");p.setProcessEnvironment(environment);
    p.start("/usr/libexec/uke-charge-control",{argument});
    if(!p.waitForStarted(1000))return {{"error","helper_start_failed"}};
    if(!p.waitForFinished(5000)){p.kill();p.waitForFinished(1000);return {{"error","helper_timeout"}};}
    const auto output=p.readAllStandardOutput();
    if(output.size()>65536)return {{"error","helper_output_too_large"}};
    auto result=QJsonDocument::fromJson(output).object();
    if(result.isEmpty())return {{"error","helper_invalid_output"}};
    result["exit_status"]=p.exitCode();return result;
}
QJsonObject dock() {
    QJsonObject out;QJsonArray devices,ports,net,displays,roles;
    const QRegularExpression usbName("^[0-9]+-[0-9]+(?:\\.[0-9]+)*$");
    for(const auto &name:QDir("/sys/bus/usb/devices").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(devices.size()>=64)break;if(!usbName.match(name).hasMatch())continue;
        const auto path="/sys/bus/usb/devices/"+name;
        auto o=fields(path,{"product","manufacturer","idVendor","idProduct","speed","version","bDeviceClass","bMaxPower","authorized","power/control","power/runtime_status","power/autosuspend_delay_ms"});
        o["name"]=name;o["driver"]=driver(path);devices.append(o);
    }
    for(const auto &name:QDir("/sys/class/typec").entryList({"port*"},QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(ports.size()>=8)break;if(!QRegularExpression("^port[0-9]+$").match(name).hasMatch())continue;
        const auto path="/sys/class/typec/"+name;
        auto o=fields(path,{"data_role","power_role","power_operation_mode","usb_power_delivery_revision","usb_typec_revision","preferred_role"});
        o["name"]=name;o["partner"]=QFileInfo::exists(path+"/"+name+"-partner");
        o["owner"]=driver(path+"/device");ports.append(o);
    }
    for(const auto &name:QDir("/sys/class/usb_role").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(roles.size()>=8)break;auto o=fields("/sys/class/usb_role/"+name,{"role"});o["name"]=name;roles.append(o);
    }
    for(const auto &name:QDir("/sys/class/net").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(net.size()>=16)break;const auto path="/sys/class/net/"+name;
        if(!QFileInfo(path+"/device").canonicalFilePath().contains(QRegularExpression("/usb[0-9]+/")))continue;
        auto o=fields(path,{"operstate","carrier","speed","duplex","mtu","statistics/rx_bytes","statistics/tx_bytes","statistics/rx_errors","statistics/tx_errors","statistics/rx_dropped","statistics/tx_dropped"});
        o["name"]=name;o["driver"]=driver(path+"/device");net.append(o);
    }
    for(const auto &name:QDir("/sys/class/drm").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        if(displays.size()>=16)break;if(!name.contains("-DP-")&&!name.contains("-HDMI-A-"))continue;
        auto o=fields("/sys/class/drm/"+name,{"status","enabled","modes","dpms"});o["name"]=name;displays.append(o);
    }
    out["devices"]=devices;out["ports"]=ports;out["networks"]=net;out["displays"]=displays;out["roles"]=roles;
    out["usb_profile"]=attribute("/sys/firmware/devicetree/base/chosen/senemos,usb-profile");
    // Requests in Uke Control perform a single swap; no persistent forced-role policy exists.
    out["kernel_managed"]=out["usb_profile"].toString()=="typec-v1"&&ports.size()==1&&ports.first().toObject()["owner"].toString()=="ucsi_glink.pmic_glink_ucsi";
    return out;
}
QJsonObject tag() {
    QJsonObject out;const auto device=QFileInfo("/sys/class/misc/fm-ntag/device").canonicalFilePath();
    out["present"]=!device.isEmpty();out["driver"]=device.isEmpty()?QString():driver(device);
    out["wakeup"]=attribute(device+"/power/wakeup");out["runtime_status"]=attribute(device+"/power/runtime_status");
    // Fixed read-only INFO ioctl from the exact fm19511 UAPI. No raw reads,
    // memory addressing, IRQ consumption, EEPROM writes or payload logging.
    struct Info {quint32 abi,capabilities,maxTransfer,reserved[5];} info{};
    static_assert(sizeof(Info)==32);
    const int fd=open("/dev/fm-ntag",O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0){out["info_errno"]=errno;return out;}
    struct stat st{};if(fstat(fd,&st)||!S_ISCHR(st.st_mode)){close(fd);out["info_errno"]=ENODEV;return out;}
    const int status=ioctl(fd,_IOR('F',0x70,Info),&info);const int error=status<0?errno:0;close(fd);
    out["info_errno"]=error;
    if(!error&&info.abi==1&&info.maxTransfer<=256){out["abi"]=int(info.abi);out["memory_read"]=bool(info.capabilities&2);out["rf_events"]=bool(info.capabilities&4);out["wakeup_capable"]=bool(info.capabilities&8);}
    return out;
}
}
int main(int argc,char **argv) {
    alarm(18);
    QCoreApplication app(argc,argv);const auto args=app.arguments();
    if(args.size()!=2||!QStringList{"dock","charging","tag"}.contains(args[1]))return 64;
    QJsonObject out{{"schema","org.uke.qt.kernel-information.v1"}};
    if(args[1]=="dock")out["data"]=dock();
    else if(args[1]=="tag")out["data"]=tag();
    else {if(geteuid()!=0)return 77;out["data"]=QJsonObject{{"admission",command("admission")},{"settings",command("settings")},{"status",command("status")}};}
    QFile output;output.open(STDOUT_FILENO,QIODevice::WriteOnly);output.write(QJsonDocument(out).toJson(QJsonDocument::Compact));output.write("\n");return 0;
}
