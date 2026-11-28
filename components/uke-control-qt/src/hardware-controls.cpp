// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include "display-overlay.hpp"
#include "wet-touch-filter.hpp"
#include <KLocalizedString>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QCoreApplication>
#include <QGuiApplication>
#include <memory>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtEndian>
#include <algorithm>
#include <sys/stat.h>

namespace uq {
namespace {
QByteArray displayAttribute(const QString &path, qsizetype limit=64) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.read(limit) : QByteArray();
}

QString fittedColourPanel() {
  const QDir panels("/sys/bus/mipi-dsi/devices");
  QString panel;
  for (const auto &entry : panels.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto path = panels.filePath(entry);
    if (!QFileInfo::exists(path+"/factory_color_profile")) continue;
    if (!panel.isEmpty()) return {};
    panel=path;
  }
  return panel;
}

QString storedProfilePath(const QString &profile) {
  const auto directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  return directory.isEmpty() ? QString() : directory+"/display/"+profile+".icc";
}

QString retainFactoryProfile(const QString &panel,const QString &profile) {
  if (panel.isEmpty()) return {};
  const auto path=storedProfilePath(profile);
  const auto attribute=profile=="factory-p3" ? "/factory_icc_p3_d65" : "/factory_icc";
  const auto data=displayAttribute(panel+attribute,4*1024*1024+1);
  if (path.isEmpty() || data.size()<132 || data.size()>4*1024*1024 ||
      qFromBigEndian<quint32>(data.constData())!=data.size() ||
      data.mid(12,4)!="mntr" || data.mid(36,4)!="acsp")
    return {};
  if (displayAttribute(path,data.size()+1)==data) return path;
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) return {};
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly) ||
      !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
      file.write(data)!=data.size() || !file.commit())
    return {};
  return path;
}

QString internalConnector(const QJsonObject &data) {
  static const QRegularExpression internal("^(?:DSI|eDP)-[0-9]{1,4}$");
  const auto name=data.value("output").toString();
  return internal.match(name).hasMatch() ? name : QString();
}

void startKScreen(QProcess *process,const QStringList &arguments) {
  auto environment=QProcessEnvironment::systemEnvironment();
  environment.insert("QT_QPA_PLATFORM","wayland");
  environment.insert("LC_ALL","C");
  process->setProcessEnvironment(environment);
  process->start("/usr/bin/kscreen-doctor",arguments);
}

