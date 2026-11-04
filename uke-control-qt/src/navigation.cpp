// SPDX-License-Identifier: MIT
#include "navigation.hpp"
#include <KLocalizedString>
namespace uq {
QVariantList areaModel() {
  struct Area {const char *id;QString title,caption;const char *icon;};
  const QList<Area> list{
    {"overview",i18n("Settings"),i18n("Device settings and information"),"preferences-system"},
    {"power",i18n("Battery and charging"),i18n("Battery use, charging and health"),"battery"},
    {"display",i18n("Display and lights"),i18n("Brightness, rotation and lights"),"video-display"},
    {"connections",i18n("Connections"),i18n("USB, NFC and Ethernet"),"network-wired"},
    {"media",i18n("Sound"),i18n("Audio devices and microphone"),"audio-card"},
    {"storage",i18n("Storage and memory"),i18n("Available space and memory"),"drive-harddisk"},
    {"updates",i18n("System and updates"),i18n("Software, kernels and device information"),"system-software-update"},
    {"diagnostics",i18n("Help and diagnostics"),i18n("Local reports and saved observations"),"document-properties"},
    {"recovery",i18n("Advanced"),i18n("Specialist controls and guarded maintenance"),"security-high"}};
  QVariantList out;
  for(int i=0;i<list.size();++i) {const auto &a=list[i];out<<QVariantMap{{"id",a.id},{"title",a.title},{"icon",a.icon},{"caption",a.caption},{"description",a.caption},{"detail",a.caption},{"index",i},{"secondary",i>=7}};}
  return out;
}
QString actionRole(const Action &a) {
  if(a.tests_package&&a.effect!=Effect::read)return i18n("Attended diagnostic");
  switch(a.effect) {
    case Effect::read:return a.presentation==Presentation::capture?i18n("Information"):i18n("Read in terminal");
    case Effect::device:return i18n("Preference");
    case Effect::packages:return i18n("Software operation");
    case Effect::local_file:return i18n("Files");
    case Effect::maintenance:return i18n("Maintenance");
    case Effect::publish:return i18n("External publication");
  }
  return {};
}
QString accessLabel(const Action &a) {
  switch(a.access) {case Access::root:return i18n("Administrator access");case Access::active_session:return i18n("Desktop session");case Access::user:return i18n("Standard user access");}
  return {};
}
int actionArea(const Action &a) {
  const auto &id=a.id;
  if(id=="boot.set-slot"||id=="boot.slot-preview"||a.category=="updates"||id=="overview.recovery")return 6;
  if(a.effect==Effect::maintenance||a.category=="recovery"||a.category=="developer"||a.category=="sensors")return 8;
  if(a.category=="battery"||a.category=="charge"||a.category=="wake")return 1;
  if(a.category=="display"||a.category=="desktop"||a.category=="lights")return 2;
  if(a.category=="usb"||a.category=="network"||id.starts_with("nfc.")||id=="overview.nfc")return 3;
  if(id=="overview.camera"||id=="hardware.status"||id=="firmware.status"||id=="overview.hardware")return 6;
  if(a.category=="media")return 4;
  if(a.category=="storage"||id=="overview.workload")return 5;
  if(a.category=="diagnostics")return 7;
  return id=="status"||id=="overview.all"?0:6;
}
QVariantMap actionItem(const Action &a) {
  return {{"id",QString::fromStdString(a.id)},{"title",friendlyTitle(a)},{"description",summary(a)},{"icon",actionIcon(a)},{"role",actionRole(a)},{"access",accessLabel(a)},{"area",actionArea(a)},{"read",a.effect==Effect::read}};
}
QVariantList taskGroups(int area) {
  QVariantList entries;
  for(const auto &a:actions())if(actionArea(a)==area)entries<<actionItem(a);
  if(entries.isEmpty())return {};
  return {QVariantMap{{"title",areaModel()[area].toMap().value("title")},{"description",i18n("Technical actions in this category")},{"entries",entries}}};
}
bool matchesAction(const Action &a,const QString &query) {
  const auto hay=QString::fromStdString(a.id+" "+a.title.en+" "+a.title.tr+" "+a.description.en+" "+a.description.tr)+" "+friendlyTitle(a)+" "+actionRole(a);
  for(const auto &word:query.simplified().split(' ',Qt::SkipEmptyParts))if(!hay.contains(word,Qt::CaseInsensitive))return false;
  return true;
}
} // namespace uq
