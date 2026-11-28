// SPDX-License-Identifier: MIT
#include "charge-telemetry.hpp"
#include <KLocalizedString>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QSocketNotifier>
#include <QtConcurrentRun>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>
#include <optional>

namespace uq {
namespace {
QByteArray readField(const QString &directory, const QString &field) {
  QFile file(directory + '/' + field);
  return file.open(QIODevice::ReadOnly) ? file.read(256).trimmed() : QByteArray();
}
std::optional<qint64> integer(const QByteArray &text, qint64 minimum, qint64 maximum) {
  bool ok = false;
  const auto value = text.toLongLong(&ok);
  if (ok && value >= minimum && value <= maximum) return value;
  return {};
}
QVariantMap readChargeState(bool previouslyConnected) {
  const QString supplies = "/sys/class/power_supply/";
  const QString usb = supplies + "qcom-battmgr-usb";
  const QString wireless = supplies + "qcom-battmgr-wls";
  const QString battery = supplies + "qcom-battmgr-bat";
  const auto usbOnline = integer(readField(usb,"online"),0,1);
  const auto wirelessOnline = integer(readField(wireless,"online"),0,1);
  const bool wired = usbOnline == 1;
  const bool inductive = wirelessOnline == 1;
  const bool connected = wired || inductive;
  const QByteArray status = readField(battery,"status");
  const QDir processors("/sys/class/remoteproc");
  QByteArray adsp;
  for (const auto &entry : processors.entryList({"remoteproc*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto path = processors.filePath(entry);
    if (readField(path,"name").toLower() == "adsp") {
      adsp = readField(path,"state");
      break;
    }
  }

  QString message;
  if (adsp == "crashed") message = i18n("The ADSP crashed. Charging readings are unavailable.");
  else if (adsp == "offline") message = i18n("The ADSP is offline. Charging readings are unavailable.");
  else if (status == "Full") message = i18n("The battery is full.");
  else if (status == "Charging") message = i18n("The battery is charging.");
  else if (connected && (status == "Not charging" || status == "Discharging"))
    message = i18n("External power is connected. The battery is not charging.");
  else if (connected) message = i18n("External power is connected. The battery driver has not reported a charging state.");
  else if (usbOnline == 0 && wirelessOnline == 0) message = i18n("No power adapter is connected. The device is not charging.");
  else message = i18n("The charging driver could not report whether a power adapter is connected.");

  QVariantList rows;
  const auto add = [&](const QString &title, const QString &value) {
    rows << QVariantMap{{"title",title},{"value",value}};
  };
  const auto number = [](double value, const QString &unit) {
    return QLocale().toString(value,'f',2) + ' ' + unit;
  };
  const auto measurement = [&](const QString &title, const std::optional<qint64> &value, const QString &unit) {
    if (value) add(title, number(double(*value) / 1e6,unit));
  };
  if (connected && adsp != "offline" && adsp != "crashed") {
    const QString source = wired ? usb : wireless;
    const auto voltage = integer(readField(source,"voltage_now"),1,60000000);
    const auto current = integer(readField(source,"current_now"),0,20000000);
    if (voltage && current) add(i18n("Adapter input power"),number(double(*voltage) * double(*current) / 1e12,"W"));
    measurement(i18n("Input voltage"),voltage,"V");
    measurement(i18n("Input current"),current,"A");


    const auto types = readField(source,"usb_type");
    const auto first = types.indexOf('[');
    const auto last = types.indexOf(']',first + 1);
    const auto type = first >= 0 && last > first ? types.mid(first + 1,last - first - 1) : types;
    const QMap<QByteArray,QString> protocols{
      {"SDP",i18n("Standard USB port")},{"CDP",i18n("USB charging port")},{"DCP",i18n("Dedicated USB charger")},
      {"C",i18n("USB Type-C")},{"PD",i18n("USB Power Delivery")},{"PD_DRP",i18n("USB Power Delivery (dual-role)")},
      {"PD_PPS",i18n("USB Power Delivery PPS")},{"ACA",i18n("USB accessory charger")}
    };
    add(i18n("Charging technology"),inductive && !wired ? i18n("Wireless charging") :
        protocols.value(type,i18n("The adapter has not reported a charging technology.")));

    measurement(i18n("Input current limit"),integer(readField(source,"input_current_limit"),1,20000000),"A");
    measurement(i18n("Adapter current limit"),integer(readField(source,"current_max"),1,20000000),"A");
    measurement(i18n("Adapter voltage limit"),integer(readField(source,"voltage_max"),1,60000000),"V");
    measurement(i18n("Battery power"),integer(readField(battery,"power_now"),-150000000,150000000),"W");
    measurement(i18n("Battery voltage"),integer(readField(battery,"voltage_now"),1,25000000),"V");
    measurement(i18n("Battery current"),integer(readField(battery,"current_now"),-20000000,20000000),"A");

    const auto chargerType=readField(battery,"charge_type");
    if (chargerType=="Fast") add(i18n("Charge stage"),i18n("Fast charging"));
    else if (chargerType=="Trickle") add(i18n("Charge stage"),i18n("Trickle charging"));
    else if (chargerType=="Standard") add(i18n("Charge stage"),i18n("Standard charging"));

    if (!voltage || !current)
      message += ' ' + i18n("The adapter did not provide a complete voltage and current reading.");
    if (integer(readField(source,"online"),0,1) == 0)
      return {{"connected",false},{"message",i18n("Power adapter disconnected.")}};
  }
  return {{"connected",connected},{"retry",!usbOnline && !wirelessOnline && previouslyConnected},
          {"message",message},{"rows",rows}};
}
}

ChargeTelemetry::ChargeTelemetry(QObject *parent) : QObject(parent) {
  timer_.setInterval(1000);
  connect(&timer_,&QTimer::timeout,this,&ChargeTelemetry::sample);
  connect(&watcher_,&QFutureWatcher<QVariantMap>::finished,this,[this] {
    if (!active_) return;
    const auto next = watcher_.result();
    if (next != state_) { state_ = next; emit changed(); }
    if (state_.value("connected").toBool() || state_.value("retry").toBool()) {
      if (!timer_.isActive()) timer_.start();
    } else timer_.stop();
  });
}

ChargeTelemetry::~ChargeTelemetry() { releaseSocket(); }

void ChargeTelemetry::setActive(bool active) {
  if (active_ == active) return;
  active_ = active;
  if (!active_) {
    timer_.stop();
    releaseSocket();
    return;
  }
  socket_ = socket(AF_NETLINK,SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,NETLINK_KOBJECT_UEVENT);
  if (socket_ >= 0) {
    sockaddr_nl address{};
    address.nl_family = AF_NETLINK;
    address.nl_groups = 1;
    if (bind(socket_,reinterpret_cast<sockaddr *>(&address),sizeof(address)) == 0) {
      events_ = new QSocketNotifier(socket_,QSocketNotifier::Read,this);
      connect(events_,&QSocketNotifier::activated,this,&ChargeTelemetry::handlePowerEvents);
    } else releaseSocket();
  }
  sample();
}

void ChargeTelemetry::releaseSocket() {
  delete events_;
  events_ = nullptr;
  if (socket_ >= 0) close(socket_);
  socket_ = -1;
}

void ChargeTelemetry::sample() {
  if (active_ && !watcher_.isRunning())
    watcher_.setFuture(QtConcurrent::run(readChargeState,state_.value("connected").toBool()));
}

void ChargeTelemetry::handlePowerEvents() {
  char buffer[4096];
  bool changed = false;
  for (int i=0; i<32; ++i) {
    sockaddr_nl sender{};
    socklen_t length = sizeof(sender);
    const auto bytes = recvfrom(socket_,buffer,sizeof(buffer),MSG_DONTWAIT,
        reinterpret_cast<sockaddr *>(&sender),&length);
    if (bytes <= 0) break;
    if (sender.nl_pid != 0) continue;
    const QByteArrayView message(buffer,bytes);
    if (message.contains(QByteArrayView("\0SUBSYSTEM=power_supply\0",24)) &&
        (!timer_.isActive() || message.contains(QByteArrayView("\0POWER_SUPPLY_ONLINE=0\0",23))))
      changed = true;
  }
  if (changed) sample();
}
} // namespace uq
