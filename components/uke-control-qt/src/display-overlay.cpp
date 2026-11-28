// SPDX-License-Identifier: MIT
#include "display-overlay.hpp"
#include <KLocalizedString>
#include <LayerShellQt/Window>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <cerrno>
#include <QGuiApplication>
#include <QPainter>
#include <QRasterWindow>
#include <QScreen>
#include <QSocketNotifier>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <memory>

namespace uq {
namespace {
QDBusMessage effectRequest(const QString &name) {
  auto request=QDBusMessage::createMethodCall("org.kde.KWin","/Effects","org.kde.kwin.Effects",name);
  request << QStringLiteral("showfps");
  return request;
}

class OverlayWindow : public QRasterWindow {
public:
  explicit OverlayWindow(QScreen *screen) {
    setSurfaceType(QSurface::RasterSurface);
    auto format=requestedFormat();
    format.setAlphaBufferSize(8);
    setFormat(format);
    if (screen) setScreen(screen);
    setTitle(i18n("DPU display monitor"));
    setFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    auto *layer=LayerShellQt::Window::get(this);
    layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop) |
                      LayerShellQt::Window::AnchorLeft);
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setExclusiveZone(-1);
    layer->setMargins(QMargins(10,10,0,0));
    layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    layer->setScope("uke-display-monitor");
    const QFontMetrics metrics(font_);
    resize(metrics.horizontalAdvance(i18n("DPU timing is unavailable")) + 40,metrics.height() * 2 + 22);
  }

  void display(const QString &timing,const QString &detail) {
    if (timing==timing_ && detail==detail_) return;
    timing_=timing;
    detail_=detail;
    const QFontMetrics metrics(font_);
    resize(qMax(metrics.horizontalAdvance(timing_),metrics.horizontalAdvance(detail_)) + 30,
           metrics.height()*2 + 22);
    update();
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(QRect(QPoint(),size()),Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(25,27,30,200));
    painter.drawRoundedRect(QRect(QPoint(),size()).adjusted(0,0,-1,-1),9,9);
    painter.setPen(Qt::white);
    painter.setFont(font_);
    const QFontMetrics metrics(font_);
    painter.drawText(15,8+metrics.ascent(),timing_);
    painter.drawText(15,12+metrics.height()+metrics.ascent(),detail_);
  }

private:
  const QFont font_=QGuiApplication::font();
  QString timing_,detail_;
};

class DisplayCadence : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface","org.uke.ControlQt.DisplayOverlay1")
public:
  explicit DisplayCadence(OverlayWindow &window) : window_(window) {}

  ~DisplayCadence() override {
    release();
    if (ownsEffect_)
      QDBusConnection::sessionBus().call(effectRequest("unloadEffect"),QDBus::Block,1200);
  }

  void start() {
    auto session=QDBusConnection::sessionBus();
    const QDBusReply<bool> alreadyLoaded=session.call(effectRequest("isEffectLoaded"),QDBus::Block,1200);
    if (alreadyLoaded.isValid() && !alreadyLoaded.value()) {
      const QDBusReply<bool> loaded=session.call(effectRequest("loadEffect"),QDBus::Block,1200);
      ownsEffect_=loaded.isValid() && loaded.value();
    }
    scan();
    connect(qGuiApp,&QGuiApplication::screenAdded,this,[this](QScreen *){scan();});
    connect(qGuiApp,&QGuiApplication::screenRemoved,this,[this](QScreen *){scan();});
    for (auto *screen:QGuiApplication::screens())
      connect(screen,&QScreen::refreshRateChanged,this,[this]{scan();});
    session.connect("org.freedesktop.ScreenSaver","/ScreenSaver","org.freedesktop.ScreenSaver",
        "ActiveChanged",this,SLOT(wake(bool)));
  }

public slots:
  void Quit() { QCoreApplication::quit(); }

private slots:
  void wake(bool locked) { if (!locked) scan(); }

