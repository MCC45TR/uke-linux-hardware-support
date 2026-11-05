// SPDX-License-Identifier: MIT
#include "frequency-source.hpp"
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <unistd.h>
namespace {
struct Fd {int n;~Fd(){if(n>=0)close(n);}explicit Fd(int value):n(value){}};
}
int main(int argc,char **argv) {
    alarm(15);QCoreApplication app(argc,argv);const auto a=app.arguments();
    if(a.size()!=8||(a[1]!="cpu"&&a[1]!="gpu")||!QStringList{"minimum","maximum","governor"}.contains(a[3]))return 64;
    if(geteuid()!=0)return 77;
    Fd lock(open("/run/uke-control-qt-frequency.lock",O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600));struct stat st{};
    if(lock.n<0||fstat(lock.n,&st)||!S_ISREG(st.st_mode)||st.st_uid!=0||st.st_nlink!=1||flock(lock.n,LOCK_EX|LOCK_NB))return 75;
    const auto initial=uq::frequencyDevice(a[1],a[2]);
    const QJsonObject expected{{"minimum",a[4]},{"maximum",a[5]},{"governor",a[6]}};
    const auto result=uq::changeFrequency(initial,a[3],a[7],expected,[&]{return uq::frequencyDevice(a[1],a[2]);},[&](const QString &field,const QString &value){
        const auto path=initial.value("path").toString();
        if(!path.startsWith("/sys/devices/"))return false;
        Fd dir(open(path.toUtf8().constData(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));struct statfs fs{};
        if(dir.n<0||fstatfs(dir.n,&fs)||fs.f_type!=SYSFS_MAGIC)return false;
        const bool cpu=a[1]=="cpu";
        const QMap<QString,QString> files{{"minimum",cpu?"scaling_min_freq":"min_freq"},{"maximum",cpu?"scaling_max_freq":"max_freq"},{"governor",cpu?"scaling_governor":"governor"}};
        Fd output(openat(dir.n,files.value(field).toUtf8().constData(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW));
        if(output.n<0)return false;const auto bytes=value.toUtf8()+'\n';return write(output.n,bytes.constData(),size_t(bytes.size()))==bytes.size();
    });
    const QJsonObject response{{"schema","org.uke.qt.frequency-control.v1"},{"applied",result.applied},{"error",result.error},{"after",result.after},{"persistent",false}};
    QFile out;out.open(STDOUT_FILENO,QIODevice::WriteOnly);out.write(QJsonDocument(response).toJson(QJsonDocument::Compact));out.write("\n");
    return result.error.isEmpty()?0:69;
}
