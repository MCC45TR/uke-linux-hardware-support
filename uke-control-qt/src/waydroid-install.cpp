// SPDX-License-Identifier: MIT
#include "waydroid-install.hpp"
#include <QtEndian>
#include <limits>
namespace uq::waydroid {
bool validVariant(const QString &variant) { return variant == "GAPPS" || variant == "VANILLA"; }
bool completeImage(const QString &path) {
  const QFileInfo info(path);
  if (!info.isFile() || info.isSymLink() || info.size() < 8 * 1024 * 1024) return false;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || !file.seek(1024)) return false;
  const auto super = file.read(1024);
  if (super.size() != 1024 || qFromLittleEndian<quint16>(super.constData() + 0x38) != 0xef53) return false;
  const auto shift = qFromLittleEndian<quint32>(super.constData() + 0x18);
  if (shift > 6) return false;
  quint64 blocks = qFromLittleEndian<quint32>(super.constData() + 4);
  if (qFromLittleEndian<quint32>(super.constData() + 0x60) & 0x80)
    blocks |= quint64(qFromLittleEndian<quint32>(super.constData() + 0x150)) << 32;
  const quint64 blockSize = quint64(1024) << shift;
  return blocks && blocks <= std::numeric_limits<quint64>::max() / blockSize &&
         blocks * blockSize <= quint64(info.size());
}
bool install(System &system, const QString &variant) {
  auto fail = [&](const QString &error) { system.report("failed", error); return false; };
  if (!validVariant(variant)) return fail("invalid-variant");
  auto before = system.inspect();
  // Never replace a working Android variant, erase user data, or force a reset.
  if (!before.variant.isEmpty() && before.variant != variant) return fail("variant-conflict");
  if (!system.kernelReady()) return fail("system-update-required");
  if (before.ready()) { system.report("ready"); return true; }
  if (!before.managed) return fail("custom-installation");
  if (!before.imagesComplete && !system.enoughSpace()) return fail("insufficient-space");
  system.report("packages");
  if (!system.run(Step::Packages, variant, false)) return fail("packages-failed");
  auto current = system.inspect();
  if (!current.variant.isEmpty() && current.variant != variant) return fail("variant-conflict");
  if (!current.initialized || !current.imagesComplete) {
    system.report("images");
    if (!system.run(Step::Initialize, variant, current.initialized)) return fail("images-failed");
    current = system.inspect();
    // Original init can return zero after refusing configuration. Verify results.
    if (!current.initialized || !current.imagesComplete || current.variant != variant)
      return fail("images-incomplete");
  }
  system.report("configure");
  if (!system.run(Step::Prepare, variant, false)) return fail("configuration-failed");
  system.report("service");
  if (!system.run(Step::Enable, variant, false)) return fail("service-failed");
  if (!system.inspect().ready()) return fail("installation-incomplete");
  system.report("ready");
  return true;
}
} // namespace uq::waydroid
