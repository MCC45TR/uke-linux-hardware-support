// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
namespace uq {
class WaydroidManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
  Q_PROPERTY(bool busy READ busy NOTIFY changed)
  Q_PROPERTY(bool authorizing READ authorizing NOTIFY changed)
  Q_PROPERTY(bool blocked READ blocked CONSTANT)
  Q_PROPERTY(QString message READ message NOTIFY changed)
  Q_PROPERTY(QString googleId READ googleId NOTIFY changed)
  Q_PROPERTY(QString phaseText READ phaseText NOTIFY changed)
  bool simulation_, readOnly_;
  QJsonObject fixture_, state_;
  QProcess status_, request_;
  QTimer poll_, deadline_;
  QByteArray statusOutput_, requestOutput_;
  QString message_, googleId_, requestKind_;
  bool watching_ = false;
  void updateState(const QByteArray &data);
  void request(const QString &kind, const QStringList &arguments);
  void showError(const QString &code);
public:
  explicit WaydroidManager(const Fixture &fixture, bool readOnly, QObject *parent = nullptr);
  ~WaydroidManager() override;
  QVariantMap state() const { return state_.toVariantMap(); }
  bool busy() const { return authorizing() || state_.value("busy").toBool(); }
  bool authorizing() const { return request_.state() != QProcess::NotRunning; }
  bool blocked() const { return simulation_ || readOnly_; }
  QString message() const { return message_; }
  QString googleId() const { return googleId_; }
  QString phaseText() const;
  Q_INVOKABLE void open();
  Q_INVOKABLE void close();
  Q_INVOKABLE void refresh();
  Q_INVOKABLE bool install(const QString &variant);
  Q_INVOKABLE bool launch();
  Q_INVOKABLE bool readGoogleId();
  Q_INVOKABLE bool openGoogleRegistration();
signals:
  void changed();
};
}
