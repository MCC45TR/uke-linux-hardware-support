// SPDX-License-Identifier: MIT
#include "kernel-manager.hpp"
#include <QPainter>
#include <QFontMetricsF>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <KLocalizedString>
#include <cmath>
#include <algorithm>
#include <limits>
#include <QPalette>
namespace uq {
namespace {
QString axisValue(double n) { return QLocale().toString(n,'g',4); }
QRectF plotArea(double width,double height,double low,double high) {
    const QFontMetricsF metrics(QGuiApplication::font());
    const auto textWidth=std::max({metrics.horizontalAdvance(axisValue(low)),
        metrics.horizontalAdvance(axisValue((low+high)/2)),metrics.horizontalAdvance(axisValue(high))});
    const double left=std::min(width*.45,textWidth+14);
    const double top=metrics.height()/2+4;
    return {left,top,std::max(1.,width-left-8),
            std::max(1.,height-top-metrics.height()*1.5-8)};
}
}
TimePlot::TimePlot(QQuickItem *parent):QQuickPaintedItem(parent){setAntialiasing(true);setAcceptHoverEvents(true);setAcceptedMouseButtons(Qt::LeftButton);}
void TimePlot::setReport(const QVariantMap &v){report_=v;update();emit reportChanged();}
void TimePlot::inspect(double x) {
    const auto area=plotArea(width(),height(),report_.value("minimum").toDouble(),report_.value("maximum").toDouble());
    const auto start=report_.value("start").toDouble(),end=report_.value("end").toDouble();
    const auto target=start+std::clamp((x-area.left())/area.width(),0.,1.)*(end-start);
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
    const double low=report_.value("minimum").toDouble(),high=report_.value("maximum").toDouble(),start=report_.value("start").toDouble(),end=report_.value("end").toDouble();
    if(high<=low||end<=start)return;
    const auto area=plotArea(width(),height(),low,high);
    const auto font=p->fontMetrics();
    const auto labelHeight=font.height();
    for(int i=0;i<=2;++i){const auto y=area.bottom()-area.height()*i/2.;p->setPen(grid);p->drawLine(QPointF(area.left(),y),QPointF(area.right(),y));p->setPen(fg);p->drawText(QRectF(0,y-labelHeight/2.,area.left()-7,labelHeight),Qt::AlignRight|Qt::AlignVCenter,axisValue(low+(high-low)*i/2.));}
    const auto now=i18n("Now");
    const auto ago=i18n("%1 s ago",QLocale().toString(end-start,'f',0));
    const auto rightWidth=font.horizontalAdvance(now);
    const auto labelWidth=std::max(0.,area.width()-rightWidth-8);
    const auto labelLine=area.bottom()+6;
    p->setPen(fg);p->drawText(QRectF(area.left(),labelLine,labelWidth,labelHeight),Qt::AlignLeft,font.elidedText(ago,Qt::ElideRight,int(labelWidth)));
    p->drawText(QRectF(area.right()-rightWidth,labelLine,rightWidth,labelHeight),Qt::AlignRight,now);
    p->save();p->setClipRect(area.adjusted(-2,-2,2,2));p->setPen(QPen(accent,2.5));p->setBrush(accent);
    for(const auto &segment:report_.value("segments").toList()){
        QPolygonF points;for(const auto &v:segment.toList()){const auto point=v.toMap();const auto t=point.value("time").toDouble(),n=point.value("value").toDouble();if(std::isfinite(t)&&std::isfinite(n))points<<QPointF(area.left()+area.width()*(t-start)/(end-start),area.bottom()-area.height()*(n-low)/(high-low));}
        if(points.size()>1)p->drawPolyline(points);if(points.size()==1)p->drawEllipse(points.first(),2.5,2.5);
    }
    if(cursorTime_>=start&&cursorTime_<=end){p->setPen(QPen(fg,1,Qt::DashLine));const auto x=area.left()+area.width()*(cursorTime_-start)/(end-start);p->drawLine(QPointF(x,area.top()),QPointF(x,area.bottom()));}
    p->restore();
}
}
