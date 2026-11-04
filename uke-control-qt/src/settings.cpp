// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QLibraryInfo>
#include <QProcess>
#include <cmath>
namespace uq {
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
QVariantMap row(const QString &name,const QString &text,double progress=-1) {
  return {{"title",name},{"value",text},{"progress",progress}};
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
    auto measure=[&](const char *key,const QString &title,const char *unit,double scale,const QString &suffix,bool level=false) {
      const auto r=readings.value(key).toObject();const auto v=r.value("value");
      const auto validity=r.value("validity").toString();
      const bool valid=coherent && validity!="stale" && validity!="invalid" && validity!="unavailable" && r.value("errno").toInt(-1)==0 && r.value("unit").toString()==unit && v.isDouble() && std::isfinite(v.toDouble());
      const auto n=v.toDouble()/scale;
      rows << row(title,valid?QLocale().toString(n,'f',scale==1?0:1)+suffix:unknown(),valid&&level&&n>=0&&n<=100?n/100:-1);
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
    rows<<row(i18n("Available"),value(f.value("available")));
    rows<<row(i18n("Flashlight"),f.value("on").isBool()?(f.value("on").toBool()?i18n("On"):i18n("Off")):unknown());
    rows<<row(i18n("Applied brightness"),f.value("brightness_percent").isDouble()?value(f.value("brightness_percent"))+" %":unknown());
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
  std::function<void(const QJsonValue &,int)> walk=[&](const QJsonValue &v,int depth) {
    if(depth>6||rows.size()>=24)return;
    if(v.isObject()) {
      const auto o=v.toObject();
      for(auto it=o.begin();it!=o.end()&&rows.size()<24;++it) {
        if(labels.contains(it.key())&&(it.value().isNull()||it.value().isBool()||it.value().isDouble()||it.value().isString()))
          rows << row(labels.value(it.key()),value(it.value()));
        else if(it.value().isObject()||it.value().isArray()) walk(it.value(),depth+1);
      }
    } else if(v.isArray()) {int count=0;for(const auto &item:v.toArray()) {if(++count>16)break;walk(item,depth+1);}}
  };
  walk(d,0);return rows;
}
Settings::Settings(Fixture f,QObject *parent,bool restricted):QObject(parent),fixture_(std::move(f)),restricted_(restricted),nfc_(fixture_.enabled,this) {
  kernelTimeout_.setSingleShot(true);
  connect(&kernelTimeout_,&QTimer::timeout,this,[this]{if(kernel_)kernel_->kill();});
  poll_.setInterval(50);
  connect(&poll_,&QTimer::timeout,this,[this] {
    if(!query_)return;query_->poll();const auto r=query_->result();
    if(!r.running){poll_.stop();query_.reset();finish(r);}
  });
  simulated_.setSingleShot(true);
  connect(&simulated_,&QTimer::timeout,this,[this] {finish(fixture_.result(current_.toStdString()));});
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
      const QMap<QString,QString> aliases{{"battery","pil batarya güç geçmiş"},{"charging","şarj sınır politika"},{"health","sağlık döngü kapasite"},{"kernel","çekirdek slot açılış"},{"updates","güncelleme yazılım kanal"},{"storage","depolama alan"},{"memory","bellek ram"},{"flashlight","fener ışık"},{"sound","ses mikrofon"},{"display","ekran parlaklık döndürme"}};
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
    for(const auto &id:t.reads)if(data_.contains(id)) {
      const auto rows=informationRows(id,data_.value(id));
      if(!rows.isEmpty()) {const auto r=rows.first().toMap();return r.value("title").toString()+": "+r.value("value").toString()+" · "+i18n("Read at %1",stamps_.value(id));}
    }
  return {};
}
QVariantList Settings::information()const {
  QVariantList out;if(!topic_)return out;
  for(const auto &id:topic_->reads) {
    if(id.startsWith("kernel.")) {
      if(data_.contains(id)) {
        auto sections=kernelInformationSections(id,data_.value(id));
        for(auto &section:sections){auto s=section.toMap();s["id"]=id;s["stale"]=errors_.contains(id);s["stamp"]=stamps_.value(id);out<<s;}
      } else out<<QVariantMap{{"title",i18n("Kernel information")},{"rows",QVariantList{row(i18n("Status"),errors_.contains(id)?errors_.value(id):i18n("Reading…"))}}};
      continue;
    }
    if(id=="update.status"&&!bootReport().isEmpty())continue;
    QString title=id;try{title=friendlyTitle(action(id.toStdString()));}catch(const std::exception &){}
    auto rows=informationRows(id,data_.value(id));
    if(rows.isEmpty())rows<<row(i18n("Information"),errors_.contains(id)?i18n("Unavailable"):current_==id?i18n("Reading…"):data_.contains(id)?i18n("No supported summary fields were reported. Technical tools contain the original output."):i18n("Not checked"));
    out<<QVariantMap{{"id",id},{"title",title},{"rows",rows},{"error",errors_.value(id)},
                    {"stale",errors_.contains(id)&&data_.contains(id)},{"stamp",stamps_.value(id)}};
  }
  return out;
}
QVariantMap Settings::bootReport()const {
  if(!topic_||topic_->id!="kernel")return {};
  const auto d=data_.value("update.status");
  return d.value("schema").toString()=="org.uke.control.update-status.v1"&&d.value("available").toBool()?d.toVariantMap():QVariantMap{};
}
bool Settings::open(const QString &id) {
  close();for(const auto &t:settingTopics())if(t.id==id){topic_=&t;emit changed();refresh();return true;}return false;
}
void Settings::refresh() {
  stop();notice_.clear();if(!topic_)return;
  queue_=topic_->reads;next();
}
void Settings::stop() {
  if(!current_.isEmpty())errors_[current_]=i18n("Read stopped; previous information may be out of date");
  poll_.stop();simulated_.stop();queue_.clear();
  kernelTimeout_.stop();if(kernel_){auto *p=kernel_;kernel_=nullptr;p->disconnect(this);p->kill();p->deleteLater();}
  if(query_){query_->cancel();query_.reset();}current_.clear();
  ++historyGeneration_;if(history_.value("busy").toBool()){history_["busy"]=false;history_["message"]=i18n("History read stopped");emit historyChanged();}
  emit changed();
}
void Settings::close(){stop();nfc_.clear();topic_=nullptr;notice_.clear();updated_.clear();emit changed();}
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
  return {{"known",data_.contains("kernel.dock")&&!errors_.contains("kernel.dock")},
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
