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
QVariantMap componentCard(const QString &key,const QString &model,const QString &role,QVariantList rows={}) {
  if (model.isEmpty()) return card(key,role,{fact(i18n("Model"),{})});
  rows.prepend(fact(i18n("Function"),role));
  return card(key,model,rows);
}
QVariantMap group(const QString &title,const QVariantList &cards) {
  return {{"title",title},{"cards",cards}};
}
}
QVariantMap platformBoardInformation() {
  const QString dt="/sys/firmware/devicetree/base";
  const auto compatible=read(dt+"/compatible",512).split('\0');
  const auto identifier=[&](const char *prefix) {
    for (const auto &part : compatible)
      if (part.startsWith(prefix)) return safe(part.mid(qstrlen(prefix)).toStdString());
    return QString();
  };
  QVariantMap identity{{"model",name(dt+"/model")},{"codename",identifier("xiaomi,")},
                       {"soc",identifier("qcom,").toUpper()}};
  const auto memory=read(dt+"/memory/reg",1024);
  const auto addressCells=read(dt+"/#address-cells",4),sizeCells=read(dt+"/#size-cells",4);
  quint64 total=0;
  if (addressCells.size()==4 && sizeCells.size()==4 && qFromBigEndian<quint32>(addressCells.constData())==2 &&
      qFromBigEndian<quint32>(sizeCells.constData())==2 && memory.size()%16==0) {
    for (qsizetype position=8;position<memory.size();position+=16)
      total+=qFromBigEndian<quint64>(memory.constData()+position);
  }
  if (total>0 && total<(1ULL<<43))
    identity["memory"]=QLocale().formattedDataSize(qint64(total));
  return identity;
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
  QVariantList cards;
  cards<<componentCard("display-panel",panel,i18n("Display panel"),{fact(i18n("Manufacturer"),manufacturer)})
       <<componentCard("touch-controller",touch,i18n("Touch controller"))
       <<componentCard("ufs-storage",ufs,i18n("Internal UFS storage"),
          {fact(i18n("Capacity"),sectors>0?QLocale().formattedDataSize(qint64(sectors*512)):QString())});

  const auto wirelessCard=[&](const QString &base,const QStringList &pattern,const QString &key,const QString &role) {
    const QDir devices(base);
    for (const auto &entry : devices.entryList(pattern,QDir::Dirs | QDir::NoDotAndDotDot)) {
      if (key=="wifi" && !QFileInfo::exists(devices.filePath(entry+"/phy80211"))) continue;
      const auto file=devices.filePath(entry+"/device/of_node/compatible");
      // Several boards export both a board-specific and a chipset-compatible identifier.
      for (const auto &id : read(file,256).split('\0')) {
        if (!id.startsWith("qcom,")) continue;
        cards<<componentCard(key,safe(id.toStdString()),role);
        return;
      }
      const auto compatible=name(file);
      if (!compatible.isEmpty()) {
        cards<<componentCard(key,compatible,role);
        return;
      }
    }
  };
  wirelessCard("/sys/class/net",{"*"},"wifi",i18n("Wi-Fi adapter"));
  wirelessCard("/sys/class/bluetooth",{"hci*"},"bluetooth",i18n("Bluetooth adapter"));
  return cards;
}
QVariantMap platformSensorInformation() {
  QMap<QString,QStringList> sensors;
  auto add=[&](const QString &model,const QString &function) {
    if (model.isEmpty() || (!sensors.contains(model) && sensors.size()>=256)) return;
    auto &functions=sensors[model];
    if (!functions.contains(function)) functions<<function;
  };
  const QList<QPair<QString,QString>> types{
      {"in_accel_",i18n("Acceleration")},{"in_anglvel_",i18n("Angular velocity")},
      {"in_magn_",i18n("Magnetic field")},{"in_proximity",i18n("Proximity")},
      {"in_illuminance",i18n("Ambient light")},{"in_pressure",i18n("Pressure")},
      {"in_temp",i18n("Temperature")},{"in_gravity_",i18n("Gravity")},
      {"in_rot_",i18n("Orientation")},{"in_voltage",i18n("Voltage")},
      {"in_current",i18n("Current")}};
  const QMap<QString,QString> namedFunctions{
      {"uke-ssc-front-color",i18n("Ambient colour")},
      {"uke-ssc-flicker",i18n("Light flicker")},
      {"uke-ssc-magnetic-heading",i18n("Magnetic heading")}};
  const QDir iio("/sys/bus/iio/devices");
  for (const auto &entry : iio.entryList({"iio:device*"},QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QDir device(iio.filePath(entry));
    const auto model=name(device.filePath("name"));
    const auto attributes=device.entryList({"in_*"},QDir::Files);
    const auto named=namedFunctions.value(model);
    if (!named.isEmpty()) {
      add(model,named);
      continue;
    }
    bool typed=false;
    for (const auto &[prefix,function] : types) {
      if (std::any_of(attributes.cbegin(),attributes.cend(),[&](const auto &attribute){return attribute.startsWith(prefix);})) {
        add(model,function);
        typed=true;
      }
    }
    if (!typed && model.contains("thermal-sensor")) add(model,i18n("Temperature"));
    else if (!typed) add(model,i18n("Sensor reading"));
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
  QVariantList other, temperatures;
  for (const auto &[model,functions] : sensors.asKeyValueRange()) {
    auto &cards=functions==QStringList{i18n("Temperature")} ? temperatures : other;
    cards<<componentCard("sensor-"+model,model,QLocale().createSeparatedList(functions));
  }
  return {{"sensors",other},{"temperatures",temperatures}};
}
QVariantList Settings::deviceInformation() const {
  const auto board=deviceBoard_.value(0).toMap();
  const auto data=data_.value("hardware.status");
  const auto identity=data.value("identity").toObject();
  const auto identified=[&](const QString &key) {return safe(identity.value(key).toString().left(128).toStdString());};
  const auto model=board.value("model",identified("board")).toString();
  const auto soc=board.value("soc").toString();
  const auto processor=identified("processor");
  QVariantList parts=deviceBoard_.value(1).toList();
  parts.prepend(componentCard("memory",identified("ram_type"),i18n("RAM"),
      {fact(i18n("Memory in the Device Tree"),board.value("memory").toString())}));
  parts.prepend(componentCard("soc",processor.isEmpty()?soc:processor,i18n("Processor"),
      soc.isEmpty()?QVariantList{}:QVariantList{fact(i18n("Chipset"),soc)}));
  const auto appendDevices=[&](const QString &category,const QString &field,const QString &role) {
    QSet<QString> seen;
    for (const auto &item : data.value(category).toObject().value("devices").toArray()) {
      const auto label=safe(item.toObject().value(field).toString().left(128).toStdString());
      if (label.isEmpty() || seen.contains(label) || seen.size()>=32) continue;
      seen.insert(label);
      parts<<componentCard(category+"-"+label,label,role);
    }
    return !seen.isEmpty();
  };
  appendDevices("soundwire","driver",i18n("Audio component"));
  appendDevices("video","driver",i18n("Video codec"));
  if (!appendDevices("media","name",i18n("Camera device")))
    parts<<card("camera",i18n("Camera"),
        {fact(i18n("Camera sensor model"),{})});

  parts.prepend(card("mainboard",model.isEmpty()?i18n("Device"):model,
      {fact(i18n("Device code name"),board.value("codename").toString())}));
  QVariantList groups{group(i18n("Hardware"),parts)};
  const auto inventory=deviceBoard_.value(2).toMap();
  const auto sensors=inventory.value("sensors").toList();
  const auto temperatures=inventory.value("temperatures").toList();
  if (!sensors.isEmpty())
    groups<<group(i18n("Sensors"),sensors);
  if (!temperatures.isEmpty())
    groups<<group(i18n("Temperature sensors"),temperatures);
  QVariantList accessories;
  const auto pen=identified("pen_model");
  const auto keyboard=identified("keyboard_model");
  if (!pen.isEmpty()) accessories<<componentCard("pen",pen,i18n("Compatible pen"));
  if (!keyboard.isEmpty()) accessories<<componentCard("keyboard",keyboard,i18n("Compatible keyboard"));
  if (!accessories.isEmpty()) groups<<group(i18n("Compatible accessories"),accessories);
  return groups;
}
}
