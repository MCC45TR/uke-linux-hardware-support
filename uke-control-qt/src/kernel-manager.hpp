// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
#include <QFutureWatcher>
#include <QProcess>
#include <QQuickPaintedItem>
#include <QColor>
#include <functional>
namespace uq {
QVariantMap cpuLoads(const QJsonObject &before,const QJsonObject &after);
class KernelManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList sensors READ sensors NOTIFY devicesChanged)
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap review READ review NOTIFY reviewChanged)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(int interval READ interval NOTIFY changed)
    Fixture fixture_;
    bool restricted_,active_=false,requested_=false,reading_=false,customReader_=false;
    std::function<QJsonObject()> reader_;
    QString mode_,message_,selectedSensor_,outcome_;
    int interval_=2,revision_=0;
    unsigned generation_=0;
    QTimer timer_,queryPoll_,deadline_;
    QElapsedTimer clock_;
    QFutureWatcher<QJsonObject> worker_;
    unsigned workerGeneration_=0;
    std::unique_ptr<Query> query_;
    QProcess *write_=nullptr;
    QByteArray writeOutput_;
    QJsonObject previousTicks_;
    QJsonObject previousCounters_;
    qint64 previousSample_=0;
    QVariantList events_;
    QMap<QString,QString> observed_;
    QVariantList devices_,sensors_;
    QVariantMap state_,review_;
    struct Point {double time;QVariant value;};
    QMap<QString,QVector<Point>> history_;
    void accept(const QJsonObject &snapshot);
    void acceptSensors(const QJsonObject &snapshot);
    void add(const QString &id,const QVariant &value);
    void failure(const QString &message);
public:
    explicit KernelManager(Fixture fixture,QObject *parent=nullptr,bool restricted=false,std::function<QJsonObject()> reader={});
    ~KernelManager() override;
    QVariantList devices()const{return devices_;}
    QVariantList sensors()const{return sensors_;}
    QVariantMap state()const{return state_;}
    QVariantMap review()const{return review_;}
    bool active()const{return active_;}
    bool busy()const{return reading_||bool(query_)||write_;}
    int revision()const{return revision_;}
    QString message()const{return outcome_.isEmpty()?message_:outcome_;}
    int interval()const{return interval_;}
    void open(const QString &mode);
    void close();
    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setInterval(int seconds);
    Q_INVOKABLE void selectSensor(const QString &id);
    Q_INVOKABLE QVariantMap plot(const QString &metric,int seconds=120,double maximum=0)const;
    Q_INVOKABLE QVariantList choices(const QString &id,const QString &field)const;
    Q_INVOKABLE QString current(const QString &id,const QString &field)const;
    Q_INVOKABLE QString format(const QVariant &value,const QString &unit={})const;
    Q_INVOKABLE bool prepare(const QString &id,const QString &field,const QString &value);
    Q_INVOKABLE bool prepareResource(const QString &id,const QString &value);
    Q_INVOKABLE bool prepareApplication(const QString &unit,const QString &preset);
    Q_INVOKABLE QString summary()const;
    Q_INVOKABLE bool exportReport(const QUrl &url);
    Q_INVOKABLE void dismissReview();
    Q_INVOKABLE bool apply(const QString &token);
signals:
    void changed();
    void devicesChanged();
    void reviewChanged();
};
class TimePlot : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantMap report READ report WRITE setReport NOTIFY reportChanged)
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY coloursChanged)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY coloursChanged)
    Q_PROPERTY(QString cursor READ cursor NOTIFY cursorChanged)
    QVariantMap report_;
    QColor foreground_,accent_;
    QString cursor_;
    double cursorTime_=-1;
    void inspect(double x);
protected:
    void hoverMoveEvent(QHoverEvent *event)override;
    void mousePressEvent(QMouseEvent *event)override;
public:
    explicit TimePlot(QQuickItem *parent=nullptr);
    QVariantMap report()const{return report_;}
    void setReport(const QVariantMap &report);
    QColor foreground()const{return foreground_;}
    QColor accent()const{return accent_;}
    void setForeground(const QColor &v){if(v.isValid()&&v!=foreground_){foreground_=v;update();emit coloursChanged();}}
    void setAccent(const QColor &v){if(v.isValid()&&v!=accent_){accent_=v;update();emit coloursChanged();}}
    QString cursor()const{return cursor_;}
    void paint(QPainter *p)override;
signals:
    void reportChanged();
    void coloursChanged();
    void cursorChanged();
};
}
