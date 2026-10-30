// SPDX-License-Identifier: MIT
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QVariantList>

class OverviewModel:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList cards READ cards NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool simulation READ simulation NOTIFY changed)
    QVariantList cards_;
    QStringList queue_;
    QSet<QProcess*> running_;
    bool fixture_=false;
    void pump();
    int index(const QString& domain) const;
    void update(const QString& domain,const QString& state,const QJsonObject& result={});
public:
    explicit OverviewModel(QObject* parent=nullptr);
    ~OverviewModel() override;
    QVariantList cards() const{return cards_;}
    bool busy() const{return !running_.isEmpty()||!queue_.isEmpty();}
    bool simulation() const{return fixture_;}
    Q_INVOKABLE void refresh();
    // Native test entry: fixture mode never launches a process or hardware query.
    Q_INVOKABLE bool setFixture(const QJsonObject& overview);
    void completeFixture(const QString& domain,const QString& state,const QJsonObject& result={});
Q_SIGNALS:
    void changed();
};
