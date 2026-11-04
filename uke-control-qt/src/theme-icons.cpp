// SPDX-License-Identifier: MIT
#include "theme-icons.hpp"
#include <KIconLoader>
#include <QIcon>
#include <QStringList>

namespace uq {
ThemeIcons::ThemeIcons(QObject *parent) : QObject(parent) {
  connect(KIconLoader::global(), &KIconLoader::iconChanged, this,
          &ThemeIcons::refresh);
  refresh();
}
void ThemeIcons::refresh() {
  // Prefer standard theme names. The platform's inherited/fallback themes
  // (Breeze on KDE) supply names omitted by smaller third-party themes.
  // Never replace QIcon::themeName() or load an absolute Breeze file.
  static const QStringList standardNames{
      "dialog-information", "dialog-warning", "dialog-error", "dialog-ok",
      "battery", "battery-charging", "video-display", "network-wired",
      "network-wireless", "audio-card", "audio-input-microphone",
      "system-software-update", "preferences-system", "preferences-desktop",
      "preferences-desktop-display", "preferences-system-power-management",
      "preferences-system-time", "preferences-desktop-color", "camera-photo",
      "camera-flash", "drive-harddisk", "drive-removable-media-usb", "cpu",
      "input-touchpad", "input-gaming", "utilities-system-monitor",
      "utilities-terminal", "document-open", "document-save", "document-preview",
      "document-export", "document-send", "document-properties", "folder",
      "view-list-details", "view-refresh", "view-filter", "edit-find",
      "edit-clear", "configure", "system-run", "system-reboot", "system-shutdown",
      "system-switch-user", "security-high", "go-previous", "go-next",
      "media-playback-stop"};
  QVariantMap names;
  for (const auto &name : standardNames) {
    const auto candidates = QStringList{name, "preferences-system",
                                        "dialog-information"};
    for (const auto &candidate : candidates)
      if (QIcon::hasThemeIcon(candidate)) {
        names.insert(name, candidate);
        break;
      }
  }
  names_ = names;
  // Emit even when the names stay equal: the underlying theme artwork changed.
  emit changed();
}
} // namespace uq
