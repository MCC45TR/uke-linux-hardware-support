// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <algorithm>
#include <cmath>
namespace uq {
QVariantMap normaliseHistory(const QVariantList &input,qint64 start,qint64 end) {
  QVariantList records;
  if(start>=end||end-start>86400||input.size()>4096)return {{"busy",false},{"available",false},{"message",i18n("Invalid history response")}};
  for(const auto &entry:input) {
    const auto v=entry.toList();if(v.size()!=3)continue;
    bool tOK=false,vOK=false,sOK=false;
    const auto t=v[0].toLongLong(&tOK);const auto n=v[1].toDouble(&vOK);const auto s=v[2].toUInt(&sOK);
    if(!tOK||!vOK||!sOK||t<start||t>end||!std::isfinite(n)||n<0||n>100||s>6)continue;
    records<<QVariantMap{{"time",t},{"value",n},{"state",s}};
  }
  std::stable_sort(records.begin(),records.end(),[](const QVariant &a,const QVariant &b){return a.toMap().value("time").toLongLong()<b.toMap().value("time").toLongLong();});
  if(records.size()>720)records.erase(records.begin(),records.end()-720);
  QVariantList segments,segment; qint64 previous=-1,first=0,last=0;int points=0,gaps=0;
  auto flush=[&]{if(!segment.isEmpty())segments<<QVariant(segment);segment.clear();};
  for(const auto &record:records) {
    const auto p=record.toMap();const auto time=p.value("time").toLongLong();
    // Unknown markers, duplicate timestamps and long uncovered spans break a
    // line. No placeholder zero or extended last value becomes a measurement.
    if(p.value("state").toUInt()==0||time==previous){flush();++gaps;previous=time;continue;}
    if(previous>=0&&time-previous>1800){flush();++gaps;}
    if(!first)first=time;last=time;previous=time;++points;segment<<p;
  }
  flush();
  return {{"busy",false},{"available",points>=2},{"segments",segments},{"points",points},{"gaps",gaps},
          {"start",start},{"end",end},{"first",first},{"last",last},
          {"message",points>=2?i18n("Recorded battery percentage. Gaps are not measurements."):i18n("Not enough valid history yet")}};
}
void Settings::call(const QString &path,const QString &interface,const QString &method,const QVariantList &args,unsigned generation,std::function<void(const QDBusMessage &)> done) {
  auto message=QDBusMessage::createMethodCall("org.freedesktop.UPower",path,interface,method);message.setArguments(args);
  auto *watcher=new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message,3000),this);
  connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher,generation,done=std::move(done)] {
    const auto reply=watcher->reply();watcher->deleteLater();
    if(generation==historyGeneration_)done(reply);
  });
}
void Settings::loadHistory(unsigned seconds) {
  if(!topic_||topic_->id!="battery"||(seconds!=3600&&seconds!=86400))return;
  const auto generation=++historyGeneration_;
  history_={{"busy",true},{"available",false},{"message",i18n("Reading existing battery history…")}};emit historyChanged();
  const auto end=QDateTime::currentSecsSinceEpoch();
  if(fixture_.enabled) {
    // Fixture data can never fall through to the real system bus.
    const auto data=fixture_.results.value("battery.status").toObject().value("data").toObject();
    auto records=data.value("history").toArray().toVariantList();
    const auto fixtureEnd=data.value("history_end").toInteger(end);
    history_=normaliseHistory(records,fixtureEnd-seconds,fixtureEnd);emit historyChanged();return;
  }
  call("/org/freedesktop/UPower","org.freedesktop.UPower","EnumerateDevices",{},generation,[this,generation,seconds](const QDBusMessage &reply) {
    if(reply.type()==QDBusMessage::ErrorMessage||reply.arguments().size()!=1) {
      history_={{"busy",false},{"available",false},{"message",i18n("Battery history is unavailable from UPower")}};emit historyChanged();return;
    }
    const auto paths=qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().first());
    QStringList bounded;
    for(const auto &path:paths) {
      if(bounded.size()>=32)break;
      if(QRegularExpression("^/org/freedesktop/UPower/devices/[A-Za-z0-9_]+$").match(path.path()).hasMatch())bounded<<path.path();
    }
    discoverHistory(bounded,0,{},generation,seconds);
  });
}
void Settings::discoverHistory(const QStringList &paths,int index,const QStringList &found,unsigned generation,unsigned seconds) {
  if(index>=paths.size()) {
    if(found.size()!=1) {
      history_={{"busy",false},{"available",false},{"message",found.size()>1?i18n("Multiple battery histories found; no source was guessed"):i18n("The battery provider has no recorded history")}};emit historyChanged();return;
    }
    const auto end=QDateTime::currentSecsSinceEpoch();
    call(found.first(),"org.freedesktop.UPower.Device","GetHistory",{QString("charge"),seconds,uint(720)},generation,[this,end,seconds](const QDBusMessage &reply) {
      QVariantList records;
      if(reply.type()!=QDBusMessage::ErrorMessage&&reply.arguments().size()==1) {
        const auto argument=reply.arguments().first().value<QDBusArgument>();
        if(argument.currentSignature()=="a(udu)") {
          argument.beginArray();
          while(!argument.atEnd()&&records.size()<4096) {
            uint time=0,state=0;double percent=0;
            argument.beginStructure();argument>>time>>percent>>state;argument.endStructure();
            records<<QVariant(QVariantList{time,percent,state});
          }
          argument.endArray();
        }
      }
      history_=normaliseHistory(records,end-seconds,end);emit historyChanged();
    });
    return;
  }
  const auto path=paths[index];
  call(path,"org.freedesktop.DBus.Properties","GetAll",{QString("org.freedesktop.UPower.Device")},generation,[this,paths,index,found,generation,seconds,path](const QDBusMessage &reply) {
    auto sources=found;
    if(reply.type()!=QDBusMessage::ErrorMessage&&reply.arguments().size()==1) {
      const auto properties=qdbus_cast<QVariantMap>(reply.arguments().first());
      if(properties.value("Type").toUInt()==2&&properties.value("PowerSupply").toBool()&&properties.value("HasHistory").toBool())sources<<path;
    }
    discoverHistory(paths,index+1,sources,generation,seconds);
  });
}
HistoryPlot::HistoryPlot(QQuickItem *parent):QQuickPaintedItem(parent) {
  setAntialiasing(true);
  connect(qGuiApp,&QGuiApplication::paletteChanged,this,[this]{update();});
}
void HistoryPlot::setReport(const QVariantMap &report){if(report_==report)return;report_=report;update();emit reportChanged();}
void HistoryPlot::paint(QPainter *p) {
  p->setRenderHint(QPainter::Antialiasing);const auto palette=qGuiApp->palette();
  p->setFont(QGuiApplication::font());
  const QRectF plot(42,12,std::max(1.,width()-56),std::max(1.,height()-42));
  const auto start=report_.value("start").toLongLong(),end=report_.value("end").toLongLong();
  p->setPen(palette.color(QPalette::Mid));
  for(int n:{0,50,100}) {const auto y=plot.bottom()-plot.height()*n/100.;p->drawLine(QPointF(plot.left(),y),QPointF(plot.right(),y));p->drawText(QRectF(0,y-10,36,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(n)+"%");}
  if(end<=start)return;
  p->setPen(palette.color(QPalette::Text));
  const auto fmt=QLocale().timeFormat(QLocale::ShortFormat);
  p->drawText(QRectF(plot.left(),plot.bottom()+8,110,24),Qt::AlignLeft,QDateTime::fromSecsSinceEpoch(start).toString(fmt));
  p->drawText(QRectF(plot.right()-110,plot.bottom()+8,110,24),Qt::AlignRight,QDateTime::fromSecsSinceEpoch(end).toString(fmt));
  p->save();p->setClipRect(plot.adjusted(-3,-3,3,3));
  const auto color=palette.color(QPalette::Highlight);p->setPen(QPen(color,2));p->setBrush(color);
  for(const auto &entry:report_.value("segments").toList()) {
    QPolygonF points;
    for(const auto &item:entry.toList()) {const auto v=item.toMap();points<<QPointF(plot.left()+plot.width()*(v.value("time").toLongLong()-start)/double(end-start),plot.bottom()-plot.height()*v.value("value").toDouble()/100.);}
    if(points.size()>1)p->drawPolyline(points);
    for(const auto &point:points)p->drawEllipse(point,2.5,2.5);
  }
  p->restore();
}
} // namespace uq
