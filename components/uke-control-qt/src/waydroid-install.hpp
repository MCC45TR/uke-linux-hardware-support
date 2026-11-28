// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
namespace uq::waydroid {
enum class Step { Packages, Initialize, Prepare, Enable };
struct Snapshot {
  bool installed = false, initialized = false, imagesComplete = false;
  bool integration = false, serviceEnabled = false;
  QString variant;
  bool managed = true;
  bool ready() const { return installed && initialized && imagesComplete && integration && serviceEnabled; }
};
bool validVariant(const QString &variant);
bool completeImage(const QString &path);
struct System {
  virtual ~System() = default;
  virtual Snapshot inspect() = 0;
  virtual bool kernelReady() = 0;
  virtual bool enoughSpace() = 0;
  virtual bool run(Step step, const QString &variant, bool repair) = 0;
  virtual void report(const QString &phase, const QString &error = {}) = 0;
};
bool install(System &system, const QString &variant);
} // namespace uq::waydroid
