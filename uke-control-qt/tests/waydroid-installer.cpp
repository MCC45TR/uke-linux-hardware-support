// SPDX-License-Identifier: MIT
#include "../src/waydroid-install.hpp"
#include "../src/waydroid-manager.hpp"
#include <QtTest>
#include <QtEndian>
#include <optional>
using namespace uq::waydroid;
struct FakeSystem final : System {
  Snapshot snapshot;
  QList<Step> calls;
  QList<bool> repairs;
  QString phase, error;
  bool kernel = true, space = true, completeDownload = true;
  std::optional<Step> failing;
  Snapshot inspect() override { return snapshot; }
  bool kernelReady() override { return kernel; }
  bool enoughSpace() override { return space; }
  void report(const QString &p, const QString &e = {}) override { phase = p; error = e; }
  bool run(Step step, const QString &variant, bool repair) override {
    calls << step; repairs << repair;
    if (failing == step) return false;
    if (step == Step::Packages) { snapshot.installed = true; snapshot.integration = true; }
    if (step == Step::Initialize) {
      snapshot.initialized = true; snapshot.imagesComplete = completeDownload; snapshot.variant = variant;
    }
    if (step == Step::Enable) snapshot.serviceEnabled = true;
    return true;
  }
};
class InstallerTests : public QObject {
  Q_OBJECT
private slots:
  void freshEditions_data() {
    QTest::addColumn<QString>("edition");
    QTest::newRow("Google Play") << QString("GAPPS");
    QTest::newRow("no Google") << QString("VANILLA");
  }
  void freshEditions() {
    QFETCH(QString, edition);
    FakeSystem s;
    QVERIFY(install(s, edition));
    QCOMPARE(s.snapshot.variant, edition);
    QVERIFY(s.snapshot.ready());
    QCOMPARE(s.phase, QString("ready"));
    QCOMPARE(s.calls, QList<Step>({Step::Packages, Step::Initialize, Step::Prepare, Step::Enable}));
    QVERIFY(!s.repairs[1]);
  }
  void existingInstallationIsUntouched() {
    FakeSystem s;
    s.snapshot = {true, true, true, true, true, "GAPPS"};
    QVERIFY(install(s, "GAPPS"));
    QVERIFY(s.calls.isEmpty());
    QVERIFY(!install(s, "VANILLA"));
    QVERIFY(s.calls.isEmpty());
    QCOMPARE(s.error, QString("variant-conflict"));
    QCOMPARE(s.snapshot.variant, QString("GAPPS"));
    FakeSystem custom;
    custom.snapshot.managed = false;
    QVERIFY(!install(custom, "GAPPS")); QVERIFY(custom.calls.isEmpty());
    QCOMPARE(custom.error, QString("custom-installation"));
    FakeSystem unknown;
    unknown.snapshot.variant = "OTHER";
    QVERIFY(!install(unknown, "GAPPS")); QVERIFY(unknown.calls.isEmpty());
    QCOMPARE(unknown.error, QString("variant-conflict"));
  }
  void partialInstallationResumesWithoutErasingData() {
    FakeSystem s;
    s.snapshot.initialized = true; s.snapshot.variant = "VANILLA";
    QVERIFY(install(s, "VANILLA"));
    QVERIFY(s.repairs[1]);
    QCOMPARE(s.snapshot.variant, QString("VANILLA"));
  }
  void zeroExitIsNotInstallationSuccess() {
    FakeSystem s;
    s.completeDownload = false;
    QVERIFY(!install(s, "GAPPS"));
    QCOMPARE(s.error, QString("images-incomplete"));
    QCOMPARE(s.calls, QList<Step>({Step::Packages, Step::Initialize}));
    QVERIFY(!s.snapshot.serviceEnabled);
  }
  void failuresDoNotRunLaterSteps_data() {
    QTest::addColumn<int>("step");
    for (int n = 0; n != 4; ++n) QTest::newRow(qPrintable(QString::number(n))) << n;
  }
  void failuresDoNotRunLaterSteps() {
    QFETCH(int, step);
    FakeSystem s; s.failing = Step(step);
    QVERIFY(!install(s, "GAPPS"));
    QCOMPARE(s.calls.size(), step + 1);
    QCOMPARE(s.phase, QString("failed"));
    QVERIFY(!s.error.isEmpty());
  }
  void prerequisitesRefuseBeforeAnyMutation() {
    for (const auto &edition : {QString("gapps"), QString("../../etc/passwd"), QString("GAPPS; id"), QString()}) {
      FakeSystem s;
      QVERIFY(!install(s, edition)); QVERIFY(s.calls.isEmpty());
    }
    FakeSystem noKernel; noKernel.kernel = false;
    QVERIFY(!install(noKernel, "GAPPS")); QVERIFY(noKernel.calls.isEmpty());
    QCOMPARE(noKernel.error, QString("system-update-required"));
    FakeSystem noSpace; noSpace.space = false;
    QVERIFY(!install(noSpace, "VANILLA")); QVERIFY(noSpace.calls.isEmpty());
    QCOMPARE(noSpace.error, QString("insufficient-space"));
  }
  void completeImageRejectsTruncationAndSymlinks() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("image");
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadWrite));
    QVERIFY(f.resize(16 * 1024 * 1024)); // Sparse synthetic metadata, never a device image.
    QByteArray super(1024, 0);
    qToLittleEndian<quint16>(0xef53, super.data() + 0x38);
    qToLittleEndian<quint32>(4096, super.data() + 4);
    qToLittleEndian<quint32>(2, super.data() + 0x18);
    QVERIFY(f.seek(1024)); QCOMPARE(f.write(super), qint64(1024)); QVERIFY(f.flush());
    QVERIFY(completeImage(path));
    QVERIFY(QFile::link(path, directory.filePath("link")));
    QVERIFY(!completeImage(directory.filePath("link")));
    QVERIFY(f.resize(8 * 1024 * 1024)); QVERIFY(f.flush());
    QVERIFY(!completeImage(path));
    QVERIFY(f.resize(16 * 1024 * 1024));
    qToLittleEndian<quint32>(0xffffffff, super.data() + 4);
    qToLittleEndian<quint32>(0x80, super.data() + 0x60);
    qToLittleEndian<quint32>(0xffffffff, super.data() + 0x150);
    QVERIFY(f.seek(1024)); QCOMPARE(f.write(super), qint64(1024)); QVERIFY(f.flush());
    QVERIFY(!completeImage(path));
  }
  void fixtureAndReadOnlyNeverLaunchCommands() {
    uq::Fixture fixture; fixture.enabled = true;
    fixture.results.insert("waydroid.installer", QJsonObject{{"ready", true}, {"kernel_ready", true},
      {"variant", "GAPPS"}, {"phase", "ready"}, {"busy", false}});
    uq::WaydroidManager sample(fixture, false);
    sample.open();
    QVERIFY(sample.state().value("ready").toBool());
    QVERIFY(!sample.install("GAPPS"));
    QVERIFY(!sample.launch());
    QVERIFY(!sample.readGoogleId());
    QVERIFY(!sample.openGoogleRegistration());
    QVERIFY(sample.googleId().isEmpty());
    sample.close();
    uq::WaydroidManager readOnly(fixture, true);
    readOnly.refresh();
    QVERIFY(!readOnly.install("VANILLA"));
    QVERIFY(!readOnly.launch());
    QVERIFY(!readOnly.readGoogleId());
  }
};
QTEST_GUILESS_MAIN(InstallerTests)
#include "waydroid-installer.moc"
