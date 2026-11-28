// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"

namespace uq {
QVariantList areaModel();
QVariantList taskGroups(int area);
int actionArea(const Action &action);
QString actionRole(const Action &action);
QString accessLabel(const Action &action);
QVariantMap actionItem(const Action &action);
bool matchesAction(const Action &action, const QString &query);
} // namespace uq