QString infraredDevice() {
  QString result;
  for (const auto &name : QDir("/sys/class/lirc").entryList({"lirc*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    static const QRegularExpression node("^lirc[0-9]{1,4}$");
    if (!node.match(name).hasMatch() ||
        QFileInfo("/sys/class/lirc/"+name+"/device/device/driver").canonicalFilePath()!="/sys/bus/spi/drivers/ir-spi")
      continue;
    const QString path="/dev/"+name;
    struct stat device{};
    if (lstat(QFile::encodeName(path).constData(),&device)!=0 || !S_ISCHR(device.st_mode) ||
        device.st_uid!=0 || (device.st_mode&S_IWOTH))
      continue;
    if (!result.isEmpty()) return {};
    result=path;
  }
  return result;
}
}
QVariantMap Settings::hardwarePolicies() const {
  const QString helper="/usr/libexec/uke-control/hardware-policy";
  const auto compatible=displayAttribute("/sys/firmware/devicetree/base/compatible",4096).split('\0');
  const bool available=!fixture_.enabled && QFileInfo(helper).isExecutable() &&
      compatible.contains("xiaomi,uke") && compatible.contains("qcom,sm7675");
  const auto active=displayAttribute("/sys/module/msm/parameters/uke_dsi_dsc_sharing").trimmed();
  const bool dscAvailable=available && (active=="N" || active=="Y");
  const auto savedDsc=displayAttribute("/etc/uke-control/dsi-dsc-sharing").trimmed();
  const bool dscEnabled=savedDsc=="on" || (savedDsc!="off" && active=="Y");
  const bool cpuAvailable=available &&
      displayAttribute("/sys/devices/system/cpu/cpu0/online").trimmed()=="1" &&
      QFileInfo::exists("/sys/devices/system/cpu/cpu3/cpu_capacity") &&
      QFileInfo::exists("/sys/devices/system/cpu/cpu3/online");
  return {{"dscAvailable",dscAvailable},{"dscActive",active=="Y"},{"dscEnabled",dscEnabled},
      {"dscPending",dscAvailable && dscEnabled!=(active=="Y")},
      {"cpuAvailable",cpuAvailable},{"cpuEnabled",displayAttribute("/etc/uke-control/cpu-sleep").trimmed()=="on"},
      {"busy",hardwarePolicyProcess_!=nullptr},{"error",hardwarePolicyError_}};
}

void Settings::setHardwarePolicy(const QString &kind,bool enabled) {
  if (fixture_.enabled || restricted_ || hardwarePolicyProcess_ || !topic_) return;
  const auto policy=hardwarePolicies();
  if ((kind=="dsc" && (topic_->id!="display" || !policy.value("dscAvailable").toBool())) ||
      (kind=="cpu" && (topic_->id!="cpu" || !policy.value("cpuAvailable").toBool())) ||
      (kind!="cpu" && kind!="dsc")) return;
  auto *process=new QProcess(this);
  hardwarePolicyProcess_=process;
  hardwarePolicyError_.clear();
  const auto done=[this,process,kind,enabled](bool ok) {
    if (hardwarePolicyProcess_!=process) return;
    hardwarePolicyProcess_=nullptr;
    process->deleteLater();
    const auto key=kind=="cpu" ? "cpuEnabled" : "dscEnabled";
    if (!ok || hardwarePolicies().value(key).toBool()!=enabled)
      hardwarePolicyError_=i18n("The hardware preference could not be saved.");
    emit changed();
  };
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
          [done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit && code==0);});
  connect(process,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(false);});
  process->setStandardOutputFile(QProcess::nullDevice());
  process->setStandardErrorFile(QProcess::nullDevice());
  process->start("/usr/bin/pkexec",{"/usr/libexec/uke-control/hardware-policy",kind,enabled?"on":"off"});
  emit changed();
}

