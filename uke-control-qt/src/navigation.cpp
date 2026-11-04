// SPDX-License-Identifier: MIT
#include "navigation.hpp"

namespace uq {
namespace {
struct Group {
  QString title, description;
  QStringList actions;
};
struct Area {
  QString id, title, icon, caption, description, detail;
  QList<Group> groups;
};
const QList<Area> &areas() {
  static const QList<Area> list{
      {"overview", "Overview", "dialog-information", "Start here",
       "Device information and shortcuts to everyday tasks.",
       "Choose a task below or browse a category. Readings appear when you request them.",
       {{"Quick checks", "Start with a focused status check.",
         {"status", "battery.status", "channel.status"}}}},
      {"power", "Battery & power", "battery", "Charging, health, sleep & wake",
       "Understand battery readings and manage charging or wake preferences.",
       "Read the battery and charging state first. Requested limits depend on the charging provider's capabilities.",
       {{"Battery information", "Reported measurements and their source.",
         {"battery.status", "battery.health", "battery.cells", "battery.protocol"}},
        {"Charging", "Check provider support before changing a policy.",
         {"charge.status", "charge.admission", "charge.mode", "charge.limit"}},
        {"Sleep & wake", "Review wake sources before changing their permissions.",
         {"overview.sleep", "wake.status", "wake.set"}}}},
      {"display", "Display & lights", "video-display", "Desktop, flashlight & notification light",
       "Adjust the desktop and lights, or inspect an external display connection.",
       "Desktop preferences affect this session. Display-link settings depend on the connected display and supported hardware path.",
       {{"Desktop preferences", "Brightness and orientation for the current desktop.",
         {"desktop.plasma", "desktop.brightness", "desktop.rotation"}},
        {"Flashlight", "Check availability, then choose a supported light level.",
         {"flashlight.status", "flashlight.state", "flashlight.level"}},
        {"Notification light", "Inspect the light or restore its automatic policy.",
         {"led.status", "led.color", "led.auto"}},
        {"External display", "Inspect the connection before reviewing a link preference.",
         {"display.bits", "display.profiles", "display.profile"}}}},
      {"connections", "Connections", "network-wired", "USB, Ethernet & NFC",
       "Inspect accessories and review connection preferences.",
       "Changing a USB role or network preference can disconnect an accessory. Start with the connection status.",
       {{"USB accessories", "Connected devices, data role and per-device power.",
         {"usb.status", "usb.role", "usb.power"}},
        {"Ethernet", "Reported link information and supported preferences.",
         {"ethernet.status", "ethernet.profile"}},
        {"NFC", "Controller availability and radio permission; no scan starts here.",
         {"nfc.status", "nfc.state"}}}},
      {"media", "Sound & cameras", "audio-card", "Audio routing, microphones & camera information",
       "Inspect audio devices, choose a microphone preference and check camera support.",
       "Microphone preferences apply to new recordings. Camera information describes reported support, rather than a photo or focus test.",
       {{"Sound & microphones", "Inspect routing before selecting a recording preference.",
         {"overview.audio", "microphone.status", "microphone.select"}},
        {"Cameras", "Check reported camera availability and support limits.",
         {"overview.camera"}}}},
      {"sensors", "Sensors & input", "utilities-system-monitor", "Sensor readings & attended input checks",
       "Explore available sensors and request focused observations.",
       "Choose a sensor for one sample or a bounded series. Input diagnostics may require an exact device and an attended terminal.",
       {{"Sensor readings", "List the available sensors before selecting one.",
         {"sensors.list", "sensors.once", "sensors.watch"}},
        {"Input diagnostics", "Read input information without changing device preferences.",
         {"test.touch", "test.sensors"}}}},
      {"updates", "Software updates", "system-software-update", "Kernel slots, channels & installation",
       "Inspect kernel approval, manage the next boot and review software updates.",
       "Read the running and stored kernels before choosing a boot slot. Channel and package changes have their own previews and review steps.",
       {{"Kernel & boot slots", "Running slot, stored kernels, pending selection and recorded approval.",
         {"update.status", "boot.set-slot", "boot.slot-preview"}},
        {"Current software state", "Check the selected channel before planning a change.",
         {"channel.status"}},
        {"Choose a channel", "Select Stable, Test or Unstable and preview its plan.",
         {"channel.set", "channel.preview"}},
        {"Install updates", "Check available updates before reviewing installation.",
         {"update.preview", "update.apply"}},
        {"Unfinished update", "Inspect pending work before attempting synchronization.",
         {"channel.sync-preview", "channel.sync"}}}},
      {"diagnostics", "Diagnostics & files", "document-properties", "System information, snapshots & support files",
       "Collect focused evidence and compare saved information.",
       "Read system information or save a private snapshot. Review file locations and contents before sharing a diagnostic report.",
       {{"System information", "Hardware, firmware and storage observations.",
         {"hardware.status", "firmware.status", "overview.workload", "overview.storage"}},
        {"Snapshots", "Save observations to a new file or compare two snapshots.",
         {"snapshot.save", "snapshot.save-all", "snapshot.compare"}},
        {"Support files", "Create a focused private log or an allowlisted support summary.",
         {"diagnostics.collect", "support.export", "report.collect"}}}},
      {"recovery", "Recovery & maintenance", "security-high", "Pending operations, storage & guarded recovery",
       "Inspect recovery state before considering maintenance.",
       "These tools can affect storage, connectivity or boot. Read the status and requirements first; maintenance keeps its existing approvals and terminal workflow.",
       {{"Recovery information", "Read current recovery and pending-operation information.",
         {"overview.recovery", "channel.private-status", "usb.transition-status"}},
        {"Storage inspection", "Inspect existing partitions and plans without preparing maintenance.",
         {"inventory", "roots.list", "swap.status", "resize.check"}},
        {"Guarded maintenance", "Review exact effects and recovery requirements before continuing.",
         {"usb.transition-recover", "resize.prepare", "singleboot.wizard", "multiboot.wizard"}}}}};
  return list;
}
} // namespace
QString actionRole(const Action &a) {
  if (a.tests_package && a.effect != Effect::read) return "Run attended diagnostic";
  switch (a.effect) {
  case Effect::read: return a.presentation == Presentation::capture ? "Read information" : "Read in terminal";
  case Effect::device: return "Change preference";
  case Effect::packages: return "Software operation";
  case Effect::local_file: return "Save or change files";
  case Effect::maintenance: return "Maintenance";
  case Effect::publish: return "Publish externally";
  }
  return {};
}
QString accessLabel(const Action &a) {
  switch (a.access) {
  case Access::root: return "Administrator access";
  case Access::active_session: return "Desktop session";
  case Access::user: return "Standard user access";
  }
  return {};
}
int actionArea(const Action &a) {
  const auto &id = a.id;
  if (id == "boot.set-slot" || id == "boot.slot-preview") return 6;
  if (a.effect == Effect::maintenance || a.category == "recovery" ||
      (a.category == "storage" && !id.starts_with("overview."))) return 8;
  if (a.category == "charge" || a.category == "battery" || a.category == "wake") return 1;
  if (a.category == "desktop" || a.category == "display" || a.category == "lights" ||
      id == "test.display" || id == "test.cadence") return 2;
  if (a.category == "usb" || a.category == "network" || id.starts_with("nfc.") || id == "overview.nfc") return 3;
  if (a.category == "media") return 4;
  if (a.category == "sensors" || id == "test.sample" || id == "test.discover") return 5;
  if (a.category == "updates") return 6;
  if (a.category == "diagnostics" || a.category == "developer" ||
      id == "hardware.status" || id == "firmware.status" ||
      id == "overview.hardware" || id == "overview.storage" || id == "overview.workload") return 7;
  return 0;
}
QVariantMap actionItem(const Action &a) {
  return {{"id", QString::fromStdString(a.id)}, {"title", friendlyTitle(a)},
          {"description", summary(a)}, {"icon", actionIcon(a)},
          {"role", actionRole(a)}, {"access", accessLabel(a)},
          {"area", actionArea(a)}, {"read", a.effect == Effect::read}};
}
QVariantList areaModel() {
  QVariantList result;
  for (int i = 0; i < areas().size(); ++i) {
    const auto &a = areas()[i];
    result << QVariantMap{{"id", a.id}, {"title", a.title}, {"icon", a.icon},
                          {"caption", a.caption}, {"description", a.description},
                          {"detail", a.detail}, {"index", i}};
  }
  return result;
}
QVariantList taskGroups(int area) {
  QVariantList result;
  if (area < 0 || area >= areas().size()) return result;
  for (const auto &group : areas()[area].groups) {
    QVariantList entries;
    for (const auto &id : group.actions) entries << actionItem(action(id.toStdString()));
    result << QVariantMap{{"title", group.title}, {"description", group.description},
                          {"entries", entries}};
  }
  return result;
}
bool matchesAction(const Action &a, const QString &query) {
  const auto haystack = QString::fromStdString(a.id + " " + a.title.en + " " +
      a.title.tr + " " + a.description.en + " " + a.description.tr) +
      " " + friendlyTitle(a) + " " + actionRole(a);
  for (const auto &word : query.simplified().split(' ', Qt::SkipEmptyParts))
    if (!haystack.contains(word, Qt::CaseInsensitive)) return false;
  return true;
}
} // namespace uq
