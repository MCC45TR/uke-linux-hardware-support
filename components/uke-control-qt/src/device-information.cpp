// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QtEndian>
#include <algorithm>

namespace uq {
namespace {
QByteArray read(const QString &path,qsizetype size=1024) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.read(size);
}
QString name(const QString &path) {
  return safe(read(path,256).split('\0').value(0).trimmed().toStdString());
}
QVariantMap fact(const QString &label,const QString &value) {
  return {{"title",label},{"value",value.isEmpty()?i18n("Not reported"):value}};
}
QVariantMap card(const QString &key,const QString &title,const QVariantList &rows) {
  return {{"key",key},{"title",title},{"rows",rows}};
}
}
QVariantList platformBoardInformation() {
  const QString dt="/sys/firmware/devicetree/base";
  QVariantList rows;
  rows<<fact(i18n("Model"),name(dt+"/model"))
      <<fact(i18n("Device code name"),name(dt+"/compatible").section(',',-1))
      <<fact(i18n("Device Tree identifiers"),safe(read(dt+"/compatible",512).trimmed().replace('\0',' ').simplified().toStdString()));
  const auto memory=read(dt+"/memory/reg",1024);
  const auto addressCells=read(dt+"/#address-cells",4),sizeCells=read(dt+"/#size-cells",4);
  quint64 total=0;
  if (addressCells.size()==4 && sizeCells.size()==4 && qFromBigEndian<quint32>(addressCells.constData())==2 &&
      qFromBigEndian<quint32>(sizeCells.constData())==2 && memory.size()%16==0) {
    for (qsizetype position=8;position<memory.size();position+=16)
      total+=qFromBigEndian<quint64>(memory.constData()+position);
  }
  if (total>0 && total<(1ULL<<43))
    rows<<fact(i18n("Memory in the Device Tree"),QLocale().formattedDataSize(qint64(total)));
  return rows;
}
QVariantList platformComponentInformation() {
  const QString dt="/sys/firmware/devicetree/base";
  QString panel,manufacturer,touch,ufs;
  quint64 sectors=0;
  for (const auto &namePart : QDir(dt+"/soc").entryList({"display-subsystem*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QString subsystem=dt+"/soc/"+namePart;
    for (const auto &dsi : QDir(subsystem).entryList({"dsi@*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
      const QString dsiPath=subsystem+"/"+dsi;
      for (const auto &node : QDir(dsiPath).entryList({"panel@*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString path=dsiPath+"/"+node;
        const auto compatible=name(path+"/compatible");
        if (name(path+"/status")=="disabled" || compatible.isEmpty()) continue;
        if (!panel.isEmpty() && panel!=compatible) {panel.clear();manufacturer.clear();break;}
        panel=compatible;
        manufacturer=name(path+"/manufacturer");
      }
      if (!panel.isEmpty()) break;
    }
    if (!panel.isEmpty()) break;
  }
  for (const auto &device : QDir("/sys/bus/spi/devices").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto compatible=name("/sys/bus/spi/devices/"+device+"/of_node/compatible");
    if (compatible.contains("touch",Qt::CaseInsensitive) && !compatible.isEmpty()) {touch=compatible;break;}
  }
  for (const auto &disk : QDir("/sys/class/block").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QString path="/sys/class/block/"+disk;
    if (QFileInfo::exists(path+"/partition") ||
        !QFileInfo(path+"/device").canonicalFilePath().contains(".ufshc/"))
      continue;
    if (ufs.isEmpty()) {
      ufs=(name(path+"/device/vendor")+" "+name(path+"/device/model")).trimmed();
    }
    const auto count=read(path+"/size",64).trimmed().toULongLong();
    if (count<(1ULL<<36)) sectors+=count;
  }
  return {fact(i18n("Panel identifier"),panel),fact(i18n("Panel manufacturer"),manufacturer),
          fact(i18n("Touch controller"),touch),fact(i18n("UFS model"),ufs),
          fact(i18n("Internal storage capacity"),sectors>0?QLocale().formattedDataSize(qint64(sectors*512)):QString())};
}
QVariantList platformSensorInformation() {
  QVariantList rows;
  QSet<QPair<QString,QString>> seen;
  auto add=[&](const QString &model,const QString &function) {
    const auto key=qMakePair(model,function);
    if (model.isEmpty() || seen.contains(key) || rows.size()>=256) return;
    seen.insert(key);
    rows<<fact(model,function);
  };
  const QList<QPair<QString,QString>> types{
      {"in_accel_",i18n("Acceleration")},{"in_anglvel_",i18n("Angular velocity")},
      {"in_magn_",i18n("Magnetic field")},{"in_proximity",i18n("Proximity")},
      {"in_illuminance",i18n("Ambient light")},{"in_pressure",i18n("Pressure")},
      {"in_temp",i18n("Temperature")},{"in_gravity_",i18n("Gravity")},
      {"in_rot_",i18n("Orientation")}};
  const QDir iio("/sys/bus/iio/devices");
  for (const auto &entry : iio.entryList({"iio:device*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QDir device(iio.filePath(entry));
    const auto model=name(device.filePath("name"));
    const auto attributes=device.entryList({"in_*"},QDir::Files);
    bool typed=false;
    for (const auto &[prefix,function] : types) {
      if (std::any_of(attributes.cbegin(),attributes.cend(),[&](const auto &attribute){return attribute.startsWith(prefix);})) {
        add(model,function);
        typed=true;
      }
    }
    if (!typed) add(model,i18n("Sensor reading"));
  }
  const QDir hwmon("/sys/class/hwmon");
  for (const auto &entry : hwmon.entryList({"hwmon*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QDir device(hwmon.filePath(entry));
    const auto model=name(device.filePath("name"));
    if (!device.entryList({"temp*_input"},QDir::Files).isEmpty()) add(model,i18n("Temperature"));
    if (!device.entryList({"in*_input"},QDir::Files).isEmpty()) add(model,i18n("Voltage"));
    if (!device.entryList({"curr*_input"},QDir::Files).isEmpty()) add(model,i18n("Current"));
  }
  const QDir thermal("/sys/class/thermal");
  for (const auto &entry : thermal.entryList({"thermal_zone*"},QDir::Dirs | QDir::NoDotAndDotDot))
    add(name(thermal.filePath(entry+"/type")),i18n("Temperature"));
  return rows;
}
QVariantList Settings::deviceInformation() const {
  QVariantList board=deviceBoard_.value(0).toList();
  const auto data=data_.value("hardware.status");
  const auto identity=data.value("identity").toObject();
  for (const auto &[key,label] : QMap<QString,QString>{{"processor",i18n("Processor")},{"ram_type",i18n("Memory type")},
      {"pen_model",i18n("Compatible pen")},{"keyboard_model",i18n("Compatible keyboard")}}.asKeyValueRange()) {
    const auto item=identity.value(key).toString();
    if (!item.isEmpty()) board<<fact(label,safe(item.left(128).toStdString()));
  }

  QVariantList cameras;
  auto sensors=deviceBoard_.value(2).toList();
  QSet<QString> cameraNames;
  for (const auto &item : data.value("media").toObject().value("devices").toArray()) {
    const auto device=item.toObject();
    const auto label=device.value("name").toString();
    if (!label.isEmpty() && !cameraNames.contains(label)) {
      cameraNames.insert(label);
      cameras<<fact(i18n("Camera device"),safe(label.left(128).toStdString()));
    }
  }
  if (cameras.isEmpty()) cameras<<fact(i18n("Camera sensor model"),QString());
  if (sensors.isEmpty()) sensors<<fact(i18n("Sensors"),QString());
  return {card("mainboard",i18n("Device and specifications"),board),
          card("components",i18n("Display and storage hardware"),deviceBoard_.value(1).toList()),
          card("camera",i18n("Cameras"),cameras),
          card("sensors",i18n("Sensor inventory"),sensors)};
}
}