QVariantMap Settings::notificationLed() const {
  const auto data=data_.value("led.status");
  const bool available=data.value("schema").toString()=="org.uke.control.rgb.v1" &&
      data.value("available").toBool() && !errors_.contains("led.status");
  return {{"available",available},{"testing",ledTesting_},{"settling",ledPending_ || !ledMethod_.isEmpty()},
          {"camera",data.value("camera_integration_available").toBool()},
          {"battery",data.value("battery_valid").toBool()},{"error",ledError_}};
}
void Settings::previewLed(const QColor &color,int percent) {
  if (fixture_.enabled || restricted_ || !topic_ || topic_->id!="led" || !color.isValid() ||
      percent<1 || percent>100 || !notificationLed().value("available").toBool())
    return;
  ledTesting_=true;
  ledMethod_="SetColor";
  ledArguments_={color.name(QColor::HexRgb),static_cast<uint>(percent)};
  sendLed();
}
void Settings::stopLedPreview() {
  if (!ledTesting_)
    return;
  ledTesting_=false;
  ledMethod_="Automatic";
  ledArguments_.clear();
  sendLed();
}
void Settings::sendLed() {
  if (ledPending_ || ledMethod_.isEmpty())
    return;
  auto request=QDBusMessage::createMethodCall("org.senemos.Uke.Rgb","/org/senemos/Uke/Rgb",
                                             "org.senemos.Uke.Rgb1",ledMethod_);
  request.setArguments(ledArguments_);
  request.setInteractiveAuthorizationAllowed(false);
  ledMethod_.clear();
  ledArguments_.clear();
  ledPending_=true;
  auto *watcher=new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(request,3000),this);
  connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher] {
    const QDBusPendingReply<QString> reply=*watcher;
    QJsonObject result;
    if (!reply.isError() && reply.value().size()<=65536)
      result=QJsonDocument::fromJson(reply.value().toUtf8()).object();
    if (result.value("schema").toString()=="org.uke.control.rgb.v1" && result.value("available").toBool()) {
      data_["led.status"]=result;
      ledError_.clear();
    } else {
      ledError_=i18n("The notification light did not confirm the change.");
      if (ledMethod_=="SetColor") ledMethod_.clear();
    }
    ledPending_=false;
    watcher->deleteLater();
    sendLed();
    emit changed();
  });
}
QVariantMap Settings::infrared() const {
  const auto device=fixture_.enabled?QString():infraredDevice();
  return {{"available",!device.isEmpty() && QFileInfo("/usr/bin/ir-ctl").isExecutable()},
          {"busy",irProcess_!=nullptr},{"message",irMessage_},{"error",irFailed_}};
}
void Settings::sendInfrared(const QString &code) {
  static const QRegularExpression nec("^[0-9a-fA-F]{4}$");
  if (fixture_.enabled || restricted_ || irProcess_ || !topic_ || topic_->id!="infrared" ||
      !nec.match(code).hasMatch())
    return;
  const auto device=infraredDevice();
  if (device.isEmpty() || !QFileInfo("/usr/bin/ir-ctl").isExecutable())
    return;
  irMessage_.clear();
  irFailed_=false;
  auto *process=new QProcess(this);
  irProcess_=process;
  const auto done=[this,process](bool ok) {
    if (irProcess_!=process) return;
    irProcess_=nullptr;
    process->deleteLater();
    irFailed_=!ok;
    irMessage_=ok?i18n("Signal sent. Check the target device."):i18n("The transmitter could not send the signal in this session.");
    emit changed();
  };
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
          [done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit && code==0);});
  connect(process,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(false);});
  process->setStandardOutputFile(QProcess::nullDevice());
  process->setStandardErrorFile(QProcess::nullDevice());
  process->start("/usr/bin/ir-ctl",{"--device",device,"--scancode","nec:0x"+code});
  QTimer::singleShot(5000,process,[process]{if(process->state()!=QProcess::NotRunning)process->kill();});
  emit changed();
}
void Settings::watchDisplayOverlay() {
  if (fixture_.enabled) return;
  auto *watcher=new QDBusServiceWatcher(displayOverlayBusName,QDBusConnection::sessionBus(),
      QDBusServiceWatcher::WatchForOwnerChange,this);
  connect(watcher,&QDBusServiceWatcher::serviceOwnerChanged,this,[this] {
    overlayPending_=false;
    emit changed();
  });
}

void Settings::setDisplayOverlay(bool enabled) {
  if (fixture_.enabled || restricted_ || overlayPending_ || !topic_ || topic_->id!="display") return;
  if (!desktopDisplay().value("overlayAvailable").toBool()) return;
  auto bus=QDBusConnection::sessionBus();
  if (enabled==desktopDisplay().value("overlayActive").toBool()) return;
  overlayPending_=true;
  displayMessage_.clear();
  if (enabled) {
    if (!QProcess::startDetached(QCoreApplication::applicationFilePath(),{"--display-overlay"}))
      displayMessage_=i18n("The display overlay could not start.");
  } else {
    bus.asyncCall(QDBusMessage::createMethodCall(displayOverlayBusName,displayOverlayBusPath,
        displayOverlayBusInterface,"Quit"),1500);
  }
  QTimer::singleShot(3000,this,[this,enabled] {
    if (!overlayPending_) return;
    overlayPending_=false;
    if (desktopDisplay().value("overlayActive").toBool()!=enabled)
      displayMessage_=i18n("KWin could not change the display overlay in this session.");
    emit changed();
  });
  emit changed();
}

QVariantMap Settings::displayResources() const {
  const QString id="kernel.display-resources";
  const auto data=data_.value(id);
  return {{"read",data_.contains(id)},{"available",data.value("available").toBool() && !errors_.contains(id)},
          {"resources",data.value("resources").toArray().toVariantList()}};
}