private:
  void scan() {
    release();
    const QDir drm("/sys/class/drm");
    for (const auto &entry:drm.entryList({"card*-DSI-*","card*-eDP-*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
      QFile connectorId(drm.filePath(entry+"/connector_id"));
      if (!connectorId.open(QIODevice::ReadOnly)) continue;
      bool ok=false;
      const auto id=connectorId.read(32).trimmed().toUInt(&ok);
      if (!ok || id==0) continue;
      const auto path=QFile::encodeName("/dev/dri/"+entry.section('-',0,0));
      const int fd=open(path.constData(),O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (fd<0) continue;
      std::unique_ptr<drmModeConnector,decltype(&drmModeFreeConnector)> connector(
          drmModeGetConnectorCurrent(fd,id),drmModeFreeConnector);
      std::unique_ptr<drmModeEncoder,decltype(&drmModeFreeEncoder)> encoder(
          connector && connector->encoder_id ? drmModeGetEncoder(fd,connector->encoder_id) : nullptr,drmModeFreeEncoder);
      if (!encoder || !encoder->crtc_id) { close(fd); continue; }
      fd_=fd;
      crtc_=encoder->crtc_id;
      drmSetClientCap(fd_,DRM_CLIENT_CAP_ATOMIC,1);
      updateMode();
      auto notifier=std::make_unique<QSocketNotifier>(fd_,QSocketNotifier::Read,this);
      connect(notifier.get(),&QSocketNotifier::activated,this,&DisplayCadence::readEvent);
      notifier_=std::move(notifier);
      if (queue(1)) {
        window_.display(i18n("Waiting for DPU frames…"),modeText());
        return;
      }
      release();
    }
    window_.display(i18n("DPU timing is unavailable"),i18n("Wake the built-in display to measure its refresh rate."));
  }

  bool queue(uint64_t frames) {
    uint64_t sequence=0;
    return drmCrtcQueueSequence(fd_,crtc_,DRM_CRTC_SEQUENCE_RELATIVE | DRM_CRTC_SEQUENCE_NEXT_ON_MISS,
                                frames,&sequence,0)==0;
  }

  void readEvent() {
    drm_event_crtc_sequence event{};
    const auto size=read(fd_,&event,sizeof(event));
    if (size<0 && (errno==EAGAIN || errno==EINTR)) return;
    if (size!=sizeof(event) || event.base.type!=DRM_EVENT_CRTC_SEQUENCE || event.time_ns<=0) {
      release();
      window_.display(i18n("DPU timing is unavailable"),i18n("The display driver stopped delivering frame events."));
      return;
    }
    if (previous_) {
      const auto frames=event.sequence-previousSequence_;
      const auto elapsed=event.time_ns-previous_;
      if (frames>0 && frames<120 && elapsed>0) {
        const auto frequency=qRound(1e9*double(frames)/double(elapsed));
        if (frequency>=1 && frequency<=1000 &&
            (reported_==0 || event.time_ns-reported_>=350000000)) {
          updateMode();
          window_.display(i18n("DPU %1 Hz",frequency),modeText());
          reported_=event.time_ns;
        }
      }
    }
    previous_=event.time_ns;
    previousSequence_=event.sequence;
    if (!queue(10)) {
      release();
      window_.display(i18n("DPU measurement paused"),i18n("The built-in display is inactive."));
    }
  }

  void updateMode() {
    std::unique_ptr<drmModeCrtc,decltype(&drmModeFreeCrtc)> crtc(drmModeGetCrtc(fd_,crtc_),drmModeFreeCrtc);
    nominal_=crtc && crtc->mode_valid ? int(crtc->mode.vrefresh) : 0;
    vrr_=-1;
    std::unique_ptr<drmModeObjectProperties,decltype(&drmModeFreeObjectProperties)> properties(
        drmModeObjectGetProperties(fd_,crtc_,DRM_MODE_OBJECT_CRTC),drmModeFreeObjectProperties);
    if (!properties) return;
    for (uint32_t i=0; i<properties->count_props; ++i) {
      std::unique_ptr<drmModePropertyRes,decltype(&drmModeFreeProperty)> property(
          drmModeGetProperty(fd_,properties->props[i]),drmModeFreeProperty);
      if (property && QByteArrayView(property->name)=="VRR_ENABLED") {
        vrr_=int(properties->prop_values[i]);
        break;
      }
    }
  }

  QString modeText() const {
    if (vrr_==1) return nominal_>0 ? i18n("VRR on · up to %1 Hz",nominal_) : i18n("VRR is on");
    if (vrr_==0) return nominal_>0 ? i18n("VRR off · mode %1 Hz",nominal_) : i18n("VRR is off");
    return nominal_>0 ? i18n("Configured mode %1 Hz",nominal_) : i18n("The driver did not report VRR.");
  }

  void release() {
    if (notifier_) { notifier_->setEnabled(false); notifier_.reset(); }
    if (fd_>=0) close(fd_);
    fd_=-1; crtc_=0; previous_=0; previousSequence_=0; reported_=0;
  }

  OverlayWindow &window_;
  int fd_=-1,nominal_=0,vrr_=-1;
  uint32_t crtc_=0;
  qint64 previous_=0,reported_=0;
  uint64_t previousSequence_=0;
  bool ownsEffect_=false;
  std::unique_ptr<QSocketNotifier> notifier_;
};
}

int runDisplayOverlay() {
  if (geteuid()==0 || !QGuiApplication::platformName().startsWith("wayland")) return 77;
  auto session=QDBusConnection::sessionBus();
  if (!session.interface() || !session.interface()->isServiceRegistered("org.kde.KWin").value() ||
      !session.registerService(displayOverlayBusName)) return 69;
  QScreen *builtin=nullptr;
  for (auto *screen:QGuiApplication::screens())
    if (screen->name().startsWith("DSI-") || screen->name().startsWith("eDP-")) { builtin=screen; break; }
  OverlayWindow window(builtin);
  DisplayCadence monitor(window);
  if (!session.registerObject(displayOverlayBusPath,&monitor,QDBusConnection::ExportAllSlots)) return 69;
  monitor.start();
  window.show();
  return QCoreApplication::exec();
}
} // namespace uq

#include "display-overlay.moc"
