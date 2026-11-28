// SPDX-License-Identifier: MIT
#include "waydroid-manager.hpp"
#include "waydroid-install.hpp"
#include <KLocalizedString>
#include <QDesktopServices>
namespace uq {
namespace { const QString helper = "/usr/libexec/uke-control-qt/waydroid-installer"; }
WaydroidManager::WaydroidManager(const Fixture &fixture, bool readOnly, QObject *parent)
    : QObject(parent), simulation_(fixture.enabled), readOnly_(readOnly),
      fixture_(fixture.results.value("waydroid.installer").toObject()) {
  state_ = {{"ready", false}, {"busy", false}, {"initialized", false}, {"phase", "idle"}, {"progress", -1}};
  poll_.setInterval(2000);
  connect(&poll_, &QTimer::timeout, this, &WaydroidManager::refresh);
  deadline_.setSingleShot(true);
  connect(&deadline_, &QTimer::timeout, this, [this] { if (status_.state() != QProcess::NotRunning) status_.kill(); });
  connect(&status_, &QProcess::readyReadStandardOutput, this, [this] {
    statusOutput_ += status_.readAllStandardOutput();
    if (statusOutput_.size() > 65536) status_.kill();
  });
  connect(&status_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this,
      [this](int code, QProcess::ExitStatus exit) {
        deadline_.stop();
        statusOutput_ += status_.readAllStandardOutput();
        if (code == 0 && exit == QProcess::NormalExit) updateState(statusOutput_);
        else showError("status-unavailable");
      });
  connect(&status_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { deadline_.stop(); showError("status-unavailable"); }
  });
  connect(&request_, &QProcess::readyReadStandardOutput, this, [this] {
    requestOutput_ += request_.readAllStandardOutput();
    if (requestOutput_.size() > 65536) request_.kill();
  });
  connect(&request_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this,
      [this](int code, QProcess::ExitStatus exit) {
        requestOutput_ += request_.readAllStandardOutput();
        const auto result = QJsonDocument::fromJson(requestOutput_).object();
        if (code != 0 || exit != QProcess::NormalExit) {
          showError(code == 126 || code == 127 ? "authorization-cancelled" : result.value("error").toString("request-failed"));
        } else if (requestKind_ == "google-id") {
          const auto id = result.value("android_id").toString();
          if (QRegularExpression("^[0-9]{1,20}$").match(id).hasMatch()) googleId_ = id;
          else showError("google-id-unavailable");
        }
        requestOutput_.fill('\0'); requestOutput_.clear();
        emit changed();
        if (watching_) refresh();
      });
  connect(&request_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { showError("request-failed"); emit changed(); }
  });
}
WaydroidManager::~WaydroidManager() { close(); }
void WaydroidManager::open() { watching_ = true; poll_.start(); refresh(); }
void WaydroidManager::close() {
  watching_ = false; poll_.stop(); deadline_.stop();
  if (status_.state() != QProcess::NotRunning) status_.kill();
  googleId_.fill(QChar('\0')); googleId_.clear();
  // The system service owns installation. Closing this view cannot stop DNF.
  emit changed();
}
void WaydroidManager::refresh() {
  if (simulation_) { updateState(QJsonDocument(fixture_.isEmpty() ? state_ : fixture_).toJson()); return; }
  if (status_.state() != QProcess::NotRunning) return;
  statusOutput_.clear();
  status_.setStandardInputFile(QProcess::nullDevice());
  status_.start(helper, {"status"});
  deadline_.start(8000);
}
void WaydroidManager::updateState(const QByteArray &data) {
  const auto value = QJsonDocument::fromJson(data).object();
  if (!simulation_ && value.value("schema") != "org.uke.waydroid-installer.v1") { showError("status-unavailable"); return; }
  state_ = value;
  if (state_.value("phase") == "failed") showError(state_.value("error").toString("request-failed"));
  emit changed();
}
void WaydroidManager::request(const QString &kind, const QStringList &arguments) {
  requestKind_ = kind; requestOutput_.clear(); message_.clear();
  request_.setStandardInputFile(QProcess::nullDevice());
  request_.start("/usr/bin/pkexec", QStringList{helper} + arguments);
  emit changed();
}
bool WaydroidManager::install(const QString &variant) {
  if (blocked() || busy() || !waydroid::validVariant(variant)) return false;
  const auto existing = state_.value("variant").toString();
  if (!existing.isEmpty() && existing != variant) { showError("variant-conflict"); return false; }
  request("install", {"start", variant});
  return true;
}
bool WaydroidManager::launch() {
  if (blocked() || busy() || !state_.value("ready").toBool() || !state_.value("kernel_ready").toBool()) return false;
  if (!QProcess::startDetached("/usr/bin/waydroid", {"show-full-ui"})) { showError("launch-failed"); return false; }
  return true;
}
bool WaydroidManager::readGoogleId() {
  if (blocked() || busy() || state_.value("variant") != "GAPPS" || !state_.value("ready").toBool()) return false;
  request("google-id", {"google-id"});
  return true;
}
bool WaydroidManager::openGoogleRegistration() {
  return !blocked() && !googleId_.isEmpty() && QDesktopServices::openUrl(QUrl("https://www.google.com/android/uncertified"));
}
QString WaydroidManager::phaseText() const {
  if (authorizing()) return i18n("Waiting for administrator authorization…");
  const auto phase = state_.value("phase").toString();
  if (phase == "ready") return i18n("Waydroid is ready");
  if (phase == "starting") return i18n("Starting installation…");
  if (phase == "packages") return i18n("Installing required packages…");
  if (phase == "images") return i18n("Downloading and verifying Android…");
  if (phase == "configure") return i18n("Preparing graphics and application windows…");
  if (phase == "service") return i18n("Enabling Waydroid…");
  if (phase == "failed") return i18n("Installation needs attention");
  return i18n("Install Android applications on this device");
}
void WaydroidManager::showError(const QString &code) {
  if (code == "variant-conflict") message_ = i18n("A different Android edition is already installed. Your applications and data have been kept. Use the existing installation.");
  else if (code == "custom-installation") message_ = i18n("An existing custom Android installation needs its original setup tools. Its settings and user data have been kept.");
  else if (code == "system-update-required") message_ = i18n("Update the system and restart it before installing Waydroid.");
  else if (code == "insufficient-space") message_ = i18n("At least 6 GiB of free storage is required to download and prepare Android.");
  else if (code == "authorization-cancelled") message_ = i18n("Installation was not authorized. You can try again.");
  else if (code == "installer-busy") message_ = i18n("A Waydroid installation is already running. Its progress will appear here.");
  else if (code == "google-id-unavailable") message_ = i18n("Open Waydroid first and wait for Android to start, then read the Google registration identifier again.");
  else if (code == "status-unavailable") message_ = i18n("Waydroid installation status is unavailable. Check that the installer package is installed.");
  else if (code == "packages-failed") message_ = i18n("Required packages could not be installed. Check your network connection and try again.");
  else if (code == "images-failed" || code == "images-incomplete") message_ = i18n("Android could not be downloaded or verified. Check your connection and try again. Existing user data has been kept.");
  else if (code == "launch-failed") message_ = i18n("Waydroid could not be opened. Try again after checking its installation.");
  else message_ = i18n("Waydroid could not be prepared. Your existing data has been kept. Try the installation again.");
  emit changed();
}
}