void Settings::readDisplayResources() {
  if (fixture_.enabled || restricted_ || busy() || !topic_ || topic_->id!="display") return;
  queue_<<QStringLiteral("kernel.display-resources");
  next();
}

QVariantMap Settings::desktopDisplay() const {
  const auto data=data_.value("desktop.plasma");
  auto *bus=QDBusConnection::sessionBus().interface();
  const bool wayland=!fixture_.enabled && QGuiApplication::platformName().startsWith("wayland");
  const bool overlayAvailable=wayland && bus && bus->isServiceRegistered("org.kde.KWin").value();
  const bool overlayActive=overlayAvailable && bus->isServiceRegistered(displayOverlayBusName).value();
  const auto parameter=[](const QString &path) { return displayAttribute(path).trimmed(); };
  const auto ambient=wayland ? parameter("/sys/module/panel_xiaomi_o82/parameters/ambient_white_balance") : QByteArray();
  const auto wetTouch=fixture_.enabled ? QByteArray() : wetTouchFilterValue(wetTouchFilterPath());
  const auto panel=ambient=="Y" ? fittedColourPanel() : QString();
  QVariantList profiles;
  if (!panel.isEmpty() && !internalConnector(data).isEmpty()) {
    profiles<<QVariantMap{{"id","edid"},{"title",i18n("Built-in display profile (EDID)")}};
    if (parameter(panel+"/factory_color_profile")=="O82 Factory sRGB D65")
      profiles<<QVariantMap{{"id","factory-srgb"},{"title",i18n("Factory sRGB · D65")}};
    if (parameter(panel+"/factory_color_profile_p3_d65")=="O82 Factory Display P3 D65")
      profiles<<QVariantMap{{"id","factory-p3"},{"title",i18n("Factory Display P3 · D65")}};
  }
  return {{"brightness",data.value("automatic_brightness").toVariant()},
          {"rotation",data.value("rotation").toVariant()},
          {"manualBrightness",brightnessMaximum_>0 && brightness_>=0 ? qRound(brightness_*100./brightnessMaximum_) : -1},
          {"output",data.value("output").toVariant()},
          {"available",data.value("schema").toString()=="org.uke.control.plasma.v1" &&
                       data.value("output").isString() && !errors_.contains("desktop.plasma")},
          {"overlayAvailable",overlayAvailable},{"overlayActive",overlayActive},{"overlayPending",overlayPending_},
          {"ambientProfiles",profiles},{"ambientProfile",displayProfile_},{"ambientReading",displayProfileProbe_!=nullptr},
          {"wetTouchAvailable",!wetTouch.isEmpty()},{"wetTouchEnabled",wetTouch=="1"},
          {"liveVrr",!fixture_.enabled && parameter("/sys/module/msm/parameters/o82_vrr_floor_api")=="1"},
          {"busy",displayProcess_!=nullptr},{"message",displayMessage_},{"error",displayFailed_}};
}
void Settings::readDisplayProfile(const QString &expected) {
  if (fixture_.enabled || displayProfileProbe_ || desktopDisplay().value("ambientProfiles").toList().isEmpty()) return;
  const auto connector=internalConnector(data_.value("desktop.plasma"));
  auto *process=new QProcess(this);
  displayProfileProbe_=process;
  const auto done=[this,process,connector,expected](bool ok) {
    if (displayProfileProbe_!=process) return;
    displayProfileProbe_=nullptr;
    auto output=QString::fromUtf8(process->readAllStandardOutput());
    process->deleteLater();
    output.remove(QRegularExpression(QString(QChar(0x1b))+R"(\[[0-9;]*m)"));
    QString source,icc;
    bool internal=false;
    static const QRegularExpression header(R"(^Output:\s+\d+\s+(\S+))");
    if (ok) for (const auto &line : output.split('\n')) {
      const auto match=header.match(line);
      if (match.hasMatch()) internal=match.captured(1)==connector;
      if (!internal) continue;
      const auto field=line.trimmed();
      if (field.startsWith("Color profile source:")) source=field.sliced(21).trimmed();
      if (field.startsWith("ICC profile:")) icc=field.sliced(12).trimmed();
    }
    displayProfile_.clear();
    if (source=="EDID") displayProfile_="edid";
    if (source=="ICC") {
      for (const auto &profile : {QStringLiteral("factory-srgb"),QStringLiteral("factory-p3")})
        if (icc==storedProfilePath(profile)) displayProfile_=profile;
    }
    if (!expected.isEmpty()) {
      displayFailed_=displayProfile_!=expected;
      displayMessage_=displayFailed_ ? i18n("Plasma did not confirm the selected display profile.")
                                     : i18n("Display profile applied.");
    }
    emit changed();
  };
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
          [done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit && code==0);});
  connect(process,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(false);});
  process->setStandardErrorFile(QProcess::nullDevice());
  startKScreen(process,{"--outputs"});
  QTimer::singleShot(5000,process,[process]{if(process->state()!=QProcess::NotRunning)process->kill();});
  emit changed();
}

