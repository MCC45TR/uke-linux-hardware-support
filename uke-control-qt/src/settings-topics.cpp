// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
namespace uq {
const QList<Topic> &settingTopics() {
  static const QList<Topic> topics{
    {"battery",i18n("Battery use"),i18n("Level, temperature and recorded history"),"battery",1,{"battery.status"},{}},
    {"charging",i18n("Charge limit and policy"),i18n("Firmware thresholds, control support and charging preference"),"battery-charging",1,{"kernel.charging"},{"charge.limit","charge.mode"}},
    {"health",i18n("Battery health"),i18n("Reported capacity, cycles and cell topology"),"battery",1,{"battery.health","battery.cells"},{}},
    {"charger",i18n("Charger"),i18n("Reported source and protocol"),"drive-removable-media-usb",1,{"battery.protocol","charge.protocols"},{}},
    {"sleep",i18n("Sleep and wake"),i18n("Sleep observations and wake permissions"),"preferences-system-power-management",1,{"overview.sleep","wake.status"},{"wake.set","wake.apply"}},
    {"display",i18n("Display preferences"),i18n("Automatic brightness and rotation"),"video-display",2,{"desktop.plasma"},{"desktop.brightness","desktop.rotation"}},
    {"flashlight",i18n("Flashlight"),i18n("Off, low or high brightness"),"camera-flash",2,{"flashlight.status"},{"flashlight.level"}},
    {"led",i18n("Notification light"),i18n("Automatic policy or a requested colour"),"preferences-desktop-color",2,{"led.status"},{"led.auto","led.color","led.test"}},
    {"external",i18n("External display"),i18n("Connection and supported link profiles"),"video-display",2,{"display.bits","display.profiles"},{"display.profile"}},
    {"usb",i18n("USB and dock"),i18n("Accessories, link speeds, Ethernet, displays and automatic role handling"),"drive-removable-media-usb",3,{"kernel.dock"},{"usb.role","usb.power","display.profile"}},
    {"nfc",i18n("NFC tags and reading"),i18n("Built-in tag information and connected NDEF readers"),"network-wireless",3,{"kernel.tag"},{}},
    {"ethernet",i18n("Ethernet"),i18n("Choose a detected interface to inspect or configure"),"network-wired",3,{}, {"ethernet.status","ethernet.profile"}},
    {"sound",i18n("Sound and microphone"),i18n("Audio devices and microphone preference"),"audio-input-microphone",4,{"microphone.status","overview.audio"},{"microphone.select"}},
    {"storage",i18n("Storage"),i18n("Used and available filesystem space"),"drive-harddisk",5,{"overview.storage"},{}},
    {"memory",i18n("Memory and swap"),i18n("Available memory, swap usage and allocation policy"),"cpu",5,{"overview.workload","swap.status"},{"swap.set","swap.apply"}},
    {"updates",i18n("Software updates"),i18n("Selected channel, available updates and installation"),"system-software-update",6,{"channel.status"},{"update.preview","update.apply","channel.set"}},
    {"kernel",i18n("Kernel and boot slots"),i18n("Running kernel, approval and next boot selection"),"system-reboot",9,{"update.status"},{"boot.set-slot"}},
    {"about",i18n("Device information"),i18n("Hardware, cameras, sensors and firmware"),"dialog-information",6,{"hardware.status","overview.camera","sensors.list"},{}},
    {"support",i18n("Logs and support files"),i18n("Collect a focused local report"),"document-save",7,{}, {"diagnostics.collect","report.collect","support.export"}},
    {"snapshots",i18n("Saved observations"),i18n("Save or compare component snapshots"),"document-properties",7,{}, {"snapshot.save","snapshot.save-all","snapshot.compare"}},
    {"display-expert",i18n("Display engineering"),i18n("Saved VRR floor, lanes and compression"),"video-display",8,{"display.vrr-configured","display.compression"},{"display.vrr-set","display.lanes","display.compression-set"}},
    {"recovery",i18n("Recovery and maintenance"),i18n("Pending work and guarded storage operations"),"security-high",8,{"overview.recovery"},{"channel.sync-preview","channel.sync","resize.check","resize.prepare","singleboot.wizard","multiboot.wizard"}},
    {"serial",i18n("Serial connection"),i18n("Separate session and saved boot policies"),"drive-removable-media-usb",8,{}, {"cdc.session","cdc.persistent"}},
    {"cpu",i18n("CPU"),i18n("Cluster frequencies, processor load and frequency policy"),"cpu",9,{},{}},
    {"gpu",i18n("GPU"),i18n("Reported clock, temperature and frequency limits"),"video-display",9,{},{}},
    {"sensors",i18n("Sensors"),i18n("Live measurements, source status and graphs"),"utilities-system-monitor",9,{}, {"sensors.once","sensors.watch"}}
  };
  return topics;
}
QVariantMap topicMap(const Topic &t) {
  QVariantList changes;
  for(const auto &id:t.changes) {
    try { const auto &a=action(id.toStdString()); changes << QVariantMap{{"id",id},{"title",friendlyTitle(a)}}; }
    catch(const std::exception &) {} // An older installed SDK does not invent a command.
  }
  return {{"id",t.id},{"title",t.title},{"description",t.subtitle},{"icon",t.icon},{"area",t.area},{"changes",changes}};
}
} // namespace uq
