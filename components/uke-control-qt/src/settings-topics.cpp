// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
namespace uq {
const QList<Topic> &settingTopics() {
  static const QList<Topic> topics{
    {"manager-overview",i18n("Kernel Manager overview"),i18n("Resource pressure, capabilities and related controls"),"utilities-system-monitor",9,{},{}},
    {"charging",i18n("Battery and charging"),i18n("Charging thresholds, battery health, history and sleep"),"battery-charging",1,{"kernel.charging","battery.status","battery.cells","battery.health","battery.protocol","charge.protocols","overview.sleep","wake.status"},{"charge.limit","charge.mode","wake.set","wake.apply"}},
    {"display",i18n("Display preferences"),i18n("Automatic brightness, rotation and refresh rate"),"video-display",2,{"desktop.plasma"},{"desktop.brightness","desktop.rotation","display.vrr-set"}},
    {"flashlight",i18n("Flashlight"),i18n("Off, low or high brightness"),"camera-flash",2,{"flashlight.status"},{}},
    {"led",i18n("Notification light"),i18n("Automatic policy or a requested colour"),"preferences-desktop-color",2,{"led.status"},{"led.auto","led.color","led.test"}},
    {"usb",i18n("USB management"),i18n("USB devices, ports, docks and external displays"),"drive-removable-media-usb",3,{"kernel.dock","display.profiles","display.bits","display.compression"},{"usb.role","usb.power","display.lanes","display.compression-set","display.profile"}},
    {"infrared",i18n("Infrared transmitter"),i18n("Send a single NEC test signal"),"network-wireless",3,{},{}},
    {"nfc",i18n("NFC tags and reading"),i18n("Built-in tag information and connected NDEF readers"),"network-wireless",3,{"kernel.tag"},{}},
    {"ethernet",i18n("Ethernet"),i18n("Choose a detected interface to inspect or configure"),"network-wired",3,{}, {"ethernet.status","ethernet.profile"}},
    {"sound",i18n("Sound and microphone"),i18n("Audio devices and microphone preference"),"audio-input-microphone",4,{"microphone.status","overview.audio"},{"microphone.select"}},
    {"storage",i18n("Storage"),i18n("Filesystem space, I/O pressure, latency and queue policy"),"drive-harddisk",5,{"overview.storage"},{}},
    {"memory",i18n("Memory and swap"),i18n("Pressure, reclaim, swap activity and allocation policy"),"cpu",5,{"overview.workload","swap.status"},{"swap.set","swap.apply"}},
    {"updates",i18n("Software updates"),i18n("Selected channel, available updates and installation"),"system-software-update",6,{"channel.status","channel.repositories"},{"update.preview","update.apply","channel.set"}},
    {"changelog",i18n("What’s new"),i18n("Release highlights for Uke"),"document-preview",6,{},{}},
    {"kernel",i18n("Kernel and boot slots"),i18n("Running kernel, approval and next boot selection"),"system-reboot",9,{"update.status"},{"boot.set-slot"}},
    {"about",i18n("Device information"),i18n("Hardware, cameras, sensors and firmware"),"dialog-information",6,{"hardware.status"},{}},
    {"about-uke",i18n("About Uke Control"),i18n("Application version and license"),"dialog-information",6,{},{}},
    {"support",i18n("Logs and support files"),i18n("Collect a focused local report"),"document-save",7,{}, {"diagnostics.collect","report.collect","support.export"}},
    {"snapshots",i18n("Saved observations"),i18n("Save or compare component snapshots"),"document-properties",7,{}, {"snapshot.save","snapshot.save-all","snapshot.compare"}},
    {"recovery",i18n("Recovery and maintenance"),i18n("Pending work and guarded storage operations"),"security-high",8,{"overview.recovery"},{"channel.sync-preview","channel.sync","resize.check","resize.prepare","singleboot.wizard","multiboot.wizard"}},
    {"serial",i18n("Serial connection"),i18n("Separate session and saved boot policies"),"drive-removable-media-usb",8,{}, {"cdc.session","cdc.persistent"}},
    {"cpu",i18n("CPU"),i18n("Cluster frequencies, processor load and frequency policy"),"cpu",9,{},{}},
    {"gpu",i18n("GPU"),i18n("Reported clock, temperature and frequency limits"),"video-display",9,{},{}},
    {"thermal",i18n("Thermal and sleep"),i18n("Grouped temperatures, cooling, battery power and wake evidence"),"preferences-system-power-management",9,{},{}},
    {"profiles",i18n("Application profiles"),i18n("Reviewed CPU sharing and memory limits for active applications"),"configure",9,{},{}},
    {"diagnostics",i18n("Kernel diagnostics"),i18n("Kernel capabilities, pressure and observed policy changes"),"utilities-system-monitor",9,{},{}},
    {"sensors",i18n("Sensors"),i18n("Live measurements, source status and graphs"),"utilities-system-monitor",9,{}, {"sensors.once","sensors.watch"}}
  };
  return topics;
}
QVariantMap topicMap(const Topic &t) {
  QVariantList changes;
  for(const auto &id:t.changes) {
    try {
      const auto &a=action(id.toStdString());
      changes << QVariantMap{{"id",id},{"title",friendlyTitle(a)},{"read",a.effect==Effect::read}};
    }
    catch(const std::exception &) {} // An older installed SDK does not invent a command.
  }
  return {{"id",t.id},{"title",t.title},{"description",t.subtitle},{"icon",t.icon},{"area",t.area},{"changes",changes}};
}
} // namespace uq
