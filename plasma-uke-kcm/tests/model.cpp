// SPDX-License-Identifier: MIT
#include "overview-model.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QtTest>

class ModelTest:public QObject {
    Q_OBJECT
private Q_SLOTS:
    void fixtureLifecycle() {
        OverviewModel model;
        QJsonObject component{{"component","battery"},{"availability","available"},{"data",QJsonObject{{"battery.cycle_count",QJsonObject{{"value",0},{"unit","cycles_semantics_unqualified"}}},{"readings",QJsonObject{{"battery.current_now",QJsonObject{{"value",-850000},{"unit","uA_positive_into_battery"}}}}}}}};
        QJsonObject fixture{{"schema","org.uke.control.overview.v1"},{"components",QJsonArray{component}}};
        QVERIFY(model.setFixture(fixture));QCOMPARE(model.cards().size(),15);
        model.refresh();QVERIFY(!model.busy());
        QCOMPARE(model.cards()[0].toMap()["state"].toString(),QStringLiteral("available"));
        QVERIFY(model.cards()[0].toMap()["details"].toString().contains("0"));
        bool zero=false,negative=false;
        for(const auto& raw:model.cards()[0].toMap()["metrics"].toList()) {
            const auto row=raw.toMap();
            if(row["id"]=="battery.cycle_count"){QVERIFY(row["known"].toBool());QCOMPARE(row["value"].toInt(),0);zero=true;}
            if(row["id"]=="battery.current_now"){QCOMPARE(row["value"].toDouble(),-0.85);QCOMPARE(row["unit"].toString(),QStringLiteral("A"));negative=true;}
        }
        QVERIFY(zero&&negative);
        model.completeFixture("usb","loading");model.completeFixture("battery","stale");
        QCOMPARE(model.cards()[0].toMap()["state"].toString(),QStringLiteral("stale"));
        QVERIFY(model.cards()[0].toMap().contains("snapshot"));
        QCOMPARE(model.cards()[3].toMap()["state"].toString(),QStringLiteral("loading"));
        model.completeFixture("usb","denied");QVERIFY(!model.busy());
    }
    void malformedFixturesDoNotActivate() {
        OverviewModel model;QVERIFY(!model.setFixture({}));
        QJsonObject component{{"component","usb"},{"availability","error"}};
        QVERIFY(!model.setFixture({{"schema","org.uke.control.overview.v1"},{"components",QJsonArray{component,component}}}));
        component["component"]="arbitrary-command";
        QVERIFY(!model.setFixture({{"schema","org.uke.control.overview.v1"},{"components",QJsonArray{component}}}));
        component["component"]="usb";component["availability"]="arbitrary-state";
        QVERIFY(!model.setFixture({{"schema","org.uke.control.overview.v1"},{"components",QJsonArray{component}}}));
    }
};
QTEST_GUILESS_MAIN(ModelTest)
#include "model.moc"
