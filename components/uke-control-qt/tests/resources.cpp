// SPDX-License-Identifier: MIT
#include "../src/resource-source.hpp"
#include "../src/application-source.hpp"
#include <QtTest>
using namespace uq;
class ResourceTests:public QObject {
    Q_OBJECT
    QTemporaryDir temp;
    ResourceRoots roots;
    void put(const QString &path,const QByteArray &data){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(data),qint64(data.size()));}
private slots:
    void initTestCase(){QVERIFY(temp.isValid());roots={temp.path()+"/proc",temp.path()+"/sys"};QDir().mkpath(roots.sys);}
    void ratesAndReset() {
        QJsonObject a{{"disk",QJsonObject{{"value",100},{"scale",.5}}}},b{{"disk",QJsonObject{{"value",140},{"scale",.5}}}};
        QCOMPARE(resourceRates(a,b,2).value("disk").toDouble(),10.);
        QVERIFY(resourceRates({},b,2).value("disk").isNull());
        QVERIFY(resourceRates(b,a,2).value("disk").isNull());
        QVERIFY(resourceRates(a,b,0).value("disk").isNull());
        QVERIFY(resourceRates(a,b,31).value("disk").isNull());
        b["disk"]=QJsonObject{{"value",140},{"scale",1}};QVERIFY(resourceRates(a,b,2).value("disk").isNull());
        b["disk"]=QJsonObject{{"value",140},{"scale",.5},{"generation","new-invocation"}};QVERIFY(resourceRates(a,b,2).value("disk").isNull());
    }
    void memoryAndPressure() {
        put(roots.proc+"/meminfo","MemTotal: 8192000 kB\nMemAvailable: 4096000 kB\nSwapTotal: 2048000 kB\nSwapFree: 1024000 kB\n");
        put(roots.proc+"/pressure/memory","some avg10=2.50 avg60=1.0 avg300=0.0 total=5000\nfull avg10=0.10 avg60=0.0 avg300=0.0 total=1000\n");
        put(roots.proc+"/vmstat","pgmajfault 10\npswpout 20\n");
        const auto s=resourceSnapshot("memory",roots);const auto m=s.value("metrics").toObject();
        QCOMPARE(m.value("memory/used").toDouble(),4000.);QCOMPARE(m.value("memory/swap").toDouble(),1000.);QCOMPARE(m.value("psi/memory/some").toDouble(),2.5);
        QCOMPARE(s.value("counters").toObject().value("vm/pswpout").toObject().value("value").toDouble(),20.);
    }
    void fixedControlsAndBounds() {
        put(roots.proc+"/sys/vm/swappiness","60\n");auto s=resourceSetting("memory.swappiness",roots);
        QVERIFY(resourceRefusal(s,"200").isEmpty());QCOMPARE(resourceRefusal(s,"201"),QString("outside_limits"));
        QVERIFY(!resourceRefusal(s,"060").isEmpty());QVERIFY(!resourceRefusal(s,"-1").isEmpty());QVERIFY(!resourceRefusal(s,"1\n2").isEmpty());
        QVERIFY(resourceSetting("memory.../../x",roots).isEmpty());
        s=resourceSetting("memory.mglru",roots);QCOMPARE(s.value("status").toString(),QString("not-exported"));QVERIFY(!resourceRefusal(s,"7").isEmpty());
        put(roots.sys+"/kernel/mm/transparent_hugepage/enabled","always [madvise] never\n");s=resourceSetting("memory.thp",roots);
        QCOMPARE(s.value("value").toString(),QString("madvise"));QVERIFY(resourceRefusal(s,"never").isEmpty());QVERIFY(!resourceRefusal(s,"surprise").isEmpty());
    }
    void partitionUsesParentQueueAnd512ByteSectors() {
        const auto disk=roots.sys+"/devices/platform/ufs/block/sda";
        QDir().mkpath(disk+"/sda32");QDir().mkpath(disk+"/queue");QDir().mkpath(roots.sys+"/class/block");
        QVERIFY(QFile::link(disk+"/sda32",roots.sys+"/class/block/sda32"));put(disk+"/sda32/partition","32\n");put(roots.proc+"/uke-test-root-device","sda32\n");
        put(disk+"/queue/scheduler","none [mq-deadline] kyber bfq\n");put(disk+"/queue/read_ahead_kb","1024\n");put(disk+"/stat","10 0 2048 50 20 0 4096 100 0 40 100\n");
        const auto setting=resourceSetting("storage.scheduler",roots);QCOMPARE(setting.value("value").toString(),QString("mq-deadline"));QVERIFY(setting.value("path").toString().endsWith("sda/queue/scheduler"));
        QVERIFY(resourceRefusal(setting,"bfq").isEmpty());QVERIFY(!resourceRefusal(setting,"noop").isEmpty());
        const auto c=resourceSnapshot("storage",roots).value("counters").toObject().value("io/read").toObject();QCOMPARE(c.value("value").toDouble()*c.value("scale").toDouble(),1.);
        QVERIFY(!resourceRefusal(resourceSetting("storage.read-ahead",roots),"3").isEmpty());
    }
    void applicationTargetRefusals() {QVERIFY(applicationScope("../../system.slice").isEmpty());QVERIFY(applicationScope("sshd.service").isEmpty());}
};
QTEST_GUILESS_MAIN(ResourceTests)
#include "resources.moc"
