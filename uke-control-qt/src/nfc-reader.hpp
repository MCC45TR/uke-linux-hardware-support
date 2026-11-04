// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <QNearFieldManager>
#include <QNearFieldTarget>
#include <QNdefMessage>
#include <QPointer>
#include <QProcess>
namespace uq {
QVariantList ndefRows(const QNdefMessage &message);
class NfcReader : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap report READ report NOTIFY changed)
    bool simulation_, restricted_, scanning_=false;
    QNearFieldManager *manager_=nullptr;
    QPointer<QNearFieldTarget> target_;
    QTimer timeout_;
    QProcess *service_=nullptr;
    QVariantMap report_;
    void release();
    void complete(const QString &message);
public:
    explicit NfcReader(bool simulation,QObject *parent=nullptr,bool restricted=false);
    ~NfcReader() override;
    QVariantMap report() const {return report_;}
    Q_INVOKABLE void inspect();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clear();
signals:
    void changed();
};
}
