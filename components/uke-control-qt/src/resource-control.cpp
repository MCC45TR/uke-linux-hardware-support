// SPDX-License-Identifier: MIT
#include "resource-source.hpp"
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <unistd.h>
namespace {struct Fd{int n;explicit Fd(int v):n(v){}~Fd(){if(n>=0)close(n);}};}
int main(int argc,char **argv) {
    alarm(15);QCoreApplication app(argc,argv);const auto a=app.arguments();
    if(a.size()!=5||a[1].size()>120||a[2].size()>128||a[3].size()>4096||a[4].size()>128)return 64;
    if(geteuid()!=0)return 77;
    QFile compatible("/sys/firmware/devicetree/base/compatible");if(!compatible.open(QIODevice::ReadOnly))return 77;
    const auto names=compatible.read(4096).split('\0');if(!names.contains("xiaomi,uke")||!names.contains("qcom,sm7675"))return 77;
    Fd lock(open("/run/uke-control-qt-resource.lock",O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600));struct stat st{};
    if(lock.n<0||fstat(lock.n,&st)||!S_ISREG(st.st_mode)||st.st_uid||st.st_nlink!=1||flock(lock.n,LOCK_EX|LOCK_NB))return 75;
    const auto before=uq::resourceSetting(a[1]);QString error=uq::resourceRefusal(before,a[4]);bool applied=false;
    if(error.isEmpty()&&(before.value("value").toString()!=a[2]||before.value("path").toString()!=a[3]))error="settings_changed";
    if(error.isEmpty()&&before.value("value").toString()!=a[4]) {
        // Paths are derived again from a fixed typed ID, including the actual
        // root backing queue. The caller's path is only a stale-provider guard.
        const auto p=before.value("path").toString();
        if(!p.startsWith("/sys/devices/")&&!p.startsWith("/sys/kernel/mm/")&&p!="/proc/sys/vm/swappiness")error="provider_changed";
        else {
            Fd fd(open(p.toUtf8().constData(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW));struct statfs fs{};
            if(fd.n<0||fstatfs(fd.n,&fs)||(fs.f_type!=SYSFS_MAGIC&&fs.f_type!=PROC_SUPER_MAGIC))error="write_failed";
            else {
                const auto fresh=uq::resourceSetting(a[1]);
                if(fresh.value("value")!=before.value("value")||fresh.value("path")!=before.value("path"))error="settings_changed";
                else {const auto b=a[4].toUtf8()+'\n';if(write(fd.n,b.constData(),size_t(b.size()))!=b.size())error="write_failed";else applied=true;}
            }
        }
    }
    const auto after=uq::resourceSetting(a[1]);if(error.isEmpty()&&after.value("value").toString()!=a[4])error="readback_differs";
    const QJsonObject response{{"schema","org.uke.qt.resource-control.v1"},{"applied",applied},{"error",error},{"after",after},{"persistent",false}};
    QFile out;out.open(STDOUT_FILENO,QIODevice::WriteOnly);out.write(QJsonDocument(response).toJson(QJsonDocument::Compact)+'\n');return error.isEmpty()?0:69;
}
