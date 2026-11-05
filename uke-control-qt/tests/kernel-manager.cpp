// SPDX-License-Identifier: MIT
#include "../src/kernel-manager.hpp"
#include "../src/frequency-source.hpp"
#include "manager-fixture.hpp"
#include <QQuickWindow>
#include <QtTest>
using namespace uq;
class ManagerTests:public QObject {
    Q_OBJECT
private slots:
    void boundsAndGovernors() {
        const auto d=managerDevice();
        QVERIFY(frequencyRefusal(d,"maximum","200000").isEmpty());
        QVERIFY(!frequencyRefusal(d,"maximum","400000").isEmpty());
        QVERIFY(!frequencyRefusal(d,"maximum","0200000").isEmpty());
        QVERIFY(!frequencyRefusal(d,"../../governor","performance").isEmpty());
        QVERIFY(!frequencyRefusal(d,"governor","userspace").isEmpty());
        QVERIFY(frequencyRefusal(d,"governor","performance").isEmpty());
        auto limited=d;limited["maximum"]="100000";
        QCOMPARE(frequencyRefusal(limited,"minimum","200000"),QString("minimum_exceeds_maximum"));
        limited["writable"]=false;QVERIFY(!frequencyRefusal(limited,"minimum","100000").isEmpty());
        QVERIFY(!frequencyRefusal(managerDevice("gpu"),"maximum","0").isEmpty());
        QVERIFY(!frequencyRefusal(d,"minimum","0").isEmpty());
    }
    void staleAndOneWriteReadback() {
        auto state=managerDevice();const auto original=state;int writes=0;
        auto read=[&]{return state;};auto write=[&](const QString &key,const QString &value){++writes;state[key]=value;return true;};
        auto result=changeFrequency(original,"maximum","200000",original,read,write);
        QVERIFY(result.applied);QVERIFY(result.error.isEmpty());QCOMPARE(writes,1);QCOMPARE(state.value("maximum").toString(),QString("200000"));
        result=changeFrequency(state,"maximum","200000",state,read,write);QVERIFY(!result.applied);QCOMPARE(writes,1);
        result=changeFrequency(state,"maximum","300000",original,read,write);QCOMPARE(result.error,QString("settings_changed"));QCOMPARE(writes,1);
        const auto expected=state;state["governor"]="performance";
        result=changeFrequency(expected,"maximum","300000",expected,read,write);QCOMPARE(result.error,QString("settings_changed"));QCOMPARE(writes,1);
        state=original;
        result=changeFrequency(state,"maximum","200000",state,read,[&](const QString &,const QString &){return false;});
        QCOMPARE(result.error,QString("write_failed"));QVERIFY(!result.applied);
        result=changeFrequency(state,"maximum","200000",state,read,[&](const QString &,const QString &){state["maximum"]="100000";return true;});
        QVERIFY(result.applied);QCOMPARE(result.error,QString("readback_differs"));
    }
    void cpuCounters() {
        const QJsonObject a{{"cpu0",QJsonArray{100,0,50,800,50,0,0,0}}},b{{"cpu0",QJsonArray{120,0,60,850,50,0,0,0}}};
        QCOMPARE(cpuLoads(a,b).value("cpu0").toDouble(),37.5);
        QVERIFY(!cpuLoads({},b).value("cpu0").isValid());
        QVERIFY(!cpuLoads(b,a).value("cpu0").isValid());
        QVERIFY(!cpuLoads(a,a).value("cpu0").isValid());
    }
    void simulationIsolationAndCombinedState() {
        Fixture f;f.enabled=true;f.results["manager.sample"]=QJsonObject{{"data",managerSample()}};f.results["sensors.once"]=QJsonObject{{"data",managerSensors()}};
        KernelManager manager(f);manager.open("cpu");
        QCOMPARE(manager.devices().size(),2);QVERIFY(manager.active());
        QCOMPARE(manager.state().value("values").toMap().value("cpu:policy0").toMap().value("frequency").toDouble(),200.);
        QCOMPARE(manager.current("cpu:policy0","maximum"),QString("300000"));
        QCOMPARE(manager.choices("cpu:policy0","governor").size(),2);
        QVERIFY(manager.prepare("cpu:policy0","maximum","200000"));
        QVERIFY(manager.review().value("blocked").toBool());QVERIFY(!manager.apply(manager.review().value("token").toString()));
        manager.open("sensors");QVERIFY(manager.review().isEmpty());QVERIFY(!manager.active());QCOMPARE(manager.sensors().size(),3);
        QVERIFY(!manager.state().value("sensorValues").toMap().value("SensorProxy/CompassHeading").toMap().value("value").isValid());
        manager.selectSensor("SensorProxy/LightLevel");QTest::qWait(3);manager.refresh();QTest::qWait(3);manager.refresh();
        QVERIFY(manager.plot("sensor:SensorProxy/LightLevel").value("points").toInt()>=2);
        manager.pause();QVERIFY(!manager.active());
    }
    void historyBoundAndInspection() {
        Fixture f;f.enabled=true;f.results["manager.sample"]=QJsonObject{{"data",managerSample()}};
        KernelManager manager(f);manager.open("gpu");
        for(int i=0;i<605;++i){QTest::qWait(2);manager.refresh();}
        const auto report=manager.plot("gpu:gpu0/frequency",600,300);
        QVERIFY(report.value("points").toInt()<=600);QVERIFY(report.value("points").toInt()>1);
        QQuickWindow window;window.resize(480,240);TimePlot plot(window.contentItem());plot.setWidth(480);plot.setHeight(200);plot.setReport(report);window.show();
        QTest::qWait(50);QTest::mouseMove(&window,QPoint(200,80));QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,QPoint(200,80));QTRY_VERIFY(!plot.cursor().isEmpty());
        manager.pause();
    }
    void asyncDeliveryAndPausedInitialRead() {
        // No sysfs or commands: only the production asynchronous delivery path.
        KernelManager manager({},nullptr,true,[]{QThread::msleep(5);return managerSample();});
        manager.open("cpu");QVERIFY(manager.busy());
        QTest::qSleep(20); // Worker is done; its queued result is not delivered yet.
        QVERIFY(manager.busy());QVERIFY(manager.devices().isEmpty());
        QTRY_VERIFY(!manager.busy());QCOMPARE(manager.devices().size(),2);
        const int revision=manager.revision();manager.refresh();manager.pause();
        QTRY_VERIFY(!manager.busy());QVERIFY(manager.revision()>revision);
        QCOMPARE(manager.devices().size(),2);QVERIFY(!manager.active());
    }
    void resourceReviewAndPrivateExport() {
        Fixture f;f.enabled=true;f.results["manager.sample"]=QJsonObject{{"data",managerSample()}};
        KernelManager manager(f);manager.open("memory");
        QCOMPARE(manager.state().value("controls").toList().size(),3);
        QVERIFY(!manager.prepareResource("memory.swappiness","201"));
        QVERIFY(manager.prepareResource("memory.swappiness","61"));QVERIFY(manager.review().value("blocked").toBool());
        QVERIFY(!manager.apply(manager.review().value("token").toString()));
        manager.open("profiles");QVERIFY(manager.prepareApplication("app-example.scope","background"));
        QVERIFY(!manager.apply(manager.review().value("token").toString()));
        QTemporaryDir dir;QVERIFY(dir.isValid());const auto path=dir.filePath("session.txt");
        QVERIFY(manager.exportReport(QUrl::fromLocalFile(path)));QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Senemos Kernel Manager"));
        QVERIFY(!(QFileInfo(path).permissions()&QFileDevice::ReadOther));
        QVERIFY(!manager.exportReport(QUrl("https://example.invalid/report")));
    }
};
QTEST_MAIN(ManagerTests)
#include "kernel-manager.moc"
