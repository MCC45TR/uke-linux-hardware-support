// SPDX-License-Identifier: MIT
// Read original images without mounting or changing them. Own only our overlay.
#include <QtCore>
#include <QtXml>
#include <ext2fs/ext2fs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>

namespace {
void require(bool ok, const char *what) { if (!ok) throw std::runtime_error(what); }
QByteArray read(const QString &path, qint64 limit = 1024 * 1024) {
  QFile f(path); require(f.open(QIODevice::ReadOnly), "Cannot read media configuration");
  auto bytes = f.read(limit + 1); require(bytes.size() <= limit, "Oversized media configuration");
  return bytes;
}
QByteArray digest(const QByteArray &bytes) { return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex(); }
class Image {
  ext2_filsys fs_ = nullptr;
public:
  explicit Image(const QString &path) {
    const QFileInfo info(path);
    require(info.isFile() && !info.isSymLink(), "Untrusted Android image");
    const auto name = QFile::encodeName(path);
    // EXT2_FLAG_RW is deliberately absent. Never mount or replay an image.
    require(!ext2fs_open(name.constData(), EXT2_FLAG_64BITS, 0, 0, unix_io_manager, &fs_), "Cannot open Android image read-only");
  }
  ~Image() { if (fs_) ext2fs_close(fs_); }
  bool lookup(const char *path, ext2_ino_t &ino) const { return !ext2fs_namei_follow(fs_, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &ino); }
  QByteArray file(const char *path) const {
    ext2_ino_t ino{}; require(lookup(path, ino), "Missing vendor manifest");
    ext2_file_t f{}; require(!ext2fs_file_open(fs_, ino, 0, &f), "Cannot read vendor manifest");
    QByteArray result; char buffer[4096]; unsigned int count{};
    errcode_t error{};
    do { error = ext2fs_file_read(f, buffer, sizeof(buffer), &count); if (!error) result.append(buffer, count); }
    while (!error && count && result.size() <= 1024 * 1024);
    ext2fs_file_close(f);
    require(!error && result.size() <= 1024 * 1024, "Invalid vendor manifest");
    return result;
  }
  bool omxService(const char *directory) const {
    ext2_ino_t ino{}; if (!lookup(directory, ino)) return false;
    bool found = false;
    const auto callback = [](ext2_ino_t, int, struct ext2_dir_entry *entry, int, int, char *, void *data) {
      const QByteArray name(entry->name, ext2fs_dirent_name_len(entry));
      if (entry->inode && name.contains("media.omx")) *static_cast<bool *>(data) = true;
      return 0;
    };
    require(!ext2fs_dir_iterate2(fs_, ino, 0, nullptr, callback, &found), "Cannot inspect Android hardware services");
    return found;
  }
};
void trustedDirectory(const QString &path, uid_t owner, bool create = false) {
  const auto name = QFile::encodeName(path);
  if (create) require(!mkdir(name.constData(), 0755) || errno == EEXIST, "Cannot create media overlay directory");
  struct stat s{};
  require(!lstat(name.constData(), &s) && S_ISDIR(s.st_mode) && s.st_uid == owner && !(s.st_mode & 0022), "Untrusted media overlay directory");
}
void save(const QString &path, const QByteArray &bytes, QFile::Permissions permissions) {
  QSaveFile file(path); require(file.open(QIODevice::WriteOnly), "Cannot stage media overlay");
  file.setPermissions(permissions);
  require(file.write(bytes) == bytes.size() && file.commit(), "Cannot commit media overlay");
}
void apply(const QString &work, const QString &compatible, uid_t owner) {
  if (!QFileInfo::exists(compatible)) return;
  const auto board = read(compatible).split('\0');
  if (!board.contains("xiaomi,uke")) return;
  trustedDirectory(work, owner);
  if (!QFileInfo::exists(work + "/waydroid.cfg")) return;
  QSettings config(work + "/waydroid.cfg", QSettings::IniFormat);
  const auto target = work + "/overlay/vendor/etc/vintf/manifest.xml";
  const auto record = work + "/.uke-media-overlay.json";
  for (const auto &suffix : {"/overlay", "/overlay/vendor", "/overlay/vendor/etc", "/overlay/vendor/etc/vintf"}) {
    const auto path = work + suffix;
    if (QFileInfo::exists(path) || QFileInfo(path).isSymLink()) trustedDirectory(path, owner);
  }
  const auto targetInfo = QFileInfo(target);
  bool owned = false;
  if (targetInfo.exists() || targetInfo.isSymLink()) {
    require(targetInfo.isFile() && !targetInfo.isSymLink(), "Untrusted existing media overlay");
    const QFileInfo recordInfo(record);
    if (recordInfo.isFile() && !recordInfo.isSymLink() && recordInfo.ownerId() == owner &&
        !(recordInfo.permissions() & (QFile::WriteGroup | QFile::WriteOther))) {
      const auto metadata = QJsonDocument::fromJson(read(record)).object();
      const auto current = QString::fromLatin1(digest(read(target)));
      owned = metadata.value("schema") == "org.uke.media-overlay.v1" &&
              (metadata.value("sha256").toString() == current || metadata.value("previous_sha256").toString() == current);
    }
    if (!owned) { puts("Android media: preserving administrator manifest"); return; }
  }
  auto clear = [&] {
    if (owned) { require(QFile::remove(target), "Cannot retire managed media overlay"); QFile::remove(record); }
  };
  if (config.value("waydroid/arch").toString() != "arm64_only" || config.value("waydroid/vendor_type").toString() != "MAINLINE" ||
      config.value("waydroid/mount_overlays").toString().compare("True", Qt::CaseInsensitive)) { clear(); return; }
  auto images = config.value("waydroid/images_path").toString();
  if (images.isEmpty()) images = work + "/images";
  // Do not reinterpret or repair a custom image directory.
  if (images != work + "/images") { clear(); return; }
  trustedDirectory(images, owner);
  Image vendor(images + "/vendor.img"), system(images + "/system.img");
  if (vendor.omxService("/bin/hw") || system.omxService("/system/bin/hw") || system.omxService("/bin/hw")) { clear(); return; }
  QDomDocument doc;
  require(bool(doc.setContent(vendor.file("/etc/vintf/manifest.xml"))), "Malformed vendor XML");
  const auto root = doc.documentElement(); require(root.tagName() == "manifest", "Invalid vendor XML root");
  int removed = 0;
  for (auto node = root.firstChildElement("hal"); !node.isNull();) {
    const auto next = node.nextSiblingElement("hal");
    if (node.firstChildElement("name").text().trimmed() == "android.hardware.media.omx") {
      require(node.attribute("format") == "hidl" && node.firstChildElement("transport").text().trimmed() == "hwbinder", "Unsupported OMX declaration");
      auto parent = node.parentNode(); parent.removeChild(node); removed++;
    }
    node = next;
  }
  require(removed <= 1, "Ambiguous OMX declarations");
  if (!removed) { clear(); return; }
  doc.insertBefore(doc.createComment(" Uke managed: absent OMX service; waydroid/waydroid#2453 "), doc.documentElement());
  const auto bytes = doc.toByteArray(4);
  for (const auto &suffix : {"/overlay", "/overlay/vendor", "/overlay/vendor/etc", "/overlay/vendor/etc/vintf"})
    trustedDirectory(work + suffix, owner, true);
  if (owned && read(target) == bytes) { puts("Android media: existing managed manifest is current"); return; }
  const auto metadata = QJsonDocument(QJsonObject{{"schema", "org.uke.media-overlay.v1"},
      {"sha256", QString::fromLatin1(digest(bytes))},
      {"previous_sha256", owned ? QString::fromLatin1(digest(read(target))) : QString{}}}).toJson(QJsonDocument::Compact);
  // Admit either generation if interrupted between the two atomic writes.
  save(record, metadata, QFile::ReadOwner | QFile::WriteOwner);
  save(target, bytes, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
  puts("Android media: omitted the declared but absent OMX service");
}
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
#ifdef UKE_OMX_FIXTURES
    require(argc == 2, "Expected fixture root");
    const auto work = QString::fromLocal8Bit(argv[1]);
    apply(work, work + "/compatible", geteuid());
#else
    require(argc == 1 && geteuid() == 0, "Fixed administrator-only operation");
    apply("/var/lib/waydroid", "/proc/device-tree/compatible", 0);
#endif
    return 0;
  } catch (const std::exception &e) { fprintf(stderr, "Android media preparation: %s\n", e.what()); return 1; }
}
