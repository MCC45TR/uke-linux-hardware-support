// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
namespace uq {
QJsonArray applicationScopes();
QJsonObject applicationScope(const QString &unit);
QJsonObject applyApplicationPreset(const QString &unit,const QString &invocation,const QString &fingerprint,const QString &preset);
}