void Settings::setDisplayPreference(const QString &id,const QString &value) {
  const bool wetTouch = id == "touch.wet-filter";
  if (fixture_.enabled || restricted_ || displayProcess_ || !topic_ || topic_->id!="display" ||
      (!wetTouch && id!="display.ambient-color" && !desktopDisplay().value("available").toBool()))
    return;
  QStringList args;
  QString program="/usr/bin/uke-control";
  const bool colour=id=="display.ambient-color";
  if (wetTouch) {
    if ((value!="0" && value!="1") || !desktopDisplay().value("wetTouchAvailable").toBool()) return;
    program="/usr/bin/pkexec";
    args << QStringLiteral("/usr/libexec/uke-control-qt/wet-touch-filter") << value;
  } else if (colour) {
    if (displayProfileProbe_) return;
    const auto profiles=desktopDisplay().value("ambientProfiles").toList();
    if (std::none_of(profiles.begin(),profiles.end(),[&](const auto &profile){
          return profile.toMap().value("id").toString()==value;
        })) return;
    const auto output="output."+internalConnector(data_.value("desktop.plasma"))+'.';
    program="/usr/bin/kscreen-doctor";
    if (value=="edid") {
      args<<output+"colorProfileSource.EDID";
    } else {
      const auto path=retainFactoryProfile(fittedColourPanel(),value);
      if (path.isEmpty()) {
        displayFailed_=true;
        displayMessage_=i18n("The fitted panel’s factory profile could not be opened.");
        emit changed();
        return;
      }
      args<<output+"iccprofile."+path<<output+"colorProfileSource.ICC";
    }
  } else try {
    if (id!="desktop.brightness" && id!="desktop.rotation") return;
    const auto &descriptor=action(id.toStdString());
    if (descriptor.access!=Access::user || descriptor.effect!=Effect::device || descriptor.parameters.size()!=1)
      return;
    const auto request=plan(descriptor.id,{{descriptor.parameters.front().id,value.toStdString()}});
    if (request.argv.empty() || request.argv.front()!="/usr/bin/uke-control") return;
    for (auto it=request.argv.begin()+1;it!=request.argv.end();++it)
      args << QString::fromStdString(*it);
  } catch (const std::exception &) {
    return;
  }
  displayMessage_.clear();
  displayFailed_=false;
  auto *process=new QProcess(this);
  displayProcess_=process;
  const auto done=[this,process,colour,value,wetTouch](bool ok) {
    if (displayProcess_!=process) return;
    displayProcess_=nullptr;
    process->deleteLater();
    displayFailed_=!ok;
    displayMessage_=ok ? QString() : wetTouch ? i18n("The wet touch filter could not be changed.")
                                             : i18n("The desktop could not apply this preference.");
    if (colour && ok) readDisplayProfile(value);
    else if (!colour && topic_ && topic_->id=="display") refresh();
    else emit changed();
  };
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
          [done](int code,QProcess::ExitStatus status){done(status==QProcess::NormalExit && code==0);});
  connect(process,&QProcess::errorOccurred,this,[done](QProcess::ProcessError error){if(error==QProcess::FailedToStart)done(false);});
  process->setStandardOutputFile(QProcess::nullDevice());
  process->setStandardErrorFile(QProcess::nullDevice());
  if (colour) startKScreen(process,args);
  else process->start(program,args);
  QTimer::singleShot(wetTouch ? 90000 : 15000, process,
      [process]{if(process->state()!=QProcess::NotRunning)process->kill();});
  emit changed();
}
void Settings::readScreenBrightness() {
  if (fixture_.enabled) return;
  const auto generation=brightnessGeneration_;
  const QString service="org.kde.ScreenBrightness", path="/org/kde/ScreenBrightness";
  auto request=QDBusMessage::createMethodCall(service,path,"org.freedesktop.DBus.Properties","Get");
  request.setArguments({service,QStringLiteral("DisplaysDBusNames")});
  auto bus=QDBusConnection::sessionBus();
  auto *watcher=new QDBusPendingCallWatcher(bus.asyncCall(request,1500),this);
  connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher,generation,service,path,bus] {
    const QDBusPendingReply<QDBusVariant> reply=*watcher;
    watcher->deleteLater();
    if (reply.isError() || generation!=brightnessGeneration_) return;
    auto names=reply.value().variant().toStringList();
    static const QRegularExpression display("^display[0-9]{1,8}$");
    names.removeIf([](const QString &name){return !display.match(name).hasMatch();});
    if (names.isEmpty() || names.size()>16) return;
    auto remaining=std::make_shared<qsizetype>(names.size());
    auto internal=std::make_shared<QList<QPair<QString,QVariantMap>>>();
    for (const auto &name : names) {
      auto properties=QDBusMessage::createMethodCall(service,path+"/"+name,"org.freedesktop.DBus.Properties","GetAll");
      properties.setArguments({QStringLiteral("org.kde.ScreenBrightness.Display")});
      auto *worker=new QDBusPendingCallWatcher(bus.asyncCall(properties,1500),this);
      connect(worker,&QDBusPendingCallWatcher::finished,this,[this,worker,generation,name,remaining,internal] {
        const QDBusPendingReply<QVariantMap> answer=*worker;
        worker->deleteLater();
        if (generation!=brightnessGeneration_) return;
        if (!answer.isError() && answer.value().value("IsInternal").toBool())
          internal->push_back({name,answer.value()});
        if (--*remaining==0 && internal->size()==1) {
          const auto data=internal->first().second;
          screenBrightnessId_=internal->first().first;
          screenBrightnessRangeChanged(screenBrightnessId_,data.value("Brightness").toInt(),data.value("MaxBrightness").toInt());
        }
      });
    }
  });
}
void Settings::screenBrightnessChanged(const QString &id,int value,const QString &,const QString &) {
  if (id==screenBrightnessId_ && value>=0 && value<=brightnessMaximum_) {brightness_=value;emit changed();}
}
void Settings::screenBrightnessRangeChanged(const QString &id,int value,int maximum) {
  if (id==screenBrightnessId_ && maximum>0 && maximum<=1000000 && value>=0 && value<=maximum) {
    brightnessMaximum_=maximum;
    brightness_=value;
    emit changed();
  }
}
void Settings::setScreenBrightness(int percent) {
  if (fixture_.enabled || restricted_ || !topic_ || topic_->id!="display" || screenBrightnessId_.isEmpty() ||
      brightnessMaximum_<=0 || percent<1 || percent>100)
    return;
  auto request=QDBusMessage::createMethodCall("org.kde.ScreenBrightness","/org/kde/ScreenBrightness/"+screenBrightnessId_,
                                             "org.kde.ScreenBrightness.Display","SetBrightness");
  request.setArguments({qMax(1,qRound(percent*brightnessMaximum_/100.)),uint(0)});
  auto *watcher=new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(request,1500),this);
  connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher] {
    if (watcher->isError()) {
      displayFailed_=true;
      displayMessage_=i18n("The desktop could not apply this preference.");
      emit changed();
    }
    watcher->deleteLater();
  });
}
}
