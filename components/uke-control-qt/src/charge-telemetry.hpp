// SPDX-License-Identifier: MIT
#pragma once
#include <QFutureWatcher>
#include <QJsonObject>
#include <QTimer>
#include <QVariantMap>

class QSocketNotifier;

namespace uq {
class ChargeTelemetry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
public:
  explicit ChargeTelemetry(QObject *parent = nullptr);
  ~ChargeTelemetry() override;
  QVariantMap state() const { return state_; }
  void setActive(bool active);

signals:
  void changed();

private:
  void sample();
  void handlePowerEvents();
  void releaseSocket();

  QFutureWatcher<QVariantMap> watcher_;
  QTimer timer_;
  QSocketNotifier *events_ = nullptr;
  int socket_ = -1;
  bool active_ = false;
  QVariantMap state_;
};
} // namespace uq
