// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QQuickPaintedItem>
#include <QColor>
#include <functional>
#include <QProcess>
#include "nfc-reader.hpp"
#include "charge-telemetry.hpp"
#include "kernel-manager.hpp"
class QNetworkAccessManager;
class QNetworkReply;

namespace uq {
struct Topic {
  QString id, title, subtitle, icon;
  int area;
  QStringList reads, changes;
};
const QList<Topic> &settingTopics();
QVariantMap topicMap(const Topic &topic);
QVariantList informationRows(const QString &id, const QJsonObject &data);
QVariantList kernelInformationSections(const QString &id, const QJsonObject &data);
QVariantMap normaliseHistory(const QVariantList &records, qint64 start, qint64 end);
class Settings : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap topic READ topic NOTIFY changed)
  Q_PROPERTY(QVariantList information READ information NOTIFY changed)
  Q_PROPERTY(QVariantMap history READ history NOTIFY historyChanged)
  Q_PROPERTY(QVariantMap bootReport READ bootReport NOTIFY changed)
  Q_PROPERTY(QObject* chargeTelemetry READ chargeTelemetry CONSTANT)
  Q_PROPERTY(bool chargeMiFc2Unavailable READ chargeMiFc2Unavailable NOTIFY changed)
  Q_PROPERTY(QStringList supportedChargeProtocols READ supportedChargeProtocols NOTIFY changed)
  Q_PROPERTY(QVariantMap chargingThresholds READ chargingThresholds NOTIFY changed)
  Q_PROPERTY(QVariantList chargingBands READ chargingBands CONSTANT)
  Q_PROPERTY(QVariantMap flashlight READ flashlight NOTIFY changed)
  Q_PROPERTY(QVariantMap notificationLed READ notificationLed NOTIFY changed)
  Q_PROPERTY(QVariantMap infrared READ infrared NOTIFY changed)
  Q_PROPERTY(QVariantMap displayResources READ displayResources NOTIFY changed)
  Q_PROPERTY(QVariantMap desktopDisplay READ desktopDisplay NOTIFY changed)
  Q_PROPERTY(QVariantMap changelog READ changelog NOTIFY changelogChanged)
  Q_PROPERTY(QVariantMap packageOperation READ packageOperation NOTIFY changed)
  Q_PROPERTY(QVariantList deviceInformation READ deviceInformation NOTIFY deviceInformationChanged)
  Q_PROPERTY(QVariantMap displayLink READ displayLink NOTIFY changed)
  Q_PROPERTY(QVariantList linuxPartitions READ linuxPartitions NOTIFY changed)
  Q_PROPERTY(QVariantMap updateChannel READ updateChannel NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(QString notice READ notice NOTIFY changed)
  Q_PROPERTY(QString updated READ updated NOTIFY changed)
  Q_PROPERTY(QObject* nfc READ nfc CONSTANT)
  Q_PROPERTY(QVariantMap usbPolicy READ usbPolicy NOTIFY changed)
  Q_PROPERTY(QObject* manager READ manager CONSTANT)
  Fixture fixture_;
  bool restricted_ = false;
  const Topic *topic_ = nullptr;
  QMap<QString, QJsonObject> data_;
  QMap<QString, QString> errors_, stamps_;
  QStringList queue_;
  QString current_, notice_, updated_;
  std::unique_ptr<Query> query_;
  QTimer poll_, simulated_;
  QVariantMap history_;
  unsigned historyGeneration_ = 0;
  NfcReader nfc_;
  ChargeTelemetry chargeTelemetry_;
  KernelManager manager_;
  QProcess *kernel_ = nullptr;
  QTimer kernelTimeout_;
  QByteArray kernelOutput_;
  bool flashlightPending_ = false;
  QString flashlightError_;
  void requestFlashlight(const QString &method, const QVariant &argument);
  bool ledPending_ = false, ledTesting_ = false;
  QString ledMethod_, ledError_;
  QVariantList ledArguments_;
  void sendLed();
  QProcess *irProcess_ = nullptr, *displayProcess_ = nullptr;
  QString irMessage_, displayMessage_;
  bool irFailed_ = false, displayFailed_ = false;
  bool overlayPending_ = false;
  void watchDisplayOverlay();
  QProcess *packageProcess_ = nullptr;
  QVariantMap packageOperation_;
  QVariantMap changelog_{{"message",QString()}};
  QNetworkAccessManager *changelogNetwork_ = nullptr;
  QNetworkReply *changelogReply_ = nullptr;
  bool changelogOnline_ = false;
  void loadChangelog();
  QVariantList deviceBoard_;
  QFutureWatcher<QVariantList> deviceInventory_;
  QString screenBrightnessId_;
  int brightness_ = -1, brightnessMaximum_ = 0;
  unsigned brightnessGeneration_ = 0;
  void readScreenBrightness();
  void readKernel(const QString &kind);
  void readCommand(const QString &program,const QStringList &arguments,int timeout);
  void next();
  void finish(const Result &result);
  void call(const QString &path, const QString &interface, const QString &method,
            const QVariantList &args, unsigned generation,
            std::function<void(const QDBusMessage &)> done);
  void discoverHistory(const QStringList &paths, int index, const QStringList &found,
                       unsigned generation, unsigned seconds);
public:
  explicit Settings(Fixture fixture, QObject *parent = nullptr, bool restricted = false);
  ~Settings() override;
  QVariantMap topic() const;
  QVariantList information() const;
  QVariantMap history() const { return history_; }
  QVariantMap bootReport() const;
  QObject *chargeTelemetry() { return &chargeTelemetry_; }
  QStringList supportedChargeProtocols() const;
  bool chargeMiFc2Unavailable() const;
  Q_INVOKABLE void setChargingVisible(bool visible);
  QVariantMap chargingThresholds() const;
  QVariantList chargingBands() const;
  QVariantMap flashlight() const;
  QVariantMap notificationLed() const;
  QVariantMap infrared() const;
  QVariantMap desktopDisplay() const;
  QVariantMap displayResources() const;
  Q_INVOKABLE void readDisplayResources();
  Q_INVOKABLE void setDisplayOverlay(bool enabled);
  QVariantMap changelog() const { return changelog_; }
  QVariantMap packageOperation() const { return packageOperation_; }
  QVariantList deviceInformation() const;
  QVariantMap displayLink() const;
  QVariantList linuxPartitions() const;
  QVariantMap updateChannel() const;
  QObject *nfc() { return &nfc_; }
  QObject *manager() { return &manager_; }
  QVariantMap usbPolicy() const;
  bool busy() const { return !current_.isEmpty() || !queue_.isEmpty(); }
  QString notice() const { return notice_; }
  QString updated() const { return updated_; }
  Q_INVOKABLE QVariantList topics(int area, const QString &search = {}) const;
  Q_INVOKABLE QString summary(int area) const;
  Q_INVOKABLE bool open(const QString &id);
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void close();
  Q_INVOKABLE void loadHistory(unsigned seconds = 86400);
  Q_INVOKABLE QVariantList choices(const QString &kind) const;
  Q_INVOKABLE QString localPath(const QUrl &url) const;
  Q_INVOKABLE QVariantList nativeLinks(int area) const;
  Q_INVOKABLE bool openNative(const QString &id);
  Q_INVOKABLE void setFlashlight(bool enabled);
  Q_INVOKABLE void setFlashlightBrightness(int percent);
  Q_INVOKABLE void previewLed(const QColor &color,int percent);
  Q_INVOKABLE void stopLedPreview();
  Q_INVOKABLE void sendInfrared(const QString &code);
  Q_INVOKABLE void setDisplayPreference(const QString &id,const QString &value);
  Q_INVOKABLE void setScreenBrightness(int percent);
  Q_INVOKABLE void runPackageOperation(const QString &operation,const QString &channel = {});
  Q_INVOKABLE QVariantList actionChoices(const QString &id, const QString &key) const;
private slots:
  void screenBrightnessChanged(const QString &id,int value,const QString &,const QString &);
  void screenBrightnessRangeChanged(const QString &id,int value,int maximum);
signals:
  void changed();
  void historyChanged();
  void changelogChanged();
  void deviceInformationChanged();
};
class HistoryPlot : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(QVariantMap report READ report WRITE setReport NOTIFY reportChanged)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY coloursChanged)
  Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY coloursChanged)
  QVariantMap report_;
  QColor foreground_, accent_;
public:
  explicit HistoryPlot(QQuickItem *parent = nullptr);
  QVariantMap report() const { return report_; }
  void setReport(const QVariantMap &report);
  QColor foreground() const { return foreground_; }
  QColor accent() const { return accent_; }
  void setForeground(const QColor &colour) { if(colour.isValid()&&colour!=foreground_){foreground_=colour;update();emit coloursChanged();} }
  void setAccent(const QColor &colour) { if(colour.isValid()&&colour!=accent_){accent_=colour;update();emit coloursChanged();} }
  void paint(QPainter *painter) override;
signals:
  void reportChanged();
  void coloursChanged();
};
} // namespace uq
