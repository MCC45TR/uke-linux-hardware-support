// SPDX-License-Identifier: MIT
#include "kernel-manager.hpp"
#include <QPainter>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <KLocalizedString>
#include <cmath>
#include <algorithm>
#include <limits>
#include <QPalette>
namespace uq {
TimePlot::TimePlot(QQuickItem *parent):QQuickPaintedItem(parent){setAntialiasing(true);setAcceptHoverEvents(true);setAcceptedMouseButtons(Qt::LeftButton);}
void TimePlot::setReport(const QVariantMap &v){report_=v;update();emit reportChanged();}
void TimePlot::inspect(double x) {
    const auto left=62.,span=std::max(1.,width()-76);const auto start=report_.value("start").toDouble(),end=report_.value("end").toDouble();
    const auto target=start+std::clamp((x-left)/span,0.,1.)*(end-start);
    double distance=std::numeric_limits<double>::max(),time=0,value=0;bool found=false;
    for(const auto &s:report_.value("segments").toList())for(const auto &v:s.toList()){
        const auto p=v.toMap();const auto d=std::abs(p.value("time").toDouble()-target);
        if(d<distance){distance=d;time=p.value("time").toDouble();value=p.value("value").toDouble();found=true;}
    }
    cursorTime_=found?time:-1;cursor_=found?i18n("%1 s ago · %2",QLocale().toString(std::max(0.,end-time),'f',1),QLocale().toString(value,'f',2)):QString();update();emit cursorChanged();
}
void TimePlot::hoverMoveEvent(QHoverEvent *e){inspect(e->position().x());e->accept();}
void TimePlot::mousePressEvent(QMouseEvent *e){inspect(e->position().x());e->accept();}
void TimePlot::paint(QPainter *p) {
    const auto fg=foreground_.isValid()?foreground_:qGuiApp->palette().color(QPalette::Text),accent=accent_.isValid()?accent_:qGuiApp->palette().color(QPalette::Highlight);
    auto grid=fg;grid.setAlpha(64);p->setFont(QGuiApplication::font());p->setRenderHint(QPainter::Antialiasing);
    const QRectF area(62,14,std::max(1.,width()-76),std::max(1.,height()-48));
    const double low=report_.value("minimum").toDouble(),high=report_.value("maximum").toDouble(),start=report_.value("start").toDouble(),end=report_.value("end").toDouble();
    if(high<=low||end<=start)return;
    for(int i=0;i<=2;++i){const auto y=area.bottom()-area.height()*i/2.;p->setPen(grid);p->drawLine(QPointF(area.left(),y),QPointF(area.right(),y));p->setPen(fg);p->drawText(QRectF(0,y-10,54,20),Qt::AlignRight|Qt::AlignVCenter,QLocale().toString(low+(high-low)*i/2.,'g',4));}
    p->setPen(fg);p->drawText(QRectF(area.left(),area.bottom()+8,130,24),Qt::AlignLeft,i18n("%1 s ago",QLocale().toString(end-start,'f',0)));
    p->drawText(QRectF(area.right()-90,area.bottom()+8,90,24),Qt::AlignRight,i18n("Now"));
    p->save();p->setClipRect(area.adjusted(-2,-2,2,2));p->setPen(QPen(accent,2));p->setBrush(accent);
    for(const auto &segment:report_.value("segments").toList()){
        QPolygonF points;for(const auto &v:segment.toList()){const auto point=v.toMap();const auto t=point.value("time").toDouble(),n=point.value("value").toDouble();if(std::isfinite(t)&&std::isfinite(n))points<<QPointF(area.left()+area.width()*(t-start)/(end-start),area.bottom()-area.height()*(n-low)/(high-low));}
        if(points.size()>1)p->drawPolyline(points);if(points.size()==1)p->drawEllipse(points.first(),2.5,2.5);
    }
    if(cursorTime_>=start&&cursorTime_<=end){p->setPen(QPen(fg,1,Qt::DashLine));const auto x=area.left()+area.width()*(cursorTime_-start)/(end-start);p->drawLine(QPointF(x,area.top()),QPointF(x,area.bottom()));}
    p->restore();
}
}
