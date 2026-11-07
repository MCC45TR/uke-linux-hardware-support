// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QVariantMap>

namespace uq {
// Names, rather than file paths or bundled artwork, keep native recoloring,
// scaling and the user's icon theme effective in every QML control.
class ThemeIcons : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap names READ names NOTIFY changed)
public:
  explicit ThemeIcons(QObject *parent = nullptr);
  QVariantMap names() const { return names_; }
  Q_INVOKABLE void refresh();
signals:
  void changed();
private:
  QVariantMap names_;
};
} // namespace uq
