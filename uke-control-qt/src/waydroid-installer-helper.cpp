// SPDX-License-Identifier: MIT
// Fixed privileged operations; the original Fedora DNF/Waydroid own installation.
#include "waydroid-install.hpp"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
using namespace uq::waydroid;
namespace {
const QString stateDirectory = "/var/lib/uke-waydroid-installer";
const QString statePath = stateDirectory + "/state.json";
const QString runtimeDirectory = "/run/uke-waydroid-installer";
const QString servicePrefix = "uke-waydroid-install@";
void need(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void output(const QJsonObject &value) {
  const auto data = QJsonDocument(value).toJson(QJsonDocument::Compact) + '\n';
  fwrite(data.constData(), 1, size_t(data.size()), stdout);
}
QByteArray read(const QString &path, qint64 limit = 65536) {
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.read(limit) : QByteArray{};
}
bool kernelReady() {
  if (!read("/sys/firmware/devicetree/base/compatible").contains("xiaomi,uke")) return false;
  struct utsname host{};
  if (uname(&host)) return false;
  const auto config = read("/usr/lib/modules/" + QString::fromLocal8Bit(host.release) + "/config", 1048576);
  for (const auto *flag : {"ANDROID_BINDER_IPC", "ANDROID_BINDERFS", "IP_ADVANCED_ROUTER",
                          "IP_MULTIPLE_TABLES", "IPV6_MULTIPLE_TABLES", "NETFILTER_XTABLES_LEGACY",
                          "IP_NF_IPTABLES_LEGACY", "IP6_NF_IPTABLES_LEGACY", "IP_NF_FILTER", "IP6_NF_FILTER"})
    if (!config.contains(QByteArray("CONFIG_") + flag + "=y\n")) return false;
  return true;
}
Snapshot inspect() {
  Snapshot s;
  s.installed = QFileInfo("/usr/bin/waydroid").isExecutable();
  s.integration = QFileInfo("/usr/libexec/uke-waydroid-prepare").isExecutable();
  const QString config = "/var/lib/waydroid/waydroid.cfg";
  if (QFileInfo(config).isFile()) {
    QSettings cfg(config, QSettings::IniFormat);
    const auto ota = cfg.value("waydroid/system_ota").toString();
    if (ota.endsWith("/GAPPS.json")) s.variant = "GAPPS";
    if (ota.endsWith("/VANILLA.json")) s.variant = "VANILLA";
    const auto images = cfg.value("waydroid/images_path").toString();
    s.managed = images.isEmpty() || images == "/var/lib/waydroid/images";
    s.initialized = !cfg.value("waydroid/arch").toString().isEmpty() &&
                    QFileInfo("/var/lib/waydroid/rootfs").isDir();
    s.imagesComplete = !images.isEmpty() && completeImage(images + "/system.img") && completeImage(images + "/vendor.img");
  }
  s.serviceEnabled = QFileInfo("/etc/systemd/system/multi-user.target.wants/waydroid-container.service").isSymLink();
  return s;
}
void directory(const QString &path, mode_t mode) {
  const auto bytes = QFile::encodeName(path);
  need(!mkdir(bytes.constData(), mode) || errno == EEXIST, "Cannot create installer directory");
  struct stat st{};
  need(!lstat(bytes.constData(), &st) && S_ISDIR(st.st_mode) && st.st_uid == 0 &&
       !(st.st_mode & 0022), "Untrusted installer directory");
}
struct Lock {
  int fd = -1;
  explicit Lock(bool wait = false) {
    directory(runtimeDirectory, 0700);
    const auto path = QFile::encodeName(runtimeDirectory + "/lock");
    fd = open(path.constData(), O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
    struct stat st{};
    need(fd >= 0 && !fstat(fd, &st) && S_ISREG(st.st_mode) && st.st_uid == 0 && st.st_nlink == 1,
         "Untrusted installer lock");
    if (flock(fd, wait ? LOCK_EX : LOCK_EX | LOCK_NB)) { close(fd); fd = -1; throw std::runtime_error("installer-busy"); }
  }
  ~Lock() { if (fd >= 0) close(fd); }
};
void save(const QString &variant, const QString &phase, const QString &error = {}, double progress = -1) {
  directory(stateDirectory, 0755);
  QSaveFile f(statePath);
  need(f.open(QIODevice::WriteOnly), "Cannot open installer status");
  f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
  const auto data = QJsonDocument(QJsonObject{{"variant", variant}, {"phase", phase},
      {"error", error}, {"progress", progress}}).toJson(QJsonDocument::Compact);
  need(f.write(data) == data.size() && f.commit(), "Cannot save installer status");
}
bool active(const QString &variant) {
  QProcess p;
  p.start("/usr/bin/systemctl", {"show", "--property=ActiveState", "--value", servicePrefix + variant + ".service"});
  if (!p.waitForFinished(1500)) { p.kill(); p.waitForFinished(); return false; }
  const auto state = p.readAllStandardOutput().trimmed();
  return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0 &&
      (state == "active" || state == "activating" || state == "reloading" || state == "deactivating");
}
QJsonObject status() {
  const auto s = inspect();
  QJsonObject o{{"schema", "org.uke.waydroid-installer.v1"}, {"installed", s.installed},
      {"initialized", s.initialized}, {"images_complete", s.imagesComplete}, {"ready", s.ready()},
      {"variant", s.variant}, {"kernel_ready", kernelReady()}, {"busy", false}, {"phase", "idle"},
      {"error", ""}, {"progress", -1}};
  const auto job = QJsonDocument::fromJson(read(statePath)).object();
  const bool busy = active("GAPPS") || active("VANILLA");
  if (busy || job.value("phase") == "failed") {
    for (const auto *key : {"phase", "error", "progress"}) if (job.contains(key)) o[key] = job[key];
  }
  if (busy && o.value("phase") == "idle") o["phase"] = "starting";
  if (!busy && s.ready()) { o["phase"] = "ready"; o["error"] = ""; }
  if (!busy && !s.ready() && job.value("phase") != "failed" && !job.isEmpty()) {
    o["phase"] = "failed"; o["error"] = "interrupted";
  }
  o["busy"] = busy;
  return o;
}
class RealSystem final : public System {
  QString variant_;
public:
  explicit RealSystem(QString variant) : variant_(std::move(variant)) {}
  Snapshot inspect() override { return ::inspect(); }
  bool kernelReady() override { return ::kernelReady(); }
  bool enoughSpace() override {
    QStorageInfo storage("/var/lib");
    return storage.isValid() && storage.bytesAvailable() >= qint64(6) * 1024 * 1024 * 1024;
  }
  void report(const QString &phase, const QString &error = {}) override { save(variant_, phase, error); }
  bool run(Step step, const QString &variant, bool repair) override {
    QString program;
    QStringList args;
    if (step == Step::Packages) {
      program = "/usr/bin/dnf";
      args = {"-y", "--setopt=install_weak_deps=False", "install", "waydroid", "waydroid-selinux", "uke-waydroid-integration"};
    } else if (step == Step::Initialize) {
      program = "/usr/bin/waydroid";
      args = {"init", "-c", "https://ota.waydro.id/system", "-v", "https://ota.waydro.id/vendor", "-s", variant};
      if (repair) args << "-f";
    } else if (step == Step::Prepare) program = "/usr/libexec/uke-waydroid-prepare";
    else { program = "/usr/bin/systemctl"; args = {"enable", "--now", "waydroid-container.service"}; }
    QProcess p;
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C"); env.insert("PYTHONUNBUFFERED", "1");
    p.setProcessEnvironment(env);
    p.setStandardInputFile(QProcess::nullDevice());
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(program, args);
    if (!p.waitForStarted(10000)) return false;
    QByteArray pending;
    double previous = -1;
    const QRegularExpression download(R"(\[Downloading\]\s*([0-9.]+)\s*MB/([0-9.]+)\s*MB)");
    while (p.state() != QProcess::NotRunning) {
      p.waitForReadyRead(500);
      pending += p.readAll();
      if (pending.size() > 8192) pending = pending.right(8192);
      if (step == Step::Initialize) {
        auto matches = download.globalMatch(QString::fromLocal8Bit(pending));
        double percent = previous;
        while (matches.hasNext()) {
          const auto match = matches.next();
          const double total = match.captured(2).toDouble();
          if (total > 0) percent = qBound(0.0, 100.0 * match.captured(1).toDouble() / total, 100.0);
        }
        if (percent != previous) { save(variant, "images", {}, percent); previous = percent; }
      }
    }
    p.readAll(); // No raw output, account identifiers or repository URLs enter public status.
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
  }
};
QJsonObject googleId() {
  need(inspect().variant == "GAPPS", "Google registration requires a GAPPS installation");
  QProcess p;
  p.start("/usr/bin/waydroid", {"shell", "--", "sh", "-c",
      "for db in /data/data/*/*/gservices.db; do [ -f \"$db\" ] || continue; sqlite3 -readonly \"$db\" 'select value from main where name = \"android_id\";'; done"});
  if (!p.waitForFinished(15000)) { p.kill(); p.waitForFinished(); throw std::runtime_error("google-id-unavailable"); }
  const auto id = QString::fromLatin1(p.readAllStandardOutput()).trimmed();
  need(p.exitCode() == 0 && QRegularExpression("^[0-9]{1,20}$").match(id).hasMatch(), "google-id-unavailable");
  return {{"android_id", id}};
}
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    const auto a = app.arguments();
    if (a.size() == 2 && a[1] == "status") { output(status()); return 0; }
    if (a.size() == 2 && a[1] == "--version") { puts("1.5.0"); return 0; }
    const bool start = a.size() == 3 && a[1] == "start" && validVariant(a[2]);
    const bool worker = a.size() == 3 && a[1] == "worker" && validVariant(a[2]);
    const bool google = a.size() == 2 && a[1] == "google-id";
    need(start || worker || google, "invalid-request");
    need(geteuid() == 0, "administrator-required");
    if (google) { output(googleId()); return 0; }
    // A newly queued service may start before its short authorization process
    // releases the start lock. The worker waits for that handoff.
    Lock lock(worker);
    if (active("GAPPS") || active("VANILLA")) {
      if (start) { output({{"started", false}, {"error", "installer-busy"}}); return 1; }
      // This unit is already activating while its worker begins.
    }
    if (worker) {
      RealSystem system(a[2]);
      return install(system, a[2]) ? 0 : 1;
    }
    const auto current = inspect();
    need(current.variant.isEmpty() || current.variant == a[2], "variant-conflict");
    need(kernelReady(), "system-update-required");
    if (current.ready()) { output({{"started", false}, {"already_ready", true}}); return 0; }
    need(current.managed, "custom-installation");
    save(a[2], "starting");
    QProcess p;
    p.start("/usr/bin/systemctl", {"start", "--no-block", servicePrefix + a[2] + ".service"});
    need(p.waitForFinished(10000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0, "service-start-failed");
    output({{"started", true}});
    return 0;
  } catch (const std::exception &e) { output({{"error", QString::fromUtf8(e.what())}}); return 1; }
}
