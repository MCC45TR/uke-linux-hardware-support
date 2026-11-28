// SPDX-License-Identifier: MIT
#pragma once
#include <QtCore>
#include <functional>
namespace uq {
QJsonObject performanceSnapshot();
QJsonObject frequencyDevice(const QString &kind,const QString &name);
QString frequencyRefusal(const QJsonObject &device,const QString &field,const QString &value);
struct FrequencyResult {bool applied=false;QString error;QJsonObject after;};
FrequencyResult changeFrequency(const QJsonObject &before,const QString &field,const QString &value,
    const QJsonObject &expected,const std::function<QJsonObject()> &read,
    const std::function<bool(const QString &,const QString &)> &write);
}
