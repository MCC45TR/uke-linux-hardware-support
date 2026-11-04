// SPDX-License-Identifier: MIT
#include "model-data.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace uq {
QString fieldTitle(const QString &key) {
  static const QMap<QString, QString> labels{
      {"available", "Information available"}, {"physical_acceptance", "Physical validation"},
      {"current_ma", "Current (mA)"}, {"voltage_mv", "Voltage (mV)"},
      {"temperature_c", "Temperature (°C)"}, {"capacity_percent", "Charge (%)"},
      {"cycles", "Charge cycles"}, {"schema", "Report format"},
      {"usb", "USB"}, {"nfc", "NFC"}, {"cpu", "CPU"}, {"gpu", "GPU"}};
  if (labels.contains(key)) return labels.value(key);
  auto label = key;
  label.replace('_', ' ');
  label.replace('-', ' ');
  if (!label.isEmpty()) label[0] = label[0].toUpper();
  return safe(label.toStdString());
}
QString actionIcon(const Action &a) {
  const auto id = QString::fromStdString(a.id);
  // The same semantic names serve daily pages and the complete SDK catalog.
  for (const auto &[part, icon] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"microphone", "audio-input-microphone"},
           {"battery", "battery"}, {"charge", "battery-charging"},
           {"brightness", "video-display"}, {"display", "video-display"},
           {"desktop", "preferences-desktop"}, {"flashlight", "camera-flash"},
           {"led", "preferences-desktop-color"}, {"usb", "drive-removable-media-usb"},
           {"ethernet", "network-wired"}, {"nfc", "network-wireless"},
           {"audio", "audio-card"}, {"camera", "camera-photo"},
           {"sensors", "utilities-system-monitor"}, {"wake", "preferences-system-time"},
           {"sleep", "preferences-system-power-management"},
           {"channel", "system-switch-user"}, {"update", "system-software-update"},
           {"hardware", "cpu"}, {"firmware", "cpu"},
           {"workload", "utilities-system-monitor"}, {"touch", "input-touchpad"},
           {"storage", "drive-harddisk"}, {"roots", "drive-harddisk"},
           {"swap", "drive-harddisk"}, {"resize", "drive-harddisk"},
           {"snapshot", "document-save"}, {"support", "document-export"},
           {"diagnostics", "document-properties"}, {"report", "document-send"},
           {"recovery", "security-high"}, {"boot", "system-reboot"}})
    if (id.contains(QString::fromLatin1(part)))
      return QString::fromLatin1(icon);
  if (a.effect == Effect::maintenance)
    return "security-high";
  if (a.effect == Effect::publish)
    return "document-send";
  if (a.effect == Effect::local_file)
    return "document-save";
  if (a.effect == Effect::read)
    return "dialog-information";
  return "configure";
}
static QString q(const std::string &s) {
  return QString::fromUtf8(s.data(), int(s.size()));
}
QString safe(const std::string &text) {
  QString out;
  for (const QChar ch : q(text)) {
    const auto u = ch.unicode();
    if ((u < 32 && u != '\n' && u != '\t') || (u >= 0x7f && u <= 0x9f) ||
        (u >= 0x202a && u <= 0x202e) || (u >= 0x2066 && u <= 0x2069))
      out += QString("\\u%1").arg(unsigned(u), 4, 16, QChar('0'));
    else
      out += ch;
  }
  return out;
}
Fixture Fixture::load(const QString &path) {
  const auto bytes = QFile::encodeName(path);
  const int fd =
      ::open(bytes.constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
  if (fd < 0)
    throw std::runtime_error(
        "Cannot open fixture as a non-symlink regular file");
  struct stat st{};
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 2 ||
      st.st_size > 1024 * 1024) {
    ::close(fd);
    throw std::runtime_error("Fixture must be a regular file of at most 1 MiB");
  }
  QByteArray data;
  char buffer[4096];
  ssize_t count;
  while ((count = ::read(fd, buffer, sizeof(buffer))) > 0) {
    data.append(buffer, int(count));
    if (data.size() > 1024 * 1024)
      break;
  }
  ::close(fd);
  if (count < 0 || data.size() > 1024 * 1024)
    throw std::runtime_error("Cannot read bounded fixture");
  QJsonParseError error;
  auto doc = QJsonDocument::fromJson(data, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject() ||
      doc.object().value("schema").toString() !=
          "org.uke.control.tui-fixture.v1" ||
      !doc.object().value("results").isObject())
    throw std::runtime_error("Invalid org.uke.control.tui-fixture.v1 fixture");
  const auto root = doc.object();
  for (auto it = root.begin(); it != root.end(); ++it)
    if (it.key() != "schema" && it.key() != "results")
      throw std::runtime_error("Unknown fixture field");
  Fixture fixture;
  fixture.enabled = true;
  fixture.results = doc.object().value("results").toObject();
  for (auto it = fixture.results.begin(); it != fixture.results.end(); ++it) {
    action(it.key().toStdString());
    if (!it.value().isObject())
      throw std::runtime_error("Fixture result must be an object");
    const auto entry = it.value().toObject();
    for (auto field = entry.begin(); field != entry.end(); ++field)
      if (field.key() != "data" && field.key() != "stdout" &&
          field.key() != "stderr" && field.key() != "status" &&
          field.key() != "delay_ms")
        throw std::runtime_error("Unknown fixture result field");
    if (entry.contains("data") && entry.contains("stdout"))
      throw std::runtime_error("Fixture data/stdout are mutually exclusive");
    for (const auto &key : {"stdout", "stderr"})
      if (entry.contains(key) && !entry.value(key).isString())
        throw std::runtime_error("Fixture stdout/stderr must be strings");
    for (const auto &key : {"delay_ms", "status"})
      if (entry.contains(key) &&
          (!entry.value(key).isDouble() ||
           entry.value(key).toDouble() != entry.value(key).toInt(-1)))
        throw std::runtime_error(
            "Fixture delay/status must be bounded integers");
    if (entry.value("delay_ms").toInt() < 0 ||
        entry.value("delay_ms").toInt() > 3000 ||
        entry.value("status").toInt() < 0 ||
        entry.value("status").toInt() > 255)
      throw std::runtime_error("Fixture delay/status outside allowed range");
  }
  return fixture;
}
Result Fixture::result(const std::string &id) const {
  Result r;
  r.action_id = id;
  if (!results.contains(q(id))) {
    r.exit_status = 69;
    r.error =
        "Unavailable: no simulated result for this action. No command was run.";
    return r;
  }
  const auto entry = results.value(q(id)).toObject();
  r.exit_status = entry.value("status").toInt();
  if (entry.contains("stdout"))
    r.output = entry.value("stdout").toString().toStdString();
  else if (entry.contains("data")) {
    const auto data = entry.value("data");
    if (data.isObject())
      r.output = QJsonDocument(data.toObject()).toJson().toStdString();
    else if (data.isArray())
      r.output = QJsonDocument(data.toArray()).toJson().toStdString();
    else {
      QJsonArray a;
      a.append(data);
      auto s = QJsonDocument(a).toJson(QJsonDocument::Compact);
      r.output = s.mid(1, s.size() - 2).toStdString();
    }
  }
  r.error = entry.value("stderr").toString().toStdString();
  return r;
}
int Fixture::delay(const std::string &id) const {
  return results.value(q(id)).toObject().value("delay_ms").toInt();
}
QString friendlyTitle(const Action &a) {
  if (QLocale().language()==QLocale::Turkish) return q(a.title.tr);
  static const std::map<std::string, QString> names = {
      {"status", "Device status"},
      {"overview.battery", "Battery overview"},
      {"overview.workload", "System activity"},
      {"hardware.status", "Hardware information"},
      {"overview.desktop", "Desktop information"},
      {"overview.storage", "Storage overview"},
      {"battery.status", "Battery readings"},
      {"battery.health", "Battery health"},
      {"charge.mode", "Charging preference"},
      {"charge.limit", "Charge limit"},
      {"charge.status", "Charging status"},
      {"charge.admission", "Charging availability"},
      {"charge.temporary-preview", "Temporary full-charge preview"},
      {"desktop.plasma", "Rotation & brightness"},
      {"desktop.rotation", "Automatic rotation"},
      {"desktop.brightness", "Automatic brightness"},
      {"flashlight.state", "Flashlight"},
      {"flashlight.level", "Flashlight brightness"},
      {"led.color", "Notification light color"},
      {"led.auto", "Automatic notification light"},
      {"led.status", "Notification light"},
      {"display.bits", "Display connection"},
      {"display.profile", "Display connection preference"},
      {"usb.status", "USB connections"},
      {"usb.role", "USB data role"},
      {"usb.power", "USB accessory power"},
      {"ethernet.status", "Ethernet connection"},
      {"ethernet.profile", "Ethernet preference"},
      {"microphone.select", "Microphone preference"},
      {"microphone.status", "Microphone"},
      {"channel.set", "Software channel"},
      {"channel.status", "Channel & pending updates"},
      {"update.apply", "Install updates"},
      {"channel.preview", "Preview software channel"},
      {"update.preview", "Check for updates"},
      {"sensors.once", "Read sensors once"},
      {"sensors.watch", "Watch sensors briefly"}};
  const auto found = names.find(a.id);
  return found == names.end() ? q(a.title.en) : found->second;
}
QString summary(const Action &a) {
  if (QLocale().language()==QLocale::Turkish) return q(a.description.tr);
  static const QMap<QString, QString> taskHelp{
      {"update.status", "Read which kernel is running, both slot kernels, the next boot selection and recorded approval."},
      {"boot.set-slot", "Choose A, B or the other running slot. Review the exact kernel in the terminal before confirming; reboot is separate."},
      {"boot.slot-preview", "Check the target slot and its kernel without changing boot selection."},
      {"status", "Check the reported charging, memory and USB state."},
      {"overview.battery", "See the battery snapshot and which measurements are available."},
      {"overview.workload", "Check current CPU, memory and workload information."},
      {"hardware.status", "Inspect the detected hardware and available device endpoints."},
      {"overview.desktop", "See the current desktop session and display information."},
      {"overview.storage", "Inspect storage and filesystem information."},
      {"battery.status", "Read the battery's reported charge, voltage, current and temperature."},
      {"battery.health", "Inspect reported battery health and the source of those values."},
      {"charge.status", "Check the charging provider and its reported policy."},
      {"charge.admission", "Check whether the provider can accept a charging request."},
      {"overview.sleep", "Inspect the reported sleep state and available information."},
      {"wake.status", "See which wake sources are available and their saved permissions."},
      {"desktop.plasma", "Inspect the current Plasma desktop settings."},
      {"flashlight.status", "Check whether the flashlight is available and its reported state."},
      {"led.status", "Read the notification light's reported state."},
      {"display.bits", "Inspect display link capabilities and reported connection details."},
      {"usb.status", "Check connected USB devices and the current USB role."},
      {"ethernet.status", "Inspect the Ethernet interface, connection and reported link."},
      {"overview.nfc", "Check NFC availability and the reported state."},
      {"overview.audio", "Inspect available audio devices and routing information."},
      {"microphone.status", "Check the microphone array and saved channel preference without recording."},
      {"overview.camera", "Inspect camera availability and reported support limits."},
      {"sensors.list", "List sensors and the readings each provider can report."},
      {"sensors.once", "Read one sample from the selected sensor."},
      {"sensors.watch", "Observe the selected sensor for a limited interval."},
      {"channel.status", "Check the selected software channel and any unfinished update."},
      {"channel.set", "Choose Stable, Test or Unstable, preview the plan, then review the change."},
      {"channel.preview", "Review the selected channel's software plan in the terminal without installing it."},
      {"update.preview", "Check available updates in the terminal without installing them."},
      {"update.apply", "Review and install updates for the selected channel. Wait for completion before rebooting."}};
  const auto help = taskHelp.constFind(q(a.id));
  if (help != taskHelp.cend())
    return *help;
  if (a.effect == Effect::maintenance)
    return q(a.description.en);
  if (a.effect == Effect::publish)
    return "This publishes a generated summary externally. Review the summary "
           "and the exact destination before continuing.";
  if (a.id == "charge.mode")
    return "Choose how charging should be limited. Review changes before "
           "applying; availability is checked before a request is accepted.";
  if (a.id == "charge.limit")
    return "Choose a requested charge limit. Check charging availability "
           "before applying.";
  if (a.id == "charge.admission")
    return "See whether charging controls are available and what is blocking "
           "them.";
  if (a.id.starts_with("charge.") && a.effect == Effect::read)
    return "Read charging information. Unavailable or unknown values stay "
           "visible.";
  if (a.id == "flashlight.level")
    return "Choose a brightness request. The light has two lit steps: 1–99% "
           "maps to 50%, and 100% maps to full brightness.";
  if (a.id == "flashlight.state")
    return "Turn the flashlight on or off. It has no automatic timeout and "
           "turns off before sleep.";
  if (a.id == "usb.role")
    return "Choose the USB data role. Changing it can disconnect accessories "
           "or a remote session; inspect the connection first.";
  if (a.id == "ethernet.profile")
    return "Choose a supported link preference. Applying changes can interrupt "
           "your network connection.";
  if (a.id == "display.profile" || a.id == "display.lanes" ||
      a.id == "display.compression-set")
    return "Review the current display connection first. Changing this "
           "preference can disconnect a display or accessory.";
  if (a.effect == Effect::packages)
    return "Review the signed software plan before continuing. Inspect "
           "unfinished work before retrying, and wait for updates to finish "
           "before rebooting.";
  if (a.id.starts_with("test."))
    return "Keep the device attended during this diagnostic. Review Details "
           "before continuing.";
  if (a.id.starts_with("desktop.") && a.effect != Effect::read)
    return "Choose a preference for your current desktop session. Changes use "
           "the existing desktop controls.";
  if (a.effect == Effect::local_file)
    return "Review the requested local file changes. Existing backend checks "
           "and private-file protections still apply.";
  if (a.effect == Effect::read)
    return "Read the current information. Missing data will be shown as "
           "unavailable or unknown.";
  return "Choose your preference, inspect current state, and review the "
         "request before applying changes.";
}
} // namespace uq
