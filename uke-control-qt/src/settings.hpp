// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QQuickPaintedItem>
#include <QColor>
#include <functional>
namespace uq {
struct Topic {
  QString id, title, subtitle, icon;
  int area;
  QStringList reads, changes;
};
const QList<Topic> &settingTopics();
QVariantMap topicMap(const Topic &topic);
QVariantList informationRows(const QString &id, const QJsonObject &data);
QVariantMap normaliseHistory(const QVariantList &records, qint64 start, qint64 end);
class Settings : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap topic READ topic NOTIFY changed)
  Q_PROPERTY(QVariantList information READ information NOTIFY changed)
  Q_PROPERTY(QVariantMap history READ history NOTIFY historyChanged)
  Q_PROPERTY(QVariantMap bootReport READ bootReport NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(QString notice READ notice NOTIFY changed)
  Q_PROPERTY(QString updated READ updated NOTIFY changed)
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
  bool busy() const { return !current_.isEmpty(); }
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
signals:
  void changed();
  void historyChanged();
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
