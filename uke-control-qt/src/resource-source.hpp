// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
namespace uq {
// Alternate roots are exclusively for native fixture tests; privileged callers
// never accept roots or paths from their arguments.
struct ResourceRoots { QString proc="/proc",sys="/sys"; };
QJsonObject resourceSnapshot(const QString &domain,const ResourceRoots &roots={});
QJsonObject resourceSetting(const QString &id,const ResourceRoots &roots={});
QString resourceRefusal(const QJsonObject &setting,const QString &value);
QJsonObject resourceRates(const QJsonObject &before,const QJsonObject &after,double seconds);
}
