// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QLibraryInfo>
#include <QProcess>
#include <QSet>
#include <algorithm>
#include <cmath>
namespace uq {
QVariantList platformBoardInformation();
QVariantList platformComponentInformation();
QVariantList platformSensorInformation();
namespace {
QString unknown() { return i18n("Not reported"); }
QString value(const QJsonValue &v) {
  if(v.isNull() || v.isUndefined()) return unknown();
  if(v.isBool()) return v.toBool()?i18n("Yes"):i18n("No");
  if(v.isDouble()) return QLocale().toString(v.toDouble(),'g',8);
  if(v.isString()) {
    const QMap<QString,QString> states{{"Full",i18n("Full")},{"Charging",i18n("Charging")},{"Discharging",i18n("Discharging")},{"Unknown",unknown()},{"auto",i18n("Automatic")},{"on",i18n("On")},{"off",i18n("Off")}};
    return states.value(v.toString(),safe(v.toString().left(500).toStdString()));
  }
  return unknown();
}
QVariantMap row(const QString &name,const QString &text,double progress=-1,bool primary=false) {
  return {{"title",name},{"value",text},{"progress",progress},{"primary",primary||progress>=0}};
}
QString bytes(double n) { return QLocale().formattedDataSize(qint64(n)); }
QJsonObject component(const QJsonObject &data) {
  const auto components=data.value("components").toArray();
  return components.isEmpty()?data:components.first().toObject().value("data").toObject();
}
}
QVariantList informationRows(const QString &id,const QJsonObject &data) {
  QVariantList rows;
  const auto d=component(data);
  if(id.startsWith("battery.") && id!="battery.protocol") {
    const auto readings=data.value("readings").toObject();
    const bool coherent=data.value("provider_verified").toBool() && data.value("snapshot_coherent").toBool();
    if(id=="battery.cells") {
      if(!coherent)
        return rows;
      const auto topology=data.value("reported_cell_topology").toObject();
      rows<<row(i18n("Readings per cell"),i18n("Individual cell voltage and health are not reported"));
      if(!topology.value("qualified").toBool())
        return rows;
      const auto series=topology.value("series_cells").toInt();
      const auto parallel=topology.value("parallel_cells").toInt();
      if(series>0 && parallel>0)
        rows<<row(i18n("Verified cell configuration"),i18n("%1 series × %2 parallel",series,parallel));
      const auto average=topology.value("average_series_cell_voltage_uv").toDouble();
      if(average>0 && std::isfinite(average))
        rows<<row(i18n("Calculated average per series cell"),i18n("%1 V",QLocale().toString(average/1000000,'f',3)));
      return rows;
    }
    auto measure=[&](const char *key,const QString &title,const char *unit,double scale,const QString &suffix,bool level=false) {
      const auto r=readings.value(key).toObject();const auto v=r.value("value");
      const auto validity=r.value("validity").toString();
      const auto n=v.toDouble()/scale;
      const bool valid=coherent && validity!="stale" && validity!="invalid" && validity!="unavailable" && r.value("errno").toInt(-1)==0 && r.value("unit").toString()==unit && v.isDouble() && std::isfinite(n) && (!level||(n>=0&&n<=100));
      rows << row(title,valid?QLocale().toString(n,'f',scale==1?0:1)+suffix:unknown(),valid&&level?n/100:-1);
    };
    if(id=="battery.status") {
      measure("battery.capacity",i18n("Battery level"),"percent",1," %",true);
      rows << row(i18n("Reported battery state"),coherent?value(readings.value("battery.status").toObject().value("value")):unknown());
      measure("battery.temp",i18n("Temperature"),"deciC",10," °C");
      measure("battery.current_now",i18n("Battery current"),"uA_positive_into_battery",1000," mA");
      measure("battery.voltage_now",i18n("Battery voltage"),"uV",1000000," V");
      rows << row(i18n("Measurement basis"),i18n("Reported by the battery provider; sample age may be unknown"));
      for(int i=3;i<rows.size();++i){auto r=rows[i].toMap();r["detail"]=true;rows[i]=r;}
    } else {
      if(id=="battery.health") {
        rows << row(i18n("Reported hardware cycles"),value(data.value("battery.cycle_count").toObject().value("value")));
        const auto h=data.value("health").toObject();
        rows << row(i18n("Qualified capacity retention"),h.value("learned_capacity_qualified").toBool()&&h.value("capacity_retention_percent").isDouble()?value(h.value("capacity_retention_percent"))+" %":unknown());
        for(const auto &key:{"battery.charge_full","battery.charge_full_design"}) {
          const auto capacity=data.value(key).toObject();
          const bool valid=coherent&&capacity.value("value").isDouble()&&capacity.value("unit").toString()=="uAh"&&capacity.value("errno").toInt(-1)==0;
          rows<<row(QString(key).endsWith("design")?i18n("Reported design capacity"):i18n("Reported full capacity"),valid?QLocale().toString(capacity.value("value").toDouble()/1000,'f',0)+" mAh":unknown());
        }
      }
      rows << row(i18n("Individual cell health"),i18n("Not measured by this provider"));
    }
    return rows;
  }
  if(id=="flashlight.status") {
    const auto f=data.value("flashlight").toObject();
    rows<<row(i18n("Current state"),f.value("on").isBool()?(f.value("on").toBool()?i18n("On"):i18n("Off")):unknown(),-1,f.value("on").isBool());
    rows<<row(i18n("Applied brightness"),f.value("brightness_percent").isDouble()?value(f.value("brightness_percent"))+" %":unknown());
    rows<<row(i18n("Available"),value(f.value("available")));
    return rows;
  }
  if(id=="battery.protocol") {
    rows<<row(i18n("Reported USB type"),value(data.value("reported_usb_type")));
    rows<<row(i18n("Negotiated protocol"),value(data.value("negotiated_protocol")));
    rows<<row(i18n("Negotiated PD revision"),value(data.value("negotiated_pd_revision")));
    return rows;
  }
  if(id=="sensors.list") {
    for(const auto &entry:data.value("readings").toArray()) {
      if(rows.size()>=24)break;
      const auto r=entry.toObject();const auto label=safe(r.value("label").toString().left(120).toStdString());
      if(!label.isEmpty())rows<<row(label,r.value("value").isNull()||r.value("value").isUndefined()?value(r.value("status")):value(r.value("value"))+" "+safe(r.value("unit").toString().left(32).toStdString()));
    }
    return rows;
  }
  if(id=="overview.storage") {
    const auto capacity=d.value("root_capacity_bytes"), available=d.value("root_available_bytes");
    if(capacity.isDouble()&&available.isDouble()&&capacity.toDouble()>0&&available.toDouble()>=0&&available.toDouble()<=capacity.toDouble()) {
      const auto used=capacity.toDouble()-available.toDouble();
      rows << row(i18n("Used storage"),bytes(used)+" / "+bytes(capacity.toDouble()),used/capacity.toDouble());
      rows << row(i18n("Available storage"),bytes(available.toDouble()));
    }
    return rows;
  }
  if(id=="overview.workload") {
    const auto mem=d.value("meminfo").toString().left(16000);
    auto read=[&](const QString &key)->double {
      const auto m=QRegularExpression("(?:^|\\n)"+key+":\\s+([0-9]+) kB(?:\\n|$)").match(mem);
      return m.hasMatch()?m.captured(1).toDouble()*1024:-1;
    };
    const auto total=read("MemTotal"),available=read("MemAvailable");
    if(total>0&&available>=0&&available<=total) {
      rows << row(i18n("Memory in use"),bytes(total-available)+" / "+bytes(total),(total-available)/total);
      rows << row(i18n("Available memory"),bytes(available));
      rows << row(i18n("Memory accounting"),i18n("Available memory includes reclaimable cache"));
    }
    return rows;
  }
  // Deliberately selected human facts. Paths, UUIDs, raw transport data and
  // arbitrary object keys remain in the expert observation view.
  const QMap<QString,QString> labels{
    {"automatic_brightness",i18n("Automatic brightness")},{"rotation",i18n("Automatic rotation")},
    {"rotation_policy",i18n("Rotation policy")},{"output",i18n("Display output")},
    {"selected",i18n("Selected channel")},{"pending",i18n("Pending operation")},
    {"channel",i18n("Channel")},{"running_kernel",i18n("Running kernel")},
    {"reported_feature_enabled",i18n("Reported charging feature")},
    {"reported_start_percent",i18n("Reported start threshold")},{"reported_end_percent",i18n("Reported end threshold")},
    {"configuration_readback_verified",i18n("Settings readback verified")},
    {"physical_charge_stopped",i18n("Physical charge stop verified")},
    {"profile_qualified",i18n("Charging profile qualified")},{"control_profile_qualified",i18n("Charging profile qualified")},
    {"threshold_admission_ready",i18n("Threshold requests admitted")},{"automatic_admission_ready",i18n("Automatic requests admitted")},
    {"ready",i18n("Provider ready")},{"available",i18n("Available")},{"supported",i18n("Supported")},
    {"enabled",i18n("Enabled")},{"brightness",i18n("Brightness")},{"brightness_percent",i18n("Reported level")},{"percent",i18n("Reported level")},
    {"color",i18n("Colour")},{"policy",i18n("Policy")},{"mode",i18n("Mode")},
    {"product",i18n("Accessory")},{"manufacturer",i18n("Manufacturer")},{"name",i18n("Name")},
    {"role",i18n("Data role")},{"interface",i18n("Network interface")},{"selection",i18n("Microphone preference")},
    {"description",i18n("Description")},{"status",i18n("Status")},{"reason",i18n("Provider explanation")},
    {"radio",i18n("Radio state")},{"powered",i18n("Radio state")},{"state",i18n("Reported state")},{"usb_type",i18n("Reported USB type")},
    {"connector",i18n("Display connector")},{"active",i18n("Active")},{"bits_per_component",i18n("Bits per colour component")},
    {"current_slot",i18n("Running slot")},{"next_boot_slot",i18n("Next boot slot")},
    {"microphone",i18n("Microphone")},{"label",i18n("Label")},{"connected",i18n("Connected")}
  };
  QSet<QPair<QString,QString>> shown;
  std::function<void(const QJsonValue &,int)> walk=[&](const QJsonValue &v,int depth) {
    if(depth>6||rows.size()>=24)return;
    if(v.isObject()) {
      const auto o=v.toObject();
      for(auto it=o.begin();it!=o.end()&&rows.size()<24;++it) {
        if(labels.contains(it.key())&&(it.value().isNull()||it.value().isBool()||it.value().isDouble()||it.value().isString())) {
          const auto fact=qMakePair(labels.value(it.key()),value(it.value()));
          if(!shown.contains(fact)) {
            shown.insert(fact);
            rows << row(fact.first,fact.second);
          }
        }
        else if(it.value().isObject()||it.value().isArray()) walk(it.value(),depth+1);
      }
    } else if(v.isArray()) {int count=0;for(const auto &item:v.toArray()) {if(++count>16)break;walk(item,depth+1);}}
  };
  walk(d,0);return rows;
}
Settings::Settings(Fixture f,QObject *parent,bool restricted):QObject(parent),fixture_(std::move(f)),restricted_(restricted),nfc_(fixture_.enabled,this,restricted),manager_(fixture_,this,restricted) {
  kernelTimeout_.setSingleShot(true);
  connect(&kernelTimeout_,&QTimer::timeout,this,[this]{if(kernel_)kernel_->kill();});
  poll_.setInterval(50);
  connect(&poll_,&QTimer::timeout,this,[this] {
    if(!query_)return;query_->poll();const auto r=query_->result();
    if(!r.running){poll_.stop();query_.reset();finish(r);}
  });
  simulated_.setSingleShot(true);
  connect(&simulated_,&QTimer::timeout,this,[this] {finish(fixture_.result(current_.toStdString()));});
  auto session=QDBusConnection::sessionBus();
  session.connect("org.kde.ScreenBrightness","/org/kde/ScreenBrightness","org.kde.ScreenBrightness","BrightnessChanged",
                  this,SLOT(screenBrightnessChanged(QString,int,QString,QString)));
  session.connect("org.kde.ScreenBrightness","/org/kde/ScreenBrightness","org.kde.ScreenBrightness","BrightnessRangeChanged",
                  this,SLOT(screenBrightnessRangeChanged(QString,int,int)));
}
Settings::~Settings(){stop();}
QVariantMap Settings::topic()const {
  if(!topic_)return {};
  auto map=topicMap(*topic_);
  bool admitted=true;
  if(topic_->id=="charging") {
    const auto a=data_.value("kernel.charging").value("admission").toObject();
    admitted=!busy()&&!errors_.contains("kernel.charging")&&(a.value("threshold_admission_ready").toBool()||a.value("automatic_admission_ready").toBool()||a.value("standard_client_admission_ready").toBool());
    const auto reason=a.value("reason").toString();
    map["blockedReason"]=reason=="control_profile_unqualified"?i18n("This kernel's charging profile has not been qualified for changes. Saved firmware thresholds can still be read. Installing a qualified provider is required before changing the limit."):i18n("Charging changes are not admitted. Check the provider information below and refresh; the charging helper retains its own checks.");
  }
  if(topic_->id=="kernel") {
    admitted=!busy()&&!errors_.contains("update.status")&&data_.value("update.status").value("complete").toBool();
    map["blockedReason"]=i18n("Boot metadata is incomplete. Slot changes require consistent recovery checks; the reported status remains visible.");
  }
  map["canApply"]=admitted;return map;
}
QVariantList Settings::topics(int area,const QString &search)const {
  QVariantList out;
  for(const auto &t:settingTopics()) {
    if(area>=0&&t.area!=area)continue;
    bool matches=true;
    for(const auto &word:search.simplified().split(' ',Qt::SkipEmptyParts)) {
      QString hay=t.id+" "+t.title+" "+t.subtitle;
      const QMap<QString,QString> aliases{{"battery","pil batarya güç geçmiş"},{"charging","pil batarya güç geçmiş sağlık şarj sınır politika uyku"},{"health","sağlık döngü kapasite"},{"kernel","çekirdek slot açılış"},{"updates","güncelleme yazılım kanal"},{"storage","depolama alan"},{"memory","bellek ram"},{"flashlight","fener ışık"},{"sound","ses mikrofon"},{"display","ekran parlaklık döndürme"}};
      hay+=' '+aliases.value(t.id);
      for(const auto &id:t.reads+t.changes)try{const auto &a=action(id.toStdString());hay+=' '+QString::fromStdString(a.id+" "+a.title.en+" "+a.title.tr+" "+a.description.en+" "+a.description.tr);}catch(const std::exception &){}
      if(!hay.contains(word,Qt::CaseInsensitive))matches=false;
    }
    if(matches)out<<topicMap(t);
  }
  return out;
}
QString Settings::summary(int area)const {
  for(const auto &t:settingTopics())if(t.area==area)
    for(const auto &id:t.reads)if(data_.contains(id)&&!errors_.contains(id)) {
      const auto rows=informationRows(id,data_.value(id));
      if(!rows.isEmpty()) {
        const auto r=rows.first().toMap();
        if (r.value("primary").toBool())
          return i18n("%1: %2",r.value("title").toString(),r.value("value").toString());
      }
    }
  return {};
}
QVariantList Settings::information()const {
  QVariantList out;if(!topic_)return out;
  for(const auto &id:topic_->reads) {
    if(id=="update.status"&&!bootReport().isEmpty())continue;
    const bool kernel=id.startsWith("kernel.");
    const bool cached=data_.contains(id);
    const bool failed=errors_.contains(id);
    const bool loading=current_==id;
    QString title=topic_->title;
    if(id=="battery.cells")
      title=i18n("Battery cells");
    else if(id=="battery.status" && topic_->id=="charging")
      title=i18n("Battery status");
    else if(!kernel) {
      try { title=friendlyTitle(action(id.toStdString())); } catch(const std::exception &) {}
    }
    QVariantList sections;
    if(cached && kernel)
      sections=kernelInformationSections(id,data_.value(id));
    if(sections.isEmpty()) {
      auto rows=cached && !kernel ? informationRows(id,data_.value(id)) : QVariantList{};
      QVariantMap card{{"key",id},{"title",title},{"rows",rows}};
      if(rows.isEmpty()) {
        card["message"]=loading?i18n("Reading…"):failed?errors_.value(id):
                        queue_.contains(id)?i18n("Waiting to read…"):cached?i18n("No summary available. Open Technical Tools for the original output."):i18n("Not checked");
      }
      sections<<card;
    }
    for(const auto &section:sections) {
      auto card=section.toMap();
      card["id"]=id;
      card["stamp"]=stamps_.value(id);
      card["stale"]=failed && cached && !card.value("rows").toList().isEmpty();
      card["failed"]=failed;
      card["loading"]=loading && !cached;
      out<<card;
    }
  }
  return out;
}
QVariantMap Settings::bootReport()const {
  if(!topic_||topic_->id!="kernel")return {};
  const auto d=data_.value("update.status");
  if(d.value("schema").toString()!="org.uke.control.update-status.v1"||!d.value("available").toBool())
    return {};
  auto report=d.toVariantMap();
  report["stale"]=errors_.contains("update.status");
  report["stamp"]=stamps_.value("update.status");
  return report;
}
QVariantMap Settings::chargingThresholds() const {
  const auto data=data_.value("kernel.charging");
  const auto saved=data.value("settings").toObject();
  const auto admission=data.value("admission").toObject();
  const int start=saved.value("reported_start_percent").toInt(-1);
  const int stop=saved.value("reported_end_percent").toInt(-1);
  const bool valid=saved.value("firmware_get_complete").toBool() &&
      saved.value("exit_status").toInt(-1)==0 && start>=0 && start<stop && stop<=100;
  const bool stale=errors_.contains("kernel.charging") && valid;
  return {
      {"available",valid}, {"start",start}, {"stop",stop},
      {"enabled",saved.value("reported_enabled").toVariant()},
      {"stale",stale}, {"stamp",stamps_.value("kernel.charging")},
      {"profileUnqualified",admission.value("reason").toString()=="control_profile_unqualified"},
      {"admitted",valid && !stale && !busy() && admission.value("threshold_admission_ready").toBool()}
  };
}
QVariantList Settings::chargingBands() const {
  static const QRegularExpression bandId("^hold([0-9]{2})-([0-9]{2,3})$");
  QVariantList bands;
  for(const auto &choice:actionChoices("charge.mode","mode")) {
    const auto name=choice.toMap().value("value").toString();
    const auto match=bandId.match(name);
    if(!match.hasMatch())continue;
    const int start=match.captured(1).toInt();
    const int stop=match.captured(2).toInt();
    if(start<stop && stop<=100)
      bands<<QVariantMap{{"mode",name},{"start",start},{"stop",stop}};
  }
  std::sort(bands.begin(),bands.end(),[](const QVariant &left,const QVariant &right) {
    return left.toMap().value("start").toInt()<right.toMap().value("start").toInt();
  });
  return bands;
}
QString Settings::currentChannel() const {
  const auto data=data_.value("channel.status");
  if(data.value("schema").toString()!="org.uke.control.channel.v1" || errors_.contains("channel.status"))
    return {};
  return data.value("selected").toString();
}
QVariantList Settings::actionChoices(const QString &id,const QString &key) const {
  QVariantList choices;
  const bool turkish=QLocale().language()==QLocale::Turkish;
  try {
    for(const auto &parameter:action(id.toStdString()).parameters) {
      if(QString::fromStdString(parameter.id)!=key)continue;
      for(const auto &choice:parameter.choices)
        choices<<QVariantMap{{"title",QString::fromStdString(turkish?choice.title.tr:choice.title.en)},
                            {"value",QString::fromStdString(choice.value)}};
    }
  } catch(const std::exception &) {
    return {};
  }
  return choices;
}
QVariantMap Settings::flashlight() const {
  const auto data=data_.value("flashlight.status");
  const auto torch=data.value("flashlight").toObject();
  const bool available=data.value("schema").toString()=="org.uke.control.lights.v1" &&
      torch.value("available").toBool() && torch.value("on").isBool() &&
      !errors_.contains("flashlight.status");
  const int requested=torch.value("requested_percent").toInt(-1);
  const int step=torch.value("brightness_step_percent").toInt();
  return {
      {"available",available}, {"on",available && torch.value("on").toBool()},
      {"level",requested>=1 && requested<=100 && step==50 ? (requested==100 ? 100 : 50) : 0},
      {"step",step==50?50:0}, {"pending",flashlightPending_}, {"error",flashlightError_}
  };
}
void Settings::setFlashlight(bool enabled) {
  requestFlashlight("SetEnabled",enabled);
}
void Settings::setFlashlightBrightness(int percent) {
  if((percent==50 || percent==100) && flashlight().value("step").toInt()==50)
    requestFlashlight("SetBrightness",static_cast<uint>(percent));
}
void Settings::requestFlashlight(const QString &method,const QVariant &argument) {
  const QString id="flashlight.status";
  if(fixture_.enabled || restricted_ || !topic_ || topic_->id!="flashlight" || busy() ||
     flashlightPending_ || !flashlight().value("available").toBool())
    return;
  auto message=QDBusMessage::createMethodCall("org.senemos.Uke.Torch","/org/senemos/Uke/Torch",
                                               "org.senemos.Uke.Torch1",method);
  message.setArguments({argument});
  message.setInteractiveAuthorizationAllowed(false);
  flashlightError_.clear();
  flashlightPending_=true;
  emit changed();
  auto *watcher=new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message,3000),this);
  connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher,id] {
    const QDBusPendingReply<QString> reply=*watcher;
    QJsonObject result;
    if(!reply.isError() && reply.value().size()<=65536)
      result=QJsonDocument::fromJson(reply.value().toUtf8()).object();
    if(reply.error().type()==QDBusError::AccessDenied) {
      flashlightError_=i18n("Flashlight controls require an active local session.");
    } else if(result.value("schema").toString()=="org.uke.control.lights.v1" &&
              result.value("flashlight").isObject()) {
      if(current_==id)stop();
      data_[id]=result;
      errors_.remove(id);
      updated_=QDateTime::currentDateTime().toString(QLocale().timeFormat(QLocale::ShortFormat));
      stamps_[id]=updated_;
    } else {
      flashlightError_=i18n("The flashlight service did not confirm the change. Refresh to check its state.");
      errors_[id]=flashlightError_;
    }
    flashlightPending_=false;
    watcher->deleteLater();
    emit changed();
  });
}
QVariantMap Settings::displayLink() const {
  const QString id="display.profiles";
  const auto data=data_.value(id);
  if(data.value("schema").toString()!="org.uke.control.display.profiles.v1")
    return {};
  const auto link=data.value("observed_link").toObject();
  const auto ceiling=data.value("training_ceiling").toObject();
  const int rate=link.value("rate_khz").toInt();
  const int lanes=link.value("lanes").toInt();
  const int bpp=link.value("bpp").toInt();
  const auto dsc=data_.value("display.compression");
  const QMap<int,QString> rates{{162000,"RBR"},{270000,"HBR"},{540000,"HBR2"},{810000,"HBR3"}};
  return {
      {"rate",rates.value(rate)}, {"lanes",lanes}, {"bitsPerPixel",bpp},
      {"ceiling",ceiling.value("current").toString()}, {"canSetLanes",ceiling.value("available").toBool()},
      {"canSetCompression",dsc.value("control_available").toBool() && !errors_.contains("display.compression")},
      {"stale",errors_.contains(id)}
  };
}
QVariantList Settings::linuxPartitions() const {
  const QString id="overview.storage";
  const auto snapshot=data_.value(id);
  const auto inventory=component(snapshot).value("blockdevices").toArray();
  static const QRegularExpression diskName("^[a-zA-Z0-9][a-zA-Z0-9_.-]{0,79}$");
  QVariantList mounted, others;
  const auto hasMount=[](const QJsonObject &node,const QString &path,const auto &visit)->bool {
    if(node.value("mountpoints").toArray().contains(path))
      return true;
    for(const auto &child:node.value("children").toArray())
      if(visit(child.toObject(),path,visit))
        return true;
    return false;
  };
  bool usesUserdata=false;
  for(const auto &drive:inventory)
    for(const auto &item:drive.toObject().value("children").toArray()) {
      const auto part=item.toObject();
      if(part.value("type").toString()=="part" && part.value("partlabel").toString()=="userdata" && hasMount(part,"/",hasMount))
        usesUserdata=true;
    }
  const QMap<QString,QString> linuxLabels{{"linux",i18n("Linux root volume")},{"linux2",i18n("Second Linux volume")},
      {"linux_boot",i18n("Linux boot volume")},{"linux_swap",i18n("Linux swap")},{"esp",i18n("EFI system partition")}};
  for(const auto &drive:inventory)
    for(const auto &item:drive.toObject().value("children").toArray()) {
      const auto part=item.toObject();
      const auto name=part.value("name").toString();
      if(part.value("type").toString()!="part" || !diskName.match(name).hasMatch())
        continue;
      const auto label=part.value("partlabel").toString();
      const auto filesystem=part.value("fstype").toString();
      const bool root=hasMount(part,"/",hasMount);
      QString role;
      if(root)role=i18n("Running Linux system");
      else if(hasMount(part,"/boot/efi",hasMount))role=i18n("EFI system partition");
      else if(hasMount(part,"/boot",hasMount))role=i18n("Linux boot volume");
      else if(usesUserdata && (label=="boot_a" || label=="boot_b"))
        role=label=="boot_a"?i18n("Boot slot A"):i18n("Boot slot B");
      else if(!filesystem.isEmpty())role=linuxLabels.value(label);
      if(role.isEmpty())continue;
      const auto size=part.value("size").toDouble();
      const bool encrypted=filesystem=="crypto_LUKS";
      (root?mounted:others)<<QVariantMap{
          {"title",role}, {"device","/dev/"+name},
          {"filesystem",encrypted?QStringLiteral("LUKS"):safe(filesystem.left(24).toStdString())},
          {"size",size>0 && std::isfinite(size)?bytes(size):QString()},
          {"root",root}, {"encrypted",encrypted}, {"stale",errors_.contains(id)}
      };
    }
  return mounted+others;
}
bool Settings::open(const QString &id) {
  const auto name=QStringList{"battery","health","charger","sleep"}.contains(id)?QStringLiteral("charging"):
      id=="external"?QStringLiteral("usb"):id=="display-expert"?QStringLiteral("display"):id;
  close();for(const auto &t:settingTopics())if(t.id==name){
    topic_=&t;emit changed();refresh();
    if(name=="display") readScreenBrightness();
    return true;
  }return false;
}
void Settings::refresh() {
  stop();notice_.clear();flashlightError_.clear();if(!topic_)return;
  queue_=topic_->reads;
  if(topic_->id=="about"){
    if(deviceBoard_.isEmpty() && !fixture_.enabled)
      deviceBoard_={platformBoardInformation(),platformComponentInformation(),platformSensorInformation()};
    queue_.removeIf([this](const QString &id){return data_.contains(id);});
  }
  next();
  if(QStringList{"cpu","gpu","sensors","manager-overview","memory","storage","thermal","profiles","diagnostics"}.contains(topic_->id))manager_.open(topic_->id);
}
void Settings::stop() {
  manager_.pause();
  if(!current_.isEmpty())errors_[current_]=i18n("Read stopped. Use Refresh to try again.");
  poll_.stop();simulated_.stop();queue_.clear();
  kernelTimeout_.stop();if(kernel_){
    auto *p=kernel_;kernel_=nullptr;p->disconnect(this);
    // pkexec may already have dropped the caller's kill permission. Let the
    // bounded read finish without blocking the GUI in QProcess's destructor.
    connect(p,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),p,&QObject::deleteLater);
    p->kill();if(p->state()==QProcess::NotRunning)p->deleteLater();
  }
  if(query_){query_->cancel();query_.reset();}current_.clear();
  ++historyGeneration_;if(history_.value("busy").toBool()){history_["busy"]=false;history_["message"]=i18n("History read stopped");emit historyChanged();}
  emit changed();
}
void Settings::close(){stopLedPreview();stop();manager_.close();nfc_.clear();++brightnessGeneration_;screenBrightnessId_.clear();brightness_=-1;brightnessMaximum_=0;topic_=nullptr;notice_.clear();updated_.clear();emit changed();}
void Settings::next() {
  if(!current_.isEmpty())return;
  if(!topic_||queue_.isEmpty()){current_.clear();emit changed();return;}
  current_=queue_.takeFirst();emit changed();
  try {
    if(current_.startsWith("kernel.")) {
      if(fixture_.enabled)simulated_.start(fixture_.delay(current_.toStdString()));
      else readKernel(current_.mid(7));
      return;
    }
    const auto p=plan(current_.toStdString(),{});
    if(p.effect!=Effect::read||p.presentation!=Presentation::capture||!p.values.empty())throw std::runtime_error("This observation requires explicit options or terminal access");
    if(fixture_.enabled)simulated_.start(fixture_.delay(current_.toStdString()));
    else{query_=std::make_unique<Query>(current_.toStdString());poll_.start();}
  } catch(const std::exception &e){Result r;r.action_id=current_.toStdString();r.exit_status=69;r.error=e.what();finish(r);}
}
void Settings::finish(const Result &r) {
  if(current_.isEmpty()||QString::fromStdString(r.action_id)!=current_)return;
  const auto id=current_;
  QJsonParseError e;const auto document=QJsonDocument::fromJson(QByteArray::fromStdString(r.output),&e);
  const auto d=document.object();
  if(r.exit_status||r.cancelled||r.timed_out||r.truncated||e.error!=QJsonParseError::NoError||!document.isObject()) {
    errors_[id]=i18n("The provider could not return usable information. Refresh to try again.");
    notice_=errors_.value(id);
  } else {
    data_[id]=id.startsWith("kernel.")&&d.value("schema").toString()=="org.uke.qt.kernel-information.v1"?d.value("data").toObject():d;errors_.remove(id);updated_=QDateTime::currentDateTime().toString(QLocale().timeFormat(QLocale::ShortFormat));stamps_[id]=updated_;
  }
  current_.clear();emit changed();QTimer::singleShot(0,this,&Settings::next);
}
void Settings::readKernel(const QString &kind) {
  auto *p=new QProcess(this);kernel_=p;kernelOutput_.clear();
  const auto id=current_;
  connect(p,&QProcess::readyReadStandardOutput,this,[this,p]{
    kernelOutput_+=p->readAllStandardOutput();if(kernelOutput_.size()>131072)p->kill();
  });
  connect(p,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,p,id](int code,QProcess::ExitStatus status){
    if(kernel_!=p)return;kernelTimeout_.stop();kernelOutput_+=p->readAllStandardOutput();
    Result r;r.action_id=id.toStdString();r.exit_status=status==QProcess::NormalExit?code:70;
    r.truncated=kernelOutput_.size()>131072;r.output=kernelOutput_.left(131072).toStdString();
    kernel_=nullptr;p->deleteLater();finish(r);
  });
  connect(p,&QProcess::errorOccurred,this,[this,p,id](QProcess::ProcessError error){
    if(error!=QProcess::FailedToStart||kernel_!=p)return;kernelTimeout_.stop();kernel_=nullptr;p->deleteLater();Result r;r.action_id=id.toStdString();r.exit_status=69;finish(r);
  });
  const QString helper="/usr/libexec/uke-control-qt/kernel-information";
  if(kind=="charging"||kind=="tag")p->start("/usr/bin/pkexec",{helper,kind});
  else p->start(helper,{kind});
  kernelTimeout_.start(20000);
}
QVariantMap Settings::usbPolicy()const {
  const auto d=data_.value("kernel.dock");
  QString currentRole;
  const auto ports=d.value("ports").toArray();
  if(ports.size()==1){const auto match=QRegularExpression("\\[([^]]+)\\]").match(ports.first().toObject().value("data_role").toString());if(match.hasMatch())currentRole=match.captured(1);}
  return {{"known",data_.contains("kernel.dock")&&!errors_.contains("kernel.dock")},
          {"role",currentRole},
          {"automatic",d.value("kernel_managed").toBool()},
          {"message",i18n("Automatic connection handling belongs to the kernel Type-C controller. Host and Device request a role for the attached partner; they do not save a forced mode. Reconnecting negotiates the role again.")}};
}
QVariantList Settings::choices(const QString &kind)const {
  QVariantList out;
  if(fixture_.enabled)return out;
  if(kind=="interface") {
    for(const auto &iface:QNetworkInterface::allInterfaces())if(!(iface.flags()&QNetworkInterface::IsLoopBack))out<<QVariantMap{{"title",iface.humanReadableName()},{"value",iface.name()}};
  } else if(kind=="usb_device") {
    for(const auto &name:QDir("/sys/bus/usb/devices").entryList(QDir::Dirs|QDir::NoDotAndDotDot))if(QRegularExpression("^[0-9]+-[0-9]+(?:\\.[0-9]+)*$").match(name).hasMatch()) {
      QFile f("/sys/bus/usb/devices/"+name+"/product");QString label=name;if(f.open(QIODevice::ReadOnly))label=QString::fromUtf8(f.read(256)).trimmed()+" ("+name+")";
      out<<QVariantMap{{"title",safe(label.toStdString())},{"value",name}};
    }
  } else if(kind=="input_device") {
    for(const auto &name:QDir("/dev/input").entryList({"event*"},QDir::System|QDir::Files))out<<QVariantMap{{"title",name},{"value","/dev/input/"+name}};
  }
  return out;
}
QString Settings::localPath(const QUrl &url)const {return url.isLocalFile()?url.toLocalFile():url.scheme().isEmpty()&&url.path().startsWith('/')&&url.authority().isEmpty()?QDir::cleanPath(url.path()):QString();}
QVariantList Settings::nativeLinks(int area)const {
  if(fixture_.enabled)return {};
  QVariantList out;
  const QList<QVariantMap> entries{
    {{"id","kcm_powerdevilprofilesconfig"},{"title",i18n("KDE energy saving settings")},{"area",1}},
    {{"id","kcm_screenlocker"},{"title",i18n("KDE screen locking settings")},{"area",2}},
    {{"id","kcm_kscreen"},{"title",i18n("KDE display settings")},{"area",2}},
    {{"id","kcm_pulseaudio"},{"title",i18n("KDE sound settings")},{"area",4}},
    {{"id","kcm_networkmanagement"},{"title",i18n("KDE network settings")},{"area",3}},
    {{"id","kcm_bluetooth"},{"title",i18n("KDE Bluetooth settings")},{"area",3}}};
  for(const auto &e:entries)if(e.value("area").toInt()==area&&QFile::exists("/usr/bin/systemsettings")&&QFile::exists(QLibraryInfo::path(QLibraryInfo::PluginsPath)+"/plasma/kcms/systemsettings/"+e.value("id").toString()+".so"))out<<e;
  return out;
}
bool Settings::openNative(const QString &id) {
  if(fixture_.enabled||restricted_)return false;
  for(int area:{1,2,3,4})for(const auto &entry:nativeLinks(area))if(entry.toMap().value("id").toString()==id)
    return QProcess::startDetached("/usr/bin/systemsettings",{id});
  return false;
}
} // namespace uq
