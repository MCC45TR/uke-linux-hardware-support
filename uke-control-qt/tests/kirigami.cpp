// SPDX-License-Identifier: MIT
#include "../src/bridge.hpp"
#include "../src/theme-icons.hpp"
#include <KIconTheme>
#include <KIconLoader>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KStyleManager>
#include <QAccessible>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtTest>
#include <QNdefNfcTextRecord>
#include <QNdefNfcUriRecord>
#include "manager-fixture.hpp"
using namespace uq;
static QQuickItem *visualItem(QQuickItem *parent, const QString &name) {
  if (!parent->isVisible())
    return nullptr;
  if (parent->objectName() == name)
    return parent;
  for (auto *child : parent->childItems())
    if (auto *found = visualItem(child, name))
      return found;
  return nullptr;
}
class KirigamiTests : public QObject {
  Q_OBJECT
  Fixture fixture;
  ThemeIcons themeIcons;
  void snapshot(QQuickWindow *window, const QString &name) {
    const auto directory = qEnvironmentVariable("UKE_QT_TEST_SCREENSHOTS");
    if (directory.isEmpty())
      return;
    QVERIFY(QDir().mkpath(directory));
    QTest::qWait(350);
    const auto image = window->grabWindow();
    QVERIFY2(!image.isNull(), qPrintable(name));
    QVERIFY(image.save(directory + "/" + name + ".png"));
  }
private slots:
  void initTestCase() {
    fixture = Fixture::load(qEnvironmentVariable("UKE_QT_TEST_FIXTURE", FIXTURE_PATH));
    const auto boot = QJsonDocument::fromJson(R"({
      "schema":"org.uke.control.update-status.v1","available":true,"complete":true,"read_only":true,
      "running_kernel":"7.3.1-1500","current_slot":"b","next_boot_slot":"b","pending_slot_switch":false,
      "slots":[
        {"slot":"a","kernel_release":"7.3.1-1400","bootable":true,"boot_successful":false,"matches_running_kernel":false},
        {"slot":"b","kernel_release":"7.3.1-1500","bootable":true,"boot_successful":true,"matches_running_kernel":true}],
      "kernel_approval":{"state":"accepted","running_kernel_approved":true,"latest_update_approved":true,
        "accepted_kernel":"7.3.1-1500","accepted_slot":"b","accepted_kernel_verified":true,
        "phase":"stable","candidate_kernel":null,"last_failed_kernel":null},"issues":[]
    })").object();
    fixture.results.insert("update.status", QJsonObject{{"data",boot}});
  }
  void themedCatalogAndEveryRequest() {
    QVERIFY(themeIcons.names().size() >= 40);
    QSignalSpy changed(&themeIcons, &ThemeIcons::changed);
    themeIcons.refresh();
    QCOMPARE(changed.size(), 1);
    QVERIFY(QMetaObject::invokeMethod(KIconLoader::global(), "iconChanged",
                                      Q_ARG(int, int(KIconLoader::Small))));
    QCOMPARE(changed.size(), 2);
    QSet<QString> semanticIcons;
    Bridge bridge(fixture, false);
    for (const auto &entry : actions()) {
      const auto id = QString::fromStdString(entry.id);
      const auto icon = actionIcon(entry);
      semanticIcons.insert(icon);
      QVERIFY2(themeIcons.names().contains(icon), qPrintable(id + ": " + icon));
      QVERIFY2(!QIcon::fromTheme(themeIcons.names().value(icon).toString())
                    .pixmap(32, 32).isNull(), qPrintable(id));
      QVERIFY(bridge.openAction(id));
      QVERIFY(bridge.observation().isEmpty());
      QVERIFY(bridge.review().isEmpty());
      // Supply syntactically valid draft identifiers only. Fixture mode never
      // opens these paths, contacts hardware or launches a terminal.
      for (const auto &p : entry.parameters) {
        if (!p.initial.empty() || p.optional)
          continue;
        QString value;
        switch (p.kind) {
        case Kind::choice: value = QString::fromStdString(p.choices.front().value); break;
        case Kind::integer: value = QString::number(qulonglong(p.minimum)); break;
        case Kind::path: case Kind::absolute_path: value = "/tmp/uke-synthetic-test"; break;
        case Kind::interface: value = "eth0"; break;
        case Kind::usb_device: value = "1-1"; break;
        case Kind::input_device: value = "/dev/input/event0"; break;
        case Kind::partition_uuid: value = "12345678-abcd"; break;
        case Kind::color: value = "#102030"; break;
        case Kind::swap_size: value = "half"; break;
        case Kind::repository: value = "example/synthetic-test"; break;
        case Kind::identifier: value = "synthetic"; break;
        }
        bridge.setDraft(QString::fromStdString(p.id), value);
      }
      bridge.submit();
      QVERIFY2(bridge.message().isEmpty(), qPrintable(id + ": " + bridge.message()));
      if (entry.effect == Effect::read && entry.presentation == Presentation::capture) {
        QTRY_VERIFY_WITH_TIMEOUT(!bridge.observation().value("running").toBool(), 4000);
        QVERIFY2(!bridge.observation().isEmpty(), qPrintable(id));
        QVERIFY(bridge.review().isEmpty());
      } else {
        QVERIFY2(!bridge.review().isEmpty(), qPrintable(id));
        QVERIFY(bridge.review().value("blocked").toBool());
        QVERIFY(!bridge.confirmReview(true, bridge.review().value("token").toString()));
      }
    }
    QVERIFY(semanticIcons.size() >= 20);
    bridge.selectArea(6);
    QSet<QString> updates;
    for (const auto &group : bridge.groups())
      for (const auto &entry : group.toMap().value("entries").toList())
        updates.insert(entry.toMap().value("id").toString());
    for (const auto &id : {"channel.set", "channel.preview", "channel.status",
                           "update.preview", "update.apply"})
      QVERIFY(updates.contains(id));
  }
  void taskTaxonomyAndCatalogFilters() {
    Bridge bridge(fixture, false);
    QCOMPARE(bridge.areas().size(), 10);
    QSet<QString> areaIds;
    for (const auto &value : bridge.areas()) {
      const auto area = value.toMap();
      const auto index = area.value("index").toInt();
      QVERIFY(!area.value("description").toString().isEmpty());
      QVERIFY(!area.value("detail").toString().isEmpty());
      areaIds.insert(area.value("id").toString());
      bridge.selectArea(index);
      QVERIFY(!bridge.groups().isEmpty());
      for (const auto &groupValue : bridge.groups()) {
        const auto group = groupValue.toMap();
        QVERIFY(!group.value("description").toString().isEmpty());
        for (const auto &entryValue : group.value("entries").toList()) {
          const auto entry = entryValue.toMap();
          QVERIFY(!entry.value("role").toString().isEmpty());
          QVERIFY(!entry.value("access").toString().isEmpty());
          QCOMPARE(QString::fromStdString(action(entry.value("id").toString().toStdString()).id), entry.value("id").toString());
        }
      }
    }
    QCOMPARE(areaIds.size(), 10);
    QSet<QString> catalogIds;
    for (const auto &group : bridge.catalog(""))
      for (const auto &entry : group.toMap().value("entries").toList()) {
        const auto id = entry.toMap().value("id").toString();
        QVERIFY(!catalogIds.contains(id));
        catalogIds.insert(id);
      }
    QCOMPARE(catalogIds.size(), int(actions().size()));
    for (const auto &kind : {"read", "change", "files", "maintenance"}) {
      int count = 0;
      for (const auto &group : bridge.catalog("", -1, kind))
        for (const auto &entry : group.toMap().value("entries").toList()) {
          const auto &a = action(entry.toMap().value("id").toString().toStdString());
          if (QString(kind) == "read") QVERIFY(a.effect == Effect::read);
          if (QString(kind) == "maintenance") QVERIFY(a.effect == Effect::maintenance);
          if (QString(kind) == "files") QVERIFY(a.effect == Effect::local_file || a.effect == Effect::publish);
          if (QString(kind) == "change") QVERIFY(a.effect == Effect::device || a.effect == Effect::packages);
          ++count;
        }
      QVERIFY(count > 0);
    }
    QVERIFY(!bridge.search("şarj").isEmpty());
    bool batteryFound = false;
    for (const auto &entry : bridge.search("battery status"))
      if (entry.toMap().value("id").toString() == "battery.status") batteryFound = true;
    QVERIFY(batteryFound);
    QCOMPARE(bridge.areaForAction("overview.nfc"), 3);
    QCOMPARE(bridge.areaForAction("overview.camera"), 6);
    QCOMPARE(bridge.areaForAction("sensors.once"), 9);
    QCOMPARE(bridge.areaForAction("resize.prepare"), 8);
    QVERIFY(bridge.catalog("no-such-action-98765").isEmpty());
    const auto previous = bridge.section();
    bridge.selectArea(999);
    QCOMPARE(bridge.section(), previous);
  }
  void observationContextAndDraftReset() {
    Bridge bridge(fixture, false);
    QVERIFY(bridge.openAction("charge.mode"));
    bridge.readRelated("preview");
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    QCOMPARE(bridge.observation().value("origin").toString(), QString("preview"));
    QVERIFY(!bridge.observation().value("stale").toBool());
    bridge.setDraft("mode", "firmware");
    QVERIFY(bridge.observation().value("stale").toBool());
    bridge.refresh();
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    QVERIFY(!bridge.observation().value("stale").toBool());
    const auto options = bridge.observation().value("options").toList();
    QCOMPARE(options.size(), 1);
    QCOMPARE(options.first().toMap().value("value").toString(), QString("Firmware policy"));
    bridge.setDraft("mode", "not-a-mode");
    bridge.refresh();
    QVERIFY(bridge.messageError());
    QVERIFY(bridge.observation().value("stale").toBool());
    bridge.resetDraft();
    QVERIFY(bridge.message().isEmpty());
    const auto parameters = bridge.guide().value("parameters").toList();
    QCOMPARE(parameters.first().toMap().value("value").toString(), QString("hold70-80"));
    bridge.readRelated("inspect");
    QCOMPARE(bridge.observation().value("origin").toString(), QString("inspect"));
    bridge.setDraft("mode", "firmware");
    QVERIFY(!bridge.observation().value("stale").toBool());
    bridge.cancelRead();
    bridge.leaveGuide();
    QVERIFY(bridge.observation().isEmpty());
  }
  void topicFactsAndHistoryContract() {
    const auto history=normaliseHistory({QVariantList{200,80.,2},QVariantList{100,90.,2},QVariantList{150,0.,0},QVariantList{200,0.,0},QVariantList{220,0.,2},QVariantList{4000,40.,2}},1,3600);
    QVERIFY(history.value("available").toBool());
    QCOMPARE(history.value("points").toInt(),3);
    QCOMPARE(history.value("gaps").toInt(),2);
    QCOMPARE(history.value("segments").toList().size(),3);
    QCOMPARE(history.value("segments").toList().last().toList().first().toMap().value("value").toDouble(),0.);
    QVERIFY(!normaliseHistory({QVariantList{100,0.,0},QVariantList{110,-1.,2}},1,3600).value("available").toBool());
    QVERIFY(!normaliseHistory({},3600,1).value("available").toBool());
    QJsonObject readings{{"battery.capacity",QJsonObject{{"value",0},{"unit","percent"},{"errno",0}}},
                         {"battery.current_now",QJsonObject{{"value",-250000},{"unit","uA_positive_into_battery"},{"errno",0}}}};
    auto data=QJsonObject{{"provider_verified",true},{"snapshot_coherent",true},{"readings",readings}};
    auto rows=informationRows("battery.status",data);
    QCOMPARE(rows.first().toMap().value("value").toString(),QString("0 %"));
    QCOMPARE(rows.first().toMap().value("progress").toDouble(),0.);
    QVERIFY(rows[3].toMap().value("value").toString().startsWith("-250"));
    data["provider_verified"]=false;
    rows=informationRows("battery.status",data);
    QCOMPARE(rows.first().toMap().value("progress").toDouble(),-1.);
    QVERIFY(!rows.first().toMap().value("value").toString().startsWith("0"));
    Bridge bridge(fixture,false);
    auto *settings=qobject_cast<Settings *>(bridge.settings());
    QVERIFY(settings);
    QVERIFY(settings->choices("interface").isEmpty());
    QCOMPARE(settings->topics(1).size(),5);
    QVERIFY(!settings->topics(-1,"pil").isEmpty());
    QVERIFY(!settings->topics(-1,"charge.mode").isEmpty());
    QVERIFY(!settings->open("unknown-topic"));
    QVERIFY(settings->open("charging"));
    settings->stop();
    QVERIFY(!settings->busy());
    QTest::qWait(100);
    QVERIFY(!settings->busy());
    QVERIFY(bridge.openAction("flashlight.level"));
    QCOMPARE(bridge.guide().value("parameters").toList().first().toMap().value("choices").toList().size(),3);
  }
  void kernelInformationAndNdef() {
    const QJsonObject dock{{"kernel_managed",true},{"ports",QJsonArray{QJsonObject{{"name","port0"},{"data_role","[host] device"},{"power_role","source [sink]"},{"partner",true}}}},
      {"devices",QJsonArray{QJsonObject{{"name","2-1"},{"product","USB hub"},{"speed","5000"},{"version","3.20"}}}},
      {"networks",QJsonArray{QJsonObject{{"name","en-test"},{"speed","1000"},{"statistics/rx_bytes","4096"}}}},
      {"displays",QJsonArray{QJsonObject{{"name","card2-DP-1"},{"status","connected"},{"modes","2560x1440\n1920x1080"}}}}};
    const auto sections=kernelInformationSections("kernel.dock",dock);
    QCOMPARE(sections.size(),5);
    const auto port=sections.first().toMap().value("rows").toList();
    QCOMPARE(port[1].toMap().value("value").toString(),QString("host"));
    QCOMPARE(port[2].toMap().value("value").toString(),QString("sink"));
    QVERIFY(sections[2].toMap().value("rows").toList()[1].toMap().value("value").toString().contains("5 Gbit/s"));
    // An unqualified profile must not hide successfully read firmware settings
    // or turn a disabled cap into a claim of physical cutoff.
    const auto charging=kernelInformationSections("kernel.charging",QJsonObject{
      {"settings",QJsonObject{{"firmware_get_complete",true},{"exit_status",0},{"reported_enabled",false},{"reported_start_percent",70},{"reported_end_percent",80}}},
      {"admission",QJsonObject{{"reason","control_profile_unqualified"},{"threshold_admission_ready",false}}}});
    QCOMPARE(charging.size(),2);
    QVERIFY(charging.first().toMap().value("rows").toList()[3].toMap().value("value").toString().contains("80"));
    auto rich=fixture;rich.results["kernel.charging"]=QJsonObject{{"data",QJsonObject{{"admission",QJsonObject{{"threshold_admission_ready",false},{"reason","control_profile_unqualified"}}}}}};
    Settings settings(rich);QVERIFY(settings.open("charging"));QTRY_VERIFY(!settings.busy());
    QVERIFY(!settings.topic().value("canApply").toBool());
    QVERIFY(settings.topic().value("changes").toList().size()>=2);
    rich.results["kernel.dock"]=QJsonObject{{"data",dock}};
    Bridge roleBridge(rich,false);auto *roleSettings=qobject_cast<Settings *>(roleBridge.settings());
    QVERIFY(roleSettings->open("usb"));QTRY_VERIFY(!roleSettings->busy());
    QVERIFY(roleBridge.openAction("usb.role"));QCOMPARE(roleBridge.draftValues().value("role").toString(),QString("host"));
    NfcReader reader(true);reader.start();reader.startService();QVERIFY(!reader.report().value("busy").toBool());reader.clear();
    NfcReader restricted(false,nullptr,true);restricted.startService();QVERIFY(!restricted.report().value("busy").toBool());
    QNdefNfcTextRecord record;record.setText("<script>alert(1)</script>");
    QNdefNfcUriRecord uri;uri.setUri(QUrl("file:///private/example"));
    QNdefMessage message;message<<record<<uri;
    const auto rows=ndefRows(message);QCOMPARE(rows.size(),2);
    QCOMPARE(rows[0].toMap().value("value").toString(),QString("<script>alert(1)</script>"));
    QVERIFY(rows[1].toMap().value("title").toString().contains("display only"));
    QNdefRecord oversized;oversized.setPayload(QByteArray(5000,'x'));message.clear();message<<oversized;
    QCOMPARE(ndefRows(message).size(),1);
    QVERIFY(ndefRows(message).first().toMap().value("title").toString().contains("limit"));
  }
  void waydroidPagePreservesDataAndBlocksSimulationWrites() {
    auto sample = fixture;
    sample.results["waydroid.installer"] = QJsonObject{{"ready", false}, {"initialized", false},
        {"variant", ""}, {"kernel_ready", true}, {"busy", false}, {"phase", "idle"}, {"progress", -1}};
    Bridge bridge(sample, false);
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &bridge);
    engine.rootContext()->setContextProperty("themeIcons", &themeIcons);
    engine.rootContext()->setContextProperty("styleNotice", QString());
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &errors) {
      for (const auto &error : errors) warnings << error.toString();
    });
    engine.loadFromModule("org.uke.ControlQtTest", "Main");
    QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window); window->resize(480, 800); window->show();
    QVERIFY(QMetaObject::invokeMethod(window, "openWaydroid"));
    QQuickItem *page = nullptr;
    QTRY_VERIFY((page = visualItem(window->contentItem(), "waydroidPage")));
    auto *manager = qobject_cast<WaydroidManager *>(bridge.waydroid());
    QVERIFY(manager); QVERIFY(manager->blocked());
    auto *install = visualItem(window->contentItem(), "installWaydroid");
    QVERIFY(install); QVERIFY(!install->isEnabled());
    QVERIFY(page->setProperty("selectedEdition", "VANILLA"));
    manager->refresh(); QCoreApplication::processEvents();
    QCOMPARE(page->property("selectedEdition").toString(), QString("VANILLA"));
    QVERIFY(!manager->install("VANILLA"));
    QVERIFY(!manager->launch());
    snapshot(window, "waydroid-installer");
    QCOMPARE(page->window(), window);
    // Qt6.12 also emits this transient reparenting warning for the retained
    // AreaPage. Require actual scene attachment and reject every other warning.
    for (const auto &warning : warnings)
      QVERIFY2(warning.endsWith("Created graphical object was not placed in the graphics scene."), qPrintable(warning));
  }
  void qmlSettingsTopics() {
    auto rich=fixture;
    rich.results["manager.sample"]=QJsonObject{{"data",managerSample()}};
    rich.results["sensors.once"]=QJsonObject{{"data",managerSensors()}};
    for(const auto &t:settingTopics())for(const auto &id:t.reads)
      if(!rich.results.contains(id))rich.results[id]=QJsonObject{{"data",QJsonObject{{"available",true},{"status","Synthetic reported state"}}}};
    auto battery=QJsonObject{{"provider_verified",true},{"snapshot_coherent",true},
      {"readings",QJsonObject{{"battery.capacity",QJsonObject{{"value",72},{"unit","percent"},{"errno",0}}}}},
      {"history_end",10000},{"history",QJsonArray{QJsonArray{1000,80,2},QJsonArray{1300,78,2},QJsonArray{1400,0,0},QJsonArray{1600,77,2},QJsonArray{1900,76,2}}}};
    rich.results["battery.status"]=QJsonObject{{"data",battery}};
    rich.results["charge.status"]=QJsonObject{{"data",QJsonObject{{"reported_start_percent",70},{"reported_end_percent",80},{"configuration_readback_verified",true}}}};
    Bridge bridge(rich,false);
    QQmlApplicationEngine engine;KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend",&bridge);
    engine.rootContext()->setContextProperty("themeIcons",&themeIcons);
    engine.rootContext()->setContextProperty("styleNotice",QString());
    QStringList warnings;
    connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors)warnings<<e.toString();});
    engine.loadFromModule("org.uke.ControlQtTest","Main");
    QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());QVERIFY(window);
    window->resize(480,800);window->show();
    auto *settings=qobject_cast<Settings *>(bridge.settings());
    for(const auto &topic:settingTopics()) {
      QVERIFY(QMetaObject::invokeMethod(window,"showArea",Q_ARG(QVariant,topic.area)));
      QVERIFY(QMetaObject::invokeMethod(window,"openTopic",Q_ARG(QVariant,topic.id)));
      QTRY_VERIFY_WITH_TIMEOUT(!settings->busy(),5000);
      QQuickItem *page=nullptr;QTRY_VERIFY((page=visualItem(window->contentItem(),"topicPage")));
      QCOMPARE(page->property("topicId").toString(),topic.id);
      QVERIFY(bridge.review().isEmpty());
      if(topic.id=="battery") {
        settings->loadHistory();QVERIFY(settings->history().value("available").toBool());
        snapshot(window,"settings-battery");
      }
      if(topic.id=="flashlight") {
        QTRY_VERIFY(visualItem(window->contentItem(),"parameter_percent"));
        QVERIFY(!visualItem(window->contentItem(),"topicAction_flashlight.level"));
        snapshot(window,"settings-flashlight");
      }
      if(topic.id=="kernel")snapshot(window,"settings-kernel");
      if(topic.id=="nfc") {QVERIFY(visualItem(window->contentItem(),"nfcInspect"));snapshot(window,"settings-nfc");}
      if(topic.id=="usb") {QVERIFY(visualItem(window->contentItem(),"usbAutomatic"));snapshot(window,"settings-dock");}
      if(topic.id=="cpu"||topic.id=="gpu"||topic.id=="sensors") {
        QVERIFY(visualItem(window->contentItem(),"kernelMonitoring"));
        snapshot(window,"settings-"+topic.id);
        if(topic.id=="gpu") {
          // Keep earlier pages in PageRow, rather than clearing navigation.
          QVERIFY(QMetaObject::invokeMethod(window,"openTopic",Q_ARG(QVariant,QString("cpu"))));
          QVERIFY(QMetaObject::invokeMethod(window,"openTopic",Q_ARG(QVariant,QString("gpu"))));
          QTest::qWait(100);
          QVERIFY(window->findChildren<QObject *>("frequencyReview").size()>=2);
          auto *manager=qobject_cast<KernelManager *>(settings->manager());QVERIFY(manager);
          QVERIFY(manager->prepare("gpu:gpu0","maximum","200000000"));
          auto visibleReviews=[&]{int count=0;for(auto *object:window->findChildren<QObject *>("frequencyReview"))if(object->property("visible").toBool())++count;return count;};
          QTRY_COMPARE(visibleReviews(),1);
          QVERIFY(!manager->apply(manager->review().value("token").toString()));
          manager->dismissReview();QTRY_COMPARE(visibleReviews(),0);
        }
      }
    }
    QVERIFY(QMetaObject::invokeMethod(window,"showArea",Q_ARG(QVariant,0)));snapshot(window,"settings-home");
    QVERIFY(QMetaObject::invokeMethod(window,"openSearch"));
    QQuickItem *search=nullptr;QTRY_VERIFY((search=visualItem(window->contentItem(),"topicSearch")));
    search->setProperty("text","kernel");snapshot(window,"settings-search");
    window->close();
    warnings.removeIf([](const QString &w){return w.contains("Main.qml:")&&w.endsWith("Created graphical object was not placed in the graphics scene.");});
    QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
  }
  void optionalDeviceReads() {
    if (!qEnvironmentVariableIsSet("UKE_QT_TEST_DEVICE_READS"))
      QSKIP("Live read-only device checks require explicit opt-in");
    Bridge bridge({}, true);
    for (const auto *id : {"battery.status", "charge.status", "channel.status",
                           "microphone.status", "update.status"}) {
      QVERIFY(bridge.openAction(id));
      bridge.submit();
      QVERIFY2(bridge.message().isEmpty(), qPrintable(bridge.message()));
      QTRY_VERIFY_WITH_TIMEOUT(!bridge.observation().value("running").toBool(), 30000);
      QVERIFY(!bridge.observation().isEmpty());
      const auto status = bridge.observation().value("exitStatus").toInt();
      // These installed providers must report successfully. Individual device
      // values and physical acceptance may still be unavailable or unknown.
      QCOMPARE(status, 0);
      if (QString(id) == "update.status") {
        const auto report = bridge.observation().value("bootReport").toMap();
        QVERIFY(!report.isEmpty());
        const auto original=QJsonDocument::fromJson(bridge.observation().value("raw").toString().toUtf8()).object().toVariantMap();
        QCOMPARE(report,original);
        if(!report.value("complete").toBool())QVERIFY(!report.value("issues").toList().isEmpty());
        QCOMPARE(report.value("slots").toList().size(), 2);
        QVERIFY(report.value("current_slot").isNull() || report.value("current_slot").toString() == "a" || report.value("current_slot").toString() == "b");
      }
      qInfo("Device read completed: %s", id);
    }
    auto *settings=qobject_cast<Settings *>(bridge.settings());
    QVERIFY(settings->open("battery"));QTRY_VERIFY_WITH_TIMEOUT(!settings->busy(),30000);
    settings->loadHistory();QTRY_VERIFY_WITH_TIMEOUT(!settings->history().value("busy").toBool(),30000);
    QVERIFY2(settings->history().value("available").toBool(),qPrintable(settings->history().value("message").toString()));
    QVERIFY(settings->history().value("points").toInt()>=2);
    qInfo("Existing UPower history completed with valid points and explicit gaps");
    settings->close();
    // Render actual installed read-only pages, separately from simulated scenes.
    QQmlApplicationEngine engine;KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend",&bridge);
    engine.rootContext()->setContextProperty("themeIcons",&themeIcons);
    engine.rootContext()->setContextProperty("styleNotice",QString());
    QStringList warnings;
    connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors)warnings<<e.toString();});
    engine.loadFromModule("org.uke.ControlQtTest","Main");
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());QVERIFY(window);
    window->resize(720,960);window->show();window->requestActivate();QTest::qWait(250);
    for(const auto *topic:{"usb","charging","nfc","cpu","gpu","sensors"}) {
      const QString id(topic);const int area=id=="charging"?1:id=="cpu"||id=="gpu"||id=="sensors"?9:3;
      QVERIFY(QMetaObject::invokeMethod(window,"showArea",Q_ARG(QVariant,area)));
      QVERIFY(QMetaObject::invokeMethod(window,"openTopic",Q_ARG(QVariant,QString(topic))));
      QTRY_VERIFY_WITH_TIMEOUT(!settings->busy(),30000);
      QVERIFY2(settings->notice().isEmpty(),qPrintable(settings->notice()));
      if(id!="cpu"&&id!="gpu"&&id!="sensors")QVERIFY(!settings->information().isEmpty());
      if(id=="cpu"||id=="gpu"||id=="sensors") {
        auto *manager=qobject_cast<KernelManager *>(settings->manager());QVERIFY(manager);
        QTRY_VERIFY_WITH_TIMEOUT(!manager->busy(),30000);
        if(id=="sensors")QTRY_VERIFY_WITH_TIMEOUT(!manager->sensors().isEmpty(),30000);else QTRY_VERIFY_WITH_TIMEOUT(!manager->devices().isEmpty(),30000);
        if(id=="sensors"){manager->setInterval(1);manager->start();}
        QTest::qWait(2200);manager->refresh();QTRY_VERIFY_WITH_TIMEOUT(!manager->busy(),30000);
        manager->pause();
      }
      if(QString(topic)=="nfc")qobject_cast<NfcReader *>(settings->nfc())->inspect();
      snapshot(window,"device-"+QString(topic));
      qInfo("Installed kernel information page completed: %s",topic);
    }
    window->close();
    warnings.removeIf([](const QString &w){return w.contains("Main.qml:")&&w.endsWith("Created graphical object was not placed in the graphics scene.");});
    QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
  }
  void typedReadsAndCancellation() {
    Bridge bridge(fixture, false);
    QVERIFY(bridge.openAction("status"));
    bridge.submit();
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    QMap<QString, QString> values;
    for (const auto &entry : bridge.observation().value("rows").toList()) {
      const auto row = entry.toMap();
      values[row.value("key").toString()] = row.value("value").toString();
    }
    QCOMPARE(values["unknown"], "Unknown (not reported)");
    QCOMPARE(values["cycles"], "0");
    QCOMPARE(values["current_ma"], "-42");
    QVERIFY(bridge.openAction("charge.status"));
    bridge.submit();
    QVERIFY(bridge.observation().value("running").toBool());
    bridge.cancelRead();
    QVERIFY(bridge.observation().value("cancelled").toBool());
    bridge.refresh();
    QVERIFY(bridge.observation().value("running").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!bridge.observation().value("running").toBool(),
                             4000);
    QCOMPARE(bridge.observation().value("exitStatus").toInt(), 69);
    QVERIFY(bridge.observation().value("error").toString().contains(
        "Synthetic provider error"));
    QVERIFY(!bridge.observation().value("raw").toString().contains(
        "Synthetic provider error"));
    for (int i = 0; i < 4; ++i) {
      bridge.refresh();
      bridge.leaveGuide();
      QVERIFY(bridge.observation().isEmpty());
    }
  }
  void frozenReviewAndAdmission() {
    Bridge bridge(fixture, false);
    QVERIFY(!bridge.confirmReview(true, ""));
    QVERIFY(bridge.openAction("charge.mode"));
    bridge.submit();
    QVERIFY(bridge.review().value("blocked").toBool());
    QVERIFY(
        !bridge.confirmReview(true, bridge.review().value("token").toString()));
    bridge.setDraft("mode", "firmware");
    QVERIFY(bridge.review().isEmpty());
    bridge.submit();
    QVERIFY(bridge.review().value("command").toString().contains("firmware"));
    bridge.setDraft("mode", "not-a-mode");
    bridge.submit();
    QVERIFY(!bridge.message().isEmpty());
    QVERIFY(bridge.review().isEmpty());
    QVERIFY(!bridge.confirmReview(true, ""));
    Bridge readOnly({}, true);
    QVERIFY(readOnly.openAction("charge.mode"));
    readOnly.submit();
    QVERIFY(readOnly.review().value("blocked").toBool());
    QVERIFY(!readOnly.confirmReview(true, ""));
    QVERIFY(bridge.openAction("update.apply"));
    bridge.submit();
    QCOMPARE(bridge.review().value("token").toString(), "APPLY");
    QVERIFY(!bridge.confirmReview(true, "apply"));
    QVERIFY(!bridge.confirmReview(false, "APPLY"));
    QVERIFY(!bridge.confirmReview(
        true, "APPLY")); // fixture remains blocked even with exact consent
    auto unavailable = fixture;
    unavailable.results.insert("status", QJsonObject{{"data", QJsonObject{{"available", false}}}});
    Bridge unavailableBridge(unavailable, false);
    QVERIFY(unavailableBridge.openAction("status"));
    unavailableBridge.submit();
    QTRY_VERIFY(!unavailableBridge.observation().value("running").toBool());
    QCOMPARE(unavailableBridge.observation().value("state").toString(), QString("Provider unavailable"));
    QCOMPARE(unavailableBridge.observation().value("icon").toString(), QString("dialog-warning"));
  }
  void startupGuideRemainsVisible() {
    Bridge bridge(fixture, false);
    QVERIFY(bridge.openAction("charge.mode"));
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &bridge);
    engine.rootContext()->setContextProperty("themeIcons", &themeIcons);
    engine.rootContext()->setContextProperty("styleNotice", QString());
    engine.loadFromModule("org.uke.ControlQtTest", "Main");
    QVERIFY(!engine.rootObjects().isEmpty());
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window);
    QTest::qWait(300);
    QCOMPARE(bridge.guide().value("id").toString(), QString("charge.mode"));
    auto *guide = visualItem(window->contentItem(), "guidePage");
    QVERIFY(guide);
    QVERIFY(guide->isVisible());
    window->close();
  }
  void qmlNavigationAndEveryGuide() {
    Bridge bridge(fixture, false);
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty("backend", &bridge);
    engine.rootContext()->setContextProperty("themeIcons", &themeIcons);
    engine.rootContext()->setContextProperty("styleNotice", QString());
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, this,
            [&](const QList<QQmlError> &errors) {
              for (const auto &error : errors)
                warnings << error.toString();
            });
    engine.loadFromModule("org.uke.ControlQtTest", "Main");
    QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QVERIFY(window);
    window->resize(480, 720);
    window->show();
    QTest::qWait(100);
    auto *drawer = window->findChild<QObject *>("navigationDrawer");
    QVERIFY(drawer);
    auto *menuIcon = drawer->property("handleClosedIcon").value<QObject *>();
    auto *closeIcon = drawer->property("handleOpenIcon").value<QObject *>();
    QVERIFY(menuIcon);
    QVERIFY(closeIcon);
    QCOMPARE(menuIcon->property("name").toString(), themeIcons.names().value("application-menu").toString());
    QCOMPARE(closeIcon->property("name").toString(), themeIcons.names().value("window-close").toString());
    QVERIFY(QMetaObject::invokeMethod(drawer, "open"));
    QTRY_VERIFY(drawer->property("position").toReal() > 0.99);
    snapshot(window, "navigation-drawer");
    QVERIFY(QMetaObject::invokeMethod(drawer, "close"));
    QTRY_VERIFY(drawer->property("position").toReal() < 0.01);
    QCOMPARE(bridge.search("").size(), int(actions().size()));
    QCOMPARE(bridge.search("no-such-action-98765").size(), 0);
    for (int area = 0; area < bridge.areas().size(); ++area) {
      QVERIFY(QMetaObject::invokeMethod(window, "showArea", Q_ARG(QVariant, area)));
      QCoreApplication::processEvents();
      QCOMPARE(bridge.section(), area);
      snapshot(window, "area-" + QString::number(area));
    }
    QVERIFY(QMetaObject::invokeMethod(window, "showArea", Q_ARG(QVariant, 0)));
    QCoreApplication::processEvents();
    QQuickItem *categoryCard = nullptr;
    QTRY_VERIFY((categoryCard = visualItem(window->contentItem(), "category_power")));
    QVERIFY(QMetaObject::invokeMethod(categoryCard, "clicked"));
    QCOMPARE(bridge.section(), 1);
    QVERIFY(QMetaObject::invokeMethod(window, "openCatalog", Q_ARG(QVariant, -1)));
    QCoreApplication::processEvents();
    QQuickItem *catalog = nullptr;
    QTRY_VERIFY((catalog = visualItem(window->contentItem(), "catalogPage")));
    QTRY_COMPARE(catalog->property("resultCount").toInt(), int(actions().size()));
    auto *search = visualItem(window->contentItem(), "advancedSearch");
    QVERIFY(search);
    search->setProperty("text", "no-such-action-98765");
    QTRY_COMPARE(catalog->property("resultCount").toInt(), 0);
    auto *reset = visualItem(window->contentItem(), "resetCatalog");
    QVERIFY(reset);
    QVERIFY(QMetaObject::invokeMethod(reset, "clicked"));
    QTRY_COMPARE(catalog->property("resultCount").toInt(), int(actions().size()));
    search->setProperty("text", "charge.mode");
    snapshot(window, "catalog-filtered");
    for (const auto &entry : actions()) {
      QVERIFY(QMetaObject::invokeMethod(
          window, "openGuide",
          Q_ARG(QVariant, QString::fromStdString(entry.id))));
      QCoreApplication::processEvents();
      QTest::qWait(20);
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
      QCOMPARE(bridge.guide().value("id").toString(),
               QString::fromStdString(entry.id));
      QCOMPARE(window->width(), 480);
      const auto guides = window->findChildren<QQuickItem *>("guidePage");
      QVERIFY2(!guides.isEmpty(), entry.id.c_str());
      QVERIFY2(guides.size() <= 2,
               qPrintable(QString("%1 guide pages retained for %2")
                              .arg(guides.size())
                              .arg(QString::fromStdString(entry.id))));
      bool inScene = false;
      for (const auto *guide : guides)
        if (guide->window() == window && guide->isVisible())
          inScene = true;
      QVERIFY2(inScene, entry.id.c_str());
      QVERIFY(bridge.observation().isEmpty());
      QVERIFY(bridge.review().isEmpty());
      QQuickItem *submit = nullptr;
      // Kirigami disables pages during transitions. Reacquire the visible
      // control while waiting: the outgoing page may be destroyed meanwhile.
      QTRY_VERIFY_WITH_TIMEOUT(
          (submit = visualItem(window->contentItem(), "submitRequest")) &&
              submit->isEnabled(), 1500);
      auto *submitAccess = QAccessible::queryAccessibleInterface(submit);
      QVERIFY(submitAccess);
      QVERIFY(!submitAccess->text(QAccessible::Name).isEmpty());
      if (entry.id == "channel.set" || entry.id == "update.apply")
        snapshot(window, QString::fromStdString(entry.id));
    }
    QVERIFY(QMetaObject::invokeMethod(window, "openCatalog", Q_ARG(QVariant, -1)));
    QCoreApplication::processEvents();
    QVERIFY(bridge.guide().isEmpty());
    QTRY_VERIFY((search = visualItem(window->contentItem(), "advancedSearch")));
    QCOMPARE(search->property("text").toString(), QString("charge.mode"));
    search->setProperty("text", "no-such-action-98765");
    QCoreApplication::processEvents();
    QCOMPARE(search->property("text").toString(), QString("no-such-action-98765"));
    search->setProperty("text", "");
    QVERIFY(QMetaObject::invokeMethod(window, "openGuide", Q_ARG(QVariant, QString("status"))));
    QCoreApplication::processEvents();
    QQuickItem *submit = nullptr;
    QTRY_VERIFY((submit = visualItem(window->contentItem(), "submitRequest")) && submit->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(submit, "clicked"));
    QVERIFY(bridge.observation().value("running").toBool());
    QVERIFY(!submit->isEnabled());
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    snapshot(window, "read-complete");
    auto *refresh = visualItem(window->contentItem(), "refreshRead");
    QVERIFY(refresh);
    QVERIFY(QMetaObject::invokeMethod(refresh, "clicked"));
    QVERIFY(bridge.observation().value("running").toBool());
    auto *stop = visualItem(window->contentItem(), "stopRead");
    QVERIFY(stop);
    QVERIFY(QMetaObject::invokeMethod(stop, "clicked"));
    QVERIFY(bridge.observation().value("cancelled").toBool());
    QVERIFY(QMetaObject::invokeMethod(window, "openGuide",
                                      Q_ARG(QVariant, QString("charge.mode"))));
    bridge.submit();
    QCoreApplication::processEvents();
    auto *review = window->findChild<QObject *>("requestReview");
    QVERIFY(review);
    QVERIFY(review->property("visible").toBool());
    QQuickItem *mode = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(
        (mode = visualItem(window->contentItem(), "parameter_mode")), 2000);
    auto *accessible = QAccessible::queryAccessibleInterface(mode);
    QVERIFY(accessible);
    QVERIFY(!accessible->text(QAccessible::Name).isEmpty());
    auto *launch = review->findChild<QObject *>("launchTerminal");
    QVERIFY(launch);
    QVERIFY(!launch->property("enabled").toBool());
    snapshot(window, "review-charge");
    QVERIFY(QMetaObject::invokeMethod(review, "reject"));
    QCoreApplication::processEvents();
    QVERIFY(bridge.review().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(window, "openGuide", Q_ARG(QVariant, QString("update.status"))));
    bridge.submit();
    QTRY_VERIFY(!bridge.observation().value("running").toBool());
    QQuickItem *bootCurrent = nullptr;
    QTRY_VERIFY((bootCurrent = visualItem(window->contentItem(), "bootCurrent")));
    QVERIFY(bootCurrent->property("description").toString().contains("Slot B"));
    auto *pending = visualItem(window->contentItem(), "bootPending");
    QVERIFY(pending);
    QCOMPARE(pending->property("description").toString(), QString("No"));
    snapshot(window, "kernel-slot-status");
    QVERIFY(QMetaObject::invokeMethod(window, "openGuide", Q_ARG(QVariant, QString("boot.set-slot"))));
    QCoreApplication::processEvents();
    snapshot(window, "slot-selection");
    QVERIFY(QMetaObject::invokeMethod(window, "showArea", Q_ARG(QVariant, 0)));
    window->resize(1100, 760);
    QTest::qWait(100);
    snapshot(window, "overview-wide");
    window->close();
    bridge.cancelRead();
    // PageRow creates a page under its nonvisual pagesLogic object before
    // inserting it into ColumnView (upstream PageRow::initPage). Qt may warn
    // about that temporary parent. Every guide above must actually enter this
    // window's scene; all other QML warnings remain failures.
    warnings.removeIf([](const QString &warning) {
      return warning.contains("Main.qml:") &&
             (warning.contains("QML GuidePage:") ||
              warning.contains("QML AreaPage:") ||
              warning.contains("QML CatalogPage:")) &&
             warning.endsWith("Created graphical object was not placed in the "
                              "graphics scene.");
    });
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
  }
};
int main(int argc, char **argv) {
  KIconTheme::initTheme();
  QApplication app(argc, argv);
  KLocalizedString::setApplicationDomain("uke-control-qt");
  KStyleManager::initStyle();
  QQuickStyle::setStyle("org.kde.desktop");
  qmlRegisterType<HistoryPlot>("org.uke.settings",1,0,"HistoryPlot");
  qmlRegisterType<TimePlot>("org.uke.settings",1,0,"TimePlot");
  KirigamiTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "kirigami.moc"
